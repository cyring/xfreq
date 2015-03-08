/*
 * xfreq-gui.c by CyrIng
 *
 * Copyright (C) 2013-2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */

#if defined(FreeBSD)
//#include <sys/param.h>
#else
#define _GNU_SOURCE
#include <sched.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/Xresource.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdatomic.h>

#if defined(FreeBSD)
#include <pthread_np.h>
#endif

#define	_APPNAME "XFreq-Gui"
#include "xfreq-smbios.h"
#include "xfreq-api.h"
#include "xfreq-gui.h"

static  char    Version[] = AutoDate;


unsigned long long int
	DumpRegister(unsigned long long int DecVal, char *pHexStr, char *pBinStr)
{
	const char HEX[0x10]=
		{
			'0',
			'1',
			'2',
			'3',

			'4',
			'5',
			'6',
			'7',

			'8',
			'9',
			'A',
			'B',

			'C',
			'D',
			'E',
			'F',
		};
	const char *BIN[0x10]=
		{
			"0000",
			"0001",
			"0010",
			"0011",

			"0100",
			"0101",
			"0110",
			"0111",

			"1000",
			"1001",
			"1010",
			"1011",

			"1100",
			"1101",
			"1110",
			"1111",
		};
	unsigned int I, H=0xf;
	for(I=1; I <= 16; I++)
	{
		const unsigned int B=H<<2, nibble=DecVal & 0xf;
		if(pHexStr != NULL)
			pHexStr[H]=HEX[nibble];
		if(pBinStr != NULL)
		{
			pBinStr[B  ]=BIN[nibble][0];
			pBinStr[B+1]=BIN[nibble][1];
			pBinStr[B+2]=BIN[nibble][2];
			pBinStr[B+3]=BIN[nibble][3];
		}
		H--; DecVal=DecVal>>4;
	}
	return(DecVal);
}

void	ClearMsg(uARG *A) {
	if(A->L.Output != NULL) {
		free(A->L.Output);
		A->L.Output=NULL;

		SetHListing(MAIN, 0);
		SetVListing(MAIN, 0);
		SetHScrolling(MAIN, 0);
		SetVScrolling(MAIN, 0);

		XSetForeground(A->display, A->W[MAIN].gc, A->W[MAIN].background);
		XFillRectangle(A->display, A->L.Page[MAIN].Pixmap, A->W[MAIN].gc,
				0,
				0,
				GetHFrame(MAIN) * One_Char_Width(MAIN),
				GetVFrame(MAIN) * One_Char_Height(MAIN));
	}
}

void	Output(uARG *A, const char *message)
{
	if(A->L.Output != NULL && GetVListing(MAIN) > GetVFrame(MAIN))
		ClearMsg(A);

	const size_t requestBlock=strlen(message) + 1;
	if(A->L.Output == NULL)
		A->L.Output=calloc(1, requestBlock);
	else {
		const size_t allocBlock=strlen(A->L.Output) + 1;
		A->L.Output=realloc(A->L.Output, allocBlock + requestBlock);
	}
	strcat(A->L.Output, message);
}

// Drawing Button functions.
void	DrawDecorationButton(uARG *A, WBUTTON *wButton)
{
	switch(wButton->ID) {
		case ID_SAVE:
		{
			XPoint xpoints[]={
					{	.x=+wButton->x	, .y=+wButton->y	},
					{	.x=+wButton->w	, .y=0			},
					{	.x=0		, .y=+wButton->h	},
					{	.x=-wButton->w+3, .y=0			},
					{	.x=-3		, .y=-3			},
					{	.x=0		, .y=-wButton->h+3	}
			};
			XDrawLines(	A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc,
					xpoints, sizeof(xpoints) / sizeof(XPoint), CoordModePrevious);
			XPoint xpoly[]={
					{	.x=wButton->x+4			, .y=wButton->y+wButton->h-(wButton->h >> 1)	},
					{	.x=wButton->x+wButton->w-2	, .y=wButton->y+wButton->h-(wButton->h >> 1)	},
					{	.x=wButton->x+wButton->w-2	, .y=wButton->y+wButton->h-1	},
					{	.x=wButton->x+4			, .y=wButton->y+wButton->h-1	}
			};
			XFillPolygon(	A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc,
					xpoly, sizeof(xpoly) / sizeof(XPoint), Nonconvex, CoordModeOrigin);

			XSegment xsegments[]={
				{	.x1=wButton->x+3,		.y1=wButton->y+3,	.x2=wButton->x+3,		.y2=wButton->y+4},
				{	.x1=wButton->x+wButton->w-3,	.y1=wButton->y+3,	.x2=wButton->x+wButton->w-3,	.y2=wButton->y+4}
			};
			XDrawSegments(	A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc,
					xsegments, sizeof(xsegments)/sizeof(XSegment));
		}
			break;
		case ID_QUIT:
		{
			XFillRectangle(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc,
					wButton->x + (wButton->w >> 1) - 1, wButton->y, 3, (wButton->h >> 1));
			XDrawArc(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc,
					wButton->x,
					wButton->y,
					wButton->w,
					wButton->h,
					34 << 8, 66 << 8);
		}
			break;
		case ID_MIN:
		{
			int inner=(MIN(wButton->w, wButton->h) >> 1) + 2;
			XDrawRectangle(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc,
					wButton->x, wButton->y, wButton->w, wButton->h );
			XFillRectangle(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc,
					wButton->x + 2, wButton->y + 2, wButton->w - inner, wButton->h - inner);
		}
			break;
		case ID_CHART:
		{
			short int h=wButton->h / 5;
			XSegment sChart[]=
			{
				{.x1=wButton->x + 2,	.y1=wButton->y + (h * 1),	.x2=wButton->x + (3 * wButton->w / 4),	.y2=wButton->y + (h * 1)},
				{.x1=wButton->x + 2,	.y1=wButton->y + (h * 2),	.x2=wButton->x + (1 * wButton->w / 2),	.y2=wButton->y + (h * 2)},
				{.x1=wButton->x + 2,	.y1=wButton->y + (h * 3),	.x2=wButton->x + wButton->w - 2,	.y2=wButton->y + (h * 3)},
				{.x1=wButton->x + 2,	.y1=wButton->y + (h * 4),	.x2=wButton->x + (2 * wButton->w / 5),	.y2=wButton->y + (h * 4)}
			};
			int	nChart=sizeof(sChart)/sizeof(XSegment);

			XDrawRectangle(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc,
					wButton->x, wButton->y, wButton->w, wButton->h );
			XDrawSegments(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, sChart, nChart);
		}
			break;
	}
}

void	DrawScrollingButton(uARG *A, WBUTTON *wButton)
{
	switch(wButton->ID)
	{
		case ID_NORTH:
		{
			unsigned int inner=1;
			XPoint Arrow[3];
			unsigned int valign=wButton->h - (inner << 1);
			unsigned int halign=valign >> 1;
			Arrow[0].x=wButton->x + inner;	Arrow[0].y=wButton->y + wButton->h - inner;
			Arrow[1].x= +halign;		Arrow[1].y= -valign;
			Arrow[2].x= +halign;		Arrow[2].y= +valign;
			XFillPolygon(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, Arrow, 3, Nonconvex, CoordModePrevious);
		}
			break;
		case ID_SOUTH:
		{
			unsigned int inner=1;
			XPoint Arrow[3];
			unsigned int valign=wButton->h - (inner << 1);
			unsigned int halign=valign >> 1;
			Arrow[0].x=wButton->x + inner;	Arrow[0].y=wButton->y + inner;
			Arrow[1].x= +halign;		Arrow[1].y= +valign;
			Arrow[2].x= +halign;		Arrow[2].y= -valign;
			XFillPolygon(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, Arrow, 3, Nonconvex, CoordModePrevious);
		}
			break;
		case ID_EAST:
		{
			unsigned int inner=1;
			XPoint Arrow[3];
			unsigned int halign=wButton->w - (inner << 1);
			unsigned int valign=halign >> 1;
			Arrow[0].x=wButton->x + inner;	Arrow[0].y=wButton->y + inner;
			Arrow[1].x= +halign;		Arrow[1].y= +valign;
			Arrow[2].x= -halign;		Arrow[2].y= +valign;
			XFillPolygon(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, Arrow, 3, Nonconvex, CoordModePrevious);
		}
			break;
		case ID_WEST:
		{
			unsigned int inner=1;
			XPoint Arrow[3];
			unsigned int halign=wButton->w - (inner << 1);
			unsigned int valign=halign >> 1;
			Arrow[0].x=wButton->x + wButton->w - inner;	Arrow[0].y=wButton->y + inner;
			Arrow[1].x= -halign;				Arrow[1].y= +valign;
			Arrow[2].x= +halign;				Arrow[2].y= +valign;
			XFillPolygon(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, Arrow, 3, Nonconvex, CoordModePrevious);
		}
			break;
	}
}

void	DrawTextButton(uARG *A, WBUTTON *wButton)
{
	XDrawRectangle(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, wButton->x, wButton->y, wButton->w, wButton->h);

	if(wButton->Type == TEXT)
	{
		size_t length=strlen(wButton->Resource.Text);
		unsigned int inner=(wButton->w - (length * One_Char_Width(wButton->Target))) >> 1;

		XDrawString(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, wButton->x+inner, wButton->y+(A->W[wButton->Target].extents.ascent), wButton->Resource.Text, length);
	}
}

void	DrawIconButton(uARG *A, WBUTTON *wButton)
{
	unsigned int inner=wButton->w >> 2;
	char str[2]={wButton->Resource.Label, '\0'};

	XDrawRectangle(A->display, A->W[MAIN].pixmap.B, A->W[MAIN].gc, wButton->x, wButton->y, wButton->w, wButton->h);
	XDrawString(A->display, A->W[MAIN].pixmap.B, A->W[MAIN].gc, wButton->x+inner, wButton->y+(A->W[MAIN].extents.ascent), str, 1);
}

// Create a button, supplying the callback, the drawing & the collision functions.
void	CreateButton(uARG *A, WBTYPE Type, char ID, int Target, int x, int y, unsigned int w, unsigned int h, void *CallBack, RESOURCE *Resource, WBSTATE *WBState)
{
	WBUTTON *NewButton=calloc(1, sizeof(WBUTTON));

	NewButton->Type=Type;
	NewButton->ID=ID;

	NewButton->x=x;
	NewButton->y=y;
	NewButton->w=w;
	NewButton->h=h;

	NewButton->CallBack=CallBack;
	int G=NewButton->Target=Target;

	switch(NewButton->Type)
	{
		case DECORATION:
			NewButton->DrawFunc=DrawDecorationButton;
			break;
		case SCROLLING:
			NewButton->DrawFunc=DrawScrollingButton;
			break;
		case TEXT:
		{
			if(Resource != NULL) {
				NewButton->Resource.Text=malloc(strlen(Resource->Text) + 1);
				strcpy(NewButton->Resource.Text, Resource->Text);
			}
			else {
				NewButton->Type=ICON;
				NewButton->Resource.Label=0x20;
			}
			NewButton->DrawFunc=DrawTextButton;
		}
			break;
		case ICON:
		{
			G=MAIN;
			NewButton->Resource.Label=Resource->Label;
			NewButton->DrawFunc=DrawIconButton;
		}
			break;
	}
	NewButton->WBState.Func=(WBState != NULL) ? WBState->Func : NULL;
	NewButton->WBState.Key=(WBState != NULL) ? WBState->Key : NULL;

	NewButton->Chain=NULL;
	if(A->W[G].wButton[HEAD] == NULL)
		A->W[G].wButton[HEAD]=NewButton;
	else
		A->W[G].wButton[TAIL]->Chain=NewButton;
	A->W[G].wButton[TAIL]=NewButton;
}

// Destroy a button attached to a Widget.
void	DestroyButton(uARG *A, int G, char ID)
{
	WBUTTON *wButton=A->W[G].wButton[HEAD], *cButton=A->W[G].wButton[HEAD];
	while(cButton != NULL)
		if(cButton->ID == ID)
		{
			if(cButton == A->W[G].wButton[TAIL])
				A->W[G].wButton[TAIL]=wButton;
			wButton->Chain=cButton->Chain;
			switch(cButton->Type)
			{
				case TEXT:
					if(cButton->Resource.Text != NULL)
						free(cButton->Resource.Text);
					break;
				case DECORATION:
				case SCROLLING:
				case ICON:
					break;
			}
			free(cButton);
			break;
		}
		else {
			wButton=cButton;
			cButton=cButton->Chain;
		}
}

void	DestroyAllButtons(uARG *A, int G)
{
	WBUTTON *wButton=A->W[G].wButton[HEAD], *cButton=NULL;
	while(wButton != NULL) {
		cButton=wButton->Chain;
		switch(wButton->Type)
		{
			case TEXT:
				if(wButton->Resource.Text != NULL)
					free(wButton->Resource.Text);
			case DECORATION:
			case SCROLLING:
			case ICON:
				break;
		}
		free(wButton);
		wButton=cButton;
	}
	A->W[G].wButton[HEAD]=A->W[G].wButton[TAIL]=NULL;
}

void	DrawAllButtons(uARG *A, int G)
{
	struct WButton *wButton=NULL;
	for(wButton=A->W[G].wButton[HEAD]; wButton != NULL; wButton=wButton->Chain)
	{
		if((wButton->WBState.Func != NULL) && (wButton->WBState.Func(A, wButton) == TRUE))
			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_FOCUS].RGB);
		else
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);

		wButton->DrawFunc(A, wButton);
	}
}

void	WidgetButtonPress(uARG *A, int G, XEvent *E)
{
	int x=0, y=0;
	switch(E->type) {
		case ButtonPress:
		{
			x=E->xbutton.x;
			y=E->xbutton.y;
		}
			break;
		case MotionNotify:
		{
			x=E->xmotion.x;
			y=E->xmotion.y;
		}
			break;
	}
	int T=G, xOffset=0, yOffset=0;
	if(_IS_MDI_) {
		for(T=LAST_WIDGET; T >= FIRST_WIDGET; T--)
			if( E->xbutton.subwindow == A->W[T].window)
			{
				xOffset=A->W[T].x;
				yOffset=A->W[T].y;
				break;
			}
	}
	// Search which button is pressed.
	WBUTTON *wButton=NULL;
	for(wButton=A->W[T].wButton[HEAD]; wButton != NULL; wButton=wButton->Chain)
		if((x > wButton->x + xOffset)
		&& (y > wButton->y + yOffset)
		&& (x < wButton->x + xOffset + wButton->w)
		&& (y < wButton->y + yOffset + wButton->h))
		{
			// Set the flashing color.
			XSetForeground(A->display, A->W[T].gc, A->L.Colors[COLOR_PULSE].RGB);
			// Draw the button through its callback function.
			wButton->DrawFunc(A, wButton);
			fDraw(T, FALSE, TRUE);
			// Sleep to visualy flash the user.
			usleep(WBUTTON_PULSE_US);
			// Execute its callback.
			wButton->CallBack(A, wButton);
			// Set the color back to the button state.
			XSetForeground(A->display, A->W[T].gc, A->W[T].foreground);
			if(wButton->WBState.Func != NULL)
			{
				if(wButton->WBState.Func(A, wButton) == TRUE)
					XSetForeground(A->display, A->W[T].gc, A->L.Colors[COLOR_FOCUS].RGB);
			}
			// Draw the button again.
			wButton->DrawFunc(A, wButton);
			fDraw(T, FALSE, TRUE);

			break;
		}
}

// Draw the shape for a window icon.
void	DrawIconWindow(Display *display, Drawable drawable, GC gc, XRectangle *rect, unsigned long int bg, unsigned long int fg)
{
	XSetForeground(display, gc, bg);
	XFillRectangle(display, drawable, gc, 0, 0, rect->width, rect->height);
	XSetForeground(display, gc, fg);

	const short int h=rect->height / 5;
	XSegment shape[]=
	{
		{.x1=rect->x + 2,	.y1=rect->y + (h * 1),	.x2=rect->x + (3 * rect->width / 4),	.y2=rect->y + (h * 1)},
		{.x1=rect->x + 2,	.y1=rect->y + (h * 2),	.x2=rect->x + (1 * rect->width / 2),	.y2=rect->y + (h * 2)},
		{.x1=rect->x + 2,	.y1=rect->y + (h * 3),	.x2=rect->x + rect->width - 4,		.y2=rect->y + (h * 3)},
		{.x1=rect->x + 2,	.y1=rect->y + (h * 4),	.x2=rect->x + (2 * rect->width / 5),	.y2=rect->y + (h * 4)}
	};
	XDrawSegments(display, drawable, gc, shape, sizeof(shape)/sizeof(XSegment));
}

// All-in-One function to print a string filled with some New Line terminated texts.
MaxText XPrint(Display *display, Drawable drawable, GC gc, int x, int y, char *NewLineStr, int spacing)
{
	char *pStartLine=NewLineStr, *pNewLine=NULL;
	MaxText  Text={0,0};
	while((pNewLine=strchr(pStartLine,'\n')) != NULL)
	{
		int cols=pNewLine - pStartLine;
		XDrawString(	display, drawable, gc,
				x,
				y + (spacing * Text.rows),
				pStartLine, cols);
		Text.cols=MAX(cols, Text.cols);
		Text.rows++ ;
		pStartLine=pNewLine+1;
	}
	return(Text);
}

// Scale the MDI window = MAIN Widget.
void	ScaleMDI(uARG *A)
{
	int G=FIRST_WIDGET, RightMost=LAST_WIDGET, BottomMost=LAST_WIDGET;
	while(G <= LAST_WIDGET)
	{
		if((A->W[G].x + A->W[G].width)  > (A->W[RightMost].x + A->W[RightMost].width))
			RightMost=G;
		if((A->W[G].y + A->W[G].height) > (A->W[BottomMost].y + A->W[BottomMost].height))
			BottomMost=G;
		G++ ;
	}
	A->W[MAIN].width=A->W[RightMost].x + A->W[RightMost].width + DEFAULT_FONT_CHAR_WIDTH;
	A->W[MAIN].height=A->W[BottomMost].y + A->W[BottomMost].height + DEFAULT_FONT_CHAR_HEIGHT;
	// Adjust the Header & Footer axes with the new width.
	int i=0;
	for(i=0; i < A->L.Axes[MAIN].N; i++)
		A->L.Axes[MAIN].Segment[i].x2=A->W[MAIN].width;
	// Adjust scrolling width.
	A->L.Page[MAIN].Geometry.cols=A->W[MAIN].width / One_Char_Width(MAIN);
	SetHViewport(MAIN, A->L.Page[MAIN].Geometry.cols - 3);
	SetVFrame(MAIN, GetVViewport(MAIN) << MAIN_FRAME_VIEW_VSHIFT);
}

// ReSize a Widget window & inform WM.
void	ReSizeMoveWidget(uARG *A, int G)
{
	XSizeHints *hints=NULL;
	if((hints=XAllocSizeHints()) != NULL)
	{
		hints->x=A->W[G].x;
		hints->y=A->W[G].y;
		hints->min_width= hints->max_width= A->W[G].width;
		hints->min_height=hints->max_height=A->W[G].height;
		hints->flags=USPosition|USSize|PMinSize|PMaxSize;
		XSetWMNormalHints(A->display, A->W[G].window, hints);
		XFree(hints);
	}
	XWindowAttributes xwa={0};
	XGetWindowAttributes(A->display, A->W[G].window, &xwa);

	// Did the Window Manager really move and size the Window ?
	if((xwa.width != A->W[G].width) || (xwa.height != A->W[G].height) || (xwa.x != A->W[G].x) || (xwa.y != A->W[G].y))
		XMoveResizeWindow(A->display, A->W[G].window, A->W[G].x, A->W[G].y, A->W[G].width, A->W[G].height);
}

// Move a Widget Window (when the MDI mode is enabled).
void	MoveWidget(uARG *A, XEvent *E)
{
	switch(E->type) {
		case ButtonPress:
			if(!A->T.Locked) {
				// Which Widget is the target ?
				if(_IS_MDI_ && E->xbutton.subwindow) {
					for(A->T.S=LAST_WIDGET; A->T.S >= FIRST_WIDGET; A->T.S--)
						if( E->xbutton.subwindow == A->W[A->T.S].window) {
							A->T.Locked=TRUE;
							break;
						}
				} else {
					for(A->T.S=LAST_WIDGET; A->T.S >= MAIN; A->T.S--)
						if( E->xbutton.window == A->W[A->T.S].window) {
							A->T.Locked=TRUE;
							break;
						}
				}
				if(A->T.Locked) {
					// A Widget is locked then store delta between the mouse position and its window origin.
					if(_IS_MDI_ && (A->T.S != MAIN)) {
						A->T.dx=E->xbutton.x - A->W[A->T.S].x;
						A->T.dy=E->xbutton.y - A->W[A->T.S].y;
					} else {
						A->T.dx=E->xbutton.x_root - A->W[A->T.S].x;
						A->T.dy=E->xbutton.y_root - A->W[A->T.S].y;
					}
					XRaiseWindow(A->display, A->W[A->T.S].window);
					XDefineCursor(A->display, A->W[A->T.S].window, A->MouseCursor[MC_MOVE]);
				}
			}
			break;
		case ButtonRelease:
			if(A->T.Locked) {
				if(A->PAUSE[A->T.S])
					XDefineCursor(A->display, A->W[A->T.S].window, A->MouseCursor[MC_WAIT]);
				else
					XDefineCursor(A->display, A->W[A->T.S].window, A->MouseCursor[MC_DEFAULT]);
				A->T.S=0;
				A->T.Locked=FALSE;
			}
			break;
		case MotionNotify:
			if(A->T.Locked) {
				// Move the Widget until the button is released.
				int	tx=0,
					ty=0,
					tw=0,
					th=0;
				if(_IS_MDI_ && (A->T.S != MAIN)) {
					tx=E->xmotion.x - A->T.dx,
					ty=E->xmotion.y - A->T.dy,
					tw=A->W[MAIN].width  - (A->W[A->T.S].width + (A->W[A->T.S].border_width << 1)),
					th=A->W[MAIN].height - (A->W[A->T.S].height + (A->W[A->T.S].border_width << 1));
				}
				else {
					tx=E->xmotion.x_root - A->T.dx,
					ty=E->xmotion.y_root - A->T.dy,
					tw=WidthOfScreen(A->screen)  - (A->W[A->T.S].width + (A->W[A->T.S].border_width << 1)),
					th=HeightOfScreen(A->screen) - (A->W[A->T.S].height + (A->W[A->T.S].border_width << 1));
				}
				// Stick the widget position to the Screen borders.
				if(tx > tw)	tx=tw;
				if(tx < 0)	tx=0;
				if(ty > th)	ty=th;
				if(ty < 0)	ty=0;
				XMoveWindow(A->display, A->W[A->T.S].window, tx, ty);
			}
			break;
	}
}

void	IconifyWidget(uARG *A, int G, XEvent *E)
{
	switch(E->type)
	{
		case UnmapNotify:
		{	// When a Widget is iconified, minimized or unmapped, create an associated button in the MAIN window.
			RESOURCE Resource[WIDGETS]=ICON_LABELS;

			int vSpacing=Header_Height(MAIN) + (G * One_Half_Char_Height(MAIN));

			CreateButton(	A, ICON, (ID_NULL + G), G,
					A->W[MAIN].width - (Twice_Char_Width(MAIN) - Quarter_Char_Width(MAIN)),
					vSpacing,
					One_Half_Char_Width(MAIN),
					One_Char_Height(MAIN),
					CallBackRestoreWidget,
					&Resource[G],
					NULL);
		}
			break;
		case MapNotify:
		{	// Remove the button when the Widget is restored on screen.
			DestroyButton(A, MAIN, (ID_NULL + G));
		}
			break;
	}
}

void	MinimizeWidget(uARG *A, int G)
{
	if(_IS_MDI_) {
		if(G != MAIN)
			XUnmapWindow(A->display, A->W[G].window);
	}
	else
		XIconifyWindow(A->display, A->W[G].window, DefaultScreen(A->display));
}

void	RestoreWidget(uARG *A, int G)
{
	XMapWindow(A->display, A->W[G].window);
}

void	SetWidgetName(uARG *A, int G, char *name)
{
	XStoreName(A->display, A->W[G].window, name);
	XSetIconName(A->display, A->W[G].window, name);
}

