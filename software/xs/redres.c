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
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>

#include "defines.h"
#include "global_structs.h"
#include "menus.h"
#include "dialogs.h"

/*** External variables ***/
extern VIEW *vP;

double gauss();
void   PostErrorDialog(Widget, char *);
void   ManageDialogCenteredOnPointer(Widget);
Widget PostWaitingDialog(Widget, char *, Widget *, int);
void   SetWaitingScale(Widget, int);
double SpecUnitBegin(scanPtr, int);
double SpecUnitEnd(scanPtr, int);
Widget CreateOptionMenu(Widget, MenuBarItem *);
void   SetDefaultOptionMenuItem(Widget, int);

list       scan_iterator(list, DataSetPtr);
int        count_scans(DataSetPtr);
scanPtr    copy_scanheader(DataSetPtr, int, scanPtr);
list      *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);
void       DeleteLastDataSet();

/*** Local variables ***/
#define REDRES_UNIFORM   0
#define REDRES_LORENTZ   1
#define REDRES_GAUSS     2
#define REDRES_EXPGAUSS  3

typedef struct _redres_struct {
     int      type;
     char    *name;
     double   res;
     double (*wfunc)();
} RedRes;

static double uniform(double, double, double, double);
static double lorentz(double, double, double, double);
static double exp_gauss(double, double, double, double);
static void   redres(RedRes *);

static RedRes ResTypes[] = {
   {REDRES_UNIFORM,  "Uniform",           0.0, uniform},
   {REDRES_LORENTZ,  "Lorentz",           0.0, lorentz},
   {REDRES_GAUSS,    "Gaussian",          0.0, gauss},
   {REDRES_EXPGAUSS, "1-Exp(-Gaussian)",  0.0, exp_gauss}
};

static void SetRedresTypeCallback();
static MenuItem RedresTypeData[] = {
   {"Uniform", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetRedresTypeCallback, "0", NULL},
   {"Lorentzian", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetRedresTypeCallback, "1", NULL},
   {"Gaussian", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetRedresTypeCallback, "2", NULL},
   {"1-Exp[-Gaussian]", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetRedresTypeCallback, "3", NULL},
EOI};
static MenuBarItem RedresTypeMenu = {
   "Type of weight function", ' ', True, RedresTypeData
};

static void SetBeginTypeCallback();
static MenuItem BeginTypeData[] = {
   {"No", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBeginTypeCallback, "0", NULL},
   {"Yes", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBeginTypeCallback, "1", NULL},
EOI};
static MenuBarItem BeginTypeMenu = {
   "Clip left at value:", ' ', True, BeginTypeData
};

static void SetEndTypeCallback();
static MenuItem EndTypeData[] = {
   {"No", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetEndTypeCallback, "0", NULL},
   {"Yes", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetEndTypeCallback, "1", NULL},
EOI};
static MenuBarItem EndTypeMenu = {
   "Clip right at value:", ' ', True, EndTypeData
};

static int redres_type, redres_unit, redres_use_rms, redres_begin, redres_end;
static double new_res, new_beg, new_end;
static double old_res, old_beg, old_end;
static Widget *global_edit;

static char *Redres_Help = "\
                       Reduce resolution (redres) help\n\
                       -------------------------------\n\
In this dialog you can specify the type of weight function to be used when\n\
reducing the resolution of the spectra. All weight functions are associated\n\
with a certain width (given in the currently displayed unit).\n\
The different weight functions are:\n\
    Uniform          Uniform weighting.\n\
    Lorentzian       Lorentzian weighting.\n\
    Gaussian         Gaussian weighting (default).\n\
    1-Exp[-Gaussian] Special.\n\
There is also an option to use the channel RMS in the weighting. This is only\n\
useful for spectra with varying RMS over the channels. Moreover, it is possible\n\
to specify the beginning and the end of the resulting spectra.\n\
";

void init_redres_parameters()
{
    redres_type = REDRES_GAUSS;
    redres_unit = UNIT_FRE;
    redres_use_rms = 0;
    redres_begin = redres_end = 1;
    new_res = new_beg = new_end = 0.0;
    old_res = old_beg = old_end = 0.0;
}

