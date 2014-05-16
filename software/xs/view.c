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
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <Xm/Xm.h>

#include "list.h"
#include "defines.h"
#include "global_structs.h"

#define STD_MARGIN_FACTOR 10   /* Valid range: > 0 */

#define XS_ABS(x)    ((x) < 0 ? -(x) : (x))

/*** External variables ***/
extern char  x1_label[], x2_label[], x3_label[], x4_label[], x5_label[];
extern char  RA_label[], Dec_label[], Pos_label[], *unit_labels[];
extern DRAW draw;

void PostWarningDialog(Widget, char *);
void send_line(char *);

/*** Local variables ***/
VIEW view, *vP;

static int marginXFactor, marginYFactor;
static int activeSubView;
static VIEW mainview, scanview;

typedef struct {
    int width, height;
    int min_x, min_y;
    int marg_x, marg_y;
    double magn_x, magn_y;
    int nX, nY;
    int i, j;
} SUBVIEW;

static SUBVIEW subview;

void init_view()
{
    vP = &view;
}

static void SetMarginFactors(int x, int y)
{
    if (x > 0) marginXFactor = x;
    if (y > 0) marginYFactor = y;
}

/* To be called by main() when data have been read. */
void InitView(int mode)
{
    void SetStdView(), SetDefWindow();
    int  SetViewMode(), GetStartingUnit();
    scanPtr first = NULL;
    
    if (view.from && view.from->scanlist)
        first = (scanPtr)DATA(view.from->scanlist);

    SetMarginFactors(STD_MARGIN_FACTOR, STD_MARGIN_FACTOR);
    subview.nX = 0;
    subview.nY = 0;
    activeSubView  = 0;

    view.xunit = GetStartingUnit();
    view.fixed_x = 0;
    view.fixed_y = 0;
    view.fixed_z = 0;
    view.autoscale_x = 0;
    view.autoscale_y = 0;
    view.Nx = 0;
    view.Ny = 0;
    view.subXmarg = 0;
    view.subYmarg = 0;
    view.subXmagn = 1.0;
    view.subYmagn = 1.0;
    view.lef_x = 0.06;
    view.lef_y = 0.92;
    view.rig_x = 0.85;
    view.rig_y = view.lef_y;
    view.tag_markers = 0;
    view.join_markers = 0;
    view.remove_joint = 0;
    view.tag_scatters = 0;
    view.special_view_type = VIEW_TOP_RIGHT;
    view.tlab_type = TLAB_SOURCE;
    view.slab_type = TLAB_NONE;
    view.llab_type = TLAB_NONE;
    view.rlab_type = TLAB_NONE;
    view.nMaps = 0;
    view.M = NULL;
    view.nScat = 0;
    view.P = NULL;
    view.xrange = 1.0;
    view.yrange = 1.0;
    view.xref = 0.0;
    view.yref = 0.0;
    view.s2 = NULL;

    if (SetViewMode(mode, first, view.m, view.p)) {
        send_line("Warning: Couldn't init the view properly. No data?");
        SetStdView();
        return;
    }
    SetStdView();
    SetDefWindow(SCALE_BOTH);
}

int SetViewMode(int mode, scan *s, MAP *m, scatter *p)
{
    void UpdateHeaderInfo();
    
    switch (mode) {
        case SHOW_SPE:
        case SHOW_SUBSPE:
        case SHOW_ALLSPE:
            if (!s)
                return 1;
            break;
        case SHOW_POSPOS:
        case SHOW_VELPOS:
        case SHOW_POSVEL:
            if (!m)
                return 1;
            break;
        case SHOW_SCATTER:
            if (!p)
                return 1;
            break;
    }

    view.mode = mode;
    view.s = s;
    view.m = m;
    view.p = p;
    
    if (mode != SHOW_SUBSPE) UpdateHeaderInfo();

    return 0;
}

void SetScanView(VIEW *v)
{
    if (v)
        scanview = *v;
    else
        scanview = view;
}

VIEW *GetScanView()
{
    return &scanview;
}

/* This function sets up a standard view. The standard view uses the
 * entire drawing area (main_w, main_h), but a margin around the view
 * port is also set depending on the margin factors.
 */
void SetStdView()
{
    int w, h;
    void SetView();

    w = view.main_w;
    h = view.main_h;
    SetView(0, 0, w/marginXFactor, h/marginYFactor);
}

/* This function initializes a subview. nX and nY are the number of
 * subpanels in the x and y directions, respectively. If the width argument
 * (or the height argument) is <= 0, the subview width (or height) will be
 * based on the current width (or height) of the view as: box_w/nX
 * (or box_h/nY). This makes it easy to have many smaller panels within a
 * large frame. Also, if a margin argument (margX or margY) is < 0, the
 * margin is set using a margin factor, otherwise its value is used.
 * Normally a subview shouldn't have a margin, thus margX and margY should
 * be 0.
 * Avoid using this routine directly, use BeginSubView() instead, see below.
 */
static void InitSubView(int width, int height, int nX, int nY, int mX, int mY)
{
    double x1, x2;

    int xunit2x(), yunit2y();

    if (nX > 0 && nY > 0) {
        subview.nX = nX;
        subview.nY = nY;
    } else {
        subview.nX = 0;
        subview.nY = 0;
        return;
    }

    if (width <= 0) {
        x1 = view.m->xleft  - view.m->xspacing/2.0;
        x2 = view.m->xright + view.m->xspacing/2.0;
        subview.width = XS_ABS(xunit2x(x1) - xunit2x(x2));
        subview.min_x = xunit2x(x1);
    } else {
        subview.width = width;
        subview.min_x = view.min_x;
    }

    if (height <= 0) {
        x1 = view.m->ylower - view.m->yspacing/2.0;
        x2 = view.m->yupper + view.m->yspacing/2.0;
        subview.height = XS_ABS(yunit2y(x1) - yunit2y(x2));
        subview.min_y = yunit2y(x1);
    } else {
        subview.height = height;
        subview.min_y = view.min_y;
    }
    
    subview.marg_x = mX;
    subview.marg_y = mY;
        
    subview.magn_x = view.subXmagn;
    subview.magn_y = view.subYmagn;
    
    activeSubView = 1;
}

/* This function should be called when the drawing window has been
 * resized. If there is an active subview it's also resized.
 */
void ResizeView(int newW, int newH)
{
    void SetWindow();

    if (subview.nX > 0 && subview.nY > 0)
        InitSubView((subview.width*newW)/view.main_w,
                    (subview.height*newH)/view.main_h,
                     subview.nX, subview.nY,
                     subview.marg_x, subview.marg_y);

    view.main_w = newW;
    view.main_h = newH;
    
    SetStdView();
    
    if (view.mode == SHOW_POSPOS)
        SetWindow(view.xleft, view.xright, view.ylower, view.yupper);
}

/* This function sets the subview according to 0 <= i < subview.nX and
 * 0 <= j < subview.nY. Use only this function if the subpanels cannot
 * be drawn in normal order, i.e. (i,j) = (0, 0), (1, 0), ..., (nX-1, nY-1).
 * In the normal order case use NextSubView().
 */
int SetSubView(int i, int j)
{
    int x, y, sW, sH, sX, sY;
    double w, h;
    int mX = subview.marg_x, mY = subview.marg_y;
    int nX = subview.nX, nY = subview.nY;

    void SetView();

    if (i < 0 || i >= nX || j < 0 || j >= nY)
        return 0;

    subview.i = i;
    subview.j = j;
    w = (double)subview.width / (double)nX;
    h = (double)subview.height / (double)nY;
    x = subview.min_x + NINT((double)i * w);
    y = subview.min_y - NINT((double)(j+1) * h);
    sW = NINT(w * subview.magn_x)-2*mX;
    if (sW <= 0) sW = 1;
    sH = NINT(h * subview.magn_y)-2*mY;
    if (sH <= 0) sH = 1;
    sX = x + mX - NINT(w * (subview.magn_x - 1.0)/2.0);
    sY = y + mY - NINT(h * (subview.magn_y - 1.0)/2.0);
    SetView(sW, sH, sX, sY);
    
    return (int)(w < h ? w : h);
}

/* Function to use before a multipanel plot. The arguments are the same
 * as those for InitSubView() above. The functions initializes the
 * subview parameters by calling InitSubview(), then it saves the current
 * view in mainview. Finally it sets up the first subview (i,j)=(0,0).
 */
VIEW *BeginSubView(int width, int height, int nX, int nY, int mX, int mY)
{
    InitSubView(width, height, nX, nY, mX, mY);

    mainview = view;

    return &mainview;
}

