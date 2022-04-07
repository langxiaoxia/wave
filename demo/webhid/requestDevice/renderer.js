const hidAll = [{ }];
const hidOne = [{ vendorId: 0x2BAB }];

const bluetoothAll = { acceptAllDevices: true };
const bluetoothOne = {
  filters: [
//    {name: 'EDIFIER BLE'},
    {namePrefix: 'Jabra'}
  ]
};

let hidButton = document.getElementById('hid-button')
let serialButton = document.getElementById('serial-button')
let bluetoothButton = document.getElementById('bluetooth-button')
let devicesButton = document.getElementById('devices-button')

let hidDevice = null
let serialPort = null
let bluetoothDevice = null
let mediaDevices = null

hidButton.onclick = async function() {
	if (!navigator.hid) {
		console.log('navigator.hid not supported');
		return;
	}
	console.log('navigator.hid supported');

	const delayFlag = document.querySelector('#delayFlag').checked;
	if (delayFlag) {
		setTimeout(async function () {
			console.log('call asyncListHid');
			await asyncListHid()
			console.log('asyncListHid return');
		}, 5 * 1000);
	} else {
		console.log('call asyncListHid');
		await asyncListHid()
		console.log('asyncListHid return');
	}

	console.log('onclick return');
}

serialButton.onclick = async function() {
	if (!navigator.serial) {
		console.log('navigator.serial not supported');
		return;
	}
	console.log('navigator.serial supported');

	const delayFlag = document.querySelector('#delayFlag').checked;
	if (delayFlag) {
		setTimeout(async function () {
			console.log('call asyncListSerial');
			await asyncListSerial()
			console.log('asyncListSerial return');
		}, 5 * 1000);
	} else {
		console.log('call asyncListSerial');
		await asyncListSerial()
		console.log('asyncListSerial return');
	}

	console.log('onclick return');
}

bluetoothButton.onclick = async function() {
	if (!navigator.bluetooth) {
		console.log('navigator.bluetooth not supported');
		return;
	}
	console.log('navigator.bluetooth supported');

	const delayFlag = document.querySelector('#delayFlag').checked;
	if (delayFlag) {
		setTimeout(async function () {
			console.log('call asyncListBluetooth');
			await asyncListBluetooth()
			console.log('asyncListBluetooth return');
		}, 5 * 1000);
	} else {
		console.log('call asyncListBluetooth');
		await asyncListBluetooth()
		console.log('asyncListBluetooth return');
	}

	console.log('onclick return');
}

devicesButton.onclick = async function() {
	if (!navigator.mediaDevices) {
		console.log('navigator.mediaDevices not supported');
		return;
	}
	console.log('navigator.mediaDevices supported');

	const delayFlag = document.querySelector('#delayFlag').checked;
	if (delayFlag) {
		setTimeout(async function () {
			console.log('call asyncListDevice');
			await asyncListDevice()
			console.log('asyncListDevice return');
		}, 5 * 1000);
	} else {
		console.log('call asyncListDevice');
		await asyncListDevice()
		console.log('asyncListDevice return');
	}

	console.log('onclick return');
}

async function asyncListHid() {
	try {
		hidDevice = await navigator.hid.requestDevice({ filters: hidAll });
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
		bluetoothDevice = await navigator.bluetooth.requestDevice(bluetoothAll);
		if (!bluetoothDevice) {
			console.log('bluetoothDevice is null');
			return;
		}

		console.log('bluetoothDevice', bluetoothDevice)
	} catch (error) {
		console.log(error);
	}
}

async function asyncListDevice() {
	try {
		mediaDevices = await navigator.mediaDevices.enumerateDevices();
		if (!mediaDevices) {
			console.log('mediaDevices is null');
			return;
		}

		console.log('mediaDevices', mediaDevices)
	} catch (error) {
		console.log(error);
	}
}
