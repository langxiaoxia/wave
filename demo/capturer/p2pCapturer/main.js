// Modules to control application life and create native browser window
const { app, BrowserWindow, ipcMain, desktopCapturer, crashReporter } = require('electron')
const path = require('path')
require('@electron/remote/main').initialize() // Migrating from remote

// app.disableHardwareAcceleration()

crashReporter.start({
  submitURL: 'http://192.168.131.37:1127',
  uploadToServer: true,
  compress: false,
})

ipcMain.handle(
  'CRASH_MAIN',
  (event) => {
    console.log('main process.crash()')
    process.crash()
  }
)

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

  // and load the index.html of the app.
  mainWindow.loadFile('./index.html')

  // Open the DevTools.
  // mainWindow.webContents.openDevTools()

  mainWindow.webContents.on('render-process-gone', (event, details) => {
    console.log('render-process-gone get exitCode: ' + details.exitCode)
    console.log(`render-process-gone error: ${details.reason}`)
  })

  app.on('child-process-gone', (event, details) => {
    console.log(`child-process-gone type: ${details.type}`)
    console.log('child-process-gone get exitCode: ' + details.exitCode)
    console.log(`child-process-gone error: ${details.reason}`)
    console.log(`child-process-gone serviceName: ${details.serviceName}`)
    console.log(`child-process-gone name: ${details.name}`)
  })

  return mainWindow
}

app.whenReady().then(() => {
  console.log(app.name + ' is ready')
  createMainWindow()
})

app.on('gpu-info-update', () => {
  console.log('gpu-info-update')
})