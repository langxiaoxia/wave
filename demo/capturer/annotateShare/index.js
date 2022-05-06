// Modules to control application life and create native browser window
const {app, BrowserWindow} = require('electron')
const path = require('path')

let mainWindow;
let annoWindow;

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
	})
	
	// and load the index.html of the app.
	mainWindow.loadFile('./index.html')
	
	// Open the DevTools.
	// mainWindow.webContents.openDevTools()
	
	mainWindow.on('close', (event) => {
		if (annoWindow) annoWindow.destroy()
		app.quit()
	})

	return mainWindow
}

async function createAnnoWindow() {
	annoWindow = new BrowserWindow({
		width: 400,
		height: 40,
		x: 600,
		y: 0,
		resizable: false,
		minimizable: false,
		maximizable: false,
		closable: false,
		fullscreenable: false,
		alwaysOnTop: true,
		frame: false,
		thickFrame: false,
		type: "toolbar",
		title: "Annotation Toolbar",
		webPreferences: {
			nodeIntegration: true,
			contextIsolation: false,
			enableRemoteModule: true,
		}
	})
	
	annoWindow.loadFile('./anno.html')

	annoWindow.setContentProtection(true)
	
	return annoWindow
}

app.whenReady().then(() => {
	createMainWindow()
//	createAnnoWindow()
})
