#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define TRUE 1
#define FALSE 0
#define EPSILON 1.0e-15
#define BAD_RESULT -1.0e+30
#define MISSING_DATA -2.0e+30

#define SQR(x) ((x) * (x))
#define CUBE(x) ((x) * (x) * (x))
#define ONEOVER(x) (1 / (x))
#define RECIPROCAL(x) (1 / (x))

typedef int Boolean;

enum boolean { false, true };
enum answer { no, yes, dont_care };

#endif
