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
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Frame.h>
#include <Xm/Separator.h>

#include "defines.h"
#include "global_structs.h"
#include "menus.h"

#ifdef HAVE_LIBPGPLOT
#include "cpgplot.h"
#endif

/*** External variables ***/
extern int     pgplot, mark_sel;
extern VIEW   *vP;
extern PSDATA  ps;
extern GLOBAL *gp;

void   PostErrorDialog(Widget, char *);
void   PostWarningDialog(Widget, char *);
void   PostMessageDialog(Widget, char *);
void   ManageDialogCenteredOnPointer(Widget);
Widget CreateOptionMenu(Widget, MenuBarItem *);
void   SetDefaultOptionMenuItem(Widget, int);
void   SetPGStyle(PSSTY *);
void   SetAnyToggle(char *, int);

/*** Local Variables ***/
static void MarkerTypeCallback(Widget, char *, XmAnyCallbackStruct *);
MenuItem MarkerTypeData[] = {
  {"Arrow", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerTypeCallback, "0", NULL},
  {"Line", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerTypeCallback, "1", NULL},
  {"Square", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerTypeCallback, "2", NULL},
  {"Circle", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerTypeCallback, "3", NULL},
  {"None", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerTypeCallback, "4", NULL},
  {"PGPLOT", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerTypeCallback, "5", NULL},
  {"Abs. line", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerTypeCallback, "6", NULL},
EOI};
MenuBarItem MarkerTypeMenu = {
   "Type of marker", ' ', True, MarkerTypeData
};

static void MarkerDirCallback(Widget, char *, XmAnyCallbackStruct *);
MenuItem MarkerDirData[] = {
  {"Down", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerDirCallback, "0", NULL},
  {"Up", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerDirCallback, "1", NULL},
  {"Left", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerDirCallback, "2", NULL},
  {"Right", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerDirCallback, "3", NULL},
EOI};
MenuBarItem MarkerDirMenu = {
   "Marker direction", ' ', True, MarkerDirData
};

static void MarkerTaggedCallback(Widget, char *, XmAnyCallbackStruct *);
MenuItem MarkerTaggedData[] = {
  {"No", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerTaggedCallback, "0", NULL},
  {"Yes", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MarkerTaggedCallback, "1", NULL},
EOI};
MenuBarItem MarkerTaggedMenu = {
   "Tag marker?", ' ', True, MarkerTaggedData
};

static char *mark_labels0[] = {
   "Frequency:",
   "Temperature:",
};
static char *mark_labels1[] = {
   "Velocity:",
   "Temperature:",
};
static char *mark_labels2[] = {
   "Freq. (MHz):",
   "Temperature:",
};
static char *mark_labelsM[] = {
   "RA offset:",
   "Dec. offset:",
};

static char *mark_std_labels[] = {
   "Length in X-dir:",
   "Length in Y-dir:",
   "Label alignment:",
   "Label angle:",
   "Label:"
};

typedef struct {
  int      n;
  Widget  *e;
  Widget   w;
} WMDATA;

int  nmark, mark_xunit;
MARK marks[MAXMARK], mark;

static int emark;

void init_mark_parameters()
{
    mark_xunit = UNIT_FRE;
    emark = -1;
    nmark = 0;
}

void change_unit_in_marks(int new_xunit)
{
    int n;
    
    double SpecUnitConv();

    mark_xunit = new_xunit;
    
    if (nmark <= 0) return;

    for (n=0; n<nmark; n++)
        marks[n].x = SpecUnitConv(new_xunit, mark_xunit, marks[n].x);

}

static void draw_mark_label(GC l_gc, int x1, int y1, int x2, int y2, MARK *m)
{
    int         x=0, y=0;
    int         dir, ascent, descent;
    XCharStruct s_info;
    Display    *dpy = XtDisplay(gp->graph);

    int point_is_inside();
    void draw_string();
    void draw_xbox();
    XID GetFIDFromGC();
    
    if (!point_is_inside(x1, y1)) return;

    XQueryTextExtents(dpy, GetFIDFromGC(l_gc), m->label, strlen(m->label),
                      &dir, &ascent, &descent, &s_info);

    if (m->dir == MARK_DIR_UP) {
        x = x1 - NINT(m->align*(double)s_info.width);
        y = y2 + descent + ascent;
    } else if (m->dir == MARK_DIR_DOWN) {
        x = x1 - NINT(m->align*(double)s_info.width);
        y = y2 - descent;
    } else if (m->dir == MARK_DIR_LEFT) {
        x = x2 + NINT(m->align*(double)s_info.width);
        y = y2 + descent;
    } else if (m->dir == MARK_DIR_RIGHT) {
        x = x2 - NINT(m->align*(double)s_info.width);
        y = y2 + descent;
    }
    draw_string(l_gc, x, y, m->label);
}

#define REL_LENGTH (0.05/20.0)
#define MIN_ARROW  (0.007)
#define MAX_ARROW  (0.020)