/* This function advances to next subview using normal ordering. Use
 * SetSubView() if other ordering is desired.
 */
int NextSubView()
{
    int size;
    
    if (subview.i < subview.nX - 1) {
        subview.i++;
    } else {
        subview.i = 0;
        if (subview.j < subview.nY - 1) {
            subview.j++;
        } else {
            subview.j = 0;
        }
    }
    size = SetSubView(subview.i, subview.j);
    
    return size;
}

/* This function should be called after the last subview has been drawn.
 * It simply restores the temporary view back into the current. Extra
 * care is taken if any resizing of the main plotting window has occured
 * since the temporary view was stored.
 */
void EndSubView()
{
    int w, h;

    /* Get the current size of the main window */
    w = view.main_w;
    h = view.main_h;
    
    activeSubView = 0;

    view = mainview;

    /* Set the correct size of the main window */
    view.main_w = w;
    view.main_h = h;

    /* Set the current view according to the coordinates of the view
     * prior to the subview plotting */
    SetStdView();
}

int OutsideView(double x, double y, VIEW *v)
{
    double min, max, tmp;

    min = v->xleft; max = v->xright;
    if (v->mode == SHOW_ALLSPE) {
        min += v->xspacing/4.0;
        max -= v->xspacing/4.0;
    }
    if (min > max) {tmp = min; min = max; max = tmp;}
    if (x < min || x > max) return 1;

    min = v->ylower; max = v->yupper;
    if (v->mode == SHOW_ALLSPE) {
        min += v->yspacing/4.0;
        max -= v->yspacing/4.0;
    }
    if (min > max) {tmp = min; min = max; max = tmp;}
    if (y < min || y > max) return 1;

    return 0;
}

void SetView(int width, int height, int margX, int margY)
{
    int amount;
    
    int CheckWedge(), GetWedgePos(), CheckHeader();
    
    if (width > 0 && margX > 0) {
        view.min_x = margX;
        view.box_w = width;
    } else if (width > 0) {
        view.min_x = (view.main_w - width)/2;
        view.box_w = width;
    } else if (margX > 0) {
        view.min_x = margX;
        view.box_w = view.main_w - 2*margX;
    } else {
        view.min_x = 0;
        view.box_w = width;
    }

    if (height > 0 && margY > 0) {
        view.min_y = margY + height;
        view.box_h = height;
    } else if (height > 0) {
        view.min_y = (view.main_h + height)/2;
        view.box_h = height;
    } else if (margY > 0) {
        view.min_y = view.main_h - margY;
        view.box_h = view.main_h - 2*margY;
    } else {
        view.min_y = view.main_h;
        view.box_h = height;
    }
    
    if (CheckWedge()) {
        switch (GetWedgePos()) {
            case POS_RIGHT:
                amount = (view.box_w*8)/60;
                view.box_w -= amount;
                break;
            case POS_LEFT:
                amount = (view.box_w*8)/60;
                view.box_w -= amount;
                view.min_x += amount;
                break;
            case POS_ABOVE:
                amount = (view.box_h*8)/60;
                view.box_h -= amount;
                break;
            case POS_BELOW:
                amount = (view.box_h*8)/60;
                view.box_h -= amount;
                view.min_y -= amount;
                break;
        }
    }
    if (CheckHeader()) view.box_h = (view.box_h*50)/60;

    if (view.xrange != 0.0)
        view.scale_x = (double)view.box_w/view.xrange;
    if (view.yrange != 0.0)
        view.scale_y = (double)view.box_h/(-view.yrange);

}

static void SetPosPosAspectRatio(double xrange, double yrange)
{
    int oldW, oldH;
    
    void SetStdView();
    
    if (view.mode != SHOW_POSPOS || xrange == 0.0 || yrange == 0.0) return;
    
    if (!activeSubView) {
        SetStdView();
        oldW = view.box_w;
        oldH = view.box_h;
    } else {
        oldW = view.box_w;
        oldH = view.box_h;
    }
        
    if ((double)view.box_w*fabs(yrange) > (double)view.box_h*fabs(xrange)) {
        if (yrange != 0.0)
            view.scale_y = (double)view.box_h/(-yrange);
        if (xrange > 0.0)
            view.scale_x = fabs(view.scale_y);
        else
            view.scale_x = -fabs(view.scale_y);
        view.box_w = NINT(xrange * view.scale_x);
        view.min_x += (oldW - view.box_w)/2;
    } else {
        if (xrange != 0.0)
            view.scale_x = (double)view.box_w/xrange;
        if (yrange > 0.0)
            view.scale_y = -fabs(view.scale_x);
        else
            view.scale_y = fabs(view.scale_x);
        view.box_h = NINT(-yrange * view.scale_y);
        view.min_y -= (oldH - view.box_h)/2;
    }
}

/* Function to use for zooming in or out on the plot.
 * Arguments are the new user coordinates. To retain the y-scale, use:
 *        SetWindow(newXLeft, newXRight, view.ylower, view.yupper);
 */
void SetWindow(double xleft, double xright, double ylower, double yupper)
{
    if (view.mode == SHOW_ALLSPE) {
        xleft  -= view.m->xspacing/2.0;
        xright += view.m->xspacing/2.0;
        ylower -= view.m->yspacing/2.0;
        yupper += view.m->yspacing/2.0;
    }

    if (!view.fixed_x) {
        view.xleft  = xleft;
        view.xright = xright;
        view.xrange = view.xright - view.xleft;
    }
    if (!view.fixed_y) {
        view.ylower = ylower;
        view.yupper = yupper;
        view.yrange = view.yupper - view.ylower;
    }

    if (view.xrange == 0.0 || view.yrange == 0.0) return;

    if (view.mode == SHOW_POSPOS) {
        SetPosPosAspectRatio(view.xrange, view.yrange);
    } else {
        view.scale_x = (double)view.box_w/view.xrange;
        view.scale_y = (double)view.box_h/(-view.yrange);
    }
}

/* Function to use when the user requested a different view of the data,
 * i.e. the user zoomed in or out. The arguments should be given in
 * X-Window coordinates, thus the transformation to the present user
 * coordinates are done here. (x1,y1) is the upper left point and
 * (x2,y2) is the lower right point.
 */
void SetWindowXCoord(int x1, int y1, int x2, int y2)
{
    double xleft, xright, ylower, yupper;
    
    double x2xunit(), y2yunit();

    xleft  = x2xunit(x1);
    xright = x2xunit(x2);
    ylower = y2yunit(y2);
    yupper = y2yunit(y1);
    SetWindow(xleft, xright, ylower, yupper);
}

void AdjustScale(Widget w, char *cmd, XtPointer cd)
{
    void SetDefWindow(), draw_main();
    
    if (strcmp(cmd, "x") == 0)
        SetDefWindow(SCALE_ONLY_X);
    else if (strcmp(cmd, "y") == 0)
        SetDefWindow(SCALE_ONLY_Y);
    else
        return;
    
    draw_main();
}

void AttachScale(Widget w, char *cmd, XtPointer cd)
{
    
    if (view.mode == SHOW_POSPOS || view.mode == SHOW_VELPOS ||
        view.mode == SHOW_POSVEL) {
        if (!view.m) return;
        if (strcmp(cmd, "Attach") == 0) {
            view.m->frame.use = 1;
            view.m->frame.x1 = view.xleft;
            view.m->frame.x2 = view.xright;
            view.m->frame.y1 = view.ylower;
            view.m->frame.y2 = view.yupper;
        } else {
            view.m->frame.use = 0;
        }
    } else if (view.mode == SHOW_SPE) {
        if (!view.s) return;
        if (strcmp(cmd, "Attach") == 0) {
            view.s->frame.use = 1;
            view.s->frame.x1 = view.xleft;
            view.s->frame.x2 = view.xright;
            view.s->frame.y1 = view.ylower;
            view.s->frame.y2 = view.yupper;
        } else {
            view.s->frame.use = 0;
        }
    } else if (view.mode == SHOW_SCATTER) {
        if (!view.p) return;
        if (strcmp(cmd, "Attach") == 0) {
            view.p->frame.use = 1;
            view.p->frame.x1 = view.xleft;
            view.p->frame.x2 = view.xright;
            view.p->frame.y1 = view.ylower;
            view.p->frame.y2 = view.yupper;
        } else {
            view.p->frame.use = 0;
        }
    }
}

