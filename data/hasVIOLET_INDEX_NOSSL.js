/*
#
#   hasVIOLET Client Javascript
#
#       Ⓒ2020 Joe Cupano, NE2Z
#
#       RELEASE: 20201224-2100
#       LICENSE: GPLv2
#
#
*/

/* -            
/* - VARIABLES    
/* -
*/

/* Object to hold imported hasVIOLET.json */
var myRadio = {
	DEFAULT: {
		countrycode: "US",
		radio: "DUMMYRF",
		oled: "128x32",
		mode: "LORA",
		channel: {'frequency': 911250, 'spread': 9, 'codingrate4': 8, 'bandwidth': 31250},
		frequency: 902000,
		txpwr: 5,
		mycall: "NOCALL",
		myssid: "00",
		mybeacon: "quack quack",
		dstcall: "BEACON",
		dstssid: "99"
	 },
	 MACROS: {
		M1: "QRZ",
		M2: "QSL",
		M3: "73",
		M4: "TEST TEST TEST"
	 },
	 CURRENT: {
		countrycode: "US",
		radio: "RFM95",
		oled: "128x32",
		node_address: 1,
		mode: "LORA",
		channel: {'frequency': 911250, 'spread': 9, 'codingrate4': 8, 'bandwidth': 31250},
		frequency: 902000,
		txpwr: 5,
		mycall: "NOCALL",
		myssid: "99",
		mybeacon: "quack quack",
		dstcall: "BEACON",
		dstssid: "99"
	 },
	 CHANNELS: {
		"HV0": {
			frequency: 911250,
			spread: 9,
			codingrate4: 8,
			bandwidth: 31250
		},
		"HV1": {
			frequency: 922300,
			spread: 9,
			codingrate4: 8,
			bandwidth: 31250
		},
		"HV2": {
			frequency: 922500,
			spread: 9,
			codingrate4: 8,
			bandwidth: 31250
		},
		"HV3": {
			frequency: 922700,
			spread: 9,
			codingrate4: 8,
			bandwidth: 31250
		}
	 }
}

/* Current MSG Exchange*/
var myContact = {
	mycall: "NOCALLS",
	myssid: "00",
	mybeacon: "TEST TEST TEST TEST",
	dstcall: "BEACON",
	dstssid: "99",
	header: "NOCALLS-00>BEACON-99",
	sent: "",
	received: ""
}

/* Programmable Channels */
var myChannels = [0,1,2,3,4];

/* Programmable Macros M1-M4 */
var myMacros = ["","","",""];

/* Last Seven Calls heard */
var myHeard = [ "NOCALLS", "NOCALLS", "NOCALLS", "NOCALLS", "NOCALLS", "NOCALLS", "NOCALLS"];

/* Radio and Msg Module Commands */
var radioCONN = 0;                                                  //  0 = Disconnected, 1 = Connected
var radioBEACON = 0;                                                //  0 = Beacon OFF, 1 = Beacon ON  
var radioTXPWR = 5;                                                 //  TX Power can be 5 to 21    
var radioLOG = 0;                                                   //  0 = Logging OFF, 1 = Logging ON
var radioMODE = "LORA";                                             //  LORA, (futures FSK, FX.25, ...
var msgRECEIVED = "";												//  data RXed b4 process  
var msgBLASTED = "";												//  data displayed in RX_Window
var msgENTERED = "";												//  Typed into TX_Window      
var msgPARSED = [];													//  For processing two-part command
var msgSUBPARSED = [];												//  For processing two-part command  
var msgPARTICLE = [];												//  For processing two-part command  
var msgSENT = "";													//  msgINPUT that was TXed  

var currCHAN = 0;													//  Channel Numbers 0 - 3
var lastCHAN = 0;													//  Channel Numbers 0 - 3
var lastCHANNEL = "";
var currCHANNEL = "";
var previous_operation = "";
var current_operation = "";
var previous_entry = "";
var current_entry = "";
var new_entry = "";
var myHostname = location.hostname;
var wsURI = "wss://" + myHostname + ":8000/ws";					//  TEST Websocket URI 
var url = "http://" + myHostname + ":8000/cfg/hasVIOLET.json";		//  JSON file location
var getHasJson = new XMLHttpRequest();								//  Holds JSON from Radio
// var setRadioCfg = new XMLHttpRequest();					        //  Replace with JSON to Radio via Websocket

