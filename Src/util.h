/* util.h: These are commonly used functions for doing simple things */
#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include "system.h"
#include <tcl.h>

typedef struct string {
  int maxChars;
  int numChars; /* Equals strlen s */
  char *s;
} *String;

typedef struct parseRec {
  Tcl_Channel channel;
  Tcl_Obj   *fileName;
  char       cookie[sizeof(int)];
  int        cookiePos;
  flag       binary;
  int        line;
  String     buf;
  char      *s;
} *ParseRec;


extern void print(int minVerbosity, const char *fmt, ...);
extern void debug(char *fmt, ...);
extern flag result(const char *fmt, ...);
extern flag append(char *fmt, ...);
extern flag warning(const char *fmt, ...);
extern int  error(const char *fmt, ...);
extern void fatalError(const char *fmt, ...);
extern flag eval(const char *fmt, ...);
extern void beep(void);

extern void *safeMalloc(unsigned size, char *name);
extern void *safeCalloc(unsigned num, unsigned size, char *name);
extern void *safeRealloc(void *ptr, unsigned size, char *name);
extern int  *intArray(int n, char *name);
extern int  **intMatrix(int m, int n, char *name);
extern real *realArray(int n, char *name);
extern real **realMatrix(int m, int n, char *name);
#define FREE(p) {if (p) {free(p); p = NULL;}}
#define FREE_MATRIX(p) {if (p) {if (p[0]) free(p[0]); free(p); p = NULL;}}

extern char *copyString(const char *s);
extern flag subString(const char *a, const char *b, int minLength);
extern flag isBlank(char *string);
extern String newString(int maxChars);
extern String newStringCopy(char *s);
extern void stringSize(String S, int maxChars);
extern void stringCat(String S, char c);
extern void stringSet(String S, int idx, char c);
extern void stringAppend(String S, char *t);
extern void stringAppendV(String S, char *fmt, ...);
extern void clearString(String S);
extern void freeString(String S);
extern void freeBuffer(void);
extern flag readFileIntoString(String S, Tcl_Obj *fileNameObj);
extern char *readName(char *list, String name, flag *result);

extern int ConsoleOutput(ClientData instanceData, const char *buf,
		    int toWrite, int *errorCode);
extern Tcl_Channel readChannel(Tcl_Obj *fileNameObj);
extern Tcl_Channel writeChannel(Tcl_Obj *fileName, flag append);
extern void closeChannel(Tcl_Channel channel);
extern flag binaryEncoding(Tcl_Channel channel);

extern flag startParser(ParseRec R, int word);    /* returns error code */
extern flag skipBlank(ParseRec R);                /* returns error code */
extern flag isNumber(ParseRec R);                 /* returns true/false */
extern flag isInteger(char *s);                   /* returns true/false */
extern flag readInt(ParseRec R, int *val);        /* returns error code */
extern flag readReal(ParseRec R, real *val);      /* returns error code */
extern flag readLine(ParseRec R, String S);       /* returns error code */
extern flag readBlock(ParseRec R, String S);      /* returns error code */
extern flag stringMatch(ParseRec R, char *s);     /* returns true/false */
extern flag stringPeek(ParseRec R, char *s);      /* returns true/false */
extern flag fileDone(ParseRec R);                 /* returns true/false */

extern flag readBinInt(Tcl_Channel channel, int *val);
extern flag readBinReal(Tcl_Channel channel, real *val);
extern flag readBinFlag(Tcl_Channel channel, flag *val);
extern flag readBinString(Tcl_Channel channel, String S);
extern flag writeBinInt(Tcl_Channel channel, int x);
extern flag writeBinReal(Tcl_Channel channel, real r);
extern flag writeBinFlag(Tcl_Channel channel, flag f);
extern flag writeBinString(Tcl_Channel channel, char *s, int len);
extern flag cprintf(Tcl_Channel channel, char *fmt, ...);
extern void writeReal(Tcl_Channel channel, real r, char *pre, char *post);
extern void smartFormatReal(char *s, real x, int width);
extern void smartPrintReal(real x, int width, flag app);

extern flag receiveChar(Tcl_Channel channel, char *val);
extern flag receiveInt(Tcl_Channel channel, int *val);
extern flag receiveFlag(Tcl_Channel channel, flag *val);
extern flag receiveReal(Tcl_Channel channel, real *val);
extern flag receiveString(Tcl_Channel channel, char *s);
extern flag sendChar(Tcl_Channel channel, char c);
extern flag sendInt(Tcl_Channel channel, int x);
extern flag sendFlag(Tcl_Channel channel, flag f);
extern flag sendReal(Tcl_Channel channel, real r);
extern flag sendString(Tcl_Channel channel, const char *s);

extern void seedRand(unsigned int seed);
extern void timeSeedRand(void);
extern unsigned int getSeed(void);
extern int  randInt(int max);
extern real randReal(real mean, real range);
extern real randProb(void);
extern real randGaussian(real mean, real range);
extern void randSort(int *array, int n);

extern void buildSigmoidTable(void);
extern real fastSigmoid(real x);

extern real multGaussianNoise(real value, real range);
extern real addGaussianNoise(real value, real range);
extern real multUniformNoise(real value, real range);
extern real addUniformNoise(real value, real range);

extern unsigned long timeElapsed(unsigned long a, unsigned long b);
extern unsigned long getTime(void);
extern unsigned long getUTime(void);
extern void printTime(unsigned long time, char *dest);

extern real ator(char *s);
extern int  roundr(real r);
extern int  imin(int a, int b);
extern int  imax(int a, int b);
extern real rmin(real a, real b);
extern real rmax(real a, real b);
extern real chooseValue(real a, real b);
extern real chooseValue3(real a, real b, real c);

#ifndef MIN
#  define MIN(a,b)    (((a)<(b))?(a):(b))
#endif /* MIN */
#ifndef MAX
#  define MAX(a,b)    (((a)>(b))?(a):(b))
#endif /* MAX */
#ifndef SQUARE
#  define SQUARE(x)   ((x) * (x))
#endif /* SQUARE */
#define SAME(x,y)     (((x) == (y)) || (isNaN(x) && isNaN(y)))
#define SIGMOID(x,g)  ((real) 1.0 / (EXP(-(x) * (g)) + 1.0))
#define LARGE_VAL     ((real) 1e8)
#define SMALL_VAL     ((real) 1e-8)
#define INV_SIGMOID(y,g) (((y) <= 0.0) ? -LARGE_VAL : \
			  ((y) >= 1.0) ? LARGE_VAL : \
			  (LOG((y) / (1-(y))) / (g)))
#ifndef PI
#define PI 3.1415927
#endif /* PI */

#define OFFSET(Obj,x) (((char *) &((Obj)->x)) - ((char *) (Obj)))

extern Tcl_Interp *Interp;
extern char Buffer[];
extern flag Gui, Batch, Console;
extern int Verbosity;
extern char *RootDir;

#endif /* UTIL_H */