static void *uSplash(void *uArg)
{
	uARG *A=(uARG *) uArg;
	XEvent	E={0};
	while(A->LOOP) {
		XNextEvent(A->display, &E);
		switch(E.type)
		{
			case ClientMessage:
				if(E.xclient.send_event)
					A->LOOP=FALSE;
				break;
			case Expose:
				if(!E.xexpose.count)
				{
					XSetForeground(A->display, A->Splash.gc, _FOREGROUND_SPLASH);
					XCopyPlane(	A->display,
							A->Splash.bitmap,
							A->Splash.window,
							A->Splash.gc,
							0,
							0,
							splash_width,
							splash_height,
							(A->Splash.w - splash_width) >> 1,
							(A->Splash.h - splash_height) >> 1,
							1);
				}
				break;
		}
	}
	return(NULL);
}

void	StartSplash(uARG *A)
{
	A->Splash.x=(WidthOfScreen(A->screen) >> 1)  - (A->Splash.w >> 1);
	A->Splash.y=(HeightOfScreen(A->screen) >> 1) - (A->Splash.h >> 1);
	if((A->Splash.window=XCreateSimpleWindow(A->display,
						DefaultRootWindow(A->display),
						A->Splash.x,
						A->Splash.y,
						A->Splash.w,
						A->Splash.h,
						0,
						_BACKGROUND_SPLASH,
						_BACKGROUND_SPLASH)) != 0
	&& (A->Splash.gc=XCreateGC(A->display, A->Splash.window, 0, NULL)) != 0)
	{
		XClassHint *clHints=NULL;
		if((clHints=XAllocClassHint()) != NULL)
		{
			clHints->res_name="Splash";
			clHints->res_class=_APPNAME;
			XSetClassHint(A->display, A->Splash.window, clHints);
			XFree(clHints);
		}
		XStoreName(A->display, A->Splash.window, _APPNAME);
		XSetIconName(A->display, A->Splash.window, _APPNAME);

		A->Splash.bitmap=XCreateBitmapFromData(A->display, A->Splash.window, (const char *) splash_bits, splash_width, splash_height);

		XSizeHints *szHints=NULL;
		if((szHints=XAllocSizeHints()) != NULL)
		{
			szHints->x=A->Splash.x;
			szHints->y=A->Splash.y;
			szHints->min_width= szHints->max_width= A->Splash.w;
			szHints->min_height=szHints->max_height=A->Splash.h;
			szHints->flags=USPosition|USSize|PMinSize|PMaxSize;
			XSetWMNormalHints(A->display, A->Splash.window, szHints);
			XFree(szHints);
		}
		else
			XMoveWindow(A->display, A->Splash.window, A->Splash.x, A->Splash.y);

		Atom property=XInternAtom(A->display, "_MOTIF_WM_HINTS", True);
		if(property != None)
		{
			struct {
				unsigned long int	flags,
							functions,
							decorations;
				long int		inputMode;
				unsigned long int	status;
				}
					data={.flags=2, .functions=0, .decorations=0, .inputMode=0, .status=0};

			XChangeProperty(A->display, A->Splash.window, property, property, 32, PropModeReplace, (unsigned char *) &data, 5);
		}

		XRectangle iconRect={.x=0, .y=0, .width=48, .height=48};
		XIconSize *xics=NULL;
		int xicn=0;
		if(XGetIconSizes(A->display, XRootWindow(A->display, DefaultScreen(A->display)), &xics, &xicn) != 0)
		{
			if(xicn > 0)
			{
				iconRect.width=xics[0].max_width;
				iconRect.height=xics[0].max_height;
			}
			XFree(xics);
		}
		if((A->Splash.icon=XCreatePixmap(A->display, A->Splash.window, iconRect.width, iconRect.height, DefaultDepthOfScreen(A->screen))))
		{
			DrawIconWindow(A->display, A->Splash.icon, A->Splash.gc, &iconRect, _BACKGROUND_SPLASH, _FOREGROUND_SPLASH);

			XWMHints *wmhints=XAllocWMHints();
			if(wmhints != NULL)
			{
				wmhints->initial_state = NormalState;
				wmhints->input = True;
				wmhints->icon_pixmap = A->Splash.icon;
				wmhints->flags = StateHint | IconPixmapHint | InputHint;
				XSetWMHints(A->display, A->Splash.window, wmhints);
				XFree(wmhints);
			}
		}
		XSelectInput(A->display, A->Splash.window, ExposureMask);
		XMapWindow(A->display, A->Splash.window);

		pthread_create(&A->TID_Draw, NULL, uSplash, A);
	}
}

void	StopSplash(uARG *A)
{
	XClientMessageEvent E={.type=ClientMessage, .window=A->Splash.window, .format=32};
	XSendEvent(A->display, A->Splash.window, 0, 0, (XEvent *)&E);
	XFlush(A->display);

	pthread_join(A->TID_Draw, NULL);

	XUnmapWindow(A->display, A->Splash.window);
	XFreePixmap(A->display, A->Splash.icon);
	XFreePixmap(A->display, A->Splash.bitmap);
	XFreeGC(A->display, A->Splash.gc);
	XDestroyWindow(A->display, A->Splash.window);

	A->TID_Draw=0;
	A->LOOP=TRUE;
}

// Release the X-Window resources.
void	CloseDisplay(uARG *A)
{
	int MC=MC_COUNT;
	do {
		MC-- ;
		if(A->MouseCursor[MC])
		{
			XFreeCursor(A->display, A->MouseCursor[MC]);
			A->MouseCursor[MC]=0;
		}
	} while(MC);

	if(A->xfont)
	{
		XFreeFont(A->display, A->xfont);
		A->xfont=NULL;
	}
	if(A->display)
		XCloseDisplay(A->display);
}

// Initialize a new X-Window display.
int	OpenDisplay(uARG *A)
{
	int noerr=TRUE;
	if((A->display=XOpenDisplay(NULL)) && (A->screen=DefaultScreenOfDisplay(A->display)) )
	{
		switch(A->xACL)
		{
			case 'Y':
			case 'y':
				XEnableAccessControl(A->display);
				break;
			case 'N':
			case 'n':
				XDisableAccessControl(A->display);
				break;
		}

		// Try to load the requested font.
		if(strlen(A->fontName) == 0)
			strcpy(A->fontName, "Fixed");

		if((A->xfont=XLoadQueryFont(A->display, A->fontName)) == NULL)
			noerr=FALSE;
		if(noerr)
		{
			A->MouseCursor[MC_DEFAULT]=XCreateFontCursor(A->display, XC_left_ptr);
			A->MouseCursor[MC_MOVE]=XCreateFontCursor(A->display, XC_fleur);
			A->MouseCursor[MC_WAIT]=XCreateFontCursor(A->display, XC_watch);
		}
	}
	else	noerr=FALSE;
	return(noerr);
}

// Release the Widget resources.
void	CloseWidgets(uARG *A)
{
	if(A->L.Output)
	{
		free(A->L.Output);
		A->L.Output=NULL;
	}
	int G=0;
	for(G=LAST_WIDGET; G >= MAIN ; G--)
	{
		XFreePixmap(A->display, A->W[G].pixmap.B);
		XFreePixmap(A->display, A->W[G].pixmap.F);
		XFreePixmap(A->display, A->W[G].pixmap.I);
		if(A->L.Page[G].Pageable)
			XFreePixmap(A->display, A->L.Page[G].Pixmap);
		XFreeGC(A->display, A->W[G].gc);
		XDestroyWindow(A->display, A->W[G].window);
		free(A->L.Axes[G].Segment);
		DestroyAllButtons(A, G);
	}
	free(A->L.WB.String);
	free(A->L.Usage.C0);
	free(A->L.Usage.C1);
	free(A->L.Usage.C3);
	free(A->L.Usage.C6);
	free(A->L.Usage.C7);
	free(A->L.Play.showTemps);

	int cpu=0;
	for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
		free(A->L.Temps[cpu].Segment);
	free(A->L.Temps);
}

void	GeometriesToLayout(uARG *A)
{
	if((A->Geometries != NULL) && (strlen(A->Geometries) > 0))
	{
		char *pGeometry=A->Geometries;
		int G=0, n=0, c=0, r=0, x=0, y=0,
			ws=(!_IS_MDI_ ? WidthOfScreen(A->screen)  : A->W[MAIN].width), hs=(!_IS_MDI_ ? HeightOfScreen(A->screen) : A->W[MAIN].height);

		while(pGeometry != NULL)
			if(strlen(pGeometry) > 0)
			{
				sscanf(pGeometry, GEOMETRY_PARSER, &G, &c, &r, &x, &y, &n);

				if((G >= MAIN) && (G <= LAST_WIDGET))
				{
					A->L.Page[G].Geometry.cols=(c > 0) ? c : A->L.Page[G].Geometry.cols;
					A->L.Page[G].Geometry.rows=(r > 0) ? r : A->L.Page[G].Geometry.rows;
					A->W[G].x=(x < 0) ? ws + x : x;
					A->W[G].y=(y < 0) ? hs + y : y;
				}
				pGeometry=(n > 0) ? pGeometry + n : NULL;
			}
			else	break;
	}
}