static void set_redres_data(scanPtr s)
{
    double chan2xunit();

    old_beg = new_beg = chan2xunit(0);
    old_end = new_end = chan2xunit(s->nChan - 1);
    redres_unit = vP->xunit;
    if (redres_unit == UNIT_FRE || redres_unit == UNIT_FOFF ||
        redres_unit == UNIT_FMHZ)
        old_res = new_res = s->freqres;
    else if (redres_unit == UNIT_VEL)
        old_res = new_res = s->velres;
    else
        old_res = new_res = 1.0;
}

static void do_redres(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    string buf;
    int new_chan, old_chan;

    void send_line(), wdscanf(), UpdateData();
    void SetWatchCursor();

    if (redres_begin) wdscanf(sf->edit[0], &new_beg);
    if (redres_end)   wdscanf(sf->edit[1], &new_end);
    wdscanf(sf->edit[2], &new_res);

    if (fabs(new_res) <= fabs(old_res)) {
        sprintf(buf, "New resolution (%f) must be larger than the old (%f).",
                fabs(new_res), fabs(old_res));
        PostErrorDialog(w, buf);
        return;
    }
    if (new_beg == new_end) {
        sprintf(buf, "End value (%f) must differ from start value (%f).",
                new_end, new_beg);
        PostErrorDialog(w, buf);
        return;
    }
    if ((new_end - new_beg)/new_res < 0.0) {
        sprintf(buf, "Sign error: (end - beg)/res <= 0.0!");
        PostErrorDialog(w, buf);
        return;
    }
    old_chan = NINT((old_end-old_beg)/old_res) + 1;
    new_chan = NINT((new_end-new_beg)/new_res) + 1;

    SetWatchCursor(True);

    switch (redres_type) {
        case REDRES_UNIFORM:
            sprintf(buf, "Doing uniform redres (new/old=%f).",
                    new_res/old_res);
            ResTypes[redres_type].res = sqrt(new_res*new_res -
                                             old_res*old_res);
            break;
        case REDRES_LORENTZ:
            sprintf(buf, "Doing Lorentz redres (new/old=%f).",
                    new_res/old_res);
            ResTypes[redres_type].res = sqrt(new_res*new_res -
                                             old_res*old_res);
            break;
        case REDRES_GAUSS:
            sprintf(buf, "Doing Gaussian redres (new/old=%f).",
                    new_res/old_res);
            ResTypes[redres_type].res = sqrt(new_res*new_res -
                                             old_res*old_res);
            break;
        case REDRES_EXPGAUSS:
            sprintf(buf, "Doing 1-Exp(-Gaussian) redres (new/old=%f).",
                    new_res/old_res);
            ResTypes[redres_type].res = sqrt(new_res*new_res -
                                             old_res*old_res);
            break;
        default:
            SetWatchCursor(False);
            PostErrorDialog(w, "Internal error: Unknown redres type.");
            return;
    }
    send_line(buf);
    sprintf(buf, "%s [%f, %f, %f] -> [%f, %f, %f]",
            ResTypes[redres_type].name, old_beg, old_end, old_res,
            new_beg, new_end, new_res);
    send_line(buf);

    sprintf(buf, "No of channels: %d -> %d", old_chan, new_chan);
    send_line(buf);

    redres(&ResTypes[redres_type]);

    SetWatchCursor(False);
    
    UpdateData(SCALE_ONLY_X, REDRAW);
}

static void SetRedresTypeCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    int n = atoi(s);
    
    if (n != redres_type) redres_type = n;
}

static void SetBeginTypeCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    int n = atoi(s);
    
    if (n != redres_begin) {
        redres_begin = n;
        XtSetSensitive(global_edit[0], n ? True : False);
    }
}

static void SetEndTypeCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    int n = atoi(s);
    
    if (n != redres_end) {
        redres_end = n;
        XtSetSensitive(global_edit[1], n ? True : False);
    }
}