void InvertScale(Widget w, char *cmd, XtPointer cd)
{
    void draw_main();
    
    if (strcmp(cmd, "x") == 0)
        SetWindow(view.xright, view.xleft, view.ylower, view.yupper);
    else if (strcmp(cmd, "y") == 0)
        SetWindow(view.xleft, view.xright, view.yupper, view.ylower);
    else
        return;
    
    draw_main();
}

void DoScroll(char *how, double magn)
{
    double dx, dy, lef, rig, upp, low;
    VIEW *v = &view;
    
    void draw_main();
    
    if (magn <= 0.0) return;

    if (v->mode == SHOW_ALLSPE) {
        dx = v->m->xspacing;
        dy = v->m->yspacing;
        lef = v->xleft + dx/2.0;  rig = v->xright - dx/2.0;
        upp = v->yupper - dy/2.0; low = v->ylower + dy/2.0;
    } else {
        dx = v->xrange * magn;
        dy = v->yrange * magn;
        lef = v->xleft;  rig = v->xright;
        upp = v->yupper; low = v->ylower;
    }
    
    if (strcmp(how, "u") == 0) { /* Scroll upwards */
        upp += dy;
        low += dy;
    } else if (strcmp(how, "d") == 0) { /* Scroll downwards */
        upp -= dy;
        low -= dy;
    } else if (strcmp(how, "l") == 0) { /* Scroll left */
        lef -= dx;
        rig -= dx;
    } else if (strcmp(how, "r") == 0) { /* Scroll right */
        lef += dx;
        rig += dx;
    }
    
    SetWindow(lef, rig, low, upp);
    draw_main();
}

void DoZoom(char *how, double magn)
{
    double dx = view.xrange;
    double dy = view.yrange;
    double x0 = (view.xleft + view.xright)/2.0;
    double y0 = (view.ylower + view.yupper)/2.0;
    double lef, rig, upp, low;
    
    void draw_main();
    
    if (magn <= 0.0) return;
    
    lef = view.xleft;
    rig = view.xright;
    upp = view.yupper;
    low = view.ylower;
    
    if (strcmp(how, "x") == 0 || strcmp(how, "b") == 0) {
        lef = x0 - dx*magn/2.0;
        rig = x0 + dx*magn/2.0;
    }
    if (strcmp(how, "y") == 0 || strcmp(how, "b") == 0) {
        low = y0 - dy*magn/2.0;
        upp = y0 + dy*magn/2.0;
    }
    
    SetWindow(lef, rig, low, upp);
    draw_main();
}

void DoMenuZoom(Widget w, char *how, XtPointer cd)
{
    if (strcmp(how, "xin") == 0) {
        DoZoom("x", 0.5);
    } else if (strcmp(how, "xout") == 0) {
        DoZoom("x", 2.0);
    } else if (strcmp(how, "yin") == 0) {
        DoZoom("y", 0.5);
    } else if (strcmp(how, "yout") == 0) {
        DoZoom("y", 2.0);
    } else if (strcmp(how, "bin") == 0) {
        DoZoom("b", 0.5);
    } else if (strcmp(how, "bout") == 0) {
        DoZoom("b", 2.0);
    }
}

/* Function to set the scales when a new plot mode is used. The function
 * sets the view parameters according to cover all data for the mode in
 * question.
 */
void SetDefWindow(int code)
{
    int x = 0, y = 0;

    void FindMapExtent();

    if (code == SCALE_NONE)
        return;

    if (code == SCALE_BOTH || code == SCALE_ONLY_X)
        x = 1;
    if (code == SCALE_BOTH || code == SCALE_ONLY_Y)
        y = 1;

    if (view.mode == SHOW_SPE && view.s) {
      if (view.use_attached_frame && view.s->frame.use) {
          view.xleft  = view.s->frame.x1;
          view.xright = view.s->frame.x2;
          view.ylower = view.s->frame.y1;
          view.yupper = view.s->frame.y2;
      } else {
        if (x && !view.fixed_x) {
            switch (view.xunit) {
                case UNIT_CHA:
                    view.xleft  = 0.0;
                    view.xright = (double)(view.s->nChan - 1);
                    view.xspacing = 1.0;
                    break;
                case UNIT_FRE:
                    view.xleft  = view.s->freq0;
                    view.xright = view.s->freq0 +
                                  (double)(view.s->nChan - 1)*view.s->freqres;
                    view.xspacing = view.s->freqres;
                    break;
                case UNIT_FMHZ:
                    view.xleft  = 1000.0 * view.s->freq0;
                    view.xright = 1000.0 * (view.s->freq0 +
                                  (double)(view.s->nChan - 1)*view.s->freqres);
                    view.xspacing = 1000.0 * view.s->freqres;
                    break;
                case UNIT_FOFF:
                    view.xleft  = 1000.0*(view.s->freq0 - view.xref);
                    view.xright = 1000.0*(view.s->freq0 - view.xref +
                                  (double)(view.s->nChan - 1)*view.s->freqres);
                    view.xspacing = 1000.0*(view.s->freqres);
                    break;
                case UNIT_VEL:
                    view.xleft  = view.s->vel0;
                    view.xright = view.s->vel0 +
                                  (double)(view.s->nChan - 1)*view.s->velres;
                    view.xspacing = view.s->velres;
                    break;
            }
        }
        if (y && !view.fixed_y) {
            if (view.s->s_min >= view.s->s_max) {
                view.ylower = 0.9*view.s->s_min;
                view.yupper = 1.1*view.s->s_max;
            } else {
                view.ylower = view.s->s_min -
                              0.1*(view.s->s_max - view.s->s_min);
                view.yupper = view.s->s_max +
                              0.1*(view.s->s_max - view.s->s_min);
            }
            view.yspacing = 0.0;
        }
      }
    } else if (view.mode == SHOW_ALLSPE && view.m) {
        if (x && !view.fixed_x) {
            view.xleft  = view.m->xleft  - view.m->xspacing*view.subXmagn/2.0;
            view.xright = view.m->xright + view.m->xspacing*view.subXmagn/2.0;
            view.xspacing = view.m->xspacing;
        }
        if (y && !view.fixed_y) {
            view.ylower = view.m->ylower - view.m->yspacing*view.subYmagn/2.0;
            view.yupper = view.m->yupper + view.m->yspacing*view.subYmagn/2.0;
            view.yspacing = view.m->yspacing;
        }
    } else if (view.mode == SHOW_SUBSPE) {
      if (view.use_attached_frame && view.s->frame.use) {
          view.xleft  = view.s->frame.x1;
          view.xright = view.s->frame.x2;
          view.ylower = view.s->frame.y1;
          view.yupper = view.s->frame.y2;
      } else {
        if (x && view.s && !view.fixed_x) {
            switch (view.xunit) {
                case UNIT_CHA:
                    view.xleft  = 0.0;
                    view.xright = (double)(view.s->nChan - 1);
                    view.xspacing = 1.0;
                    break;
                case UNIT_FRE:
                    view.xleft  = view.s->freq0;
                    view.xright = view.s->freq0 +
                                  (double)(view.s->nChan - 1)*view.s->freqres;
                    view.xspacing = view.s->freqres;
                    break;
                case UNIT_FMHZ:
                    view.xleft  = 1000.0 * view.s->freq0;
                    view.xright = 1000.0 * (view.s->freq0 +
                                  (double)(view.s->nChan - 1)*view.s->freqres);
                    view.xspacing = 1000.0 * view.s->freqres;
                    break;
                case UNIT_FOFF:
                    view.xleft  = 1000.0*(view.s->freq0 - view.xref);
                    view.xright = 1000.0*(view.s->freq0 - view.xref +
                                  (double)(view.s->nChan - 1)*view.s->freqres);
                    view.xspacing = 1000.0*(view.s->freqres);
                    break;
                case UNIT_VEL:
                    view.xleft  = view.s->vel0;
                    view.xright = view.s->vel0 +
                                  (double)(view.s->nChan - 1)*view.s->velres;
                    view.xspacing = view.s->velres;
                    break;
            }
        }
        if (y && !view.fixed_y) {
            if (view.s && view.autoscale_y) {
                if (view.s->s_min >= view.s->s_max) {
                    view.ylower = 0.9*view.s->s_min;
                    view.yupper = 1.1*view.s->s_max;
                } else {
                    view.ylower = view.s->s_min -
                                  0.1*(view.s->s_max - view.s->s_min);
                    view.yupper = view.s->s_max +
                                  0.1*(view.s->s_max - view.s->s_min);
                }
            } else if (view.m) {
                view.ylower = view.m->gt_min -
                              0.1*(view.m->gt_max - view.m->gt_min);
                view.yupper = view.m->gt_max +
                              0.1*(view.m->gt_max - view.m->gt_min);
            }
            if (view.m) view.yspacing = view.m->yspacing;
        }
      }
    } else if (view.mode == SHOW_POSPOS && view.m) {
        if (x && !view.fixed_x) {
            FindMapExtent(view.m, &view.xleft, &view.xright, NULL, NULL);
            view.xspacing = view.m->xspacing;
        }
        if (y && !view.fixed_y) {
            FindMapExtent(view.m, NULL, NULL, &view.ylower, &view.yupper);
            view.yspacing = view.m->yspacing;
        }
        if (view.use_attached_frame && view.m->frame.use) {
            view.xleft  = view.m->frame.x1;
            view.xright = view.m->frame.x2;
            view.ylower = view.m->frame.y1;
            view.yupper = view.m->frame.y2;
        }
    } else if (view.mode == SHOW_SCATTER && view.p) {
        if (x && !view.fixed_x) {
            view.xleft  = view.p->xmin - 0.1*(view.p->xmax - view.p->xmin);
            view.xright = view.p->xmax + 0.1*(view.p->xmax - view.p->xmin);
            view.xspacing = 0.0;
        }
        if (y && !view.fixed_y) {
            view.ylower = view.p->ymin - 0.1*(view.p->ymax - view.p->ymin);
            view.yupper = view.p->ymax + 0.1*(view.p->ymax - view.p->ymin);
            view.yspacing = 0.0;
        }
        if (view.use_attached_frame && view.p->frame.use) {
            view.xleft  = view.p->frame.x1;
            view.xright = view.p->frame.x2;
            view.ylower = view.p->frame.y1;
            view.yupper = view.p->frame.y2;
        }
    } else if ((view.mode == SHOW_VELPOS || view.mode == SHOW_POSVEL) &&
               view.m) {
        if (x && !view.fixed_x) {
            view.xleft  = view.m->xleft;
            view.xright = view.m->xright;
            view.xspacing = view.m->xspacing;
        }
        if (y && !view.fixed_y) {
            view.ylower = view.m->ylower;
            view.yupper = view.m->yupper;
            view.yspacing = view.m->yspacing;
        }
        if (view.use_attached_frame && view.m->frame.use) {
            view.xleft  = view.m->frame.x1;
            view.xright = view.m->frame.x2;
            view.ylower = view.m->frame.y1;
            view.yupper = view.m->frame.y2;
        }
    }

    view.xrange = view.xright - view.xleft;
    view.yrange = view.yupper - view.ylower;
    if (view.xrange == 0.0 || view.yrange == 0.0)
        return;

    if (view.mode == SHOW_POSPOS) {
        SetPosPosAspectRatio(view.xrange, view.yrange);
    } else {
        if (x)
            view.scale_x = (double)view.box_w/view.xrange;
        if (y)
            view.scale_y = (double)view.box_h/(-view.yrange);
    }
}

