/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree.
 */
/* global TimelineDataSeries, TimelineGraphView */
const {ipcRenderer} = require('electron')

'use strict';

const instantMeter = document.querySelector('#instant meter');
const slowMeter = document.querySelector('#slow meter');
const clipMeter = document.querySelector('#clip meter');

const audioInputSelect = document.querySelector('select#audioSource');

const instantValueDisplay = document.querySelector('#instant .value');
const slowValueDisplay = document.querySelector('#slow .value');
const clipValueDisplay = document.querySelector('#clip .value');

const startButton0 = document.querySelector('button#startButton0');
const stopButton0 = document.querySelector('button#stopButton0');
const muteButton0 = document.querySelector('button#muteButton0');
startButton0.onclick = start0;
stopButton0.onclick = stop0;
muteButton0.onclick = mute0;

const startButton = document.querySelector('button#startButton');
const stopButton = document.querySelector('button#stopButton');
const muteButton = document.querySelector('button#muteButton');
startButton.onclick = start;
stopButton.onclick = stop;
muteButton.onclick = mute;


const audio2 = document.querySelector('audio#audio2');
const callButton = document.querySelector('button#callButton');
const hangupButton = document.querySelector('button#hangupButton');
const codecSelector = document.querySelector('select#codec');
callButton.onclick = call;
hangupButton.onclick = hangup;


let meterRefresh = null;

let pc1;
let pc2;
let localStream0;
let localStream;

let bitrateGraph;
let bitrateSeries;
let headerrateSeries;

let packetGraph;
let packetSeries;

let lastResult;

const offerOptions = {
  offerToReceiveAudio: 1,
  offerToReceiveVideo: 0,
  voiceActivityDetection: false
};

const audioLevels = [];
let audioLevelGraph;
let audioLevelSeries;

// Enabling opus DTX is an expert option without GUI.
// eslint-disable-next-line prefer-const
let useDtx = false;

// Disabling Opus FEC is an expert option without GUI.
// eslint-disable-next-line prefer-const
let useFec = true;

// We only show one way of doing this.
const codecPreferences = document.querySelector('#codecPreferences');
const supportsSetCodecPreferences = window.RTCRtpTransceiver &&
  'setCodecPreferences' in window.RTCRtpTransceiver.prototype;
if (supportsSetCodecPreferences) {
  codecSelector.style.display = 'none';

  const {codecs} = RTCRtpSender.getCapabilities('audio');
  codecs.forEach(codec => {
    if (['audio/CN', 'audio/telephone-event'].includes(codec.mimeType)) {
      return;
    }
    const option = document.createElement('option');
    option.value = (codec.mimeType + ' ' + codec.clockRate + ' ' +
      (codec.sdpFmtpLine || '')).trim();
    option.innerText = option.value;
    codecPreferences.appendChild(option);
  });
  codecPreferences.disabled = false;
} else {
  codecPreferences.style.display = 'none';
}

// Change the ptime. For opus supported values are [10, 20, 40, 60].
// Expert option without GUI.
// eslint-disable-next-line no-unused-vars
async function setPtime(ptime) {
  const offer = await pc1.createOffer();
  await pc1.setLocalDescription(offer);
  const desc = pc1.remoteDescription;
  if (desc.sdp.indexOf('a=ptime:') !== -1) {
    desc.sdp = desc.sdp.replace(/a=ptime:.*/, 'a=ptime:' + ptime);
  } else {
    desc.sdp += 'a=ptime:' + ptime + '\r\n';
  }
  await pc1.setRemoteDescription(desc);
}


const selectors = [audioInputSelect];

function gotDevicesSuccess(deviceInfos) {
  // Handles being called several times to update labels. Preserve values.
  const values = selectors.map(select => select.value);
  selectors.forEach(select => {
    while (select.firstChild) {
      select.removeChild(select.firstChild);
    }
  });

  for (let i = 0; i !== deviceInfos.length; ++i) {
    const deviceInfo = deviceInfos[i];
    const option = document.createElement('option');
    option.value = deviceInfo.deviceId;
    if (deviceInfo.kind === 'audioinput') {
      option.text = deviceInfo.label || `microphone ${audioInputSelect.length + 1}`;
      audioInputSelect.appendChild(option);
    } else if (deviceInfo.kind === 'audiooutput') {
    } else if (deviceInfo.kind === 'videoinput') {
    } else {
      console.log('Some other kind of source/device: ', deviceInfo);
    }
  }

  selectors.forEach((select, selectorIndex) => {
    if (Array.prototype.slice.call(select.childNodes).some(n => n.value === values[selectorIndex])) {
      select.value = values[selectorIndex];
    }
  });
}

