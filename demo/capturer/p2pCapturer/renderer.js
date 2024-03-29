const { ipcRenderer } = require('electron');
const desktopCapturer = {
  getSources: (opts) => ipcRenderer.invoke('DESKTOP_CAPTURER_GET_SOURCES', opts)
}
// const { Menu } = require('electron').remote; // Migrating from remote
window.remote = require('@electron/remote')
console.log(window.remote)
const { Menu } = window.remote // Migrating from remote

let connectButton = document.querySelector('button#connect');
let hangupButton = document.querySelector('button#hangup');

let local_framesPerSecond = document.querySelector('div#local_framesPerSecond');
let remote_framesPerSecond = document.querySelector('div#remote_framesPerSecond');
let send_packetsLost = document.querySelector('div#send_packetsLost')
let recv_packetsLost = document.querySelector('div#recv_packetsLost')
let bitrateDiv = document.querySelector('div#bitrate');
let peerDiv = document.querySelector('div#peer');
let senderStatsDiv = document.querySelector('div#senderStats');
let receiverStatsDiv = document.querySelector('div#receiverStats');

let localCameraVideo = document.querySelector('div#localCamera video');
let remoteCameraVideo = document.querySelector('div#remoteCamera video');
let localCameraVideoStatsDiv = document.querySelector('div#localCamera div');
let remoteCameraVideoStatsDiv = document.querySelector('div#remoteCamera div');

let localScreenVideo = document.querySelector('div#localScreen video');
let remoteScreenVideo = document.querySelector('div#remoteScreen video');

let localPeerConnection;
let remotePeerConnection;
let localCameraStream;
let localScreenStream;
let bytesPrev;
let timestampPrev;
let videoType = ''

let selectObj = document.getElementById('rembAndTransportCC')
let rembAndTransportCCEnabled = true

function crashMain() {
  ipcRenderer.invoke('CRASH_MAIN')
}

function crashRenderer() {
  console.log('renderer process.crash()')
  process.crash()
}

function getGpuFeatures() {
  console.dir(JSON.stringify(window.remote.app.getGPUFeatureStatus()))
}

function getGpuInfo() {
  window.remote.app.getGPUInfo('complete').then(completeObj => {
    console.dir(JSON.stringify(completeObj))
  })
}

function setXGoogleSelect() {
  let selectedIndex = selectObj.selectedIndex
  let selectOption = selectObj.options[selectedIndex]
  let xGoogleSelect = document.getElementsByClassName('x-google-set')[0]
  if (selectOption.value === 'false') {
    xGoogleSelect.style.display = 'none'
    rembAndTransportCCEnabled = false
  } else {
    xGoogleSelect.style.display = 'block'
    rembAndTransportCCEnabled = true
  }
}

/************************************************* 取流部分 *************************************************************/

function getVideoMedia() {
  console.info('getUserMedia start!');
  closeCameraStream()
  let constraints = {
    audio: false,
    video: {
      width: { ideal: 1920 },
      height: { ideal: 1080 },
      frameRate: { ideal: 15 }
    }
  }
  console.warn("getUserMedia constraint:\n" + JSON.stringify(constraints, null, '  '));
  navigator.mediaDevices.getUserMedia(constraints).then(function (stream) {
    console.warn('getUserMedia success:', stream.id);
    connectButton.disabled = false;
    localCameraStream = stream;
    localCameraVideo.srcObject = stream;
    videoType = 'main'
  }).catch(function (e) {
    console.error(e)
    console.warn('getUserMedia error:' + e.name);
  });
}

async function getScreenCapture() {
  const inputSources = await desktopCapturer.getSources({
    types: ['window', 'screen']
  });

  const videoOptionsMenu = Menu.buildFromTemplate(
    inputSources.map(source => {
      return {
        label: source.name,
        click: () => selectSource(source)
      };
    })
  );

  videoOptionsMenu.popup();
}