static void GetMarkEnd(MARK *m, double *x, double *y, double *rx, double *ry)
{
    double dx, dy;
    
    if (!m) return;
    
    dx = REL_LENGTH * vP->xrange * m->xlength;
    dy = REL_LENGTH * vP->yrange * m->ylength;
    if (m->type == MARK_TYPE_SQUARE || m->type == MARK_TYPE_CIRCLE) {
        dx /= 2.0;
        dy /= 2.0;
    } else if (m->type == MARK_TYPE_PGPLOT) {
        dx /= 4.0;
        dy /= 4.0;
    } else if (m->type == MARK_TYPE_NONE) {
        dx = 0.0;
        dy = 0.0;
    } else if (m->type == MARK_TYPE_ALINE) {
        dx = m->xlength;
        dy = m->ylength;
    }
    switch (m->dir) {
        case MARK_DIR_LEFT:
            if (x) *x = m->x + dx;
            if (y) *y = m->y;
            break;
        case MARK_DIR_RIGHT:
            if (x) *x = m->x - dx;
            if (y) *y = m->y;
            break;
        case MARK_DIR_UP:
            if (x) *x = m->x;
            if (y) *y = m->y - dy;
            break;
        case MARK_DIR_DOWN:
            if (x) *x = m->x;
            if (y) *y = m->y + dy;
            break;
        default:
            if (x) *x = m->x;
            if (y) *y = m->y;
            break;
    }
    if (rx) *rx = dx;
    if (ry) *ry = dy;
}

static void GetArrowPairs(MARK *m, double *x1, double *y1,
                                   double *x2, double *y2)
{
    if (!m || !x1 || !y1 || !x2 || !y2) return;
    
    switch (m->dir) {
        case MARK_DIR_LEFT:
            *x1 = m->x + MAX_ARROW * vP->xrange;
            *x2 = *x1;
            *y1 = m->y + MIN_ARROW * vP->yrange;
            *y2 = m->y - MIN_ARROW * vP->yrange;
            break;
        case MARK_DIR_RIGHT:
            *x1 = m->x - MAX_ARROW * vP->xrange;
            *x2 = *x1;
            *y1 = m->y + MIN_ARROW * vP->yrange;
            *y2 = m->y - MIN_ARROW * vP->yrange;
            break;
        case MARK_DIR_UP:
            *x1 = m->x - MIN_ARROW * vP->xrange;
            *x2 = m->x + MIN_ARROW * vP->xrange;
            *y1 = m->y - MAX_ARROW * vP->yrange;
            *y2 = *y1;
            break;
        case MARK_DIR_DOWN:
        default:
            *x1 = m->x - MIN_ARROW * vP->xrange;
            *x2 = m->x + MIN_ARROW * vP->xrange;
            *y1 = m->y + MAX_ARROW * vP->yrange;
            *y2 = *y1;
            break;
    }
}

#ifdef HAVE_LIBPGPLOT
static void GetLabelPos(MARK *m, PLFLT x0, PLFLT y0, PLFLT *x, PLFLT *y)
{
    int n, in_x = 0, in_y = 0;
    PLFLT xmin, xmax, ymin, ymax, dx, dy, fx[4], fy[4];
    PLFLT rdx, rdy;
    
    if (!m || !x || !y) return;
    
    /* Inquire the bounding box of the string */
    cpgqtxt(x0, y0, (PLFLT)m->angle, (PLFLT)m->align, m->label, fx, fy);
    
    ymin = ymax = fy[0];
    xmin = xmax = fx[0];
    
    for (n=1; n<4; n++) {
        if (fx[n] < xmin) xmin = fx[n];
        if (fx[n] > xmax) xmax = fx[n];
        if (fy[n] < ymin) ymin = fy[n];
        if (fy[n] > ymax) ymax = fy[n];
    }
    dx = 0.1*(xmax - xmin);
    dy = 0.1*(ymax - ymin);
    
    rdx = 0.05*(PLFLT)vP->xrange;
    if (rdx < 0.0) rdx *= -1.0;
    rdy = 0.05*(PLFLT)vP->yrange;
    if (rdy < 0.0) rdy *= -1.0;
    
    if (dx > rdx) dx = rdx;
    if (dy > rdy) dy = rdy;
    
    if (x0 >= xmin && x0 <= xmax) in_x = 1;
    if (vP->xrange < 0.0) in_x *= -1;
    
    if (y0 >= ymin && y0 <= ymax) in_y = 1;
    if (vP->yrange < 0.0) in_y *= -1;
    
    switch (m->dir) {
        case MARK_DIR_LEFT:
            *x = x0;
            *y = y0;
            if (in_x == 1)
                *x += x0 - xmin;
            else if (in_x == -1)
                *x += x0 - xmax;
            *y += y0 - (ymax + ymin)/2.0;
            break;
        case MARK_DIR_RIGHT:
            *x = x0;
            *y = y0;
            if (in_x == 1)
                *x += x0 - xmax;
            else if (in_x == -1)
                *x += x0 - xmin;
            *y += y0 - (ymax + ymin)/2.0;
            break;
        case MARK_DIR_UP:
            *x = x0;
            *y = y0;
            if (in_y == 1)
                *y += y0 - ymax - dy;
            else if (in_y == -1)
                *y += y0 - ymin + dy;
            break;
        case MARK_DIR_DOWN:
            *x = x0;
            *y = y0;
            if (in_y == 1)
                *y += y0 - ymin + dy;
            else if (in_y == -1)
                *y += y0 - ymax - dy;
            break;
        default:
           *x = x0;
           *y = y0;
           break;
    }
}
#endif

