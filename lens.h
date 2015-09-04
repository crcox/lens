/* lens.h: This is the interface to the Lens library */

#ifndef LENS_H
#define LENS_H

#include "system.h"
extern flag startLens(char *progName, flag batchMode);
extern flag lens(char *fmt, ...);

#endif /* LENS_H */
