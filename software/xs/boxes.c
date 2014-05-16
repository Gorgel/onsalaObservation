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
#include <string.h>
#include <math.h>

#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Scale.h>

#include "defines.h"
#include "global_structs.h"
#include "menus.h"
#include "dialogs.h"

#ifdef HAVE_LIBPGPLOT
#include "cpgplot.h"
#endif

/*** External variables ***/
extern BOX     box;
extern BLINE   bl;
extern PSDATA  ps;
extern int     mod_sel, rbox_sel, pgplot;
extern VIEW   *vP;
extern GLOBAL *gp;

void   PostErrorDialog(Widget, char *);
void   PostWarningDialog(Widget, char *);
void   PostMessageDialog(Widget, char *);
void   ManageDialogCenteredOnPointer(Widget);
void   SetPGStyle(PSSTY *);
Widget CreateOptionMenu(Widget, MenuBarItem *);
void   SetDefaultOptionMenuItem(Widget, int);

double SpecUnitConv();
double zmap(scanPtr), emap(scanPtr);
void   SetAnyToggle(char *, int);

/*** Local variables ***/
int nbox, nreg;
BOX boxar[MAXBOX], regs[MAXBOX];

typedef struct {
    Widget edit[2];
    int xunit;
    int type;
    BOX *b;
    Widget slider;
    unsigned long time;
    XtIntervalId movie_timer_id;
} EDITBOX;

#define TIMER_LTOR 0
#define TIMER_RTOL 1
#define TIMER_BAF  2

static int TimerDirType=TIMER_LTOR;

static void SetTimerDirCallback();
static MenuItem TimerDirData[] = {
   {"Left to right", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetTimerDirCallback, "0", NULL},
   {"Right to left", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetTimerDirCallback, "1", NULL},
   {"Back and forth", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetTimerDirCallback, "2", NULL},
EOI};
static MenuBarItem TimerDirMenu = {
   "Direction of box", ' ', True, TimerDirData
};


/* Check new box so no overlap occurs, returns 1 if new box is ok */
static int check_new_box(char *type, BOX b)
{
    int n;

    if (b.begin > b.end) return 0;

    if (strncmp(type, "box", 3) == 0) {
        if (nbox == 0) return 1;

        for (n=0; n<nbox; n++) {
            if (b.begin >= boxar[n].begin && b.begin <= boxar[n].end) return 0;
            if (b.end   >= boxar[n].begin && b.end   <= boxar[n].end) return 0;
            if (b.begin <= boxar[n].begin && b.end   >= boxar[n].end) return 0;
        }
    } else if (strncmp(type, "mom", 3) == 0) {
        if (nreg == 0) return 1;

        for (n=0; n<nreg; n++) {
            if (b.begin >= regs[n].begin && b.begin <= regs[n].end) return 0;
            if (b.end   >= regs[n].begin && b.end   <= regs[n].end) return 0;
            if (b.begin <= regs[n].begin && b.end   >= regs[n].end) return 0;
        }
    }
    return 1;
}

void new_box(Widget w, char *type, XtPointer call_data)
{
    void UpdateData();

    if (strncmp(type, "box", 3) == 0) {
        if (nbox < MAXBOX) {
            if (check_new_box(type, box) == 1) {   /* Box is ok to use! */
                boxar[nbox] = box;
                nbox++;
                SetAnyToggle("boxes", 0);
                UpdateData(SCALE_NONE, REDRAW);
            } else {
                PostErrorDialog(w, "New baseline box overlaps with old one.");
            }
        } else {
            PostErrorDialog(w, "Too many baseline boxes.");
        }
    } else if (strncmp(type, "mom", 3) == 0) {
        if (nreg < MAXBOX) {
            if (check_new_box(type, box) == 1) {   /* Box is ok to use! */
                regs[nreg] = box;
                nreg++;
                SetAnyToggle("boxes", 0);
                UpdateData(SCALE_NONE, REDRAW);
            } else {
                PostErrorDialog(w, "New moment box overlaps with old one.");
            }
        } else {
            PostErrorDialog(w, "Too many moment boxes.");
        }
    }
}

int SetSingleMomentBox(int unit, double x1, double x2)
{
    int tmp, c1, c2;
    
    if (!vP->s) return 1;
    
    nreg = 1;
    c1 = NINT(SpecUnitConv(UNIT_CHA, unit, x1));
    c2 = NINT(SpecUnitConv(UNIT_CHA, unit, x2));
    if (c1 > c2) {
        tmp = c1; c1 = c2; c2 = tmp;
    }
    
    if (c1 < 0) return 1;
    if (c2 >= vP->s->nChan) return 1;
    
    regs[0].begin = c1;
    regs[0].end = c2;
    
    return 0;
}

