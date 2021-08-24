#ifndef THIRD_PARTY_RNNOISE_SRC_RNN_DATA_H_
#define THIRD_PARTY_RNNOISE_SRC_RNN_DATA_H_

#include "rnn.h"

struct RNNModel {
  int input_dense_size;
  const DenseLayer *input_dense;

  int vad_gru_size;
  const GRULayer *vad_gru;

  int noise_gru_size;
  const GRULayer *noise_gru;

  int denoise_gru_size;
  const GRULayer *denoise_gru;

  int denoise_output_size;
  const DenseLayer *denoise_output;

  int vad_output_size;
  const DenseLayer *vad_output;
};

struct RNNState {
  const struct RNNModel *model;
  float *vad_gru_state;
  float *noise_gru_state;
  float *denoise_gru_state;
};

extern const struct RNNModel rnnoise_model_orig;

#endif /* THIRD_PARTY_RNNOISE_SRC_RNN_DATA_H_ */
