const ipc = require('electron').ipcRenderer;

showBtn = document.querySelector('#showNotify');
hideBtn = document.querySelector('#hideNotify');
setBtn = document.querySelector('#setHeight');
resetBtn = document.querySelector('#resetHeight');

showBtn.addEventListener('click', () => {
	console.info("showNotify: before ipc.send");
	ipc.send('show-hide-notify', true);
	console.info("showNotify: after ipc.send");
});

hideBtn.addEventListener('click', () => {
	console.info("hideNotify: before ipc.send");
	ipc.send('show-hide-notify', false);
	console.info("hideNotify: after ipc.send");
});

setBtn.addEventListener('click', () => {
	console.info("setHeight: before ipc.send");
	ipc.send('set-notify-height', 400);
	console.info("setHeight: after ipc.send");
});

resetBtn.addEventListener('click', () => {
	console.info("resetHeight: before ipc.send");
	ipc.send('set-notify-height', 1);
	console.info("resetHeight: after ipc.send");
});
