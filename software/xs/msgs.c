/**************************** XS ********************************************
Copyright (C) 2000-2012  P. Bergman

This program is free software; you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
software; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA  02111-1307 USA                             
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#if XmVersion <= 1100
#include <X11/StringDefs.h>
#endif

#include "msgs.h"
#include "defines.h"
#include "global_structs.h"

/*** External stuff ***/
extern GLOBAL *gp;
extern USER   *pP;
extern int     msg_w, msg_h;

/*** Local stuff ***/
static MSG msg;

static void update_viewer(MSG *m)
{
    Arg   wargs[10];
    int   n, size, maxv;

    if (!gp->msgTop) return;
    
    size = m->canvas_height / m->fontheight + 1;
    if (size >= m->nitems) size = m->nitems;
    if (size < 1) size = 1;
    maxv = m->nitems + 1;
    if (maxv <= 2) maxv = 2;
    m->top = maxv - size;
    if (m->scrollbar) {
        n = 0;
        XtSetArg(wargs[n], XmNminimum,           0);  n++;
        XtSetArg(wargs[n], XmNmaximum,        maxv);  n++;
        XtSetArg(wargs[n], XmNvalue,        m->top);  n++;
        XtSetArg(wargs[n], XmNsliderSize,     size);  n++;
        XtSetArg(wargs[n], XmNpageIncrement,  size);  n++;
        XtSetValues(m->scrollbar, wargs, n);
    }
    if (m->canvas && XtIsRealized(m->canvas)) {
        XClearArea(XtDisplay(m->canvas), XtWindow(m->canvas),
                   0, 0, 0, 0, TRUE);
    }
}

void send_line(char *line)
{
    int         i, j, dir, ascent, desc;
    XCharStruct char_info;
    char        buf[MAXSIZE];

    i = msg.nitems;
    if (i == MAXLINES) {
        for (j=1; j<i; j++) {
            strcpy(buf, msg.chars[j]);
            if (msg.chars[j-1])
                msg.chars[j-1] = XtRealloc(msg.chars[j-1], strlen(buf) + 1);
            else
                msg.chars[j-1] = XtMalloc(strlen(buf)+1);
/*            buf[MAXSIZE - 1] = '\0'; */
            strcpy(msg.chars[j-1], buf);
            
            msg.length[j-1] = strlen(msg.chars[j-1]);
            XTextExtents(msg.font, msg.chars[j-1],
                         msg.length[j-1], &dir, &ascent, 
                         &desc, &char_info);
            msg.rbearing[j-1] = char_info.rbearing;
            msg.descent       = desc;
            msg.fontheight    = ascent + desc;
        }
        i--;
    }
    strcpy(buf, line);
    if (msg.chars[i])
        msg.chars[i] = XtRealloc(msg.chars[i], strlen(buf) + 1);
    else
        msg.chars[i] = XtMalloc(strlen(buf) + 1);
/*    buf[strlen(buf) - 1] = '\0'; */
    strcpy(msg.chars[i], buf);
    
    msg.length[i] = strlen(msg.chars[i]);
    XTextExtents(msg.font, msg.chars[i],
                 msg.length[i], &dir, &ascent, 
                 &desc, &char_info);
    msg.rbearing[i] = char_info.rbearing;
    msg.descent     = desc;
    msg.fontheight  = ascent + desc;
    i++;
    msg.nitems = i;
    update_viewer(&msg);
}

void init_msg()
{
    int i;

    msg.nitems = 0;
    msg.top = 0;
    msg.font = gp->font14;
    msg.scrollbar = NULL;
    for (i=0; i<MAXLINES; i++)
        msg.chars[i] = NULL;
    
    send_line("Message log:");
}

static void scroll_bar_moved(Widget w, MSG *m, XmScrollBarCallbackStruct *cb)
{
    int     sliderpos = cb->value;
    int     ysrc,  redraw_top, delta;
    Display *dpy = XtDisplay(m->canvas);
    Window   win = XtWindow(m->canvas);
   
   /* 
    * Compute number of pixels the text needs to be moved.
    */
    delta = ABS((m->top - sliderpos) * m->fontheight);   
    delta = MIN(delta, m->canvas_height);
   /*
    * If we are scrolling down, we start at zero and simply 
    * move by the delta. The portion that must be redrawn
    * is simply between zero and delta.
    */ 
    ysrc = redraw_top = 0;
   /* 
    * If we are scrolling up, we start at the delta and move
    * to zero. The part to redraw lies between the bottom
    * of the window and the bottom - delta.
    */
    if (sliderpos >= m->top) { 
        ysrc        =  delta;
        redraw_top  =  m->canvas_height - delta;
    }
   /*
    * Set the top line of the text buffer.
    */
    m->top = sliderpos;
   /*
    * Move the existing text to its new position.
    * Turn off any clipping on the GC first.
    */
    XSetClipMask(dpy, m->gc, None);
    XCopyArea(dpy, win, win, m->gc, 0, ysrc, 
              m->canvas_width, m->canvas_height - delta,
              0,  delta - ysrc);
   /*
    * Clear the remaining area of any old text, 
    */
    XClearArea(dpy, win, 0, redraw_top, 0, delta, FALSE); 
    {
        int     yloc = 0, index = m->top;
        
        while (index < m->nitems && yloc < m->canvas_height) {
            yloc += m->fontheight;
            if (yloc >= redraw_top
               && (yloc - m->fontheight) <= (redraw_top + delta))
                XDrawImageString(dpy, win, m->gc,
                                 MARGIN, yloc, m->chars[index], 
                                 m->length[index]);
            index++;
        }
    }
}