static void DrawMarker(GC gc, MARK *m)
{
    int x1, y1, x2, y2;
    int dx, dy;
    double xe, ye, xe2, ye2, xa[2], ya[2];
#ifdef HAVE_LIBPGPLOT
    PLFLT x1fl, x2fl, y1fl, y2fl, xrange, yrange, ch, current_ch;
    PLFLT fdx, fdy;
    PLFLT fx[5], fy[5];
    Beam b;
    int symbol[1];
#endif
    MARK *m2=NULL;
    
    int xunit2x(), yunit2y();
    void draw_line(), draw_xbox(), draw_xcircle(), draw_xellipse();
#ifdef HAVE_LIBPGPLOT
    int point_is_inside();
    void draw_beam();
#endif
    
    xe2 = ye2 = xa[0] = xa[1] = ya[0] = ya[1] = 0.0;
    if (!m) return;
    
    if (vP->mode != SHOW_SUBSPE || m->mode != SHOW_SPE) {
        if (m->mode != vP->mode) return;
    }
    
    x1 = xunit2x(m->x);
    y1 = yunit2y(m->y);
    
    GetMarkEnd(m, &xe, &ye, NULL, NULL);
    x2 = xunit2x(xe);
    y2 = yunit2y(ye);

    if (m->type == MARK_TYPE_LINE || m->type == MARK_TYPE_ALINE) {
        draw_line(gc, x1, y1, x2, y2);
    } else if (m->type == MARK_TYPE_ARROW) {
        draw_line(gc, x1, y1, x2, y2);
        GetArrowPairs(m, &xa[0], &ya[0], &xa[1], &ya[1]);
        draw_line(gc, x1, y1, xunit2x(xa[0]), yunit2y(ya[0]));
        draw_line(gc, x1, y1, xunit2x(xa[1]), yunit2y(ya[1]));
    } else if (m->type == MARK_TYPE_SQUARE || m->type == MARK_TYPE_PGPLOT) {
        dx = (int)(m->xlength+0.5);
        dy = (int)(m->ylength+0.5);
        if (m->mode == SHOW_POSPOS) {
            dx = abs(xunit2x(m->x + m->xlength/2.0) -
                     xunit2x(m->x - m->xlength/2.0));
            dy = abs(yunit2y(m->y + m->ylength/2.0) -
                     yunit2y(m->y - m->ylength/2.0));
        }
        draw_xbox(gc, x1-dx, y1-dy, x1+dx, y1+dy);
    } else if (m->type == MARK_TYPE_CIRCLE) {
        dx = (int)(m->xlength+0.5);
        dy = (int)(m->ylength+0.5);
        if (m->mode == SHOW_POSPOS) {
            dx = abs(xunit2x(m->x + m->xlength/2.0) -
                     xunit2x(m->x - m->xlength/2.0));
            dy = abs(yunit2y(m->y + m->ylength/2.0) -
                     yunit2y(m->y - m->ylength/2.0));
        }
        if (dx == dy) {
            draw_xcircle(gc, x1, y1, dx);
        } else {
            draw_xellipse(gc, x1, y1, dx, dy);
        }
    } else {
        y2 = y1;
        x2 = x1;
    }
    if ((m2 = m->mark)) {
        GetMarkEnd(m2, &xe2, &ye2, NULL, NULL);
        draw_line(gc, x2, y2, xunit2x(xe2), yunit2y(ye2));
    }
    
    draw_mark_label(gc, x1, y1, x2, y2, m);

#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.marker);
        xrange = (PLFLT)fabs(vP->xrange);
        yrange = (PLFLT)fabs(vP->yrange);
        x1fl = (PLFLT)m->x;
        y1fl = (PLFLT)m->y;
        x2fl = (PLFLT)xe;
        y2fl = (PLFLT)ye;
        if (m->type == MARK_TYPE_LINE || m->type == MARK_TYPE_ALINE) {
            fx[0] = x1fl; fy[0] = y1fl;
            fx[1] = x2fl; fy[1] = y2fl;
            cpgline(2, fx, fy);
        } else if (m->type == MARK_TYPE_ARROW) {
            fx[0] = x1fl; fy[0] = y1fl;
            fx[1] = x2fl; fy[1] = y2fl;
            cpgline(2, fx, fy);
            fx[1] = (PLFLT)xa[0]; fy[1] = (PLFLT)ya[0];
            cpgline(2, fx, fy);
            fx[1] = (PLFLT)xa[1]; fy[1] = (PLFLT)ya[1];
            cpgline(2, fx, fy);
        } else if (m->type == MARK_TYPE_SQUARE) {
            fdx = REL_LENGTH*xrange*(PLFLT)(m->xlength);
            fdy = REL_LENGTH*yrange*(PLFLT)(m->ylength);
            if (m->mode == SHOW_POSPOS) {
                fdx = (PLFLT)(m->xlength/2.0);
                fdy = (PLFLT)(m->ylength/2.0);
            }            
            fx[0] = x1fl-fdx; fy[0] = y1fl-fdy;
            fx[1] = x1fl+fdx; fy[1] = y1fl+fdy;
            cpgrect(fx[0], fx[1], fy[0], fy[1]);
        } else if (m->type == MARK_TYPE_CIRCLE) {
            fdx = REL_LENGTH*xrange*(PLFLT)(m->xlength);
            fdy = REL_LENGTH*yrange*(PLFLT)(m->ylength);
            if (m->mode == SHOW_POSPOS) {
                fdx = (PLFLT)(m->xlength);
                fdy = (PLFLT)(m->ylength);
            }
            if (fdx == fdy) {
                cpgcirc(x1fl, y1fl, fdx/2.0);
            } else {
                b.maj = (double)fdy; b.min = (double)fdx;
                b.PA = 0.0;
                draw_beam(gc, m->x, m->y, &b);
            }
        } else if (m->type == MARK_TYPE_PGPLOT) {
            symbol[0] = m->pgtype;
            fx[0] = x1fl; fy[0] = y1fl;
            cpgqch(&current_ch);
            ch = (PLFLT)(m->xlength)/20.0 * current_ch;
            cpgsch(ch);
            cpgpnts(1, fx, fy, symbol, 1);
            cpgsch(current_ch);
        }
        if (m2) {
            fx[0] = x2fl; fy[0] = y2fl;
            fx[1] = (PLFLT)xe2; fy[1] = (PLFLT)ye2;
            cpgline(2, fx, fy);
        }
        
        /* Inquire the placement of the string */
        if (point_is_inside(x1, y1)) {
            GetLabelPos(m, x2fl, y2fl, &fx[0], &fy[0]);
            cpgptxt(fx[0], fy[0], (PLFLT)m->angle, (PLFLT)m->align, m->label);
        }
    }