char *GetXLabel()
{
    static string label;
    
    switch (view.mode) {
        case SHOW_SPE:
        case SHOW_VELPOS:
        case SHOW_SUBSPE:
            if (view.xunit == UNIT_FRE)
                strcpy(label, x1_label);
            else if (view.xunit == UNIT_VEL)
                strcpy(label, x2_label);
            else if (view.xunit == UNIT_CHA)
                strcpy(label, x3_label);
            else if (view.xunit == UNIT_FOFF)
                strcpy(label, x4_label);
            else if (view.xunit == UNIT_FMHZ)
                strcpy(label, x5_label);
            break;
        case SHOW_ALLSPE:
        case SHOW_POSPOS:
            strcpy(label, RA_label);
            if (!view.m) break;
            switch (view.m->coordType) {
                case COORD_TYPE_EQU:
                    break;
                case COORD_TYPE_HOR:
                    strcpy(label, "Azimuth offset [\"]");
                    break;
                case COORD_TYPE_GAL:
                    strcpy(label, "Gal. longitude offset [\"]");
                    break;
            }
            break;
        case SHOW_POSVEL:
            strcpy(label, Pos_label);
            break;
        case SHOW_SCATTER:
            strcpy(label, "Distance [\"]");
            if (!view.p) break;
            switch (view.p->xtype) {
                case XTYPE_SCA_NO:
                    strcpy(label, "Running number");
                    break;
                case XTYPE_SCA_SCAN:
                    strcpy(label, "Scan number");
                    break;
                case XTYPE_SCA_RECT:
                    strcpy(label, "RA [hr]");
                    break;
                case XTYPE_SCA_DECL:
                    strcpy(label, "Dec [degr]");
                    break;
                case XTYPE_SCA_RA:
                    strcpy(label, "RA offset [\"]");
                    break;
                case XTYPE_SCA_DEC:
                    strcpy(label, "Dec offset [\"]");
                    break;
                case XTYPE_SCA_EQDIST:
                case XTYPE_SCA_DIST:
                    strcpy(label, "Distance [\"]");
                    break;
                case XTYPE_SCA_POSANG:
                    strcpy(label, "Position angle [deg]");
                    break;
                case XTYPE_SCA_EL:
                    strcpy(label, "Elevation [deg]");
                    break;
                case XTYPE_SCA_OSOAZ:
                case XTYPE_SCA_AZ:
                    strcpy(label, "Azimuth [deg]");
                    break;
                case XTYPE_SCA_ELOFF:
                    strcpy(label, "Elevation offset [\"]");
                    break;
                case XTYPE_SCA_AZOFF:
                    strcpy(label, "Azimuth offset [\"]");
                    break;
                case XTYPE_SCA_INT:
                    strcpy(label, "Integrated intensity [K km/s]");
                    break;
                case XTYPE_SCA_MEAN:
                    strcpy(label, "Mean intensity [K]");
                    break;
                case XTYPE_SCA_SIGMA:
                    strcpy(label, "Sigma [K]");
                    break;
                case XTYPE_SCA_TSYS:
                    strcpy(label, "System temperature [K]");
                    break;
                case XTYPE_SCA_TSQRT:
                    strcpy(label, "T\\dsys\\u/sqrt(tB) [K]");
                    break;
                case XTYPE_SCA_TAU:
                    strcpy(label, "Atm. opacity [neper]");
                    break;
                case XTYPE_SCA_EXPTAU:
                    strcpy(label, "Atm. exp(tau)-1");
                    break;
                case XTYPE_SCA_DATE:
                    strcpy(label, "Date.UT");
                    break;
                case XTYPE_SCA_UT:
                    strcpy(label, "UT");
                    break;
                case XTYPE_SCA_JD:
                    strcpy(label, "Julian Day");
                    break;
                case XTYPE_SCA_MJD:
                    strcpy(label, "Modified Julian Day");
                    break;
                case XTYPE_SCA_GAMP:
                    strcpy(label, "Amplitude [K]");
                    break;
                case XTYPE_SCA_GWID:
                    strcpy(label, "Velocity width [km/s]");
                    break;
                case XTYPE_SCA_GCEN:
                    strcpy(label, "Gaussian centre");
                    break;
                case XTYPE_SCA_POL0:
                    strcpy(label, "Baseline [K]");
                    break;
                case XTYPE_SCA_POL1:
                    strcpy(label, "Baseline slope [K/(km/s)]");
                    break;
                case XTYPE_SCA_VCENT:
                    strcpy(label, "Centroid velocity [km/s]");
                    break;
                case XTYPE_SCA_V2MOM:
                    strcpy(label, "2nd moment velocity [km/s]");
                    break;
                case XTYPE_SCA_VELO:
                case XTYPE_SCA_VRES:
                    strcpy(label, "Velocity [km/s]");
                    break;
                case XTYPE_SCA_FREQ:
                    strcpy(label, "Frequency [GHz]");
                    break;
                case XTYPE_SCA_FRES:
                    strcpy(label, "Frequency [kHz]");
                    break;
                case XTYPE_SCA_CHAN:
                    strcpy(label, "Channel number");
                    break;
                case XTYPE_SCA_TEMP:
                    strcpy(label, "Temperature [K]");
                    break;
                case XTYPE_SCA_TMAX:
                    strcpy(label, "Max. intensity  [K]");
                    break;
                case XTYPE_SCA_TMIN:
                    strcpy(label, "Min. intensity  [K]");
                    break;
                case XTYPE_SCA_PDIST:
                    strcpy(label, "Distance  [pixels]");
                    break;
                case XTYPE_SCA_CORR:
                    strcpy(label, "Correlation coeffcient");
                    break;
                case XTYPE_SCA_POLA_EL:
                case XTYPE_SCA_POLA_AZ:
                    strcpy(label, "Polarization angle  [deg]");
                    break;
                case XTYPE_SCA_COSPA:
                    strcpy(label, "Cos(Polarization angle)");
                    break;
                case XTYPE_SCA_BEFF:
                    strcpy(label, "Main beam efficiency");
                    break;
                case XTYPE_SCA_AIRMASS:
                    strcpy(label, "Air mass");
                    break;
                case XTYPE_SCA_TAIR:
                    strcpy(label, "Air temp. [K]");
                    break;
                case XTYPE_SCA_PAIR:
                    strcpy(label, "Air pressure [mb]");
                    break;
                case XTYPE_SCA_RAIR:
                    strcpy(label, "Air relative hum. [%]");
                    break;
            }
            break;
    }
    
    return label;
}

