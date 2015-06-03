/*
 * xfreq-websocket.js by CyrIng
 *
 * XFreq
 * Copyright (C) 2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */


var XFreq={WS:null, Host:location.hostname, Port:location.port, Protocol: "json-lite", Transmission:{}};

function XFreqWebSocket()
{
	try {
		XFreq.WS=new WebSocket("ws://" + XFreq.Host + ":" + XFreq.Port, XFreq.Protocol);
	}
	catch(err) {
		document.getElementById("SocketState").innerHTML="XFreq WebSocket [" + err + "]";
	}
	XFreq.WS.onopen=function(evt)
	{
		document.getElementById("SocketState").innerHTML="XFreq WebSocket [" + evt.type + "]";
		document.getElementById("SuspendBtn").disabled=false;
	}

	XFreq.WS.onclose=function(evt)
	{
		document.getElementById("SocketState").innerHTML="XFreq WebSocket [" + evt.type + "]";
		document.getElementById("SuspendBtn").disabled=true;
	}
	XFreq.WS.onmessage=function(evt)
	{
		var Obj={};
		try {
			var Obj=JSON.parse(evt.data);
			XFreq.Transmission=Obj.Transmission;
		}
		catch(err) {
			History.LogWindow(err + evt);
		}
		if(XFreq.Transmission.Suspended == false) {
			document.getElementById("SuspendBtn").disabled=false;
			document.getElementById("ResumeBtn").disabled=true;
		} else {
			document.getElementById("SuspendBtn").disabled=true;
			document.getElementById("ResumeBtn").disabled=false;
		}
		switch(XFreq.Transmission.Stage)
		{
		case 0:
			SHM.H=Obj.H;
			SHM.P[0]=Obj.P;

			uBuild();
		break;
		case 1:
			SHM.P[1]=Obj.P;
			SHM.C=Obj.C;

			uDraw();
		break;
		}
		History.LogWindow(JSON.stringify(SHM, null, 2));
	}
}
