/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <algorithm>

#include "modules/audio_processing/ns/rnn_ns/rnn.h"

namespace webrtc {
namespace rnn_ns {

RnnNs::RnnNs() {
	st_ = rnnoise_create(NULL);
}

RnnNs::~RnnNs() {
	rnnoise_destroy(st_);
}

void RnnNs::Process(float* out, const float* in) {
	rnnoise_process_frame(st_, out, in);
	for (size_t i = 0; i < 480; i++) {
		out[i] = std::min(std::max(out[i], -32768.f), 32767.f);
	}
}

} // namespace rnn_ns
} // namespace webrtc