char *GetYLabel()
{
    static string label;
    
    switch (view.mode) {
        case SHOW_SPE:
        case SHOW_SUBSPE:
            strcpy(label, "Intensity [K]");
            break;
        case SHOW_POSVEL:
            if (view.yunit == UNIT_FRE)
                strcpy(label, x1_label);
            else if (view.yunit == UNIT_VEL)
                strcpy(label, x2_label);
            else if (view.yunit == UNIT_CHA)
                strcpy(label, x3_label);
            else if (view.yunit == UNIT_FOFF)
                strcpy(label, x4_label);
            else if (view.yunit == UNIT_FMHZ)
                strcpy(label, x5_label);
            break;
        case SHOW_ALLSPE:
        case SHOW_POSPOS:
            strcpy(label, Dec_label);
            if (!view.m) break;
            switch (view.m->coordType) {
                case COORD_TYPE_EQU:
                    break;
                case COORD_TYPE_HOR:
                    strcpy(label, "Elevation offset [\"]");
                    break;
                case COORD_TYPE_GAL:
                    strcpy(label, "Gal. latitude offset [\"]");
                    break;
            }
            break;
        case SHOW_VELPOS:
            strcpy(label, Pos_label);
            break;
        case SHOW_SCATTER:
            strcpy(label, "Intensity [K km/s]");
            if (!view.p) break;
            switch (view.p->ytype) {
                case YTYPE_SCA_NO:
                    strcpy(label, "Running number");
                    break;
                case YTYPE_SCA_SCAN:
                    strcpy(label, "Scan number");
                    break;
                case YTYPE_SCA_RECT:
                    strcpy(label, "RA [hr]");
                    break;
                case YTYPE_SCA_DECL:
                    strcpy(label, "Dec [degr]");
                    break;
                case YTYPE_SCA_RA:
                    strcpy(label, "RA offset [\"]");
                    break;
                case YTYPE_SCA_DEC:
                    strcpy(label, "Dec offset [\"]");
                    break;
                case YTYPE_SCA_EQDIST:
                case YTYPE_SCA_DIST:
                    strcpy(label, "Distance [\"]");
                    break;
                case YTYPE_SCA_POSANG:
                    strcpy(label, "Position angle [deg]");
                    break;
                case YTYPE_SCA_EL:
                    strcpy(label, "Elevation [deg]");
                    break;
                case YTYPE_SCA_OSOAZ:
                case YTYPE_SCA_AZ:
                    strcpy(label, "Azimuth [deg]");
                    break;
                case YTYPE_SCA_ELOFF:
                    strcpy(label, "Elevation offset [\"]");
                    break;
                case YTYPE_SCA_AZOFF:
                    strcpy(label, "Azimuth offset [\"]");
                    break;
                case YTYPE_SCA_INT:
                    strcpy(label, "Integrated intensity [K km/s]");
                    break;
                case YTYPE_SCA_MEAN:
                    strcpy(label, "Mean intensity [K]");
                    break;
                case YTYPE_SCA_SIGMA:
                    strcpy(label, "Sigma [K]");
                    break;
                case YTYPE_SCA_TSYS:
                    strcpy(label, "System temperature [K]");
                    break;
                case YTYPE_SCA_TSQRT:
                    strcpy(label, "T\\dsys\\u/sqrt(tB)  [K]");
                    break;
                case YTYPE_SCA_TAU:
                    strcpy(label, "Atm. opacity [neper]");
                    break;
                case YTYPE_SCA_EXPTAU:
                    strcpy(label, "Atm. exp(tau)-1");
                    break;
                case YTYPE_SCA_DATE:
                    strcpy(label, "Date.UT");
                    break;
                case YTYPE_SCA_UT:
                    strcpy(label, "UT");
                    break;
                case YTYPE_SCA_JD:
                    strcpy(label, "Julian Day");
                    break;
                case YTYPE_SCA_MJD:
                    strcpy(label, "Modified Julian Day");
                    break;
                case YTYPE_SCA_GAMP:
                    strcpy(label, "Amplitude [K]");
                    break;
                case YTYPE_SCA_GWID:
                    strcpy(label, "Velocity width [km/s]");
                    break;
                case YTYPE_SCA_GCEN:
                    strcpy(label, "Gaussian centre");
                    break;
                case YTYPE_SCA_POL0:
                    strcpy(label, "Baseline [K]");
                    break;
                case YTYPE_SCA_POL1:
                    strcpy(label, "Baseline slope [K/(km/s)]");
                    break;
                case YTYPE_SCA_VCENT:
                    strcpy(label, "Centroid velocity [km/s]");
                    break;
                case YTYPE_SCA_V2MOM:
                    strcpy(label, "2nd moment velocity [km/s]");
                    break;
                case YTYPE_SCA_VELO:
                case YTYPE_SCA_VRES:
                    strcpy(label, "Velocity [km/s]");
                    break;
                case YTYPE_SCA_FREQ:
                    strcpy(label, "Frequency [GHz]");
                    break;
                case YTYPE_SCA_FRES:
                    strcpy(label, "Frequency [kHz]");
                    break;
                case YTYPE_SCA_CHAN:
                    strcpy(label, "Channel number");
                    break;
                case YTYPE_SCA_TEMP:
                    strcpy(label, "Temperature [K]");
                    break;
                case YTYPE_SCA_TMAX:
                    strcpy(label, "Max. intensity  [K]");
                    break;
                case YTYPE_SCA_TMIN:
                    strcpy(label, "Min. intensity  [K]");
                    break;
                case YTYPE_SCA_PDIST:
                    strcpy(label, "Distance  [pixels]");
                    break;
                case YTYPE_SCA_CORR:
                    strcpy(label, "Correlation coeffcient");
                    break;
                case YTYPE_SCA_POLA_EL:
                case YTYPE_SCA_POLA_AZ:
                    strcpy(label, "Polarization angle  [deg]");
                    break;
                case YTYPE_SCA_COSPA:
                    strcpy(label, "Cos(Polarization angle)");
                    break;
                case YTYPE_SCA_BEFF:
                    strcpy(label, "Main beam efficiency");
                    break;
                case YTYPE_SCA_AIRMASS:
                    strcpy(label, "Air mass");
                    break;
                case YTYPE_SCA_TAIR:
                    strcpy(label, "Air temp. [K]");
                    break;
                case YTYPE_SCA_PAIR:
                    strcpy(label, "Air pressure [mb]");
                    break;
                case YTYPE_SCA_RAIR:
                    strcpy(label, "Air relative hum. [%]");
                    break;
            }
            break;
    }
    
    return label;
}

char *GetWedgeLabel()
{
    static string label;
    
    strcpy(label, "");
    
    switch (view.mode) {
        case SHOW_POSVEL:
        case SHOW_VELPOS:
            strcpy(label, "Intensity [K]");
            break;
        case SHOW_WEDGE:
        case SHOW_POSPOS:
            strcpy(label, "Intensity [K km/s]");
            if (!view.m) break;
            switch (view.m->zType) {
                case ZTYPE_MOMENT:
                    strcpy(label, "Intensity [K km/s]");
                    break;
                case ZTYPE_MEAN:
                    strcpy(label, "Intensity [K]");
                    break;
                case ZTYPE_GAMP:
                    strcpy(label, "Amplitude [K]");
                    break;
                case ZTYPE_GCEN:
                    strcpy(label, "Centre vel. [km/s]");
                    break;
                case ZTYPE_GWID:
                    strcpy(label, "Vel. width [km/s]");
                    break;
                case ZTYPE_POL0:
                    strcpy(label, "Fitted offset [K]");
                    break;
                case ZTYPE_POL1:
                    strcpy(label, "Fitted slope");
                    break;
                case ZTYPE_VCENT:
                    strcpy(label, "Centroid vel. [km/s]");
                    break;
                case ZTYPE_V2MOM:
                    strcpy(label, "2nd vel. mom. [km/s]");
                    break;
                case ZTYPE_TMAX:
                    strcpy(label, "Maximum intensity [K]");
                    break;
                case ZTYPE_TMIN:
                    strcpy(label, "Minimum intensity [K]");
                    break;
                case ZTYPE_TRMS:
                    strcpy(label, "Intensity RMS [K]");
                    break;
                default:
                    break;
            }
            break;
        case SHOW_SPE:
        case SHOW_SUBSPE:
        case SHOW_ALLSPE:
        case SHOW_SCATTER:
        default:
            break;
    }
    
    return label;
}