// Create the Widgets.
int	OpenWidgets(uARG *A)
{
	int noerr=TRUE;
	char *str=calloc(sizeof(DEFAULT_HEADER_STR), 1);

	// Allocate memory for chart elements.
	A->L.Usage.C0=calloc(A->SHM->P.CPU, sizeof(XRectangle));
	A->L.Usage.C1=calloc(A->SHM->P.CPU, sizeof(XRectangle));
	A->L.Usage.C3=calloc(A->SHM->P.CPU, sizeof(XRectangle));
	A->L.Usage.C6=calloc(A->SHM->P.CPU, sizeof(XRectangle));
	A->L.Usage.C7=calloc(A->SHM->P.CPU, sizeof(XRectangle));

	XRectangle iconRect={.x=0, .y=0, .width=48, .height=48};
	XIconSize *xics=NULL;
	int xicn=0;
	if(XGetIconSizes(A->display, XRootWindow(A->display, DefaultScreen(A->display)), &xics, &xicn) != 0)
	{
		if(xicn > 0)
		{
			iconRect.width=xics[0].max_width;
			iconRect.height=xics[0].max_height;
		}
		XFree(xics);
	}

	GeometriesToLayout(A);

	XSetWindowAttributes swa={
	/* Pixmap: background, None, or ParentRelative	*/	.background_pixmap=None,
	/* unsigned long: background pixel		*/	.background_pixel=A->L.globalBackground,
	/* Pixmap: border of the window or CopyFromParent */	.border_pixmap=CopyFromParent,
	/* unsigned long: border pixel value */			.border_pixel=A->L.globalForeground,
	/* int: one of bit gravity values */			.bit_gravity=0,
	/* int: one of the window gravity values */		.win_gravity=0,
	/* int: NotUseful, WhenMapped, Always */		.backing_store=DoesBackingStore(DefaultScreenOfDisplay(A->display)),
	/* unsigned long: planes to be preserved if possible */	.backing_planes=AllPlanes,
	/* unsigned long: value to use in restoring planes */	.backing_pixel=0,
	/* Bool: should bits under be saved? (popups) */	.save_under=DoesSaveUnders(DefaultScreenOfDisplay(A->display)),
	/* long: set of events that should be saved */		.event_mask=EventMaskOfScreen(DefaultScreenOfDisplay(A->display)),
	/* long: set of events that should not propagate */	.do_not_propagate_mask=0,
	/* Bool: boolean value for override_redirect */		.override_redirect=False,
	/* Colormap: color map to be associated with window */	.colormap=DefaultColormap(A->display, DefaultScreen(A->display)),
	/* Cursor: Cursor to be displayed (or None) */		.cursor=A->MouseCursor[MC_DEFAULT]};

	int G=0;
	for(G=MAIN; noerr && (G < WIDGETS); G++)
	{
		// Create the Widgets.
		if((A->W[G].window=XCreateWindow(A->display,
						_IS_MDI_ && (G != MAIN) ?
						A->W[MAIN].window
						: DefaultRootWindow(A->display),
						A->W[G].x, A->W[G].y, A->W[G].width, A->W[G].height, A->W[G].border_width,
						CopyFromParent, InputOutput, CopyFromParent, CWBorderPixel|CWOverrideRedirect|CWCursor, &swa)) )
		{
			sprintf(str, TITLE_MAIN_FMT, _MAJOR, _MINOR, _NIGHTLY);

			XClassHint *hints=NULL;
			if((hints=XAllocClassHint()) != NULL)
			{
				hints->res_name=str;
				hints->res_class=_APPNAME;
				XSetClassHint(A->display, A->W[G].window, hints);
				XFree(hints);
			}
			SetWidgetName(A, G, str);

			if((A->W[G].gc=XCreateGC(A->display, A->W[G].window, 0, NULL)))
			{
				XSetFont(A->display, A->W[G].gc, A->xfont->fid);

				switch(G)
				{
					// Compute Widgets scaling.
					case MAIN:
					{
						SetHViewport(G, MAIN_TEXT_WIDTH  - 3);
						SetVViewport(G, MAIN_TEXT_HEIGHT - 2);
						SetHFrame(G, GetHViewport(G) << MAIN_FRAME_VIEW_HSHIFT);
						SetVFrame(G, GetVViewport(G) << MAIN_FRAME_VIEW_VSHIFT);

						XTextExtents(	A->xfont, DEFAULT_HEADER_STR, MAIN_TEXT_WIDTH,
								&A->W[G].extents.dir, &A->W[G].extents.ascent,
								&A->W[G].extents.descent, &A->W[G].extents.overall);

						A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
									- A->xfont->min_bounds.lbearing;
						A->W[G].extents.charHeight=A->W[G].extents.ascent
									+ A->W[G].extents.descent;

						A->W[G].width	= A->W[G].extents.overall.width;
						A->W[G].height	= One_Char_Height(G) * MAIN_TEXT_HEIGHT;

						// Prepare the chart axes.
						A->L.Axes[G].N=2;
						A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
						// Header.
						A->L.Axes[G].Segment[0].x1=0;
						A->L.Axes[G].Segment[0].y1=Header_Height(G) + 1;
						A->L.Axes[G].Segment[0].x2=A->W[G].width;
						A->L.Axes[G].Segment[0].y2=A->L.Axes[G].Segment[0].y1;
						// Footer.
						A->L.Axes[G].Segment[1].x1=A->L.Axes[G].Segment[0].x1;
						A->L.Axes[G].Segment[1].y1=A->W[G].height - 1;
						A->L.Axes[G].Segment[1].x2=A->L.Axes[G].Segment[0].x2;
						A->L.Axes[G].Segment[1].y2=A->L.Axes[G].Segment[1].y1;
						A->W[G].height+=Footer_Height(G);

						// First run if MAIN is not defined as MDI then create buttons.
						if(!_IS_MDI_)
						{
							unsigned int square=MAX(One_Char_Height(G), One_Char_Width(G));

							CreateButton(	A, DECORATION, ID_SAVE, G,
									A->W[G].width - (One_Char_Width(G) * 8) - 2,
									2,
									square - 2,
									square - 2,
									CallBackSave,
									NULL,
									NULL);

							CreateButton(	A, DECORATION, ID_QUIT, G,
									A->W[G].width - (One_Char_Width(G) * 5) - 2,
									2,
									square - 2,
									square - 2,
									CallBackQuit,
									NULL,
									NULL);

							CreateButton(	A, DECORATION, ID_MIN, G,
									A->W[G].width - Twice_Char_Width(G) - 2,
									2,
									square - 2,
									square - 2,
									CallBackMinimizeWidget,
									NULL,
									NULL);

							CreateButton(	A, SCROLLING, ID_NORTH, G,
									A->W[G].width - square,
									Header_Height(G) + 2,
									square,
									square,
									CallBackButton,
									NULL,
									NULL);

							CreateButton(	A, SCROLLING, ID_SOUTH, G,
									A->W[G].width - square,
									A->W[G].height - (Footer_Height(G) + square + 2),
									square,
									square,
									CallBackButton,
									NULL,
									NULL);

							CreateButton(	A, SCROLLING, ID_EAST, G,
									A->W[G].width
									- (MAX(Twice_Char_Height(G),Twice_Char_Width(G)) + 2),
									A->W[G].height - (square + 2),
									square,
									square,
									CallBackButton,
									NULL,
									NULL);

							CreateButton(	A, SCROLLING, ID_WEST, G,
									A->W[G].width
									- (MAX( 3 * One_Char_Height(G)+Quarter_Char_Height(G),
										3 * One_Char_Width(G)+Quarter_Char_Width(G)) + 2),
									A->W[G].height - (square + 2),
									square,
									square,
									CallBackButton,
									NULL,
									NULL);
						}
					}
						break;
					case CORES:
					{
						SetHViewport(G, CORES_TEXT_WIDTH);
						SetVViewport(G, CORES_TEXT_HEIGHT);
						SetHFrame(G, GetHViewport(G));
						SetVFrame(G, GetVViewport(G));

						XTextExtents(	A->xfont, DEFAULT_HEADER_STR, CORES_TEXT_WIDTH << 1,
								&A->W[G].extents.dir, &A->W[G].extents.ascent,
								&A->W[G].extents.descent, &A->W[G].extents.overall);

						A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
									- A->xfont->min_bounds.lbearing;
						A->W[G].extents.charHeight=A->W[G].extents.ascent
										+ A->W[G].extents.descent;

						A->W[G].width	= A->W[G].extents.overall.width
								+ (One_Char_Width(G) << 2)
								+ Half_Char_Width(G);
						A->W[G].height	= Quarter_Char_Height(G)
								+ One_Char_Height(G) * (CORES_TEXT_HEIGHT + 2);

						// Prepare the chart axes.
						A->L.Axes[G].N=CORES_TEXT_WIDTH + 2;
						// A->L.Axes[G].N=CORES_TEXT_WIDTH + 1; /* Without a footer (plus 1 to malloc size) */
						A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
						int	i=0,
							j=A->W[G].extents.overall.width / CORES_TEXT_WIDTH;
						for(i=0; i <= CORES_TEXT_WIDTH; i++)
						{
							A->L.Axes[G].Segment[i].x1=(j * i) + (One_Char_Width(G) * 3);
							A->L.Axes[G].Segment[i].y1=3 + One_Char_Height(G);
							A->L.Axes[G].Segment[i].x2=A->L.Axes[G].Segment[i].x1;
							A->L.Axes[G].Segment[i].y2=((CORES_TEXT_HEIGHT + 1) * One_Char_Height(G)) - 3;
						}
						// With a footer.
						A->L.Axes[G].Segment[i].x1=0;
						A->L.Axes[G].Segment[i].y1=A->W[G].height - 1;
						A->L.Axes[G].Segment[i].x2=A->W[G].width;
						A->L.Axes[G].Segment[i].y2=A->L.Axes[G].Segment[i].y1;
						A->W[G].height+=Footer_Height(G);

						unsigned int square=MAX(One_Char_Height(G), One_Char_Width(G));

						WBSTATE WBState={Button_State, &A->L.Play.fillGraphics};
						CreateButton(	A, DECORATION, ID_CHART, G,
								A->W[G].width - (One_Char_Width(G) * 5) - 2,
								2,
								square - 2,
								square - 2,
								CallBackButton,
								NULL,
								&WBState);

						CreateButton(	A, DECORATION, ID_MIN, G,
								A->W[G].width - Twice_Char_Width(G) - 2,
								2,
								square - 2,
								square - 2,
								CallBackMinimizeWidget,
								NULL,
								NULL);

						struct {
							char		ID;
							RESOURCE	RSC;
							WBSTATE		WBState;
						} Loader[]={	{.ID=ID_PAUSE, .RSC={.Text=RSC_PAUSE}, {Button_State, &A->PAUSE[G]}},
								{.ID=ID_FREQ , .RSC={.Text=RSC_FREQ},  {Button_State, &A->L.Play.freqHertz}},
								{.ID=ID_CYCLE, .RSC={.Text=RSC_CYCLE}, {Button_State, &A->L.Play.showCycles}},
								{.ID=ID_IPS, .  RSC={.Text=RSC_IPS}, {Button_State, &A->L.Play.showIPS}},
								{.ID=ID_IPC, .  RSC={.Text=RSC_IPC}, {Button_State, &A->L.Play.showIPC}},
								{.ID=ID_CPI, .  RSC={.Text=RSC_CPI}, {Button_State, &A->L.Play.showCPI}},
								{.ID=ID_RATIO, .RSC={.Text=RSC_RATIO}, {Button_State, &A->L.Play.showRatios}},
								{.ID=ID_SCHED, .RSC={.Text=RSC_SCHED}, {Button_State, &A->SHM->S.Monitor}},
								{.ID=ID_NULL , .RSC={.Text=NULL},      {NULL, NULL}}
							};
						int spacing=MAX(One_Char_Height(G), One_Char_Width(G)) + 2 + 2;
						for(i=0; Loader[i].ID != ID_NULL; i++, spacing+=One_Char_Width(G) * (2 + strlen(Loader[i-1].RSC.Text)))
							CreateButton(	A, TEXT, Loader[i].ID, G,
									spacing,
									A->W[G].height - Footer_Height(G) + 2,
									One_Char_Width(G) * (1 + strlen(Loader[i].RSC.Text)),
									One_Char_Height(G),
									CallBackButton,
									&Loader[i].RSC,
									&Loader[i].WBState);
					}
						break;
					case CSTATES:
					{
						SetHViewport(G, CSTATES_TEXT_WIDTH);
						SetVViewport(G, CSTATES_TEXT_HEIGHT);
						SetHFrame(G, GetHViewport(G));
						SetVFrame(G, GetVViewport(G));

						XTextExtents(	A->xfont, DEFAULT_HEADER_STR, CSTATES_TEXT_WIDTH,
								&A->W[G].extents.dir, &A->W[G].extents.ascent,
								&A->W[G].extents.descent, &A->W[G].extents.overall);

						A->W[G].extents.charWidth	= A->xfont->max_bounds.rbearing
										- A->xfont->min_bounds.lbearing;
						A->W[G].extents.charHeight	= A->W[G].extents.ascent
										+ A->W[G].extents.descent;

						A->W[G].width	= Quarter_Char_Width(G)
								+ Twice_Half_Char_Width(G)
								+ (CSTATES_TEXT_WIDTH * One_Half_Char_Width(G));
						A->W[G].height	= Twice_Half_Char_Height(G)
								+ (One_Char_Height(G) * CSTATES_TEXT_HEIGHT);

						// Prepare the chart axes.
						A->L.Axes[G].N=CSTATES_TEXT_HEIGHT + 2;
						// A->L.Axes[G].N=CSTATES_TEXT_HEIGHT + 1; /* Without a footer */
						A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
						int i=0;
						for(i=0; i < CSTATES_TEXT_HEIGHT; i++)
						{
							A->L.Axes[G].Segment[i].x1=0;
							A->L.Axes[G].Segment[i].y1=(i + 1) * One_Char_Height(G);
							A->L.Axes[G].Segment[i].x2=A->W[G].width;
							A->L.Axes[G].Segment[i].y2=A->L.Axes[G].Segment[i].y1;
						}
						// Percent.
						A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x1=A->W[G].width
											- One_Char_Width(G) - Quarter_Char_Width(G);
						A->L.Axes[G].Segment[A->L.Axes[G].N - 2].y1=One_Char_Height(G);
						A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x2=A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x1;
						A->L.Axes[G].Segment[A->L.Axes[G].N - 2].y2=One_Char_Height(G)
											+ (One_Char_Height(G) * CSTATES_TEXT_HEIGHT);
						// With a footer.
						A->L.Axes[G].Segment[A->L.Axes[G].N - 1].x1=0;
						A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y1=A->W[G].height - 1;
						A->L.Axes[G].Segment[A->L.Axes[G].N - 1].x2=A->W[G].width;
						A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y2=A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y1;
						A->W[G].height+=Footer_Height(G);

						unsigned int square=MAX(One_Char_Height(G), One_Char_Width(G));

						CreateButton(	A, DECORATION, ID_MIN, G,
								A->W[G].width - Twice_Char_Width(G) - 2,
								2,
								square - 2,
								square - 2,
								CallBackMinimizeWidget,
								NULL,
								NULL);

						struct {
							char		ID;
							RESOURCE	RSC;
							WBSTATE		WBState;
						} Loader[]={	{.ID=ID_PAUSE,  .RSC={.Text=RSC_PAUSE},  {Button_State, &A->PAUSE[G]}},
								{.ID=ID_CSTATE, .RSC={.Text=RSC_CSTATE}, {Button_State, &A->L.Play.cStatePercent}},
								{.ID=ID_NULL ,  .RSC={.Text=NULL},       {NULL, NULL}}
							};
						int spacing=MAX(One_Char_Height(G), One_Char_Width(G)) + 2 + 2;
						for(i=0; Loader[i].ID != ID_NULL; i++, spacing+=One_Char_Width(G) * (2 + strlen(Loader[i-1].RSC.Text)))
							CreateButton(	A, TEXT, Loader[i].ID, G,
									spacing,
									A->W[G].height - Footer_Height(G) + 2,
									One_Char_Width(G) * (1 + strlen(Loader[i].RSC.Text)),
									One_Char_Height(G),
									CallBackButton,
									&Loader[i].RSC,
									&Loader[i].WBState);
					}
						break;
					case TEMPS:
					{
						int i=0;
						SetHViewport(G, TEMPS_TEXT_WIDTH);
						SetVViewport(G, TEMPS_TEXT_HEIGHT);
						SetHFrame(G, GetHViewport(G));
						SetVFrame(G, GetVViewport(G));

						XTextExtents(	A->xfont, DEFAULT_HEADER_STR, TEMPS_TEXT_WIDTH,
								&A->W[G].extents.dir, &A->W[G].extents.ascent,
								&A->W[G].extents.descent, &A->W[G].extents.overall);

						A->W[G].extents.charWidth	= A->xfont->max_bounds.rbearing
										- A->xfont->min_bounds.lbearing;
						A->W[G].extents.charHeight	= A->W[G].extents.ascent
										+ A->W[G].extents.descent;

						A->W[G].width	= A->W[G].extents.overall.width
								+ (One_Char_Width(G) * 6);
						A->W[G].height	=  Quarter_Char_Height(G)
								+ One_Char_Height(G) * (TEMPS_TEXT_HEIGHT + 1 + 1);

						unsigned int square=MAX(One_Char_Height(G), One_Char_Width(G));

						A->L.Play.showTemps=calloc(A->SHM->P.CPU, sizeof(Bool32));
						A->L.Temps=calloc(A->SHM->P.CPU, sizeof(struct NXSEGMENT));

						WBSTATE WBState={Button_State, NULL};
						RESOURCE RSC={.Text=calloc(3, sizeof(char))};

						int cpu=0;
						for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
						{
							if(A->SHM->C[cpu].T.Thread_ID == 0)
								A->L.Play.showTemps[cpu]=TRUE;

							A->L.Temps[cpu].N=TEMPS_TEXT_WIDTH;
							A->L.Temps[cpu].Segment=calloc(A->L.Temps[cpu].N, sizeof(XSegment));

							for(i=0; i < TEMPS_TEXT_WIDTH; i++)
							{
								A->L.Temps[cpu].Segment[i].x1=Twice_Char_Width(G) + (i * One_Char_Width(G));
								A->L.Temps[cpu].Segment[i].y1=(TEMPS_TEXT_HEIGHT + 1) * One_Char_Height(G);
								A->L.Temps[cpu].Segment[i].x2=A->L.Temps[cpu].Segment[i].x1 + One_Char_Width(G);
								A->L.Temps[cpu].Segment[i].y2=A->L.Temps[cpu].Segment[i].y1;
							}
							sprintf(RSC.Text, "%2d", cpu);
							WBState.Key=&A->L.Play.showTemps[cpu];

							CreateButton(	A, TEXT, ID_TEMP, G,
									(One_Char_Width(G) * 5) + Quarter_Char_Width(G) + (cpu << 1) * Twice_Char_Width(G),
									2,
									One_Char_Width(G) * 3,
									square - 2,
									CallBackTemps,
									&RSC,
									&WBState);
						}
						free(RSC.Text);

						// Prepare the chart axes.
						A->L.Axes[G].N=3;
						A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
						// Header.
						A->L.Axes[G].Segment[0].x1=0;
						A->L.Axes[G].Segment[0].y1=Header_Height(G) + 1;
						A->L.Axes[G].Segment[0].x2=A->W[G].width;
						A->L.Axes[G].Segment[0].y2=A->L.Axes[G].Segment[0].y1;
						// Footer.
						A->L.Axes[G].Segment[1].x1=A->L.Axes[G].Segment[0].x1;
						A->L.Axes[G].Segment[1].y1=A->W[G].height - 1;
						A->L.Axes[G].Segment[1].x2=A->L.Axes[G].Segment[0].x2;
						A->L.Axes[G].Segment[1].y2=A->L.Axes[G].Segment[1].y1;
						// Temps.
						A->L.Axes[G].Segment[2].x1=Twice_Char_Width(G);
						A->L.Axes[G].Segment[2].y1=A->L.Axes[G].Segment[0].y1;
						A->L.Axes[G].Segment[2].x2=A->L.Axes[G].Segment[2].x1;
						A->L.Axes[G].Segment[2].y2=A->L.Axes[G].Segment[0].y1 + (One_Char_Height(G) * TEMPS_TEXT_HEIGHT);

						A->W[G].height+=Footer_Height(G);

						CreateButton(	A, DECORATION, ID_MIN, G,
								A->W[G].width - Twice_Char_Width(G) - 2,
								2,
								square - 2,
								square - 2,
								CallBackMinimizeWidget,
								NULL,
								NULL);

						struct {
							char		ID;
							RESOURCE	RSC;
							WBSTATE		WBState;
						} Loader[]={	{.ID=ID_PAUSE, .RSC={.Text=RSC_PAUSE}, {Button_State, &A->PAUSE[G]}},
								{.ID=ID_RESET, .RSC={.Text=RSC_RESET}, {NULL, NULL}},
								{.ID=ID_NULL , .RSC={.Text=NULL},      {NULL, NULL}}
							};
						int spacing=MAX(One_Char_Height(G), One_Char_Width(G)) + 2 + 2;
						for(i=0; Loader[i].ID != ID_NULL; i++, spacing+=One_Char_Width(G) * (2 + strlen(Loader[i-1].RSC.Text)))
							CreateButton(	A, TEXT, Loader[i].ID, G,
									spacing,
									A->W[G].height - Footer_Height(G) + 2,
									One_Char_Width(G) * (1 + strlen(Loader[i].RSC.Text)),
									One_Char_Height(G),
									CallBackButton,
									&Loader[i].RSC,
									&Loader[i].WBState);
					}
						break;
					case SYSINFO:
					{
						SetHViewport(G, SYSINFO_TEXT_WIDTH  - 3);
						SetVViewport(G, SYSINFO_TEXT_HEIGHT - 2);
						SetHFrame(G, GetHViewport(G) << SYSINFO_FRAME_VIEW_HSHIFT);
						SetVFrame(G, GetVViewport(G) << SYSINFO_FRAME_VIEW_VSHIFT);

						XTextExtents(	A->xfont, DEFAULT_HEADER_STR, SYSINFO_TEXT_WIDTH,
								&A->W[G].extents.dir, &A->W[G].extents.ascent,
								&A->W[G].extents.descent, &A->W[G].extents.overall);

						A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
									- A->xfont->min_bounds.lbearing;
						A->W[G].extents.charHeight=A->W[G].extents.ascent
									+ A->W[G].extents.descent;

						A->W[G].width	= A->W[G].extents.overall.width;
						A->W[G].height	= One_Char_Height(G) * SYSINFO_TEXT_HEIGHT;

						// Prepare the chart axes.
						A->L.Axes[G].N=2;
						A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
						// Header.
						A->L.Axes[G].Segment[0].x1=0;
						A->L.Axes[G].Segment[0].y1=Header_Height(G) + 1;
						A->L.Axes[G].Segment[0].x2=A->W[G].width;
						A->L.Axes[G].Segment[0].y2=A->L.Axes[G].Segment[0].y1;
						// Footer.
						A->L.Axes[G].Segment[1].x1=A->L.Axes[G].Segment[0].x1;
						A->L.Axes[G].Segment[1].y1=A->W[G].height - 1;
						A->L.Axes[G].Segment[1].x2=A->L.Axes[G].Segment[0].x2;
						A->L.Axes[G].Segment[1].y2=A->L.Axes[G].Segment[1].y1;
						A->W[G].height+=Footer_Height(G);

						unsigned int square=MAX(One_Char_Height(G), One_Char_Width(G));

						CreateButton(	A, DECORATION, ID_MIN, G,
								A->W[G].width - Twice_Char_Width(G) - 2,
								2,
								square - 2,
								square - 2,
								CallBackMinimizeWidget,
								NULL,
								NULL);

						if(A->L.Page[G].Pageable)
						{
						CreateButton(	A, SCROLLING, ID_NORTH, G,
								A->W[G].width - square,
								Header_Height(G) + 2,
								square,
								square,
								CallBackButton,
								NULL,
								NULL);

						CreateButton(	A, SCROLLING, ID_SOUTH, G,
								A->W[G].width - square,
								A->W[G].height - (Footer_Height(G) + square + 2),
								square,
								square,
								CallBackButton,
								NULL,
								NULL);

						CreateButton(	A, SCROLLING, ID_EAST, G,
								A->W[G].width
								- (MAX(Twice_Char_Height(G),Twice_Char_Width(G)) + 2),
								A->W[G].height - (square + 2),
								square,
								square,
								CallBackButton,
								NULL,
								NULL);

						CreateButton(	A, SCROLLING, ID_WEST, G,
								2,
								A->W[G].height - (square + 2),
								square,
								square,
								CallBackButton,
								NULL,
								NULL);
						}
						struct {
							char		ID;
							RESOURCE	RSC;
							WBSTATE		WBState;
						} Loader[]={	{.ID=ID_PAUSE,     .RSC={.Text=RSC_PAUSE},     {Button_State, &A->PAUSE[G]}},
								{.ID=ID_WALLBOARD, .RSC={.Text=RSC_WALLBOARD}, {Button_State, &A->L.Play.wallboard}},
								{.ID=ID_INCLOOP,   .RSC={.Text=RSC_INCLOOP},   {NULL, NULL}},
								{.ID=ID_TSC,       .RSC={.Text=RSC_TSC},       {TSC_State,  NULL}},
								{.ID=ID_TSC_AUX,   .RSC={.Text=RSC_TSC_AUX},   {TSC_AUX_State,  NULL}},
								{.ID=ID_BIOS,      .RSC={.Text=RSC_BIOS},      {BIOS_State, NULL}},
								{.ID=ID_SPEC,      .RSC={.Text=RSC_SPEC},      {SPEC_State, NULL}},
								{.ID=ID_ROM,       .RSC={.Text=RSC_ROM},       {ROM_State,  NULL}},
								{.ID=ID_DECLOOP,   .RSC={.Text=RSC_DECLOOP},   {NULL, NULL}},
								{.ID=ID_NULL ,     .RSC={.Text=NULL},          {NULL, NULL}}
							};
						int spacing=MAX(One_Char_Height(G), One_Char_Width(G)) + 2 + 2;
						int i=0;
						for(i=0; Loader[i].ID != ID_NULL; i++, spacing+=One_Char_Width(G) * (2 + strlen(Loader[i-1].RSC.Text)))
							CreateButton(	A, TEXT, Loader[i].ID, G,
										spacing,
										A->W[G].height - Footer_Height(G) + 2,
										One_Char_Width(G) * (1 + strlen(Loader[i].RSC.Text)),
										One_Char_Height(G),
										CallBackButton,
										&Loader[i].RSC,
										&Loader[i].WBState);

						// Prepare a Wallboard string with the Processor information.
						int padding=SYSINFO_TEXT_WIDTH - strlen(A->L.Page[G].Title) - 6;
						sprintf(str, OVERCLOCK, A->SHM->P.Features.BrandString, A->SHM->P.Boost[1] * A->SHM->P.ClockSpeed);
						A->L.WB.Length=strlen(str) + (padding << 1);
						A->L.WB.String=calloc(A->L.WB.Length, 1);
						memset(A->L.WB.String, 0x20, A->L.WB.Length);
						A->L.WB.Scroll=padding;
						A->L.WB.Length=strlen(str) + A->L.WB.Scroll;
					}
						break;
					case DUMP:
					{
						SetHViewport(G, DUMP_TEXT_WIDTH  - 3);
						SetVViewport(G, DUMP_TEXT_HEIGHT - 2);
						SetHFrame(G, GetHViewport(G) << DUMP_FRAME_VIEW_HSHIFT);
						SetVFrame(G, GetVViewport(G) << DUMP_FRAME_VIEW_VSHIFT);

						XTextExtents(	A->xfont, DEFAULT_HEADER_STR, DUMP_TEXT_WIDTH,
								&A->W[G].extents.dir, &A->W[G].extents.ascent,
								&A->W[G].extents.descent, &A->W[G].extents.overall);

						A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
									- A->xfont->min_bounds.lbearing;
						A->W[G].extents.charHeight=A->W[G].extents.ascent
									+ A->W[G].extents.descent;

						A->W[G].width	= A->W[G].extents.overall.width;
						A->W[G].height	= One_Char_Height(G) * DUMP_TEXT_HEIGHT;

						// Prepare the chart axes.
						A->L.Axes[G].N=2;
						A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
						// Header.
						A->L.Axes[G].Segment[0].x1=0;
						A->L.Axes[G].Segment[0].y1=Header_Height(G) + 1;
						A->L.Axes[G].Segment[0].x2=A->W[G].width;
						A->L.Axes[G].Segment[0].y2=A->L.Axes[G].Segment[0].y1;
						// Footer.
						A->L.Axes[G].Segment[1].x1=A->L.Axes[G].Segment[0].x1;
						A->L.Axes[G].Segment[1].y1=A->W[G].height - 1;
						A->L.Axes[G].Segment[1].x2=A->L.Axes[G].Segment[0].x2;
						A->L.Axes[G].Segment[1].y2=A->L.Axes[G].Segment[1].y1;
						A->W[G].height+=Footer_Height(G);

						unsigned int square=MAX(One_Char_Height(G), One_Char_Width(G));

						CreateButton(	A, DECORATION, ID_MIN, G,
								A->W[G].width - Twice_Char_Width(G) - 2,
								2,
								square - 2,
								square - 2,
								CallBackMinimizeWidget,
								NULL,
								NULL);

						if(A->L.Page[G].Pageable)
						{
						CreateButton(	A, SCROLLING, ID_NORTH, G,
								A->W[G].width - square,
								Header_Height(G) + 2,
								square,
								square,
								CallBackButton,
								NULL,
								NULL);

						CreateButton(	A, SCROLLING, ID_SOUTH, G,
								A->W[G].width - square,
								A->W[G].height - (Footer_Height(G) + square + 2),
								square,
								square,
								CallBackButton,
								NULL,
								NULL);

						CreateButton(	A, SCROLLING, ID_EAST, G,
								A->W[G].width
								- (MAX(Twice_Char_Height(G),Twice_Char_Width(G)) + 2),
								A->W[G].height - (square + 2),
								square,
								square,
								CallBackButton,
								NULL,
								NULL);

						CreateButton(	A, SCROLLING, ID_WEST, G,
								2,
								A->W[G].height - (square + 2),
								square,
								square,
								CallBackButton,
								NULL,
								NULL);
						}
						struct {
							char		ID;
							RESOURCE	RSC;
							WBSTATE		WBState;
						} Loader[]={	{.ID=ID_PAUSE, .RSC={.Text=RSC_PAUSE}, {Button_State, &A->PAUSE[G]}},
								{.ID=ID_NULL , .RSC={.Text=NULL},      {NULL, NULL}}
							};
						int spacing=MAX(One_Char_Height(G), One_Char_Width(G)) + 2 + 2;
						int i=0;
						for(i=0; Loader[i].ID != ID_NULL; i++, spacing+=One_Char_Width(G) * (2 + strlen(Loader[i-1].RSC.Text)))
							CreateButton(	A, TEXT, Loader[i].ID, G,
									spacing,
									A->W[G].height - Footer_Height(G) + 2,
									One_Char_Width(G) * (1 + strlen(Loader[i].RSC.Text)),
									One_Char_Height(G),
									CallBackButton,
									&Loader[i].RSC,
									&Loader[i].WBState);
					}
						break;
				}
				ReSizeMoveWidget(A, G);
			}
			else	noerr=FALSE;
		}
		else	noerr=FALSE;
	}
	if(noerr && _IS_MDI_)
	{
		ScaleMDI(A);
		ReSizeMoveWidget(A, MAIN);

		unsigned int square=MAX(A->W[MAIN].extents.charHeight, A->W[MAIN].extents.charWidth);

		CreateButton(	A, DECORATION, ID_SAVE, MAIN,
				A->W[MAIN].width - (A->W[MAIN].extents.charWidth * 8) - 2,
				2,
				square - 2,
				square - 2,
				CallBackSave,
				NULL,
				NULL);

		CreateButton(	A, DECORATION, ID_QUIT, MAIN,
				A->W[MAIN].width - (A->W[MAIN].extents.charWidth * 5) - 2,
				2,
				square - 2,
				square - 2,
				CallBackQuit,
				NULL,
				NULL);

		CreateButton(	A, DECORATION, ID_MIN, MAIN,
				A->W[MAIN].width - (A->W[MAIN].extents.charWidth << 1) - 2,
				2,
				square - 2,
				square - 2,
				CallBackMinimizeWidget,
				NULL,
				NULL);

		CreateButton(	A, SCROLLING, ID_NORTH, MAIN,
				A->W[MAIN].width - square,
				Header_Height(MAIN) + 2,
				square,
				square,
				CallBackButton,
				NULL,
				NULL);

		CreateButton(	A, SCROLLING, ID_SOUTH, MAIN,
				A->W[MAIN].width - square,
				A->L.Axes[MAIN].Segment[1].y2 - (square + 2),
				square,
				square,
				CallBackButton,
				NULL,
				NULL);

		CreateButton(	A, SCROLLING, ID_EAST, MAIN,
				A->W[MAIN].width
				- (MAX((A->W[MAIN].extents.charHeight << 1), (A->W[MAIN].extents.charWidth << 1)) + 2),
				A->L.Axes[MAIN].Segment[1].y2 + 2,
				square,
				square,
				CallBackButton,
				NULL,
				NULL);

		CreateButton(	A, SCROLLING, ID_WEST, MAIN,
				A->W[MAIN].width
				- (MAX(	3 * A->W[MAIN].extents.charHeight+(A->W[MAIN].extents.charHeight >> 2),
					3 * A->W[MAIN].extents.charWidth+(A->W[MAIN].extents.charWidth >> 2))
				+ 2),
				A->L.Axes[MAIN].Segment[1].y2 + 2,
				square,
				square,
				CallBackButton,
				NULL,
				NULL);
	}
	if(noerr)
	{
		A->atom[0]=XInternAtom(A->display, "WM_DELETE_WINDOW", False);
		A->atom[1]=XInternAtom(A->display, "_MOTIF_WM_HINTS", True);
		A->atom[2]=XInternAtom(A->display, "_NET_WM_STATE", False);
		A->atom[3]=XInternAtom(A->display, "_NET_WM_STATE_ABOVE", False);
		A->atom[4]=XInternAtom(A->display, "_NET_WM_STATE_SKIP_TASKBAR", False);
	}
	for(G=MAIN; noerr && (G < WIDGETS); G++)
		if((A->W[G].pixmap.B=XCreatePixmap(	A->display, A->W[G].window,
							A->W[G].width, A->W[G].height,
							DefaultDepthOfScreen(A->screen) ))
		&& (A->W[G].pixmap.F=XCreatePixmap(	A->display, A->W[G].window,
							A->W[G].width, A->W[G].height,
							DefaultDepthOfScreen(A->screen) ))
		&& (A->W[G].pixmap.I=XCreatePixmap(	A->display, A->W[G].window,
							iconRect.width, iconRect.height,
							DefaultDepthOfScreen(A->screen))))
		{
			DrawIconWindow(A->display, A->W[G].pixmap.I, A->W[G].gc, &iconRect, A->W[G].background, A->W[G].foreground);

			XWMHints *wmHints=XAllocWMHints();
			if(wmHints != NULL)
			{
				wmHints->initial_state = NormalState;
				wmHints->input = True;
				wmHints->icon_pixmap = A->W[G].pixmap.I;
				wmHints->flags = StateHint | IconPixmapHint | InputHint;
				XSetWMHints(A->display, A->W[G].window, wmHints);
				XFree(wmHints);
			}

			if(A->L.Page[G].Pageable)
			{
				A->L.Page[G].width	= (GetHViewport(G) * One_Char_Width(G))
							+ One_Char_Width(G);
				A->L.Page[G].height	= (GetVViewport(G) * One_Char_Height(G))
							+ Half_Char_Height(G) + Quarter_Char_Height(G)
							- 3;	// above the footer line.

				A->L.Page[G].Pixmap=XCreatePixmap(	A->display, A->W[G].window,
									GetHFrame(G) * One_Char_Width(G),
									GetVFrame(G) * One_Char_Height(G),
									DefaultDepthOfScreen(A->screen) );
				// Preset the scrolling area.
				XSetForeground(A->display, A->W[G].gc, A->W[G].background);
				XFillRectangle(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
						0,
						0,
						GetHFrame(G) * One_Char_Width(G),
						GetVFrame(G) * One_Char_Height(G));
			}
			long EventProfile=BASE_EVENTS;
			if(_IS_MDI_) {
				if(G == MAIN)
					EventProfile=EventProfile | CLICK_EVENTS | MOVE_EVENTS;
			}
			else	EventProfile=EventProfile | CLICK_EVENTS | MOVE_EVENTS;

			XSetWMProtocols(A->display, A->W[G].window, &A->atom[0], 1);

			struct {
				unsigned long int	flags,
							functions,
							decorations;
				long int		inputMode;
				unsigned long int	status;
				}
				wmProp={.flags=(1L << 1), .functions=0, .decorations=0, .inputMode=0, .status=0};

			if(A->L.Play.noDecorations && (A->atom[1] != None))
				XChangeProperty(A->display, A->W[G].window, A->atom[1], A->atom[1], 32, PropModeReplace, (unsigned char *) &wmProp, 5);
			if(A->L.Play.alwaysOnTop)
				XChangeProperty(A->display, A->W[G].window, A->atom[2], XA_ATOM, 32, PropModeReplace, (unsigned char *) &A->atom[3], 1);
			if(A->L.Play.skipTaskbar)
				XChangeProperty(A->display, A->W[G].window, A->atom[2], XA_ATOM, 32, PropModeAppend, (unsigned char *) &A->atom[4], 1);

			XSelectInput(A->display, A->W[G].window, EventProfile);
		}
		else	noerr=FALSE;

	free(str);
	return(noerr);
}