#endif
}

void draw_markers(GC gc, GC alt_gc, GC tagged_gc)
{
    int n;
    
    if (nmark <= 0) return;

    for (n=0; n<nmark; n++)
        if (n != emark)
            DrawMarker(marks[n].tagged ? tagged_gc : gc, &marks[n]);

    if (emark >= 0)
        DrawMarker(alt_gc, &marks[emark]);
}

static void update_marks(Widget w, WMDATA *d, XmAnyCallbackStruct *cb)
{
    void draw_main(), wdscanf(), wsscanf(), wiscanf();

    wdscanf((Widget)d->e[0],  &marks[d->n].x);
    wdscanf((Widget)d->e[1],  &marks[d->n].y);
    wiscanf((Widget)d->e[3],  &marks[d->n].pgtype);
    wdscanf((Widget)d->e[6],  &marks[d->n].xlength);
    wdscanf((Widget)d->e[7],  &marks[d->n].ylength);
    wdscanf((Widget)d->e[8],  &marks[d->n].align);
    wdscanf((Widget)d->e[9],  &marks[d->n].angle);
    wsscanf((Widget)d->e[10],  marks[d->n].label);

    if (d->n == nmark) {
        marks[nmark].mode = vP->mode;
        nmark++;
    }

    SetAnyToggle("markers", 0);

    draw_main();
}

static void MarkEditPrint(Widget *e, MARK *m)
{
    void wprintf();

    wprintf(e[0], "%f", m->x);
    wprintf(e[1], "%f", m->y);
    SetDefaultOptionMenuItem(e[2], m->type);
    wprintf(e[3], "%d", m->pgtype);
    SetDefaultOptionMenuItem(e[4], m->dir);
    SetDefaultOptionMenuItem(e[5], m->tagged);
    wprintf(e[6], "%f", m->xlength);
    wprintf(e[7], "%f", m->ylength);
    wprintf(e[8], "%f", m->align);
    wprintf(e[9], "%f", m->angle);
    wprintf(e[10], "%s", m->label);
}

static void edit_next_mark(Widget w, WMDATA *d, XmAnyCallbackStruct *cb)
{
    int next = d->n + 1;

    void draw_main();

    if (next >= nmark) next = 0;

    d->n = next;
    emark = next;

    MarkEditPrint(d->e, &marks[next]);

    draw_main();
}

static void edit_prev_mark(Widget w, WMDATA *d, XmAnyCallbackStruct *cb)
{
    int prev = d->n - 1;

    void draw_main();

    if (prev < 0) prev = nmark- 1;

    d->n = prev;
    emark = prev;

    MarkEditPrint(d->e, &marks[prev]);

    draw_main();
}

static void cancel_edit_mark(Widget w, WMDATA *d, XmAnyCallbackStruct *cb)
{
    void draw_main();

    XtDestroyWidget(d->w);
    emark = -1;

    draw_main();
}

static void MarkerTypeCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str);

    void draw_main();
    
    if (emark < 0 || emark >= nmark) return;
    
    if (n != marks[emark].type) {
        marks[emark].type = n;
        draw_main();
    }
}