function gotDevicesError(error) {
  console.log('gotDevicesError: ', error.message, error.name);
}

navigator.mediaDevices.enumerateDevices().then(gotDevicesSuccess).catch(gotDevicesError);


function start0() {
  console.log('Requesting local stream 0');

  try {
    window.AudioContext = window.AudioContext || window.webkitAudioContext;
    window.audioContext = new AudioContext();
  } catch (e) {
    alert('Web Audio API not supported.');
  }

  const audioSource = audioInputSelect.value;
  const constraints0 = {
    audio: {
      deviceId: {exact: audioSource}
    },
    video: false
  };
  console.warn('Requesting local stream 0 with deviceId=', constraints0.audio.deviceId);
  navigator.mediaDevices
    .getUserMedia(constraints0)
    .then(gotStreamSuccess0)
    .catch(gotStreamError0);
}

function stop0() {
	console.log('Stopping local stream 0');
  startButton0.disabled = false;
  stopButton0.disabled = true;
  muteButton0.disabled = true;

  localStream0.getTracks().forEach(track => track.stop());

  window.soundMeter.stop();
  window.audioContext.close();
  clearInterval(meterRefresh);

  instantMeter.value = instantValueDisplay.innerText = '';
  slowMeter.value = slowValueDisplay.innerText = '';
  clipMeter.value = clipValueDisplay.innerText = '';
}

function mute0() {
	console.log('Mute local stream 0');
	const audioTracks = localStream0.getAudioTracks();
	if (audioTracks.length > 0) {
		audioTracks[0].enabled = !audioTracks[0].enabled;
    muteButton0.innerText = audioTracks[0].enabled ? 'Mute' : 'Unmute';
	}
}

function onAudio0Inactive() {
  console.warn("onAudio0Inactive");
}

function gotStreamSuccess0(stream) {
  console.warn('Received local stream 0');
  startButton0.disabled = true;
  stopButton0.disabled = false;
  muteButton0.disabled = false;

  localStream0 = stream;
  localStream0.oninactive = onAudio0Inactive;

  const audioTracks = localStream0.getAudioTracks();
  if (audioTracks.length > 0) {
    console.log(`Using Audio device: ${audioTracks[0].label}`);

    capabilities = audioTracks[0].getCapabilities();
    console.log('capabilities:', capabilities);

    constraints = audioTracks[0].getConstraints();
    console.log('constraints:', constraints);

    settings = audioTracks[0].getSettings();
    console.warn('settings:', settings);
  }

  const soundMeter = window.soundMeter = new SoundMeter(window.audioContext);
  soundMeter.connectToSource(localStream0, function(e) {
    if (e) {
      alert(e);
      return;
    }
    meterRefresh = setInterval(() => {
      instantMeter.value = instantValueDisplay.innerText =
        soundMeter.instant.toFixed(2);
      slowMeter.value = slowValueDisplay.innerText =
        soundMeter.slow.toFixed(2);
      clipMeter.value = clipValueDisplay.innerText =
        soundMeter.clip;
    }, 200);
  });  
}

function gotStreamError0(error) {
  console.warn('gotStreamError0: ', error.message, error.name);
}

function start() {
	console.log('Requesting local stream');

  const audioSource = audioInputSelect.value;
  const hasNoiseSuppression = document.querySelector('#noiseSuppression').checked;
  const hasRnnNoiseSuppression = document.querySelector('#rnnNoiseSuppression').checked;
  const constraints = {
		audio: {
			deviceId: {exact: audioSource},
			noiseSuppression: {exact: hasNoiseSuppression},
			rnnNoiseSuppression: {exact: hasRnnNoiseSuppression}
		},
    video: false
  };
  console.warn('Requesting local stream with deviceId=', constraints.audio.deviceId, ', noiseSuppression=', constraints.audio.noiseSuppression, ', rnnNoiseSuppression=', constraints.audio.rnnNoiseSuppression);
  navigator.mediaDevices
    .getUserMedia(constraints)
    .then(gotStreamSuccess)
    .catch(gotStreamError);
}

