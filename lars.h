#ifndef LARS__H
#define LARS__H

#include <cstdio>
#include <iostream>
#include <fstream>
#include <iterator>
#include <numeric>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <valarray>

#include "dense_cholesky.h"

using namespace std;

template< typename T >
class Lars {
 public:
  typedef typename T::real real;

  template<typename U>
#ifdef DEBUG_PRINT
  void print( const U& v ) {
    for(int i=0; i<v.size(); ++i)
      fprintf(fid, "%12.5f", v[i]);
    fprintf(fid, "\n");
  }
#endif

  /** get the current parameters */
  const vector<pair<int,real> >& getParameters();
  /** get least squares parameters for active set */
  const void getParameters(vector<pair<int,real> >* p,
		     const vector<pair<int,real> >& b);

  /** Constructor accepts a LarsDenseData object */
  Lars( T& data):
    data_(data),
    chol_( min(data.nrows(),data.ncols())),
  {
    initialize();
  }

  ~Lars() {
#ifdef DEBUG_PRINT
    fprintf(fid, "DONE\n");
#endif

    fclose(fid);
  }
  /** Perform a single interation of the LARS loop. */
  bool iterate(){
    if(vars >= nvars ) return false;
    // if( beta_.size() >= data_.ncols() ) return false;
    k++;
#ifdef DEBUG_PRINT
    fprintf(fid, "K: %12d\n", k );
    fprintf(fid, "%12d %12d\n", vars, nvars);
#endif
    // [C j] = max(abs(c(I)));
    // j = I(j);
    real C; int j;
    GetMaxAbsCorrelation(j, C);

#ifdef DEBUG_PRINT
    fprintf(fid, "[C,j] = [%12.5f, %12d]\n", C, j+1 );
#endif
#ifdef DEBUG_PRINT
    fprintf(fid, "activating %d\n", j+1 );
#endif
    activate(j);
    // computes w, AA, and X'w
    real AA = findSearchDirection();
#ifdef DEBUG_PRINT
    fprintf(fid, "W:");
    print(w_);
    fprintf(fid, "AA: %12.5f\n", AA);
#endif
    real gamma;
    if( vars == nvars ) {
      gamma = C/AA;
#ifdef DEBUG_PRINT
      fprintf(fid, "gamma: %12.5f\n", gamma);
#endif
    } else {
      //fprintf(fid, "a:");
      //print(a_);
      gamma = C/AA;
      int min_index = -1;
      // temp = [(C - c(I))./(AA - a(I)); (C + c(I))./(AA + a(I))];
      // gamma = min([temp(temp > 0); C/AA]);
      for(int j=0; j<a_.size(); ++j) {
        // only consider inactive features
        if(isActive(j)) continue;
        real t1 = (C - c_[j])/(AA - a_[j]);
        real t2 = (C + c_[j])/(AA + a_[j]);
        // consider only positive items
        if( t1 > 0 && t1 < gamma ) {
          gamma = t1; min_index = j;
        }
        if( t2 > 0 && t2 < gamma ) {
          gamma = t2; min_index = j;
        }
      }
#ifdef DEBUG_PRINT
      fprintf(fid, "min_index: %12d\n", min_index+1);
      fprintf(fid, "gamma: %12.5f\n", gamma);
#endif
    }
    // add lambda * w to beta
    for(int i=0; i<beta_.size(); ++i)
      beta_[i].second += gamma * w_[i];

    // update correlation with a
    for(int i=0; i<c_.size(); ++i)
      c_[i] -= gamma * a_[i];

    // print the beta
#ifdef DEBUG_PRINT
    fprintf(fid, "beta: ");
    for(int i=0; i<beta_.size(); ++i) {
      fprintf(fid, "%12.5f", beta_[i].second );
    }
    fprintf(fid, "\n");
#endif
    return true;
  }

 private:
  /** initialize the state of LARS */
  void initialize();

  /** returns true if parameter i is active */
  bool isActive( int i );

  /** activate parameter i and updates cholesky */
  bool activate( int i );

  /** deactivates parameter i and downdates cholesky */
  void deactivate( int i );


  /**
   * Function: sign
   * --------------
   *  returns sign of input (1,0,-1)
   */
  inline real sign( real temp ) {
    if( temp > 0 ) return 1.0;
    if( temp < 0 ) return  -1.0;
    return 0;
  }

  /**
   * Function: findSearchDirection
   * -----------------------------
   *
   * Solves for the step in parameter space given the current
   *  active parameters.
   *
   *  GA1 = R\(R'\s);
   *  AA = 1/sqrt(dot(GA1,s));
   *  w = AA*GA1;
   *  returns AA
   **/

  real findSearchDirection() {
    w_.resize(beta_.size());
    assert(w_.size()==beta_.size());
    // set w_ = sign(c_[A])
    for(int i=0; i<w_.size(); ++i){
      w_[i] = sign(c_[beta_[i].first]);
    }
    //fprintf(fid, "sign(c_[A]):");
    //print(w_);
    // w_ = R\(R'\s)
    chol_.solve(w_, &w_ );
    //fprintf(fid, "w_:");
    //print(w_);

    // AA = 1/sqrt(dot(GA1,s));
    real AA(0.0);
    for(int i=0; i<w_.size(); ++i) AA += w_[i]*sign(c_[beta_[i].first]);
    AA = real(1.0)/sqrt(AA);
    //fprintf(fid, "AA: %12.5f\n", AA);
    for(int i=0; i<w_.size(); ++i) w_[i] *= AA;


    // calculate the a (uses beta to get active indices )
    // a_ = X'Xw
    data_.compute_direction_correlation( beta_, w_, &(a_[0]) );

    return AA;
  }


