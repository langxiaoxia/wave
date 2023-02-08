const { ipcRenderer } = require('electron');

function startSend() {
	const sendOnce = document.querySelector('#send_once').checked;
	if (sendOnce) {
    ipcRenderer.invoke('ONE_SEND')
  } else {
    ipcRenderer.invoke('START_SEND')
  }
}

function stopSend() {
  ipcRenderer.invoke('STOP_SEND')
}
