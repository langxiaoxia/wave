// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/audio_device_description_mojom_traits.h"

namespace mojo {
// static
bool StructTraits<audio::mojom::AudioDeviceDescriptionDataView,
                  media::AudioDeviceDescription>::
    Read(audio::mojom::AudioDeviceDescriptionDataView data,
         media::AudioDeviceDescription* output) {
  return data.ReadDeviceName(&output->device_name) &&
         data.ReadUniqueId(&output->unique_id) &&
         data.ReadGroupId(&output->group_id) && 
         data.ReadContainerId(&output->container_id); //+by xxlang2021-09-06
}

}  // namespace mojo
