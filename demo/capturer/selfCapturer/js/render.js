const { desktopCapturer, remote } = require('electron');
const { Menu } = remote;

async function startCamera() {
  const constraints = {
    audio: false,
    video: {
      cursor: "always"
    }
  };

  // Create a Stream
  const stream = await navigator.mediaDevices.getUserMedia(constraints);

  // Preview the source in a video element
  const videoElement = document.getElementById('cameraVideo');
  videoElement.srcObject = stream;
  videoElement.play();
}

async function stopCamera() {
  const videoElement = document.getElementById('cameraVideo');
  let tracks = videoElement.srcObject.getTracks();

  tracks.forEach(track => track.stop());
  videoElement.srcObject = null;
}

async function startScreen() {
  const inputSources = await desktopCapturer.getSources({
    types: ['window', 'screen']
  });

  const videoOptionsMenu = Menu.buildFromTemplate(
    inputSources.map(source => {
      return {
        label: source.name + ' [' + source.id + ']',
        click: () => selectSource(source)
      };
    })
  );


  videoOptionsMenu.popup();
}

async function selectSource(source) {
  // Stop old Stream
  stopScreen();

  const constraints = {
    audio: false,
    video: {
      mandatory: {
        chromeMediaSource: 'desktop',
        chromeMediaSourceId: source.id
      }
    }
  };

  // Create a Stream
  const stream = await navigator.mediaDevices.getUserMedia(constraints);

  // Preview the source in a video element
  const videoElement = document.getElementById('screenVideo');
  videoElement.srcObject = stream;
  videoElement.play();
}

async function stopScreen() {
  const videoElement = document.getElementById('screenVideo');
  let tracks = videoElement.srcObject.getTracks();
  if (tracks) {
    tracks.forEach(track => track.stop());
  }
  videoElement.srcObject = null;
}
