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
		switch(this.kind)
		{
		case "CORES":
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
			var thickness=h - 4;

			this.gtx.clearRect(0, 0, this.div.offsetWidth, this.div.offsetHeight);
			this.gtx.fillStyle="#8fcefa";
			for(cpu=0; cpu < SHM.P[0].CPU; cpu++)
			{
				for(i=0; i < 9; i++)
					if(SHM.P[0].Boost[i] != 0)
						if(!(SHM.C[cpu].RelativeRatio > SHM.P[0].Boost[i]))
							break;
				this.gtx.strokeStyle=Bar[i];

				var x=Math.round(SHM.C[cpu].RelativeRatio * 16), y=(h * cpu) + 2;
				this.gtx.strokeRect(0, y, x, thickness);
				this.gtx.fillText(SHM.C[cpu].RelativeRatio.toFixed(1), this.gfx.width - 24, y + 8);
			}
		}
		break;
		case "CSTATES":
		{
			var cpu=0, h=this.div.offsetHeight / SHM.P[0].CPU;
			var thickness=h - 4;

			this.gtx.clearRect(0, 0, this.div.offsetWidth, this.div.offsetHeight);
			this.gtx.strokeStyle=this.gtx.fillStyle="#8fcefa";
			for(cpu=0; cpu < SHM.P[0].CPU; cpu++)
			{
				var x=Math.round(SHM.C[cpu].State.C0 * this.div.offsetWidth - 40), y=(h * cpu) + 2;
				this.gtx.strokeRect(0, y, x, thickness);
				this.gtx.fillText(100 * SHM.C[cpu].State.C0.toFixed(1) + "%", this.gfx.width - 32, y + 8);
			}
		}
		break;
		case "TEMPS":
		{
			var cpu=0, h=this.div.offsetHeight / SHM.P[0].CPU;
			this.gtx.clearRect(0, 0, this.div.offsetWidth, this.div.offsetHeight);
			this.gtx.fillStyle="#8fcefa";
			for(cpu=0; cpu < SHM.P[0].CPU; cpu++)
			{
				var y=(h * cpu) + 10;
				this.gtx.fillText("#" + cpu, 2, y);
				this.gtx.fillText(SHM.C[cpu].TjMax.Target - SHM.C[cpu].ThermStat.DTS, this.gfx.width - 20, y);
			}
		}
		break;
		}
	}
}



var UI={ WinStack:[], Launcher:null, Widgets:0 };

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