void invert_boxar_and_regs(scanPtr s)
{
    int n;
    BOX tmp;
    
    for (n=0; n<nbox; n++) {
        tmp.begin = s->nChan - 1 - boxar[n].end;
        tmp.end   = s->nChan - 1 - boxar[n].begin;
        boxar[n] = tmp;
    }
    for (n=0; n<nreg; n++) {
        tmp.begin = s->nChan - 1 - regs[n].end;
        tmp.end   = s->nChan - 1 - regs[n].begin;
        regs[n] = tmp;
    }
}

void draw_box(GC gc, BOX b)
{
    int i, first = 1;
    int x1, x2, y1, y2;
    double *d, ymax=0.0, ymin=0.0;

    int yunit2y(), chan2x();
    double *chan2s();
    void draw_xbox();

    for (i=b.begin; i<=b.end; i++) {
        if ((d = chan2s(i)) == NULL) continue;
        if (first) {
            ymin = ymax = *d;
            first = 0;
        } else {
            if (*d > ymax) ymax = *d;
            if (*d < ymin) ymin = *d;
        }
    }
    x1 = chan2x(b.begin);
    x2 = chan2x(b.end);
    if (!first) {
        if (ymin == ymax) {
            y1 = yunit2y(ymin - 0.2*fabs(ymin));
            y2 = yunit2y(ymin + 0.2*fabs(ymin));
        } else {
            y1 = yunit2y(ymin);
            y2 = yunit2y(ymax);
        }
    } else {
        y1 = vP->min_y - vP->box_h/10;
        y2 = vP->min_y - (9*vP->box_h)/10;
    }
    draw_xbox(gc, x1, y1, x2, y2);
}

int GetBox(int type, int n, int *b, int *e)
{
    if (type == BOX_MOM) {
        if (n < 0 || n >= nreg) return 1;
        *b = regs[n].begin;
        *e = regs[n].end;
    } else if (type == BOX_RMS) {
        if (n < 0 || n >= nbox) return 1;
        *b = boxar[n].begin;
        *e = boxar[n].end;
    } else
        return 1;
        
    return 0;
}

void draw_boxes(GC gc1, GC gc2)
{
    int n, i, first = 1;
    int x1, x2, y1, y2;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx1, fx2, fy1=0.0, fy2=0.0;
#endif
    double *d, ymax=0.0, ymin=0.0;

    int chan2x(), yunit2y();
    double *chan2s(), chan2xunit();
    void draw_xbox();

    for (n=0; n<nbox; n++) {
        for (i=boxar[n].begin; i<=boxar[n].end; i++) {
            if ((d = chan2s(i)) == NULL) continue;
            if (first) {
                ymin = ymax = *d;
                first = 0;
            } else {
                if (*d > ymax) ymax = *d;
                if (*d < ymin) ymin = *d;
            }
        }
    }
    if (!first) {
        if (ymin == ymax) {
            y1 = yunit2y(ymin - 0.2*fabs(ymin));
            y2 = yunit2y(ymin + 0.2*fabs(ymin));
        } else {
            y1 = yunit2y(ymin);
            y2 = yunit2y(ymax);
        }
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            if (ymin == ymax) {
                fy1 = (PLFLT)(ymin - 0.2*fabs(ymin));
                fy2 = (PLFLT)(ymin + 0.2*fabs(ymin));
            } else {
                fy1 = (PLFLT)ymin;
                fy2 = (PLFLT)ymax;
            }
        }
#endif
    } else {
        y1 = vP->min_y - vP->box_h/10;
        y2 = vP->min_y - (9*vP->box_h)/10;
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fy1 = (PLFLT)(vP->ylower + 0.1*(vP->yupper - vP->ylower));
            fy2 = (PLFLT)(vP->yupper - 0.1*(vP->yupper - vP->ylower));
        }
#endif
    }

#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.blbox);
    }
#endif

    for (n=0; n<nbox; n++) {
        x1 = chan2x(boxar[n].begin);
        x2 = chan2x(boxar[n].end);
        draw_xbox(gc1, x1, y1, x2, y2);
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx1 = (PLFLT)chan2xunit(boxar[n].begin);
            fx2 = (PLFLT)chan2xunit(boxar[n].end);
            cpgrect(fx1, fx2, fy1, fy2);
        }