static Widget create_scrollbar(Widget parent, MSG *m)
{
    Arg    wargs[10];
    int    n = 0;
    Widget scrollbar;
  /*
   * Set the scrollbar so that movements are
   * reported in terms of lines of text. Set the
   * scrolling increment to a single line, and the page
   * increment to the number of lines the canvas widget
   * can hold. Also set the slider size to be proportional.
   */
    XtSetArg(wargs[n], XmNminimum,           0);  n++;
    XtSetArg(wargs[n], XmNmaximum,   m->nitems);  n++;
    XtSetArg(wargs[n], XmNincrement,         1);  n++;        
    XtSetArg(wargs[n], XmNsliderSize,        1);  n++;
    XtSetArg(wargs[n], XmNpageIncrement,     1);  n++;
    scrollbar = XtCreateManagedWidget("scrollbar",
                                      xmScrollBarWidgetClass,
                                      parent, wargs, n);
    XtAddCallback(scrollbar, XmNvalueChangedCallback,
                  (XtCallbackProc)scroll_bar_moved, m);
    XtAddCallback(scrollbar, XmNdragCallback,
                  (XtCallbackProc)scroll_bar_moved, m);
    
    return scrollbar;
}

static void create_gc(MSG *m)
{
    XGCValues  gcv;
    Display   *dpy  = XtDisplay(m->canvas);
    Window     win  = XtWindow(m->canvas);
    int        mask = GCFont | GCForeground | GCBackground;
    Arg        wargs[10];
    int        n;
  /*
   * Create a GC using the colors of the canvas widget.
   */
     n = 0;
     XtSetArg(wargs[n], XtNforeground, &gcv.foreground); n++;
     XtSetArg(wargs[n], XtNbackground, &gcv.background); n++;
     XtGetValues(m->canvas, wargs, n);

     gcv.font    = m->font->fid;
     m->gc       = XCreateGC(dpy, win, mask, &gcv);
}

static void handle_exposures(Widget w, MSG *m,
                             XmDrawingAreaCallbackStruct *cb)
{
    int     yloc = 0, index = m->top;
    Region  region;
    Display *dpy = XtDisplay(m->canvas);
    Window   win = XtWindow(m->canvas);
    
  /*
   * Create a region and add the contents of the of the event
   */
    region = XCreateRegion();

    XtAddExposureToRegion(cb->event, region);
    
  /*
   * Set the clip mask of the GC.
   */
    XSetRegion(dpy, m->gc, region); 
  /*
   * Loop through each line until the bottom of the
   * window is reached, or we run out of lines. Redraw any 
   * lines that intersect the exposed region.
   */
    while (index < m->nitems && yloc < m->canvas_height) {
        yloc += m->fontheight;
        if (XRectInRegion(region, MARGIN, yloc - m->fontheight,
                          m->rbearing[index],
                          m->fontheight) != RectangleOut)  
            XDrawImageString(dpy, win, m->gc,
                             MARGIN, yloc, m->chars[index], 
                             m->length[index]);
        index++;
    }
    XDestroyRegion(region);
    region = NULL;
}

static void resize(Widget w, MSG *m, XtPointer call_data)
{
    Arg   wargs[10];
    int   n, size, maxv, val;
  /*
   * Determine the new widget of the canvas widget.
   */
    n = 0;
    XtSetArg(wargs[n], XtNheight, &m->canvas_height);n++;
    XtSetArg(wargs[n], XtNwidth,  &m->canvas_width);n++;
    XtGetValues(w, wargs, n);
    
    if (!m->scrollbar) return;
    
  /*
   * Reset the scrollbar slider to indicate the relative
   * proportion of text displayed and also the new page size.
   */
    size = m->canvas_height / m->fontheight + 1;
    if (size >= m->nitems) size = m->nitems;
    if (size < 1) size = 1;
    maxv = m->nitems + 1;
    if (maxv <= 2) maxv = 2;

    val = m->top;
    if (val < 0) val = 0;
    if (val > maxv - size) val = maxv - size;

    n = 0;
    XtSetArg(wargs[n], XmNminimum,           0);  n++;
    XtSetArg(wargs[n], XmNmaximum,        maxv);  n++;
    XtSetArg(wargs[n], XmNincrement,         1);  n++;        
    XtSetArg(wargs[n], XmNsliderSize,     size);  n++;
    XtSetArg(wargs[n], XmNpageIncrement,  size);  n++;
    XtSetArg(wargs[n], XmNvalue,          val);  n++;
    XtSetValues(m->scrollbar, wargs, n);
}


