/*
 * xfreq-webui.js by CyrIng
 *
 * XFreq
 * Copyright (C) 2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */

function Log(depth)
{
	var _depth=_count=depth;

	this.div=document.createElement("div");
	this.div.id="Log";
	this.div.draggable=true;
	this.div.setAttribute("ondragstart", "UI.Drag(event)");

	this.trace=document.createElement("div");
	this.trace.id="Trace";
	this.div.appendChild(this.trace);

	this.controlBar=document.createElement("div");
	this.controlBar.id="ControlBar";
	this.div.appendChild(this.controlBar);

	this.socketState=document.createElement("div");
	this.socketState.id="SocketState";
	this.controlBar.appendChild(this.socketState);

	this.ctrlButtons=document.createElement("div");
	this.ctrlButtons.id="CtrlButtons";
	this.controlBar.appendChild(this.ctrlButtons);

	this.suspendBtn=document.createElement("button");
	this.suspendBtn.id="SuspendBtn";
	this.suspendBtn.type="button";
	this.suspendBtn.disabled=true;
	this.suspendBtn.addEventListener("click", this, false);
	this.suspendText=document.createTextNode("Suspend");
	this.suspendBtn.appendChild(this.suspendText);
	this.ctrlButtons.appendChild(this.suspendBtn);

	this.resumeBtn=document.createElement("button");
	this.resumeBtn.id="ResumeBtn";
	this.resumeBtn.type="button";
	this.resumeBtn.disabled=true;
	this.resumeBtn.addEventListener("click", this, false);
	this.resumeText=document.createTextNode("Resume");
	this.resumeBtn.appendChild(this.resumeText);
	this.ctrlButtons.appendChild(this.resumeBtn);

	this.traceCtrl=document.createElement("div");
	this.traceCtrl.id="TraceCtrl";
	this.controlBar.appendChild(this.traceCtrl);

	this.traceDepth=document.createElement("input");
	this.traceDepth.id="TraceDepth";
	this.traceDepth.type="number";
	this.traceDepth.min=1;
	this.traceDepth.max=9;
	this.traceDepth.maxlength=2;
	this.traceDepth.value=_depth.toString();
	this.traceCtrl.appendChild(this.traceDepth);

	this.submitDepth=document.createElement("input");
	this.submitDepth.id="SubmitDepth";
	this.submitDepth.type="button";
	this.submitDepth.value="OK";
	this.submitDepth.addEventListener("click", this, false);
	this.traceCtrl.appendChild(this.submitDepth);

	this.div.style.display="none";
	document.body.appendChild(this.div);

	this.div.style.left=Math.round((document.body.clientWidth - this.div.offsetWidth)
				* Math.random()) + "px";
	this.div.style.top=Math.round((document.body.clientHeight - this.div.offsetHeight)
				* Math.random()) + "px";

	this.handleEvent=function(event)
	{
		switch(event.target.id) {
		case "SubmitDepth":
			_depth=parseInt(this.traceDepth.value, 10);
		break;
		default:
			CX[0].WS.send(JSON.stringify(event.target.id));
		}
	}

	this.Trace=function(str) {
		var tag="<pre>" + str + "</pre>";

		if(!--_count)
		{
			this.trace.innerHTML=tag;
			_count=_depth;
		}
		else
			this.trace.innerHTML=tag + "<hr>" + this.trace.innerHTML;
	}

	this.UpdateState=function(str)
	{
		this.socketState.innerHTML=str;
	}

	this.ToggleBtn=function(state)
	{
		this.suspendBtn.disabled=state;
		this.resumeBtn.disabled=!state;
	}

	this.Draw=function()
	{
		if(this.div.style.display != "none")
			this.Trace(JSON.stringify(SHM, null, 2));
	}
}


