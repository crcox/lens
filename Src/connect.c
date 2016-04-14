#include "system.h"
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include "util.h"
#include "type.h"
#include "network.h"
#include "connect.h"

#ifndef DOUBLE_REAL
#define FLOAT_REAL
#endif // DOUBLE_REAL

char **LinkTypeName;
#define MAX_NAME_LENGTH  128    /* Names for Xerion groups */


/********************************* Link Types ********************************/

void initLinkTypes(void) {
  LinkTypeName = (char **) safeCalloc(LINK_NUM_TYPES, sizeof(char *),
				      "LinkTypeName");
}

mask lookupLinkType(char *typeName) {
  mask i;
  for (i = 0; i < LINK_NUM_TYPES; i++)
    if (LinkTypeName[i] && !strcmp(LinkTypeName[i], typeName))
      return i;
  warning("Unknown link type: \"%s\"", typeName);
  return ALL_LINKS;
}

flag registerLinkType(char *typeName, mask *type) {
  mask i;
  for (i = 1; i < LINK_NUM_TYPES; i++)
    if (LinkTypeName[i] && !strcmp(LinkTypeName[i], typeName)) {
      *type = i;
      return TCL_OK;
    }
  for (i = 1; i < LINK_NUM_TYPES; i++)
    if (LinkTypeName[i] == NULL) {
      LinkTypeName[i] = copyString(typeName);
      *type = i;
      return TCL_OK;
    }
  *type = 0;
  return warning("Too many link types registered, could not add %s", typeName);
}

flag unregisterLinkType(char *typeName) {
  mask i;
  for (i = 0; i < LINK_NUM_TYPES; i++)
    if (LinkTypeName[i] && !strcmp(LinkTypeName[i], typeName)) {
      FREE(LinkTypeName[i]);
      return TCL_OK;
    }
  return warning("Link type \"%s\" is not registered", typeName);
}

flag listLinkTypes(void) {
  mask i;
  for (i = 0; i < LINK_NUM_TYPES; i++)
    if (LinkTypeName[i]) append("\"%s\"\n", LinkTypeName[i]);
  return TCL_OK;
}

/* If there are multiple links of the same type between the units, this only
   returns the first. */
Link lookupLink(Unit preUnit, Unit postUnit, int *num, mask linkType) {
  int b, l;
  Block B;

  if (!preUnit || !postUnit) return NULL;
  for (b = 0, l = 0; b < postUnit->numBlocks; b++, l += B->numUnits) {
    B = postUnit->block + b;
    if (LINK_TYPE_MATCH(linkType, B) && IN_BLOCK(preUnit, B)) {
      if (num) *num = l + (preUnit - B->unit);
      return postUnit->incoming + l + (preUnit - B->unit);
    }
  }
  return NULL;
}

Block getLinkBlock(Unit U, int l, int *offset) {
  int n = 0;
  FOR_EACH_BLOCK(U, {
    if (B->numUnits + n > l) {
      *offset = l - n;
      return B;
    } else n += B->numUnits;
  });
  return NULL;
}

/* If there are multiple links of the same type between the units, this only
   returns the first. */
Unit lookupPreUnit(Unit postUnit, int num) {
  int offset;
  Block B;
  if (!(B = getLinkBlock(postUnit, num, &offset)))
    return NULL;
  return B->unit + offset;
}

int getLinkType(Unit U, Link L) {
  int b, l = (L - U->incoming);
  Block B;
  if (l < 0 || l >= U->numIncoming)
    return -1;
  for (b = 0; b < U->numBlocks; b++, l -= B->numUnits) {
    B = U->block + b;
    if (B->numUnits > l) return GET_LINK_TYPE(B);
  }
  return -1;
}

Link2 getLink2(Unit U, Link L) {
  return U->incoming2 + (L - U->incoming);
}

/***************************** Weight Randomization **************************/

static void randomizeLinkWeight(Link L, Block B, Group G) {
  real mean, range;
  mean = chooseValue3(B->randMean, G->randMean, Net->randMean);
  range = chooseValue3(B->randRange, G->randRange, Net->randRange);
  L->weight = randReal(mean, range);
}

void randomizeUnitWeights(Unit U, real range, real mean, real reqRange,
			  real reqMean, mask linkType, flag doFrozen) {
  int l = 0;
  Link L, sL;
  real m, r;

  if (!doFrozen && (U->type & FROZEN)) return;
  FOR_EACH_BLOCK(U, {
    if (LINK_TYPE_MATCH(linkType, B) && (doFrozen || !(B->type & FROZEN))) {
      m = chooseValue3(reqMean, B->randMean, mean);
      r = chooseValue3(reqRange, B->randRange, range);
      for (L = U->incoming + l, sL = L + B->numUnits; L < sL; L++)
	L->weight = randReal(m, r);
    }
    l += B->numUnits;
  });
}

void randomizeGroupWeights(Group G, real range, real mean, real reqRange,
			   real reqMean, mask linkType, flag doFrozen) {
  if (!doFrozen && (G->type & FROZEN)) return;
  mean = chooseValue3(reqMean, G->randMean, mean);
  range = chooseValue3(reqRange, G->randRange, range);
  FOR_EVERY_UNIT(G, randomizeUnitWeights(U, range, mean, reqRange, reqMean,
					linkType, doFrozen));
}

/* reqRange is the requested range.  If NaN, the default will be used. */
void randomizeWeights(real reqRange, real reqMean, mask linkType,
		      flag doFrozen) {
  real range, mean;
  if (!doFrozen && (Net->type & FROZEN)) return;
  mean = chooseValue(reqMean, Net->randMean);
  range = chooseValue(reqRange, Net->randRange);
  FOR_EACH_GROUP(randomizeGroupWeights(G, range, mean, reqRange, reqMean,
				       linkType, doFrozen));
}