function stop() {
	console.log('Stopping local stream');
  startButton.disabled = false;
  stopButton.disabled = true;
  muteButton.disabled = true;

  localStream.getTracks().forEach(track => track.stop());
}

function mute() {
	console.log('Mute local stream');
	const audioTracks = localStream.getAudioTracks();
	if (audioTracks.length > 0) {
		audioTracks[0].enabled = !audioTracks[0].enabled;
    muteButton.innerText = audioTracks[0].enabled ? 'Mute' : 'Unmute';
	}
}

function onAudioInactive() {
  console.warn("onAudioInactive");
}

function gotStreamSuccess(stream) {
  console.warn('Received local stream');
  startButton.disabled = true;
  stopButton.disabled = false;
  muteButton.disabled = false;

  localStream = stream;
  localStream.oninactive = onAudioInactive;

  const audioTracks = localStream.getAudioTracks();
  if (audioTracks.length > 0) {
    console.log(`Using Audio device: ${audioTracks[0].label}`);

    capabilities = audioTracks[0].getCapabilities();
    console.log('capabilities:', capabilities);

    constraints = audioTracks[0].getConstraints();
    console.log('constraints:', constraints);

    settings = audioTracks[0].getSettings();
    console.warn('settings:', settings);
  }
}

function gotStreamError(error) {
  console.warn('gotStreamError: ', error);
}

function onCreateSessionDescriptionError(error) {
  console.log(`Failed to create session description: ${error.toString()}`);
}

function call() {
  console.log('Starting call');
  callButton.disabled = true;
  hangupButton.disabled = false;
  codecSelector.disabled = true;

  const servers = null;
  pc1 = new RTCPeerConnection(servers);
  console.log('Created local peer connection object pc1');
  pc1.onicecandidate = e => onIceCandidate(pc1, e);
  pc2 = new RTCPeerConnection(servers);
  console.log('Created remote peer connection object pc2');
  pc2.onicecandidate = e => onIceCandidate(pc2, e);
  pc2.ontrack = gotRemoteStream;

  console.warn('Adding Local Stream to peer connection');
  localStream.getTracks().forEach(track => pc1.addTrack(track, localStream));

  if (supportsSetCodecPreferences) {
    const preferredCodec = codecPreferences.options[codecPreferences.selectedIndex];
    if (preferredCodec.value !== '') {
      const [mimeType, clockRate, sdpFmtpLine] = preferredCodec.value.split(' ');
      const {codecs} = RTCRtpSender.getCapabilities('audio');
      console.log(mimeType, clockRate, sdpFmtpLine);
      console.log(JSON.stringify(codecs, null, ' '));
      const selectedCodecIndex = codecs.findIndex(c => c.mimeType === mimeType && c.clockRate === parseInt(clockRate, 10) && c.sdpFmtpLine === sdpFmtpLine);
      const selectedCodec = codecs[selectedCodecIndex];
      codecs.splice(selectedCodecIndex, 1);
      codecs.unshift(selectedCodec);
      const transceiver = pc1.getTransceivers().find(t => t.sender && t.sender.track === localStream.getAudioTracks()[0]);
      transceiver.setCodecPreferences(codecs);
      console.log('Preferred video codec', selectedCodec);
    }
  }

  pc1.createOffer(offerOptions)
      .then(gotDescription1, onCreateSessionDescriptionError);

  bitrateSeries = new TimelineDataSeries();
  bitrateGraph = new TimelineGraphView('bitrateGraph', 'bitrateCanvas');
  bitrateGraph.updateEndDate();

  headerrateSeries = new TimelineDataSeries();
  headerrateSeries.setColor('green');

  packetSeries = new TimelineDataSeries();
  packetGraph = new TimelineGraphView('packetGraph', 'packetCanvas');
  packetGraph.updateEndDate();

  audioLevelSeries = new TimelineDataSeries();
  audioLevelGraph = new TimelineGraphView('audioLevelGraph', 'audioLevelCanvas');
  audioLevelGraph.updateEndDate();
}

