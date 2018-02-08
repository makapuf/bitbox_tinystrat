// embed data (in a C file for now since C++ cannot avoid having a zero at end of string ...)

#define DATA_IMPLEMENTATION
#include "data.h"
#undef DATA_IMPLEMENTATION

#define DEFS_IMPLEMENTATION
#include "defs.h"
#undef DEFS_IMPLEMENTATION