/*************************** Setting Link Values *****************************/

void setUnitBlockValues(Unit U, flag ext, MemInfo M, char *value,
			mask linkType) {
  void (*setValue)(void *object, char *value) = M->info->setValue;
  if (ext) {
    FOR_EACH_BLOCK(U, {
      if (LINK_TYPE_MATCH((int) linkType, B) && B->ext)
	setValue((char *) B->ext + M->offset, value);
    });
  } else {
    FOR_EACH_BLOCK(U, {
      if (LINK_TYPE_MATCH((int) linkType, B))
	setValue((char *) B + M->offset, value);
    });
  }
}

void setGroupBlockValues(Group G, flag ext, MemInfo M, char *value,
			 mask linkType) {
  FOR_EVERY_UNIT(G, setUnitBlockValues(U, ext, M, value, linkType));
}

void setBlockValues(flag ext, MemInfo M, char *value, mask linkType) {
  FOR_EACH_GROUP(setGroupBlockValues(G, ext, M, value,
				     linkType));
//   FOR_EACH_GROUP(setGroupBlockValues(Net->group[g], ext, M, value,
//				     linkType));
}

static void initBlockValues(Block B, real mean, real range) {
  B->learningRate = DEF_B_learningRate;
  B->momentum     = DEF_B_momentum;
  B->weightDecay  = DEF_B_weightDecay;
  B->randMean     = chooseValue(mean, DEF_B_randMean);
  B->randRange    = chooseValue(range, DEF_B_randRange);
  B->min          = DEF_B_min;
  B->max          = DEF_B_max;
}

void initLinkValues(Link L, Link2 M) {
  L->deriv = DEF_L_deriv;
  M->lastWeightDelta = DEF_L_lastWeightDelta;
# ifdef ADVANCED
  /* This should be 1.0 for DBD.  It shouldn't affect quickProp. */
  M->lastValue = DEF_L_lastValue;
# endif
}


/**************************** Building Connections ***************************/

static flag growIncoming(Unit U, int change, int l) {
  U->numIncoming += change;
  U->incoming = safeRealloc(U->incoming,
			    U->numIncoming * sizeof(struct link),
			    "growIncoming:U->incoming");
  U->incoming2 = safeRealloc(U->incoming2,
			     U->numIncoming * sizeof(struct link2),
			     "growIncoming:U->incoming2");
  if (l != U->numIncoming - change) {
    memmove(U->incoming + l + change, U->incoming + l,
	    (U->numIncoming - l - change) * sizeof(struct link));
    memmove(U->incoming2 + l + change, U->incoming2 + l,
	    (U->numIncoming - l - change) * sizeof(struct link2));
  }
  return TCL_OK;
}

static flag shrinkIncoming(Unit U, int change, int l) {
  U->numIncoming -= change;
  if (l != U->numIncoming) {
    memmove(U->incoming + l, U->incoming + l + change,
	    (U->numIncoming - l) * sizeof(struct link));
    memmove(U->incoming2 + l, U->incoming2 + l + change,
	    (U->numIncoming - l) * sizeof(struct link2));
  }
  U->incoming = safeRealloc(U->incoming,
			    U->numIncoming * sizeof(struct link),
			    "shrinkIncoming:U->incoming");
  U->incoming2 = safeRealloc(U->incoming2,
			     U->numIncoming * sizeof(struct link2),
			     "shrinkIncoming:U->incoming2");
  return TCL_OK;
}

static flag growBlocks(Unit U, int b) {
  U->numBlocks++;
  U->block = safeRealloc(U->block, U->numBlocks * sizeof(struct block),
			 "growBlocks:U->block");
  if (b != U->numBlocks - 1)
    memmove(U->block + b + 1, U->block + b,
	    (U->numBlocks - b - 1) * sizeof(struct block));
  U->block[b].ext = NULL;
  return initBlockExtension(U->block + b);
}


static flag shrinkBlocks(Unit U, int b) {
  if (freeBlockExtension(U->block + b)) return TCL_ERROR;
  U->numBlocks--;
  if (b != U->numBlocks)
    memmove(U->block + b, U->block + b + 1,
	    (U->numBlocks - b) * sizeof(struct block));
  U->block = safeRealloc(U->block, U->numBlocks * sizeof(struct block),
			 "shrinkBlocks:U->block");
  return TCL_OK;
}

