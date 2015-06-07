/*
 * xfreq-websocket.js by CyrIng
 *
 * XFreq
 * Copyright (C) 2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */


var SHM={ H:{}, P:[], C:[] };

function XFreq(host, port)
{
	this.WS=null;
	this.Host=host;
	this.Port=port;
	this.Protocol="json-lite";
	this.Transmission={};

	try {
		this.WS=new WebSocket("ws://" + this.Host + ":" + this.Port, this.Protocol);
	}
	catch(err) {
		UI.WinStack[0].UpdateState("XFreq WebSocket [" + err + "]");
	}
	this.WS.onopen=function(event)
	{
		UI.WinStack[0].UpdateState("XFreq WebSocket [" + event.type + "]");
		UI.WinStack[0].ToggleBtn(false);
	}

	this.WS.onclose=function(event)
	{
		UI.WinStack[0].UpdateState("XFreq WebSocket [" + event.type + "]");
		UI.WinStack[0].ToggleBtn(true);
	}
	this.WS.onmessage=function(event)
	{
		var Obj={};
		try {
			var Obj=JSON.parse(event.data);
			this.Transmission=Obj.Transmission;
		}
		catch(err) {
			WinStack[0].Trace(err + event);
		}
		UI.WinStack[0].ToggleBtn(this.Transmission.Suspended);

		switch(this.Transmission.Stage)
		{
		case 0:
			SHM.H=Obj.H;
			SHM.P[0]=Obj.P;

			UI.Brand("Brand");
		break;
		case 1:
			SHM.P[1]=Obj.P;
			SHM.C=Obj.C;

			UI.Refresh();
		break;
		}
		if(UI.WinStack[0].div.style.display != "none")
			UI.WinStack[0].Trace(JSON.stringify(SHM, null, 2));
	}
}
