#include "global.h"
#include "arrays.h"
#include "optim.h"
#include <math.h>

double slope(double x, double (*f)(double))
{
  double h = (fabs(x) > 1) ? 0.01 * x : 0.01;
  return ((*f)(x + h) - (*f)(x - h)) / 2 / h;
}

double BisectionMin(double Xa, double Xb,
             double tolerance, double (*f)(double))
{
  double Xc;
  double Fa, Fb, Fc;

  Fa = (*f)(Xa);
  Fb = (*f)(Xb);
  do {
    Xc = (Xa + Xb) / 2;
    Fc = (*f)(Xc);
    if (slope(Xc, f) * slope(Xa, f) > 0) {
      Xa = Xc;
      Fc = Fa;
    }
    else {
      Xb = Xc;
      Fc = Fb;
    }
  } while (fabs(Xb - Xa) > tolerance);
  return Xc;
}

double NewtonMin(double X, double tolerance,
                 int maxIter, double (*f)(double))
{
  double h, diff;
  double Fm, F0, Fp;
  double firstDeriv, secondDeriv;
  int iter = 0;

  do {
    /* caluclate increment */
    h = (fabs(X) > 1) ? 0.01 * X : 0.01;
    /* calculate function values at X-h, X, and X+h */
    Fm = (*f)(X - h);
    F0 = (*f)(X);
    Fp = (*f)(X + h);
    /* calculate the first derivative */
    firstDeriv = (Fp - Fm) / 2 / h;
    /* calculate the second derivative */
    secondDeriv = (Fp - 2 * F0 + Fm) / SQR(h);
    /* calculate the guess refinement */
    diff = firstDeriv / secondDeriv;
    /* refine the guess */
    X -= diff;
    iter++; /* the increment iteration counter */
  } while (fabs(diff) > tolerance &&
           iter < maxIter);

  return (iter <= maxIter) ? X : OPTIM_BAD_RESULT;
}


double GoldenSearchMin(double Xa, double Xb,
                    double tolerance, int maxIter,
                    double (*f)(double))
{
  double Xc, Xd, Fc, Fd;
  double oneMinusTau = 1 - (sqrt(5) - 1) / 2;
  int iter = 0;

  Xc = Xa + oneMinusTau * (Xb - Xa);
  Fc = (*f)(Xc);
  Xd = Xb - oneMinusTau * (Xb - Xa);
  Fd = (*f)(Xd);
  do {
    iter++;
    if (Fc < Fd) {
       Xb = Xd;
       Xd = Xc;
       Xc = Xa + oneMinusTau * (Xb - Xa);
       Fd = Fc;
       Fc = (*f)(Xc);
    }
    else {
       Xa = Xc;
       Xc = Xd;
       Xd = Xb - oneMinusTau * (Xb - Xa);
       Fc = Fd;
       Fd = (*f)(Xd);
    }
  } while (fabs(Xb - Xa) > tolerance && iter < maxIter);
  return (iter <= maxIter) ? (Xa + Xb) / 2 : OPTIM_BAD_RESULT;
}

double QuadIntMin(double Xa, double Xb, double Xc,
                  double Xtol, double Ftol,
                  int maxIter,
                  double (*f)(double))
{
  double Fa, Fb, Fc, lastFc, x, Fx;
  int iter = 0;
  int ok = FALSE;

  Fa = (*f)(Xa);
  Fb = (*f)(Xb);
  Fc = (*f)(Xc);

  do {
    iter++;
    lastFc = Fc;
    x = 0.5 *
          (
            (SQR(Xb) - SQR(Xc)) * Fa +
            (SQR(Xc) - SQR(Xa)) * Fb +
            (SQR(Xa) - SQR(Xb)) * Fc
          ) /
          (
            (Xb - Xc) * Fa +
            (Xc - Xa) * Fb +
            (Xa - Xb) * Fc
          );
    Fx = (*f)(x);
    if (x < Xc && Fx < Fc) {
      Xb = Xc;
      Xc = x;
      Fb = Fc;
      Fc = Fx;
    }
    else if (x > Xc && Fx > Fc) {
      Xb = x;
      Fb = Fx;
    }
    else if (x < Xc && Fx > Fc) {
      Xa = x;
      Fa = Fx;
    }
    else {
      Xa = Xc;
      Xc = x;
      Fa = Fc;
      Fc = Fx;
    }
  if (fabs(Xa - Xc) < Xtol ||
      fabs(Xc - Xb) < Xtol)
    ok = TRUE;

  } while (!(ok ||
           fabs((lastFc - Fc)/lastFc) < Ftol &&
           iter > maxIter));
  return (iter <= maxIter) ? x : OPTIM_BAD_RESULT;
}

