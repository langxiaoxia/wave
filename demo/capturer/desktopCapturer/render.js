const videoSelectBtn = document.getElementById('videoSelectBtn');
videoSelectBtn.onclick = getVideoSources;

const startBtn = document.getElementById('startBtn');
startBtn.onclick = startCapturer;

const stopBtn = document.getElementById('stopBtn');
stopBtn.onclick = stopCapturer;

const { desktopCapturer, remote } = require('electron');
const { Menu } = remote;

// Get the available video sources
async function getVideoSources() {
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

  videoSelectBtn.innerText = source.name;

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
  console.log("before call getUserMedia");
  const stream = await navigator.mediaDevices
    .getUserMedia(constraints);
  console.log("after call getUserMedia");

  // Preview the source in a video element
  const videoElement = document.querySelector('video');
  videoElement.srcObject = stream;
  videoElement.play();
}

async function startCapturer() {
  const constraints = {
    audio: false,
    video: {
      cursor: "always"
    }
  };

  // Create a Stream
  const stream = await navigator.mediaDevices
    .getUserMedia(constraints);

  // Preview the source in a video element
  const videoElement = document.querySelector('video');
  videoElement.srcObject = stream;
  videoElement.play();
}

async function stopCapturer() {
  const videoElement = document.querySelector('video');
  let tracks = videoElement.srcObject.getTracks();

  tracks.forEach(track => track.stop());
  videoElement.srcObject = null;

  videoSelectBtn.innerText = "Choose a Video Source";
}
