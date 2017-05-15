#include <cstdarg>
#include <cstdlib>
#include <cmath>
#include <cstdio>

#ifndef UTIL_H
#define UTIL_H

typedef float Real;

struct Idx {
  int id;
  Real v;

  Idx(int id_in, Real v_in): id(id_in), v(v_in) {}
};

inline Real sign(Real tmp) {
  if (tmp > 0) return 1.0;
  if (tmp < 0) return -1.0;
  return 0;
}

#define DEBUG
inline void print(const char *format, ...) {
#ifdef DEBUG
  va_list arg;
  char buf[1000];
  va_start(arg, format);
  vsnprintf(buf, sizeof(buf), format, arg);
  va_end(arg);
  printf("%s", buf);
  fflush(stdout);
#endif
}

template <class T>
inline T normalRand(T mean = T(0), T stdev = T(1)) {
  const double norm = 1.0/(RAND_MAX + 1.0);
  double u = 1.0 - std::rand()*norm;
  double v = rand()*norm;
  double z = sqrt(-2.0*log(u))*cos(2.0*M_PI*v);
  return T(mean + stdev*z);
}

template <class T>
inline void prepareData(const int D, const int K, const int r,
			                  const bool normalize, T *Xt, T *y) {
  prepare_Xt(D, K, normalize, Xt);
  prepare_Yt(D, r, y);
}

/*
create random Xt(K rows, D cols) and normalize each row of Xt
*/
template <class T>
inline void prepare_Xt(const int D, const int K, const bool normalize, T *Xt) {
  for (int j = 0, k = 0; j < K; j++) {
    T sum = T(0);
    T sum2 = T(0);
    for (int i=0;i<D;i++,k++) {
      T v = normalRand<T>();
      Xt[k] = v;
      sum += v;
      sum2 += v*v;
    }
    if (normalize) {
      T std = sqrt(sum2 - sum*sum/T(D));
      k -= D;
      for (int i=0;i<D;i++,k++) {
         Xt[k] = (Xt[k] - sum/T(D))/std;
      }
    }
  }
}
/*
create random Yt(r rows, D cols)
*/
template <class T>
inline void prepare_Yt(const int D, const int r, T *y) {
  for (int i=0;i<D*r;i++) {
    y[i] = normalRand<T>();
  }
}

/*
create random BETAt(r rows, K cols)
*/
template <class T>
inline void prepare_Beta(const int K, const int r, T *beta) {
  for (int i=0;i<K*r;i++) {
    beta[i] = normalRand<T>();
  }
}

inline Real get_square_error(const Real *beta, const Real *beta_h, const int size) {
  Real sqr_err = 0.0;
  for (int i = 0; i < size; i++) {
    sqr_err += (beta[i]-beta_h[i]) * (beta[i]-beta_h[i]);
  }
  return sqr_err;
}
#endif