function Widget(id)
{
	this.kind=id;
	this.div=null;
	this.gfx=null;
	this.gtx=null;

	this.div=document.createElement("div");
	this.div.setAttribute("class", "Widget");
	this.div.setAttribute("id", id + "_" + ++UI.Widgets);
	this.div.setAttribute("draggable", "true");
	this.div.setAttribute("ondragstart", "UI.Drag(event)");

	this.gfx=document.createElement("canvas");
	this.gtx=this.gfx.getContext("2d");
	switch(this.kind)
	{
		case "CORES":
		{
			this.gfx.width=26 + (SHM.P[0].Boost[9] * 16);
			this.gfx.height=SHM.P[0].CPU * 16;

			var barStyle=[
				UI.Colors["Low"],
				UI.Colors["Medium"],
				UI.Colors["High"],
				UI.Colors["High"],
				UI.Colors["High"],
				UI.Colors["High"],
				UI.Colors["High"],
				UI.Colors["High"],
				UI.Colors["Turbo"],
				UI.Colors["Turbo"],
			];
		}
		break;
		case "CSTATES":
		{
			this.gfx.width=(SHM.P[0].CPU * 48) + 8;
			this.gfx.height=(10 + 2) * 16;
		}
		break;
		case "TEMPS":
		{
			this.gfx.width=8 * 32;
			this.gfx.height=(10 + 2) * 16;

			var hsv=[]; hsv=rgbToHsv(rgbToTriple(UI.Colors["Temps"]));
			var barStyle=[];
			var Temps=[[]], cpu=0;
			for(cpu=0; cpu < SHM.P[0].CPU; cpu++)
			{
				hsv[0]+=0.1;
				barStyle[cpu]=tripleToRgb(hsvToRgb(hsv));

				for(i=0; i < 9; i++)
					Temps[cpu]=[];
			}
		}
		break;
	}		
	this.div.style.height=this.gfx.height + "px";
	this.div.style.width=this.gfx.width + "px";
	this.div.style.left=Math.round((document.body.clientWidth - this.gfx.width)
				* Math.random()) + "px";
	this.div.style.top=Math.round((document.body.clientHeight - this.gfx.height)
				* Math.random()) + "px";
	this.div.appendChild(this.gfx);

	this.div.style.display="block";
	document.body.appendChild(this.div);

	this.Draw=function()
	{
		switch(this.kind)
		{
		case "CORES":
		{
			var cpu=0, h=this.gfx.height / SHM.P[0].CPU;
			var thickness=h - 4;

			this.gtx.clearRect(0, 0, this.gfx.width, this.gfx.height);
			this.gtx.fillStyle=UI.Colors["Label"];
			for(cpu=0; cpu < SHM.P[0].CPU; cpu++)
			{
				for(i=0; i < 9; i++)
					if(SHM.P[0].Boost[i] != 0)
						if(!(SHM.C[cpu].RelativeRatio > SHM.P[0].Boost[i]))
							break;
				this.gtx.strokeStyle=barStyle[i];

				var x=Math.round(SHM.C[cpu].RelativeRatio * 16),
				    y=(h * cpu) + 2;
				this.gtx.strokeRect(24, y, x, thickness);

				this.gtx.fillText("#" + cpu, 2, y + 8);
				this.gtx.fillText(SHM.C[cpu].RelativeRatio.toFixed(1), this.gfx.width - 24, y + 8);
			}
		}
		break;
		case "CSTATES":
		{
			var cpu=0, x=0, y=0, w=0, h=0;

			this.gtx.clearRect(0, 0, this.gfx.width, this.gfx.height);
			for(cpu=0; cpu < SHM.P[0].CPU; cpu++)
			{
				x=(cpu * 48) + 8;
				this.gtx.fillStyle=UI.Colors["Label"];
				this.gtx.fillText("#" + cpu, x, 16);

				this.gtx.beginPath();
				x=(cpu * 48) + 8;
				w=Math.round(10 * SHM.C[cpu].State.C0);
				if(w > 0)
				{
					h=w * 16;
					y=this.gfx.height - h - 16;
					this.gtx.moveTo(x, y);
					this.gtx.lineTo(x, y + h);
					this.gtx.lineTo(x + 4, y + h);
					this.gtx.lineTo(x + 4, y);
					this.gtx.closePath();
				}
				this.gtx.fillStyle=UI.Colors["C0"];
				this.gtx.fill();

				this.gtx.beginPath();
				x=x + 8;
				w=Math.round(10 * SHM.C[cpu].State.C1);
				if(w > 0)
				{
					h=w * 16;
					y=this.gfx.height - h - 16;
					this.gtx.moveTo(x, y);
					this.gtx.lineTo(x, y + h);
					this.gtx.lineTo(x + 4, y + h);
					this.gtx.lineTo(x + 4, y);
					this.gtx.closePath();
				}
				this.gtx.fillStyle=UI.Colors["C1"];
				this.gtx.fill();

				this.gtx.beginPath();
				x=x + 8;
				w=Math.round(10 * SHM.C[cpu].State.C3);
				if(w > 0)
				{
					h=w * 16;
					y=this.gfx.height - h - 16;
					this.gtx.moveTo(x, y);
					this.gtx.lineTo(x, y + h);
					this.gtx.lineTo(x + 4, y + h);
					this.gtx.lineTo(x + 4, y);
					this.gtx.closePath();
				}
				this.gtx.fillStyle=UI.Colors["C3"];
				this.gtx.fill();

				this.gtx.beginPath();
				x=x + 8;
				w=Math.round(10 * SHM.C[cpu].State.C6);
				if(w > 0)
				{
					h=w * 16;
					y=this.gfx.height - h - 16;
					this.gtx.moveTo(x, y);
					this.gtx.lineTo(x, y + h);
					this.gtx.lineTo(x + 4, y + h);
					this.gtx.lineTo(x + 4, y);
					this.gtx.closePath();
				}
				this.gtx.fillStyle=UI.Colors["C6"];
				this.gtx.fill();

				this.gtx.beginPath();
				x=x + 8;
				w=Math.round(10 * SHM.C[cpu].State.C7);
				if(w > 0)
				{
					h=w * 16;
					y=this.gfx.height - h - 16;
					this.gtx.moveTo(x, y);
					this.gtx.lineTo(x, y + h);
					this.gtx.lineTo(x + 4, y + h);
					this.gtx.lineTo(x + 4, y);
					this.gtx.closePath();
				}
				this.gtx.fillStyle=UI.Colors["C7"];
				this.gtx.fill();
			}
		}
		break;
		case "TEMPS":
		{
			var cpu=0, x=0, h=0;
			this.gtx.clearRect(0, 0, this.gfx.width, this.gfx.height);
			for(cpu=0; cpu < SHM.P[0].CPU; cpu++)
			{
				x=(32 * cpu) + 8;
				this.gtx.fillStyle=UI.Colors["Label"];
				this.gtx.fillText("#" + cpu, x, 16);
				this.gtx.fillStyle=barStyle[cpu];
				this.gtx.fillText(SHM.C[cpu].TjMax.Target - SHM.C[cpu].ThermStat.DTS, x, this.gfx.height - 8);

				this.gtx.beginPath();
				this.gtx.strokeStyle=barStyle[cpu];
				var i=0;
				Temps[cpu][i]=Temps[cpu][i + 1];
				x=(32 * i) + 8;
				this.gtx.moveTo(x, Temps[cpu][i]);

				for(i=1; i < 8; i++) {
					Temps[cpu][i]=Temps[cpu][i + 1];
					x=(32 * i) + 8;
					this.gtx.lineTo(x, Temps[cpu][i]);
				}

				Temps[cpu][i]=2 * SHM.C[cpu].ThermStat.DTS;
				x=(32 * i) + 8;
				this.gtx.lineTo(x, Temps[cpu][i]);
				this.gtx.stroke();
			}
		}
		break;
		}
	}
}


