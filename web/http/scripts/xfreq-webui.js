/*
 * xfreq-webui.js by CyrIng
 *
 * XFreq
 * Copyright (C) 2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */

var SHM={ H:{}, P:[], C:[] };

var Win=[];

var ControlBar=new function()
{
	this.UpdateState=function(str) {
		document.getElementById("SocketState").innerHTML="XFreq WebSocket [" + str + "]";
	}
	this.ToggleBtn=function(v) {
		if(v == false) {
			document.getElementById("SuspendBtn").disabled=false;
			document.getElementById("ResumeBtn").disabled=true;
		} else {
			document.getElementById("SuspendBtn").disabled=true;
			document.getElementById("ResumeBtn").disabled=false;
		}
	}
}

function TraceClass(v)
{
	this.depth=v;
	this.count=this.depth;
	this.SetDepth=function(v) {
		this.depth=v;
	}
	this.GetDepth=function() {
		return(this.depth);
	}
	this.LogWindow=function(str) {
		tag="<pre>" + str + "</pre>";

		if(!--this.count)
		{
			document.getElementById("Trace").innerHTML=tag;
			this.count=this.depth;
		}
		else
			document.getElementById("Trace").innerHTML=
			tag + document.getElementById("Trace").innerHTML;
	}
}

var Trace=new TraceClass(1);

function BtnHandler(id)
{
	switch(id) {
	case "TraceDepth":
		Trace.SetDepth(document.getElementById(id).value);
	break;
	default:
		XFreqWS.send(JSON.stringify(id));
	}
}

function uDrag(e)
{
	var style = window.getComputedStyle(e.target, null);
	e.dataTransfer.setData("text",
				e.target.id
				+ ','
				+ (parseInt(style.getPropertyValue("left"),10) - e.clientX)
				+ ','
				+ (parseInt(style.getPropertyValue("top"),10) - e.clientY));
}

function uDrop(e)
{
	var o=e.dataTransfer.getData("text").split(',');
	var src=document.getElementById(o[0]);
	src.style.left=(e.clientX + parseInt(o[1],10)) + "px";
	src.style.top=(e.clientY + parseInt(o[2],10)) + "px";
	e.preventDefault();
	return(false);
}

function Widget(id)
{
	this.div=null;
	this.gfx=null;
	this.gtx=null;
	this.thickness=0;

	this.Build=function(id)
	{
		this.div=document.createElement("div");
		this.div.setAttribute("class", "Widget");
		this.div.setAttribute("id", id);
		this.div.setAttribute("draggable", "true");
		this.div.setAttribute("ondragstart", "uDrag(event)");
		this.gfx=document.createElement("canvas");
		this.gtx=this.gfx.getContext("2d");
		this.gfx.height=SHM.P[0].CPU * 16;
		this.gfx.width=SHM.P[0].Boost[9] * 16;
		this.div.style.height=(this.gfx.height - 0) + "px";
		this.div.style.width=this.gfx.width + "px";
		this.div.style.left=Math.round(1280 * Math.random()) + "px";
		this.div.style.top=Math.round(1024 * Math.random()) + "px";
		this.div.appendChild(this.gfx);
		document.body.appendChild(this.div);
	}

	this.Draw=function()
	{
		var Bar=[
			"#6666b0",
			"#00aa66",
			"#e49400",
			"#e49400",
			"#e49400",
			"#e49400",
			"#e49400",
			"#e49400",
			"#fd0000",
			"#fd0000"
			];

		var cpu=0, h=this.div.offsetHeight / SHM.P[0].CPU;
		this.thickness=h - 4;

		this.gtx.clearRect(0, 0, this.div.offsetWidth, this.div.offsetHeight);

		for(cpu=0; cpu < SHM.P[0].CPU; cpu++)
		{
			for(i=0; i < 9; i++)
				if(SHM.P[0].Boost[i] != 0)
					if(!(SHM.C[cpu].RelativeRatio > SHM.P[0].Boost[i]))
						break;
			this.gtx.strokeStyle=Bar[i];

			var x=Math.round(SHM.C[cpu].RelativeRatio * 16), y=(h * cpu) + 2;
			this.gtx.strokeRect(0, y, x, this.thickness);
			this.gtx.fillStyle="#8fcefa";
			this.gtx.fillText(SHM.C[cpu].RelativeRatio.toFixed(1), this.gfx.width - 20, y + 8);
		}
	}
	this.Build(id);
};

function XFreqWebUI()
{
	document.body.style.width="1280px";	//window.innerWidth + "px";
	document.body.style.height="1024px";	//window.innerHeight + "px";
	document.getElementById("TraceDepth").value=Trace.GetDepth();
}