enum connectMode {NEW_BLOCK, PREPEND_BLOCK, APPEND_BLOCK, JOIN_BLOCKS};
flag connectUnits(Unit preUnit, Unit postUnit, mask linkType,
		  real range, real mean, flag frozen) {
  int b, l, mode;
  Link L;
  Block B = NULL;
  mode = NEW_BLOCK;
  /* Figure out where to add it */
  for (b = 0, l = 0; b < postUnit->numBlocks; b++, l += B->numUnits) {
    B = postUnit->block + b;
    /* Sorting them is problematic because the order will change if the user
       changes the group labels or something */
    if (GET_LINK_TYPE(B) != linkType) continue;
    /* If the unit is in an existing block, that's an error */
    if (IN_BLOCK(preUnit, B))
      return warning("connectUnits: %s is already connected to %s with a link "
		     "of type \"%s\"", preUnit->name, postUnit->name,
	     LinkTypeName[linkType]);
    /* If the unit is at the start of the next block */
    if (preUnit == (B->unit - 1)) {
      mode = PREPEND_BLOCK;
      break;
    }
    if (preUnit == (B->unit + B->numUnits)) {
      l += B->numUnits;
      if ((b + 1) < postUnit->numBlocks &&
	  preUnit == (postUnit->block[b + 1].unit - 1)) {
	mode = JOIN_BLOCKS;
	break;
      }
      else {
	mode = APPEND_BLOCK;
	break;
      }
    }
    if (preUnit > B->unit) {
      if ((b + 1) < postUnit->numBlocks &&
	  GET_LINK_TYPE(postUnit->block + b + 1) != linkType) {
        b++; l += B->numUnits; break;
      } else continue;
    }
    break;
  }

  switch (mode) {
  case NEW_BLOCK: /* l is the position of the new link */
    if (growBlocks(postUnit, b)) return TCL_ERROR;
    B = postUnit->block + b;
    B->numUnits = 1;
    B->unit = preUnit;
    B->groupUnits = preUnit->group->numUnits;
    B->output = preUnit->group->output + preUnit->num;
    B->type = 0;
    SET_LINK_TYPE(linkType, B);
    eval("catch {.initBlock group(%d).unit(%d).block(%d)}",
	 postUnit->group->num, postUnit->num, b);
    break;
  case PREPEND_BLOCK:
    B->unit--;
    B->output--;
    B->numUnits++;
    break;
  case APPEND_BLOCK:
    B->numUnits++;
    break;
  case JOIN_BLOCKS:
    B->numUnits += postUnit->block[b + 1].numUnits + 1;
    if (shrinkBlocks(postUnit, b + 1)) return TCL_ERROR;
    break;
  default:
    fatalError("bad mode (%d) in connectUnits");
  }

  if (growIncoming(postUnit, 1, l)) return TCL_ERROR;
  L = postUnit->incoming + l;

  initBlockValues(B, mean, range);
  randomizeLinkWeight(L, B, postUnit->group);
  initLinkValues(L, getLink2(postUnit, L));
  preUnit->numOutgoing++;
  postUnit->group->numIncoming++;
  preUnit->group->numOutgoing++;
  Net->numLinks++;

  if (frozen) freezeUnitInputs(postUnit, linkType);
  else thawUnitInputs(postUnit, linkType);

  return TCL_OK;
}

flag connectGroupToUnit(Group preGroup, Unit postUnit, mask linkType,
			real range, real mean, flag frozen) {
  int b, l, stop;
  Link L;
  Block B;
  /* Figure out where to add it */
  for (b = 0, l = 0; b < postUnit->numBlocks; b++, l += B->numUnits) {
    B = postUnit->block + b;
    /* Sorting them is problematic because the order will change if the user
       changes the group labels or something */
    if (GET_LINK_TYPE(B) != linkType) continue;
    /* If the block is from the group already, that's an error */
    if (IN_GROUP(B, preGroup))
      return warning("connectGroupToUnit: %s is already connected to %s with "
		     "a link of type \"%s\"", preGroup->name, postUnit->name,
		     LinkTypeName[linkType]);
    /*    if (preGroup->unit > B->unit)
	  break;*/
  }

  if (growBlocks(postUnit, b)) return TCL_ERROR;
  B = postUnit->block + b;
  B->numUnits = preGroup->numUnits;
  B->unit = preGroup->unit;
  B->groupUnits = preGroup->numUnits;
  B->output = preGroup->output;
  B->type = 0;
  SET_LINK_TYPE(linkType, B);
  initBlockValues(B, mean, range);
  eval("catch {.initBlock group(%d).unit(%d).block(%d)}",
       postUnit->group->num, postUnit->num, b);

  if (growIncoming(postUnit, preGroup->numUnits, l)) return TCL_ERROR;

  for (stop = l + preGroup->numUnits; l < stop; l++) {
    L = postUnit->incoming + l;
    randomizeLinkWeight(L, B, postUnit->group);
    initLinkValues(L, getLink2(postUnit, L));
    preGroup->unit[l - stop + preGroup->numUnits].numOutgoing++;
  }

  postUnit->group->numIncoming += preGroup->numUnits;
  preGroup->numOutgoing += preGroup->numUnits;
  Net->numLinks += preGroup->numUnits;

  if (frozen) freezeUnitInputs(postUnit, linkType);
  else thawUnitInputs(postUnit, linkType);

  return TCL_OK;
}

flag fullConnectGroups(Group preGroup, Group postGroup, mask linkType,
		       real strength, real range, real mean) {
  FOR_EVERY_UNIT(postGroup, {
    if (connectGroupToUnit(preGroup, U, linkType, range, mean, FALSE))
      return TCL_ERROR;});
  return TCL_OK;
}

static flag randomConnectGroups(Group preGroup, Group postGroup, mask linkType,
				real strength, real range, real mean) {
  Unit V;
  FOR_EVERY_UNIT(preGroup, {
    V = U;
    FOR_EVERY_UNIT(postGroup, {
      if (randProb() < strength)
	if (connectUnits(V, U, linkType, range, mean, FALSE)) return TCL_ERROR;
    });
  });
  return TCL_OK;
}

static flag fixedInConnectGroups(Group preGroup, Group postGroup,
				 mask linkType, real strength, real range,
				 real mean) {
  int u, v, inputs, *array;
  inputs = preGroup->numUnits * strength;
  array = intArray(preGroup->numUnits, "fixedInConnectGroups:array");
  for (u = 0; u < preGroup->numUnits; u++)
    array[u] = u;
  for (u = 0; u < postGroup->numUnits; u++) {
    randSort(array, preGroup->numUnits);
    for (v = 0; v < inputs; v++)
      if (connectUnits(preGroup->unit + array[v], postGroup->unit + u,
		       linkType, range, mean, FALSE)) return TCL_ERROR;
  }
  FREE(array);
  return TCL_OK;
}

