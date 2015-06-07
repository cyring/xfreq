/*
 * xfreq-webui.js by CyrIng
 *
 * XFreq
 * Copyright (C) 2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */

function Log(depth)
{
	this.depth=this.count=depth;
/*
	this.GetDepth=function() {
		return(this.depth);
	}
*/
	this.div=document.createElement("div");
	this.div.id="Log";
	this.div.draggable=true;
	this.div.setAttribute("ondragstart", "Widget.Drag(event)");

	this.trace=document.createElement("div");
	this.trace.id="Trace";
	this.div.appendChild(this.trace);

	this.controlBar=document.createElement("div");
	this.controlBar.id="ControlBar";
	this.trace.appendChild(this.controlBar);

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
	this.suspendBtn.onclick=function() {this.BtnHandler("SuspendBtn")};
	this.suspendText=document.createTextNode("Suspend");
	this.suspendBtn.appendChild(this.suspendText);
	this.ctrlButtons.appendChild(this.suspendBtn);

	this.resumeBtn=document.createElement("button");
	this.resumeBtn.id="ResumeBtn";
	this.resumeBtn.type="button";
	this.resumeBtn.disabled=true;
	this.resumeBtn.onclick=function() {this.BtnHandler("ResumeBtn")};
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
	this.traceDepth.value=this.depth;
	this.traceCtrl.appendChild(this.traceDepth);

	this.submitDepth=document.createElement("input");
	this.submitDepth.id="SubmitDepth";
	this.submitDepth.type="button";
	this.submitDepth.value="OK";
	this.submitDepth.onclick=function() {this.BtnHandler("TraceDepth")};
	this.traceCtrl.appendChild(this.submitDepth);

	this.div.style.display="block";
	document.body.appendChild(this.div);

	this.div.style.left=Math.round((document.body.clientWidth - this.div.offsetWidth)
				* Math.random()) + "px";
	this.div.style.top=Math.round((document.body.clientHeight - this.div.offsetHeight)
				* Math.random()) + "px";

	this.Trace=function(str) {
		var tag="<pre>" + str + "</pre>";

		if(!--this.count)
		{
			this.trace.innerHTML=tag;
			this.count=this.depth;
		}
		else
			this.trace.innerHTML=tag + this.Trace.innerHTML;
	}

	this.UpdateState=function(str)
	{
		this.socketState.innerHTML="XFreq WebSocket [" + str + "]";
	}

	this.BtnHandler=function(id)
	{
		switch(id) {
		case "TraceDepth":
			this.depth=this.traceDepth.value;
		break;
		default:
			XFreq.WS.send(JSON.stringify(id));
		}
	}

	this.ToggleBtn=function(v)
	{
		if(v == false) {
			this.suspendBtn.disabled=false;
			this.resumeBtn.disabled=true;
		} else {
			this.suspendBtn.disabled=true;
			this.resumeBtn.disabled=false;
		}
	}

	this.Draw=function()
	{
	}
}


function Widget(id)
{
	this.div=null;
	this.gfx=null;
	this.gtx=null;
	this.thickness=0;

	this.div=document.createElement("div");
	this.div.setAttribute("class", "Widget");
	this.div.setAttribute("id", id);
	this.div.setAttribute("draggable", "true");
	this.div.setAttribute("ondragstart", "Widget.Drag(event)");

	this.gfx=document.createElement("canvas");
	this.gtx=this.gfx.getContext("2d");
	this.gfx.height=SHM.P[0].CPU * 16;
	this.gfx.width=SHM.P[0].Boost[9] * 16;
	this.div.style.height=(this.gfx.height - 0) + "px";
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
}

Widget.Drag=function(event)
{
	var style=window.getComputedStyle(event.target, null);
	event.dataTransfer.setData("text",
				event.target.id
				+ ','
				+ (parseInt(style.getPropertyValue("left"),10) - event.clientX)
				+ ','
				+ (parseInt(style.getPropertyValue("top"),10) - event.clientY));
}

Widget.Drop=function(event)
{
	var item=event.dataTransfer.getData("text").split(',');
	var src=document.getElementById(item[0]);
	src.style.left=(event.clientX + parseInt(item[1],10)) + "px";
	src.style.top=(event.clientY + parseInt(item[2],10)) + "px";
	event.preventDefault();
	return(false);
}



var UI={ WinStack:[], Launcher:null };

UI.Init=function(bodyWidth, bodyHeight, allMargins)
{
	document.body.style.width=(bodyWidth - 4 * allMargins) + "px";
	document.body.style.height=(bodyHeight - 4 * allMargins) + "px";
	document.body.style.margin=allMargins + "px";

	UI.Launcher=document.getElementById("Launcher");
	UI.Add("Log");
}

UI.Brand=function(id)
{
	document.getElementById(id).innerHTML=SHM.P[0].Brand;
}

UI.Add=function(kind)
{
	var widget=null;
	switch(kind)
	{
	case "Log":
		widget=new Log(1);
	break;
	case "Core":
		widget=new Widget(kind + Math.round(1000*Math.random()));
	break;
	}
	if(widget != null)
	{
		UI.WinStack.push(widget);

		var node=document.createElement("li");
		node.onclick=function()
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
				}
		var text=document.createTextNode(kind);
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
