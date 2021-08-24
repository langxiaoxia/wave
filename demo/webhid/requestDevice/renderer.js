const {ipcRenderer} = require('electron')

const gsDeviceFilters = [{ vendorId: 0x2BAB }];

let options = {
  filters: [
//    {name: 'EDIFIER BLE'},
    {namePrefix: 'EDIFIER'}
  ]
//acceptAllDevices:true
}

let hidButton = document.getElementById('hid-button')
let serialButton = document.getElementById('serial-button')
let bluetoothButton = document.getElementById('bluetooth-button')

let hidDevice = null
let serialPort = null
let bluetoothDevice = null

function sleep(time) {
	return new Promise((resolve) => setTimeout(resolve, time));
}

hidButton.onclick = async function() {
	if (!navigator.hid) {
		console.log('navigator.hid not supported');
		return;
	}
	console.log('navigator.hid supported');
	await sleep(5000)
	console.log('call asyncListHid');
	asyncListHid()
//	setTimeout(async function (){
//    console.warn("≥¨ ±≤‚ ‘")
//    await asyncListHid()
//  }, 5 * 1000)
	
	console.log('asyncListHid return');
}

serialButton.onclick = async function() {
	if (!navigator.serial) {
		console.log('navigator.serial not supported');
		return;
	}
	console.log('navigator.serial supported');
	await sleep(5000)
	console.log('call asyncListSerial');
	asyncListSerial()
	console.log('asyncListSerial return');
}

bluetoothButton.onclick = async function() {
	if (!navigator.bluetooth) {
		console.log('navigator.bluetooth not supported');
		return;
	}
	console.log('navigator.bluetooth supported');
	await sleep(5000)
	console.log('call asyncListBluetooth');
	asyncListBluetooth()
	console.log('asyncListBluetooth return');
}

async function asyncListHid() {
	// Show a device chooser dialog. The vendor ID filter matches GrandStream GUV30xx devices.
	try {
		hidDevice = await navigator.hid.requestDevice({ filters: gsDeviceFilters });
		if (!hidDevice) {
			console.log('hidDevice is null');
			return;
		}

		console.log('hidDevice', hidDevice)
	} catch (error) {
		console.log(error);
	}
}

async function asyncListSerial() {
	try {
		serialPort = await navigator.serial.requestPort();
		if (!serialPort) {
			console.log('serialPort is null');
			return;
		}

		console.log('serialPort', serialPort)
	} catch (error) {
		console.log(error);
	}
}

async function asyncListBluetooth() {
	try {
		bluetoothDevice = await navigator.bluetooth.requestDevice(options);
		if (!bluetoothDevice) {
			console.log('bluetoothDevice is null');
			return;
		}

		console.log('bluetoothDevice', bluetoothDevice)
	} catch (error) {
		console.log(error);
	}
}