GetStyleValue=function(selector, property)
{
	var id=document.getElementById(selector), tmp=null, value=null;

	if(id == null) {
		id=tmp=document.createElement("div");
		id.setAttribute("id", selector);
		id.style.display="none";
		document.body.appendChild(id);
	}
	value=window.getComputedStyle(id).getPropertyValue(property);

	if(tmp != null)
		tmp.parentElement.removeChild(id);
	return(value);
}

var UI={ WinStack:[], Launcher:null, Widgets:0, Colors:[] };

UI.Init=function(bodyWidth, bodyHeight, allMargins)
{
	document.body.style.width=(bodyWidth - 4 * allMargins) + "px";
	document.body.style.height=(bodyHeight - 4 * allMargins) + "px";
	document.body.style.margin=allMargins + "px";

	UI.Launcher=document.getElementById("Launcher");

	var P=["Label", "Low", "Medium", "High", "Turbo", "C0", "C1", "C3", "C6", "C7", "Temps"];
	for(i in P)
		UI.Colors[P[i]]=GetStyleValue("Colors_" + P[i], "color");

	UI.Add("Log");
}

UI.Brand=function(id)
{
	document.getElementById(id).innerHTML=SHM.P[0].Brand;
}

UI.Drag=function(event)
{
	var style=window.getComputedStyle(event.target, null);
	event.dataTransfer.setData("text",
				event.target.id
				+ ','
				+ (parseInt(style.getPropertyValue("left"),10) - event.clientX)
				+ ','
				+ (parseInt(style.getPropertyValue("top"),10) - event.clientY));
}

