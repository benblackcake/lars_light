#ifndef TIMER_ID_H
#define TIMER_ID_H

enum TIMER_ID{
  INIT_CORRELATION,
  GET_ACTIVE_IDX,
  FUSED_CHOLESKY,
  GET_A,
  GET_GAMMA,
  UPDATE_BETA,
  COMPUTE_LAMBDA,
  END_ITR
};

static const char *TIMER_ID_STR[] = {
  "INIT_CORRELATION",
  "GET_ACTIVE_IDX",
  "FUSED_CHOLESKY",
  "GET_A",
  "GET_GAMMA",
  "UPDATE_BETA",
  "COMPUTE_LAMBDA",
  "END_ITR"
};

#endif