static void MarkerDirCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str);

    void draw_main();
    
    if (emark < 0 || emark >= nmark) return;
    
    if (n != marks[emark].dir) {
        marks[emark].dir = n;
        draw_main();
    }
}

static void MarkerTaggedCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str);

    void draw_main();
    
    if (emark < 0 || emark >= nmark) return;
    
    if (n != marks[emark].tagged) {
        marks[emark].tagged = n;
        draw_main();
    }
}

static void edit_mark(Widget w, int m_no, MARK *m)
{
    Widget form, rc, rc_u, rc_m, rc_b, fr_u, fr_m, fr_b, sep;
    Widget cancel, apply, next, prev;
    Widget label;
    static Widget entries[11];
    static WMDATA data;
    int i, n;
    string str;
    Arg wargs[10];

    void draw_main();

    sprintf(str, "Mark no. %d ", m_no+1);
    n = 0;
    XtSetArg(wargs[n], XmNautoUnmanage,  False); n++;
    XtSetArg(wargs[n], XmNtitle,  str); n++;
    form = XmCreateFormDialog(w, "marker", wargs, n);

    emark = m_no;
    
    draw_main();
    
    n = 0;
    XtSetArg(wargs[n], XmNorientation,       XmVERTICAL); n++;
    XtSetArg(wargs[n], XmNnumColumns,        1); n++;
    rc    = XtCreateManagedWidget("rowcol", xmRowColumnWidgetClass,
                                  form, wargs, n);

    fr_u = XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
				                   rc, XmNshadowType, XmSHADOW_OUT, NULL);
    n = 0;
    XtSetArg(wargs[n], XmNorientation,       XmHORIZONTAL); n++;
    XtSetArg(wargs[n], XmNnumColumns,        2); n++;
    XtSetArg(wargs[n], XmNadjustLast,        FALSE); n++;
    XtSetArg(wargs[n], XmNpacking,           XmPACK_COLUMN); n++;
    rc_u  = XtCreateManagedWidget("rowcol", xmRowColumnWidgetClass,
                                  fr_u, wargs, n);
    for (i=0; i<2; i++) {
        if (m->mode == SHOW_SPE) {
            if (mark_xunit == UNIT_FRE)
                label  = XtCreateManagedWidget(mark_labels0[i],
                                               xmLabelWidgetClass,
                                               rc_u, NULL, 0);
            else if (mark_xunit == UNIT_FOFF)
                label  = XtCreateManagedWidget(mark_labels2[i],
                                               xmLabelWidgetClass,
                                               rc_u, NULL, 0);
            else if (mark_xunit == UNIT_FMHZ)
                label  = XtCreateManagedWidget(mark_labels2[i],
                                               xmLabelWidgetClass,
                                               rc_u, NULL, 0);
            else
                label  = XtCreateManagedWidget(mark_labels1[i],
                                               xmLabelWidgetClass,
                                               rc_u, NULL, 0);
        } else
            label  = XtCreateManagedWidget(mark_labelsM[i],
                                               xmLabelWidgetClass,
                                               rc_u, NULL, 0);
        entries[i] = XtCreateManagedWidget("entry", xmTextWidgetClass,
                                           rc_u, NULL, 0);
    }
    
    entries[2] = CreateOptionMenu(rc, &MarkerTypeMenu);
    
    fr_m = XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
				                   rc, XmNshadowType, XmSHADOW_OUT, NULL);
    n = 0;
    XtSetArg(wargs[n], XmNorientation,       XmHORIZONTAL); n++;
    XtSetArg(wargs[n], XmNnumColumns,        1); n++;
    XtSetArg(wargs[n], XmNadjustLast,        FALSE); n++;
    XtSetArg(wargs[n], XmNpacking,           XmPACK_COLUMN); n++;
    rc_m  = XtCreateManagedWidget("rowcol", xmRowColumnWidgetClass,
                                  fr_m, wargs, n);
    label = XtCreateManagedWidget("PGPLOT marker (1-31)", xmLabelWidgetClass,
                                  rc_m, NULL, 0);
    entries[3] = XtCreateManagedWidget("entry", xmTextWidgetClass,
                                       rc_m, NULL, 0);
    
    entries[4] = CreateOptionMenu(rc, &MarkerDirMenu);
    
    entries[5] = CreateOptionMenu(rc, &MarkerTaggedMenu);
    
    fr_b = XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
				                   rc, XmNshadowType, XmSHADOW_OUT, NULL);
    n = 0;
    XtSetArg(wargs[n], XmNorientation,       XmHORIZONTAL); n++;
    XtSetArg(wargs[n], XmNnumColumns,        5); n++;
    XtSetArg(wargs[n], XmNadjustLast,        FALSE); n++;
    XtSetArg(wargs[n], XmNpacking,           XmPACK_COLUMN); n++;
    rc_b  = XtCreateManagedWidget("rowcol", xmRowColumnWidgetClass,
                                  fr_b, wargs, n);
    for (i=0; i<5; i++) {
        label = XtCreateManagedWidget(mark_std_labels[i],
                                      xmLabelWidgetClass,
                                      rc_b, NULL, 0);
        entries[i+6] = XtCreateManagedWidget("entry", xmTextWidgetClass,
                                             rc_b, NULL, 0);
    }
    
    sep = XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
				                 form, XmNseparatorType, XmSHADOW_ETCHED_IN,
				                 NULL);
    
    data.n = m_no;
    data.e = entries;
    data.w = form;
    apply  = XtCreateManagedWidget(BUTT_APPLY, xmPushButtonWidgetClass,
                                   form, NULL, 0);
    cancel = XtCreateManagedWidget(BUTT_CANCEL, xmPushButtonWidgetClass,
                                   form, NULL, 0);
    next   = XtCreateManagedWidget("next", xmPushButtonWidgetClass,
                                   form, NULL, 0);
    prev   = XtCreateManagedWidget("previous", xmPushButtonWidgetClass,
                                   form, NULL, 0);
    XtAddCallback(apply, XmNactivateCallback,
                  (XtCallbackProc)update_marks, &data);                        
    XtAddCallback(cancel, XmNactivateCallback,
                  (XtCallbackProc)cancel_edit_mark, &data);                        
    XtAddCallback(next, XmNactivateCallback,
                  (XtCallbackProc)edit_next_mark, &data);                        
    XtAddCallback(prev, XmNactivateCallback,
                  (XtCallbackProc)edit_prev_mark, &data);                        

    XtVaSetValues(form, XmNdefaultButton, apply, NULL);
    
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNtopOffset,           10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,          10); n++;
    XtSetArg(wargs[n], XmNrightAttachment,     XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,         10); n++;
    XtSetValues(rc, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,           rc); n++;
    XtSetArg(wargs[n], XmNtopOffset,           10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,          1); n++;
    XtSetArg(wargs[n], XmNrightAttachment,     XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,         1); n++;
    XtSetValues(sep, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,           sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,           10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,          30); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,        10); n++;
    XtSetValues(apply, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,           sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,           10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,          apply); n++;
    XtSetArg(wargs[n], XmNleftOffset,          10); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,        10); n++;
    XtSetValues(cancel, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,           sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,           10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,          cancel); n++;
    XtSetArg(wargs[n], XmNleftOffset,          20); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,        10); n++;
    XtSetValues(prev, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,           sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,           10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,          prev); n++;
    XtSetArg(wargs[n], XmNleftOffset,          10); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,        10); n++;
    XtSetValues(next, wargs, n);

    XtManageChild(entries[2]);
    XtManageChild(entries[4]);
    XtManageChild(entries[5]);
    
    MarkEditPrint(entries, m);
    
    ManageDialogCenteredOnPointer(form);
}