#endif
    }

    first = 1;
    for (n=0; n<nreg; n++) {
        for (i=regs[n].begin; i<=regs[n].end; i++) {
            if ((d = chan2s(i)) == NULL) continue;
            if (first) {
                ymin = ymax = *d;
                first = 0;
            } else {
                if (*d > ymax) ymax = *d;
                if (*d < ymin) ymin = *d;
            }
        }
    }

    if (!first) {
        if (ymin == ymax) {
            y1 = yunit2y(ymin - 0.2*fabs(ymin));
            y2 = yunit2y(ymin + 0.2*fabs(ymin));
        } else {
            y1 = yunit2y(ymin);
            y2 = yunit2y(ymax);
        }
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            if (ymin == ymax) {
                fy1 = (PLFLT)(ymin - 0.2*fabs(ymin));
                fy2 = (PLFLT)(ymin + 0.2*fabs(ymin));
            } else {
                fy1 = (PLFLT)ymin;
                fy2 = (PLFLT)ymax;
            }
        }
#endif
    } else {
        y1 = vP->min_y - vP->box_h/10;
        y2 = vP->min_y - (9*vP->box_h)/10;
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fy1 = (PLFLT)(vP->ylower + 0.1*(vP->yupper - vP->ylower));
            fy2 = (PLFLT)(vP->yupper - 0.1*(vP->yupper - vP->ylower));
        }
#endif
    }

#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.mobox);
    }
#endif

    for (n=0; n<nreg; n++) {
        x1 = chan2x(regs[n].begin);
        x2 = chan2x(regs[n].end);
        draw_xbox(gc2, x1, y1, x2, y2);
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx1 = (PLFLT)chan2xunit(regs[n].begin);
            fx2 = (PLFLT)chan2xunit(regs[n].end);
            cpgrect(fx1, fx2, fy1, fy2);
        }
#endif
    }
}

void box_reset(Widget w, char *client_data, XtPointer call_data)
{
    void UpdateData();

    if (strncmp(client_data, "ball", 4) == 0) {
        nbox = 0;
    } else if (strncmp(client_data, "mall", 4) == 0) {
        nreg = 0;
    } else if (strncmp(client_data, "all", 3) == 0) {
        nreg = 0;
        nbox = 0;
    } else if (strncmp(client_data, "bcursor", 7) == 0) {
        if (rbox_sel != 1) {
            rbox_sel = 1;
            mod_sel = 0;
        }
        return;
    } else if (strncmp(client_data, "mcursor", 7) == 0) {
        if (rbox_sel != 2) {
            rbox_sel = 2;
            mod_sel = 0;
        }
        return;
    } else if (strncmp(client_data, "bedit", 7) == 0) {
        if (rbox_sel != 3) {
            rbox_sel = 3;
            mod_sel = 0;
        }
        return;
    } else if (strncmp(client_data, "medit", 7) == 0) {
        if (rbox_sel != 4) {
            rbox_sel = 4;
            mod_sel = 0;
        }
        return;
    } else if (strncmp(client_data, "blatest", 7) == 0) {
        nbox--;
        if (nbox < 0) nbox = 0;
    } else if (strncmp(client_data, "mlatest", 7) == 0) {
        nreg--;
        if (nreg < 0) nreg = 0;
    }
    
    UpdateData(SCALE_NONE, REDRAW);
}

static int find_box(int type, int chan)
{
    int n, nfound = -1;
    
    if (type == BOX_RMS) {
        for (n=0; n<nbox; n++) {
            if (chan >= boxar[n].begin && chan <= boxar[n].end) {
                nfound = n;
                break;
            }
        }
    } else if (type == BOX_MOM) {
        for (n=0; n<nreg; n++) {
            if (chan >= regs[n].begin && chan <= regs[n].end) {
                nfound = n;
                break;
            }
        }
    }
    return nfound;
}

void remove_box(int type, int chan, int update)
{
    int n, nfound;
   
    void UpdateData();
   
    if (type == 1) {
        nfound = find_box(BOX_RMS, chan);
        if (nfound < 0) return;
        for (n=nfound+1; n<nbox; n++) boxar[n-1] = boxar[n];
        nbox--;
    } else {
        nfound = find_box(BOX_MOM, chan);
        if (nfound < 0) return;
        for (n=nfound+1; n<nreg; n++) regs[n-1] = regs[n];
        nreg--;
    }

    if (update) UpdateData(SCALE_NONE, REDRAW);
}