// Draw the layout background.
void	BuildLayout(uARG *A, int G)
{
	XSetBackground(A->display, A->W[G].gc, A->W[G].background);
	// Clear entirely the background.
	XSetForeground(A->display, A->W[G].gc, A->W[G].background);
	XFillRectangle(A->display, A->W[G].pixmap.B, A->W[G].gc, 0, 0, A->W[G].width, A->W[G].height);

	// Draw the axes.
	XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_AXES].RGB);
	XDrawSegments(A->display, A->W[G].pixmap.B, A->W[G].gc, A->L.Axes[G].Segment, A->L.Axes[G].N);

	// Call the buttons drawing function.
	DrawAllButtons(A, G);

	switch(G)
	{
		case MAIN:
		{
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
					One_Char_Width(G),
					One_Char_Height(G),
					A->L.Page[G].Title,
					strlen(A->L.Page[G].Title) );

			// If the MDI mode is enabled then draw a separator line.
			if(_IS_MDI_)
			{
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_MDI_SEP].RGB);
				XDrawLine(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->L.Axes[G].Segment[1].x1,
					A->L.Axes[G].Segment[1].y2 + Footer_Height(G) + 1,
					A->L.Axes[G].Segment[1].x2,
					A->L.Axes[G].Segment[1].y2 + Footer_Height(G) + 1);
			}
			if(A->L.Output)
			{
				if(A->L.Page[G].Pageable)
				{
					// Clear the scrolling area.
					XSetForeground(A->display, A->W[G].gc, A->W[G].background);
					XFillRectangle(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
							0,
							0,
							GetHFrame(G) * One_Char_Width(G),
							GetVFrame(G) * One_Char_Height(G));
					// Draw the Output log in a scrolling area.
					XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_PRINT].RGB);

					MaxText Listing=XPrint(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
								Half_Char_Width(G),
								One_Char_Height(G),
								A->L.Output,
								One_Char_Height(G));

					SetHListing(G, MAX(Listing.cols, GetHScrolling(G)));
					SetVListing(G, MAX(Listing.rows, GetVScrolling(G)));

					if(GetVListing(G) >= GetVViewport(G))
						SetVScrolling(G, GetVListing(G) - GetVViewport(G));
					SetHScrolling(G, 0);
				}
				else
				{	// Draw the Output log in a fixed height area.
					XRectangle R[]=	{ {
							.x=0,
							.y=Header_Height(G),
							.width=A->W[G].width,
							.height=(One_Char_Height(G) * MAIN_TEXT_HEIGHT) - Footer_Height(G),
							} };
 					XSetClipRectangles(A->display, A->W[G].gc, 0, 0, R, 1, Unsorted);

 					XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_PRINT].RGB);
					A->L.Page[G].Listing=XPrint(A->display, A->W[G].pixmap.B, A->W[G].gc,
								One_Char_Width(G),
								One_Char_Height(G),
								A->L.Output,
								One_Char_Height(G));

					XSetClipMask(A->display, A->W[G].gc, None);
				}
			}
		}
			break;
		case CORES:
		{
			char str[8]={0};

			// Draw the Core identifiers, the header, and the footer on the chart.
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			int cpu=0;
			for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
			{
				sprintf(str, CORE_NUM, cpu);
				XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
						Half_Char_Width(G),
						( One_Char_Height(G) * (cpu + 1 + 1) ),
						str, strlen(str) );
			}
			sprintf(str, "%02d%02d%02d", A->SHM->P.Boost[0], A->SHM->P.Boost[1], A->SHM->P.Boost[9]);

			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_LABEL].RGB);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					Quarter_Char_Width(G),
					One_Char_Height(G),
					"Core", 4);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					Quarter_Char_Width(G),
					One_Char_Height(G) * (CORES_TEXT_HEIGHT + 1 + 1),
					"Ratio", 5);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					One_Char_Width(G) * ((5 + 1) * 2),
					One_Char_Height(G) * (CORES_TEXT_HEIGHT + 1 + 1),
					"5", 1);
			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_LOW_VALUE].RGB);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					One_Char_Width(G) * ((A->SHM->P.Boost[0] + 1) * 2),
					One_Char_Height(G) * (CORES_TEXT_HEIGHT + 1 + 1),
					&str[0], 2);
			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_MED_VALUE].RGB);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					One_Char_Width(G) * ((A->SHM->P.Boost[1] + 1) * 2),
					One_Char_Height(G) * (CORES_TEXT_HEIGHT + 1 + 1),
					&str[2], 2);
			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_HIGH_VALUE].RGB);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					One_Char_Width(G) * ((A->SHM->P.Boost[9] + 1) * 2),
					One_Char_Height(G) * (CORES_TEXT_HEIGHT + 1 + 1),
					&str[4], 2);
			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_LABEL].RGB);
			if(A->L.Play.showCycles)
			{
				if(!A->L.Play.showIPS
				&& !A->L.Play.showIPC
				&& !A->L.Play.showCPI)
				{
				XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
						One_Char_Width(G) * 13,
						One_Char_Height(G),
						"INST  UCC:URC  C-ST / TSC ", 26);
				}
			}
			else
			{
				if(A->SHM->S.Monitor)
				{
					if(!A->L.Play.showRatios)
					{
					XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
							One_Char_Width(G) * 37,
							One_Char_Height(G),
							"w/PID", 5);
					}
					if(!A->L.Play.showIPS
					&& !A->L.Play.showIPC
					&& !A->L.Play.showCPI)
					{
						XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
								One_Char_Width(G) * 17,
								One_Char_Height(G),
								TASK_SECTION, sizeof(TASK_SECTION)-1);
					}
				}
				else if(!A->L.Play.showRatios
				&&	!A->L.Play.showIPS
				&&	!A->L.Play.showIPC
				&&	!A->L.Play.showCPI)
				{
					XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
							One_Char_Width(G) * 26,
							One_Char_Height(G),
							"UCC:URC", 7);
				}
			}
			if(A->L.Play.showIPS)
			{
				XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
						One_Char_Width(G) * 15,
						One_Char_Height(G),
						"IPS", 3);
			}
			if(A->L.Play.showIPC)
			{
				XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
						One_Char_Width(G) * 23,
						One_Char_Height(G),
						"IPC", 3);
			}
			if(A->L.Play.showCPI)
			{
				XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
						One_Char_Width(G) * 31,
						One_Char_Height(G),
						"CPI", 3);
			}
		}
			break;
		case CSTATES:
		{
			char str[8]={0};

			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_LABEL].RGB);
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - One_Half_Char_Width(G) - Quarter_Char_Width(G),
					One_Char_Height(G)
					+ (One_Char_Height(G) * CSTATES_TEXT_HEIGHT),
					"%", 1 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width(G) - Quarter_Char_Width(G),
					One_Char_Height(G)
					+ (10 * (One_Char_Height(G) * CSTATES_TEXT_HEIGHT)) / 100,
					"90", 2 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width(G) - Quarter_Char_Width(G),
					One_Char_Height(G)
					+ (30 * (One_Char_Height(G) * CSTATES_TEXT_HEIGHT)) / 100,
					"70", 2 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width(G) - Quarter_Char_Width(G),
					One_Char_Height(G)
					+ (50 * (One_Char_Height(G) * CSTATES_TEXT_HEIGHT)) / 100,
					"50", 2 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width(G) - Quarter_Char_Width(G),
					One_Char_Height(G)
					+ (70 * (One_Char_Height(G) * CSTATES_TEXT_HEIGHT)) / 100,
					"30", 2 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width(G) - Quarter_Char_Width(G),
					One_Char_Height(G)
					+ (90 * (One_Char_Height(G) * CSTATES_TEXT_HEIGHT)) / 100,
					"10", 2 );
			XDrawString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					One_Char_Width(G),
					One_Char_Height(G) + (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT + 1)),
					CSTATES_FOOTER, strlen(CSTATES_FOOTER) );

			// Draw the Core identifiers.
			int cpu=0;
			for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
			{
				XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
				sprintf(str, CORE_NUM, cpu);
				XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
						Twice_Char_Width(G) + ((cpu * CSTATES_TEXT_SPACING) * One_Half_Char_Width(G)),
						One_Char_Height(G),
						str, strlen(str) );

				if(A->SHM->C[cpu].T.Offline == TRUE)	// Is the Core disabled ?
				{
					XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_LABEL].RGB);
					XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
							One_Half_Char_Width(G) + ((cpu * CSTATES_TEXT_SPACING) * One_Half_Char_Width(G)),
							One_Char_Height(G)
							+ (One_Char_Height(G) * CSTATES_TEXT_HEIGHT),
							"OFF", 3 );
				}
			}
		}
			break;
		case TEMPS:
		{
			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_LABEL].RGB);
			XDrawImageString(A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					Quarter_Char_Width(G),
					One_Char_Height(G),
					"Core", 4);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					Quarter_Char_Width(G),
					One_Char_Height(G) * (TEMPS_TEXT_HEIGHT + 1 + 1),
					"Temps", 5);
		}
			break;
		case SYSINFO:
		{
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
					One_Char_Width(G),
					One_Char_Height(G),
					A->L.Page[G].Title,
					strlen(A->L.Page[G].Title) );

			char *str=calloc(256, 1);

			int padding=SYSINFO_TEXT_WIDTH - strlen(A->L.Page[G].Title) - 6;
			sprintf(str, OVERCLOCK, A->SHM->P.Features.BrandString, A->SHM->P.Boost[1] * A->SHM->P.ClockSpeed);
			memcpy(&A->L.WB.String[padding], str, strlen(str));

			sprintf(str, "Loop(%d usecs)", (IDLE_BASE_USEC * A->SHM->P.IdleTime) );
			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_LABEL].RGB);
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - (24 * One_Char_Width(G)),
					A->W[G].height - Half_Char_Height(G),
					str, strlen(str) );

			if(A->L.Page[G].Pageable)
			{
				char 		*items=calloc(8192, 1);
				#define		powered(bit) ((bit == 1) ? 'Y' : 'N')
				#define		enabled(bit) ((bit == 1) ? "ON" : "OFF")
				const char	*ClockSrcStr[SRC_COUNT]={"TSC", "BIOS", "SPEC", "ROM", "AUX"};

				sprintf(items, PROC_FORMAT,
					A->SHM->P.Features.BrandString,
					A->SHM->P.ClockSpeed,					ClockSrcStr[A->SHM->P.ClockSrc],
					A->SHM->P.Features.Std.AX.ExtFamily + A->SHM->P.Features.Std.AX.Family,
					(A->SHM->P.Features.Std.AX.ExtModel << 4) + A->SHM->P.Features.Std.AX.Model,
					A->SHM->P.Features.Std.AX.Stepping,
					A->SHM->P.Features.ThreadCount,
					A->SHM->H.Architecture,
					powered(A->SHM->P.Features.Std.DX.VME),
					powered(A->SHM->P.Features.Std.DX.DE),
					powered(A->SHM->P.Features.Std.DX.PSE),
					A->SHM->P.Features.InvariantTSC ? "INVARIANT":"VARIANT",powered(A->SHM->P.Features.Std.DX.TSC),
					powered(A->SHM->P.Features.Std.DX.MSR),			enabled(A->SHM->CPL.MSR),
					powered(A->SHM->P.Features.Std.DX.PAE),
					powered(A->SHM->P.Features.Std.DX.APIC),
					powered(A->SHM->P.Features.Std.DX.MTRR),		enabled(A->SHM->P.MTRRdefType.Enable),
					powered(A->SHM->P.Features.Std.DX.PGE),
					powered(A->SHM->P.Features.Std.DX.MCA),
					powered(A->SHM->P.Features.Std.DX.PAT),
					powered(A->SHM->P.Features.Std.DX.PSE36),
					powered(A->SHM->P.Features.Std.DX.PSN),
					powered(A->SHM->P.Features.Std.DX.DS_PEBS),		enabled(!A->SHM->P.MiscFeatures.PEBS),
					powered(A->SHM->P.Features.Std.DX.ACPI),
					powered(A->SHM->P.Features.Std.DX.SS),
					powered(A->SHM->P.Features.Std.DX.HTT),			enabled(A->SHM->P.Features.HTT_enabled),
					powered(A->SHM->P.Features.Std.DX.TM1),			enabled(A->SHM->P.MiscFeatures.TCC),
					powered(A->SHM->P.Features.Std.CX.TM2),			enabled(A->SHM->P.MiscFeatures.TM2_Enable),
					powered(A->SHM->P.Features.Std.DX.PBE),
					powered(A->SHM->P.Features.Std.CX.DTES64),
					powered(A->SHM->P.Features.Std.CX.DS_CPL),
					powered(A->SHM->P.Features.Std.CX.VMX),
					powered(A->SHM->P.Features.Std.CX.SMX),
					enabled(A->SHM->P.Power.Control.C1E), powered(A->SHM->P.Features.Std.CX.EIST), enabled(A->SHM->P.MiscFeatures.EIST),
					powered(A->SHM->P.Features.Std.CX.CNXT_ID),
					powered(A->SHM->P.Features.Std.CX.FMA),
					powered(A->SHM->P.Features.Std.CX.xTPR),		enabled(!A->SHM->P.MiscFeatures.xTPR),
					powered(A->SHM->P.Features.Std.CX.PDCM),
					powered(A->SHM->P.Features.Std.CX.PCID),
					powered(A->SHM->P.Features.Std.CX.DCA),
					powered(A->SHM->P.Features.Std.CX.x2APIC),
					powered(A->SHM->P.Features.Std.CX.TSCDEAD),
					powered(A->SHM->P.Features.Std.CX.XSAVE),
					powered(A->SHM->P.Features.Std.CX.OSXSAVE),
					powered(A->SHM->P.Features.ExtFunc.DX.XD_Bit),		enabled(!A->SHM->P.MiscFeatures.XD_Bit),
					powered(A->SHM->P.Features.ExtFunc.DX.PG_1GB),
					powered(A->SHM->P.Features.ExtFeature.BX.HLE),
					powered(A->SHM->P.Features.ExtFeature.BX.RTM),
					powered(A->SHM->P.Features.ExtFeature.BX.FastStrings),	enabled(A->SHM->P.MiscFeatures.FastStrings),
					powered(A->SHM->P.Features.Thermal_Power_Leaf.AX.DTS),
												enabled(A->SHM->P.MiscFeatures.TCC),
												enabled(A->SHM->P.MiscFeatures.PerfMonitoring),
												enabled(!A->SHM->P.MiscFeatures.BTS),
												enabled(A->SHM->P.MiscFeatures.CPUID_MaxVal),

					powered(A->SHM->P.Features.Thermal_Power_Leaf.AX.TurboIDA),	(A->SHM->P.MiscFeatures.Turbo_IDA) ? "OFF"
													: ((A->SHM->P.PerfControl.Turbo_IDA) ? "DIS" : "ON"),

					A->SHM->P.Boost[0], A->SHM->P.Boost[1], A->SHM->P.Boost[2], A->SHM->P.Boost[3], A->SHM->P.Boost[4], A->SHM->P.Boost[5], A->SHM->P.Boost[6], A->SHM->P.Boost[7], A->SHM->P.Boost[8], A->SHM->P.Boost[9],

					powered(A->SHM->P.Features.Std.DX.FPU),
					powered(A->SHM->P.Features.Std.DX.CX8),
					powered(A->SHM->P.Features.Std.DX.SEP),
					powered(A->SHM->P.Features.Std.DX.CMOV),
					powered(A->SHM->P.Features.Std.DX.CLFSH),
					powered(A->SHM->P.Features.Std.DX.MMX),
					powered(A->SHM->P.Features.Std.DX.FXSR),
					powered(A->SHM->P.Features.Std.DX.SSE),
					powered(A->SHM->P.Features.Std.DX.SSE2),
					powered(A->SHM->P.Features.Std.CX.SSE3),
					powered(A->SHM->P.Features.Std.CX.SSSE3),
					powered(A->SHM->P.Features.Std.CX.SSE41),
					powered(A->SHM->P.Features.Std.CX.SSE42),
					powered(A->SHM->P.Features.Std.CX.PCLMULDQ),
					powered(A->SHM->P.Features.Std.CX.MONITOR),		enabled(A->SHM->P.MiscFeatures.FSM),
					powered(A->SHM->P.Features.Std.CX.CX16),
					powered(A->SHM->P.Features.Std.CX.MOVBE),
					powered(A->SHM->P.Features.Std.CX.POPCNT),
					powered(A->SHM->P.Features.Std.CX.AES),
					powered(A->SHM->P.Features.Std.CX.AVX), powered(A->SHM->P.Features.ExtFeature.BX.AVX2),
					powered(A->SHM->P.Features.Std.CX.F16C),
					powered(A->SHM->P.Features.Std.CX.RDRAND),
					powered(A->SHM->P.Features.ExtFunc.CX.LAHFSAHF),
					powered(A->SHM->P.Features.ExtFunc.DX.RDTSCP),
					powered(A->SHM->P.Features.ExtFunc.DX.IA64),
					powered(A->SHM->P.Features.ExtFeature.BX.BMI1), powered(A->SHM->P.Features.ExtFeature.BX.BMI2),
					powered(A->SHM->P.Features.ExtFunc.DX.SYSCALL),		enabled(A->SHM->P.ExtFeature.SCE),
					(A->SHM->P.ExtFeature.LMA) ? "ON" : "DIS",
					(A->SHM->P.ExtFeature.NXE) ? "ON" : "DIS");

				char *buf[2]={malloc(1024), malloc(1024)};
				sprintf(buf[0], TOPOLOGY_SECTION, A->SHM->P.OnLine);
				sprintf(buf[1], PERF_SECTION,
						A->SHM->P.Features.Perf_Monitoring_Leaf.AX.MonCtrs, A->SHM->P.Features.Perf_Monitoring_Leaf.AX.MonWidth,
						A->SHM->P.Features.Perf_Monitoring_Leaf.DX.FixCtrs, A->SHM->P.Features.Perf_Monitoring_Leaf.DX.FixWidth);
				int cpu=0;
				for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
				{
					if(A->SHM->C[cpu].T.Offline != TRUE)
						sprintf(str, TOPOLOGY_FORMAT,
							cpu,
							A->SHM->C[cpu].T.APIC_ID,
							A->SHM->C[cpu].T.Core_ID,
							A->SHM->C[cpu].T.Thread_ID,
							enabled(1));
					else
						sprintf(str, "%03u        -       -       -   [%3s]\n",
							cpu,
							enabled(0));
					strcat(buf[0], str);

					if(A->SHM->C[cpu].T.Offline != TRUE)
						sprintf(str, PERF_FORMAT,
							cpu,
							A->SHM->C[cpu].FixedPerfCounter.EN0_OS,
							A->SHM->C[cpu].FixedPerfCounter.EN0_Usr,
							A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN0,
							A->SHM->C[cpu].FixedPerfCounter.EN1_OS,
							A->SHM->C[cpu].FixedPerfCounter.EN1_Usr,
							A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN1,
							A->SHM->C[cpu].FixedPerfCounter.EN2_OS,
							A->SHM->C[cpu].FixedPerfCounter.EN2_Usr,
							A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN2);
					else
						sprintf(str, "%03u      -     -     -            -     -     -            -     -     -\n", cpu);
					strcat(buf[1], str);
				}
				strcat(items, buf[0]);
				strcat(items, buf[1]);
				free(buf[0]);
				free(buf[1]);

				if(A->SHM->CPL.SMBIOS == TRUE)
				{
					const float tension[0B1000]={0.0f, 5.0f, 3.3f, 0.0f, 2.9f, 0.0f, 0.0f, 0.0f};
					unsigned long long totalMemSize=0;
					int                ix=0;
					for(ix=0; ix < A->SHM->B->MemArray->Attrib->Number_Devices; ix++)
						totalMemSize+=A->SHM->B->Memory[ix]->Attrib->Size;

					strcat(items, SMBIOS_SECTION);

					sprintf(str, SMBIOS4_FORMAT,
						Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Proc, A->SHM->B->Proc->Attrib->Version),
						Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Proc, A->SHM->B->Proc->Attrib->Socket),
						A->SHM->B->Proc->Attrib->Voltage.Mode ? \
							A->SHM->B->Proc->Attrib->Voltage.Tension / 10.0f \
							: tension[A->SHM->B->Proc->Attrib->Voltage.Tension & 0B0111]);
					strcat(items, str);

					for(ix=0; ix < 3; ix++)
					{
						sprintf(str, SMBIOS7_FORMAT, \
							Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Cache[ix], A->SHM->B->Cache[ix]->Attrib->Socket), \
							A->SHM->B->Cache[ix]->Attrib->Installed_Size, ix < 2 ? "     " : "");
						strcat(items, str);
					}
					sprintf(str, "        |- manufactured by %s\n", Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Proc, A->SHM->B->Proc->Attrib->Manufacturer));
					strcat(items, str);

					sprintf(str, SMBIOS2_FORMAT,
						Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Board, A->SHM->B->Board->Attrib->Product),
						Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Board, A->SHM->B->Board->Attrib->Version),
						Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Board, A->SHM->B->Board->Attrib->Manufacturer),
						Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Board, A->SHM->B->Board->Attrib->Serial));
					strcat(items, str);

					sprintf(str, SMBIOS0_FORMAT,
						Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Bios, A->SHM->B->Bios->Attrib->Vendor),
						Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Bios, A->SHM->B->Bios->Attrib->Version),
						Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Bios, A->SHM->B->Bios->Attrib->Release_Date),
						A->SHM->B->Bios->Attrib->Major_Release, A->SHM->B->Bios->Attrib->Minor_Release,
						64 * (1 + A->SHM->B->Bios->Attrib->ROM_Size), A->SHM->B->Bios->Attrib->Address);
					strcat(items, str);

					sprintf(str, SMBIOS16_FORMAT,
						totalMemSize, A->SHM->B->MemArray->Attrib->Maximum_Capacity / 1024);
					strcat(items, str);

					for(ix=0; ix < A->SHM->B->MemArray->Attrib->Number_Devices; ix++)
					{
						sprintf(str, SMBIOS17_FORMAT, \
							Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Memory[ix], A->SHM->B->Memory[ix]->Attrib->Socket), \
							Smb_Find_String((struct STRUCTINFO*) A->SHM->B->Memory[ix], A->SHM->B->Memory[ix]->Attrib->Bank), \
							A->SHM->B->Memory[ix]->Attrib->Size, A->SHM->B->Memory[ix]->Attrib->Speed);
						strcat(items, str);
					}
				}
				strcat(items, RAM_SECTION);
				strcat(items, CHA_FORMAT);

				if(A->SHM->CPL.IMC && (A->SHM->M.ChannelCount > 0))	// Is the IMC found ?
				{
					unsigned int cha=0;
					for(cha=0; cha < A->SHM->M.ChannelCount; cha++)
					{
						sprintf(str, CAS_FORMAT,
							cha,
							A->SHM->M.Channel[cha].Timing.tCL,
							A->SHM->M.Channel[cha].Timing.tRCD,
							A->SHM->M.Channel[cha].Timing.tRP,
							A->SHM->M.Channel[cha].Timing.tRAS,
							A->SHM->M.Channel[cha].Timing.tRRD,
							A->SHM->M.Channel[cha].Timing.tRFC,
							A->SHM->M.Channel[cha].Timing.tWR,
							A->SHM->M.Channel[cha].Timing.tRTPr,
							A->SHM->M.Channel[cha].Timing.tWTPr,
							A->SHM->M.Channel[cha].Timing.tFAW,
							A->SHM->M.Channel[cha].Timing.B2B);
						strcat(items, str);
					}
				}
				else
					strcat(items, "Unknown IMC\n");

				// Clear the scrolling area.
				XSetForeground(A->display, A->W[G].gc, A->W[G].background);
				XFillRectangle(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
						0,
						0,
						GetHFrame(G) * One_Char_Width(G),
						GetVFrame(G) * One_Char_Height(G));
				// Draw the system information.
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_PRINT].RGB);
				A->L.Page[G].Listing=XPrint(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
							Half_Char_Width(G),
							One_Char_Height(G),
							items,
							One_Char_Height(G));
				free(items);
			}
			else
			{
				sprintf(str, OVERCLOCK, A->SHM->P.Features.BrandString, A->SHM->P.Boost[1] * A->SHM->P.ClockSpeed);
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_PRINT].RGB);
				XDrawString(A->display, A->W[G].pixmap.B, A->W[G].gc,
						Half_Char_Width(G),
						Header_Height(G) + One_Char_Height(G),
						str,
						strlen(str));
			}
			free(str);
		}
			break;
		case DUMP:
		{
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
					One_Char_Width(G),
					One_Char_Height(G),
					A->L.Page[G].Title,
					strlen(A->L.Page[G].Title) );
		}
			break;
	}
}

// Fusion in one map the background and the foreground layouts.
void	MapLayout(uARG *A, int G)
{
	XCopyArea(A->display, A->W[G].pixmap.B, A->W[G].pixmap.F, A->W[G].gc, 0, 0, A->W[G].width, A->W[G].height, 0, 0);
}

// Send the map to the display.
void	FlushLayout(uARG *A, int G)
{
	if(A->L.Page[G].Pageable)
		XCopyArea(A->display, A->L.Page[G].Pixmap, A->W[G].pixmap.F, A->W[G].gc,
			One_Char_Width(G) *  A->L.Page[G].hScroll,
			One_Char_Height(G) * A->L.Page[G].vScroll,
			A->L.Page[G].width,
			A->L.Page[G].height,
			0,
			Header_Height(G) + 2 );	// below the header line.

	XCopyArea(A->display,A->W[G].pixmap.F, A->W[G].window, A->W[G].gc, 0, 0, A->W[G].width, A->W[G].height, 0, 0);
	XFlush(A->display);
}

// Draw the TextCursor shape.
void	DrawCursor(uARG *A, int G, XPoint *Origin)
{
	// Flash the TextCursor.
	A->L.Play.flashCursor=!A->L.Play.flashCursor;
	XSetForeground(A->display, A->W[G].gc, A->L.Play.flashCursor ? A->L.Colors[COLOR_CURSOR].RGB : A->W[G].background);

	if(A->L.Play.cursorShape == TRUE)
		{
		A->L.TextCursor[0].x=Origin->x;
		A->L.TextCursor[0].y=Origin->y;
		XFillPolygon(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.TextCursor, 3, Nonconvex, CoordModePrevious);
		}
	else
		XDrawRectangle(A->display, A->W[G].pixmap.F, A->W[G].gc, Origin->x - 1, Origin->y - Footer_Height(G) + 1, One_Char_Width(G), Footer_Height(G) - 2);
}

