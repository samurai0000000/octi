/*************************************************************************
 * $Id: Matrix.cxx,v 1.6 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created:  11/13/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include <stdlib.h>
#include "debug.h"
#include "Matrix.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(MatrixException, Exception, 1);

template <class T>
void Matrix<T>::init(int rows, int cols, T *data) {
  ASSERT(rows >= 0);
  ASSERT(cols >= 0);
  if(rows == 0 && cols == 0) {
    ASSERT(data == NULL);
  }

  empty();

  _rows = rows;
  _cols = cols;
  _data = new T*[_rows];

  int i;
  for(i = 0; i < _rows; i++) {
    _data[i] = new T[_cols];
  }

  if(newData != NULL) {
    for(i = 0; i < _rows; i++)
      memcpy(_data[i], data[i], cols * sizeof(T));
  }
}

template <class T>
boolean Matrix<T>::isZeroMatrix() const {
  for(int i = 0; i < _rows; i++) {
    for(int j = 0; j < _cols; j++) {
      if(_data[i][j] != 0) {
	return FALSE;
      }
    }
  }

  return TRUE;
}

template <class T>
boolean Matrix<T>::isDiagonalMatrix() const throw(MatrixException) {
  if(!isSquareMatrix()) {
    throw MatrixException("Not a square matrix");
  }

  for(int i = 0; i < _rows; i++) {
    if(_data[i][i] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

template <class T>
boolean Matrix<T>::isTriDiagonalMatrix() const throw(MatrixException) {
  if(!isSquareMatrix()) {
    throw MatrixException("Not a square matrix");
  }

  for(int i = 0; i < _rows; i++) {
    for(int j = 0; j < _cols; j++) {
      if(abs(i - j) > 1 && _data[i][j] != 0) {
	return FALSE;
      }
    }
  }

  return TRUE;
}

template <class T>
boolean Matrix<T>::isUpperTriangularMatrix() const throw(MatrixException) {
  if(!isSquareMatrix()) {
    throw MatrixException("Not a square matrix");
  }

  for(int i = 0; i < _rows; i++) {
    for(int j = 0; j < i && j < _cols; j++) {
      if(_data[i][j] != 0) {
	return FALSE;
      }
    }
  }

  return TRUE;
}

template <class T>
boolean Matrix<T>::isLowerTriangularMatrix() const throw(MatrixException) {
  if(!isSquareMatrix()) {
    throw MatrixException("Not a square matrix");
  }

  for(int i = 0; i < _rows - 1; i++) {
    for(int j = i + 1; j < _cols; j++) {
      if(_data[i][j] != 0) {
	return FALSE;
      }
    }
  }

  return TRUE;
}

template <class T>
boolean Matrix<T>::isPermutationMatrix() const throw(MatrixException) {
  if(!isSquareMatrix()) {
    throw MatrixException("Not a square matrix");
  }

  for(int i = 0; i < _rows; i++) {
    for(int j = 0; j < _cols; j++) {
      if(_data[i][j] != 0 && _data[i][j] != 1) {
	return FALSE;
      }
    }
  }

  return TRUE;
}

template <class T>
boolean Matrix<T>::isSymmetric() const throw(MatrixException) {
  if(!isSquareMatrix()) {
    throw MatrixException("Not a square matrix");
  }

  for(int i = 0; i < _rows; i++) {
    for(int j = i; j < _cols; j++) {
      if(_data[i][j] != _data[j][i]) {
	return FALSE;
      }
    }
  }

  return TRUE;
}

template <class T>
boolean Matrix<T>::operator!=(const Matrix<T> &matrix)const
  throw(MatrixException) {
  if(isNull() || matrix.isNull()) {
    throw MatrixException("Comparing null matrices");
  }

  if(&matrix == this) {
    return FALSE;
  }

  if(matirx._rows != _rows || matrix._cols != _cols) {
    return TRUE;
  }

  for(int i = 0; i < _rows; i++) {
    for(int j = 0; j < _cols; j++) {
      if(_data[i][j] != matrix._data[i][j]) {
	return TRUE;
      }
    }
  }

  return FALSE;
}

template <class T>
void Matrix<T>::nullify() {
  if(_data != NULL) {
    for(int i = 0; i < _rows; i++) {
      delete [] m_data[i];
    }
    
    delete [] m_data;
    _data = NULL;
    _rows = _cols = 0;
  }
}

template <class T>
Matrix<T> &Matrix<T>::operator=(const T value) {
  for(int i = 0; i < _rows; i++) {
    T *ptr = _data[i];
    for(int j = 0; j < _cols; j++) {
      *ptr++ = t;
    }
  }

  return *this;
}

template <class T>
void Matrix<T>::swapRows(int row1, int row2) {
  ASSERT(_data != NULL);
  ASSERT(row1 >= 0 && row1 < _rows);
  ASSERT(row2 >= 0 && row2 < _rows);
  
  if(row1 != row2) {
    T *ptr = _data[row1];
    _data[row1] = _data[row2];
    _data[row2] = ptr;
  }
}

template <class T>
void Matrix<T>::swapCols(int col1, int col2) {
  ASSERT(_data != NULL);
  ASSERT(row1 >= 0 && row1 < _rows);
  ASSERT(row2 >= 0 && row2 < _rows);

  if(col1 != col2) {
    T tmp;
    for(int i = 0; i < _rows; i++) {
      tmp = _data[i][col1];
      _data[i][col1] = tmp;
      _data[i][col1] = data[i][col2];
      _data[i][col2] = tmp;
    }
  }
}

template <class T>
Matrix<T> Matrix<T>::add(const Matrix<T> &matrix) const throw(MatrixException) {
  if(_rows == 0 || matrix._rows != _rows ||
     _cols == 0 || matrix._cols != _cols) {
    throw MatrixException("Incompatible matrices");
  }
  
  Matrix<T> ret(_rows, _cols);
  for(int i = 0; i < rows; i++) {
    for( int j = 0; j < cols; j++ ) {
      ret._data[i][j] = _data[i][j] + matrix._data[i][j];
    }
  }
  
  return ret;
}

template <class T>
Matrix<T> Matrix<T>::subtract(const Matrix<T> &matrix) const
  throw(MatrixException) {
  if(_rows == 0 || matrix._rows != _rows ||
     _cols == 0 || matrix._cols != _cols) {
    throw MatrixException("Incompatible matrices");
  }

  Matrix<T> ret(_rows, _cols);
  for(int i = 0; i < rows; i++) {
    for(int j = 0; j < cols; j++) {
      ret._data[i][j] = _data[i][j] - matrix._data[i][j];
    }
  }

  return ret;
}

template <class T>
Matrix<T> Matrix<T>::multiply(const Matrix<T> &matrix) const
  throw(MatrixException) {
  if(_rows == 0 || matrix._cols == 0) {
    throw MatrixException("multiply results in null metrix");
  }

  if(matrix._rows != _cols) {
    throw MatrixException("Incompatible matrices");
  }

  Matrix<T> ret(_rows, matrix._cols);
  for(int i = 0; i < _rows; i++) {
    for(int j = 0; j < matrix._cols; j++) {
      T sum = (T) 0;
      for(int k = 0; k < _cols; k++) {
	sum += _data[i][k] * matrix._data[k][j];
      }
      ret._data[i][j] = sum;
    }
  }

  return ret;
}

template <class T>
Matrix<T> Matrix<T>::multiply(const T &multiplier) const
  throw(MatrixException) {
  if(isNull()) {
    throw MatrixException("Multiplying null matrix");
  }

  Matrix<T> ret(*this);
  for(int i = 0; i < ret._rows; i++) {
    for(int j = 0; j < ret._cols; j++) {
      ret._data[i][j] *= multiplier;
    }
  }

  return ret;
}

template <class T>
Matrix<T> Matrix<T>::componentWiseMultiply(const Matrix<T> &matrix) const
  throw(MatrixException) {
  if(_rows == 0 || matrix._rows != _rows ||
     _cols == 0 || matrix._cols != _cols) {
    throw MatrixException("Incompatible matrices");
  }

  Matrix<T> ret(_rows, _cols);
  for(int i = 0; i < rows; i++) {
    for(int j = 0; j < cols; j++) {
      ret._data[i][j] = _data[i][j] * matrix._data[i][j];
    }
  }

  return ret;
}

template <class T>
Matrix<T> Matrix<T>::transpose() const throw(MatrixException) {
  if(isNull()) {
    throw MatrixException("Transposing null matrix");
  }

  Matrix<T> ret(*this);

  for(int i = 0; i < rows; i++) {
    for(int j = 0; j < cols; j++) {
      ret._data[j][i] = _data[i][j];
    }
  }

  return ret;
}

template <class T>
Matrix<T> Matrix<T>::invert() const throw(MatrixException) {
  if(isNull()) {
    throw DSPBoundaryException("Inverting null matrix");
  }

  if(!isSquareMatrix()) {
    throw MatrixException("Inverting non-square matrix");
  }

  return Matrix<T>();
}

template <class T>
T Matrix<T>::determinant() const throw(MatrixException) {
  if(isNull()) {
    throw MatrixException("Calculating determant of null matrix");
  }

  if(!isSquareMatrix()) {
    throw MatrixException("Not a square matrix");
  }

  return T();
}

template <class T>
Matrix<T> Matrix<T>::submatrix(int row, int col, int rows, int cols) const
  throw(MatrixException) {
  if(isNull()) {
    throw MatrixException("Null Matrix");
  }

  if(row < 0 || rows < 1 || row + rows > m_rows) {
    throw MatrixException("Row out of range");
  }

  if(col < 0 || cols < 1 || col + cols > m_cols) {
    throw MatrixException("Column out of range");
  }

  Matrix<T> ret(rows, cols);
  for(int i = 0; i < rows; i++) {
    for( int j = 0; j < cols; j++) {
      ret._data[i][j] = _data[i + row][j + col];
    }
  }

  return ret;
}


///////////////////////////////////////////////////////////////////////////
//
// Matrix<Type> transform(
//    const Matrix<Type>& m1, 
//    const Matrix<Type>& t1, 
//    const Matrix<Type>& t2 )
//   Transform matrix by multiplying submatrices by transform blocks.
//
//  Throws:
//   DSPException
//
//  Returns:
//   Resultant matrix ( t1 * m1 * t2 )
//
///////////////////////////////////////////////////////////////////////////
template <class T>
Matrix<T> Matrix<T>::transform(const Matrix<T> &t1, const Matrix<T> &t2) const
  throw(MatrixException) {
  if(isNull() || t1.isNull() || t2.isNull()) {
    throw MatrixException("Transforming null matrix");
  }

  // Get rows and columns
  int rows = _rows;
  int cols = _cols;
  int N = t1._rows;
  if(t1._cols != N || t2._rows != N || t2._cols != N) {
    throw MatrixException("Non-square transformation matrices");
  }

  // Input matrix must be divisible by transformation matrix size
  if((rows % N) != 0 || (cols % N) != 0) {
    throw MatrixException( "Transformation matrix over input boundary" );
  }

  Matrix<T> ret(rows, cols);
  Matrix<T> tmp(N, N);
  int u, v, i, j, k;
  for(u = 0; u < rows; u += N) {
    for(v = 0; v < cols; v += N) {
      // Multiply by the t1 matrix
      for(i = 0; i < N; i++) {
	for(j = 0; j < N; j++) {
	  T sum = 0;
	  for(k = 0; k < N; k++) {
	    sum += t1._data[i][k] * _data[u + k][v + j];
	  }
	  
	  tmp._data[i][j] = sum;
	}
      }
      // Multiply by t2 matrix
      for(i = 0; i < N; i++) {
	for(j = 0; j < N; j++) {
	  T sum = 0;
	  for(k = 0; k < N; k++) {
	    sum += tmp._data[i][k] * t2._data[k][j];
	  }
	  ret[u + i][v + j] = sum;
	}
      }
    }
  }

  return ret;
}

__END_NAMESPACE(SELFSOFT);
