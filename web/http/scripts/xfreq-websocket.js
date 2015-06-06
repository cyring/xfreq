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
		ControlBar.UpdateState("XFreq WebSocket [" + err + "]");
	}
	XFreq.WS.onopen=function(evt)
	{
		ControlBar.UpdateState("XFreq WebSocket [" + evt.type + "]");
		ControlBar.ToggleBtn(false);
	}

	XFreq.WS.onclose=function(evt)
	{
		ControlBar.UpdateState("XFreq WebSocket [" + evt.type + "]");
		ControlBar.ToggleBtn(true);
	}
	XFreq.WS.onmessage=function(evt)
	{
		var Obj={};
		try {
			var Obj=JSON.parse(evt.data);
			XFreq.Transmission=Obj.Transmission;
		}
		catch(err) {
			Trace.LogWindow(err + evt);
		}
		ControlBar.ToggleBtn(XFreq.Transmission.Suspended);

		switch(XFreq.Transmission.Stage)
		{
		case 0:
			SHM.H=Obj.H;
			SHM.P[0]=Obj.P;

			document.getElementById("Brand").innerHTML=SHM.P[0].Brand;
		break;
		case 1:
			SHM.P[1]=Obj.P;
			SHM.C=Obj.C;

			for(i in Win) {
				Win[i].Draw();
			}
		break;
		}
		if(document.getElementById("Log").style.display != "none")
			Trace.LogWindow(JSON.stringify(SHM, null, 2));
	}
}