  // member variables
  T& data_; // data(contains X and y)
  vector<pair<int,real> > beta_;   // current parameters(solution) [Not Sorted]

  // incrementally updated quantities
  valarray<int> active_; // active[i] = position in beta of active param or -1
  vector<real> c_; // correlation of columns of X with current residual
  vector<real> w_;          // step direction ( w_.size() == # active )
  valarray<real> a_;   // correlation of columns of X with current step dir
  DenseCholesky<real> chol_;   // keeps track of cholesky
  // temporaries
  valarray<real> temp_;      // temporary storage for active correlations


  /** New Variable to exactly replicate matlab lars */
  vector<real> Xty; // correlation of columns of X with current residual
  bool stopcond;
  int vars;
  int nvars;
  int k;
  FILE* fid;

  /**
   * Function: GetMaxCorrelation
   * ---------------------------
   * [C j] = max(abs(c(I)));
   * j = I(j);
   */
  void GetMaxAbsCorrelation( int& index, real& val ) {
    val = real(0);
    for(int i=0; i<c_.size(); ++i){
      if( isActive(i) ) continue;
      if( fabs(c_[i]) > val ) {
        index = i;
        val = fabs(c_[i]);
      }
    }
  }
};

/** Initialization routine. */
template<typename T>
void Lars<T>::initialize(){
  // initially all parameters are 0
  // so current residual is y

  data_.getXtY( &Xty );
  nvars = min<int>(data_.nrows()-1,data_.ncols());
  stopcond = 0;
  k = 0;
  vars = 0;
  fid = fopen("vlarspp_debug.txt","w");
 // fid = stderr;

  data_.getXtY( &c_ );
  // step dir = 0 so a_ = 0
  a_.resize(c_.size());
  active_.resize(data_.ncols(),-1);
  temp_.resize(data_.ncols());
}

/** Returns bool of whether that row is active. */
template<typename T>
inline bool Lars<T>::isActive(int i) {
  return active_[i] != -1;
}

/**
 * Function: activate
 * ------------------
 * Update state so that feature i is active with weight 0
 *  if it is not already active.
 *
 * R = cholinsert(R,X(:,j),X(:,A));
 * A = [A j];
 * I(I == j) = [];
 * vars = vars + 1;
 *
 **/
template<typename T>
bool Lars<T>::activate(int i) {
  //fprintf(fid, "activate(%d)\n", i );
  if(isActive(i) || beta_.size() >= data_.nrows()) {
    fprintf(fid, "blash\n");
    return false;
  }
  active_[i] = beta_.size();
  beta_.push_back(make_pair(i,0.0));
  w_.resize(beta_.size());
  //fprintf(fid, "beta.size(): %d\n", beta_.size());
  // dot i with all the other active columns f => xtx(i,j)
  for(int f=0; f<beta_.size(); ++f){
    temp_[f] = data_.col_dot_product(i, beta_[f].first );
  }
  chol_.addRowCol( &temp_[0] );
  vars++;
  // fprintf(fid, "vars %d\n", vars );
  return true;
}


/**
 * Function: deactivate
 * --------------------
 * Update state so that feature i is no longer active if it is
 * already active.
 *
 *  R = choldelete(R,j);
 *  I = [I A(j)];
 *  A(j) = [];
 *  vars = vars - 1;
 **/
template<typename T>
void Lars<T>::deactivate(int i) {
  assert(!isActive(i));
  int beta_index = active_[i];
  beta_.erase( beta_.begin() + beta_index ); // check this!!
  active_[i] = -1;
  chol_.removeRowCol( beta_index );
  // fix the active set!
  for(int r=0; r<active_.size(); ++r)
    active_[r] = -1;
  for(int r=0; r<beta_.size(); ++r)
    active_[beta_[r].first]=r;
  vars--;
}


/** Return a reference to the current active set of beta parameters. */
template<typename T>
const vector<pair<int,typename T::real> >& Lars<T>::getParameters() {
  return beta_;
}

/** Return the Least-squares solution to X*beta = y for the subset
 * of currently active beta parameters */
template<typename T>
const void Lars<T>::
getParameters(vector<pair<int,typename T::real> >* p,
	const vector<pair<int,typename T::real> >& b) {

  vector<real> temp(c_.size());
  vector<real> temp2(w_.size());

  p->resize(b.size());
  data_.getXtY( &temp );
  for(int i=0; i<b.size(); ++i){
    temp2[i] = temp[b[i].first];
  }
  chol_.solve(temp2, &temp2 );
  for(int i=0; i<b.size(); ++i){
    (*p)[i].first=b[i].first;
    (*p)[i].second=temp2[i];
  }
}
#endif
