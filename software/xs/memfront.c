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

#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>

#include "list.h"
#include "defines.h"
#include "global_structs.h"
#include "mem.h"

/*** External variables ***/
void PostErrorDialog(Widget, char *);
void PostWarningDialog(Widget, char *);
void PostMessageDialog(Widget, char *);
void ManageDialogCenteredOnPointer(Widget);

/*** Local variables ***/
static int     MEM_in_progress = 0, MEM_halt = 0;
static MAP    *memMap;
static MEMData memdata;

typedef struct {
    MAP   **m;
    Widget *w;
} mapWidget;

typedef struct {
    Widget w;
    Widget *e;
} dWidget;

static void set_default_memdata()
{
    memdata.aIter  = MEM_DEFAULT_AITER;
    memdata.eIter  = MEM_DEFAULT_EITER;
    memdata.aGain  = MEM_DEFAULT_AGAIN;
    memdata.aLimit = MEM_DEFAULT_ALIM;
    memdata.eLimit = MEM_DEFAULT_ELIM;
}

void init_memdata()
{
    memdata.beam   = MEM_DEFAULT_BEAM;
    memdata.chi2   = MEM_DEFAULT_CHI2;
    memdata.obs    = NULL;
    memdata.mem    = NULL;
    memdata.doBlank = 0;
    memdata.blankLim = memdata.beam;
    
    set_default_memdata();
}

int QueryMEM()
{
    return MEM_in_progress;
}

int QueryHaltMEM()
{
    return MEM_halt;
}

static void MapMEMGet(Widget w, mapWidget *mw, XmListCallbackStruct *cb)
{
    int n, nL;
    list *pL;
    list *mL;
    MAP *m = NULL;

    list *get_action_list();
    list *get_maplist();
    void wprintf();

    mL = get_maplist();

    if ((pL = get_action_list(cb, &nL, *mL)) == NULL)
        return;

    for (n=0; n<nL; n++)
        m = (MAP *)DATA(pL[n]);

    *(mw->m) = m;

    wprintf(mw->w[0], "MEMed %s", m->name);
    wprintf(mw->w[1], "%.3f", m->b.maj);
    wprintf(mw->w[3], "%.3f", m->xleft);
    wprintf(mw->w[4], "%.3f", m->xright);
    wprintf(mw->w[5], "%.3f", -m->xspacing);
    wprintf(mw->w[6], "%.3f", m->ylower);
    wprintf(mw->w[7], "%.3f", m->yupper);
    wprintf(mw->w[8], "%.3f", m->yspacing);

    free(pL);
}

static void ToggleButtCallback(Widget w, dWidget *d,
                               XmToggleButtonCallbackStruct *cd)
{
    if (cd->set) {
        memdata.doBlank = 1;
    } else {
        memdata.doBlank = 0;
    }
}