function gotDescription1(desc) {
  console.log(`Offer from pc1\n${desc.sdp}`);
  pc1.setLocalDescription(desc)
      .then(() => {
        if (!supportsSetCodecPreferences) {
          desc.sdp = forceChosenAudioCodec(desc.sdp);
        }
        pc2.setRemoteDescription(desc).then(() => {
          return pc2.createAnswer().then(gotDescription2, onCreateSessionDescriptionError);
        }, onSetSessionDescriptionError);
      }, onSetSessionDescriptionError);
}

function gotDescription2(desc) {
  console.log(`Answer from pc2\n${desc.sdp}`);
  pc2.setLocalDescription(desc).then(() => {
    if (!supportsSetCodecPreferences) {
      desc.sdp = forceChosenAudioCodec(desc.sdp);
    }
    if (useDtx) {
      desc.sdp = desc.sdp.replace('useinbandfec=1', 'useinbandfec=1;usedtx=1');
    }
    if (!useFec) {
      desc.sdp = desc.sdp.replace('useinbandfec=1', 'useinbandfec=0');
    }
    pc1.setRemoteDescription(desc).then(() => {}, onSetSessionDescriptionError);
  }, onSetSessionDescriptionError);
}

function hangup() {
  console.log('Ending call');

  pc1.close();
  pc2.close();
  pc1 = null;
  pc2 = null;

  callButton.disabled = false;
  hangupButton.disabled = true;
  codecSelector.disabled = false;
}

function gotRemoteStream(e) {
  if (audio2.srcObject !== e.streams[0]) {
    console.warn('Received remote stream');
    audio2.srcObject = e.streams[0];
  }
}

function getOtherPc(pc) {
  return (pc === pc1) ? pc2 : pc1;
}

function getName(pc) {
  return (pc === pc1) ? 'pc1' : 'pc2';
}

function onIceCandidate(pc, event) {
  getOtherPc(pc).addIceCandidate(event.candidate)
      .then(
          () => onAddIceCandidateSuccess(pc),
          err => onAddIceCandidateError(pc, err)
      );
  console.log(`${getName(pc)} ICE candidate:\n${event.candidate ? event.candidate.candidate : '(null)'}`);
}

function onAddIceCandidateSuccess() {
  console.log('AddIceCandidate success.');
}

function onAddIceCandidateError(error) {
  console.log(`Failed to add ICE Candidate: ${error.toString()}`);
}

function onSetSessionDescriptionError(error) {
  console.log(`Failed to set session description: ${error.toString()}`);
}

function forceChosenAudioCodec(sdp) {
  return maybePreferCodec(sdp, 'audio', 'send', codecSelector.value);
}

// Copied from AppRTC's sdputils.js:

// Sets |codec| as the default |type| codec if it's present.
// The format of |codec| is 'NAME/RATE', e.g. 'opus/48000'.
function maybePreferCodec(sdp, type, dir, codec) {
  const str = `${type} ${dir} codec`;
  if (codec === '') {
    console.log(`No preference on ${str}.`);
    return sdp;
  }

  console.log(`Prefer ${str}: ${codec}`);

  const sdpLines = sdp.split('\r\n');

  // Search for m line.
  const mLineIndex = findLine(sdpLines, 'm=', type);
  if (mLineIndex === null) {
    return sdp;
  }

  // If the codec is available, set it as the default in m line.
  const codecIndex = findLine(sdpLines, 'a=rtpmap', codec);
  console.log('codecIndex', codecIndex);
  if (codecIndex) {
    const payload = getCodecPayloadType(sdpLines[codecIndex]);
    if (payload) {
      sdpLines[mLineIndex] = setDefaultCodec(sdpLines[mLineIndex], payload);
    }
  }

  sdp = sdpLines.join('\r\n');
  return sdp;
}

// Find the line in sdpLines that starts with |prefix|, and, if specified,
// contains |substr| (case-insensitive search).
function findLine(sdpLines, prefix, substr) {
  return findLineInRange(sdpLines, 0, -1, prefix, substr);
}