async function selectSource(source) {
  console.info('getUserMedia start!');
  closeScreenStream()
  let constraints = {
    audio: false,
    video: {
      mandatory: {
        chromeMediaSource: 'desktop',
        chromeMediaSourceId: source.id,
        maxWidth: 1920,
        maxHeight: 1080,
        maxFrameRate: 5
      }
    }
  }
  console.warn("getUserMedia constraint:\n" + JSON.stringify(constraints, null, '  '));
  navigator.mediaDevices.getUserMedia(constraints).then(function (stream) {
    console.warn('getUserMedia success:', stream.id);
    connectButton.disabled = false;
    localScreenStream = stream;
    localScreenVideo.srcObject = stream;
    videoType = 'slides'
  }).catch(function (e) {
    console.error(e)
    console.warn('getUserMedia error:' + e.name);
  });
}

function closeCameraStream() {
  console.log("clear camera stream first")
  // clear first
  let stream = localCameraVideo.srcObject
  if (stream) {
    try {
      stream.oninactive = null;
      let tracks = stream.getTracks();
      for (let track in tracks) {
        tracks[track].onended = null;
        console.info("close camera stream");
        tracks[track].stop();
      }
    }
    catch (error) {
      console.info('closeCameraStream: Failed to close stream');
      console.error(error);
    }
    stream = null;
    localCameraVideo.srcObject = null
  }

  if (localCameraStream) {
    localCameraStream.getTracks().forEach(function (track) {
      track.stop();
    });
    let videoTracks = localCameraStream.getVideoTracks();
    for (let i = 0; i !== videoTracks.length; ++i) {
      videoTracks[i].stop();
    }
  }
}

function closeScreenStream() {
  console.log("clear screen stream first")
  // clear first
  let stream = localScreenVideo.srcObject
  if (stream) {
    try {
      stream.oninactive = null;
      let tracks = stream.getTracks();
      for (let track in tracks) {
        tracks[track].onended = null;
        console.info("close stream");
        tracks[track].stop();
      }
    }
    catch (error) {
      console.info('closeScreenStream: Failed to close stream');
      console.error(error);
    }
    stream = null;
    localScreenVideo.srcObject = null
  }

  if (localScreenStream) {
    localScreenStream.getTracks().forEach(function (track) {
      track.stop();
    });
    let videoTracks = localScreenStream.getVideoTracks();
    for (let i = 0; i !== videoTracks.length; ++i) {
      videoTracks[i].stop();
    }
  }
}

/************************************************* 创建p2p连接 *************************************************************/