static void DoMEMMap(Widget w, dWidget *d, XmListCallbackStruct *cb)
{
    int i, j, nX, nY, error_code;
    double xlef, xrig, xspa, ylow, yupp, yspa;
    string buf;
    MAP *m;
    list *mL;

    void wdscanf(), wsscanf(), wiscanf(), SetWatchCursor();
    void MapDraw();
    int MakeMEM();
    MAP *new_map();
    list *map_delete();
    list *get_maplist();
    
    mL = get_maplist();

    if (!memMap) {
        PostWarningDialog(w, "No map selected to be MEMed.");
        return;
    }
    
    wdscanf(d->e[1], &memdata.beam);
    if (memdata.beam <= 0.0) {
        sprintf(buf, "Beam size %f > 0.0.", memdata.beam);
        PostErrorDialog(w, buf);
        return;
    }
    if (memdata.doBlank) {
        wdscanf(d->e[2], &memdata.blankLim);
        if (memdata.blankLim < 0.0) {
            sprintf(buf, "Blanking limit %f >= 0.0.", memdata.blankLim);
            PostErrorDialog(w, buf);
            return;
        }
    }
    wdscanf(d->e[3], &xlef);
    wdscanf(d->e[4], &xrig);
    wdscanf(d->e[5], &xspa);
    xspa *= -1.0;
    if (xlef == xrig || xspa == 0.0 || (xrig - xlef)/xspa < 0.0) {
        sprintf(buf, "Error in X coordinates: %f %f %f.",
                xlef, xrig, xspa);
        PostErrorDialog(w, buf);
        return;
    }
    wdscanf(d->e[6], &ylow);
    wdscanf(d->e[7], &yupp);
    wdscanf(d->e[8], &yspa);
    if (ylow == yupp || yspa == 0.0 || (yupp - ylow)/yspa < 0.0) {
        sprintf(buf, "Error in Y coordinates: %f %f %f.",
                ylow, yupp, yspa);
        PostErrorDialog(w, buf);
        return;
    }
    
    wiscanf(d->e[9], &memdata.aIter);
    wiscanf(d->e[10], &memdata.eIter);
    if (memdata.aIter < 0 || memdata.eIter < 0) {
        sprintf(buf, "Iterations must be positive: Approx=%d Exact=%d.",
                memdata.aIter, memdata.eIter);
        PostErrorDialog(w, buf);
        return;
    }
    
    wdscanf(d->e[11], &memdata.chi2);
    if (memdata.chi2 <= 0.0) {
        sprintf(buf, "Desired Chi^2-value %f must be positive.", memdata.chi2);
        PostErrorDialog(w, buf);
        return;
    }
    
    if (memdata.aIter > 0) {
        wdscanf(d->e[12], &memdata.aGain);
        if (memdata.aGain <= 0.0 || memdata.aGain >= 1.0) {
            sprintf(buf, "Approx. gain %f must be in the interval ]0,1[.",
                    memdata.aGain);
            PostErrorDialog(w, buf);
            return;
        }

        wdscanf(d->e[13], &memdata.aLimit);
        if (memdata.aLimit <= 0.0) {
            sprintf(buf, "Approx. limit %f must be positive.", memdata.aLimit);
            PostErrorDialog(w, buf);
            return;
        }
    }
    
    if (memdata.eIter > 0) {
        wdscanf(d->e[14], &memdata.eLimit);
        if (memdata.eLimit <= 0.0) {
            sprintf(buf, "Exact limit %f must be positive.", memdata.aLimit);
            PostErrorDialog(w, buf);
            return;
        }
    }
    
    if (memMap->memed && !memMap->original) {
        sprintf(buf, "This is a previously MEMed map lacking obs. data.");
        PostErrorDialog(w, buf);
        return;
    }
    
    nX = 1 + NINT((xrig - xlef)/xspa);
    nY = 1 + NINT((yupp - ylow)/yspa);
    
    if (memMap->memed) {
        if (nX != memMap->i_no || nY != memMap->j_no) {
            sprintf(buf,
              "To continue to MEM a map, its size cannot be altered.");
            PostErrorDialog(w, buf);
            return;
        }
    }

    m = new_map(mL, nX, nY);
    
    m->type    = memMap->type;
    m->swapped = memMap->swapped;
    m->memed   = memMap->memed;
    if (m->memed) {
        for (i=0; i<nX; i++) {
            for (j=0; j<nY; j++) {
                m->d[i][j] = memMap->d[i][j];
                m->e[i][j] = memMap->e[i][j];
                m->f[i][j] = memMap->f[i][j];
            }
        }
        m->lam1 = memMap->lam1;
        m->lam2 = memMap->lam2;
    }
    m->x0      = memMap->x0;
    m->y0      = memMap->y0;
    m->equinox = memMap->equinox;
    m->epoch   = memMap->epoch;
    m->v       = memMap->v;
    m->dv      = memMap->dv;
    
    m->ndata    = nX * nY;
    m->i_min    = NINT(xlef/xspa);
    m->j_min    = NINT(ylow/yspa);
    m->j_min    = NINT(xrig/xspa);
    m->j_max    = NINT(yupp/yspa);
    m->xleft    = xlef;
    m->xright   = xrig;
    m->xspacing = xspa;
    m->ylower   = ylow;
    m->yupper   = yupp;
    m->yspacing = yspa;
    
 /*  If the "observed" map already has been MEMed, use the original
  *  observed map so that one can continue to MEM it.
  */
    if (m->memed)
        memdata.obs = memMap->original;
    else
        memdata.obs = memMap;
    memdata.mem = m;
    
    MEM_halt = 0;
    SetWatchCursor(True);
    MEM_in_progress = 1;
    
    error_code = MakeMEM(&memdata);
    
    MEM_in_progress = 0;
    SetWatchCursor(False);
    
    if (error_code != 0) {
        sprintf(buf, "Internal MEM error=%d, see the message log.",
                error_code);
        PostErrorDialog(w, buf);
        map_delete(mL, *mL);
        return;
    }

    wsscanf(d->e[0], m->name);
    sprintf(buf, "MEMed map stored as '%s', size %d x %d.", m->name, nX, nY);
    PostMessageDialog(NULL, buf);

    XtDestroyWidget(d->w);
    MapDraw(NULL, m, NULL);
}

