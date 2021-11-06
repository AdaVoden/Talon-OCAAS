#include "arrays.h"
#include <stdio.h>

int newMat(Matrix* Mat, int maxRows, int maxCols)
{
  maxRows = (maxRows < 1) ? 1 : maxRows;
  maxCols = (maxCols < 1) ? 1 : maxCols;
  Mat->maxRows = maxRows;
  Mat->maxCols = maxCols;
  Mat->pData = NULL;
  Mat->pData = (double*) malloc(maxRows * maxCols *
                                sizeof(double));
  return (Mat->pData != NULL) ? 1 : 0;
}

void deleteMat(Matrix* Mat)
{
  if (Mat->pData)
    free(Mat->pData);
  Mat->pData = NULL;
  Mat->maxCols = 0;
  Mat->maxRows = 0;
}

int newVect(Vector* Vect, int maxSize)
{
  maxSize = (maxSize < 1) ? 1 : maxSize;
  Vect->maxSize = maxSize;
  Vect->pData = NULL;
  Vect->pData = (double*) malloc(maxSize * sizeof(double));
  return (Vect->pData != NULL) ? 1 : 0;
}

void deleteVect(Vector* Vect)
{
  if (Vect->pData)
    free(Vect->pData);
  Vect->pData = NULL;
  Vect->maxSize = 0;
}

int newIntVect(IntVector* Vect, int maxSize)
{
  maxSize = (maxSize < 1) ? 1 : maxSize;
  Vect->maxSize = maxSize;
  Vect->pData = NULL;
  Vect->pData = (int*) malloc(maxSize * sizeof(int));
  return (Vect->pData != NULL) ? 1 : 0;
}

void deleteIntVect(IntVector* Vect)
{
  if (Vect->pData)
    free(Vect->pData);
  Vect->pData = NULL;
  Vect->maxSize = 0;
}


int checkRowCol(Matrix Mat, int row, int col)
{
  return (row >= 0 && col >= 0 &&
          row < Mat.maxRows && col < Mat.maxCols) ? 1 : 0;

}

int checkIndex(Vector Vect, int index)
{
  return (index >= 0 && index < Vect.maxSize) ? 1 : 0;
}