var rxDisplay = [];													//  Holds whole RX window as 27 lines of text
var rxDispY = 26;													//  Numbe of rowa for RX Window


/* -            
/* - FUNCTIONS
/* -
*/

/* --- Tuner-Module --- */

function updateTunerDisplay(myChannel) {
	var el = document.getElementById('tuner-setting');
	el.textContent = myChannel;
	console.log("TUNER: DISPLAY " + myChannel);
}

function btnHOME() {
	lastCHANNEL = currCHANNEL;
	lastCHAN = currCHAN;
	currCHAN = 0;
	currCHANNEL = myChannels[currCHAN];
	var squkey = myChannels[currCHAN];
	var squish = myRadio.CHANNELS[squkey];
	var squirt = JSON.stringify(squish);
	console.log("TUNER: HOME " + currCHANNEL);
	socket.send("SET:CHANNEL:" + currCHAN + ":" + squirt);
	updateTunerDisplay(currCHANNEL);
}

function btnRST() {
	getHasJson.open('GET', url, true);
	getHasJson.send(null);
	getHasJson.onload = function() {
		if (getHasJson.readyState === getHasJson.DONE && getHasJson.status === 200) {
			myRadio = JSON.parse(getHasJson.responseText);
			console.log("TUNER: btnRESET");
		}
		previous_operation = "RESET";
		previous_entry = "";
		current_entry = "";
		updateTunerDisplay(myRadio.CURRENT.channel);
	}
}

function btnLCH() {
	currCHAN = lastCHAN;
	currCHANNEL = myChannels[currCHAN];
	lastCHANNEL = currCHANNEL;
	lastCHAN = currCHAN;
	var squkey = myChannels[currCHAN];
	var squish = myRadio.CHANNELS[squkey];
	var squirt = JSON.stringify(squish);
	console.log("TUNER: LCH " + currCHANNEL);
	socket.send("SET:CHANNEL:" + currCHAN + ":" + squirt);
	updateTunerDisplay(currCHANNEL);
}

function btnUP() {
	lastCHANNEL = currCHANNEL;
	lastCHAN = currCHAN;
	currCHAN++;
	if (currCHAN > 3) {
		currCHAN = 3;
	}
	currCHANNEL = myChannels[currCHAN];
	var squkey = myChannels[currCHAN];
	var squish = myRadio.CHANNELS[squkey];
	var squirt = JSON.stringify(squish);
	console.log("TUNER: UP " + currCHANNEL);
	socket.send("SET:CHANNEL:" + currCHAN + ":" + squirt);
	updateTunerDisplay(currCHANNEL);
}

function btnDOWN() {
	lastCHANNEL = currCHANNEL;
	lastCHAN = currCHAN;
	--currCHAN;
	if (currCHAN < 0) {
		currCHAN = 0;
	}
	currCHANNEL = myChannels[currCHAN];
	var squkey = myChannels[currCHAN];
	var squish = myRadio.CHANNELS[squkey];
	var squirt = JSON.stringify(squish);
	console.log("TUNER: DOWN " + currCHANNEL);
	socket.send("SET:CHANNEL:" + currCHAN + ":" + squirt);
	updateTunerDisplay(currCHANNEL);
}

function btnNUM(knum) {
	lastCHANNEL = currCHANNEL;
	lastCHAN = currCHAN;
	if (knum > 3) {
		return;
	}
	currCHAN = knum;
	currCHANNEL = myChannels[currCHAN];
	var squkey = myChannels[currCHAN];
	var squish = myRadio.CHANNELS[squkey];
	var squirt = JSON.stringify(squish);
	console.log("TUNER: CHANL " + currCHANNEL);
	socket.send("SET:CHANNEL:" + currCHAN + ":" + squirt);
	updateTunerDisplay(currCHANNEL);
}


/* --- Radio Panel-Module --- */

