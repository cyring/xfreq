/*
 * xfreq-webui.js by CyrIng
 *
 * XFreq
 * Copyright (C) 2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */

var SHM={ H:{}, P:[], C:[] };

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

function HistoryEx(v)
{
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
var History=new HistoryEx(1);

var uCanvas={div:null, g:null, gc:null, thickness:0};

function uBuild()
{
	document.getElementById("Brand").innerHTML=SHM.P[0].Brand;
	uCanvas.div=document.createElement("div");
	uCanvas.div.setAttribute("id", "Chart");
	uCanvas.g=document.createElement("canvas");
	uCanvas.gc=uCanvas.g.getContext("2d");
	uCanvas.g.height=SHM.P[0].CPU * 16;
	uCanvas.g.width=SHM.P[0].Boost[9] * 16;
	uCanvas.div.style.height=(uCanvas.g.height - 0) + "px";
	uCanvas.div.style.width=uCanvas.g.width + "px";
	uCanvas.div.appendChild(uCanvas.g);
	document.body.appendChild(uCanvas.div);
}

function uDraw()
{
	var Bar=["#6666b0", "#00aa66", "#e49400", "#e49400", "#e49400", "#e49400", "#e49400", "#e49400", "#fd0000", "#fd0000"];

	var h=uCanvas.div.offsetHeight / SHM.P[0].CPU;
	uCanvas.thickness=h - 4;

	uCanvas.gc.clearRect(0, 0, uCanvas.div.offsetWidth, uCanvas.div.offsetHeight);
	for(cpu=0; cpu < SHM.P[0].CPU; cpu++)
	{
		x=Math.round(SHM.C[cpu].RelativeRatio * 16);
		y=(h * cpu) + 2;

		for(i=0; i < 9; i++)
			if(SHM.P[0].Boost[i] != 0)
				if(!(SHM.C[cpu].RelativeRatio > SHM.P[0].Boost[i]))
					break;

		uCanvas.gc.strokeStyle=Bar[i];
		uCanvas.gc.strokeRect(0, y, x, uCanvas.thickness);
	}
}

function XFreqWebUI()
{
	document.getElementById("HistoryDepth").value=History.GetDepth();
}