void mark_handling(Widget w, int ms, double freq, double temp,
                   double freq2, double temp2)
{
    int n, nfound=-1;
    double frange, trange, tmp;
    double fdist, tdist, dist, mindist=1.e10;
    string buf;
    MARK mark;
    
    double chan_to_xunit();
    void draw_main();

    if (ms == 1 || ms == 3) {
        frange = fabs(vP->xrange);
        trange = fabs(vP->yrange);
        for (n=0; n<nmark; n++) {
            if (marks[n].mode != vP->mode) continue;
            fdist = fabs(freq - marks[n].x)/frange;
            tdist = fabs(temp - marks[n].y)/trange;
            dist = fdist + tdist;
            if (n == 0) {
                mindist = dist;
                nfound = 0;
            } else if (dist < mindist) {
                mindist = dist;
                nfound = n;
            }
        }
        if (nfound < 0 || mindist >= 0.1) return;
    }

    if (ms == 1) {                /* Remove a mark near freq, temp */
        for (n=nfound+1; n<nmark; n++) marks[n-1] = marks[n];
        nmark--;
        draw_main();
    } else if (ms == 2) {                /* Add a mark at freq, temp      */
        if (nmark >= MAXMARK) {
            sprintf(buf, "Too many markers %d > %d.", nmark+1, MAXMARK);
            PostErrorDialog(w, buf);
            return;
        }
        mark.mode = vP->mode;
        mark.x = freq;
        mark.y = temp;
        mark.type = MARK_TYPE_ARROW;
        mark.pgtype = 1;
        mark.dir  = MARK_DIR_DOWN;
        mark.align = 0.5;
        mark.angle = 0.0;
        mark.xlength = 20.0;
        mark.ylength = 20.0;
        strcpy(mark.label, "<empty>");
        mark.tagged = 0;
        mark.mark = NULL;
        marks[nmark] = mark;
        edit_mark(w, nmark, &mark);
    } else if (ms == 3) {                /* Edit a mark near freq, temp   */
        mark = marks[nfound];
        edit_mark(w, nfound, &mark);
    } else if (ms == 4) {
        if (freq2 < freq) { tmp = freq; freq = freq2; freq2 = tmp;}
        if (temp2 > temp) { tmp = temp; temp = temp2; temp2 = tmp;}
        for (n=0; n<nmark; n++) {
            if (marks[n].mode != vP->mode) continue;
            if (marks[n].x >= freq  && marks[n].x <= freq2 &&
                marks[n].y >= temp2 && marks[n].y <= temp) {
                if (marks[n].tagged == 0) {
                    marks[n].tagged = 1;
                } else {
                    marks[n].tagged = 0;
                }
            }
        }
        draw_main();
    }
}