static void write_editbox_str(EDITBOX *eb)
{
    void wprintf();
    double SpecUnitConv();
    
    if (eb->xunit == UNIT_FRE) {
        wprintf(eb->edit[0], "%f",
                SpecUnitConv(UNIT_FRE, UNIT_CHA, (double)eb->b->begin));
        wprintf(eb->edit[1], "%f",
                SpecUnitConv(UNIT_FRE, UNIT_CHA, (double)eb->b->end));
    } else if (eb->xunit == UNIT_VEL) {
        wprintf(eb->edit[0], "%f",
                SpecUnitConv(UNIT_VEL, UNIT_CHA, (double)eb->b->begin));
        wprintf(eb->edit[1], "%f",
                SpecUnitConv(UNIT_VEL, UNIT_CHA, (double)eb->b->end));
    } else if (eb->xunit == UNIT_FOFF) {
        wprintf(eb->edit[0], "%f",
                SpecUnitConv(UNIT_FOFF, UNIT_CHA, (double)eb->b->begin));
        wprintf(eb->edit[1], "%f",
                SpecUnitConv(UNIT_FOFF, UNIT_CHA, (double)eb->b->end));
    } else if (eb->xunit == UNIT_FMHZ) {
        wprintf(eb->edit[0], "%f",
                SpecUnitConv(UNIT_FMHZ, UNIT_CHA, (double)eb->b->begin));
        wprintf(eb->edit[1], "%f",
                SpecUnitConv(UNIT_FMHZ, UNIT_CHA, (double)eb->b->end));
    } else {
        wprintf(eb->edit[0], "%d", eb->b->begin);
        wprintf(eb->edit[1], "%d", eb->b->end);
    }
}

static void get_editbox_str(Widget w, EDITBOX *eb, XmAnyCallbackStruct *cb)
{
    int tmp;
    double b, e;
    BOX new, *bptr;
    
    void wdscanf(), wiscanf();
    
    if (!eb) return;
    
    if (cb == NULL) {
        new = *(eb->b);
    } else if (eb->xunit == UNIT_FRE || eb->xunit == UNIT_VEL ||
        eb->xunit == UNIT_FOFF || eb->xunit == UNIT_FMHZ) {
        wdscanf(eb->edit[0], &b);
        wdscanf(eb->edit[1], &e);
        new.begin = NINT(SpecUnitConv(UNIT_CHA, eb->xunit, b));
        new.end   = NINT(SpecUnitConv(UNIT_CHA, eb->xunit, e));
    } else {
        wiscanf(eb->edit[0], &new.begin);
        wiscanf(eb->edit[1], &new.end);
    }
    
    if (new.begin > new.end) {
        tmp = new.begin; new.begin = new.end; new.end = tmp;
    }
    
    box = *(eb->b);
    
    if (eb->type == BOX_RMS) {
        remove_box(1, eb->b->begin, 0);
        if (!check_new_box("box", new)) {
            new_box(w, "box", NULL);
            PostErrorDialog(w, "The baseline box range is not valid.");
            return;
        }
        box = new;
        new_box(w, "box", NULL);
        bptr = &boxar[nbox-1];
    } else {
        remove_box(2, eb->b->begin, 0);
        if (!check_new_box("mom", new)) {
            new_box(w, "mom", NULL);
            PostErrorDialog(w, "The moment box range is not valid.");
            return;
        }
        box = new;
        new_box(w, "mom", NULL);
        bptr = &regs[nreg-1];
    }
    
    eb->b = bptr;
    write_editbox_str(eb);
}

static void GetSliderBoxValues(BOX *b, int *min, int *max, int *val)
{
    int lef, rig, wid, cen;
    
    lef = NINT(SpecUnitConv(UNIT_CHA, vP->xunit, vP->xleft));
    rig = NINT(SpecUnitConv(UNIT_CHA, vP->xunit, vP->xright));
    
    wid = b->end - b->begin;
    cen = (b->begin+b->end)/2;
    if (abs(wid) >= abs(rig-lef)) wid = 0;
    
    if (min) *min = lef + wid/2;
    if (max) *max = rig - wid/2;
    if (val) *val = cen;
}