static void revert_to_def_values(Widget w, dWidget *d, XmListCallbackStruct *cb)
{
    void wprintf();
    
    set_default_memdata();
    
    wprintf(d->e[9],  "%d", memdata.aIter);
    wprintf(d->e[10], "%d", memdata.eIter);
    wprintf(d->e[12], "%f", memdata.aGain);
    wprintf(d->e[13], "%e", memdata.aLimit);
    wprintf(d->e[14], "%e", memdata.eLimit);
}

static void DoHaltMEM(Widget w, Widget p, XmListCallbackStruct *cb)
{
    if (MEM_in_progress)
        MEM_halt = 1;
}

Widget ThreeHorEdit(Widget p, Widget *e1, Widget *e2, Widget *e3)
{
    int n = 0;
    Widget rc;
    Dimension wid = 300;

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, p,
                                 XmNorientation, XmHORIZONTAL,
                                 XmNpacking, XmPACK_TIGHT,
                                 NULL);
    if (e1) n++;
    if (e2) n++;
    if (e3) n++;
    
    if (n) wid = 300/n;
    
    if (e1)
        *e1 = XtVaCreateManagedWidget("edit", xmTextWidgetClass, rc,
                                      XmNwidth, wid,
                                      NULL);
    if (e2)
        *e2 = XtVaCreateManagedWidget("edit", xmTextWidgetClass, rc,
                                      XmNwidth, wid,
                                      NULL);
    if (e3)
        *e3 = XtVaCreateManagedWidget("edit", xmTextWidgetClass, rc,
                                      XmNwidth, wid,
                                      NULL);

    return rc;
}