static flag fixedOutConnectGroups(Group preGroup, Group postGroup,
				  mask linkType, real strength, real range,
				  real mean) {
  int u, v, outputs, *array;
  outputs = postGroup->numUnits * strength;
  array = intArray(postGroup->numUnits, "fixedInConnectGroups:array");
  for (u = 0; u < postGroup->numUnits; u++)
    array[u] = u;
  for (u = 0; u < preGroup->numUnits; u++) {
    randSort(array, postGroup->numUnits);
    for (v = 0; v < outputs; v++)
      if (connectUnits(preGroup->unit + u, postGroup->unit + array[v],
		       linkType, range, mean, FALSE)) return TCL_ERROR;
  }
  FREE(array);
  return TCL_OK;
}

/* This satisfies the constraints that each output will receive n inputs,
   where n is floor(from->numUnits * Strength).  In addition, each input will
   send either floor or ceil of (n * to->numUnits / from->numUnits).  Finally,
   no two units will be connected more than once.  */
static flag fairConnectGroups(Group preGroup, Group postGroup, mask linkType,
			      real strength, real range, real mean) {
  int i, j, k, n, s, *preArray, *postArray;
  n = preGroup->numUnits * strength;

  postArray = intArray(postGroup->numUnits, "fairConnectGroups:postArray");
  for (j = 0; j < postGroup->numUnits; j++)
    postArray[j] = j;
  randSort(postArray, postGroup->numUnits);

  preArray = intArray(preGroup->numUnits, "fairConnectGroups:preArray");
  for (i = 0; i < preGroup->numUnits; i++)
    preArray[i] = i;
  randSort(preArray, preGroup->numUnits);

  for (j = 0, i = 0; j < postGroup->numUnits; j++) {
    for (k = 0, s = i; k < n; k++) {
      if (i == preGroup->numUnits) {
        randSort(preArray, s);
        randSort(preArray + s, preGroup->numUnits - s);
        i = 0;
      }
      if (connectUnits(preGroup->unit + preArray[i++],
		       postGroup->unit + postArray[j], linkType,
		       range, mean, FALSE)) return TCL_ERROR;
    }
  }
  FREE(postArray);
  FREE(preArray);
  return TCL_OK;
}

/* This connects each preUnit to a set of topographically neighboring
   postUnits. */
static flag fanConnectGroups(Group preGroup, Group postGroup, mask linkType,
			     real strength, real range, real mean) {
  int i, j, n, k, start;
  n = postGroup->numUnits * strength;
  for (i = 0; i < preGroup->numUnits; i++) {
    start = ((i * postGroup->numUnits) / preGroup->numUnits) - (n / 2);
    for (j = start; j < start + n; j++) {
      if (j < 0) k = (j + postGroup->numUnits) % postGroup->numUnits;
      else k = j % postGroup->numUnits;
      if (connectUnits(preGroup->unit + i, postGroup->unit + k, linkType,
		       range, mean, FALSE)) return TCL_ERROR;
    }
  }
  return TCL_OK;
}

/* This may freeze links it has not created if they are of the same type. */
static flag oneToOneConnectGroups(Group preGroup, Group postGroup,
				  mask linkType, real strength, real range,
				  real mean) {
  int i, units;
  units = MIN(preGroup->numUnits, postGroup->numUnits);
  for (i = 0; i < units; i++) {
    if (connectUnits(preGroup->unit + i, postGroup->unit + i, linkType,
		     range, mean, TRUE)) return TCL_ERROR;
  }
  return TCL_OK;
}

void registerProjectionTypes(void) {
  registerProjectionType("FULL", FULL, fullConnectGroups);
  registerProjectionType("RANDOM", RANDOM, randomConnectGroups);
  registerProjectionType("FIXED_IN", FIXED_IN, fixedInConnectGroups);
  registerProjectionType("FIXED_OUT", FIXED_OUT, fixedOutConnectGroups);
  registerProjectionType("FAIR", FAIR, fairConnectGroups);
  registerProjectionType("FAN", FAN, fanConnectGroups);
  registerProjectionType("ONE_TO_ONE", ONE_TO_ONE, oneToOneConnectGroups);
}


/*********************************** Elman ***********************************/

flag elmanConnect(Group source, Group elman) {
  GroupProc P;
  flag set = FALSE;

  if (!(elman->outputType & ELMAN_CLAMP))
    return warning("Group \"%s\" has no ELMAN_CLAMP output procedures.",
		   elman->name);
  if (source->numUnits < elman->numUnits)
    return warning("Cannot elmanConnect groups \"%s\" and \"%s\".\n"
		   "The first has %d units and the second has %d units.",
		   source->name, elman->name,
		   source->numUnits, elman->numUnits);
  for (P = elman->outputProcs; P && !set; P = P->next) {
    if (P->type == ELMAN_CLAMP && !P->otherData) {
      P->otherData = (void *) source;
      set = TRUE;
    }
  }
  if (!set) return warning("Group \"%s\" has no empty ELMAN_CLAMP slots.",
			   elman->name);
  if (isNaN(elman->minOutput)) elman->minOutput = source->minOutput;
  else elman->minOutput += source->minOutput;
  if (isNaN(elman->maxOutput)) elman->maxOutput = source->maxOutput;
  else elman->maxOutput += source->maxOutput;
  elman->initOutput = source->initOutput;
  return TCL_OK;
}


/****************************** Removing Connections *************************/

/* If type is ALL_LINKS, removes all connections.
   Otherwise just removes all connections of the specified type. */