static void box_moved(Widget w, EDITBOX *eb, XmScaleCallbackStruct *cb)
{
    int val = cb->value;
    int cen = (eb->b->begin + eb->b->end)/2;
    
    eb->b->begin += val - cen;
    eb->b->end += val - cen;
    get_editbox_str(w, eb, NULL);
}

static void speed_changed(Widget w, EDITBOX *eb, XmScaleCallbackStruct *cb)
{    
    eb->time = cb->value;
}

static void shift_box(EDITBOX *eb, XtIntervalId id)
{
    static int dir = 1;
    int val, min, max;
    
    XtVaGetValues(eb->slider, XmNvalue, &val, XmNminimum, &min,
                  XmNmaximum, &max, NULL);
    
    if (TimerDirType == TIMER_BAF) {
        if (val + dir > max || val + dir < min) dir = -dir;
    } else if (TimerDirType == TIMER_LTOR) {
        dir = 1;
        if (val + dir > max) {
            val = min-1;
            eb->b->begin += val - max;
            eb->b->end   += val - max;
        }
    } else if (TimerDirType == TIMER_RTOL) {
        dir = -1;
        if (val + dir < min) {
            val = max+1;
            eb->b->begin += val - min;
            eb->b->end   += val - min;
        }
    }
    val += dir;
    eb->b->begin += dir;
    eb->b->end   += dir;

    get_editbox_str(NULL, eb, NULL);
    XmScaleSetValue(eb->slider, val);
    
    eb->movie_timer_id = XtAppAddTimeOut(gp->app_cntxt, eb->time,
                                         (XtTimerCallbackProc)shift_box,
                                         (XtPointer)eb);
}

static void start_movie(Widget w, EDITBOX *eb, XmAnyCallbackStruct *cb)
{
    if (eb->movie_timer_id == 0) {
        eb->movie_timer_id = XtAppAddTimeOut(gp->app_cntxt, eb->time,
                                             (XtTimerCallbackProc)shift_box,
                                             (XtPointer)eb);
        XtSetSensitive(eb->slider, False);
        return;
    }
    PostWarningDialog(w, "Timer already running!");
}

static void stop_movie(Widget w, EDITBOX *eb, XmAnyCallbackStruct *cb)
{
    if (eb->movie_timer_id) {
        XtSetSensitive(eb->slider, True);
        XtRemoveTimeOut(eb->movie_timer_id);
        eb->movie_timer_id = 0;
        return;
    }
    if (w) PostMessageDialog(w, "Timer not running.");
}

static void SetTimerDirCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    int n = atoi(s);
    
    if (n != TimerDirType) TimerDirType = n;
}

static void editbox_cleanup(XtPointer user)
{
    if (!user) return;
    
    stop_movie(NULL, (EDITBOX *)user, NULL);
}