function btnRADIO() {
	console.log("RADIO: btnRADIO: clicked");
	var el_btnRADIO = document.getElementById('btnRADIO');
	if (radioCONN == 0) {
		radioCONN = 1;
		el_btnRADIO.style.background = "MediumSeaGreen";
		socket.send("SET:RADIO:ON");
		console.log("RADIO: btnRADIO: Radio ON");
		previous_entry = 0;
		previous_operation = "RADIO:ON";
	 }
	 else if (radioCONN == 1){
		 radioCONN = 0;
		 el_btnRADIO.style.background = "Black";
		 socket.send("SET:RADIO:OFF");
		 console.log("RADIO: btnRADIO: Radio OFF");
		 previous_entry = 0;
		 previous_operation = "RADIO:OFF";
	 }
 }

function btnTXPWR() {
	console.log("RADIO: btnTXPWR: clicked");
	var el_btnTXPWR = document.getElementById('btnTXPWR');
	if (radioTXPWR == 5) {
		radioTXPWR = 10;
		el_btnTXPWR.style.background = "MediumSeaGreen";
		el_btnTXPWR.innerHTML = "MEDM";
		socket.send("SET:TXPWR:MEDIUM");
		console.log("RADIO: btnTXPWR: MEDIUM");
		previous_entry = 0;
		previous_operation = "TXPWR:MEDIUM";
	 }
	 else if (radioTXPWR == 10){
		 radioTXPWR = 20;
		 el_btnTXPWR.style.background = "Orange";
		 el_btnTXPWR.innerHTML = "HIGH";
		 socket.send("SET:TXPWR:HIGH");
		 console.log("RADIO: btnTXPWR: HIGH");
		 previous_entry = 0;
		 previous_operation = "TXPWR:HIGH";
	 }
	 else if (radioTXPWR == 20){
		radioTXPWR = 5;
		el_btnTXPWR.style.background = "Black";
		el_btnTXPWR.innerHTML = "PA";
		socket.send("SET:TXPWR:LOW");
		console.log("RADIO: btnTXPWR: LOW");
		previous_entry = 0;
		previous_operation = "TXPWR:LOW";
	}
}

function btnMODE() {
	console.log("RADIO: btnMODE: clicked");
}

function btnBEACON() {
	console.log("RADIO: btnBEACON: clicked");
	var el_btnBEACON = document.getElementById('btnBEACON');
	if (radioBEACON == 0) {
		radioBEACON = 1;
		el_btnBEACON.style.background = "MediumSeaGreen";
		socket.send("SET:BEACON:ON");
		console.log("RADIO: btnBEACON: Beacon ON");
		previous_entry = 0;
		previous_operation = "BEACON:ON";
	 }
	 else if (radioBEACON == 1){
		 radioBEACON = 0;
		 el_btnBEACON.style.background = "Black";
		 socket.send("SET:BEACON:OFF");
		 console.log("RADIO: btnBEACON: Beacon OFF");
		 previous_entry = 0;
		 previous_operation = "BEACON:OFF";
	 }
}

/* --- Macros Module --- */

function btnM1() {
	console.log("MACRO: btnM1: clicked");
	msgENTERED = msgENTERED + myMacros[0];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "M1";
	previous_operation == "CLICK";
	console.log("MACRO: btnM1: " + myMacros[0]);
}

function btnM2() {
	console.log("MACRO: btnM2: clicked");
	msgENTERED = msgENTERED + myMacros[1];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "M2";
	previous_operation == "CLICK";
	console.log("MACRO: btnM2: " + myMacros[1]);
}

function btnM3() {
	console.log("MACRO: btnM3: clicked");
	msgENTERED = msgENTERED + myMacros[2];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "M3";
	previous_operation == "CLICK";
	console.log("MACRO: btnM3: " + myMacros[2]);
}

function btnM4() {
	console.log("MACRO: btnM4: clicked");
	msgENTERED = msgENTERED + myMacros[3];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "M4";
	previous_operation == "CLICK";
	console.log("MACRO: btnM4: " + myMacros[3]);
}


/* --- MSG Panel-Module --- */