double CubicIntMin(double Xa, double Xb,
                  double Xtol, double Gtol,
                  int maxIter,
                  double (*f)(double))
{
  double Fa, Fb, Fx;
  double Ga, Gb, Gx;
  double w, x, v, h, Gmin;
  int iter = 0;

  Fa = (*f)(Xa);
  Fb = (*f)(Xb);
  /* calculate slope at Xa */
  Ga = slope(Xa, f);
  /* calculate slope at Xb */
  Gb = slope(Xb, f);
  do {
    iter++;
    w = 3 / (Xb - Xa) * (Fa - Fb) + Ga + Gb;
    v = sqrt(SQR(w) - Ga * Gb);
    x = Xa + (Xb - Xa) *
         (1 - (Gb + v - w) / (Gb - Ga + 2 * v));
    Fx = (*f)(x);
    /* calculate slope at x */
    Gx = slope(x, f);
    if (Ga < 0 && (Gx > 0 || Fx > Fa)) {
      Xb = x;
      Fb = Fx;
      Gb = Gx;
    }
    else {
      Xa = x;
      Fa = Fx;
      Ga = Gx;
    }
    Gmin = (fabs(Ga) > fabs(Gb)) ? fabs(Gb) : fabs(Ga);
  } while (!(fabs(Xb - Xa) < Xtol ||
             Gmin < Gtol ||
             iter > maxIter));
  return (iter <= maxIter) ? x : OPTIM_BAD_RESULT;
}

void CalcYSimplex(Matrix x, Vector y, int numVars,
                  double (*fx)(double*))
{
  int numPoints = numVars + 1;
  int j, i;
  Vector Xarr;

  newVect(&Xarr, numVars);

  /* calculate the values for y[] using sumX[] */
  for (i = 0; i < numPoints; i++) {
    for (j = 0; j < numVars; j++)
      VEC(Xarr, j) = MAT(x, i, j);
    VEC(y, i) = (*fx)(Xarr.pData);
  }

  deleteVect(&Xarr);
}