static char *GetAnyLabel(int type)
{
    int is_seq=0;
    double val, err;
    static string label;
    
    double xmap(scanPtr), ymap(scanPtr);
    double SpecUnitConv();
    
    strcpy(label, "");
    
    switch (view.mode) {
        case SHOW_SPE:
        case SHOW_SUBSPE:
            switch (type) {
                case TLAB_NONE:
                    break;
                case TLAB_SOURCE:
                    if (view.s) strcpy(label, view.s->name);
                    break;
                case TLAB_MOLECULE:
                    if (view.s) strcpy(label, view.s->molecule);
                    break;
                case TLAB_DATE:
                    if (view.s) sprintf(label, "%4d%02d%02d",
                                        view.s->date.Year,
                                        view.s->date.Month,
                                        view.s->date.Day);
                    break;
                case TLAB_TIME:
                    if (view.s) sprintf(label, "%2dh%02dm%02ds",
                                        view.s->date.Hour,
                                        view.s->date.Min,
                                        view.s->date.Sec);
                    break;
                case TLAB_POSITION:
                    is_seq = (view.from->sequence);
                    if (view.s) sprintf(label, "(%+6.1f,%+6.1f)",
                                        is_seq ? view.s->tx : xmap(view.s),
                                        is_seq ? view.s->ty : ymap(view.s));
                    break;
                case TLAB_INTINT:
                    if (!view.s) break;
                    val = view.s->mom.iint;
                    err = view.s->mom.iunc;
		    if (draw.yebars) {
                      if (fabs(val) >= 10.0)
                          sprintf(label, "%5.1f(%.1f)", val, err);
                      else if (fabs(val) >= 1.0)
                          sprintf(label, "%5.2f(%.2f)", val, err);
                      else
                          sprintf(label, "%6.3f(%.3f)", val, err);
		    } else {
                      if (fabs(val) >= 10.0)
                          sprintf(label, "%5.1f", val);
                      else if (fabs(val) >= 1.0)
                          sprintf(label, "%5.2f", val);
                      else
                          sprintf(label, "%6.3f", val);
		    }
                    break;
                case TLAB_SCANNO:
                    if (view.s) sprintf(label, "%d", view.s->scan_no);
                    break;
                case TLAB_RESTFQ:
                    if (!view.s) break;
		    val = view.s->freq0 + (double)(view.s->nChan/2 + 1) *
		          view.s->freqres;
		    if (val < 100.0)
		        sprintf(label, "%7.1f MHz", val*1.0e3);
		    else if (val < 1000.0)
		        sprintf(label, "%7.3f GHz", val);
		    else
		        sprintf(label, "%.2f \\gmm", SPEEDOFLIGHT/val);
                    break;
                case TLAB_GAUSS:
                    if (view.s) {
		      if (view.s->gaussFit) {
		        double v1, v2;
                        v1 = SpecUnitConv(view.xunit, UNIT_CHA,
			                  view.s->g.cen - view.s->g.wid/2.0);
                        v2 = SpecUnitConv(view.xunit, UNIT_CHA,
			                  view.s->g.cen + view.s->g.wid/2.0);
		        sprintf(label, "a=%f w=%f c=%f",
			        view.s->g.amp,
				fabs(v1 - v2),
				SpecUnitConv(view.xunit, UNIT_CHA,
				             view.s->g.cen));
		      } else {
		        sprintf(label, "No fit");
		      }
		    }
                    break;
            }
            break;
        case SHOW_ALLSPE:
            switch (type) {
                case TLAB_POSITION:
                    is_seq = (view.from->sequence);
                    if (view.s) sprintf(label, "(%+6.1f,%+6.1f)",
                                        is_seq ? view.s->tx : xmap(view.s),
                                        is_seq ? view.s->ty : ymap(view.s));
                    break;
                case TLAB_INTINT:
                    if (!view.s) break;
                    val = view.s->mom.iint;
                    err = view.s->mom.iunc;
                    if (fabs(val) >= 10.0)
                        sprintf(label, "%5.1f(%.1f)", val, err);
                    else if (fabs(val) >= 1.0)
                        sprintf(label, "%5.2f(%.2f)", val, err);
                    else
                        sprintf(label, "%6.3f(%.3f)", val, err);
                    break;
                case TLAB_SCANNO:
                    if (view.s) sprintf(label, "%d", view.s->scan_no);
                    break;

            }
        case SHOW_POSVEL:
        case SHOW_POSPOS:
        case SHOW_VELPOS:
            switch (type) {
                case TLAB_NONE:
                    break;
                case TLAB_SOURCE:
                    if (view.m) strcpy(label, view.m->name);
                    break;
                case TLAB_MOLECULE:
                    if (view.m) strcpy(label, view.m->molecule);
                    break;
                case TLAB_DATE:
                    if (view.m) sprintf(label, "%4d%02d%02d",
                                        view.m->date.Year,
                                        view.m->date.Month,
                                        view.m->date.Day);
                    break;
                case TLAB_TIME:
                    if (view.m) sprintf(label, "%2dh%02dm%02ds",
                                        view.m->date.Hour,
                                        view.m->date.Min,
                                        view.m->date.Sec);
                    break;
                case TLAB_RESTFQ:
                    if (view.m) sprintf(label, "%9.1f MHz", view.m->fMHz);
                    break;
            }
            break;
        case SHOW_SCATTER:
            switch (type) {
                case TLAB_NONE:
                    break;
                case TLAB_SOURCE:
                    if (view.p) strcpy(label, view.p->name);
                    break;
                case TLAB_MOLECULE:
                    if (view.p) strcpy(label, view.p->molecule);
                    break;
                case TLAB_DATE:
                    if (view.p) sprintf(label, "%4d%02d%02d",
                                        view.p->date.Year,
                                        view.p->date.Month,
                                        view.p->date.Day);
                    break;
                case TLAB_TIME:
                    if (view.p) sprintf(label, "%2dh%02dm%02ds",
                                        view.p->date.Hour,
                                        view.p->date.Min,
                                        view.p->date.Sec);
                    break;
            }
            break;
    }
    
    return label;
}

char *GetTopLabel()
{
    static char buf[512];
    
    if (view.slab_type == TLAB_NONE) {
        return GetAnyLabel(view.tlab_type);
    } else if (view.tlab_type == TLAB_NONE) {
        return GetAnyLabel(view.slab_type);
    } else {
        strcpy(buf, GetAnyLabel(view.tlab_type));
        strcat(buf, "  ");
        strcat(buf, GetAnyLabel(view.slab_type));
        return buf;
    }
}

char *GetLeftLabel()
{
    return GetAnyLabel(view.llab_type);
}

char *GetRightLabel()
{
    return GetAnyLabel(view.rlab_type);
}

char *GetUnitString(int unit)
{
    static string ustr;
    
    switch (unit) {
        case UNIT_FRE:
        case UNIT_VEL:
        case UNIT_CHA:
        case UNIT_ASEC:
        case UNIT_AMIN:
        case UNIT_FOFF:
            strcpy(ustr, unit_labels[unit]);
            break;
        default:
            strcpy(ustr, "");
            break;
    }
    
    return ustr;
}