void mark_reset(Widget w, char *cmd, XtPointer cd)
{
   int n, m, any_tagged=0;
   double tag_max=0.0, tag_min=0.0;
   
   void draw_main();
   
   for (n=0; n<nmark; n++) {
       if (marks[n].tagged) {
           if (!any_tagged) {
               tag_min = tag_max = marks[n].y;
           } else {
               if (marks[n].y < tag_min) tag_min = marks[n].y;
               if (marks[n].y > tag_max) tag_max = marks[n].y;
           }
           any_tagged = 1;
       }
   }
   
   if (strncmp(cmd, "all", 3) == 0) {
       for (n=0; n<nmark; n++) {
           marks[n].tagged = 0;
           marks[n].mark = NULL;
       }
       nmark = 0;
   } else if (strncmp(cmd, "rcursor", 7) == 0) {
       if (mark_sel != 1) {
           mark_sel = 1;
       }
      return;
   } else if (strncmp(cmd, "acursor", 7) == 0) {
       if (mark_sel != 2) {
           mark_sel = 2;
       }
       return;
   } else if (strncmp(cmd, "ecursor", 7) == 0) {
       if (mark_sel != 3) {
           mark_sel = 3;
       }
       return;
   } else if (strncmp(cmd, "tcursor", 7) == 0) {
       if (mark_sel == 4) {
           mark_sel = 0;
       } else {
           mark_sel = 4;
       }
       return;
   } else if (strncmp(cmd, "untag", 5) == 0) {
       for (n=0; n<nmark; n++) marks[n].tagged = 0;
   } else if (strncmp(cmd, "up", 2) == 0) {
       for (n=0; n<nmark; n++)
           if (marks[n].tagged) marks[n].y = tag_max;
   } else if (strncmp(cmd, "center", 6) == 0) {
       for (n=0; n<nmark; n++)
           if (marks[n].tagged) marks[n].y = (tag_max+tag_min)/2.0;
   } else if (strncmp(cmd, "down", 4) == 0) {
       for (n=0; n<nmark; n++)
           if (marks[n].tagged) marks[n].y = tag_min;
   } else if (strncmp(cmd, "rtagged", 7) == 0) {
       m = 0;
       for (n=0; n<nmark; n++) {
           if (marks[n].tagged == 0) {
               marks[m] = marks[n];
               if (marks[m].mark) {
                   if (marks[m].mark->tagged) marks[m].mark = NULL;
               }
               m++;
           }
       }
       nmark = m;
   } else if (strncmp(cmd, "latest", 6) == 0) {
       nmark--;
       if (nmark < 0) nmark = 0;
   }
   draw_main();
}

MARK *GetMarker(double x, double y)
{
    int n, nfound = -1;
    double xrange, yrange, mindist=0.0, d;
    
    xrange = fabs(vP->xrange);
    yrange = fabs(vP->yrange);
    for (n=0; n<nmark; n++) {
        if (marks[n].mode != vP->mode) continue;
        d = fabs(x - marks[n].x)/xrange + fabs(y - marks[n].y)/yrange;
        if (n == 0) {
            mindist = d;
            nfound = 0;
        } else if (d < mindist) {
            mindist = d;
            nfound = n;
        }
    }
    if (nfound < 0) return NULL;
    
    return &marks[nfound];
}

void JoinMarkers(MARK *m1, MARK *m2)
{
    if (!m1 || !m2) return;
    
    m1->mark = m2;
}

int SaveMarkerFile(char *file)
{
    int n, i, joined;
    FILE *fp;
    MARK *m;
    
    if (!file) return 1;
    
    fp = fopen(file, "w");
    if (!fp) return 1;
    
    fprintf(fp, "%d\n", mark_xunit);
    
    for (n=0; n<nmark; n++) {
        m = &marks[n];
        joined = -1;
        if (m->mark) {
            i = 0;
            while (i < nmark) {
                if (m->mark == &marks[i]) {
                    joined  = i;
                    break;
                }
                i++;
            }
        }
        fprintf(fp, "%d %16.9e %e %d %d %d %e %e %e %e %d %d \"%s\"\n",
                m->mode, m->x, m->y, m->type, m->pgtype, m->dir,
                m->xlength, m->ylength, m->align, m->angle,
                m->tagged, joined, m->label);
    }
    
    fclose(fp);
    
    return 0;
}

/* This routine extracts a string that is enclosed by two
   double quotes, i.e. "string"                          */