static void PostEditBoxDialog(Widget parent, int type, BOX *b)
{
    int min=0, max=100, val=50;
    string unit, title;
    Widget rc, rc1, rc2, slider, menu, speed, start, stop;
    XmString xstr;
    StdForm *sf;
    EDITBOX *eb;
    
    eb = (EDITBOX *) XtMalloc(sizeof(EDITBOX));
    if (!eb) return;
    
    if (vP->xunit == UNIT_FRE) {
        strcpy(unit, "Freq. limits (GHz)");
    } else if (vP->xunit == UNIT_VEL) {
        strcpy(unit, "Velocity limits");
    } else if (vP->xunit == UNIT_FOFF) {
        strcpy(unit, "Freq. offset limits");
    } else if (vP->xunit == UNIT_FMHZ) {
        strcpy(unit, "Freq. limits (MHz)");
    } else {
        strcpy(unit, "Channel limits");
    }
    
    if (type == BOX_RMS) {
        xstr = MKSTRING("Move RMS box");
        sprintf(title, "Edit RMS box");
    } else {
        xstr = MKSTRING("Move moment box");
        sprintf(title, "Edit moment box");
    }
    sf = PostStdFormDialog(parent, title,
             BUTT_APPLY, (XtCallbackProc)get_editbox_str, (XtPointer)eb,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             0, editbox_cleanup);

    rc = XtVaCreateWidget("editbox", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          NULL);

    rc1 = XtVaCreateWidget("editbox", xmRowColumnWidgetClass, rc,
                           XmNorientation, XmHORIZONTAL,
                           XmNnumColumns, 2,
                           XmNadjustLast, False,
                           XmNpacking, XmPACK_COLUMN,
                           NULL);
    XtVaCreateManagedWidget("Left edge of box", xmLabelWidgetClass, rc1, NULL);
    eb->edit[0] = XtVaCreateManagedWidget("begin", xmTextWidgetClass,
                                          rc1, NULL);
    XtVaCreateManagedWidget("Right edge of box", xmLabelWidgetClass, rc1, NULL);
    eb->edit[1] = XtVaCreateManagedWidget("end", xmTextWidgetClass,
                                          rc1, NULL);
    GetSliderBoxValues(b, &min, &max, &val);
    slider = XtVaCreateManagedWidget("scale", xmScaleWidgetClass, rc,
                                     XmNminimum, min,
                                     XmNvalue, val,
                                     XmNmaximum, max,
                                     XmNshowValue, True,
                                     XmNtitleString, xstr,
                                     XmNorientation, XmHORIZONTAL,
                                     NULL);
    XmStringFree(xstr);
    
    rc2 = XtVaCreateWidget("editbox", xmRowColumnWidgetClass, rc,
                           XmNorientation, XmHORIZONTAL,
                           XmNnumColumns, 1,
                           XmNadjustLast, FALSE,
                           XmNpacking, XmPACK_COLUMN,
                           NULL);
    XtVaCreateManagedWidget("Timer control:", xmLabelWidgetClass, rc2, NULL);
    start  = XtCreateManagedWidget("Start", xmPushButtonWidgetClass,
                                   rc2, NULL, 0);
    stop   = XtCreateManagedWidget("Stop", xmPushButtonWidgetClass,
                                   rc2, NULL, 0);
    
    menu = CreateOptionMenu(rc, &TimerDirMenu);
    SetDefaultOptionMenuItem(menu, TimerDirType);

    xstr = MKSTRING("Delay [ms]");
    speed = XtVaCreateManagedWidget("scale", xmScaleWidgetClass, rc,
                                    XmNminimum, 5,
                                    XmNvalue, 200,
                                    XmNmaximum, 1000,
                                    XmNshowValue, True,
                                    XmNtitleString, xstr,
                                    XmNorientation, XmHORIZONTAL,
                                    NULL);
    XmStringFree(xstr);
    
    XtAddCallback(start, XmNactivateCallback,
                  (XtCallbackProc)start_movie, eb);
    XtAddCallback(stop, XmNactivateCallback,
                  (XtCallbackProc)stop_movie, eb);

    eb->b = b;
    eb->xunit = vP->xunit;
    eb->type = type;
    eb->slider = slider;
    eb->time = 200;
    eb->movie_timer_id = 0;
    
    sf->user = (XtPointer)eb;
    
    XtAddCallback(slider, XmNvalueChangedCallback,
                  (XtCallbackProc)box_moved, eb);
    XtAddCallback(slider, XmNdragCallback,
                  (XtCallbackProc)box_moved, eb);
    XtAddCallback(speed, XmNvalueChangedCallback,
                  (XtCallbackProc)speed_changed, eb);
    XtAddCallback(speed, XmNdragCallback,
                  (XtCallbackProc)speed_changed, eb);

    ArrangeStdFormDialog(sf, rc);

    write_editbox_str(eb);
    
    XtManageChild(rc1);
    XtManageChild(rc2);
    XtManageChild(menu);
    XtManageChild(rc);

    ManageDialogCenteredOnPointer(sf->form);
}

void EditBox(Widget w, int type, int chan)
{
    int nfound, btype=0;
    BOX *b = NULL;
    
    if (type == 3) {         /* RMS box */
        btype = BOX_RMS;
        nfound = find_box(BOX_RMS, chan);
        if (nfound < 0) return;
        b = &boxar[nfound];
    } else if (type == 4) {  /* MOM box */
        btype = BOX_MOM;
        nfound = find_box(BOX_MOM, chan);
        if (nfound < 0) return;
        b = &regs[nfound];
    }
    
    if (b) PostEditBoxDialog(w, btype, b);
}