flag disconnectUnits(Unit preUnit, Unit postUnit, mask linkType) {
  int start = 0, l;
  Block R;

  FOR_EACH_BLOCK(postUnit, {
    l = (preUnit - B->unit) + start;
    start += B->numUnits;
    if (LINK_TYPE_MATCH(linkType, B) && IN_BLOCK(preUnit, B)) {
      postUnit->group->numIncoming--;
      preUnit->numOutgoing--;
      preUnit->group->numOutgoing--;
      Net->numLinks--;
      B->numUnits--;
      start--;
      if (B->numUnits == 0) {
	if (shrinkBlocks(postUnit, b)) return TCL_ERROR;
	b--; /* The next block is now at position b */
      }
      else if (preUnit == B->unit) {
	/* It is at the beginning of the block */
	B->unit++;
        B->output++;
      }
      else if (preUnit == (B->unit + B->numUnits)) {
	/* It is at the end of the block */
	/* Do nothing */
      }
      else {
	/* It is in the middle of the block so we have to split */
	if (growBlocks(postUnit, b)) return TCL_ERROR;
	B = postUnit->block + b;
	R = postUnit->block + b + 1;
	B->unit = R->unit;
        B->groupUnits = R->groupUnits;
	B->output = R->output;
	B->type = R->type;
	B->learningRate = R->learningRate;
	B->momentum = R->momentum;
	B->weightDecay = R->weightDecay;
	B->randMean = R->randMean;
        B->randRange = R->randRange;
	B->min = R->min;
	B->max = R->max;
	B->numUnits = preUnit - R->unit;
	R->numUnits -= B->numUnits;
	R->unit += B->numUnits + 1;
	R->output += B->numUnits + 1;
	eval("catch {.initBlock group(%d).unit(%d).block(%d)}",
	     postUnit->group->num, postUnit->num, b);
	if (copyBlockExtension(R, B)) return TCL_ERROR;
	b++; /* Skip over the R block on the next pass */
      }

      if (shrinkIncoming(postUnit, 1, l)) return TCL_ERROR;
    }
  });
  if (postUnit->numBlocks == 0) {
    postUnit->block = NULL;
    postUnit->incoming = NULL;
    postUnit->incoming2 = NULL;
  }
  return TCL_OK;
}

flag disconnectGroupFromUnit(Group preGroup, Unit postUnit, mask linkType) {
  int l = 0;
  Unit U, sU;

  FOR_EACH_BLOCK(postUnit, {
    if (LINK_TYPE_MATCH(linkType, B) && IN_GROUP(B, preGroup)) {
      postUnit->group->numIncoming -= B->numUnits;
      preGroup->numOutgoing -= B->numUnits;
      Net->numLinks -= B->numUnits;
      for (U = B->unit, sU = U + B->numUnits; U < sU; U++)
	U->numOutgoing--;

      /* Shift the links */
      if (shrinkIncoming(postUnit, B->numUnits, l)) return TCL_ERROR;
      if (shrinkBlocks(postUnit, b)) return TCL_ERROR;
      b--;
    } else l += B->numUnits;
  });
  if (postUnit->numBlocks == 0) {
    postUnit->block = NULL;
    postUnit->incoming = NULL;
    postUnit->incoming2 = NULL;
  }
  return TCL_OK;
}

flag disconnectGroups(Group preGroup, Group postGroup, mask linkType) {
  GroupProc P;
  FOR_EVERY_UNIT(postGroup, {
    if (disconnectGroupFromUnit(preGroup, U, linkType)) return TCL_ERROR;
  });
  if (linkType == ALL_LINKS && postGroup->outputType & ELMAN_CLAMP)
    for (P = postGroup->outputProcs; P; P = P->next) {
      if ((P->type & ELMAN_CLAMP) && ((Group) P->otherData == preGroup))
	P->otherData = NULL;
    }
  return TCL_OK;
}

flag deleteUnitInputs(Unit U, mask linkType) {
  Unit preUnit, sU;
  int l = 0;

  FOR_EACH_BLOCK(U, {
    if (LINK_TYPE_MATCH(linkType, B)) {
      U->group->numIncoming -= B->numUnits;
      B->unit->group->numOutgoing -= B->numUnits;
      Net->numLinks -= B->numUnits;
      for (preUnit = B->unit, sU = preUnit + B->numUnits; preUnit < sU;
	   preUnit++)
	preUnit->numOutgoing--;

      /* Shift the links */
      if (shrinkIncoming(U, B->numUnits, l)) return TCL_ERROR;
      if (shrinkBlocks(U, b)) return TCL_ERROR;
      b--;
    } else l += B->numUnits;
  });

  if (U->numBlocks == 0) {
    U->block = NULL;
    U->incoming = NULL;
    U->incoming2 = NULL;
  }
  return TCL_OK;
}

flag deleteGroupInputs(Group G, mask linkType) {
  GroupProc P;
  FOR_EVERY_UNIT(G, if (deleteUnitInputs(U, linkType)) return TCL_ERROR);
  if (linkType == ALL_LINKS && G->outputType & ELMAN_CLAMP)
    for (P = G->outputProcs; P; P = P->next) {
      if (P->type & ELMAN_CLAMP) P->otherData = NULL;
    }
  return TCL_OK;
}

flag deleteUnitOutputs(Unit V, mask linkType) {
  FOR_EACH_GROUP({
    FOR_EVERY_UNIT(G, {
      if (disconnectUnits(V, U, linkType)) return TCL_ERROR;
    });
  });
  return TCL_OK;
}

flag deleteGroupOutputs(Group H, mask linkType) {
  FOR_EACH_GROUP(if (disconnectGroups(H, G, linkType)) return TCL_ERROR);
  return TCL_OK;
}

flag deleteAllLinks(mask linkType) {
  FOR_EACH_GROUP(if (deleteGroupInputs(G, linkType)) return TCL_ERROR);
  return TCL_OK;
}


/****************************** Freezing and Thawing *************************/

void freezeUnitInputs(Unit U, mask linkType) {
  if (linkType == ALL_LINKS) U->type |= FROZEN;
  FOR_EACH_BLOCK(U, {
    if (LINK_TYPE_MATCH(linkType, B)) B->type |= FROZEN;
  });
}