async function createPeerConnection() {
  console.info("begin create peerConnections");
  connectButton.disabled = true;
  hangupButton.disabled = false;

  bytesPrev = 0;
  timestampPrev = 0;
  localPeerConnection = new RTCPeerConnection(null);
  remotePeerConnection = new RTCPeerConnection(null);

  if (localCameraStream) {
    localCameraStream.getTracks().forEach(
      function (track) {
        console.info("localPeerConnection addTack for camera video!");
        localPeerConnection.addTrack(track, localCameraStream);
      }
    );
  }

  // if (localScreenStream) {
  //   localScreenStream.getTracks().forEach(
  //     function (track) {
  //       console.info("localPeerConnection addTack for screen video!");
  //       localPeerConnection.addTrack(track, localScreenStream);
  //     }
  //   );
  // }

  console.info('localPeerConnection creating offer');
  localPeerConnection.onnegotiationeeded = function () {
    console.info('Negotiation needed - localPeerConnection');
  };
  remotePeerConnection.onnegotiationeeded = function () {
    console.info('Negotiation needed - remotePeerConnection');
  };
  localPeerConnection.onicecandidate = function (e) {
    console.info('Candidate localPeerConnection');
    remotePeerConnection.addIceCandidate(e.candidate).then(onAddIceCandidateSuccess, onAddIceCandidateError);
  };
  remotePeerConnection.onicecandidate = function (e) {
    console.info('Candidate remotePeerConnection');
    localPeerConnection.addIceCandidate(e.candidate).then(onAddIceCandidateSuccess, onAddIceCandidateError);
  };

  remotePeerConnection.ontrack = function (e) {
    console.log(e.streams.length)
    if (remoteCameraVideo.srcObject !== e.streams[0]) {
      console.info('remotePeerConnection got stream ' + e.streams[0].id);
      remoteCameraVideo.srcObject = e.streams[0];
    }
  };

  try {
    let offer = await localPeerConnection.createOffer()
    console.info('localPeerConnection createOffer success');
    await localPeerConnection.setLocalDescription(offer)
    console.info('localPeerConnection setLocalDescription success, sdp: \r\n', offer.sdp);

    let maxBitRate = parseInt(document.getElementById('maxBitrate').value)
    if (maxBitRate) {
      setEncodingParameters(localPeerConnection, videoType, maxBitRate)
    }

    console.log('decorate local offer sdp')
    offer.sdp = commonDecorateLo(offer.sdp)

    console.warn(`remotePeerConnection setRemoteDescription : \n${offer.sdp}`);
    await remotePeerConnection.setRemoteDescription(offer)
    console.info('remotePeerConnection setRemoteDescription success')


    // TODO: create offer process
    let answer = await remotePeerConnection.createAnswer()
    console.info('remotePeerConnection setLocalDescription: \n', answer.sdp);
    await remotePeerConnection.setLocalDescription(answer)
    console.info('remotePeerConnection setLocalDescription success')

    console.log('decorate local answer sdp')
    answer.sdp = decorateSdp(answer.sdp)
    console.warn(`localPeerConnection setRemoteDescription:\n${answer.sdp}`);
    await localPeerConnection.setRemoteDescription(answer)
    console.info('localPeerConnection setRemoteDescription success')
  } catch (e) {
    console.error(e.message)
    console.error(e)
  }
}

function onAddIceCandidateSuccess() {
  console.info('AddIceCandidate success.');
}

function onAddIceCandidateError(error) {
  console.info('Failed to add Ice Candidate: ' + error.toString());
}

function hangup() {
  console.info('Ending call');
  localPeerConnection.close();
  remotePeerConnection.close();

  // query stats one last time.
  Promise.all([
    remotePeerConnection.getStats(null).then(showRemoteStats, function (err) { console.info(err); }),
    localPeerConnection.getStats(null).then(showLocalStats, function (err) { console.info(err); })
  ]).then(() => {
    localPeerConnection = null;
    remotePeerConnection = null;
  });

  localCameraStream.getTracks().forEach(function (track) { track.stop(); });
  localCameraStream = null;
  localScreenStream.getTracks().forEach(function (track) { track.stop(); });
  localScreenStream = null;
  hangupButton.disabled = true;

  clearInterval(statisticsInterval)

  window.location.reload();
}

/**
 * show peer status
 * @param results
 */
