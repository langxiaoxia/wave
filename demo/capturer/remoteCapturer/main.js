// Modules to control application life and create native browser window
const { app, BrowserWindow, ipcMain, desktopCapturer } = require('electron')
const path = require('path')
require('@electron/remote/main').initialize() // Migrating from remote

ipcMain.handle(
  'DESKTOP_CAPTURER_GET_SOURCES',
  (event, opts) => desktopCapturer.getSources(opts)
)

let mainWindow;

async function createMainWindow() {
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      nodeIntegration: true,
      contextIsolation: false,
      // enableRemoteModule: true, // Migrating from remote
    }
  })

  require('@electron/remote/main').enable(mainWindow.webContents) // Migrating from remote

  // mainWindow.setContentProtection(true)

  // and load the index.html of the app.
  mainWindow.loadFile('./index.html')

  // Open the DevTools.
  // mainWindow.webContents.openDevTools()

  return mainWindow
}

app.whenReady().then(() => {
  createMainWindow()
})