// Find the line in sdpLines[startLine...endLine - 1] that starts with |prefix|
// and, if specified, contains |substr| (case-insensitive search).
function findLineInRange(sdpLines, startLine, endLine, prefix, substr) {
  const realEndLine = endLine !== -1 ? endLine : sdpLines.length;
  for (let i = startLine; i < realEndLine; ++i) {
    if (sdpLines[i].indexOf(prefix) === 0) {
      if (!substr ||
        sdpLines[i].toLowerCase().indexOf(substr.toLowerCase()) !== -1) {
        return i;
      }
    }
  }
  return null;
}

// Gets the codec payload type from an a=rtpmap:X line.
function getCodecPayloadType(sdpLine) {
  const pattern = new RegExp('a=rtpmap:(\\d+) \\w+\\/\\d+');
  const result = sdpLine.match(pattern);
  return (result && result.length === 2) ? result[1] : null;
}

// Returns a new m= line with the specified codec as the first one.
function setDefaultCodec(mLine, payload) {
  const elements = mLine.split(' ');

  // Just copy the first three parameters; codec order starts on fourth.
  const newLine = elements.slice(0, 3);

  // Put target payload first and copy in the rest.
  newLine.push(payload);
  for (let i = 3; i < elements.length; i++) {
    if (elements[i] !== payload) {
      newLine.push(elements[i]);
    }
  }
  return newLine.join(' ');
}

// query getStats every second
window.setInterval(() => {
  if (!pc1) {
    return;
  }
  const sender = pc1.getSenders()[0];
  if (!sender) {
    return;
  }
  sender.getStats().then(res => {
    res.forEach(report => {
      let bytes;
      let headerBytes;
      let packets;
      if (report.type === 'outbound-rtp') {
        if (report.isRemote) {
          return;
        }
        const now = report.timestamp;
        bytes = report.bytesSent;
        headerBytes = report.headerBytesSent;

        packets = report.packetsSent;
        if (lastResult && lastResult.has(report.id)) {
          const deltaT = now - lastResult.get(report.id).timestamp;
          // calculate bitrate
          const bitrate = 8 * (bytes - lastResult.get(report.id).bytesSent) /
            deltaT;
          const headerrate = 8 * (headerBytes - lastResult.get(report.id).headerBytesSent) /
            deltaT;

          // append to chart
          bitrateSeries.addPoint(now, bitrate);
          headerrateSeries.addPoint(now, headerrate);
          bitrateGraph.setDataSeries([bitrateSeries, headerrateSeries]);
          bitrateGraph.updateEndDate();

          // calculate number of packets and append to chart
          packetSeries.addPoint(now, 1000 * (packets -
            lastResult.get(report.id).packetsSent) / deltaT);
          packetGraph.setDataSeries([packetSeries]);
          packetGraph.updateEndDate();
        }
      }
    });
    lastResult = res;
  });
}, 1000);

if (window.RTCRtpReceiver && ('getSynchronizationSources' in window.RTCRtpReceiver.prototype)) {
  let lastTime;
  const getAudioLevel = (timestamp) => {
    window.requestAnimationFrame(getAudioLevel);
    if (!pc2) {
      return;
    }
    const receiver = pc2.getReceivers().find(r => r.track.kind === 'audio');
    if (!receiver) {
      return;
    }
    const sources = receiver.getSynchronizationSources();
    sources.forEach(source => {
      audioLevels.push(source.audioLevel);
    });
    if (!lastTime) {
      lastTime = timestamp;
    } else if (timestamp - lastTime > 500 && audioLevels.length > 0) {
      // Update graph every 500ms.
      const maxAudioLevel = Math.max.apply(null, audioLevels);
      audioLevelSeries.addPoint(Date.now(), maxAudioLevel);
      audioLevelGraph.setDataSeries([audioLevelSeries]);
      audioLevelGraph.updateEndDate();
      audioLevels.length = 0;
      lastTime = timestamp;
    }
  };
  window.requestAnimationFrame(getAudioLevel);
}

const supports = navigator.mediaDevices.getSupportedConstraints();
console.warn('supports:', supports);
if (!supports["rnnNoiseSuppression"]) {
	console.warn('rnnNoiseSuppression unsupported');
	document.querySelector('#rnnNoiseSuppression').disabled = true;
}
