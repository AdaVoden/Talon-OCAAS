#ifndef WCSTRIG
#define WCSTRIG

#include <math.h>

#if __STDC__
   double cosd(double);
   double sind(double);
   double tand(double);
   double acosd(double);
   double asind(double);
   double atand(double);
   double atan2d(double, double);
#else
   double cosd();
   double sind();
   double tand();
   double acosd();
   double asind();
   double atand();
   double atan2d();
#endif

#endif /* WCSTRIG */
