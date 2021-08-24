/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_AUDIO_PROCESSING_NS_RNN_NS_RNN_H_
#define MODULES_AUDIO_PROCESSING_NS_RNN_NS_RNN_H_

#include <stddef.h>
#include <sys/types.h>

#include "third_party/rnnoise/src/rnnoise.h"

namespace webrtc {
namespace rnn_ns {

// Recurrent network for noise supression.
class RnnNs {
public:
	explicit RnnNs();
	RnnNs(const RnnNs&) = delete;
	RnnNs& operator=(const RnnNs&) = delete;
	~RnnNs();

	void Process(float* out, const float* in);

private:
	struct DenoiseState* st_;
};

} // namespace rnn_ns
} // namespace webrtc

#endif // MODULES_AUDIO_PROCESSING_NS_RNN_NS_RNN_H_