void freezeGroupInputs(Group G, mask linkType) {
  if (linkType == ALL_LINKS) G->type |= FROZEN;
  FOR_EVERY_UNIT(G, freezeUnitInputs(U, linkType));
}

void freezeAllLinks(mask linkType) {
  if (linkType == ALL_LINKS) Net->type |= FROZEN;
  FOR_EACH_GROUP(freezeGroupInputs(G, linkType));
}

void thawUnitInputs(Unit U, mask linkType) {
  Net->type &= ~FROZEN;
  U->group->type &= ~FROZEN;
  U->type &= ~FROZEN;
  FOR_EACH_BLOCK(U, {
    if (LINK_TYPE_MATCH(linkType, B))
      B->type &= ~FROZEN;
  });
}

void thawGroupInputs(Group G, mask linkType) {
  FOR_EVERY_UNIT(G, thawUnitInputs(U, linkType));
}

void thawAllLinks(mask linkType) {
  FOR_EACH_GROUP(thawGroupInputs(G, linkType));
}


/******************************** Link Lesioning *****************************/

static void deleteLinks(Unit U, real prop, mask linkType) {
  int l = 0, offset;
  Block B;

  for (l = 0; l < U->numIncoming;) {
    B = getLinkBlock(U, l, &offset);
    if (!LINK_TYPE_MATCH(linkType, B)) l += B->numUnits;
    else if (randProb() < prop)
      disconnectUnits(B->unit + offset, U, linkType);
    else l++;
  }
}

static void setLinkWeights(Unit U, real prop, real value, mask linkType) {
  int l = 0, stop;
  FOR_EACH_BLOCK(U, {
    if (LINK_TYPE_MATCH(linkType, B)) {
      for (stop = l + B->numUnits; l < stop; l++)
	if (randProb() < prop)
	  U->incoming[l].weight = value;
    } else l += B->numUnits;
  });
}

static void injectLinkWeightNoise(Unit U, real prop, real range,
				  real (*noiseProc)(real value, real range),
				  mask linkType) {
  int l = 0, stop;
  FOR_EACH_BLOCK(U, {
    if (LINK_TYPE_MATCH(linkType, B)) {
      for (stop = l + B->numUnits; l < stop; l++)
	if (randProb() < prop)
	  U->incoming[l].weight = noiseProc(U->incoming[l].weight, range);
    } else l += B->numUnits;
  });
}

static void randomizeLinkWeights(Unit U, real prop, real range,
				 real (*noiseProc)(real value, real range),
				 mask linkType) {
  int l = 0, stop;
  FOR_EACH_BLOCK(U, {
    if (LINK_TYPE_MATCH(linkType, B)) {
      for (stop = l + B->numUnits; l < stop; l++)
	if (randProb() < prop)
	  U->incoming[l].weight = noiseProc(0, range);
    } else l += B->numUnits;
  });
}

flag lesionLinks(Group G, real proportion, int mode, real value,
		 real (*noiseProc)(real value, real range), mask linkType) {
  FOR_EVERY_UNIT(G, {
    switch (mode) {
    case 0:
      deleteLinks(U, proportion, linkType);
      break;
    case 1:
      setLinkWeights(U, proportion, value, linkType);
      break;
    case 2:
      injectLinkWeightNoise(U, proportion, value, noiseProc, linkType);
      break;
    case 3:
      randomizeLinkWeights(U, proportion, value, noiseProc, linkType);
      break;
    default:
      return warning("lesionLinks: bad mode: %d", mode);
    }
  });
  return TCL_OK;
}


/*********************************** Weight Files ****************************/

static int numThawedLinks(void) {
  int n = 0;
  if (Net->type & FROZEN) return 0;

  FOR_EACH_GROUP({
    if (!(G->type & FROZEN)) {
      FOR_EVERY_UNIT(G, {
	if (!(U->type & FROZEN)) {
	  FOR_EACH_BLOCK(U, {
	    if (!(B->type & FROZEN)) n += B->numUnits;
	  })}})}});
  return n;
}

static void printBinaryLinkData(Tcl_Channel channel, Link L, Link2 M,
				int numValues) {
  writeBinReal(channel, L->weight);
  if (numValues > 1)
    writeBinReal(channel, M->lastWeightDelta);
#ifdef ADVANCED
  if (numValues > 2)
    writeBinReal(channel, M->lastValue);
#endif /* ADVANCED */
}

static void printTextLinkData(Tcl_Channel channel, Link L, Link2 M,
			      int numValues) {
  writeReal(channel, L->weight, "", "");
  if (numValues > 1)
    writeReal(channel, M->lastWeightDelta, " ", "");
#ifdef ADVANCED
  if (numValues > 2)
    writeReal(channel, M->lastValue, " ", "");
#endif /* ADVANCED */
  cprintf(channel, "\n");
}