// Draw the layout foreground.
void	DrawLayout(uARG *A, int G)
{
	switch(G)
	{
		case MAIN:
		{
			int edline=_IS_MDI_ ? A->L.Axes[G].Segment[1].y2 + Footer_Height(G) + 1 : A->W[G].height;
			const int KeyStop=MAIN_TEXT_WIDTH - 8;
			XPoint Origin=	{
					.x=0,
					.y=edline - 1
					};
			if(A->L.Input.KeyInsert > KeyStop)
				Origin.x=(KeyStop * One_Char_Width(G)) + Quarter_Char_Width(G);
			else
				Origin.x=(A->L.Input.KeyInsert * One_Char_Width(G)) + Quarter_Char_Width(G);

			// Draw the buffer if it is not empty.
			if(A->L.Input.KeyLength > 0)
			{
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_PROMPT].RGB);

				if(A->L.Input.KeyInsert > KeyStop)
				{
					const int KeyShift=A->L.Input.KeyInsert - KeyStop;

					XDrawImageString(A->display, A->W[G].pixmap.F, A->W[G].gc,
							Quarter_Char_Width(G),
							edline - Half_Char_Height(G),
							&A->L.Input.KeyBuffer[KeyShift], KeyStop);
				}
				else
					XDrawImageString(A->display, A->W[G].pixmap.F, A->W[G].gc,
							Quarter_Char_Width(G),
							edline - Half_Char_Height(G),
							&A->L.Input.KeyBuffer[0], MIN(A->L.Input.KeyLength, KeyStop));
			}
			DrawCursor(A, G, &Origin);
		}
			break;
		case CORES:
		{
			char str[TASK_COMM_LEN]={0};
			int cpu=0;
			for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
				if(A->SHM->C[cpu].T.Offline != TRUE)
				{
					// Select a color based ratio.
					const struct {
						unsigned long int Bg, Fg;
					} Bar[10]
					= {
						{A->L.Colors[COLOR_INIT_VALUE].RGB, A->L.Colors[COLOR_DYNAMIC].RGB},
						{A->L.Colors[COLOR_LOW_VALUE].RGB,  A->L.Colors[BACKGROUND_CORES].RGB},
						{A->L.Colors[COLOR_MED_VALUE].RGB,  A->L.Colors[BACKGROUND_CORES].RGB},
						{A->L.Colors[COLOR_MED_VALUE].RGB,  A->L.Colors[BACKGROUND_CORES].RGB},
						{A->L.Colors[COLOR_MED_VALUE].RGB,  A->L.Colors[BACKGROUND_CORES].RGB},
						{A->L.Colors[COLOR_MED_VALUE].RGB,  A->L.Colors[BACKGROUND_CORES].RGB},
						{A->L.Colors[COLOR_MED_VALUE].RGB,  A->L.Colors[BACKGROUND_CORES].RGB},
						{A->L.Colors[COLOR_MED_VALUE].RGB,  A->L.Colors[BACKGROUND_CORES].RGB},
						{A->L.Colors[COLOR_HIGH_VALUE].RGB, A->L.Colors[BACKGROUND_CORES].RGB},
						{A->L.Colors[COLOR_HIGH_VALUE].RGB, A->L.Colors[BACKGROUND_CORES].RGB},
					};
					unsigned int i=0;
					for(i=0; i < 9; i++)
						if(A->SHM->P.Boost[i] != 0)
							if(!(A->SHM->C[cpu].RelativeRatio > A->SHM->P.Boost[i]))
								break;

					// Draw the Core frequency.
					XSetForeground(A->display, A->W[G].gc, Bar[i].Bg);
					if(A->L.Play.fillGraphics)
						XFillRectangle(	A->display, A->W[G].pixmap.F, A->W[G].gc,
								One_Char_Width(G) * 3,
								3 + ( One_Char_Height(G) * (cpu + 1) ),
								(A->W[G].extents.overall.width * A->SHM->C[cpu].RelativeRatio) / CORES_TEXT_WIDTH,
								One_Char_Height(G) - 3);
					else
						XDrawRectangle(	A->display, A->W[G].pixmap.F, A->W[G].gc,
								One_Char_Width(G) * 3,
								3 + ( One_Char_Height(G) * (cpu + 1) ),
								(A->W[G].extents.overall.width * A->SHM->C[cpu].RelativeRatio) / CORES_TEXT_WIDTH,
								One_Char_Height(G) - 3);

					// For each Core, display its frequency, C-STATE & ratio.
					if(A->L.Play.freqHertz && (A->SHM->C[cpu].RelativeRatio >= 5.0f) )
					{
						sprintf(str, CORE_FREQ, A->SHM->C[cpu].RelativeFreq);
						if(A->L.Play.fillGraphics)
							XSetForeground(A->display, A->W[G].gc, Bar[i].Fg);
						else
							XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_DYNAMIC].RGB);
						XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
								One_Char_Width(G) * 5,
								One_Char_Height(G) * (cpu + 1 + 1),
								str, strlen(str) );
					}

					if(A->L.Play.showCycles)
					{
						if(!A->L.Play.showIPS
						&& !A->L.Play.showIPC
						&& !A->L.Play.showCPI)
						{
						useconds_t LoopTime=IDLE_BASE_USEC * A->SHM->P.IdleTime;

						sprintf(str,	CORE_DELTA,
								A->SHM->C[cpu].Delta.INST / LoopTime,
								A->SHM->C[cpu].Delta.C0.UCC / LoopTime,
								A->SHM->C[cpu].Delta.C0.URC / LoopTime,
								(A->SHM->C[cpu].Delta.C1+A->SHM->C[cpu].Delta.C3+A->SHM->C[cpu].Delta.C6+A->SHM->C[cpu].Delta.C7) / LoopTime,
								A->SHM->C[cpu].Delta.TSC / LoopTime);
						XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_DYNAMIC].RGB);
						XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
								One_Char_Width(G) * 13,
								One_Char_Height(G) * (cpu + 1 + 1),
								str, strlen(str) );
						}
					}
					else
					{
						if(A->SHM->S.Monitor)
						{
							if(!A->L.Play.showRatios)
							{
								if(A->SHM->C[cpu].Task[0].pid > 0)
								{
									sprintf(str, TASK_PID_FMT, A->SHM->C[cpu].Task[0].pid);
									int l=strlen(str);

									XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_GRAPH1].RGB);
									XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
											A->W[G].width - (One_Char_Width(G) * l),
											One_Char_Height(G) * (cpu + 1 + 1),
											str, l );
								}
							}
							if(!A->L.Play.showIPS
							&& !A->L.Play.showIPC
							&& !A->L.Play.showCPI)
							{
							XRectangle R[]=
							{ {
								.x=One_Char_Width(G) * 13,
								.y=Header_Height(G),
								.width=One_Char_Width(G) * ((CORES_TEXT_WIDTH - 7) << 1),
								.height=One_Char_Height(G) * CORES_TEXT_HEIGHT
							} };
							XSetClipRectangles(A->display, A->W[G].gc, 0, 0, R, 1, Unsorted);

							int depth=0, L=(CORES_TEXT_WIDTH << 1);
							do
							{
								int l=(A->SHM->C[cpu].Task[depth].pid == 0) ? 0 : strlen(A->SHM->C[cpu].Task[depth].comm);
								L-=((l > 0) ? (l + 2) : 0);

								if(l > 0)
								{
									XSetForeground(	A->display, A->W[G].gc,
											(depth == 0) ? A->L.Colors[COLOR_PULSE].RGB
											: !strcasecmp(A->SHM->C[cpu].Task[depth].state, "R") ?
												A->L.Colors[COLOR_GRAPH1].RGB
												: A->L.Colors[COLOR_LABEL].RGB);
									XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
											One_Char_Width(G) * L,
											One_Char_Height(G) * (cpu + 1 + 1),
											A->SHM->C[cpu].Task[depth].comm, l );
								}
								depth++;
							}
							while(depth < TASK_PIPE_DEPTH) ;

							XSetClipMask(A->display, A->W[G].gc, None);
							}
						}
						else if(!A->L.Play.showRatios
						&&	!A->L.Play.showIPS
						&&	!A->L.Play.showIPC
						&&	!A->L.Play.showCPI)
						{
							sprintf(str,	CORE_CYCLES,
									A->SHM->C[cpu].Cycles.C0[1].UCC,
									A->SHM->C[cpu].Cycles.C0[1].URC);
							XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_DYNAMIC].RGB);
							XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
									One_Char_Width(G) * 13,
									One_Char_Height(G) * (cpu + 1 + 1),
									str, strlen(str) );
						}
					}
					if(A->L.Play.showIPS)
					{
						sprintf(str, CORE_IPS, A->SHM->C[cpu].IPS);
						XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_DYNAMIC].RGB);
						XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
								One_Char_Width(G) * 15,
								One_Char_Height(G) * (cpu + 1 + 1),
								str, strlen(str) );
					}
					if(A->L.Play.showIPC)
					{
						sprintf(str, CORE_IPS, A->SHM->C[cpu].IPC);
						XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_DYNAMIC].RGB);
						XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
								One_Char_Width(G) * 23,
								One_Char_Height(G) * (cpu + 1 + 1),
								str, strlen(str) );
					}
					if(A->L.Play.showCPI)
					{
						sprintf(str, CORE_IPS, A->SHM->C[cpu].CPI);
						XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_DYNAMIC].RGB);
						XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
								One_Char_Width(G) * 31,
								One_Char_Height(G) * (cpu + 1 + 1),
								str, strlen(str) );
					}
					if(A->L.Play.showRatios)
					{
						sprintf(str, CORE_RATIO, A->SHM->C[cpu].RelativeRatio);
						XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_DYNAMIC].RGB);
						XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
								Twice_Char_Width(G) * CORES_TEXT_WIDTH,
								One_Char_Height(G) * (cpu + 1 + 1),
								str, strlen(str) );
					}
				}
				else
				{
					XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_LABEL].RGB);
					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							Twice_Char_Width(G) * CORES_TEXT_WIDTH,
							One_Char_Height(G) * (cpu + 1 + 1),
							"OFF", 3 );
				}
		}
			break;
		case CSTATES:
		{
			char *str=calloc(72, 1);
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			int cpu=0;
			for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
				if(A->SHM->C[cpu].T.Offline != TRUE)
				{
					// Prepare the C0 chart.
					A->L.Usage.C0[cpu].x=Half_Char_Width(G) + ((cpu * CSTATES_TEXT_SPACING) * One_Half_Char_Width(G));
					A->L.Usage.C0[cpu].y=One_Char_Height(G)
								+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * (1 - A->SHM->C[cpu].State.C0);
					A->L.Usage.C0[cpu].width=(One_Char_Width(G) * CSTATES_TEXT_SPACING) / 5;
					A->L.Usage.C0[cpu].height=One_Char_Height(G)
								+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * A->SHM->C[cpu].State.C0;
					// Prepare the C1 chart.
					A->L.Usage.C1[cpu].x=Half_Char_Width(G) + A->L.Usage.C0[cpu].x + A->L.Usage.C0[cpu].width;
					A->L.Usage.C1[cpu].y=One_Char_Height(G)
								+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * (1 - A->SHM->C[cpu].State.C1);
					A->L.Usage.C1[cpu].width=(One_Char_Width(G) * CSTATES_TEXT_SPACING) / 5;
					A->L.Usage.C1[cpu].height=One_Char_Height(G)
								+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * A->SHM->C[cpu].State.C1;
					// Prepare the C3 chart.
					A->L.Usage.C3[cpu].x=Half_Char_Width(G) + A->L.Usage.C1[cpu].x + A->L.Usage.C1[cpu].width;
					A->L.Usage.C3[cpu].y=One_Char_Height(G)
								+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * (1 - A->SHM->C[cpu].State.C3);
					A->L.Usage.C3[cpu].width=(One_Char_Width(G) * CSTATES_TEXT_SPACING) / 5;
					A->L.Usage.C3[cpu].height=One_Char_Height(G)
								+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * A->SHM->C[cpu].State.C3;
					// Prepare the C6 chart.
					A->L.Usage.C6[cpu].x=Half_Char_Width(G) + A->L.Usage.C3[cpu].x + A->L.Usage.C3[cpu].width;
					A->L.Usage.C6[cpu].y=One_Char_Height(G)
								+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * (1 - A->SHM->C[cpu].State.C6);
					A->L.Usage.C6[cpu].width=(One_Char_Width(G) * CSTATES_TEXT_SPACING) / 5;
					A->L.Usage.C6[cpu].height=One_Char_Height(G)
								+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * A->SHM->C[cpu].State.C6;
					// Prepare the C7 chart.
					A->L.Usage.C7[cpu].x=Half_Char_Width(G) + A->L.Usage.C6[cpu].x + A->L.Usage.C6[cpu].width;
					A->L.Usage.C7[cpu].y=One_Char_Height(G)
								+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * (1 - A->SHM->C[cpu].State.C7);
					A->L.Usage.C7[cpu].width=(One_Char_Width(G) * CSTATES_TEXT_SPACING) / 5;
					A->L.Usage.C7[cpu].height=One_Char_Height(G)
								+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * A->SHM->C[cpu].State.C7;
				}			// Display the C-State averages.
			sprintf(str, CSTATES_AVERAGE,	100.f * A->SHM->P.Avg.Turbo,
							100.f * A->SHM->P.Avg.C0,
							100.f * A->SHM->P.Avg.C1,
							100.f * A->SHM->P.Avg.C3,
							100.f * A->SHM->P.Avg.C6,
							100.f * A->SHM->P.Avg.C7);
			XDrawString(A->display, A->W[G].pixmap.F, A->W[G].gc,
						Twice_Char_Width(G),
						One_Char_Height(G) + (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT + 1)),
						str, strlen(str) );

			// Draw C0 to C7 states.
			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_GRAPH1].RGB);
			XFillRectangles(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Usage.C0, A->SHM->P.CPU);
			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_GRAPH2].RGB);
			XFillRectangles(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Usage.C1, A->SHM->P.CPU);
			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_GRAPH3].RGB);
			XFillRectangles(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Usage.C3, A->SHM->P.CPU);
			XFillRectangles(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Usage.C6, A->SHM->P.CPU);
			XFillRectangles(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Usage.C7, A->SHM->P.CPU);

			if(A->L.Play.cStatePercent)
				for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
					if(A->SHM->C[cpu].T.Offline != TRUE)
					{
						XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
						sprintf(str, CORE_NUM, cpu);
						XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
								0,
								One_Char_Height(G) * (cpu + 1 + 1),
								str, strlen(str) );

						XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_PRINT].RGB);
						sprintf(str, CSTATES_PERCENT,	100.f * A->SHM->C[cpu].State.Turbo,
										100.f * A->SHM->C[cpu].State.C0,
										100.f * A->SHM->C[cpu].State.C1,
										100.f * A->SHM->C[cpu].State.C3,
										100.f * A->SHM->C[cpu].State.C6,
										100.f * A->SHM->C[cpu].State.C7);

						XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
								One_Char_Width(G) << 2,
								One_Char_Height(G) * (cpu + 1 + 1),
								str, strlen(str) );
					}
			free(str);
		}
			break;
		case TEMPS:
		{
			char str[8]={0};
			// Display the Core temperature.
			int cpu=0;
			for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
				if(A->SHM->C[cpu].T.Offline != TRUE)
				{
					// Update & draw the temperature histogram per logical core.
					int i=0;
					XSegment *U=&A->L.Temps[cpu].Segment[i], *V=&A->L.Temps[cpu].Segment[i + 1];
					for(i=0; i < (TEMPS_TEXT_WIDTH - 1); i++, U=&A->L.Temps[cpu].Segment[i], V=&A->L.Temps[cpu].Segment[i + 1])
					{
						U->x1=V->x1 - One_Char_Width(G);
						U->x2=V->x2 - One_Char_Width(G);
						U->y1=V->y1;
						U->y2=V->y2;
					}
					V=&A->L.Temps[cpu].Segment[i - 1];
					U->x1=(TEMPS_TEXT_WIDTH + 2) * One_Char_Width(G);
					U->y1=V->y2;
					U->x2=U->x1 + One_Char_Width(G);
					U->y2=(A->W[G].height * A->SHM->C[cpu].ThermStat.DTS) / A->SHM->C[cpu].TjMax.Target;

					XSetForeground(A->display, A->W[G].gc,	A->L.Colors[COLOR_GRAPH1].RGB
										| fROL32(0x28104080, cpu)
										| fROR32(0x01010011, A->SHM->C[cpu].T.APIC_ID)
										| fROL32(0x80808080, A->SHM->C[cpu].T.Core_ID)
										| fROR32(0x01010100, A->SHM->C[cpu].T.Thread_ID));
					if(A->L.Play.showTemps[cpu] == TRUE)
						XDrawSegments(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Temps[cpu].Segment, A->L.Temps[cpu].N);

					sprintf(str, TEMPERATURE, A->SHM->C[cpu].TjMax.Target - A->SHM->C[cpu].ThermStat.DTS);
					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							(One_Char_Width(G) * 5) + Quarter_Char_Width(G) + (cpu << 1) * Twice_Char_Width(G),
							One_Char_Height(G) * (TEMPS_TEXT_HEIGHT + 1 + 1),
							str, strlen(str));
				}
				else	// Is the Core disabled ?
				{
					XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_LABEL].RGB);
					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							(One_Char_Width(G) * 5) + Quarter_Char_Width(G) + (cpu << 1) * Twice_Char_Width(G),
							One_Char_Height(G) * (TEMPS_TEXT_HEIGHT + 1 + 1),
							"OFF", 3);
				}
			// Show Temperature Thresholds
/*
			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_GRAPH2].RGB);
			int Threshold[2]={
					(( (TEMPS_TEXT_HEIGHT * A->SHM->C[A->SHM->P.Hot].ThermIntr.Threshold1)
					/ A->SHM->C[A->SHM->P.Hot].TjMax.Target) + 2) * One_Char_Height(G),
					(( (TEMPS_TEXT_HEIGHT * A->SHM->C[A->SHM->P.Hot].ThermIntr.Threshold2)
					/ A->SHM->C[A->SHM->P.Hot].TjMax.Target) + 2) * One_Char_Height(G)
					};
			XDrawLine(	A->display, A->W[G].pixmap.F, A->W[G].gc,
					Twice_Char_Width(G),
					Threshold[0],
					U->x2,
					Threshold[1]);
			XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_DYNAMIC].RGB);
			XDrawString(A->display,
					A->W[G].pixmap.F,
					A->W[G].gc,
					One_Half_Char_Width(G) << 2,
					Threshold[0],
					"T#1", 3);
			XDrawString(A->display,
					A->W[G].pixmap.F,
					A->W[G].gc,
					TEMPS_TEXT_WIDTH * One_Char_Width(G),
					Threshold[1],
					"T#2", 3);
*/
			// Display the hottest temperature between all Cores, and the coldest since start up.
			int	HotValue=A->SHM->C[A->SHM->P.Hot].TjMax.Target - A->SHM->C[A->SHM->P.Hot].ThermStat.DTS;

			if(HotValue <= LOW_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_INIT_VALUE].RGB);
			if(HotValue > LOW_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_LOW_VALUE].RGB);
			if(HotValue > MED_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_MED_VALUE].RGB);
			if(HotValue >= HIGH_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_HIGH_VALUE].RGB);
			sprintf(str, TEMPERATURE, HotValue);
			XDrawImageString(A->display,
					A->W[G].pixmap.F,
					A->W[G].gc,
					((TEMPS_TEXT_WIDTH + 2) * One_Char_Width(G)) + One_Char_Width(G),
					(A->W[G].height * A->SHM->C[A->SHM->P.Hot].ThermStat.DTS) / A->SHM->C[A->SHM->P.Hot].TjMax.Target,
					str, 3);

			int	ColdValue=A->SHM->C[A->SHM->P.Hot].TjMax.Target - A->SHM->P.Cold,
				yCold=(A->W[G].height * A->SHM->P.Cold) / A->SHM->C[A->SHM->P.Hot].TjMax.Target;

			if(ColdValue <= LOW_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_INIT_VALUE].RGB);
			if(ColdValue > LOW_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_LOW_VALUE].RGB);
			if(ColdValue > MED_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_MED_VALUE].RGB);
			if(ColdValue >= HIGH_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_HIGH_VALUE].RGB);
			sprintf(str, TEMPERATURE, ColdValue);
			XDrawImageString(A->display,
					A->W[G].pixmap.F,
					A->W[G].gc,
					0,
					yCold,
					str, 3);
		}
			break;
		case SYSINFO:
			// Display the Wallboard.
			if(A->L.Play.wallboard)
			{
				// Scroll the Wallboard.
				if(A->L.WB.Scroll < A->L.WB.Length)
					A->L.WB.Scroll++;
				else
					A->L.WB.Scroll=0;
				int padding=SYSINFO_TEXT_WIDTH - strlen(A->L.Page[G].Title) - 6;

				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_LABEL].RGB);
				XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
						One_Char_Width(G) * (strlen(A->L.Page[G].Title) + 2),
						One_Char_Height(G),
						&A->L.WB.String[A->L.WB.Scroll], padding);
			}
			break;
		case DUMP:
			// Dump a bunch of Registers with their Address, Name & Value.
		{
			// Dump the built in array then add the user specified registers.
			int Rows=DUMP_ARRAY_DIMENSION, row=0;

			char *items=calloc(Rows, 128);
			char *mask=calloc(128, 1), *str=calloc(128, 1);
			char *binStr=calloc(DUMP_BIN64_STR + 1, 1);
			for(row=0; row < Rows; row++)
			{
				DumpRegister(A->SHM->D.Array[row].Value, NULL, binStr);

				sprintf(mask, REG_FORMAT_BOL, row,
					A->SHM->D.Array[row].Addr,
					A->SHM->D.Array[row].Name,
					DUMP_REG_ALIGN - strlen(A->SHM->D.Array[row].Name));
				sprintf(str, mask, 0x20);
				strcat(items, str);

				int H=0;
				for(H=0; H < 15; H++)
				{
					strncat(items, &binStr[H << 2], 4);
					strcat(items, " ");
				};
				strncat(items, &binStr[H << 2], 4);
				sprintf(str, REG_FORMAT_EOL, A->SHM->D.Array[row].Core);
				strcat(items, str);
			}
			free(binStr);
			free(str);
			free(mask);
			if(A->L.Page[G].Pageable)
			{	// Clear the scrolling area.
				XSetForeground(A->display, A->W[G].gc, A->W[G].background);
				XFillRectangle(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
						0,
						0,
						GetHFrame(G) * One_Char_Width(G),
						GetVFrame(G) * One_Char_Height(G));
				// Draw the Registers in a scrolling area.
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_PRINT].RGB);
				A->L.Page[G].Listing=XPrint(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
							One_Char_Width(G),
							One_Char_Height(G),
							items,
							One_Char_Height(G));
			}
			else
			{	// Draw the Registers in a fixed height area.
				XSetForeground(A->display, A->W[G].gc, A->L.Colors[COLOR_PRINT].RGB);
				A->L.Page[G].Listing=XPrint(A->display, A->W[G].pixmap.F, A->W[G].gc,
							One_Char_Width(G),
							Header_Height(G) + One_Char_Height(G),
							items,
							One_Char_Height(G));
			}
			free(items);
		}
			break;
	}
}

// Update the Widget name with the Top Core frequency, temperature, c-states.
void	UpdateWidgetName(uARG *A, int G)
{
	char *str=calloc(64, 1);
	switch(G) {
		case MAIN:
			if(_IS_MDI_)
			{
				sprintf(str, TITLE_MDI_FMT,
						A->SHM->C[A->SHM->P.Top].RelativeFreq,
						A->SHM->C[A->SHM->P.Hot].TjMax.Target - A->SHM->C[A->SHM->P.Hot].ThermStat.DTS);
				SetWidgetName(A, G, str);
			}
			break;
		case CORES:
			if(!_IS_MDI_)
			{
				sprintf(str, TITLE_CORES_FMT, A->SHM->P.Top, A->SHM->C[A->SHM->P.Top].RelativeFreq);
				SetWidgetName(A, G, str);
			}
			break;
		case CSTATES:
			if(!_IS_MDI_)
			{
				sprintf(str, TITLE_CSTATES_FMT, 100 * A->SHM->P.Avg.C0, 100 * (A->SHM->P.Avg.C1 + A->SHM->P.Avg.C3 + A->SHM->P.Avg.C6 + A->SHM->P.Avg.C7));
				SetWidgetName(A, G, str);
			}
			break;
		case TEMPS:
			if(!_IS_MDI_)
			{
				sprintf(str, TITLE_TEMPS_FMT,
						A->SHM->P.Top, A->SHM->C[A->SHM->P.Hot].TjMax.Target - A->SHM->C[A->SHM->P.Hot].ThermStat.DTS);
				SetWidgetName(A, G, str);
			}
			break;
		case SYSINFO:
			if(!_IS_MDI_)
			{
				sprintf(str, TITLE_SYSINFO_FMT, A->SHM->P.ClockSpeed);
				SetWidgetName(A, G, str);
			}
			break;
	}
	free(str);
}