char *ExtractString(char *str)
{
    int n=0, first=0, last=0, escape = 0;
    static string r;
    char *p;
    
    strcpy(r, "<error>");
    
    if (!str) return r;
    
    p = str;
    
    while (*p != '\0' && *p != '\n') {
        if (*p == '"') {
            if (escape) {
                escape = 0;
                r[n-1] = '"';
            } else if (first) {
                r[n] = '\0';
                last = 1;
                break;
            } else {
                first = 1;
            }
        } else {
            escape = (*p == '\\') ? 1 : 0;
            r[n] = *p;
            n++;
        }
        p++;
    }
    
    if (!last) strcpy(r, "<error>");
    
    return r;
}

static int GetMarkerEntry(char *str, MARK *m, int *joined)
{
    int n=1;
    char *ptr;
    string tmp;
    
    if (!str || !m) return 1;
    
    strcpy(tmp, str);
    
    ptr = strtok(tmp, " \t");
    
    while (ptr) {
        switch (n) {
            case 1:
                m->mode = atoi(ptr);
                break;
            case 2:
                m->x = atof(ptr);
                break;
            case 3:
                m->y = atof(ptr);
                break;
            case 4:
                m->type = atoi(ptr);
                break;
            case 5:
                m->pgtype = atoi(ptr);
                break;
            case 6:
                m->dir = atoi(ptr);
                break;
            case 7:
                m->xlength = atof(ptr);
                break;
            case 8:
                m->ylength = atof(ptr);
                break;
            case 9:
                m->align = atof(ptr);
                break;
            case 10:
                m->angle = atof(ptr);
                break;
            case 11:
                m->tagged = atoi(ptr);
                break;
            case 12:
                if (joined) *joined = atoi(ptr);
                break;
            case 13:
                strcpy(m->label, ExtractString(ptr));
                break;
            default:
                return 2;
                break; 
        }
        if (n >= 12)
            ptr = strtok(NULL, "\n\0");
        else
            ptr = strtok(NULL, " \t");
        n++;
    }
    
    return 0;
}

int ReadMarkerFile(char *file, char how)
{
    int n, nread, err, joined=0;
    string tmp;
    char buf[MAXBUFSIZE];
    FILE *fp;
    MARK *m;
    
    void send_line();
    
    if (!file) return 1;
    
    fp = fopen(file, "r");
    if (!fp) return 1;
    
    while (fgets(buf, MAXBUFSIZE, fp) != NULL) {
        if (buf[0] == '#' || buf[0] == '!') continue;
        err = sscanf(buf, "%d", &mark_xunit);
        if (err != 1) {
            sprintf(tmp, "Error when reading expected marker unit %s in %s.",
                    buf, file);
            PostErrorDialog(NULL, tmp);
            fclose(fp);
            return 1;
        }
        break;
    }
    
    if (how == 'a')
        n = nmark;
    else
        n = 0;
    
    nread = 0;
    while ((fgets(buf, MAXBUFSIZE, fp)) != NULL) {
        if (buf[0] == '#' || buf[0] == '!') continue;
        if (n >= MAXMARK) break;
        m = &marks[n];
        strcpy(m->label, "");
        err = GetMarkerEntry(buf, m, &joined);
        if (err) {
            sprintf(buf, "Error (%d) when reading marker %d in %s.",
                    err, nread+1, file);
            PostErrorDialog(NULL, buf);
            break;
        }
        if (joined >= 0 && joined < MAXMARK) {
            m->mark = &marks[joined];
        } else {
            m->mark = NULL;
        }
        n++;
        nread++;
    }
    
    fclose(fp);
    
    nmark = n;
    
    if (vP->mode == SHOW_SPE && mark_xunit != vP->xunit)
        change_unit_in_marks(vP->xunit);

    if (nread) SetAnyToggle("markers", 0);
    
    sprintf(buf, "Read %d (%d) markers from '%s'.", nread, nmark, file);
    send_line(buf);
    
    return 0;
}

static void scale_markers(char c, double fact)
{
    char buf[MAXBUFSIZE];
    int n, m=0;
    
    void send_line();
    void draw_main();
    
    if (nmark <= 0) return;

    for (n=0; n<nmark; n++) {
      if ((marks[n].tagged && c == 't') || c == 'a') {
        marks[n].y *= fact;
	m++;
      }
    }
    
    if (m) draw_main();
    
    sprintf(buf, "Scaled the y-values of %d markers by %f.", m, fact);
    send_line(buf);
}

void ScaleMarkers(Widget w, char *how, XtPointer cd)
{
    if (strcmp(how, "0.5") == 0) {
      scale_markers('a', 0.5);
    } else if (strcmp(how, "0.9") == 0) {
      scale_markers('a', 0.9);
    } else if (strcmp(how, "1.1") == 0) {
      scale_markers('a', 1.0/0.9);
    } else if (strcmp(how, "2.0") == 0) {
      scale_markers('a', 2.0);
    } else if (strcmp(how, "t0.5") == 0) {
      scale_markers('t', 0.5);
    } else if (strcmp(how, "t0.9") == 0) {
      scale_markers('t', 0.9);
    } else if (strcmp(how, "t1.1") == 0) {
      scale_markers('t', 1.0/0.9);
    } else if (strcmp(how, "t2.0") == 0) {
      scale_markers('t', 2.0);
    }
}
