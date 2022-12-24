const { ipcRenderer } = require('electron');
const desktopCapturer = {
  getSources: (opts) => ipcRenderer.invoke('DESKTOP_CAPTURER_GET_SOURCES', opts)
}
// const { Menu } = require('electron').remote; // Migrating from remote
window.remote = require('@electron/remote')
console.log(window.remote)
const { Menu } = window.remote // Migrating from remote

const listSourceBtn = document.querySelector('#listSource');
const startScreenBtn = document.querySelector('#startScreen');
const stopScreenBtn = document.querySelector('#stopScreen');

const screenVideoElement = document.getElementById('screenVideo');

async function listSource() {
  const inputSources = await desktopCapturer.getSources({
    types: ['window', 'screen']
  });

  const videoOptionsMenu = Menu.buildFromTemplate(
    inputSources.map(source => {
      return {
        label: source.name + ' [' + source.id + ']'
      };
    })
  );

  videoOptionsMenu.popup();
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

function onScreenInactive(e) {
  console.log("onScreenInactive", e);
  startScreenBtn.disabled = false;
  startScreenBtn.innerText = "StartScreen";
  stopScreenBtn.disabled = true;
  screenVideoElement.srcObject = null;
}

async function selectSource(source) {
  const constraints_without_audio = {
    audio: false,
    video: {
      mandatory: {
        chromeMediaSource: 'desktop',
        chromeMediaSourceId: source.id,
        maxFrameRate: 30
      }
    }
  };
  const constraints_with_audio = {
    audio: {
      mandatory: {
        chromeMediaSource: 'desktop',
        chromeMediaSourceId: source.id
      }
    },
    video: {
      mandatory: {
        chromeMediaSource: 'desktop',
        chromeMediaSourceId: source.id,
        maxFrameRate: 30
      }
    }
  };

  // Create a Stream
  let stream = await navigator.mediaDevices.getUserMedia(constraints_without_audio);
  if (stream && stream.active) {
    stream.oninactive = onScreenInactive;

    let tracks = stream.getVideoTracks();
    tracks[0].onended = function () {
      console.log("video track onended");
    }

    tracks[0].onmute = function (e) {
      console.log("video track onmute", e);
    }

    tracks[0].onunmute = function (e) {
      console.log("video track onunmute", e);
    }

    screenVideoElement.srcObject = stream;
    screenVideoElement.play();

    startScreenBtn.disabled = true;
    startScreenBtn.innerText = source.id;
    stopScreenBtn.disabled = false;
  }
}

async function stopScreen() {
  if (screenVideoElement.srcObject) {
    let tracks = screenVideoElement.srcObject.getTracks();
    if (tracks) {
      tracks.forEach(track => {
        console.log("stop track", track.kind, ":", track.label);
        track.stop();
      });
    }
  }
}