function btnLOG() {
	console.log("MSG: btnLOG: clicked");
	var el_btnLOG = document.getElementById('btnLOG');
	if (radioLOG == 0) {
		radioLOG = 1;
		el_btnLOG.style.background = "MediumSeaGreen";
		// Insert local action
		console.log("MSG: btnLOG: Logging ON");
		previous_entry = "LOG";
		previous_operation = "ON";
	 }
	 else if (radioLOG == 1){
		 radioLOG = 0;
		 el_btnLOG.style.background = "Black";
		 // Insert local action
		 console.log("MSG: btnLOG: Logging OFF");
		 previous_entry = "LOG";
		 previous_operation = "OFF";
	 }
}

function btnRCLR() {
	console.log("MSG: btnRCLR: clicked");
	msgENTERED = "";
	msgPARSED = "";
	previous_entry = "RCLR";
	previous_operation == "CLICK";
	rxwinRCLR();
}

function btnCALL() {
	console.log("MSG: btnCALL: clicked");
	msgENTERED = msgENTERED + myContact.mycall + "-" + myContact.myssid;
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "CALL";
	previous_operation == "CLICK";
}

function btnDEST() {
	console.log("MSG: btnDEST: clicked");
	msgENTERED = msgENTERED + myContact.dstcall + "-" + myContact.dstssid;
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "DEST";
	previous_operation == "CLICK";
}

function btnHEAD() {
	console.log("MSG: btnHEAD: clicked");
	myContact.header = myContact.mycall + "-" + myContact.myssid + ">" + myContact.dstcall + "-" + myContact.dstssid + "|";
	msgENTERED = msgENTERED + myContact.header;
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "HEAD";
	previous_operation == "CLICK";
}

function btnHELP() {
	console.log("MSG: btnHELP: clicked");
	rxwinMSGhelp();
	rxwinMSG('-');
	previous_entry = "HELP";
	previous_operation == "CLICK";
}

function btnMCLR() {
	console.log("MSG: btnMCLR: clicked");
	msgENTERED = "";
	msgPARSED = "";
	previous_entry = "MCLR";
	previous_operation == "CLICK";
	document.getElementById('msgENTRY').value = "";
}

function btnSEND() {
	console.log("MSG: btnSEND: clicked");
	msgENTERED = document.getElementById('msgENTRY').value;
	console.log("PANEL: btnSEND", msgENTERED);
	previous_entry = "CMDLINE";
	previous_operation == "CLICK";

	/* --- Sort through commands first --- */
	
	msgPARSED = msgENTERED.split(" ");
		
	if (msgPARSED[0] === ".msgBECN") {                    /*  Set BEACON message  */
		msgSUBPARSED = msgENTERED.replace('msgBECN ', '')
		myContact.mybeacon = msgSUBPARSED;
		myRadio.CURRENT.mybeacon = myContact.mybeacon;
		message = "BEACON = " + myContact.mybeacon;
		rxwinMSG(message);
		console.log("CMD: set: Beacon MSG = " + myContact.mybeacon);
		previous_operation = "msBECN";
	
	} else if (msgPARSED[0] === ".myCALL") {                  /*  Set myCALL and mySSID  */
		msgSUBPARSED = msgPARSED[1];
		msgPARTICLE = msgSUBPARSED.split("-");
		myContact.mycall = msgPARTICLE[0];
		myContact.myssid = msgPARTICLE[1];
		myRadio.CURRENT.mycall = myContact.mycall;
		myRadio.CURRENT.myssid = myContact.myssid;
		message = "CALL = " + myContact.mycall + "-" + myContact.myssid;
		rxwinMSG(message);
		console.log("CMD: set: CALL =" + myContact.mycall + "-" + myContact.myssid);
		previous_operation = "myCALL";
	
	} else if (msgPARSED[0] === ".dstCALL") {                 /*  Set myContact.dstcall and myContact.dstssid  */
		msgSUBPARSED = msgPARSED[1];
		msgPARTICLE = msgSUBPARSED.split("-");
		myContact.dstcall = msgPARTICLE[0];
		myContact.dstssid = msgPARTICLE[1];
		myRadio.CURRENT.dstcall = myContact.dstcall;
		myRadio.CURRENT.dstssid = myContact.dstssid;
		message = "DEST = " + myContact.dstcall + "-" + myContact.dstssid;
		rxwinMSG(message);
		console.log("CMD: set: DEST =" + myContact.dstcall + "-" + myContact.dstssid);
		previous_operation = "myDEST";
	
	} else if (msgPARSED[0] === ".macro1") {                 /*  Set Macro Button 1  */
		msgSUBPARSED = msgENTERED.replace('.macro1 ', '')
		myMacros[0] = msgSUBPARSED;
		message = "MACRO1 = " + myMacros[0];
		rxwinMSG(message);
		console.log("CMD: set: M1 =" + myMacros[0]);
		previous_operation = "M1";
	
	} else if (msgPARSED[0] === ".macro2") {                 /*  Set Macro Button 2  */
		msgSUBPARSED = msgENTERED.replace('.macro2 ', '')
		myMacros[1] = msgSUBPARSED;
		message = "MACRO3 = " + myMacros[1];
		rxwinMSG(message);
		console.log("CMD: set: PANEL: btnM2: M2 = " + myMacros[1]);
		previous_operation = "M2";
	
	} else if (msgPARSED[0] === ".macro3") {                 /*  Set Macro Button 3  */
		msgSUBPARSED = msgENTERED.replace('.macro3 ', '')
		myMacros[2] = msgSUBPARSED;
		message = "MACRO1 = " + myMacros[2];
		rxwinMSG(message);
		console.log("CMD: set: M3 = " + myMacros[2]);
		previous_operation = "M3";
		
	} else if (msgPARSED[0] === ".macro4") {                 /*  Set Macro Button 4  */
		msgSUBPARSED = msgENTERED.replace('.macro4 ', '')
		myMacros[3] = msgSUBPARSED;
		message = "MACRO1 = " + myMacros[3];
		rxwinMSG(message);
		console.log("CMD: set: M4 = " + myMacros[3]);
		previous_operation = "M4";

	/* Anything else is a message to be TX */
	} else {
		socket.send("TX:" + msgENTERED);
		rxwinMSG("TX: "+ msgENTERED);
		console.log("CMD: SEND:" + msgENTERED);
		previous_operation = "SENT";
	}

	msgENTERED = "";
	msgPARSED = "";
	previous_operation = "NULL";
	document.getElementById('msgENTRY').value = "";

}