// Implementation of CallBack functions.
Bool32	SCHED_State(uARG *A, WBUTTON *wButton)
{
	return(A->SHM->S.Monitor);
}

Bool32	TSC_State(uARG *A, WBUTTON *wButton)
{
	return(A->SHM->P.ClockSrc == SRC_TSC);
}

Bool32	TSC_AUX_State(uARG *A, WBUTTON *wButton)
{
	return(A->SHM->P.ClockSrc == SRC_TSC_AUX);
}

Bool32	BIOS_State(uARG *A, WBUTTON *wButton)
{
	return(A->SHM->P.ClockSrc == SRC_BIOS);
}

Bool32	SPEC_State(uARG *A, WBUTTON *wButton)
{
	return(A->SHM->P.ClockSrc == SRC_SPEC);
}

Bool32	ROM_State(uARG *A, WBUTTON *wButton)
{
	return(A->SHM->P.ClockSrc == SRC_ROM);
}

Bool32	Button_State(uARG *A, WBUTTON *wButton)
{
	return( (*(wButton->WBState.Key)) );
}

void	Play(uARG *A, int G, char ID, XCHG_MAP *XChange)
{
#if defined(DEBUG)
	printf(">Play(ID[%03hhd], XID[%03hhd])\n", ID, (XChange != NULL) ? XChange->Map.ID : ID_NULL);
#endif
	switch(ID)
	{
		case ID_NORTH:
			if(A->L.Page[G].Pageable)
			{
				Bool32 CallFlush=FALSE;
				if(GetVScrolling(G) > 0)
				{
					SetVScrolling(G, GetVScrolling(G) - SCROLLED_ROWS_PER_ONCE);
					CallFlush=TRUE;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_SOUTH:
			if(A->L.Page[G].Pageable)
			{
				Bool32 CallFlush=FALSE;
				if(GetVScrolling(G) < (GetVListing(G) - GetVViewport(G)))
				{
					SetVScrolling(G, GetVScrolling(G) + SCROLLED_ROWS_PER_ONCE);
					CallFlush=TRUE;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_EAST:
			if(A->L.Page[G].Pageable)
			{
				Bool32 CallFlush=FALSE;
				if(GetHScrolling(G) < (GetHListing(G) - GetHViewport(G)))
				{
					SetHScrolling(G, GetHScrolling(G) + SCROLLED_COLS_PER_ONCE);
					CallFlush=TRUE;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_WEST:
			if(A->L.Page[G].Pageable)
			{
				Bool32 CallFlush=FALSE;
				if(GetHScrolling(G) > 0)
				{
					SetHScrolling(G, GetHScrolling(G) - SCROLLED_COLS_PER_ONCE);
					CallFlush=TRUE;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_PGUP:
			if(A->L.Page[G].Pageable)
			{
				Bool32 CallFlush=FALSE;
				if(GetVScrolling(G) > SCROLLED_ROWS_PER_PAGE)
				{
					SetVScrolling(G, GetVScrolling(G) - SCROLLED_ROWS_PER_PAGE);
					CallFlush=TRUE;
				}
				else if(GetVScrolling(G) > 0)
				{
					SetVScrolling(G, 0);
					CallFlush=TRUE;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_PGDW:
			if(A->L.Page[G].Pageable)
			{
				Bool32 CallFlush=FALSE;
				if(GetVScrolling(G) < (GetVListing(G) - GetVViewport(G) - SCROLLED_ROWS_PER_PAGE))
				{
					SetVScrolling(G, GetVScrolling(G) + SCROLLED_ROWS_PER_PAGE);
					CallFlush=TRUE;
				}
				else if(GetVScrolling(G) < (GetVListing(G) - GetVViewport(G)))
				{
					SetVScrolling(G, GetVListing(G) - GetVViewport(G));
					CallFlush=TRUE;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_PGHOME:
			if(A->L.Page[G].Pageable)
			{
				Bool32 CallFlush=FALSE;
				if(GetHScrolling(G) > 0)
				{
					SetHScrolling(G, 0);
					CallFlush=TRUE;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_PGEND:
			if(A->L.Page[G].Pageable)
			{
				Bool32 CallFlush=FALSE;
				if(GetHScrolling(G) < (GetHListing(G) - GetHViewport(G)))
				{
					SetHScrolling(G, GetHListing(G) - GetHViewport(G));
					CallFlush=TRUE;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_CTRLHOME:
			if(A->L.Page[G].Pageable)
			{
				Bool32 CallFlush=FALSE;
				if(GetHScrolling(G) > 0)
				{
					SetHScrolling(G, 0);
					CallFlush=TRUE;
				}
				if(GetVScrolling(G) > 0)
				{
					SetVScrolling(G, 0);
					CallFlush=TRUE;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_CTRLEND:
			if(A->L.Page[G].Pageable)
			{
				Bool32 CallFlush=FALSE;
				if(GetHScrolling(G) < (GetHListing(G) - GetHViewport(G)))
				{
					SetHScrolling(G, GetHListing(G) - GetHViewport(G));
					CallFlush=TRUE;
				}
				if(GetVScrolling(G) < (GetVListing(G) - GetVViewport(G)))
				{
					SetVScrolling(G, GetVListing(G) - GetVViewport(G));
					CallFlush=TRUE;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_PAUSE:
			{
				A->PAUSE[G]=!A->PAUSE[G];
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_WAIT * A->PAUSE[G]]);
			}
			break;
		case ID_STOP:
			{
				A->PAUSE[G]=TRUE;
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_WAIT]);
			}
			break;
		case ID_RESUME:
			{
				A->PAUSE[G]=FALSE;
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_DEFAULT]);
			}
			break;
		case ID_CHART:
			{
				A->L.Play.fillGraphics=!A->L.Play.fillGraphics;
			}
			break;
		case ID_FREQ:
			{
				A->L.Play.freqHertz=!A->L.Play.freqHertz;
			}
			break;
		case ID_CYCLE:
			{
				A->L.Play.showCycles=!A->L.Play.showCycles;
				BuildLayout(A, CORES);
			}
			break;
		case ID_IPS:
			{
				A->L.Play.showIPS=!A->L.Play.showIPS;
				BuildLayout(A, CORES);
			}
			break;
		case ID_IPC:
			{
				A->L.Play.showIPC=!A->L.Play.showIPC;
				BuildLayout(A, CORES);
			}
			break;
		case ID_CPI:
			{
				A->L.Play.showCPI=!A->L.Play.showCPI;
				BuildLayout(A, CORES);
			}
			break;
		case ID_RATIO:
			{
				A->L.Play.showRatios=!A->L.Play.showRatios;
				BuildLayout(A, CORES);
			}
			break;
		case ID_CSTATE:
			{
				A->L.Play.cStatePercent=!A->L.Play.cStatePercent;
			}
			break;
		case ID_WALLBOARD:
			{
				A->L.Play.wallboard=!A->L.Play.wallboard;
			}
			break;
		case ID_DUMPMSR:
		case ID_READMSR:
		case ID_WRITEMSR:
		case ID_CTLFEATURE:
		case ID_REFRESH:
		case ID_INCLOOP:
		case ID_DECLOOP:
		case ID_SCHED:
		case ID_TSC:
		case ID_TSC_AUX:
		case ID_BIOS:
		case ID_SPEC:
		case ID_ROM:
			{
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_WAIT]);

				if(XChange != NULL)
				{	// Append the Server request ID to the given parameters.
					XChange->Map.ID=ID;
					atomic_store(&A->SHM->Sync.Play, XChange->Map64);
				}
				else
				{	// No parameters supplied.
					XCHG_MAP XChange={.Map={.Addr=0, .Core=0, .Arg=0, .ID=ID}};
					atomic_store(&A->SHM->Sync.Play, XChange.Map64);
				}
				Sync_Signal(0, &A->SHM->Sync);
			}
			break;
	}
#if defined(DEBUG)
	printf("<Play(XID[%03hhd])\n", (XChange != NULL) ? XChange->Map.ID : ID_NULL);
#endif
}

char	*FQN_Settings(const char *fName)
{
	char *Home=getenv("HOME");
	if(Home != NULL)
	{
		char	*lHome=strdup(Home),
			*rHome=strdup(Home),
			*dName=dirname(lHome),
			*bName=basename(rHome),
			*FQN=malloc(4096);

		if(!strcmp(dName, "/"))
			sprintf(FQN, "%s%s/%s", dName, bName, fName);
		else
			sprintf(FQN, "%s/%s/%s", dName, bName, fName);

		free(lHome);
		free(rHome);

		return(FQN);
	}
	return(NULL);
}

Bool32	SaveSettings(uARG *A)
{
	Bool32 rc=FALSE;
	char *storePath=calloc(4096, 1);

	if((A->configFile != NULL) && strlen(A->configFile) > 0)
		strcpy(storePath, A->configFile);
	else
	{
		char *FQN=FQN_Settings(XDB_SETTINGS_FILE);
		if(FQN != NULL)
		{
			strcpy(storePath, FQN);
			free(FQN);
		}
	}
	if(strlen(storePath) > 0)
	{
		XrmDatabase xdb=XrmGetFileDatabase(storePath);

		char *strVal=calloc(256, 1), *strKey=calloc(32, 1);
		int i=0;
		for(i=1; i < OPTIONS_COUNT; i++)
			if(A->Options[i].xrmName != NULL)
			{
				switch(A->Options[i].format[1])
				{
					case 'd': {
						sprintf(strKey, "%s: %s", A->Options[i].xrmName, A->Options[i].format);
						signed int *pINT=A->Options[i].pointer;
						sprintf(strVal, strKey, *pINT);
						}
						break;
					case 'u': {
						sprintf(strKey, "%s: %s", A->Options[i].xrmName, A->Options[i].format);
						unsigned int *pUINT=A->Options[i].pointer;
						sprintf(strVal, strKey, *pUINT);
						}
						break;
					case 'X':
					case 'x': {
						sprintf(strKey, "%s: 0x%s", A->Options[i].xrmName, A->Options[i].format);
						unsigned int *pHEX=A->Options[i].pointer;
						sprintf(strVal, strKey, *pHEX);
						}
						break;
					case 'c': {
						sprintf(strKey, "%s: %s", A->Options[i].xrmName, A->Options[i].format);
						unsigned char *pCHAR=A->Options[i].pointer;
						sprintf(strVal, strKey, pCHAR);
						}
						break;
					case 's': {
					default:
						sprintf(strKey, "%s: %s", A->Options[i].xrmName, A->Options[i].format);
						char *pSTR=A->Options[i].pointer;
						sprintf(strVal, strKey, pSTR);
						}
						break;
				}
				XrmPutLineResource(&xdb, strVal);
			}
		const char *xrmClass[WIDGETS]={	XDB_CLASS_MAIN,
						XDB_CLASS_CORES,
						XDB_CLASS_CSTATES,
						XDB_CLASS_TEMPS,
						XDB_CLASS_SYSINFO,
						XDB_CLASS_DUMP};
		int G=0;
		for(G=MAIN; G < WIDGETS; G++)
		{
			sprintf(strVal, "%s.%s.%s: %d", _IS_MDI_ ? "MDI":"", xrmClass[G], XDB_KEY_LEFT, A->W[G].x);
			XrmPutLineResource(&xdb, strVal);
			sprintf(strVal, "%s.%s.%s: %d", _IS_MDI_ ? "MDI":"", xrmClass[G], XDB_KEY_TOP, A->W[G].y);
			XrmPutLineResource(&xdb, strVal);
		}
		for(i=0; i < COLOR_COUNT; i++)
		{
			sprintf(strVal, "%s.%s: 0x%lx", A->L.Colors[i].xrmClass, A->L.Colors[i].xrmKey, A->L.Colors[i].RGB);
			XrmPutLineResource(&xdb, strVal);
		}
		free(strVal);
		free(strKey);
		XrmPutFileDatabase(xdb, storePath);
		XrmDestroyDatabase(xdb);

		rc=TRUE;
	}
	else
		rc=FALSE;

	free(storePath);
	return(rc);
}

// Commands set.
void	Proc_Menu(uARG *A)
{
	Output(A, MENU_FORMAT);
}

void	Proc_Help(uARG *A)
{
	char *items=calloc(COMMANDS_COUNT, 16), *stringNL=calloc(16, 1);

	int cmd=0;
	for(cmd=1; cmd < COMMANDS_COUNT; cmd++)
	{
		sprintf(stringNL, "%s\n", A->Commands[cmd].Inst);
		strcat(items, stringNL);
	}
	Output(A, items);

	free(stringNL);
	free(items);
}

void	Proc_Release(uARG *A)
{
	Output(A, Version);
}

void	Proc_Quit(uARG *A)
{
	Output(A, "Shutting down ...\n");
	A->LOOP=FALSE;
}

void	Proc_Restart(uARG *A)
{
	Output(A, "Restarting ...\n");
	A->RESTART=TRUE;
	A->LOOP=FALSE;
}

void	Proc_History(uARG *A)
{
	char *items=calloc(HISTORY_DEPTH, KEYINPUT_DEPTH), *str=malloc(KEYINPUT_DEPTH);
	int idx=0;
	while(idx < A->L.Input.Top)
	{
		sprintf(str, "[%2d] ", idx);
		strcat(items, str);
		memcpy(str, A->L.Input.History[idx].KeyBuffer, A->L.Input.History[idx].KeyLength);
		str[A->L.Input.History[idx].KeyLength]='\n';
		str[A->L.Input.History[idx].KeyLength+1]='\0';
		strcat(items, str);
		idx++;
	}
	Output(A, items);
	free(str);
	free(items);
}

void	Get_Color(uARG *A, int cmd)
{
	int idx=0;

	if((sscanf(&A->L.Input.KeyBuffer[strlen(A->Commands[cmd].Inst)], A->Commands[cmd].Spec, &idx) == 1) && (idx < COLOR_COUNT))
	{
		char stringNL[16]={0};

		sprintf(stringNL, "0x%lx\n", A->L.Colors[idx].RGB);
		Output(A, stringNL);
	}
	else
		Output(A,	"Usage: get color p1\n"	\
				"Where: p1=index (Int)\n");
}

void	Set_Color(uARG *A, int cmd)
{
	unsigned long int RGB=0;
	int idx=0;

	if((sscanf(&A->L.Input.KeyBuffer[strlen(A->Commands[cmd].Inst)], A->Commands[cmd].Spec, &idx, &RGB) == 2) && (idx < COLOR_COUNT))
	{
		A->L.Colors[idx].RGB=RGB;
	}
	else
		Output(A,	"Usage: set color p1 p2\n"	\
				"Where: p1=index (Int), p2=RGB (Hex)\n");
}

void	Set_Font(uARG *A, int cmd)
{
	if(sscanf(&A->L.Input.KeyBuffer[strlen(A->Commands[cmd].Inst)], A->Commands[cmd].Spec, A->fontName) != 1)
		Output(A,	"Usage: set font p1\n"	\
				"Where: p1=font name (String)\n");
}

void	Svr_Dump_MSR(uARG *A, int cmd)
{
	XCHG_MAP XChange={.Map={.Addr=0, .Core=0, .Arg=0, .ID=ID_NULL}};

	if((sscanf(&A->L.Input.KeyBuffer[strlen(A->Commands[cmd].Inst)], A->Commands[cmd].Spec, &XChange.Map.Addr, &XChange.Map.Core, &XChange.Map.Arg) == 3)
	&& (XChange.Map.Core < A->SHM->P.CPU) && (XChange.Map.Core >= 0) && (XChange.Map.Arg < DUMP_ARRAY_DIMENSION) && (XChange.Map.Arg >=0))
	{
		Play(A, MAIN, ID_DUMPMSR, &XChange);
	}
	else
		Output(A,	"Usage: dump msr p1 p2 p3\n"	\
				"Where: p1=address (Hex), p2=Core# (Int), p3=index (Int)\n");
}

void	Svr_Read_MSR(uARG *A, int cmd)
{
	XCHG_MAP XChange={.Map={.Addr=0, .Core=0, .Arg=0, .ID=ID_NULL}};

	if((sscanf(&A->L.Input.KeyBuffer[strlen(A->Commands[cmd].Inst)], A->Commands[cmd].Spec, &XChange.Map.Addr, &XChange.Map.Core) == 2)
	&& (XChange.Map.Core < A->SHM->P.CPU) && (XChange.Map.Core >= 0))
	{
		Play(A, MAIN, ID_READMSR, &XChange);
	}
	else
		Output(A,	"Usage: read msr p1 p2\n"	\
				"Where: p1=address (Hex), p2=Core# (Int)\n");
}

void	Svr_Write_MSR(uARG *A, int cmd)
{
	XCHG_MAP XChange={.Map={.Addr=0, .Core=0, .Arg=0, .ID=ID_NULL}};
	unsigned long long int data=0;

	if((sscanf(&A->L.Input.KeyBuffer[strlen(A->Commands[cmd].Inst)], A->Commands[cmd].Spec, &XChange.Map.Addr, &XChange.Map.Core, &data) == 3)
	&& (XChange.Map.Core < A->SHM->P.CPU) && (XChange.Map.Core >= 0))
	{
		atomic_store(&A->SHM->Sync.Data, data);
		Play(A, MAIN, ID_WRITEMSR, &XChange);
	}
	else
		Output(A,	"Usage: write msr p1 p2 p3\n"	\
				"Where: p1=address (Hex), p2=Core# (Int), p3=value (Hex)\n");
}

unsigned int Ctl_Feature_Transcode(char *pStr)
{
	CTL_FEATURES CtlList[]=FEATURES_LIST, *pCtl=CtlList;

	while(pCtl->Inst != NULL)
		if(!strncmp(pStr, pCtl->Inst, strlen(pCtl->Inst)))
			break;
		else
			pCtl++;
	return(pCtl->ID);
}

void	Ctl_Feature_Help(char *items)
{
	CTL_FEATURES CtlList[]=FEATURES_LIST, *pCtl=CtlList;

	while(pCtl->Inst != NULL)
	{
		strcat(items, " ");
		strcat(items, pCtl->Inst);
		strcat(items, " ");
		pCtl++;
	}
}

void	Svr_Enable_Feature(uARG *A, int cmd)
{
	XCHG_MAP XChange={.Map={.Addr=0, .Core=0, .Arg=0, .ID=ID_NULL}};
	char *str=malloc(16);
	Bool32 noerr=TRUE;

	if((sscanf(&A->L.Input.KeyBuffer[strlen(A->Commands[cmd].Inst)], A->Commands[cmd].Spec, str) == 1) && (str != NULL))
	{
		XChange.Map.Arg=CTL_ENABLE;

		if((XChange.Map.Addr=Ctl_Feature_Transcode(str)))
			Play(A, MAIN, ID_CTLFEATURE, &XChange);
		else
			noerr=FALSE;
	}
	else noerr=FALSE;

	if(noerr != TRUE)
	{
		char *items=calloc(80,1);
		strcpy(items,	"Usage: enable p1\n"		\
				"Where: p1=feature (String)\n"	\
				"and  : feature={");
		Ctl_Feature_Help(items);
		strcat(items, "}\n");
		Output(A, items);
		free(items);
	}
	free(str);
}

void	Svr_Disable_Feature(uARG *A, int cmd)
{
	XCHG_MAP XChange={.Map={.Addr=0, .Core=0, .Arg=0, .ID=ID_NULL}};
	char *str=malloc(16);
	Bool32 noerr=TRUE;

	if((sscanf(&A->L.Input.KeyBuffer[strlen(A->Commands[cmd].Inst)], A->Commands[cmd].Spec, str) == 1) && (str != NULL))
	{
		XChange.Map.Arg=CTL_DISABLE;

		if((XChange.Map.Addr=Ctl_Feature_Transcode(str)))
			Play(A, MAIN, ID_CTLFEATURE, &XChange);
		else
			noerr=FALSE;
	}
	else noerr=FALSE;

	if(noerr != TRUE)
	{
		char *items=calloc(80,1);
		strcpy(items,	"Usage: disable p1\n"	\
				"Where: p1=feature (String)\n"	\
				"and  : feature={");
		Ctl_Feature_Help(items);
		strcat(items, "}\n");
		Output(A, items);
		free(items);
	}
	free(str);
}

// Process the commands language.
Bool32	ExecCommand(uARG *A)
{
	int cmd=0;
	for(cmd=0; cmd < COMMANDS_COUNT; cmd++)
		if(strncmp(A->L.Input.KeyBuffer, A->Commands[cmd].Inst, strlen(A->Commands[cmd].Inst)) == 0)
			break;

	if(cmd < COMMANDS_COUNT)
	{
		A->Commands[cmd].Proc(A, cmd);
		return(TRUE);
	}
	else
		return(FALSE);
}

void	CallBackButton(uARG *A, WBUTTON *wButton)
{
	Play(A, wButton->Target, wButton->ID, NULL);
}

void	CallBackTemps(uARG *A, WBUTTON *wButton)
{
	int cpu=atoi(wButton->Resource.Text);

	A->L.Play.showTemps[cpu]=!A->L.Play.showTemps[cpu];
}

void	CallBackSave(uARG *A, WBUTTON *wButton)
{
	if(SaveSettings(A))
		Output(A, "Settings successfully saved.\n");
	else
		Output(A, "Failed to save settings.\n");
	fDraw(MAIN, TRUE, FALSE);
}

void	CallBackQuit(uARG *A, WBUTTON *wButton)
{
	Proc_Quit(A);
	fDraw(MAIN, TRUE, FALSE);
}
void	CallBackMinimizeWidget(uARG *A, WBUTTON *wButton)
{
	MinimizeWidget(A, wButton->Target);
}

void	CallBackRestoreWidget(uARG *A, WBUTTON *wButton)
{
	RestoreWidget(A, wButton->Target);
}

void	Server(uARG *A, int G, XCHG_MAP *XChange)
{
#if defined(DEBUG)
	if(XChange != NULL) printf(">Server(XID[%03hhd], Arg[%03hhd])\n", XChange->Map.ID, XChange->Map.Arg);
#endif
	switch(XChange->Map.Arg)
	{
		case ID_READMSR:
			if(G == MAIN)
			{
				unsigned long long int decVal=atomic_load(&A->SHM->Sync.Data);

				char hexStr[DUMP_HEX16_STR], binStr[DUMP_BIN64_STR],
					fullStr[20 + 1 + 1 + DUMP_HEX16_STR + 7 + 1 + 1 + 1 + DUMP_BIN64_STR + 15 + 1 + 1];

				DumpRegister(decVal, hexStr, binStr);
				sprintf(fullStr, "%llu (", decVal);

				int H=0;
				for(H=0; H < 7; H++)
				{
					strncat(fullStr, &hexStr[H << 1], 2);
					strcat(fullStr, " ");
				}
				strncat(fullStr, &hexStr[H << 1], 2);
				strcat(fullStr, ")\n[");

				for(H=0; H < 15; H++)
				{
					strncat(fullStr, &binStr[H << 2], 4);
					strcat(fullStr, " ");
				};
				strncat(fullStr, &binStr[H << 2], 4);
				strcat(fullStr, "]\n");

				Output(A, fullStr);
				fDraw(G, TRUE, FALSE);
			}
		// No break
		case ID_WRITEMSR:
		case ID_CTLFEATURE:
			if(G == MAIN)
			{
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_DEFAULT]);

				XChange->Map.Arg=ID_NULL;
			}
			break;
		case ID_REFRESH:
				fDraw(SYSINFO, FALSE, TRUE);
		case ID_DUMPMSR:
		case ID_INCLOOP:
		case ID_DECLOOP:
		case ID_SCHED:
		case ID_TSC:
		case ID_TSC_AUX:
		case ID_BIOS:
		case ID_SPEC:
		case ID_ROM:
			{
				int g=0;
				for(g=MAIN; g < WIDGETS; g++)
					XDefineCursor(A->display, A->W[g].window, A->MouseCursor[MC_DEFAULT]);
			}
		// No break
		case ID_DONE:
				XChange->Map.Arg=ID_NULL;
		break;
	}
#if defined(DEBUG)
	if(XChange != NULL) printf("<Server(XID[%03hhd], Arg[%03hhd])\n", XChange->Map.ID, XChange->Map.Arg);
#endif
}

// The far drawing procedure which paints the foreground.
static void *uDraw(void *uArg)
{
	uARG *A=(uARG *) uArg;
	pthread_setname_np(A->TID_Draw, "xfreq-gui-draw");

	long int idleRemaining;
	while(A->LOOP)
		if((idleRemaining=Sync_Wait(A->Room, &A->SHM->Sync, IDLE_COEF_MAX + IDLE_COEF_DEF + IDLE_COEF_MIN)))
		{
			XCHG_MAP XChange={.Map64=atomic_load(&A->SHM->Sync.Play)};
			int G=0;
			for(G=MAIN; G < WIDGETS; G++)
			{
				if(!A->PAUSE[G])
					fDraw(G, (XChange.Map.ID == ID_DONE) ? TRUE : FALSE, TRUE);

				if(!(A->L.UnMapBitmask & (1 << G)))
					UpdateWidgetName(A, G);

				if((XChange.Map.ID == ID_DONE) && (XChange.Map.Arg != ID_NULL)) {
					Server(A, G, &XChange);
					atomic_store(&A->SHM->Sync.Play, XChange.Map64);
				}
			}
		}
	else
		Proc_Quit(A);
	return(NULL);
}

// the main thread which manages the X-Window events loop.
static void *uLoop(uARG *A)
{
	XEvent	E={0};

	while(A->LOOP)
	{
		XNextEvent(A->display, &E);

		int G=0;
		for(G=MAIN; G < WIDGETS; G++)
			if(E.xany.window == A->W[G].window)
				break;

		if(G < WIDGETS)
		    switch(E.type)
		    {
			case Expose: {
				if(!E.xexpose.count)
					FlushLayout(A, G);
			}
				break;
			case ClientMessage:
				if(E.xclient.data.l[0] == A->atom[0])
				{
					Proc_Quit(A);
					fDraw(MAIN, TRUE, FALSE);
				}
				break;
			case KeyPress:
			{
				KeySym	KeySymPressed;
				XComposeStatus ComposeStatus={0};
				char xkBuffer[KEYINPUT_DEPTH];
				int  xkLength;

				if((xkLength=XLookupString(&E.xkey,
							xkBuffer,
							KEYINPUT_DEPTH,
							&KeySymPressed,
							&ComposeStatus)) )
					if(!(E.xkey.state & AllModMask)
					&& (	((KeySymPressed >= XK_space) && (KeySymPressed <= XK_asciitilde))
					||	((KeySymPressed >= XK_KP_0) && (KeySymPressed <= XK_KP_9))	)
					&& ((A->L.Input.KeyLength + xkLength) < (KEYINPUT_DEPTH - 1))
					&& (G == MAIN))
					{
						if(A->L.Input.KeyInsert == A->L.Input.KeyLength)
						{
							memcpy(&A->L.Input.KeyBuffer[A->L.Input.KeyLength], xkBuffer, xkLength);
							A->L.Input.KeyInsert=A->L.Input.KeyLength+=xkLength;
						}
						else {
							memmove(&A->L.Input.KeyBuffer[A->L.Input.KeyInsert + 1],
								&A->L.Input.KeyBuffer[A->L.Input.KeyInsert],
								A->L.Input.KeyLength - A->L.Input.KeyInsert);
							memcpy(&A->L.Input.KeyBuffer[A->L.Input.KeyInsert], xkBuffer, xkLength);
							A->L.Input.KeyInsert+=xkLength;
							A->L.Input.KeyLength+=xkLength;
						}
						A->L.Input.Expand.KeyLength=0;

						fDraw(MAIN, FALSE, TRUE);
					}
				switch(KeySymPressed)
				{
					case XK_Escape:
						if((G == MAIN) && (A->L.Input.KeyLength > 0))
						{
							A->L.Input.Expand.KeyLength=A->L.Input.KeyInsert=A->L.Input.KeyLength=0;
							A->L.Input.Browse=A->L.Input.Recent;

							fDraw(MAIN, FALSE, TRUE);
						}
						break;
					case XK_Delete:
					case XK_KP_Delete:
						if((G == MAIN) && (A->L.Input.KeyLength > 0) && (A->L.Input.KeyInsert < A->L.Input.KeyLength))
						{
							memmove(&A->L.Input.KeyBuffer[A->L.Input.KeyInsert],
								&A->L.Input.KeyBuffer[A->L.Input.KeyInsert + 1],
								A->L.Input.KeyLength - A->L.Input.KeyInsert);
							A->L.Input.KeyLength--;
							A->L.Input.Expand.KeyLength=0;

							fDraw(MAIN, FALSE, TRUE);
						}
						break;
					case XK_BackSpace:
						if((G == MAIN) && (A->L.Input.KeyInsert > 0))
						{
							if(A->L.Input.KeyInsert < A->L.Input.KeyLength)
							{
								memmove(&A->L.Input.KeyBuffer[A->L.Input.KeyInsert - 1],
									&A->L.Input.KeyBuffer[A->L.Input.KeyInsert],
									A->L.Input.KeyLength - A->L.Input.KeyInsert);
							}
							A->L.Input.KeyInsert--;
							A->L.Input.KeyLength--;
							A->L.Input.Expand.KeyLength=0;

							fDraw(MAIN, FALSE, TRUE);
						}
						break;
					case XK_Return:
					case XK_KP_Enter:
						if((G == MAIN) && (A->L.Input.KeyLength > 0))
						{
							A->L.Input.KeyBuffer[A->L.Input.KeyLength]='\0';
							Output(A, A->L.Input.KeyBuffer);
							Output(A, "\n");
							{
								A->L.Input.History[A->L.Input.Recent].KeyLength=A->L.Input.KeyLength;
								memcpy(A->L.Input.History[A->L.Input.Recent].KeyBuffer,
									A->L.Input.KeyBuffer,
									A->L.Input.History[A->L.Input.Recent].KeyLength);
								if(A->L.Input.Top < (HISTORY_DEPTH - 1))
									A->L.Input.Top++;
								A->L.Input.Browse=A->L.Input.Recent=(A->L.Input.Recent < A->L.Input.Top) ? A->L.Input.Recent + 1 : 0;
							}
							if(!ExecCommand(A))
								Output(A, "Syntax Error\n");

							A->L.Input.Expand.KeyLength=A->L.Input.KeyInsert=A->L.Input.KeyLength=0;

							fDraw(MAIN, TRUE, FALSE);
						}
						break;
					case XK_F12:
						if((G == MAIN) && (A->L.Input.KeyLength > 0))
						{
							long int idx=-1;
							char *endptr;
							A->L.Input.KeyBuffer[A->L.Input.KeyLength]='\0';
							idx=strtol(A->L.Input.KeyBuffer, &endptr, 10);
							if((*endptr == '\0') && (idx < A->L.Input.Top))
							{
								A->L.Input.KeyInsert=A->L.Input.KeyLength=A->L.Input.History[idx].KeyLength;
								memcpy(A->L.Input.KeyBuffer, A->L.Input.History[idx].KeyBuffer, A->L.Input.KeyLength);
							}
							fDraw(MAIN, FALSE, TRUE);
						}
						break;
					case XK_Tab:
						if((G == MAIN) && (A->L.Input.KeyLength > 0))
						{
							if(A->L.Input.Expand.KeyLength > 0)
							{
								A->L.Input.KeyInsert=A->L.Input.KeyLength=A->L.Input.Expand.KeyLength;
								memcpy(A->L.Input.KeyBuffer, A->L.Input.Expand.KeyBuffer, A->L.Input.KeyLength);
							}
							int cmd=0;
							for(cmd=0; cmd < COMMANDS_COUNT; cmd++)
							{
								A->L.Input.Cmd=(A->L.Input.Cmd < (COMMANDS_COUNT - 1)) ? A->L.Input.Cmd + 1 : 0;
								if(strncmp(A->L.Input.KeyBuffer, A->Commands[A->L.Input.Cmd].Inst, A->L.Input.KeyLength) == 0)
								{
									A->L.Input.Expand.KeyLength=A->L.Input.KeyLength;
									memcpy(A->L.Input.Expand.KeyBuffer, A->L.Input.KeyBuffer, A->L.Input.Expand.KeyLength);
									A->L.Input.KeyInsert=A->L.Input.KeyLength=strlen(A->Commands[A->L.Input.Cmd].Inst);
									memcpy(A->L.Input.KeyBuffer, A->Commands[A->L.Input.Cmd].Inst, A->L.Input.KeyLength);
									break;
								}
							}
							if(cmd == COMMANDS_COUNT)
								A->L.Input.Cmd=-1;
							else
								fDraw(MAIN, FALSE, TRUE);
						}
						break;
					case XK_Pause:
						if(G != MAIN)
							Play(A, G, ID_PAUSE, NULL);
						break;
					case XK_l:
					case XK_L:
						// Draw the Widget once.
						if(E.xkey.state & ControlMask)
						{
							if(G == SYSINFO)
								Play(A, G, ID_REFRESH, NULL);
							else
								fDraw(G, FALSE, TRUE);
						}
						break;
					case XK_p:
					case XK_P:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_CSTATE, NULL);
						break;
					case XK_q:
					case XK_Q:
						if(E.xkey.state & ControlMask)
						{
							Proc_Quit(A);
							fDraw(MAIN, TRUE, FALSE);
						}
						break;
					case XK_r:
					case XK_R:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_RATIO, NULL);
						break;
					case XK_t:
					case XK_T:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_SCHED, NULL);
						break;
					case XK_w:
					case XK_W:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_WALLBOARD, NULL);
						break;
					case XK_y:
					case XK_Y:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_CYCLE, NULL);
						break;
					case XK_z:
					case XK_Z:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_FREQ, NULL);
						break;
					case XK_Up:
					case XK_KP_Up:
						if(E.xkey.state & ShiftMask)
							Play(A, G, ID_NORTH, NULL);
						else
							if((G == MAIN) && (A->L.Input.Top > 0))
							{
								A->L.Input.Browse=(A->L.Input.Browse > 0) ? A->L.Input.Browse - 1 : (A->L.Input.Top - 1);
								A->L.Input.KeyInsert=A->L.Input.KeyLength=A->L.Input.History[A->L.Input.Browse].KeyLength;
								memcpy(A->L.Input.KeyBuffer, A->L.Input.History[A->L.Input.Browse].KeyBuffer, A->L.Input.KeyLength);
								A->L.Input.Expand.KeyLength=0;

								fDraw(MAIN, FALSE, TRUE);
							}
						break;
					case XK_Down:
					case XK_KP_Down:
						if(E.xkey.state & ShiftMask)
							Play(A, G, ID_SOUTH, NULL);
						else
							if((G == MAIN) && (A->L.Input.Top > 0))
							{
								A->L.Input.Browse=(A->L.Input.Browse < (A->L.Input.Top - 1)) ? A->L.Input.Browse + 1 : 0;
								A->L.Input.KeyInsert=A->L.Input.KeyLength=A->L.Input.History[A->L.Input.Browse].KeyLength;
								memcpy(A->L.Input.KeyBuffer, A->L.Input.History[A->L.Input.Browse].KeyBuffer, A->L.Input.KeyLength);
								A->L.Input.Expand.KeyLength=0;

								fDraw(MAIN, FALSE, TRUE);
							}
						break;
					case XK_Right:
					case XK_KP_Right:
						if(E.xkey.state & ShiftMask)
							Play(A, G, ID_EAST, NULL);
						else
							if((G == MAIN) && (A->L.Input.KeyInsert < A->L.Input.KeyLength))
							{
								A->L.Input.KeyInsert++;
								A->L.Input.Expand.KeyLength=0;

								fDraw(MAIN, FALSE, TRUE);
							}
						break;
					case XK_Left:
					case XK_KP_Left:
						if(E.xkey.state & ShiftMask)
							Play(A, G, ID_WEST, NULL);
						else
							if((G == MAIN) && (A->L.Input.KeyInsert > 0))
							{
								A->L.Input.KeyInsert--;
								A->L.Input.Expand.KeyLength=0;

								fDraw(MAIN, FALSE, TRUE);
							}
						break;
					case XK_Page_Up:
					case XK_KP_Page_Up:
						if(E.xkey.state & ShiftMask)
							Play(A, G, ID_PGUP, NULL);
						break;
					case XK_Page_Down:
					case XK_KP_Page_Down:
						if(E.xkey.state & ShiftMask)
							Play(A, G, ID_PGDW, NULL);
						break;
					case XK_Home:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_CTRLHOME, NULL);
						else if(E.xkey.state & ShiftMask)
							Play(A, G, ID_PGHOME, NULL);
						else if(G == MAIN)
						{
							A->L.Input.KeyInsert=0;
							A->L.Input.Expand.KeyLength=0;

							fDraw(MAIN, FALSE, TRUE);
						}
						break;
					case XK_End:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_CTRLEND, NULL);
						else if(E.xkey.state & ShiftMask)
							Play(A, G, ID_PGEND, NULL);
						else if(G == MAIN)
						{
							A->L.Input.KeyInsert=A->L.Input.KeyLength;
							A->L.Input.Expand.KeyLength=0;

							fDraw(MAIN, FALSE, TRUE);
						}
						break;
					case XK_KP_Add:
						Play(A, G, ID_DECLOOP, NULL);
						break;
					case XK_KP_Subtract:
						Play(A, G, ID_INCLOOP, NULL);
						break;
					case XK_F1:
					{
						Proc_Menu(A);
						fDraw(MAIN, TRUE, FALSE);
					}
						break;
					case XK_F2:
					case XK_F3:
					case XK_F4:
					case XK_F5:
					case XK_F6:
					{
						// Convert the function key number into a Widget index.
						int T=XLookupKeysym(&E.xkey, 0) - XK_F1;
						// Get the Map state.
						XWindowAttributes xwa={0};
						XGetWindowAttributes(A->display, A->W[T].window, &xwa);
						// Hide or unhide the Widget.
						switch(xwa.map_state)
						{
							case IsViewable:
								MinimizeWidget(A, T);
								break;
							case IsUnmapped:
								RestoreWidget(A, T);
								break;
						}
					}
						break;
				}
			}
				break;
			case ButtonPress:
				switch(E.xbutton.button)
				{
					case Button1:
						WidgetButtonPress(A, G, &E);
						break;
					case Button3:
						MoveWidget(A, &E);
						break;
					case Button4:
					{
						int T=G;
						if(_IS_MDI_)
						{
							for(T=LAST_WIDGET; T >= FIRST_WIDGET; T--)
								if( E.xbutton.subwindow == A->W[T].window)
									break;
						}
						Play(A, T, ID_NORTH, NULL);
						break;
					}
					case Button5:
					{
						int T=G;
						if(_IS_MDI_)
						{
							for(T=LAST_WIDGET; T >= FIRST_WIDGET; T--)
								if( E.xbutton.subwindow == A->W[T].window)
									break;
						}
						Play(A, T, ID_SOUTH, NULL);
						break;
					}
				}
				break;
			case ButtonRelease:
				switch(E.xbutton.button)
				{
					case Button3:
						MoveWidget(A, &E);
						break;
				}
				break;
			case MotionNotify:
				if(E.xmotion.state & Button3Mask)
					MoveWidget(A, &E);
				if(E.xmotion.state & Button1Mask)
					WidgetButtonPress(A, G, &E);
				break;
			case ConfigureNotify:
				A->W[G].x=E.xconfigure.x;
				A->W[G].y=E.xconfigure.y;
				A->W[G].width=E.xconfigure.width;
				A->W[G].height=E.xconfigure.height;
				A->W[G].border_width=E.xconfigure.border_width;
				break;
			case FocusIn:
				XSetWindowBorder(A->display, A->W[G].window, A->L.Colors[COLOR_FOCUS].RGB);
				break;
			case FocusOut:
				XSetWindowBorder(A->display, A->W[G].window, A->W[G].foreground);
				break;
			case DestroyNotify:
				A->LOOP=FALSE;
				break;
			case UnmapNotify:
			{
				if(G != MAIN)
					A->PAUSE[G]=TRUE;

				IconifyWidget(A, G, &E);
				fDraw(MAIN, TRUE, FALSE);
			}
				break;
			case MapNotify:
			{
				if(G != MAIN)
					A->PAUSE[G]=FALSE;

				IconifyWidget(A, G, &E);
				fDraw(MAIN, TRUE, FALSE);
			}
				break;
/*			case VisibilityNotify:
				switch (E.xvisibility.state)
				{
					case VisibilityUnobscured:
						break;
					case VisibilityPartiallyObscured:
					case VisibilityFullyObscured:
						if(A->L.Play.alwaysOnTop)
							XRaiseWindow(A->display, A->W[G].window);
						break;
				}
				break;	*/
		    }
	}
	return(NULL);
}

// Load settings
int	LoadSettings(uARG *A, int argc, char *argv[])
{
	XrmDatabase xdb=NULL;
	char *xtype=NULL;
	XrmValue xvalue={0, NULL};
	char xrmKey[32]={0};
	int i=0, j=0, G=0;
	Bool32 noerr=TRUE;

	// Is the option '-C' specified ?
	if((argc >= 3) && !strcmp(argv[1], A->Options[0].argument))
		sscanf(argv[2], A->Options[0].format, A->Options[0].pointer);
	// then use it as the settings configuration file.
	if((A->configFile != NULL) && (strlen(A->configFile) > 0))
		xdb=XrmGetFileDatabase(A->configFile);
	else
	{
		char *FQN=FQN_Settings(XDB_SETTINGS_FILE);
		if(FQN != NULL)
		{
			xdb=XrmGetFileDatabase(FQN);
			free(FQN);
		}
	}
	if(xdb != NULL)
	{
		for(i=0; i < OPTIONS_COUNT; i++)
			if((A->Options[i].xrmName != NULL)
			 && XrmGetResource(xdb, A->Options[i].xrmName, NULL, &xtype, &xvalue))
				sscanf(xvalue.addr, A->Options[i].format, A->Options[i].pointer);

		for(i=0; i < COLOR_COUNT; i++)
		{
			sprintf(xrmKey, "%s.%s", A->L.Colors[i].xrmClass, A->L.Colors[i].xrmKey);
			if(XrmGetResource(xdb, xrmKey, NULL, &xtype, &xvalue))
				sscanf(xvalue.addr, "%lx", &A->L.Colors[i].RGB);
		}
	}
	//  Parse the command line options which may override settings.
	if( (argc - ((argc >> 1) << 1)) )
	{
		for(j=(A->configFile != NULL) ? 3 : 1; j < argc; j+=2)
		{	// Count from 1 to ignore scanning the '-C option.
			for(i=1; i < OPTIONS_COUNT; i++)
				if(!strcmp(argv[j], A->Options[i].argument))
				{
					sscanf(argv[j+1], A->Options[i].format, A->Options[i].pointer);
					break;
				}
			if(i == OPTIONS_COUNT)
			{
				noerr=FALSE;
				break;
			}
		}
		if(noerr)
		{
			if(!xdb)
			{
				Bool32 noGeometries=(A->Geometries == NULL);
				if(noGeometries)
					A->Geometries=calloc(WIDGETS, GEOMETRY_SIZE + 1);

				for(G=MAIN; G < WIDGETS; G++)
				{
					if(noGeometries)
					{
						char geometry[GEOMETRY_SIZE + 1];

						sprintf(geometry, GEOMETRY_FORMAT, G,
								A->L.Page[G].Geometry.cols,
								A->L.Page[G].Geometry.rows,
								A->W[G].x,
								A->W[G].y);
						strcat(A->Geometries, geometry);
					}
					// Override the compiled colors with the specified arguments.
					if(A->L.globalBackground != _BACKGROUND_GLOBAL)
						A->W[G].background=A->L.globalBackground;

					if(A->L.globalForeground != _FOREGROUND_GLOBAL)
						A->W[G].foreground=A->L.globalForeground;
				}
			} else {
				const char *xrmClass[WIDGETS]={	XDB_CLASS_MAIN,
								XDB_CLASS_CORES,
								XDB_CLASS_CSTATES,
								XDB_CLASS_TEMPS,
								XDB_CLASS_SYSINFO,
								XDB_CLASS_DUMP};
				for(G=MAIN; G < WIDGETS; G++)
				{
					sprintf(xrmKey, "%s.%s.%s", _IS_MDI_ ? "MDI":"", xrmClass[G], XDB_KEY_LEFT);
					if(XrmGetResource(xdb, xrmKey, NULL, &xtype, &xvalue))
						sscanf(xvalue.addr, "%d", &A->W[G].x);

					sprintf(xrmKey, "%s.%s.%s", _IS_MDI_ ? "MDI":"", xrmClass[G], XDB_KEY_TOP);
					if(XrmGetResource(xdb, xrmKey, NULL, &xtype, &xvalue))
						sscanf(xvalue.addr, "%d", &A->W[G].y);

					//  Override or use loaded colors.
					if(A->L.globalBackground != _BACKGROUND_GLOBAL)
						A->W[G].background=A->L.Colors[(G << 1) + 0].RGB=A->L.globalBackground;
					else
						A->W[G].background=A->L.Colors[(G << 1) + 0].RGB;

					if(A->L.globalForeground != _FOREGROUND_GLOBAL)
						A->W[G].foreground=A->L.Colors[(G << 1) + 1].RGB=A->L.globalForeground;
					else
						A->W[G].foreground=A->L.Colors[(G << 1) + 1].RGB;
				}
			}
		}
	}
	else
		noerr=FALSE;

	if(xdb != NULL)
		XrmDestroyDatabase(xdb);

	if(noerr == FALSE) {
		char	*program=strdup(argv[0]),
			*progName=basename(program);

		if(!strcmp(argv[1], "-h"))
		{
			printf(	_APPNAME" ""usage:\n\t%s [-option argument] .. [-option argument]\n\nwhere options include:\n", progName);

			for(i=0; i < OPTIONS_COUNT; i++)
				printf("\t%s\t%s\n", A->Options[i].argument, A->Options[i].manual);

			printf(	"\t-v\tPrint version information\n" \
				"\t-h\tPrint out this message\n" \
				"\nExit status:\n0\tif OK,\n1\tif problems,\n2\tif serious trouble.\n" \
				"\nReport bugs to xfreq[at]cyring.fr\n");
		}
		else if(!strcmp(argv[1], "-v"))
			printf(Version);
		else
		{
			if(j > 0)
				printf("%s: unrecognized option '%s'\nTry '%s -h' for more information.\n", progName, argv[j], progName);
			else
				printf("%s: malformed options.\nTry '%s -h' for more information.\n", progName, progName);
		}
		free(program);
	}
	return(noerr);
}

static void *uEmergency(void *uArg)
{
	uARG *A=(uARG *) uArg;
	pthread_setname_np(A->TID_SigHandler, "xfreq-gui-kill");

	int caught=0;
	while(A->LOOP && !sigwait(&A->Signal, &caught))
		switch(caught)
		{
			case SIGINT:
			case SIGQUIT:
			case SIGUSR1:
			case SIGTERM:
			{
				char str[sizeof(SIG_EMERGENCY_FMT)];
				sprintf(str, SIG_EMERGENCY_FMT, caught);
				tracerr(str);
				A->LOOP=FALSE;
			}
				break;
			case SIGCONT:
			{
				int G=0;
				for(G=FIRST_WIDGET; G <= LAST_WIDGET; G++)
					Play(A, G, ID_RESUME, NULL);
			}
				break;
			case SIGUSR2:
//			case SIGTSTP:
			{
				int G=0;
				for(G=FIRST_WIDGET; G <= LAST_WIDGET; G++)
					Play(A, G, ID_STOP, NULL);
			}
		}
	return(NULL);
}

// Verify the prerequisites & start the threads.
int main(int argc, char *argv[])
{
	pthread_setname_np(pthread_self(), "xfreq-gui-main");

	uARG	A={
			.FD={.Shm=0, .SmBIOS=0}, .SHM=MAP_FAILED, .SmBIOS=MAP_FAILED,
			.display=NULL,
			.screen=NULL,
			.TID_Draw=0,
			.Geometries=NULL,
			.fontName=calloc(256, sizeof(char)),
			.xfont=NULL,
			.MouseCursor={0},
			.Splash={.window=0, .gc=0, .x=0, .y=0, .w=splash_width + (splash_width >> 2), .h=splash_height << 1, .attributes=0x0014},
			.W={
				// MAIN
				{
				.window=0,
				.pixmap={.B=0, .F=0},
				.gc=0,
				.x=+0,
				.y=+0,
				.width=(GEOMETRY_MAIN_COLS * DEFAULT_FONT_CHAR_WIDTH),
				.height=((1 + GEOMETRY_MAIN_ROWS + 1) * DEFAULT_FONT_CHAR_HEIGHT),
				.border_width=1,
				.extents={
					.overall={0},
					.dir=0,
					.ascent=DEFAULT_FONT_ASCENT,
					.descent=DEFAULT_FONT_DESCENT,
					.charWidth=DEFAULT_FONT_CHAR_WIDTH,
					.charHeight=DEFAULT_FONT_CHAR_HEIGHT,
				},
				.background=_BACKGROUND_MAIN,
				.foreground=_FOREGROUND_MAIN,
				.wButton={NULL},
				},
				// CORES
				{
				.window=0,
				.pixmap={.B=0, .F=0},
				.gc=0,
				.x=+DEFAULT_FONT_CHAR_WIDTH,
				.y=+545,
				.width=(((GEOMETRY_CORES_COLS << 1) + 4 + 1) * DEFAULT_FONT_CHAR_WIDTH),
				.height=((2 + GEOMETRY_CORES_ROWS + 2) * DEFAULT_FONT_CHAR_HEIGHT),
				.border_width=1,
				.extents={
					.overall={0},
					.dir=0,
					.ascent=DEFAULT_FONT_ASCENT,
					.descent=DEFAULT_FONT_DESCENT,
					.charWidth=DEFAULT_FONT_CHAR_WIDTH,
					.charHeight=DEFAULT_FONT_CHAR_HEIGHT,
				},
				.background=_BACKGROUND_CORES,
				.foreground=_FOREGROUND_CORES,
				.wButton={NULL},
				},
				// CSTATES
				{
				.window=0,
				.pixmap={.B=0, .F=0},
				.gc=0,
				.x=+750,
				.y=+545,
				.width=((((GEOMETRY_CSTATES_COLS * CSTATES_TEXT_SPACING) << 1) + 2) * DEFAULT_FONT_CHAR_WIDTH),
				.height=((2 + GEOMETRY_CSTATES_ROWS + 2) * DEFAULT_FONT_CHAR_HEIGHT),
				.border_width=1,
				.extents={
					.overall={0},
					.dir=0,
					.ascent=DEFAULT_FONT_ASCENT,
					.descent=DEFAULT_FONT_DESCENT,
					.charWidth=DEFAULT_FONT_CHAR_WIDTH,
					.charHeight=DEFAULT_FONT_CHAR_HEIGHT,
				},
				.background=_BACKGROUND_CSTATES,
				.foreground=_FOREGROUND_CSTATES,
				.wButton={NULL},
				},
				// TEMPS
				{
				.window=0,
				.pixmap={.B=0, .F=0},
				.gc=0,
				.x=+400,
				.y=+545,
				.width=((GEOMETRY_TEMPS_COLS + 6) * DEFAULT_FONT_CHAR_WIDTH),
				.height=((2 + GEOMETRY_TEMPS_ROWS + 2) * DEFAULT_FONT_CHAR_HEIGHT),
				.border_width=1,
				.extents={
					.overall={0},
					.dir=0,
					.ascent=DEFAULT_FONT_ASCENT,
					.descent=DEFAULT_FONT_DESCENT,
					.charWidth=DEFAULT_FONT_CHAR_WIDTH,
					.charHeight=DEFAULT_FONT_CHAR_HEIGHT,
				},
				.background=_BACKGROUND_TEMPS,
				.foreground=_FOREGROUND_TEMPS,
				.wButton={NULL},
				},
				// SYSINFO
				{
				.window=0,
				.pixmap={.B=0, .F=0},
				.gc=0,
				.x=+520,
				.y=+30,
				.width=(GEOMETRY_SYSINFO_COLS * DEFAULT_FONT_CHAR_WIDTH),
				.height=((1 + GEOMETRY_SYSINFO_ROWS + 1) * DEFAULT_FONT_CHAR_HEIGHT),
				.border_width=1,
				.extents={
					.overall={0},
					.dir=0,
					.ascent=DEFAULT_FONT_ASCENT,
					.descent=DEFAULT_FONT_DESCENT,
					.charWidth=DEFAULT_FONT_CHAR_WIDTH,
					.charHeight=DEFAULT_FONT_CHAR_HEIGHT,
				},
				.background=_BACKGROUND_SYSINFO,
				.foreground=_FOREGROUND_SYSINFO,
				.wButton={NULL},
				},
				// DUMP
				{
				.window=0,
				.pixmap={.B=0, .F=0},
				.gc=0,
				.x=DEFAULT_FONT_CHAR_WIDTH << 5,
				.y=+755,
				.width=(GEOMETRY_DUMP_COLS * DEFAULT_FONT_CHAR_WIDTH),
				.height=((1 + GEOMETRY_DUMP_ROWS + 1) * DEFAULT_FONT_CHAR_HEIGHT),
				.border_width=1,
				.extents={
					.overall={0},
					.dir=0,
					.ascent=DEFAULT_FONT_ASCENT,
					.descent=DEFAULT_FONT_DESCENT,
					.charWidth=DEFAULT_FONT_CHAR_WIDTH,
					.charHeight=DEFAULT_FONT_CHAR_HEIGHT,
				},
				.background=_BACKGROUND_DUMP,
				.foreground=_FOREGROUND_DUMP,
				.wButton={NULL},
				},
			},
			.T={
				.Locked=FALSE,
				.S=0,
				.dx=0,
				.dy=0,
			},
			.L={
				.UnMapBitmask=0,
				.globalBackground=_BACKGROUND_GLOBAL,
				.globalForeground=_FOREGROUND_GLOBAL,
				.Colors={
					{.RGB=_BACKGROUND_MAIN,    .xrmClass="Background", .xrmKey="Main"       },
					{.RGB=_FOREGROUND_MAIN,    .xrmClass="Foreground", .xrmKey="Main"       },
					{.RGB=_BACKGROUND_CORES,   .xrmClass="Background", .xrmKey="Cores"      },
					{.RGB=_FOREGROUND_CORES,   .xrmClass="Foreground", .xrmKey="Cores"      },
					{.RGB=_BACKGROUND_CSTATES, .xrmClass="Background", .xrmKey="CStates"    },
					{.RGB=_FOREGROUND_CSTATES, .xrmClass="Foreground", .xrmKey="CStates"    },
					{.RGB=_BACKGROUND_TEMPS,   .xrmClass="Background", .xrmKey="Temps"      },
					{.RGB=_FOREGROUND_TEMPS,   .xrmClass="Foreground", .xrmKey="Temps"      },
					{.RGB=_BACKGROUND_SYSINFO, .xrmClass="Background", .xrmKey="SysInfo"    },
					{.RGB=_FOREGROUND_SYSINFO, .xrmClass="Foreground", .xrmKey="SysInfo"    },
					{.RGB=_BACKGROUND_DUMP,    .xrmClass="Background", .xrmKey="Dump"       },
					{.RGB=_FOREGROUND_DUMP,    .xrmClass="Foreground", .xrmKey="Dump"       },
					{.RGB=_COLOR_AXES,         .xrmClass="Color",      .xrmKey="Axes"       },
					{.RGB=_COLOR_LABEL,        .xrmClass="Color",      .xrmKey="Label"      },
					{.RGB=_COLOR_PRINT,        .xrmClass="Color",      .xrmKey="Print"      },
					{.RGB=_COLOR_PROMPT,       .xrmClass="Color",      .xrmKey="Prompt"     },
					{.RGB=_COLOR_CURSOR,       .xrmClass="Color",      .xrmKey="Cursor"     },
					{.RGB=_COLOR_DYNAMIC,      .xrmClass="Color",      .xrmKey="Dynamic"    },
					{.RGB=_COLOR_GRAPH1,       .xrmClass="Color",      .xrmKey="Graph1"     },
					{.RGB=_COLOR_GRAPH2,       .xrmClass="Color",      .xrmKey="Graph2"     },
					{.RGB=_COLOR_GRAPH3,       .xrmClass="Color",      .xrmKey="Graph3"     },
					{.RGB=_COLOR_INIT_VALUE,   .xrmClass="Color",      .xrmKey="InitValue"  },
					{.RGB=_COLOR_LOW_VALUE,    .xrmClass="Color",      .xrmKey="LowValue"   },
					{.RGB=_COLOR_MED_VALUE,    .xrmClass="Color",      .xrmKey="MediumValue"},
					{.RGB=_COLOR_HIGH_VALUE,   .xrmClass="Color",      .xrmKey="HighValue"  },
					{.RGB=_COLOR_PULSE,        .xrmClass="Color",      .xrmKey="Pulse"      },
					{.RGB=_COLOR_FOCUS,        .xrmClass="Color",      .xrmKey="Focus"      },
					{.RGB=_COLOR_MDI_SEP,      .xrmClass="Color",      .xrmKey="MDIlineSep" },
				},
				.Page={
					// MAIN
					{
						.Pageable=TRUE,
						.Geometry={.cols=GEOMETRY_MAIN_COLS, .rows=GEOMETRY_MAIN_ROWS},
						.Visible={.cols=0, .rows=0},
						.Listing={.cols=0, .rows=1},
						.FrameSize={.cols=1, .rows=1},
						.hScroll=0,
						.vScroll=0,
						.Pixmap=0,
						.width=0,
						.height=0,
						.Title=MAIN_SECTION,
					},
					// CORES
					{
						.Pageable=FALSE,
						.Geometry={.cols=GEOMETRY_CORES_COLS, .rows=GEOMETRY_CORES_ROWS},
						.Visible={.cols=0, .rows=0},
						.Listing={.cols=0, .rows=0},
						.FrameSize={.cols=1, .rows=1},
						.hScroll=0,
						.vScroll=0,
						.Pixmap=0,
						.width=0,
						.height=0,
						.Title=NULL,
					},
					// CSTATES
					{
						.Pageable=FALSE,
						.Geometry={.cols=GEOMETRY_CSTATES_COLS, .rows=GEOMETRY_CSTATES_ROWS},
						.Visible={.cols=0, .rows=0},
						.Listing={.cols=0, .rows=0},
						.FrameSize={.cols=1, .rows=1},
						.hScroll=0,
						.vScroll=0,
						.Pixmap=0,
						.width=0,
						.height=0,
						.Title=NULL,
					},
					// TEMPS
					{
						.Pageable=FALSE,
						.Geometry={.cols=GEOMETRY_TEMPS_COLS, .rows=GEOMETRY_TEMPS_ROWS},
						.Visible={.cols=0, .rows=0},
						.Listing={.cols=0, .rows=0},
						.FrameSize={.cols=1, .rows=1},
						.hScroll=0,
						.vScroll=0,
						.Pixmap=0,
						.width=0,
						.height=0,
						.Title=NULL,
					},
					// SYSINFO
					{
						.Pageable=TRUE,
						.Geometry={.cols=GEOMETRY_SYSINFO_COLS, .rows=GEOMETRY_SYSINFO_ROWS},
						.Visible={.cols=0, .rows=0},
						.Listing={.cols=0, .rows=1},
						.FrameSize={.cols=1, .rows=1},
						.hScroll=0,
						.vScroll=0,
						.Pixmap=0,
						.width=0,
						.height=0,
						.Title=SYSINFO_SECTION,
					},
					// DUMP
					{
						.Pageable=FALSE,
						.Geometry={.cols=GEOMETRY_DUMP_COLS, .rows=GEOMETRY_DUMP_ROWS},
						.Visible={.cols=0, .rows=0},
						.Listing={.cols=0, .rows=1},
						.FrameSize={.cols=1, .rows=1},
						.hScroll=0,
						.vScroll=0,
						.Pixmap=0,
						.width=0,
						.height=0,
						.Title=DUMP_SECTION,
					},
				},
				.Play={
					.fillGraphics=TRUE,
					.freqHertz=TRUE,
					.showCycles=FALSE,
					.showRatios=TRUE,
					.showSchedule=FALSE,
					.cStatePercent=FALSE,
					.wallboard=FALSE,
					.flashCursor=TRUE,
					.alwaysOnTop=FALSE,
					.noDecorations=FALSE,
					.skipTaskbar=FALSE,
					.cursorShape=FALSE
				},
				.WB={
					.Scroll=0,
					.Length=0,
					.String=NULL,
				},
				.Usage={.C0=NULL, .C3=NULL, .C6=NULL, .C7=NULL},
				.Axes={{0, NULL}},
				// Design the TextCursor
				.TextCursor={	{.x=+0, .y=+0},
						{.x=+4, .y=-4},
						{.x=+4, .y=+4} },
				.Input={.KeyBuffer=calloc(KEYINPUT_DEPTH, 1), .KeyLength=0, .KeyInsert=0, .Top=0, .Cmd=-1},
				.Output=NULL,
			},
			.LOOP=TRUE,
			.RESTART=FALSE,
			.PAUSE={FALSE},
			.configFile=NULL,
			.Options=
			{
				{"-C", "%ms", &A.configFile,           "Path & file configuration name (String)\n" \
				                                       "\t\t  this option must be first\n" \
				                                       "\t\t  default is '$HOME/.xfreq'",                                      NULL                                       },
				{"-D", "%d",  &A.MDI,                  "Run as a MDI Window (Bool) [0/1]",                                     NULL                                       },
				{"-U", "%x",  &A.L.UnMapBitmask,       "Bitmap of unmap Widgets (Hex) eq. 0b00111111\n" \
								       "\t\t  where each bit set in the argument is a hidden Widget",          NULL                                       },
				{"-u", "%u",  &A.L.Play.cursorShape,   "Set the cursor shape (Bool) [0/1]",                                    XDB_CLASS_MAIN"."XDB_KEY_CURSOR_SHAPE      },
				{"-F", "%s",  A.fontName,              "Font name (String)\n" \
				                                       "\t\t  default font is 'Fixed'",                                        XDB_CLASS_MAIN"."XDB_KEY_FONT              },
				{"-x", "%c",  &A.xACL,                 "Enable or disable the X ACL (Char) ['Y'/'N']",                         NULL                                       },
				{"-g", "%ms", &A.Geometries,           "Widgets geometries (String)\n" \
				                                       "\t\t  argument is a series of '#:[cols]x[rows]+[x]+[y], .. ,'",        NULL                                       },
				{"-b", "%x",  &A.L.globalBackground,   "Background color (Hex) {RGB}\n" \
								       "\t\t  argument is coded with primary colors 0xRRGGBB",                 NULL                                       },
				{"-f", "%x",  &A.L.globalForeground,   "Foreground color (Hex) {RGB}",                                         NULL                                       },
				{"-l", "%u",  &A.L.Play.fillGraphics,  "Fill or not the graphics (Bool) [0/1]",                                XDB_CLASS_CORES"."XDB_KEY_PLAY_GRAPHICS    },
				{"-z", "%u",  &A.L.Play.freqHertz,     "Show the Core frequency (Bool) [0/1]",                                 XDB_CLASS_CORES"."XDB_KEY_PLAY_FREQ        },
				{"-y", "%u",  &A.L.Play.showCycles,    "Show the Core Cycles (Bool) [0/1]",                                    XDB_CLASS_CORES"."XDB_KEY_PLAY_CYCLES      },
				{"-j", "%u",  &A.L.Play.showIPS,       "Show the Instructions Per Second (Bool) [0/1]",                        XDB_CLASS_CORES"."XDB_KEY_PLAY_IPS         },
				{"-J", "%u",  &A.L.Play.showIPC,       "Show the Instructions Per Cycle (Bool) [0/1]",                         XDB_CLASS_CORES"."XDB_KEY_PLAY_IPC         },
				{"-i", "%u",  &A.L.Play.showCPI,       "Show the Cycles Per Instructions (Bool) [0/1]",                        XDB_CLASS_CORES"."XDB_KEY_PLAY_CPI         },
				{"-r", "%u",  &A.L.Play.showRatios,    "Show the Core Ratio (Bool) [0/1]",                                     XDB_CLASS_CORES"."XDB_KEY_PLAY_RATIOS      },
				{"-p", "%u",  &A.L.Play.cStatePercent, "Show the Core C-State percentage (Bool) [0/1]",                        XDB_CLASS_CSTATES"."XDB_KEY_PLAY_CSTATES   },
				{"-w", "%u",  &A.L.Play.wallboard,     "Scroll the Processor brand wallboard (Bool) [0/1]",                    XDB_CLASS_SYSINFO"."XDB_KEY_PLAY_WALLBOARD },
				{"-o", "%u",  &A.L.Play.alwaysOnTop,   "Keep the Widgets always on top of the screen (Bool) [0/1]",            NULL                                       },
				{"-n", "%u",  &A.L.Play.noDecorations, "Remove the Window Manager decorations (Bool) [0/1]",                   NULL                                       },
				{"-N", "%u",  &A.L.Play.skipTaskbar,   "Remove the Widgets title name from the WM taskbar (Bool) [0/1]",       NULL                                       },
				{"-I", "%hx", &A.Splash.attributes,    "Splash screen attributes 0x{H}{NNN} (Hex)\n" \
				                                       "\t\t  where {H} bit:13 hides Splash and {NNN} (usec) defers start-up", NULL                                       },
			},
			.Commands=COMMANDS_LIST
		};
	for(A.L.Input.Recent=HISTORY_DEPTH - 1; A.L.Input.Recent >= 0; A.L.Input.Recent--)
	{
		A.L.Input.History[A.L.Input.Recent].KeyBuffer=calloc(KEYINPUT_DEPTH, 1);
		A.L.Input.History[A.L.Input.Recent].KeyLength=0;
	}
	A.L.Input.Expand.KeyBuffer=calloc(KEYINPUT_DEPTH, 1);
	A.L.Input.Expand.KeyLength=A.L.Input.Recent=A.L.Input.Browse=0;

	uid_t	UID=geteuid();
	Bool32	ROOT=(UID == 0),	// Check root access.
		fEmergencyThread=FALSE;
	int	rc=0;

	XrmInitialize();
	if(LoadSettings(&A, argc, argv))
	{
	    do
	    {
	    	A.LOOP=TRUE;
		A.RESTART=FALSE;
		// Initialize & run the Widget.
		if(XInitThreads() && OpenDisplay(&A))
		{
			char *BootLog=calloc(1024, 1);

			sigemptyset(&A.Signal);
			sigaddset(&A.Signal, SIGINT);	// [CTRL] + [C]
			sigaddset(&A.Signal, SIGQUIT);
			sigaddset(&A.Signal, SIGUSR1);
			sigaddset(&A.Signal, SIGUSR2);
			sigaddset(&A.Signal, SIGTERM);
			sigaddset(&A.Signal, SIGCONT);
//			sigaddset(&A.Signal, SIGTSTP);	// [CTRL] + [Z]

			if(!pthread_sigmask(SIG_BLOCK, &A.Signal, NULL)
			&& !pthread_create(&A.TID_SigHandler, NULL, uEmergency, &A))
				fEmergencyThread=TRUE;
			else
				strcat(BootLog, "Remark: cannot start the signal handler.\n");

			if(!SPLASH_HIDDEN_FLAG)
				StartSplash(&A);

			if(SPLASH_DEFERRED_TIME > 0)
				usleep(SPLASH_DEFERRED_TIME * IDLE_BASE_USEC);

			if(ROOT)
				strcat(BootLog, "Warning: running as root.\n");

			struct stat shmStat={0};
			if((A.FD.Shm=shm_open(SHM_FILENAME, O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)) == -1)
				strcat(BootLog, "Error: opening the shared memory.\n");
			else if((fstat(A.FD.Shm, &shmStat) != -1) && (A.SHM=mmap(0, shmStat.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, A.FD.Shm, 0)) == MAP_FAILED)
				strcat(BootLog, "Error: mapping the shared memory.\n");
			else
				A.Room=Sync_Open(&A.SHM->Sync);

			if(!SPLASH_HIDDEN_FLAG)
				StopSplash(&A);

			if((A.FD.Shm != -1) && (A.Room != 0) && OpenWidgets(&A))
			{
				struct stat smbStat={0};
				if(((A.FD.SmBIOS=shm_open(SMB_FILENAME, O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)) == -1)
				|| (fstat(A.FD.SmBIOS, &smbStat) == -1)
				|| ((A.SmBIOS=mmap(A.SHM->B, smbStat.st_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, A.FD.SmBIOS, 0)) == MAP_FAILED))
					strcat(BootLog, "Error: opening the SmBIOS shared memory");
#if defined(DEBUG)

					printf(	"\n--- SHM Map ---\n"
						"A.SHM[%p]\n" \
						"A.SHM->P[%p]\n" \
						"A.SHM->H[%p]\n" \
						"A.SHM->D[%p]\n" \
						"A.SHM->M[%p]\n" \
						"A.SHM->S[%p]\n" \
						"A.SHM->B[%p]\n" \
						"A.SHM->C[%p]\n", \
						A.SHM, \
						&A.SHM->P, \
						&A.SHM->H, \
						&A.SHM->D, \
						&A.SHM->M, \
						&A.SHM->S, \
						A.SHM->B, \
						&A.SHM->C);
					fflush(stdout);
#endif
				int G=0;
				for(G=MAIN; G < WIDGETS; G++)
				{
					BuildLayout(&A, G);
					MapLayout(&A, G);
					if(!(A.L.UnMapBitmask & (1 << G)))
						XMapWindow(A.display, A.W[G].window);
					else
						A.PAUSE[G]=TRUE;
				}
				if(!pthread_create(&A.TID_Draw, NULL, uDraw, &A))
				{
					sprintf(BootLog, "%s\n%s Ready with [%s]\n\n", BootLog, _APPNAME, A.SHM->AppName);
					Output(&A, BootLog);
					Output(&A, "Enter help to list commands.\n");

					uLoop(&A);

					// Shutting down.
					struct timespec absoluteTime={.tv_sec=0, .tv_nsec=0}, gracePeriod={.tv_sec=0, .tv_nsec=0};
					abstimespec(IDLE_BASE_USEC * IDLE_COEF_MAX, &absoluteTime);
					if(!addtimespec(&gracePeriod, &absoluteTime)
					&& pthread_timedjoin_np(A.TID_Draw, NULL, &gracePeriod))
					{
						rc=pthread_kill(A.TID_Draw, SIGKILL);
						rc=pthread_join(A.TID_Draw, NULL);
					}
				}
				else
				{
					tracerr("Error: cannot start the thread uDraw()");
					rc=2;
				}
				if((A.SmBIOS != MAP_FAILED) && (munmap(A.SmBIOS, smbStat.st_size) == -1))
					tracerr("Error: unmapping the SmBIOS shared memory");
				if((A.FD.SmBIOS != -1) && (close(A.FD.SmBIOS) == -1))
					tracerr("Error: closing the SmBIOS shared memory");

				CloseWidgets(&A);
			}
			else
			{
				tracerr("Error: failure in starting the Widgets");
				rc=2;
			}
			// Release the ressources.
			if(A.Room)
				Sync_Close(A.Room, &A.SHM->Sync);
			if((A.SHM != MAP_FAILED) && (munmap(A.SHM, shmStat.st_size) == -1))
				tracerr("Error: unmapping the shared memory");
			if((A.FD.Shm != -1) && (close(A.FD.Shm) == -1))
				tracerr("Error: closing the shared memory");
			if(fEmergencyThread)
			{
				pthread_kill(A.TID_SigHandler, SIGUSR1);
				pthread_join(A.TID_SigHandler, NULL);
			}
			free(BootLog);
		}
		else
		{
			rc=2;
		}
		CloseDisplay(&A);

	    } while(A.RESTART);
	}
	else
	{
		rc=1;
	}
	for(A.L.Input.Recent=HISTORY_DEPTH - 1; A.L.Input.Recent >= 0; A.L.Input.Recent--)
		if(A.L.Input.History[A.L.Input.Recent].KeyBuffer != NULL)
			free(A.L.Input.History[A.L.Input.Recent].KeyBuffer);
	if(A.L.Input.Expand.KeyBuffer != NULL)
		free(A.L.Input.Expand.KeyBuffer);
	if(A.L.Input.KeyBuffer != NULL)
		free(A.L.Input.KeyBuffer);
	if(A.Geometries != NULL)
		free(A.Geometries);
	if(A.fontName != NULL)
		free(A.fontName);
	if(A.configFile != NULL)
		free(A.configFile);

	return(rc);
}
