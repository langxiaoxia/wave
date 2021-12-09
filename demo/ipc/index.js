// Modules to control application life and create native browser window
const {app, BrowserWindow} = require('electron')
const path = require('path')
const ipc = require('electron').ipcMain

let mainWindow = null;
let notifyWindow = null;

async function createMainWindow() {
	mainWindow = new BrowserWindow({
		width: 800,
		height: 600,
		webPreferences: {
			preload: path.join(__dirname, 'preload.js'),
			nodeIntegration: true,
			contextIsolation: false,
			enableRemoteModule: true,
		}
	});

	notifyWindow = new BrowserWindow({
		x: 200,
		y: 200,
		width: 400,
		height: 400,
		frame: false,
		resizable: false,
		moveable: false,
		minimizable: false,
		maximizable: false,
		skipTaskbar: true,
		webPreferences: {
			preload: path.join(__dirname, 'preload.js'),
			nodeIntegration: true,
			contextIsolation: false,
			enableRemoteModule: true,
		}
	});
	
	// and load the index.html of the app.
	mainWindow.loadFile('./index.html');

	notifyWindow.loadFile('./index2.html');
	notifyWindow.excludedFromShownWindowsMenu = true;
	
	// Open the DevTools.
	// mainWindow.webContents.openDevTools();

	mainWindow.on('closed', () => {
		console.info("mainWindow.on closed");
		mainWindow = null;
		if (notifyWindow) {
			console.info("close notify window");
			notifyWindow.close();
		}
	});
	
	notifyWindow.on('closed', () => {
		console.info("notifyWindow.on closed");
		notifyWindow = null;
	});
}

app.whenReady().then(() => {
	createMainWindow();
})

ipc.on('set-notify-height', function (event, arg) {
	console.info("ipc.on set-notify-height before setBounds: arg=", arg);
	notifyWindow.setBounds({width: arg, height: arg});
	console.info("ipc.on set-notify-height after setBounds");
})

ipc.on('show-hide-notify', function (event, arg) {
	if (arg == true) {
		console.info("ipc.on show-hide-notify before show: arg=", arg);
		notifyWindow.show();
		console.info("ipc.on show-hide-notify after show");
	} else {
		console.info("ipc.on show-hide-notify before hide: arg=", arg);
		notifyWindow.hide();
		console.info("ipc.on show-hide-notify after hide");
	}
})