static void set_redres_rms_usage(Widget w, Widget butt, XmAnyCallbackStruct *cb)
{
    if (redres_use_rms)
        redres_use_rms = 0;
    else
        redres_use_rms = 1;
}

static double uniform(double x, double c, double a, double w)
{
    if (fabs(x - c) <= w)
        return a;
    return 0.0;
}

static double lorentz(double x, double c, double a, double w)
{
    double arg = (x - c)/w;

    return a/(arg*arg + 1.0);
}

static double exp_gauss(double x, double c, double a, double w)
{
    return (1.0 - exp(-gauss(x, c, a, w)));
}

static scanPtr redres_one(RedRes *r, DataSetPtr dsp, scanPtr s,
                          double span, int nochan)
{
    int i, j, j_beg, j_end;
    double x, y, z, w, wsum, cres = r->res;
    scanPtr new = NULL;

    int xunit2chan();
    double chan2xunit();

    new = copy_scanheader(dsp, nochan, s);
    if (!new) return NULL;
    
    if (redres_unit == UNIT_FRE || redres_unit == UNIT_FOFF ||
        redres_unit == UNIT_FMHZ) {
        new->freq0 = new_beg;
        new->freqn = new_beg + (double)(new->nChan - 1) * new_res;
        new->freqres = new_res;
        new->velres = (new_res/old_res)*s->velres;
        new->vel0   = s->vel0 +
                      new->velres/new->freqres*(new->freq0 - s->freq0);
    } else if (redres_unit == UNIT_VEL) {
        new->vel0 = new_beg;
        new->velres = new_res;
        new->freqres = (new_res/old_res)*s->freqres;
        new->freq0   = s->freq0 +
                       new->freqres/new->velres*(new->vel0 - s->vel0);
        new->freqn = new->freq0 + (double)(new->nChan - 1) * new->freqres;
    } else {
        new->vel0 = s->vel0 + new_beg * s->velres;
        new->velres = (new_res/old_res)*s->velres;
        new->freqres = (new_res/old_res)*s->freqres;
        new->freq0   = s->freq0 + new_beg * s->freqres;
        new->freqn = new->freq0 + (double)(new->nChan - 1) * new->freqres;
    }
    for (i=0; i<new->nChan; i++) {
        x = new_beg + (double)i * new_res;

        j_beg = xunit2chan(x - 5.0*cres);
        j_end = xunit2chan(x + 5.0*cres);

        if (j_beg > j_end) {
            j = j_end; j_end = j_beg; j_beg = j;
        }

        if (j_beg < 0) j_beg = 0;
        if (j_end >= s->nChan) j_end = s->nChan-1;

        y = z = 0.0;
        wsum = 0.0;
        for (j=j_beg; j<=j_end; j++) {
            w = r->wfunc(chan2xunit(j), x, 1.0, cres);
            if (redres_use_rms && s->e[j] != 0.0 && !(s->fft))
                w /= (s->e[j] * s->e[j]);
            y += w * s->d[j];
            if (s->fft)
                z += w * s->e[j];
            else if (s->e[j] != 0.0)
                z += w / s->e[j] / s->e[j];
            wsum += w;
        }
        if (wsum == 0.0) {
            new->d[i] = 0.0;
            if (s->fft)
                new->e[i] = UNDEF;
            else
                new->e[i] = 0.0;
        } else {
            new->d[i] = y/wsum;
            if (s->fft)
                new->e[i] = z/wsum;
            else if (z != 0.0)
                new->e[i] = sqrt(wsum/z);
            else
                new->e[i] = UNDEF;
        }
    }
    
    return new;
}

