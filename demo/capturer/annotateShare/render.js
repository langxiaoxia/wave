const { desktopCapturer } = require('electron');

const startCameraBtn = document.querySelector('button#startCamera');
const stopCameraBtn = document.querySelector('button#stopCamera');
const cameraVideoElement = document.getElementById('cameraVideo');

const startScreenBtn = document.querySelector('button#startScreen');
const stopScreenBtn = document.querySelector('button#stopScreen');
const screenVideoElement = document.getElementById('screenVideo');
const thumbListElement = document.getElementById('thumbList');

let imageCapture;
var imageClipElement = document.querySelector('#imageClip');

async function listShare() {
	while(thumbListElement.lastChild) {
		thumbListElement.removeChild(thumbListElement.lastChild);
	}

	const inputSources = await desktopCapturer.getSources({
		types: ['window', 'screen'],
		thumbnailSize: {width: 200, height: 200},
	});

	inputSources.map(source => {
		let imgEle = document.createElement("img");
		imgEle.src = source.thumbnail.toDataURL();
		imgEle.title = source.name;
		imgEle.id = source.id;
		imgEle.ondblclick = function() {
			selectSource(this.id);
		}
		thumbListElement.appendChild(imgEle);
	})
}

function onCameraInactive() {
  console.log("onCameraInactive");
  startCameraBtn.disabled = false;
  stopCameraBtn.disabled = true;
  cameraVideoElement.srcObject = null;
  imageCapture = null;
  imageClipElement.src = '';
}

async function startCamera() {
  const constraints_without_audio = {
    audio: false,
    video: {
      cursor: "always",
      frameRate: 30
    }
  };

  const constraints_with_audio = {
    audio: true,
    video: {
      cursor: "always",
      frameRate: 30
    }
  };

  let stream = await navigator.mediaDevices.getUserMedia(constraints_without_audio);
  if (stream && stream.active) {
    stream.oninactive = onCameraInactive;

    let tracks = stream.getVideoTracks();
    tracks[0].onended = function() {
      console.log("video track onended");
    }

    tracks[0].onmute = function(e) {
      console.log("video track onmute", e);
    }

    tracks[0].onunmute = function(e) {
      console.log("video track onunmute", e);
    }

    cameraVideoElement.srcObject = stream;
    cameraVideoElement.play();
    setTakePhoto(tracks[0]);

    startCameraBtn.disabled = true;
    stopCameraBtn.disabled = false;
  }
}

async function stopCamera() {
  if (cameraVideoElement.srcObject) {
    let tracks = cameraVideoElement.srcObject.getTracks();
    if (tracks) {
      tracks.forEach(track => {
      	console.log("stop track", track.kind, ":", track.label);
      	track.stop();
      });
    }
  }
}

function handleSuccess(stream) {
    stream.oninactive = onScreenInactive;

    let tracks = stream.getVideoTracks();
    tracks[0].onended = function() {
      console.log("video track onended");
    }

    tracks[0].onmute = function(e) {
      console.log("video track onmute", e);
    }

    tracks[0].onunmute = function(e) {
      console.log("video track onunmute", e);
    }

    screenVideoElement.srcObject = stream;
    screenVideoElement.play();
    setTakePhoto(tracks[0]);

    startScreenBtn.disabled = true;
    startScreenBtn.innerText = 'Sharing';
    stopScreenBtn.disabled = false;
}

function handleError(error) {
	console.error('getDisplayMedia() error: ', error);
}

async function startScreen() {
	navigator.mediaDevices.getDisplayMedia({video: true}).then(handleSuccess, handleError);
}

function onScreenInactive(e) {
  console.log("share stopped", e);
  startScreenBtn.disabled = false;
  startScreenBtn.innerText = "StartScreen";
  stopScreenBtn.disabled = true;
  screenVideoElement.srcObject = null;
  imageCapture = null;
  imageClipElement.src = '';
}

async function selectSource(source_id) {
	if (screenVideoElement.srcObject) {
		console.log('already share')
		return;
	}
	console.log('start share')

  const constraints_without_audio = {
    audio: false,
    video: {
      mandatory: {
        chromeMediaSource: 'desktop',
        chromeMediaSourceId: source_id,
        maxFrameRate: 30
      }
    }
  };
  const constraints_with_audio = {
    audio: {
      mandatory: {
        chromeMediaSource: 'desktop',
        chromeMediaSourceId: source_id
      }
    },
    video: {
      mandatory: {
        chromeMediaSource: 'desktop',
        chromeMediaSourceId: source_id,
        maxFrameRate: 30
      }
    }
  };

  // Create a Stream
  let stream = await navigator.mediaDevices.getUserMedia(constraints_without_audio);
  if (stream && stream.active) {
    stream.oninactive = onScreenInactive;

    let tracks = stream.getVideoTracks();
    tracks[0].onended = function() {
      console.log("video track onended");
    }

    tracks[0].onmute = function(e) {
      console.log("video track onmute", e);
    }

    tracks[0].onunmute = function(e) {
      console.log("video track onunmute", e);
    }

    screenVideoElement.srcObject = stream;
    screenVideoElement.play();
    setTakePhoto(tracks[0]);

    startScreenBtn.disabled = true;
    startScreenBtn.innerText = source_id;
    stopScreenBtn.disabled = false;
		console.log('share started')
  }
}

async function stopScreen() {
	if (screenVideoElement.srcObject) {
		console.log('stop share')
		let tracks = screenVideoElement.srcObject.getTracks();
		if (tracks) {
			tracks.forEach(track => {
				console.log("stop track", track.kind, ":", track.label);
				track.stop();
			});
		}
	}
}

function setTakePhoto(track) {
	imageCapture = new ImageCapture(track);
  imageCapture.getPhotoCapabilities().then(function(capabilities) {
    console.log('Photo capabilities:', capabilities);
  }).catch(function(error) {
    console.error('getPhotoCapabilities() error: ', error);
  });
  imageCapture.getPhotoSettings().then(function(settings) {
    console.log('Photo settings:', settings);
  }).catch(function(error) {
    console.error('getPhotoSettings() error: ', error);
  });
}

async function takePhoto() {
	if (imageCapture) {
	  imageCapture.takePhoto().then(function(blob) {
	    console.log('Took photo:', blob);
	    imageClipElement.src = URL.createObjectURL(blob);
	  }).catch(function(error) {
	    console.error('takePhoto() error: ', error);
	  });
	}
}

async function grabFrame() {
	if (imageCapture) {
		imageCapture.grabFrame()
		.then(function(imageBitmap) {
			console.log('Grabbed frame:', imageBitmap);
			const canvas = document.createElement("canvas");
			canvas.width = imageBitmap.width;
			canvas.height = imageBitmap.height;
			canvas.getContext('2d').drawImage(imageBitmap, 0, 0);
	    imageClipElement.src = canvas.toDataURL("image/png");
		})
		.catch(function(error) {
			console.log('grabFrame() error: ', error);
		});
	}
}
