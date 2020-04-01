#ifndef __ETHER_MATH_C
#define __ETHER_MATH_C

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))

#define CLAMP_MIN(x, min) MAX(x, min)
#define CLAMP_MAX(x, max) MIN(x, max)

#endif
