#pragma once


template<typename T, size_t D>
struct Vector {
  T _d[D];

  Vector() {}
  Vector(T d0, T d1) { _d[0] = d0; _d[1] = d1; }
  Vector(T d0, T d1, T d2) { _d[0] = d0; _d[1] = d1; _d[2] = d2; }
  Vector(T d0, T d1, T d2, T d3) { _d[0] = d0; _d[1] = d1; _d[2] = d2; _d[3] = d3; }

  T& operator[](size_t i) { return _d[i]; }

  T *begin() { return _d; }
  T *end() { return _d + D; }
  size_t size() { return D; }

  Vector operator+() { return *this; }
  Vector operator-() { return *this * -1; }

  Vector& operator*=(T s) {
    for(size_t i = 0; i < D; ++i) _d[i] *= s;
    return *this;
  }
  Vector operator*(T s) { Vector a(*this); a *= s; return a; }

  Vector& operator/=(T s) {
    for(size_t i = 0; i < D; ++i) _d[i] /= s;
    return *this;
  }
  Vector operator/(T s) { Vector a(*this); a /= s; return a; }

  Vector& operator+=(Vector v) {
    for(size_t i = 0; i < D; ++i) _d[i] += v._d[i];
    return *this;
  }
  Vector operator+(Vector v) { v += *this; return v; }

  Vector& operator-=(Vector v) {
    for(size_t i = 0; i < D; ++i) _d[i] -= v._d[i];
    return *this;
  }
  Vector operator-(Vector v) { Vector a(*this); a -= v; return a; }

  T prod() {
    T p = 1;
    for(size_t i = 0; i < D; ++i) p *= _d[i];
    return p;
  }

  T sum() {
    T p = 0;
    for(size_t i = 0; i < D; ++i) p += _d[i];
    return p;
  }

  T dot(Vector v) { return this->cwiseProduct(v).sum(); }
  T squaredNorm() { return this->dot(*this); }
  T norm() { return sqrt(squaredNorm()); }
  Vector& normalize() { *this /= norm(); return *this; }
  Vector normalized() { Vector v(*this); v.normalize(); return v; }

  Vector cwiseMin(Vector v) {
    Vector a(*this);
    for(size_t i = 0; i < D; ++i) a[i] = min(_d[i], v[i]);
    return a;
  }

  Vector cwiseMax(Vector v) {
    Vector a(*this);
    for(size_t i = 0; i < D; ++i) a[i] = max(_d[i], v[i]);
    return a;
  }

  Vector cwiseProduct(Vector v) {
    Vector a(*this);
    for(size_t i = 0; i < D; ++i) a[i] *= v[i];
    return a;
  }

  Vector cwiseQuotient(Vector v) {
    Vector a(*this);
    for(size_t i = 0; i < D; ++i) a[i] /= v[i];
    return a;
  }

  template<typename To>
  Vector<To, D> cast() {
    Vector<To, D> a;
    for(size_t i = 0; i < D; ++i) a[i] = (To)_d[i];
    return a;
  }
};


template<typename T, size_t D>
Vector<T, D> operator*(T s, Vector<T, D> v) { return v * s; }


template<typename T, size_t D>
bool operator==(Vector<T, D> a, Vector<T, D> b) {
  bool r = true;
  for(size_t i = 0; i < D; ++i) r &= (a[i] == b[i]);
  return r;
}
