#ifndef _ARRAYS_H_
#define _ARRAYS_H_

struct VectTag {
  double* pData;
  int maxSize;
};

struct MatTag {
  double* pData;
  int maxRows;
  int maxCols;
};

struct IntVectTag {
  int* pData;
  int maxSize;
};

typedef struct MatTag Matrix;
typedef struct VectTag Vector;
typedef struct IntVectTag IntVector;

int newMat(Matrix* Mat, int maxRows, int maxCols);
void deleteMat(Matrix* Mat);
int newVect(Vector* Vect, int maxSize);
void deleteVect(Vector* Vect);
int newIntVect(IntVector* Vect, int maxSize);
void deleteIntVect(IntVector* Vect);
int checkRowCol(Matrix Mat, int row, int col);
int checkIndex(Vector Vect, int index);

#define MAT(Mat, row, col) Mat.pData[(row) + Mat.maxRows * (col)]
#define VEC(Vect, index) Vect.pData[(index)]


#endif

