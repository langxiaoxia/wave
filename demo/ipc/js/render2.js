const ipc = require('electron').ipcRenderer;

hideBtn = document.querySelector('#hideNotify');
resetBtn = document.querySelector('#resetHeight');

hideBtn.addEventListener('click', () => {
	console.info("hideNotify: before ipc.send");
	ipc.send('show-hide-notify', false);
	console.info("hideNotify: after ipc.send");
});

resetBtn.addEventListener('click', () => {
	console.info("resetHeight: before ipc.send");
	ipc.send('set-notify-height', 1);
	console.info("resetHeight: after ipc.send");
});
