'use strict';

const electron = require('electron');
const app = electron.app;  // Module to control application life.
const BrowserWindow = electron.BrowserWindow;  // Module to create native browser window.
const globalShortcut = electron.globalShortcut;
const ipcMain = electron.ipcMain;

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
var mainWindow = null;

app.commandLine.appendSwitch('ppapi-out-of-process','');
app.commandLine.appendSwitch('register-pepper-plugins', '../Browser/DJIViewerPlugin.dll;plugin/dji_viewer' + ';'
 + '../Browser/DJILiveVideoPlugin.dll;plugin/dji_live_video');

// Quit when all windows are closed.
app.on('window-all-closed', function() {
  // On OS X it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q
  
  // if (process.platform != 'darwin') {
    app.quit();
  // }
});

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
app.on('ready', function() {

  mainWindow = new BrowserWindow({
    width: 1080, height: 650, minWidth: 1080, minHeight: 650, useContentSize: true, title: 'DJI Assistant 2', backgroundColor: '#fff', autoHideMenuBar: true,
    'web-preferences': {
      'plugins': true
    }
  });
  mainWindow.setMenuBarVisibility(false);
  mainWindow.loadURL('file://' + __dirname + '/app.asar/index.html');
  // Open the DevTools.
  //mainWindow.webContents.openDevTools();


  mainWindow.on('close', function(e) {

    var dialog = require('dialog');
    var choice = dialog.showMessageBox(mainWindow, {
        type: 'question',
        buttons: ['OK', 'Cancel'],
        title: 'Confirm',
        message: 'Are you sure you want to quit?'
    });

    if(choice !== 0) e.preventDefault();

  });


  // Emitted when the window is closed.
  mainWindow.on('closed', function() {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
    mainWindow = null;
  });

});

ipcMain.on('asynchronous-message', function(event, arg) {

  switch(arg.type){

    case 'openWindow':

      var opts = arg.options

      if(/(http|ftp|https|file):\/\/[\w\-_]+(\.[\w\-_]+)+([\w\-\.,@?^=%&amp;:/~\+#]*[\w\-\@?^=%&amp;/~\+#])?/.test(arg.url)){
        var url = arg.url
        opts['node-integration'] = false
      }else{
        var url = 'file://' + __dirname + '/app.asar/' + arg.url
      }

      opts['web-preferences'] = {'plugins': true}
      var win = new BrowserWindow(opts)
      win.on('closed', function() {
        win = null
      })

      win.setMenuBarVisibility(false)
      win.loadURL(url)
      win.show()

      break;

    default:
      break;

  }

  event.sender.send('asynchronous-reply', 'success');

});