// Modules to control application life and create native browser window
const { app, BrowserWindow, ipcMain } = require('electron')
const path = require('path')
const { autoUpdater } = require('electron-updater')
const fs = require('fs')
const v8 = require('v8')

function createHeapSnapshot() {
  process.getProcessMemoryInfo().then((obj) => {
    console.log(`Process residentSet: ${obj.residentSet}`)
    console.log(`Process privateBytes: ${obj.private}`)
    console.log(`Process sharedBytes: ${obj.shared}`)
  })

  // It's important that the filename end with `.heapsnapshot`,
  // otherwise Chrome DevTools won't open it.
  const fileName = `${Date.now()}.heapsnapshot`;
  const fileStream = fs.createWriteStream(fileName);
  const snapshotStream = v8.getHeapSnapshot();
  snapshotStream.pipe(fileStream);
}

autoUpdater.forceDevUpdateConfig = true
autoUpdater.autoDownload = false

let feedServer = 'https://fw.gdms.cloud/wave/desktop/'
feedServer = feedServer + 'download/ucm/1.0.20.x/'
let feed = feedServer + 'win'
let checking = false
let count = 0
let num = 1000
autoUpdater.setFeedURL(feed)
console.log('current version:', autoUpdater.currentVersion.version)

autoUpdater.on('checking-for-update', () => {
  console.log('autoUpdater checking-for-update:', count)
})

autoUpdater.on('update-available', (info) => {
  console.log('autoUpdater update-available:', info.version)
})

autoUpdater.on('update-not-available', (info) => {
  console.log('autoUpdater update-not-available')
})

autoUpdater.on('error', (err) => {
  console.log('autoUpdater error')
})

let mainWindow = null;
let sendTimer = null;

ipcMain.handle('ONE_SEND', (event) => {
  if (sendTimer) {
    console.log('timer already started!')
  } else {
    count = 0
    doSend(true)
  }
})

ipcMain.handle('START_SEND', (event) => {
  if (sendTimer) {
    console.log('timer already started!')
  } else {
    // createHeapSnapshot()

    count = 0
    sendTimer = setInterval(doSend, 1000);
    console.log('timer started!')
  }
})

function doSend(verbose = false) {
  if (count >= num) {
    stopSend()
    return
  }
  if (!checking) {
    checking = true
    count++
    autoUpdater.checkForUpdates().then(() => {
      checking = false
    }).catch((e) => {
      checking = false
      if (verbose) {
        console.log('autoUpdater checkForUpdates exception:', e)
      } else {
        console.log('autoUpdater checkForUpdates exception')
      }
    })
  } else {
    console.log('autoUpdater still in checking!')
  }
}

function stopSend() {
  if (sendTimer) {
    clearInterval(sendTimer);
    sendTimer = null;
    console.log('timer stopped!')
    console.log('update count:' + count)

    // createHeapSnapshot()
  } else {
    console.log('timer not started!')
  }
}

ipcMain.handle('STOP_SEND', (event) => {
  stopSend()
})

async function createMainWindow() {
  // Create the browser window.
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      nodeIntegration: true,
      contextIsolation: false
    }
  })

  // and load the index.html of the app.
  mainWindow.loadFile('./index.html')

  // Open the DevTools.
  // mainWindow.webContents.openDevTools()

  return mainWindow
}

app.whenReady().then(() => {
  createMainWindow()
})