static void PrintMomentWidgets(int nb, int nc, double *x, double *e)
{
    string bstr, cstr;
    
    void wprintf();
    char *GetZType();
    
    if (nb == 1)
        strcpy(bstr, "box");
    else
        strcpy(bstr, "boxes");
    
    if (nc == 1)
        strcpy(cstr, "channel");
    else
        strcpy(cstr, "channels");
    
    if (nb > 0 && nc > 0)
        wprintf(gp->TMoment[0],  "%d %s, %d %s", nb, bstr, nc, cstr);
    else if (nb > 0 && nc == 0)
        wprintf(gp->TMoment[0],  "%d %s, no %s", nb, bstr, cstr);
    else if (nb == 0 && nc > 0)
        wprintf(gp->TMoment[0],  "No %s, %d %s", bstr, nc, cstr);
    else if (nb == 0 && nc == 0)
        wprintf(gp->TMoment[0],  "No %s, no %s", bstr, cstr);
    
    wprintf(gp->TMoment[1], "%s", GetZType(bstr));
    
    if (!x || !e)
        wprintf(gp->TMoment[2],  "");
    else if (fabs(*x) < 1.e-3)
        wprintf(gp->TMoment[2],  "%9.2e(%8.2e) %s", *x, *e, bstr);
    else if (fabs(*x) < 0.1)
        wprintf(gp->TMoment[2],  "%7.4f(%6.4f) %s", *x, *e, bstr);
    else if (fabs(*x) < 10.0)
        wprintf(gp->TMoment[2],  "%7.3f(%6.3f) %s", *x, *e, bstr);
    else if (fabs(*x) < 1000.0)
        wprintf(gp->TMoment[2],  "%7.1f(%6.1f) %s", *x, *e, bstr);
    else
        wprintf(gp->TMoment[2],  "%9.2e(%8.2e) %s", *x, *e, bstr);
}

void update_mom_data(scanPtr s)
{
    int n, i, x1, x2, nodata=0, imin=0, imax=0;
    double s0, s1=0.0, s2=0.0, s3=0.0, s4=0.0, s5=0.0, ymin=0.0, ymax=0.0;
    double e4=0.0, e5=0.0;
    double v, v1, v2, vmin=0.0, vmax=0.0, tmp, dv, z, ez;
    MOMENT *m;

    void wprintf();
    double chan2xunit();

    if (!s) return;
    
    m = &(s->mom);
    
    if (!m) return;
    
    for (n=0; n<nreg; n++) {
        x1 = regs[n].begin;
        x2 = regs[n].end;
        for (i=x1; i<=x2; i++) {
            if (i < 0 || i >= s->nChan) continue;
            v  = chan2xunit(i);
            v2 = v * v;
            s0 = s->d[i];
            s1 += s0;
            s2 += s0 * s0;
            s3 += s->e[i];
            s4 += v * s0;
            s5 += v2 * s0;
            e4 += v2;
            e5 += v2 * v2;
            if (nodata == 0) {
                ymin = ymax = s0;
		imin = imax = i;
            } else {
                if (s0 < ymin) {
		    ymin = s0;
		    imin = i;
		}
                if (s0 > ymax) {
		    ymax = s0;
		    imax = i;
		}
            }
            nodata++;
        }
        v1 = chan2xunit(x1);
        v2 = chan2xunit(x2);
        if (v1 > v2) {
            tmp = v1; v1 = v2; v2 = tmp;
        }
        if (n == 0) {
            vmin = v1;
            vmax = v2;
        } else {
            if (v1 < vmin) vmin = v1;
            if (v2 > vmax) vmax = v2;
        }
    }
    if (nodata > 0) {
        dv = fabs(s->velres);
        m->mean  = s1/((double)nodata);
        m->iint  = s1*dv;
        m->v     = (vmax + vmin)/2.0;
        m->dv    = (vmax - vmin + dv);
        m->TMin  = ymin;
        m->TMax  = ymax;
	m->xTMin = chan2xunit(imin);
	m->xTMax = chan2xunit(imax);
        if (s1 != 0.0) {
            m->vcent = s4/s1;
            v2 = (m->vcent)*(m->vcent);
            if (s5/s1 > v2)
                m->v2mom = sqrt(2.0*ALPHA*(s5/s1 - v2));
            else
                m->v2mom = 0.0;
        } else {
            m->vcent = 0.0;
            m->v2mom = 0.0;
        }
        if (m->sigma > 0.0) {
            m->iunc = m->sigma*sqrt((double)nodata)*dv;
            if (s1 != 0.0) {
                m->ucent = m->sigma/s1 *
                           sqrt(e4 + (double)nodata * m->vcent * m->vcent);
                if (m->v2mom != 0.0)
                    m->u2mom = m->sigma/2.0/s1/m->v2mom *
                               sqrt(e5 + (double)nodata * s5*s5/s1/s1);
                else
                    m->u2mom = 0.0;
            } else {
                m->ucent = 0.0;
                m->u2mom = 0.0;
            }
        } else {
            m->iunc = s3/sqrt((double)nodata)*dv;
            if (s1 != 0.0) {
                m->ucent = s3/(double)nodata/s1 *
                           sqrt(e4 + (double)nodata * m->vcent * m->vcent);
                m->u2mom = s3/(double)nodata/s1 *
                           sqrt(e5 + (double)nodata * m->vcent * m->vcent);
            } else {
                m->ucent = 0.0;
                m->u2mom = 0.0;
            }
        }
        if (s == vP->s) {
            z = zmap(s);
            ez = emap(s);
            PrintMomentWidgets(nreg, nodata, &z, &ez);
        }
    } else {
        m->mean  = 0.0;
        m->iint  = 0.0;
        m->iunc  = 0.0;
        m->v     = 0.0;
        m->dv    = 0.0;
        m->vcent = 0.0;
        m->v2mom = 0.0;
        m->TMin = m->TMax = 0.0;
        m->xTMin = m->xTMax = 0.0;
        nodata = 0;
        if (s == vP->s) {
            PrintMomentWidgets(nreg, nodata, NULL, NULL);
        }
    }
    m->nchan = nodata;
}