/* numValues will not be > 2 if not ADVANCED */
flag standardSaveWeights(Tcl_Obj *fileNameObj, flag binary, int numValues,
			 flag thawed, flag frozen)
{
  const char *fileName = Tcl_GetStringFromObj(fileNameObj, NULL);
  int numLinks = 0, thawedLinks, i, si, nCallsToPrintLinkData = 0;
  Tcl_Channel channel;
  void (*printLinkData)(Tcl_Channel, Link, Link2, int);

  if (!(channel = writeChannel(fileNameObj, FALSE)))
    return warning("standardSaveWeights: couldn't open the file \"%s\"",
		   fileName);

  thawedLinks = numThawedLinks();
  if (frozen) numLinks += Net->numLinks - thawedLinks;
  if (thawed) numLinks += thawedLinks;

  //print(1, "standardSaveWeights: numLinks  = %d\n", numLinks);
  //print(1, "standardSaveWeights: numvalues = %d\n", numValues);
  if (binary) {
    binaryEncoding(channel);
    writeBinInt(channel, BINARY_WEIGHT_COOKIE);
    writeBinInt(channel, numLinks);
    writeBinInt(channel, numValues);
    writeBinInt(channel, Net->totalUpdates);
    printLinkData = printBinaryLinkData;
  } else {
    cprintf(channel, "%d\n%d\n%d\n%d\n", BINARY_WEIGHT_COOKIE, numLinks,
	    numValues, Net->totalUpdates);
    printLinkData = printTextLinkData;
  }


  if (frozen || !(Net->type & FROZEN)) {
    FOR_EACH_GROUP({
      if (!frozen && (G->type & FROZEN)) continue;
      FOR_EVERY_UNIT(G, {
        if (!frozen && (U->type & FROZEN)) continue;
        i = 0;
        FOR_EACH_BLOCK(U, {
          if ((!frozen && (B->type & FROZEN)) ||
              (!thawed && (!((Net->type | G->type | U->type | B->type)
                 & FROZEN)))) i += B->numUnits;
          else for (si = i + B->numUnits; i < si; i++) {
            printLinkData(channel, U->incoming + i, U->incoming2 + i,
              numValues);
            nCallsToPrintLinkData++;
            }
	})})})}
  //print(1, "standardSaveWeights: nCallsPLD = %d\n", nCallsToPrintLinkData);

  closeChannel(channel);
  return TCL_OK;
}

static flag readBinaryLinkData(Tcl_Channel channel, Link L, Link2 M,
			       int numValues, ParseRec R) {
  real v=0 ;
  if (readBinReal(channel, &L->weight))
  return warning("standardLoadWeights: channel \"%s\" ended prematurely",
		 Tcl_GetStringFromObj(R->fileName, NULL));
  if (numValues > 1)
    readBinReal(channel, &M->lastWeightDelta);
  if (numValues > 2) {
    readBinReal(channel, &v);
#ifdef ADVANCED
    M->lastValue = v;
#endif // ADVANCED
  }
  return TCL_OK;
}

static flag readTextLinkData(Tcl_Channel channel, Link L, Link2 M,
			     int numValues, ParseRec R) {
  real v;
  if (readReal(R, &L->weight))
    return warning("standardLoadWeights: error reading weight on line "
		   "%d of\nfile \"%s\"", R->line, R->fileName);
  if (numValues > 1)
    readReal(R, &M->lastWeightDelta);
  if (numValues > 2)
    readReal(R, &v);
#ifdef ADVANCED
  M->lastValue = v;
#endif /* ADVANCED */
  return TCL_OK;
}

flag standardLoadWeights(Tcl_Obj *fileNameObj, flag thawed, flag frozen) {
  int i, si, numValues = 1, numLinks = 0, totalUpdates = -1, thawedLinks;
  const char *fileName = Tcl_GetStringFromObj(fileNameObj, NULL);
  flag result = TCL_OK;
  Tcl_Channel channel;
  struct parseRec rec;
  ParseRec R = &rec;
  flag (*readLinkData)(Tcl_Channel, Link, Link2, int, ParseRec);
  rec.buf = NULL;
  rec.fileName = fileNameObj;

  if (!(channel = readChannel(fileNameObj)))
    return warning("standardLoadWeights: couldn't open the file \"%s\"",
		   fileName);

  thawedLinks = numThawedLinks();
  if (frozen) numLinks += Net->numLinks - thawedLinks;
  if (thawed) numLinks += thawedLinks;

  if (readBinInt(channel, &i)) {
    result = warning("standardLoadWeights: file \"%s\" is empty", fileName);
    goto done;}

  if (i == BINARY_WEIGHT_COOKIE) {
    binaryEncoding(channel);
    print(1, "standardLoadWeights: binary\n");
    if (readBinInt(channel, &i)) {
      result = warning("standardLoadWeights: can't read numLinks");
      goto done;}
    if (i != numLinks) {
      result = warning("standardLoadWeights: network has %d links but the "
		       "file \"%s\" contains %d", numLinks, fileName, i);
      goto done;}

    if (readBinInt(channel, &numValues)) {
      result = warning("standardLoadWeights: can't read numValues");
      goto done;}
    if (numValues <= 0 || numValues > 3) {
      result = warning("standardLoadWeights: bad number of values (%d) in "
		       "file \"%s\"", numValues, fileName);
      goto done;
    }
    if (readBinInt(channel, &totalUpdates)) {
      result = warning("standardLoadWeights: can't read totalUpdates");
      goto done;}

    readLinkData = readBinaryLinkData;


  } else if (i == OLD_BINARY_WEIGHT_COOKIE) {
    if (readBinInt(channel, &i)) {
      result = warning("standardLoadWeights: can't read numLinks");
      goto done;}
    if (i != numLinks) {
      result = warning("standardLoadWeights: network has %d links but the "
		       "file \"%s\" contains %d", numLinks, fileName, i);
      goto done;}

    if (readBinInt(channel, &i)) {
      result = warning("standardLoadWeights: can't read sizeof(real)");
      goto done;}
    if (i % sizeof(real) != 0) {
      result = warning("standardLoadWeights: the file \"%s\" is encoded with "
		       "reals of size %d instead of %d", fileName, i,
		       sizeof(real));
      goto done;}
    numValues = i / sizeof(real);
    if (numValues <= 0 || numValues > 3) {
      result = warning("standardLoadWeights: bad number of values (%d) in "
		       "file \"%s\"", numValues, fileName);
      goto done;
    }
    readLinkData = readBinaryLinkData;


  } else {
    rec.channel = channel;
    rec.fileName = fileNameObj;
    rec.buf = newString(256);
    if (startParser(R, HTONL(i))) {
      result = warning("standardLoadWeights: error reading text header of "
		       "file \"%s\"", fileName);
      goto done;}
    if (readInt(R, &i)) {
      result = warning("standardLoadWeights: error reading text header of "
		       "file \"%s\"", fileName);
      goto done;}
    if (i == BINARY_WEIGHT_COOKIE) {
      if (readInt(R, &i)) {
	result = warning("standardLoadWeights: error reading text header of "
			 "file \"%s\"", fileName);
	goto done;}
      if (i != numLinks) {
	result = warning("standardLoadWeights: network has %d links but the "
			 "file \"%s\" contains %d", numLinks, fileName, i);
	goto done;}
      if (readInt(R, &numValues)) {
	result = warning("standardLoadWeights: error reading text header of "
			 "file \"%s\"", fileName);
	goto done;}
      if (numValues <= 0 || numValues > 3) {
	result = warning("standardLoadWeights: bad number of values (%d) in "
			 "file \"%s\"", numValues, fileName);
	goto done;
      }
      if (readInt(R, &totalUpdates)) {
	result = warning("standardLoadWeights: error reading text header of "
			 "file \"%s\"", fileName);
	goto done;}
    } else {
      if (i < 0) {
	numValues = -i;
	if (readInt(R, &i)) {
	  result = warning("standardLoadWeights: error reading text header of "
			   "file \"%s\"", fileName);
	  goto done;}
      }

      if (i != numLinks) {
	result = warning("standardLoadWeights: network has %d links but the "
			 "file \"%s\" contains %d", numLinks, fileName, i);
	goto done;}
    }
    readLinkData = readTextLinkData;
  }

  int jjj = 0;
  if (frozen || !(Net->type & FROZEN)) {
    FOR_EACH_GROUP({
      if (!frozen && (G->type & FROZEN)) continue;
      FOR_EVERY_UNIT(G, {
        if (!frozen && (U->type & FROZEN)) continue;
        i = 0;
        FOR_EACH_BLOCK(U, {
          if ((!frozen && (B->type & FROZEN)) ||
              (!thawed && (!((Net->type | G->type | U->type | B->type)
                 & FROZEN)))) i += B->numUnits;
          else for (si = i + B->numUnits; i < si; i++) {
            readLinkData(channel, U->incoming + i, U->incoming2 + i,
             numValues, R);
            //print(1, "\b\b\b\b% 4d", jjj++);
          };

	})})})}
  print(1,"\n");
  if (totalUpdates >= 0 && thawed)
    Net->totalUpdates = totalUpdates;

 done:
  if (rec.buf) freeString(rec.buf);
  closeChannel(channel);
  return result;
}

