/*************************************************************************
 * $Id: Matrix.hxx,v 1.5 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created: 11/13/99 by Charles Chiou
 * Comments: This matrix implementation assumes a densly-populated matrix
 *           or a non-sparse matrix.
 * Status: determinant(), and inverse() need to be worked on.
 *         Add LU decomposition and other functions later.
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _MATRIX_HXX_
#define _MATRIX_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"
#include "Exception.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI MatrixException : public Exception {

  DECLARE_RUNTIME_SERIALIZABLE(MatrixException);

public:

  MatrixException();
  MatrixException(const char *msg);

};

template <class T>
class Matrix : public BaseObject {

public:

  Matrix(int rows = 0, int cols = 0);
  Matrix(const Matrix<T> &matrix);
  Matrix<T> &operator=(const Matrix<T> &matrix);

  ~Matrix();

  Matrix<T> &setSize(int rows, int cols);

  int numRows() const;
  int numCols() const;

  boolean isZeroMatrix() const;
  boolean isSquareMatrix() const;
  boolean isDiagonalMatrix() const throw(MatrixException);
  boolean isTriDiagonalMatrix() const throw(MatrixException);
  boolean isUpperTriangularMatrix() const throw(MatrixException);
  boolean isLowerTriangularMatrix() const throw(MatrixException);
  boolean isPermutationMatrix() const throw(MatrixException);
  boolean isSymmetric() const throw(MatrixException);
  boolean operator==(const Matrix<T> &matrix) const throw(MatrixException);
  boolean operator!=(const Matrix<T> &matrix) const throw(MatrixException);

  const T &operator()(int row, int col) const;
  T &operator()(int row, int col);

  void nullify();
  boolean isNull() const;
  Matrix &operator=(const T value);
  void swapRows(int row1, int row2);
  void swapCols(int col1, int col2);

  // Matrix transformations
  Matrix<T> add(const Matrix<T>& matrix) const throw(MatrixException);
  Matrix<T> operator+(const Matrix<T> &matrix) const throw(MatrixException);
  Matrix<T> subtract(const Matrix<T> &matrix) const throw(MatrixException);
  Matrix<T> operator-(const Matrix<T> &matrix) const throw(MatrixException);
  Matrix<T> multiply(const Matrix<T> &matrix) const throw(MatrixException);
  Matrix<T> operator*(const Matrix<T> &matrix) const throw(MatrixException);
  Matrix<T> multiply(const T &multipler) const throw(MatrixException);
  Matrix<T> operator*(const T &multiplier) const throw(MatrixException);
  Matrix<T> componentWiseMultiply(const Matrix<T> &matrix) const
    throw(MatrixException);
  Matrix<T> operator<<(const Matrix<T> &matrix) const throw(MatrixException);
  Matrix<T> transpose() const throw(MatrixException);
  Matrix<T> invert() const throw(MatrixException);
  T determinant() const throw(MatrixException);
  T sum() const;

  Matrix<T> submatrix(int row, int col, int rows, int cols) const
    throw(MatrixException);
  Matrix<T> transform(const Matrix<T> &t1, const Matrix<T> &t2) const
    throw(MatrixException);

private:

  void init(int rows, int cols, T *data = NULL);

  T *_data;

  int _rows;
  int _cols;

};

template <class T>
Matrix<T> multiply(const T &multipler, const Matrix<T> &matrix)
  throw(MatrixException);

template <class T>
Matrix<T> operator*(const T &multipler, const Matrix<T> &matrix)
  throw(MatrixException);

// Inline functions.

inline MatrixException::MatrixException() : Exception() {

}

inline MatrixException::MatrixException(const char *msg) : Exception(msg) {

}

template <class T>
inline Matrix<T>::Matrix(int rows, int cols) {
  _data = NULL;
  _rows = _cols = 0;
  init(rows, cols);
}

template <class T>
inline Matrix<T>::Matrix(const Matrix<T> &matrix) {
  _data = NULL;
  _rows = _cols = 0;
  init(matrix._rows, matrix._cols, matrix._data);
}

template <class T>
Matrix<T> &Matrix<T>::operator=(const Matrix<T> &matrix) {
  if(&matrix != this) {
    init(matrix._rows, matrix._cols, matrix._data);
  }

  return *this;
}

template <class T>
inline Matrix<T>::~Matrix() {
  nullify();
}

template <class T>
inline Matrix<T> &Matrix<T>::setSize(int rows, int cols) {
  init(rows, cols);
  return *this;
}

template <class T>
inline boolean Matrix<T>::isNull() const {
  return (_rows == 0 || _cols == 0);
}

template <class T>
inline boolean Matrix<T>::isSquareMatrix() const {
  return (_rows == _cols && _rows != 0);
}

template <class T>
inline boolean Matrix<T>::operator==(const Matrix<T> &matrix) const
  throw(MatrixException) {
  return !operator!=(matrix);
}

template <class T>
inline const T &Matrix<T>::operator()(int row, int col) const {
  return _data[row][col];
}

template <class T>
inline T &Matrix<T>::operator()(int row, int col) {
  return _data[row][col];
}

template <class T>
inline Matrix<T> Matrix<T>::operator+(const Matrix<T>& matrix) const
  throw(MatrixException) {
  return add(matrix);
}

template <class T>
inline Matrix<T> Matrix<T>::operator-(const Matrix<T>& matrix) const
  throw(MatrixException) {
  return subtract(matrix);
}

template <class T>
inline Matrix<T> Matrix<T>::operator*(const Matrix<T>& matrix) const
  throw(MatrixException) {
  return multiply(matrix);
}

template <class T>
Matrix<T> Matrix<T>::operator*(const T &multiplier) const
  throw(MatrixException) {
  return multiply(multiplier);
}

template <class T>
inline Matrix<T> Matrix<T>::operator<<(const Matrix<T> &matrix) const
  throw(MatrixException) {
  return componentWiseMultiply(matrix);
}

template <class T>
Matrix<T> multiply(const T &multiplier, const Matrix<T> &matrix)
  throw(MatrixException) {
  return matrix.multiply(multiplier);
}

template <class T>
Matrix<T> operator*(const T &multiplier, const Matrix<T> &matrix)
  throw(MatrixException) {
  return matrix.multiply(multiplier);
}

__END_NAMESPACE(SELFSOFT);

#endif
