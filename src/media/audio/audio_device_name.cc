// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_device_name.h"

#include <utility>

#include "media/audio/audio_device_description.h"

namespace media {

AudioDeviceName::AudioDeviceName() = default;

AudioDeviceName::AudioDeviceName(std::string device_name, std::string unique_id, std::string container_id) //+by xxlang@2021-09-06
    : device_name(std::move(device_name)), unique_id(std::move(unique_id)), container_id(std::move(container_id)) {}

// static
AudioDeviceName AudioDeviceName::CreateDefault() {
  return AudioDeviceName(std::string(),
                         AudioDeviceDescription::kDefaultDeviceId,
                         std::string()); //+by xxlang@2021-09-06
}

// static
AudioDeviceName AudioDeviceName::CreateCommunications() {
  return AudioDeviceName(std::string(),
                         AudioDeviceDescription::kCommunicationsDeviceId,
                         std::string()); //+by xxlang@2021-09-06
}

}  // namespace media