/* --- Receiver-Window --- */

function rxwinRCLR() {
	console.log("rxWIN: RCLR");
	for (i = rxDispY; i > 0; i=i-1) {
		rxDisplay[i] = "-";
		var rxWINid = "rxWIN" + i;
		var el_RXwin = document.getElementById(rxWINid);
		el_RXwin.innerHTML = rxDisplay[i];
	  }
	  rxDisplay[0] = "-";
	  var el_RXwin = document.getElementById("rxWIN0");
	  el_RXwin.innerHTML = rxDisplay[0];
}

function rxwinMSG(message) {
  for (i = rxDispY; i > 0; i=i-1) {
	rxDisplay[i] = rxDisplay[i-1];
	var rxWINid = "rxWIN" + i;
    var el_RXwin = document.getElementById(rxWINid);
	el_RXwin.innerHTML = rxDisplay[i];
  }
  rxDisplay[0] = message;
  var el_RXwin = document.getElementById("rxWIN0");
  el_RXwin.innerHTML = message;
}

function rxwinMSGhelp() {
	rxDisplay [26] = "-"
	rxDisplay [25] = "................ hasVIOLET WebUI Instructions .............."
	rxDisplay [24] = "-"
	rxDisplay [23] = "......... KEYPAD .......... .......... RF CONTROLS ........."
	rxDisplay [22] = "-"
	rxDisplay [21] = "- FUN = Disabled ---------- -- RADIO = Radio on/off (toggle) "
  	rxDisplay [20] = "- RST = Reset Keypad ------ ----- PA = RF Power (LOW, MED, HIGH) "
  	rxDisplay [19] = "- CLR = Clear Display ----- --- MODE = LORA, FSK, AFSK, etc"
  	rxDisplay [18] = "- ENT = Update Radio Freq - ---- BECN = Beacon msg on/off (toggle)"
	rxDisplay [17] = "-"
	rxDisplay [16] = "..................... MESSAGE CONTROLS ....................."
	rxDisplay [15] = "-"
  	rxDisplay [14] = "- CALL = myCall Macro ----- ---- LOG = Log on/off (toggle)"
  	rxDisplay [13] = "- DEST = dstCall Macro ---- --- RCLR = Clear RX Window"
  	rxDisplay [12] = "- HEAD = Message Header---- --- MCLR = TX window clear"
  	rxDisplay [11] = "- HELP = This screen ------ --- SEND = Send whats in TX Window"
  	rxDisplay [10] = "- "
	rxDisplay [9] = ".................... TX WINDOW COMMANDS ...................."
	rxDisplay [8] = "-"	  
  	rxDisplay [7] = "- .myCALL NOCALL-00     Changes CALL macro to hold NOCALL-00"
  	rxDisplay [6] = "- .dstCALL BL0B-50      Changes DEST macro to hold BL0B-50"
  	rxDisplay [5] = "- .macro1 blah blah     Changes M1 button to hold blah blah"
  	rxDisplay [4] = "-"
	rxDisplay [3] = "..... TO SEND A MESSAGE"
	rxDisplay [2] = "-"  
  	rxDisplay [1] = "- Via Macro buttons click HEAD, M1, then SEND"
  	rxDisplay [0] = "- Type YourCall>DestCall|MESSAGE in TX window then click SEND"
}