void ChangeSpecUnit(int newUnit)
{
    double first, last;
    VIEW *v;

    double SpecUnitConv();

    if (view.mode != SHOW_SPE && view.mode != SHOW_ALLSPE &&
        view.mode != SHOW_VELPOS && view.mode != SHOW_POSVEL)
        return;
        
    if (view.fixed_x) {
        PostWarningDialog(NULL,
                          "The x-scale is fixed. Cannot change units now.");
        return;
    }

    if (view.mode == SHOW_POSVEL) {
        if (newUnit == view.yunit) return;
        first = SpecUnitConv(newUnit, view.yunit, view.ylower);
        last  = SpecUnitConv(newUnit, view.yunit, view.yupper);
        view.yunit = newUnit;
        SetWindow(view.xleft, view.xright, first, last);
    } else {
        if (view.mode == SHOW_ALLSPE)
            v = &scanview;
        else
            v = &view;
        if (newUnit == v->xunit) return;
        first = SpecUnitConv(newUnit, v->xunit, v->xleft);
        last  = SpecUnitConv(newUnit, v->xunit, v->xright);
        v->xunit = newUnit;
        if (view.mode == SHOW_ALLSPE) {
            view.xunit = newUnit;
            v->xleft  = first;
            v->xright = last;
        } else 
            SetWindow(first, last, v->ylower, v->yupper);
    }
}

double SpecUnitConv(int newUnit, int oldUnit, double old)
{
    double newO, oldO, newR, oldR;

    if (newUnit == oldUnit)
        return old;

    if (!view.s)
        return UNDEF;

    if (newUnit == UNIT_CHA) {
        newO = 0.0;
        newR = 1.0;
    } else if (newUnit == UNIT_VEL) {
        newO = view.s->vel0;
        newR = view.s->velres;
    } else if (newUnit == UNIT_FRE) {
        newO = view.s->freq0;
        newR = view.s->freqres;
    } else if (newUnit == UNIT_FMHZ) {
        newO = 1000.0*(view.s->freq0);
        newR = 1000.0*(view.s->freqres);
    } else if (newUnit == UNIT_FOFF) {
        newO = 1000.0*(view.s->freq0 - view.xref);
        newR = 1000.0*(view.s->freqres);
    } else
        return UNDEF;

    if (oldUnit == UNIT_CHA) {
        oldO = 0.0;
        oldR = 1.0;
    } else if (oldUnit == UNIT_VEL) {
        oldO = view.s->vel0;
        oldR = view.s->velres;
    } else if (oldUnit == UNIT_FRE) {
        oldO = view.s->freq0;
        oldR = view.s->freqres;
    } else if (oldUnit == UNIT_FMHZ) {
        oldO = 1000.0*(view.s->freq0);
        oldR = 1000.0*(view.s->freqres);
    } else if (oldUnit == UNIT_FOFF) {
        oldO = 1000.0*(view.s->freq0 - view.xref);
        oldR = 1000.0*(view.s->freqres);
    } else
        return UNDEF;

    if (oldR == 0.0)
        return UNDEF;

    return newO + (old - oldO)/oldR * newR;
}

double SpecUnitRes(scanPtr s, int unit)
{
    scanPtr S = view.s;
    
    if (s) S = s;
    
    switch (unit) {
        case UNIT_CHA:
            return 1.0;
        case UNIT_VEL:
            return S->velres;
        case UNIT_FRE:
            return S->freqres;
        case UNIT_FMHZ:
        case UNIT_FOFF:
            return 1000.0*(S->freqres);
        default:
            return UNDEF;
    }
}

double SpecUnitBegin(scanPtr s, int unit)
{
    scanPtr S = view.s;
    
    if (s) S = s;
    
    switch (unit) {
        case UNIT_CHA:
            return 0.0;
        case UNIT_VEL:
            return S->vel0;
        case UNIT_FRE:
            return S->freq0;
        case UNIT_FMHZ:
        case UNIT_FOFF:
            return 1000.0*(S->freq0);
        default:
            return UNDEF;
    }
}

double SpecUnitEnd(scanPtr s, int unit)
{
    scanPtr S = view.s;
    
    if (s) S = s;
    
    switch (unit) {
        case UNIT_CHA:
            return (double)(S->nChan - 1);
        case UNIT_VEL:
            return S->vel0 + (double)(S->nChan - 1)* S->velres;
        case UNIT_FRE:
            return S->freqn;
        case UNIT_FMHZ:
        case UNIT_FOFF:
            return 1000.0*(S->freqn);
        default:
            return UNDEF;
    }
}

double x2xunit(int x)
{
    int i;
    MAP *m;
    double val;

    val = view.xleft + (double)(x - view.min_x)/view.scale_x;
    if (view.mode == SHOW_ALLSPE) {
        m = view.m;
        if (m) {
            i = NINT((val - m->xleft)/m->xspacing);
            val = m->xleft + (double)i * m->xspacing;
        }
    }
    return val;
}

double y2yunit(int y)
{
    int j;
    MAP *m;
    double val;

    val = view.ylower + (double)(y - view.min_y)/view.scale_y;
    if (view.mode == SHOW_ALLSPE) {
        m = view.m;
        if (m) {
            j = NINT((val - m->ylower)/m->yspacing);
            val = m->ylower + (double)j * m->yspacing;
        }
    }
    return val;
}

int xunit2x(double x)
{
    return view.min_x + NINT((x - view.xleft)*view.scale_x);
}

int yunit2y(double y)
{
    return view.min_y + NINT((y - view.ylower)*view.scale_y);
}

void uv2xy(Point p, double a, Point *r)
{
    double cp = cos(a/RADTODEG);
    double sp = sin(a/RADTODEG);
    
    if (r) r->x = cp*p.x + sp*p.y;
    if (r) r->y = cp*p.y - sp*p.x;
}

void uvunit2xy(Point p, double a, int *rot_x, int *rot_y)
{
    Point r;
    
    uv2xy(p, a, &r);
    
    if (rot_x) *rot_x = view.min_x + NINT((r.x - view.xleft)*view.scale_x);
    if (rot_y) *rot_y = view.min_y + NINT((r.y - view.ylower)*view.scale_y);
}

double MollweideTheta(double Theta)
{
    int n=0;
    double MT, dT=1.0, t = Theta/RADTODEG;
    
    MT = 5.0*t/6.0;
    while (fabs(dT) > 1.0e-7 || n < 5) {
        dT = (PI*sin(t) - 2.0*MT - sin(2.0*MT))/(2.0 + 2.0*cos(2.0*MT));
        MT += dT;
        n++;
        if (n > 50) break;
    }
    
    return MT;
}

void Orthographic(double p, double t, double p0, double t0,
                  double *x, double *y)
{
    if (x) *x = cos(t)*sin(p-p0);
    if (y) *y = cos(t0)*sin(t) - sin(t0)*cos(t)*cos(p-p0);
}

void pt2xy(double p, double t, int type, Point *r)
{
    double a=0.0, R=0.0;
    
    switch (type) {
        case 0:  /* None */
            if (r) r->x = p;
            if (r) r->y = t;
            break;
        case 1:  /* Sinusodial projection (Sanson-Flamstead) */
            if (r) r->x = p * sin(t/RADTODEG);
            if (r) r->y = t;
            break;
        case 2:  /* (Co)Sinusodial projection (Sanson-Flamstead) */
            if (r) r->x = p * cos(t/RADTODEG);
            if (r) r->y = t;
            break;
        case 3:  /* Mollweide */
            a = MollweideTheta(t);
            if (r) r->x = p * cos(a);
            if (r) r->y = 90.0 * sin(a);
            break;
        case 4:  /* Hammer-Aitoff */
            R = sqrt(1.0 - cos(t/RADTODEG)*cos(p/RADTODEG/2.0));
            a = atan2(sin(p/RADTODEG/2.0), tan(t/RADTODEG));
            if (r) r->x = 180.0 * R * sin(a);
            if (r) r->y = 90.0 * R * cos(a);
            break;
        case 5:  /* Parabolic */
            if (r) r->x = p * (2.0*cos(t/RADTODEG/3.0) - 1.0);
            if (r) r->y = 90.0 * 2.0*sin(t/RADTODEG/3.0);
            break;
        case 6: /* Orthographic: (Phi,Theta)=(0.0:Pi, 0.0) */
            if (fabs(p) <= 90.0) {
                Orthographic(p/RADTODEG, t/RADTODEG, 0.0, 0.0, &a, &R);
                if (r) r->x = 90.0 * a - 90.0;
            } else {
                Orthographic(p/RADTODEG, t/RADTODEG, PI, 0.0, &a, &R);
                if (r) r->x = 90.0 * a + 90.0;
            }
            if (r) r->y = 90.0 * R;
            break;
        case 7: /* Orthographic: (Phi,Theta)=(0.0, +/-Pi/2) */
            if (t >= 0.0) {
                Orthographic(p/RADTODEG, t/RADTODEG, 0.0, PI/2.0, &a, &R);
                if (r) r->x = 90.0 * a - 90.0;
            } else {
                Orthographic(p/RADTODEG, t/RADTODEG, 0.0, -PI/2.0, &a, &R);
                if (r) r->x = 90.0 * a + 90.0;
            }
            if (r) r->y = 90.0 * R;
            break;
        case 8: /* Orthographic: (Phi,Theta)=(+/-Pi/2, 0.0) */
            if (p <= 0.0 && p >= -180.0) {
                Orthographic(p/RADTODEG, t/RADTODEG, -PI/2.0, 0.0, &a, &R);
                if (r) r->x = 90.0 * a - 90.0;
            } else {
                Orthographic(p/RADTODEG, t/RADTODEG, +PI/2.0, 0.0, &a, &R);
                if (r) r->x = 90.0 * a + 90.0;
            }
            if (r) r->y = 90.0 * R;
            break;
    }
}