void CreateMEMDialog(Widget parent, Arg *args, int nArgs)
{
    int n;
    Arg wargs[10];
    Widget form, rc, maps, butt, sep, cancelB, applyB, defaultB, haltB;
    static Widget edit[15];
    static mapWidget mw;
    static dWidget dw;

    void wprintf(), cancel_dialog();

    n = 0;
    XtSetArg(wargs[n], XmNautoUnmanage, False); n++;
    XtSetArg(wargs[n], XmNtitle, "MEM Deconvolution"); n++;
    form = XmCreateFormDialog(parent, "deconvolution", wargs, n);

    n = 0;
    XtSetArg(wargs[n], XmNorientation,       XmVERTICAL); n++;
    XtSetArg(wargs[n], XmNpacking,           XmPACK_TIGHT); n++;
    rc    = XtCreateWidget("rowcol", xmRowColumnWidgetClass,
                           form, wargs, n);

    maps  = XmCreateScrolledList(rc,  "list",  args,  nArgs);

    XtCreateManagedWidget("Name of MEMed map:", xmLabelWidgetClass,
                          rc, NULL, 0);
    edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                    rc, NULL, 0);
    XtCreateManagedWidget("Beam size:", xmLabelWidgetClass,
                          rc, NULL, 0);
    edit[1] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                    rc, NULL, 0);
    butt = XtVaCreateManagedWidget("Blank positions off by",
                                   xmToggleButtonWidgetClass, rc,
                                   XmNset, memdata.doBlank ? True : False,
                                   NULL);
    edit[2] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                    rc, NULL, 0);

    XtCreateManagedWidget("X: (Left, right, spacing)", xmLabelWidgetClass,
                                    rc, NULL, 0);
    ThreeHorEdit(rc, &edit[3], &edit[4], &edit[5]);

    XtCreateManagedWidget("Y: (Lower, upper, spacing)", xmLabelWidgetClass,
                                    rc, NULL, 0);
    ThreeHorEdit(rc, &edit[6], &edit[7], &edit[8]);

    XtCreateManagedWidget("Number of iterations: (Approx., Exact)",
                                    xmLabelWidgetClass,
                                    rc, NULL, 0);
    ThreeHorEdit(rc, &edit[9], &edit[10], NULL);

    XtCreateManagedWidget("Aim at Chi^2-value:",
                                    xmLabelWidgetClass,
                                    rc, NULL, 0);
    edit[11] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                    rc, NULL, 0);
    XtCreateManagedWidget("Approx. method: (Gain, Limit)",
                                    xmLabelWidgetClass,
                                    rc, NULL, 0);
    ThreeHorEdit(rc, &edit[12], &edit[13], NULL);

    XtCreateManagedWidget("Exact method: (Limit)",
                                    xmLabelWidgetClass,
                                    rc, NULL, 0);
    edit[14] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                    rc, NULL, 0);

    dw.w = form;
    dw.e = edit;
    XtAddCallback(butt, XmNvalueChangedCallback,
                  (XtCallbackProc)ToggleButtCallback, &dw);
    
    sep = XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
				                  form, XmNseparatorType,
				                  XmSHADOW_ETCHED_IN, NULL);
    applyB = XtCreateManagedWidget("MEM", xmPushButtonWidgetClass,
                                   form, NULL, 0);
    XtAddCallback(applyB, XmNactivateCallback,
                  (XtCallbackProc)DoMEMMap, &dw);

    cancelB = XtCreateManagedWidget(BUTT_CANCEL, xmPushButtonWidgetClass,
                                    form, NULL, 0);
    XtAddCallback(cancelB, XmNactivateCallback,
                  (XtCallbackProc)cancel_dialog, form);
    
    defaultB = XtCreateManagedWidget("default", xmPushButtonWidgetClass,
                                     form, NULL, 0);
    XtAddCallback(defaultB, XmNactivateCallback,
                  (XtCallbackProc)revert_to_def_values, &dw);
    
    haltB = XtCreateManagedWidget("abort MEM", xmPushButtonWidgetClass,
                                  form, NULL, 0);
    XtAddCallback(haltB, XmNactivateCallback,
                  (XtCallbackProc)DoHaltMEM, form);

    mw.m = &memMap;
    mw.w = edit;
    XtAddCallback(maps, XmNsingleSelectionCallback,
                  (XtCallbackProc)MapMEMGet, &mw);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetArg(wargs[n], XmNrightAttachment,   XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,       10); n++;
    XtSetValues(rc, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         rc); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,        1); n++;
    XtSetArg(wargs[n], XmNrightAttachment,   XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,       1); n++;
    XtSetValues(sep, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,  XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,      10); n++;
    XtSetValues(applyB, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,        applyB); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,  XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,      10); n++;
    XtSetValues(defaultB, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,        defaultB); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,  XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,      10); n++;
    XtSetValues(haltB, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,        haltB); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,  XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,      10); n++;
    XtSetValues(cancelB, wargs, n);

    XtManageChild(maps);
    XtManageChild(rc);

    wprintf(edit[0],  "<none>");
    wprintf(edit[1],  "%f", memdata.beam);
    wprintf(edit[2],  "%f", memdata.blankLim);
    wprintf(edit[3],  "%f", 0.0);
    wprintf(edit[4],  "%f", 0.0);
    wprintf(edit[5],  "%f", 0.0);
    wprintf(edit[6],  "%f", 0.0);
    wprintf(edit[7],  "%f", 0.0);
    wprintf(edit[8],  "%f", 0.0);
    wprintf(edit[9],  "%d", memdata.aIter);
    wprintf(edit[10], "%d", memdata.eIter);
    wprintf(edit[11], "%f", memdata.chi2);
    wprintf(edit[12], "%f", memdata.aGain);
    wprintf(edit[13], "%e", memdata.aLimit);
    wprintf(edit[14], "%e", memdata.eLimit);
    
    ManageDialogCenteredOnPointer(form);
}