function showRemoteStats(results) {
  let statsString = dumpStats(results);

  receiverStatsDiv.innerHTML = '<h2>Receiver stats</h2>' + statsString;
  // calculate video bitrate
  results.forEach(function (report) {
    if (report.framesPerSecond) {
      remote_framesPerSecond.innerHTML = '<strong>framesPerSecond:</strong> ' + report.framesPerSecond;
    }
    if (report.framerateMean) {
      remote_framesPerSecond.innerHTML = '<strong>framerateMean:</strong> ' + report.framerateMean;
    }
    if (report.packetsLost) {
      recv_packetsLost.innerHTML = '<strong style="color: red;">Recv packetsLost:</strong> ' + report.packetsLost;
    }

    let now = report.timestamp;
    let bitrate;
    if (report.type === 'inbound-rtp' && report.mediaType === 'video') {
      let bytes = report.bytesReceived;
      if (timestampPrev) {
        bitrate = 8 * (bytes - bytesPrev) / (now - timestampPrev);
        bitrate = Math.floor(bitrate);
      }
      bytesPrev = bytes;
      timestampPrev = now;
    }
    if (bitrate) {
      bitrate += ' kbits/sec';
      bitrateDiv.innerHTML = '<strong>Bitrate:</strong> ' + bitrate;
    }
  });

  // figure out the peer's ip
  let activeCandidatePair = null;
  let remoteCandidate = null;

  // Search for the candidate pair, spec-way first.
  results.forEach(function (report) {
    if (report.type === 'transport') {
      activeCandidatePair = results.get(report.selectedCandidatePairId);
    }
  });
  // Fallback for Firefox and Chrome legacy stats.
  if (!activeCandidatePair) {
    results.forEach(function (report) {
      if (report.type === 'candidate-pair' && report.selected ||
        report.type === 'googCandidatePair' &&
        report.googActiveConnection === 'true') {
        activeCandidatePair = report;
      }
    });
  }
  if (activeCandidatePair && activeCandidatePair.remoteCandidateId) {
    remoteCandidate = results.get(activeCandidatePair.remoteCandidateId);
  }
  if (remoteCandidate) {
    if (remoteCandidate.ip && remoteCandidate.port) {
      peerDiv.innerHTML = '<strong>Connected to:</strong> ' +
        remoteCandidate.ip + ':' + remoteCandidate.port;
    } else if (remoteCandidate.ipAddress && remoteCandidate.portNumber) {
      // Fall back to old names.
      peerDiv.innerHTML = '<strong>Connected to:</strong> ' +
        remoteCandidate.ipAddress +
        ':' + remoteCandidate.portNumber;
    }
  }
}

function showLocalStats(results) {
  let statsString = dumpStats(results);
  senderStatsDiv.innerHTML = '<h2>Sender stats</h2>' + statsString;

  results.forEach(function (report) {
    if (report.framesPerSecond) {
      local_framesPerSecond.innerHTML = '<strong>framesPerSecond:</strong> ' + report.framesPerSecond;
    }
    if (report.framerateMean) {
      local_framesPerSecond.innerHTML = '<strong>framerateMean:</strong> ' + report.framerateMean;
    }

    if (report.packetsLost) {
      send_packetsLost.innerHTML = '<strong style="color: red;">Send packetsLost:</strong> ' + report.packetsLost;
    }
  });
}

// Display statistics
let statisticsInterval = setInterval(function () {
  if (localPeerConnection && remotePeerConnection) {
    remotePeerConnection.getStats(null)
      .then(showRemoteStats, function (err) {
        console.info(err);
      });
    localPeerConnection.getStats(null)
      .then(showLocalStats, function (err) {
        console.info(err);
      });
  } else {
  }
  // Collect some stats from the video tags.
  if (localCameraVideo.videoWidth) {
    localCameraVideoStatsDiv.innerHTML = '<strong>Video dimensions:</strong> ' +
      localCameraVideo.videoWidth + 'x' + localCameraVideo.videoHeight + 'px';
  }
  if (remoteCameraVideo.videoWidth) {
    remoteCameraVideoStatsDiv.innerHTML = '<strong>Video dimensions:</strong> ' +
      remoteCameraVideo.videoWidth + 'x' + remoteCameraVideo.videoHeight + 'px';
  }
}, 1000);

// Dumping a stats letiable as a string.
// might be named toString?
function dumpStats(results) {
  let statsString = '';
  results.forEach(function (res) {
    if (res.type === 'outbound-rtp' || res.type === 'remote-inbound-rtp' || res.type === 'inbound-rtp' || res.type === 'remote-outbound-rtp') {
      statsString += '<h3>Report type=';
      statsString += res.type;
      statsString += '</h3>\n';
      statsString += 'id ' + res.id + '<br>\n';
      statsString += 'time ' + res.timestamp + '<br>\n';
      Object.keys(res).forEach(function (k) {
        if (k !== 'timestamp' && k !== 'type' && k !== 'id') {
          statsString += k + ': ' + res[k] + '<br>\n';
        }
      });
    } else {

    }
  });
  return statsString;
}