UI.Drop=function(event)
{
	var item=event.dataTransfer.getData("text").split(',');
	var src=document.getElementById(item[0]);
	src.style.left=(event.clientX + parseInt(item[1],10)) + "px";
	src.style.top=(event.clientY + parseInt(item[2],10)) + "px";
	event.preventDefault();
	return(false);
}

UI.Add=function(kind)
{
	var widget=null;
	switch(kind)
	{
	case "Log":
		widget=new Log(1);
	break;
	default:
		widget=new Widget(kind);
	break;
	}
	if(widget != null)
	{
		UI.WinStack.push(widget);

		var node=document.createElement("li");
		node.addEventListener("click", function()
				{
					switch(widget.div.style.display)
					{
					case "none":
						widget.div.style.display="block";
					break;
					case "block":
						widget.div.style.display="none";
					break;
					}
				}, false);
		var text=document.createTextNode(widget.div.id);
		node.appendChild(text);

		UI.Launcher.appendChild(node);
	}
}

UI.Del=function(id)
{
	var i;
	for(i in UI.WinStack) {
		if(UI.WinStack[i] == id) {
			UI.WinStack.splice(i, 1);
			break;
		}
	}
}

UI.Refresh=function()
{
	var i;
	for(i in UI.WinStack) {
		UI.WinStack[i].Draw();
	}
}

UI.Settings=function(what)
{

	switch(what)
	{
		case "COLORS":
		{
		}
		break;
	}
}

function rgbToHsv(triple)
{
	r=triple[0]/255, g=triple[1]/255, b=triple[2]/255;
	var max=Math.max(r, g, b), min=Math.min(r, g, b);
	var h, s, v=max;

	var d=max - min;
	s=max == 0 ? 0 : d / max;

	if(max == min){
		h = 0; // achromatic
	} else {
		switch(max) {
			case r: h = (g - b) / d + (g < b ? 6 : 0); break;
			case g: h = (b - r) / d + 2; break;
			case b: h = (r - g) / d + 4; break;
		}
		h /= 6;
	}
	return([h, s, v]);
}

function hsvToRgb(triple)
{
	var h=triple[0], s=triple[1], v=triple[2];
	var r, g, b;

	var i=Math.floor(h * 6);
	var f=h * 6 - i;
	var p=v * (1 - s);
	var q=v * (1 - f * s);
	var t=v * (1 - (1 - f) * s);

	switch(i % 6) {
		case 0: r=v, g=t, b=p; break;
		case 1: r=q, g=v, b=p; break;
		case 2: r=p, g=v, b=t; break;
		case 3: r=p, g=q, b=v; break;
		case 4: r=t, g=p, b=v; break;
		case 5: r=v, g=p, b=q; break;
	}
	return([r * 255, g * 255, b * 255]);
}

function rgbToTriple(rgb)
{
	var str=rgb.match(/(\d+)/g);
	var triple=[];
	triple[0]=parseInt(str[0]);
	triple[1]=parseInt(str[1]);
	triple[2]=parseInt(str[2]);
	return(triple);
}

function tripleToRgb(triple)
{
	return("rgb("	+ Math.round(triple[0])	+ ", "
			+ Math.round(triple[1])	+ ", "
			+ Math.round(triple[2]) + ")");
}