flag loadXerionWeights(Tcl_Obj *fileNameObj) {
  Tcl_Channel channel;
  Tcl_DString ds;
  flag result = TCL_OK;
  char fromGroup[MAX_NAME_LENGTH], toGroup[MAX_NAME_LENGTH];
  int fromUnit, toUnit;
  real weight;
  Group F, T;
  Link L;
  const char *fileName = Tcl_GetStringFromObj(fileNameObj, NULL);

  if (!(channel = readChannel(fileNameObj)))
    return warning("loadXerionWeights: couldn't open the file \"%s\"",
		   fileName);
  Tcl_DStringInit(&ds);
  /* Discard the first line. */
  Tcl_Gets(channel, &ds);
  Tcl_DStringFree(&ds);
  while (Tcl_Gets(channel, &ds) > 0) {
    char *s = Tcl_DStringValue(&ds);
    if (subString("Bias", s, 4)) {
      if (!(F = lookupGroup(BIAS_NAME))) {
	    result = warning("loadXerionWeights: we need a bias group");
	    goto done;
      }
      fromUnit = 0;
      #ifdef FLOAT_REAL
        sscanf(s, "Bias -> %[^.].%d: %f", toGroup, &toUnit, &weight);
      #else
        sscanf(s, "Bias -> %[^.].%d: %lf", toGroup, &toUnit, &weight);
      #endif
    } else {
      #ifdef FLOAT_REAL
        sscanf(s, "%[^.].%d -> %[^.].%d: %f", fromGroup, &fromUnit,
	       toGroup, &toUnit, &weight);
      #else
         sscanf(s, "%[^.].%d -> %[^.].%d: %lf", fromGroup, &fromUnit,
	       toGroup, &toUnit, &weight);
      #endif // FLOAT_REAL

      if (!(F = lookupGroup(fromGroup))) {
	    result = warning("loadXerionWeights: unknown group: %s", fromGroup);
	    goto done;
      }
      if (fromUnit < 0 || fromUnit >= F->numUnits) {
	    result = warning("loadXerionWeights: unit %d out of range in group s",
			 fromUnit, fromGroup);
	goto done;
      }
    }
    if (!(T = lookupGroup(toGroup))) {
      result = warning("loadXerionWeights: unknown group: %s", toGroup);
      goto done;
    }
    if (toUnit < 0 || toUnit >= T->numUnits) {
      result = warning("loadXerionWeights: unit %d out of range in group s",
		       toUnit, toGroup);
      goto done;
    }

    if (!(L = lookupLink(F->unit + fromUnit, T->unit + toUnit, NULL, ALL_LINKS))) {
      result = warning("loadXerionWeights: missing link from %s:%d to %s:%d",
		       F->name, fromUnit, T->name, toUnit);
      goto done;
    }
    initLinkValues(L, getLink2(T->unit + toUnit, L));
    L->weight = weight;
    Tcl_DStringFree(&ds);
  }

 done:
  closeChannel(channel);
  return result;
}