/*
 * Workaround for DrawingArea widget deficiency:
 *
 * If a GraphicsExpose is recieved, redraw the window by calling the
 * DrawingArea widget's XmNexposeCallback list.
 */

void handle_g_exposures(Widget w, MSG *m, XEvent *event)
{
  /*
   * This routine will be called for all non-maskable events. Make sure 
   * it's the type we want.
   */
    if (event->type == GraphicsExpose) {

        XmDrawingAreaCallbackStruct cb;
      /*
       * Fill out a call data struct.
       */
        cb.reason = XmCR_EXPOSE;
        cb.event = event;
        cb.window = XtWindow(w);
      /*
       * Invoke all handlers on the XmNexposeCallback list.
       */
        XtCallCallbacks(w, XmNexposeCallback, &cb);
    }
}

static void destroy_message_viewer(Widget w, Widget m, XmAnyCallbackStruct *cb)
{
    if (m) {
        gp->msgTop = NULL;
    }
}

Widget make_msg_viewer(Widget o)
{
    Widget top, sw;
    Arg wargs[10];
    string buf;
    int n;
    Dimension p_x, p_y, p_w, p_h;
    Dimension o_x, o_y, o_w, o_h;
    COLOR *c;
    static Pixmap iconPixmap = 0;
    
    COLOR *GetColorInfo();

    XtVaGetValues(gp->top, XmNx, &p_x, XmNy, &p_y, XmNwidth, &p_w,
                  XmNheight, &p_h, NULL);
    XtVaGetValues(o, XmNx, &o_x, XmNy, &o_y, XmNwidth, &o_w,
                  XmNheight, &o_h, NULL);

    sprintf(buf, "%s Message Log", PKGNAME);
    
    c = GetColorInfo();
    
    n = 0;
    XtSetArg(wargs[n], XmNwidth,     o_w); n++;
    XtSetArg(wargs[n], XmNheight,    msg_h); n++;
    XtSetArg(wargs[n], XmNx,         p_x); n++;
    XtSetArg(wargs[n], XmNy,         p_y + p_h + 20); n++;
    XtSetArg(wargs[n], XmNtitle,     buf); n++;
    XtSetArg(wargs[n], XmNiconName,  "Msg log"); n++;
    if (gp->privateColors) {
        XtSetArg(wargs[n], XmNcolormap, c->cmap); n++;
    }
    top = XtAppCreateShell("Messages", "", topLevelShellWidgetClass,
                           XtDisplay(gp->top), wargs, n);
    if (iconPixmap == 0) {
        iconPixmap = XmGetPixmap(XtScreen(top), pP->msgs_xpm, c->black, c->white);
    }
    XtVaSetValues(top, XmNiconPixmap, iconPixmap, NULL);
/*
    n = 0;
    XtSetArg(wargs[n], XmNscrollingPolicy, XmAUTOMATIC); n++;
 */
    sw = XtCreateManagedWidget("scroller", xmScrolledWindowWidgetClass,
                               top, NULL, 0);
    msg.canvas = XtCreateManagedWidget("canvas", xmDrawingAreaWidgetClass,
                                       sw, NULL, 0);
    XtSetArg(wargs[0], XtNheight, &msg.canvas_height);
    XtSetArg(wargs[1], XtNwidth, &msg.canvas_width);
    XtGetValues(msg.canvas, wargs, 2);

#ifdef DEBUG
    printf("init_msg()...\n");
#endif  
    init_msg();
#ifdef DEBUG
    printf("left init_msg().\n");
#endif  
  
    XtAddCallback(msg.canvas, XmNexposeCallback, 
                  (XtCallbackProc)handle_exposures, &msg);
    XtAddCallback(msg.canvas, XmNresizeCallback, 
                  (XtCallbackProc)resize, &msg);
    XtAddCallback(top, XmNdestroyCallback,
                  (XtCallbackProc)destroy_message_viewer, top);

 /*
  * The DrawingArea widget doesn't call its expose callback when 
  * Graphics Expose events occur. This event handler watches for them
  */
    XtAddEventHandler(msg.canvas, 0, TRUE,
                      (XtEventHandler)handle_g_exposures, &msg);
  
#ifdef DEBUG
    printf("before create_scrollbar().\n");
#endif
    msg.scrollbar = create_scrollbar(sw, &msg);
  
    n = 0;
    XtSetArg(wargs[n], XmNverticalScrollBar, msg.scrollbar); n++;
    XtSetArg(wargs[n], XmNworkWindow, msg.canvas); n++;
    XtSetValues(sw, wargs, n);

#ifdef DEBUG
    printf("before XtRealizeWidget().\n");
#endif  
    XtRealizeWidget(top);
  
    create_gc(&msg);
  
    return sw;
}