void ptunit2xy(double p, double t, int type, int *proj_x, int *proj_y)
{
    Point r;
    
    pt2xy(p, t, type, &r);
    
    if (proj_x) *proj_x = view.min_x + NINT((r.x - view.xleft)*view.scale_x);
    if (proj_y) *proj_y = view.min_y + NINT((r.y - view.ylower)*view.scale_y);
}

void pt4xy(double p1, double p2, double t1, double t2, int type,
           Point *r)
{
    switch (type) {
        case 0:
        case 1:
        case 2:
        case 4:
        case 5:
            if (t2 > 90.0 && t1 < 90.0) {
                t2 = 90.0;
            } else if (t1 < -90.0 && t2 > -90.0) {
                t1 = -90.0;
            }
            break;
        case 3:
            if (t2 > 90.0) t2 = 90.0;
            if (t1 > 90.0) t1 = 90.0;
            if (t2 < -90.0) t2 = -90.0;
            if (t1 < -90.0) t1 = -90.0;
            break;
        case 6:
            if (fabs(p1) < 90.0 && fabs(p2) > 90.0) {
                p2 = (p2 < 0.0) ? -90.0 : 90.0;
            } else if (fabs(p1) > 90.0 && fabs(p2) < 90.0) {
                p1 = (p1 < 0.0) ? -90.0: 90.0;
            } else if (fabs(p1) < 180.0 && fabs(p2) > 180.0) {
                p2 = (p2 < 0.0) ? -180.0 : 180.0;
            }
            break;
        case 7:
            if (t2 > 90.0 && t1 < 90.0) {
                t2 = 90.0;
            } else if (t2 > 0.0 && t1 < 0.0) {
                t1 = 0.0;
            } else if (t2 > -90.0 && t1 < -90.0) {
                t1 = -90.0;
            }
            break;
        case 8:
            if (fabs(p1) < 90.0 && fabs(p2) < 90.0) {
                if (p1 <= 0.0 && p2 > 0.0) p2 = 0.0;
                if (p2 <= 0.0 && p1 > 0.0) p1 = 0.0;
            } else if (fabs(p1) > 90.0 && fabs(p2) > 90.0) {
                if (p1 < 0.0) {
                    if (fabs(p1) > 180.0) p1 = -180.0;
                    if (fabs(p2) > 180.0) p2 = -180.0;
                } else {
                    if (fabs(p1) > 180.0) p1 = 179.9999;
                    if (fabs(p2) > 180.0) p2 = 179.9999;
                }
            }
            break;
        default:
            break;
    }
    pt2xy(p1, t1, type, &r[0]);
    pt2xy(p1, t2, type, &r[1]);
    pt2xy(p2, t2, type, &r[2]);
    pt2xy(p2, t1, type, &r[3]);
}

void ptunit4xy(double p1, double p2, double t1, double t2, int type,
               int *proj_x, int *proj_y)
{
    int n;
    Point r[4];
    
    pt4xy(p1, p2, t1, t2, type, r);
    
    for (n=0; n<4; n++) {
        proj_x[n] = view.min_x + NINT((r[n].x - view.xleft)*view.scale_x);
        proj_y[n] = view.min_y + NINT((r[n].y - view.ylower)*view.scale_y);
    }
}

double xunit2dx(double x)
{
    return (double)view.min_x + (x - view.xleft)*view.scale_x;
}

double yunit2dy(double y)
{
    return (double)view.min_y + (y - view.ylower)*view.scale_y;
}

int xunit2chan(double x)
{
    return NINT(SpecUnitConv(UNIT_CHA, view.xunit, x));
}

double chan2xunit(int c)
{
    return SpecUnitConv(view.xunit, UNIT_CHA, (double)c);
}

int chan2x(int c)
{
    int    xunit2x();

    return xunit2x(SpecUnitConv(view.xunit, UNIT_CHA, (double)c));
}

int dchan2x(double c)
{
    int    xunit2x();

    return xunit2x(SpecUnitConv(view.xunit, UNIT_CHA, c));
}

int x2chan(int x)
{
    double x2xunit();

    return NINT(SpecUnitConv(UNIT_CHA, view.xunit, x2xunit(x)));
}

double x2dchan(int x)
{
    double x2xunit();

    return SpecUnitConv(UNIT_CHA, view.xunit, x2xunit(x));
}

double *chan2s(int c)
{
    if (view.s && c >= 0 && c < view.s->nChan)
        return &(view.s->d[c]);
    else
        return NULL;
}

double *chan2r(int c)
{
    if (view.s && c >= 0 && c < view.s->nChan)
        return &(view.s->e[c]);
    else
        return NULL;
}

static char *GetFloatFormat(double range, double sum, char *unit)
{
    static string format;
    double lrange=0.0, lsum=0.0;
    
    if (range != 0.0) lrange = log10(fabs(range));
    if (sum != 0.0) lsum = log10(fabs(sum));
    
    if (lrange > lsum) {
        if (lrange < -3.3 || lrange > 4.0) {
            sprintf(format, "%s %s", unit, "%11.4e");
        } else if (lrange < 1.0) {
            sprintf(format, "%s %s", unit, "%8.3f");
        } else {
            sprintf(format, "%s %s", unit, "%8.1f");
        }
    } else {
        if (lrange < -3.3 || lrange > 4.0) {
            sprintf(format, "%s %s", unit, "%12.5e");
        } else if (lrange < 1.0) {
            sprintf(format, "%s %s", unit, "%8.4f");
        } else {
            sprintf(format, "%s %s", unit, "%8.3f");
        }
    }
    
    return format;
}

static char *GetDualFloatFormat(double range, double sum, char *unit)
{
    static string format;
    double lrange=0.0, lsum=0.0;
    
    if (range != 0.0) lrange = log10(fabs(range));
    if (sum != 0.0) lsum = log10(fabs(sum));
    
    if (lrange > lsum) {
        if (lrange < -3.3 || lrange > 4.0) {
            sprintf(format, "%s %s", unit, "%11.4e(%11.4e)");
        } else if (lrange < 1.0) {
            sprintf(format, "%s %s", unit, "%8.3f(%8.3f)");
        } else {
            sprintf(format, "%s %s", unit, "%8.1f(%8.1f)");
        }
    } else {
        if (lrange < -3.3 || lrange > 4.0) {
            sprintf(format, "%s %s", unit, "%12.5e(%12.5e)");
        } else if (lrange < 1.0) {
            sprintf(format, "%s %s", unit, "%8.4f(%8.4f)");
        } else {
            sprintf(format, "%s %s", unit, "%8.3f(%8.3f)");
        }
    }
    
    return format;
}

char *GetXTrackerFormat(double x, char *unit)
{
    static string form;
    double range = fabs(view.xright - view.xleft);
    double sum   = fabs(view.xright + view.xleft);
    
    strcpy(form, GetFloatFormat(range, sum, unit));
    
    return form;
}

char *GetDualXTrackerFormat(double x1, double x2, char *unit)
{
    static string form;
    double range = fabs(view.xright - view.xleft);
    double sum   = fabs(view.xright + view.xleft);
    
    strcpy(form, GetDualFloatFormat(range, sum, unit));
    
    return form;
}

char *GetYTrackerFormat(double y, char *unit)
{
    static string form;
    double range = fabs(view.yupper - view.ylower);
    double sum   = fabs(view.yupper + view.ylower);
    
    strcpy(form, GetFloatFormat(range, sum, unit));
    
    return form;
}

char *GetZTrackerFormat(double z, char *unit)
{
    static string form;
    double range = fabs(z);
    
    strcpy(form, GetFloatFormat(range, 0.0, unit));
    
    return form;
}