void AttachBoxesToDataset(Widget w, char *cmd, XtPointer cd)
{
    DataSetPtr d = vP->from;
    
    void detach_boxes_from_dataset(DataSetPtr);
    void attach_boxes_to_dataset(DataSetPtr, int, BOX *, int, BOX *);

    if (!d) {
        PostErrorDialog(NULL, "No current data set!");
        return;
    }
    
    if (strcmp(cmd, "attach") == 0) {
        if (nbox == 0 && nreg == 0) {
            PostErrorDialog(NULL, "There are no current boxes to attach!");
            return;
        }
        if (d->r || d->m) detach_boxes_from_dataset(d);
        attach_boxes_to_dataset(d, nbox, &boxar[0], nreg, &regs[0]);
    } else if (strcmp(cmd, "detach") == 0) {
        if (d->r == 0 && d->m == 0) {
            PostErrorDialog(NULL, "There are no boxes to detach from data set!");
            return;
        }
        detach_boxes_from_dataset(d);
    }
}

void SetBoxesFromDataset(DataSetPtr d)
{
    int n;
    
    if (!d) return;
    
    for (n=0; n<d->r; n++) {
        if (n >= MAXBOX) break;
        boxar[n] = d->rms[n];
    }
    nbox = d->r;
    
    for (n=0; n<d->m; n++) {
        if (n >= MAXBOX) break;
        regs[n] = d->mom[n];
    }
    nreg = d->m;
}

int SaveBoxesToFile(char *file)
{
    int n;
    FILE *fp;
    
    fp = fopen(file, "w");
    if (!fp) {
        return 1;
    }
    
    for (n=0; n<nbox; n++) {
        fprintf(fp, "B %d %d %d\n", n, boxar[n].begin, boxar[n].end);
    }
    for (n=0; n<nreg; n++) {
        fprintf(fp, "R %d %d %d\n", n, regs[n].begin, regs[n].end);
    }
    fclose(fp);
    
    return 0;
}

int ReadBoxesFromFile(char *file)
{
    int n;
    int err, begin, end;
    FILE *fp;
    char buf[MAXBUFSIZE];
    
    void send_line();
    
    fp = fopen(file, "r");
    if (!fp) {
        return 1;
    }
    
    while (fgets(buf, MAXBUFSIZE, fp) != NULL) {
        if (buf[0] == '#' || buf[0] == '!') continue;
        if (buf[0] == 'B') {
            err = sscanf(buf, "B %d %d %d", &n, &begin, &end);
            if (err == 3 && n < MAXBOX) {
                boxar[n].begin = begin;
                boxar[n].end   = end;
                nbox = n + 1;
            }
        } else if (buf[0] == 'R') {
            err = sscanf(buf, "R %d %d %d", &n, &begin, &end);
            if (err == 3 && n < MAXBOX) {
                regs[n].begin = begin;
                regs[n].end   = end;
                nreg = n + 1;
            }
        }
    }
    
    fclose(fp);

    SetAnyToggle("boxes", 0);
    
    sprintf(buf, "Read %d baseline and %d moment boxes from '%s'.",
            nbox, nreg, file);
    send_line(buf);
    
    return 0;
}
