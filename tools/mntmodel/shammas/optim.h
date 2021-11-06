#ifndef _OPTIM_H_
#define _OPTIM_H_

#include "global.h"
#include "arrays.h"

#define OPTIM_BAD_RESULT -1.0E+30

double BisectionMin(double Xa, double Xb,
                    double tolerance, double (*f)(double));

double NewtonMin(double X, double tolerance,
                 int MaxIter, double (*f)(double));

double GoldenSearchMin(double Xa, double Xb,
                    double tolerance, int maxIter,
                    double (*f)(double));

double QuadIntMin(double Xa, double Xb, double Xc,
                  double Xtol, double Ftol,
                  int maxIter,
                  double (*f)(double));

double CubicIntMin(double Xa, double Xb,
                  double Xtol, double Gtol,
                  int maxIter,
                  double (*f)(double));

void CalcYSimplex(Matrix x, Vector y, int numVars,
                  double (*fx)(double*));

void Simplex(Matrix x, Vector y, int numVars, double tolerance,
             double reflectFact, double expandFact,
             double contractFact, int maxIter,
             double (*fx)(double*));

int NewtonMultiMin(Vector x, int numVars, double tolerance,
                   int maxIter, double (*fx)(double*));
#endif