static void redres(RedRes *r)
{
    int n, nochan;
    double span;
    Widget wait=NULL, scale;
    list curr = NULL;
    scanPtr s, new, tmp = vP->s;
    DataSetPtr dsp;
    
    dsp = new_dataset(get_listlist(), "Redressed <empty>", vP->from);
    if (!dsp) return;

    if (vP->mode == SHOW_SPE) {
        if (!redres_begin) new_beg = SpecUnitBegin(NULL, vP->xunit);
        if (!redres_end) new_end = SpecUnitEnd(NULL, vP->xunit);
        span = new_end - new_beg;
        nochan = NINT(span/new_res) + 1;
        new = redres_one(r, dsp, vP->s, span, nochan);
        if (new) {
            vP->from = vP->to = dsp;
            vP->s = new;
            sprintf(dsp->name, "Redressed (%s %g)", r->name, new_res);
        } else {
            DeleteLastDataSet();
        }
        return;
    } else
        n = count_scans(vP->from);
    
    if (n > WAITSPECTRA)
        wait = PostWaitingDialog(NULL, "Reducing resolution in spectra...",
                                 &scale, n);
    
    n = 0;
    while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
        s = (scanPtr)DATA(curr);
        vP->s = s;
        if (wait) SetWaitingScale(scale, n);
        if (!redres_begin) new_beg = SpecUnitBegin(s, vP->xunit);
        if (!redres_end) new_end = SpecUnitEnd(s, vP->xunit);
        span = new_end - new_beg;
        nochan = NINT(span/new_res) + 1;
        new = redres_one(r, dsp, s, span, nochan);
        if (!new) break;
        n++;
    }
    
    if (wait) XtDestroyWidget(wait);
    
    if (n == 0) {
        vP->s = tmp;
        DeleteLastDataSet();
    } else {
        vP->from = vP->to = dsp;
        vP->s = (scanPtr)DATA(dsp->scanlist);
        sprintf(dsp->name, "Redressed (%s %g)", r->name, new_res);
    }
}

void redres_scans(Widget wid, char *cmd, XtPointer call_data)
{
    string str, unit;
    Widget rc, w = wid, menu, rms_butt, menuB, menuE;
    StdForm *sf;

    void wprintf();

    while (!XtIsWMShell(w))
        w = XtParent(w);

    set_redres_data(vP->s);

    if (redres_unit == UNIT_FRE || redres_unit == UNIT_FOFF ||
        redres_unit == UNIT_FMHZ)
        strcpy(unit, "[GHz]");
    else if (redres_unit == UNIT_VEL)
        strcpy(unit, "[km/s]");
    else
        strcpy(unit, "[channel]");

    sf = PostStdFormDialog(w, "Reduce resolution",
             BUTT_APPLY, (XtCallbackProc)do_redres, NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, Redres_Help,
             3, NULL);

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);

    menu = CreateOptionMenu(rc, &RedresTypeMenu);
    SetDefaultOptionMenuItem(menu, redres_type);

    rms_butt = XtVaCreateManagedWidget("RMS usage?", xmToggleButtonWidgetClass,
                                rc, XmNset, redres_use_rms ? True : False,
                                NULL);
    XtAddCallback(rms_butt, XmNvalueChangedCallback,
                  (XtCallbackProc)set_redres_rms_usage, rms_butt);

    sprintf(str, "Resulting resolution %s", unit);
    XtCreateManagedWidget(str, xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[2]  = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                     rc, NULL, 0);
    
    menuB = CreateOptionMenu(rc, &BeginTypeMenu);
    SetDefaultOptionMenuItem(menuB, redres_begin);
    sf->edit[0]  = XtVaCreateManagedWidget("edit", xmTextWidgetClass, rc,
                          /* XmNsensitive, redres_begin ? True : False, */
                                       NULL);
    menuE = CreateOptionMenu(rc, &EndTypeMenu);
    SetDefaultOptionMenuItem(menuE, redres_end);
    sf->edit[1]  = XtVaCreateManagedWidget("edit", xmTextWidgetClass, rc,
                          /* XmNsensitive, redres_end ? True : False, */
                                       NULL);

    global_edit = sf->edit;
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(menu);
    XtManageChild(menuB);
    XtManageChild(menuE);

    wprintf(sf->edit[0], "%f", new_beg);
    wprintf(sf->edit[1], "%f", new_end);
    wprintf(sf->edit[2], "%f", new_res);
    
    XtSetSensitive(sf->edit[0], redres_begin ? True : False);
    XtSetSensitive(sf->edit[1], redres_end ? True : False);
    
    ManageDialogCenteredOnPointer(sf->form);
}