void Simplex(Matrix x, Vector y, int numVars, double tolerance,
             double reflectFact, double expandFact,
             double contractFact, int maxIter,
             double (*fx)(double*))
{
  const double EPS = 1.0e-8;
  const double defReflectFact = 1.0;
  const double defExpandFact = 2.0;
  const double defContractFact = 0.5;

  int numIter = 0;
  int numPoints = numVars + 1;
  int j, i, worstI, bestI;
  Boolean goOn = TRUE;
  Boolean flag;
  double y1, y2, x0;
  double yMean, sum;
  Vector X1, X2, Centroid;

  newVect(&X1, numVars);
  newVect(&X2, numVars);
  newVect(&Centroid, numVars);

  /* check Simplex modification factors */
  if (reflectFact < EPS)
    reflectFact = defReflectFact;
  if (expandFact < EPS)
    expandFact = defExpandFact;
  if (contractFact < EPS)
    contractFact = defContractFact;

  /* calculate the values for y[] using X1 */
  for (i = 0; i < numPoints; i++) {
    for (j = 0; j < numVars; j++)
      VEC(X1, j) = MAT(x, i, j);
    VEC(y, i) = (*fx)(X1.pData);
  }

  while ((++numIter < maxIter) && goOn) {
    /* find worst and best point */
    worstI = 0;
    bestI = 0;
    for (i = 1; i < numPoints; i++)
      if (VEC(y, i) < VEC(y, bestI))
        bestI = i;
      else if (VEC(y, i) > VEC(y, worstI))
        worstI = i;

    /* calculate centroid (exclude worst point) */
    for (i = 0; i < numVars; i++) {
      VEC(Centroid, i) = 0;
      for (j = 0; j < numPoints; j++)
        if (j != worstI)
          VEC(Centroid, i) += MAT(x, j, i);
      VEC(Centroid, i) /= numVars;
    }
    /* calculate reflected point */
    for (i = 0; i < numVars; i++)
      VEC(X1, i) = (1 + reflectFact) * VEC(Centroid, i) -
                   reflectFact * MAT(x, worstI, i);
    y1 = fx(X1.pData);

    if (y1 < VEC(y, bestI)) {
      /* calculate expanded point */
      for (i = 0; i < numVars; i++)
        VEC(X2, i) = (1 + expandFact) * VEC(X1, i) -
                     expandFact * VEC(Centroid, i);
      y2 = fx(X2.pData);
      if (y2 < VEC(y, bestI)) {
        /* replace worst point by X2 */
        for (i = 0; i < numVars; i++)
          MAT(x, worstI, i) = VEC(X2, i);
      }
      else {
        /* replace worst point by X1 */
        for (i = 0; i < numVars; i++)
          MAT(x, worstI, i) = VEC(X1, i);
      }
    }
    else {
      flag = TRUE;
      for (i = 0; i < numPoints; i++)
        if (i != worstI && y1 <= VEC(y, i)) {
          flag = FALSE;
          break;
        }
      if (flag) {
        if (y1 < VEC(y, worstI)) {
          /* replace worst point by X1 */
          for (i = 0; i < numVars; i++)
            MAT(x, worstI, i) = VEC(X1, i);
          VEC(y, worstI) = y1;
        }
        /* calculate contracted point */
        for (i = 0; i < numVars;  i++)
          VEC(X2, i) = contractFact * MAT(x, worstI, i) +
                     (1 - contractFact) * VEC(Centroid, i);
          y2 = fx(X2.pData);
          if (y2 > VEC(y, worstI)) {
            /* store best x in X1 */
            for (i = 0; i < numVars; i++)
              VEC(X2, i) = MAT(x, bestI, i);
            for (j = 0; j < numPoints; j++) {
              for (i = 0; i < numVars; i++)
                MAT(x, j, i) = 0.5 * (VEC(X2, i) +
                                      MAT(x, j, i));
            }
          }
          else {
            /* replace worst point by X2 */
            for (i = 0; i < numVars; i++)
              MAT(x, worstI, i) = VEC(X2, i);
          }
      }
      else {
         /* replace worst point by X1 */
         for (i = 0; i < numVars; i++)
           MAT(x, worstI, i) = VEC(X1, i);
      }
    }
    /* calculate the values for y[] using X1 */
    for (i = 0; i < numPoints; i++) {
      for (j = 0; j < numVars; j++)
        VEC(X1, j) = MAT(x, i, j);
      VEC(y, i) = (*fx)(X1.pData);
    }
    /* calculate mean y */
    yMean = 0.;
    for (i = 0; i < numPoints; i++)
      yMean += VEC(y, i);
    yMean /= numPoints;
    /* calculate deviation fron mean y */
    sum = 0.;
    for (i = 0; i < numPoints; i++)
      sum += SQR(VEC(y, i) - yMean);
    /* test convergence */
    goOn = (sqrt(sum / numPoints) > tolerance) ? TRUE : FALSE;
  }

  /* find the best point */
  bestI = 0;
  for (i = 1; i < numPoints; i++)
    if (VEC(y, i) < VEC(y, bestI))
      bestI = i;
  if (bestI != 0) {
    /* move best point to index 0 */
    for (i = 0; i < numVars; i++) {
       x0 = MAT(x, 0, i);
       MAT(x, 0, i) = MAT(x, bestI, i);
       MAT(x, bestI, i) = x0;
    }
    y1 = VEC(y, 0);
    VEC(y, 0) = VEC(y, bestI);
    VEC(y, bestI) = y1;
  }

  deleteVect(&X1);
  deleteVect(&X2);
  deleteVect(&Centroid);

}


int ExNewtonMin(Vector X, double tolerance,
                 double* diff, int maxIter, int index,
                 double (*f)(double*))
{
  double h;
  double Fm, F0, Fp;
  double firstDeriv, secondDeriv;
  int iter = 0;
  double xOld = VEC(X, index);
  double x = xOld;

  do {
    /* calculate increment */
    h = (fabs(x) > 1) ? 0.01 * x : 0.01;
    /* calculate function values at x-h, X, and x+h */
    VEC(X, index) = x - h;
    Fm = (*f)(X.pData);
    VEC(X, index) = x + h;
    Fp = (*f)(X.pData);
    VEC(X, index) = x;
    F0 = (*f)(X.pData);
    /* calculate the first derivative */
    firstDeriv = (Fp - Fm) / 2 / h;
    /* calculate the second derivative */
    secondDeriv = (Fp - 2 * F0 + Fm) / SQR(h);
    /* calculate the guess refinement */
    *diff = firstDeriv / secondDeriv;
    /* refine the guess */
    x -= *diff;
    VEC(X, index) = x;
    iter++; /* the increment iteration counter */
  } while (fabs(*diff) > tolerance &&
           iter < maxIter);

  if (iter <= maxIter)
    return TRUE;
  else {
    VEC(X, index) = xOld;
    return FALSE;
  }
}

int NewtonMultiMin(Vector x, int numVars, double tolerance,
                   int maxIter, double (*fx)(double*))
{
  int i, j, k;
  double diff;
  int ok;

  do {
    ok = FALSE;
    for (i = 0; i < numVars; i++)
      if (ExNewtonMin(x, tolerance, &diff, maxIter,
                        i, fx) == TRUE) {

        if (diff > tolerance)
          ok = TRUE;
      } else
        return FALSE;
  } while (ok);
}