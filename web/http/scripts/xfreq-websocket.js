/*
 * xfreq-websocket.js by CyrIng
 *
 * XFreq
 * Copyright (C) 2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */


var XFreqWS;

function XFreqWebSocket()
{
	try {
		XFreqWS=new WebSocket("ws://" + location.hostname + ":" + location.port, "json-lite");
	}
	catch(err) {
		document.getElementById("SocketState").innerHTML="XFreq WebSocket [" + err + "]";
	}
	XFreqWS.onopen=function(evt)
	{
		document.getElementById("SocketState").innerHTML="XFreq WebSocket [" + evt.type + "]";
		document.getElementById("SuspendBtn").disabled=false;
	}

	XFreqWS.onclose=function(evt)
	{
		document.getElementById("SocketState").innerHTML="XFreq WebSocket [" + evt.type + "]";
		document.getElementById("SuspendBtn").disabled=true;
	}
	XFreqWS.onmessage=function(evt)
	{
		var SHM;
		try {
			SHM=JSON.parse(evt.data);
		}
		catch(err) {
			History.LogWindow(err + evt);
		}
		var str;
		try {
			str=JSON.stringify(SHM, null, 2);
		}
		catch(err) {
			str=err.message;
		}
		History.LogWindow(str);

		if(!SHM.Transmission.Suspended) {
			document.getElementById("SuspendBtn").disabled=false;
			document.getElementById("ResumeBtn").disabled=true;
		} else {
			document.getElementById("SuspendBtn").disabled=true;
			document.getElementById("ResumeBtn").disabled=false;
		}
	}
}
