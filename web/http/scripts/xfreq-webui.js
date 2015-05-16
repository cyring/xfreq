/*
 * xfreq-webui.js by CyrIng
 *
 * XFreq
 * Copyright (C) 2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */

function BtnHandler(id)
{
	switch(id) {
	case "HistoryDepth":
		History.SetDepth(document.getElementById(id).value);
	break;
	default:
		XFreqWS.send(JSON.stringify(id));
	}
}

function HistoryEx(v) {
	this.depth=v;
	this.count=this.depth;
	this.SetDepth=function(v) {this.depth=v;}
	this.GetDepth=function() {return(this.depth);}
	this.LogWindow=function(str) {
		tag="<pre>" + str + "</pre>";

		if(!--this.count)
		{
			document.getElementById("LogHistory").innerHTML=tag;
			this.count=this.depth;
		}
		else
			document.getElementById("LogHistory").innerHTML=
			tag + document.getElementById("LogHistory").innerHTML;
	}
}
var History=new HistoryEx(2);

function XFreqWebUI()
{
	document.getElementById("HistoryDepth").value=History.GetDepth();
}