/* --- Transmitter-Module --- */

function msgENTRY() {
	// Get the focus to the text input to enter a word right away.
	document.getElementById('msgENTRY').focus();

	
	// Getting the text from the input
	var msgENTERED = document.getElementById('msgENTRY').value;
  }


/* --- ROOT-Module --- */


/* -            
/* - MAIN
/* -
*/

// Get hasVIOLET.json on page load
getHasJson.open('GET', url, true);
getHasJson.send(null);
console.log("INIT: radioCONFIG retrieved");
getHasJson.onload = function() {
	if (getHasJson.readyState === getHasJson.DONE && getHasJson.status === 200) {
		myRadio = JSON.parse(getHasJson.responseText)
	}
}
console.log("INIT: myRadio Object Loaded");

// Channel assignments
myChannels = Object.keys(myRadio.CHANNELS);
myRadio.CURRENT.channel = myChannels[0];           // Set HOME button channel
console.log("INIT: Channels CH1-CH4 set as " + myChannels[0], myChannels[1],  myChannels[2],  myChannels[3]);

// Set MAcros M1 to M4
myMacros[0] = myRadio.MACROS.M1;
myMacros[1] = myRadio.MACROS.M2;
myMacros[2] = myRadio.MACROS.M3;  
myMacros[3] = myRadio.MACROS.M4;
console.log("INIT: Macros M1-M4 set as " + myMacros[0] + "|" + myMacros[1] + "|" + myMacros[2] + "|" + myMacros[3]);

console.log("INIT: Hostname  :" + myHostname);
console.log("INIT: Websocket :" + wsURI);
console.log("INIT: COMPLETE");
previous_entry = "";  
previous_operation = "INIT";                    // Set last operation
updateTunerDisplay(myChannels[0]);              // Update Tuner Display with Home Channel

// Load RX Window with Help Message
rxwinMSGhelp();
rxwinMSG('-');

/* Establish Websocket */
socket = new WebSocket(wsURI);

socket.onopen = function(e) {
  console.log("WSS: [open] Connection established");
  console.log("WSS: Sending to server");
  socket.send("CONNECT:" + myHostname);
};

socket.onmessage = function(event) {
	var wsRXD = event.data.split(':');
	if (wsRXD[0] == "RX") {
		rxwinMSG("RX: " + wsRXD[1]);
		console.log("WSS: RX: " + wsRXD[1]);
	} else if (wsRXD[0] == "RESPONSE") {
		rxwinMSG("RX: " + wsRXD[1]);
		console.log("WSS: RX: " + wsRXD[1]);
	};

	console.log(`WSS: [message] Data received from server: ${event.data}`);
};

socket.onclose = function(event) {
  if (event.wasClean) {
    console.log(`WSS: [close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
  } else {
    // e.g. server process killed or network down
    // event.code is usually 1006 in this case
    console.log('WSS: [close] Connection died');
  }
};

socket.onerror = function(error) {
	var rxWINerror = "WSS:ERROR connecting to " + wsURI;
	rxwinMSG(rxWINerror);
	console.log(`WSS: [error] ${error.message}`);
};