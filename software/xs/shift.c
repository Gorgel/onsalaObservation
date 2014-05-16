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

#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushBG.h>
#include <Xm/Frame.h>

#include "defines.h"
#include "global_structs.h"
#include "menus.h"
#include "dialogs.h"

/*** External variables ***/
extern VIEW  *vP;

void PostErrorDialog(Widget, char *);
void PostWarningDialog(Widget, char *);
void PostMessageDialog(Widget, char *);
void ManageDialogCenteredOnPointer(Widget);
Widget CreateOptionMenu(Widget, MenuBarItem *);
void SetDefaultOptionMenuItem(Widget, int);

void wprintf();
void send_line(char *);
void wdscanf(Widget, double *);
void UpdateData(int, int);

int        count_scans(DataSetPtr);
list       scan_iterator(list, DataSetPtr);
scanPtr    copy_scanheader(DataSetPtr, int, scanPtr);
scanPtr    copy_scan(DataSetPtr, scanPtr);
list      *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);
void       DeleteLastDataSet();

/*** Local variables ***/
#define SHIFT_VEL_TO_FREF  0
#define SHIFT_FRE_TO_VREF  1
#define SHIFT_FRE_WITH_PAD 2
#define SHIFT_FRE_WITH_CLP 3
#define SHIFT_VEL_WITH_PAD 4
#define SHIFT_VEL_WITH_CLP 5
#define SHIFT_ODIN         6

#define LSR_TO_HEL 0
#define HEL_TO_LSR 1

typedef struct {
    int    type, veltype;
    double ref, val;
    double fbeg_o, fend_o;
    double fbeg_i, fend_i;
    double fres;
    Widget lref, lval;
} shift;

static shift Shift;
static string buf;

static void ShiftTypeCallback(Widget, char *, XmAnyCallbackStruct *);

MenuItem ShiftMenuData[] = {
  {"Set velocity at a reference frequency", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ShiftTypeCallback, "0", NULL},
  {"Set frequency at a reference velocity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ShiftTypeCallback, "1", NULL},
  {"Align in frequency with padding", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ShiftTypeCallback, "2", NULL},
  {"Align in frequency with clipping", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ShiftTypeCallback, "3", NULL},
  {"Align in velocity with padding", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ShiftTypeCallback, "4", NULL},
  {"Align in velocity with clipping", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ShiftTypeCallback, "5", NULL},
  /* {"Shift Odin scans", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ShiftTypeCallback, "4", NULL}, */
EOI};

MenuBarItem ShiftOptionMenu = {
   "Type of shift", ' ', True, ShiftMenuData
};

static void VelShiftTypeCallback(Widget, char *, XmAnyCallbackStruct *);

MenuItem VelShiftMenuData[] = {
  {"LSR to heliocentric", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, VelShiftTypeCallback, "0", NULL},
  {"Heliocentric to LSR", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, VelShiftTypeCallback, "1", NULL},
EOI};

MenuBarItem VelShiftOptionMenu = {
   "Type of shift", ' ', True, VelShiftMenuData
};

static char *Shift_Help = "\
                            Shift help\n\
                            ----------\n\
In this dialog you can select different ways of shifting the x-scale of the\n\
spectral data. The options are:\n\
    Set velocity at a reference frequency:\n\
          Here you select a reference frequency, for instance the rest\n\
          frequency of a molecular line. Here you can change the velocity\n\
          scale by entering the velocity corresponding to the reference\n\
          frequency. Only the velocity scale will be altered.\n\
    Set frequency at a reference velocity:\n\
          Here you select a reference velocity and may alter the frequency\n\
          scale according to the supplied frequency.  Only the frequency\n\
          scale is affected.\n\
    Align in frequency with padding:\n\
          For a data set consisting of 2 or more scans with (slightly)\n\
          different frequencies this option can be used to create a new\n\
          data set in which scans are padded with zeros at both ends so that\n\
          each scan will have an identical frequency scale.\n\
    Align in frequency with clipping:\n\
          Same as above but clip the scans instead of padding them.\n\
    Align in velocity with padding:\n\
          Same as above but use velocity axis instead of frequency.\n\
    Align in velocity with clipping:\n\
          Same as above but clip the scans instead of padding them.\n\
";

void init_shift_parameters()
{
    Shift.type = SHIFT_FRE_TO_VREF;
    Shift.ref  = 0.0;
    Shift.val  = 0.0;
    
    Shift.veltype = LSR_TO_HEL;
}

static int FindFreBorders(DataSetPtr d, shift *h)
{
    int first=1;
    list curr = NULL;
    scanPtr s;
    string buf;
    double fbo=0.0, fbi=0.0, feo=0.0, fei=0.0, df=0.0;
    
    if (!d || !h) return 1;
    
    while ( (curr = scan_iterator(curr, d)) != NULL) {
        s = (scanPtr)DATA(curr);
        if (!s) continue;
        if (first == 1) {
            first = 0;
            df = s->freqres;
            fbo = fbi = s->freq0;
            feo = fei = s->freqn;
            continue;
        } else if (fabs(s->freqres - df) >= 1.0e-5) {
            sprintf(buf, "Warning: fres=%f and %f.", s->freqres, df);
            send_line(buf);
            continue;
        }
        if (s->freqn > feo) feo = s->freqn;
        if (s->freqn < fei) fei = s->freqn;
        if (s->freq0 < fbo) fbo = s->freq0;
        if (s->freq0 > fbi) fbi = s->freq0;
    }
    
    if (df > 0.0) {
	h->fbeg_o = fbo;
	h->fbeg_i = fbi;
	h->fend_o = feo;
	h->fend_i = fei;
    } else {
	h->fbeg_o = fbi;
	h->fbeg_i = fbo;
	h->fend_o = fei;
	h->fend_i = feo;
    }
    h->fres = df;
    
    return 0;
}

static int FindVelBorders(DataSetPtr d, shift *h)
{
    int first=1;
    list curr = NULL;
    scanPtr s;
    string buf;
    double vbo=0.0, vbi=0.0, veo=0.0, vei=0.0, dv=0.0, veln;
    
    if (!d || !h) return 1;
    
    while ( (curr = scan_iterator(curr, d)) != NULL) {
        s = (scanPtr)DATA(curr);
        if (!s) continue;
        if (first == 1) {
            first = 0;
            dv = s->velres;
            vbo = vbi = s->vel0;
            veo = vei = s->vel0 + (s->nChan - 1)*dv;
            continue;
        } else if (fabs(s->velres - dv) >= 1.0e-5) {
            sprintf(buf, "Warning: vres=%f and %f.", s->velres, dv);
            send_line(buf);
            continue;
        }
	veln = s->vel0 + (s->nChan - 1)*s->velres;
        if (veln > veo) veo = veln;
        if (veln < vei) vei = veln;
        if (s->vel0 < vbo) vbo = s->vel0;
        if (s->vel0 > vbi) vbi = s->vel0;
    }
    
    if (dv > 0.0) {
	h->fbeg_o = vbo;
	h->fbeg_i = vbi;
	h->fend_o = veo;
	h->fend_i = vei;
    } else {
	h->fbeg_o = vbi;
	h->fbeg_i = vbo;
	h->fend_o = vei;
	h->fend_i = veo;
    }
    h->fres = dv;
    
    return 0;
}

static scanPtr ShiftSpe(DataSetPtr d, scanPtr s, shift *h)
{
    int n, nChan=0, n_old;
    double nref;
    scanPtr new = NULL;
    
    if (!d || !s || !h) return new;
    
    if (h->type == SHIFT_FRE_WITH_PAD || h->type == SHIFT_VEL_WITH_PAD) {
        nChan = (h->fend_o - h->fbeg_o)/h->fres + 1;
        new = copy_scanheader(d, nChan, s);
    } else if (h->type == SHIFT_FRE_WITH_CLP || h->type == SHIFT_VEL_WITH_CLP) {
        nChan = (h->fend_i - h->fbeg_i)/h->fres + 1;
        new = copy_scanheader(d, nChan, s);
    } else {
        new = copy_scan(d, s);
    }
    if (!new) return new;
    
    if (h->type == SHIFT_VEL_TO_FREF) {
        nref = (h->ref - s->freq0)/s->freqres;
        new->vel0 = h->val - nref*s->velres;
    } else if (h->type == SHIFT_FRE_TO_VREF) {
        nref = (h->ref - s->vel0)/s->velres;
        new->freq0 = h->val - nref*s->freqres;
        new->freqn = new->freq0 + (double)(s->nChan - 1)*s->freqres;
	new->lofreq = s->lofreq - s->freq0 + new->freq0;
    } else if (h->type == SHIFT_FRE_WITH_PAD) {
        new->freq0 = h->fbeg_o;
        new->freqn = new->freq0 + (double)(new->nChan - 1)*h->fres;
        new->freqres = h->fres;
        new->vel0 = s->vel0 + (new->freq0 - s->freq0)/new->freqres * s->velres;
        new->velres = -new->freqres*SPEEDOFLIGHT * 2.0/(new->freq0+new->freqn);
	new->lofreq = s->lofreq - s->freq0 + new->freq0;
        for (n=0; n<nChan; n++) {
            new->d[n] = 0.0;
            new->e[n] = 0.0;
            n_old = NINT((new->freq0 + (double)n * new->freqres - s->freq0)/
                    s->freqres);
            if (n_old < 0 || n_old >= s->nChan) continue;
            new->d[n] = s->d[n_old];
            new->e[n] = s->e[n_old];
        }
    } else if (h->type == SHIFT_FRE_WITH_CLP) {
        new->freq0 = h->fbeg_i;
        new->freqn = new->freq0 + (double)(new->nChan - 1)*h->fres;
        new->freqres = h->fres;
        new->vel0 = s->vel0 + (new->freq0 - s->freq0)/new->freqres * s->velres;
        new->velres = -new->freqres*SPEEDOFLIGHT*2.0/(new->freq0+new->freqn);
        /* printf("Aligning %d.%04d %f [%.2f->%.2f].\n", s->scan_no, s->subscan,
               (new->freq0 - s->freq0)/new->freqres, s->vel0, new->vel0); */
	new->lofreq = s->lofreq - s->freq0 + new->freq0;
        for (n=0; n<nChan; n++) {
            n_old = NINT((new->freq0 + (double)n * new->freqres - s->freq0)/
                    s->freqres);
            if (n_old < 0 || n_old >= s->nChan) continue;
            new->d[n] = s->d[n_old];
            new->e[n] = s->e[n_old];
        }
    } else if (h->type == SHIFT_VEL_WITH_PAD) {
        new->vel0 = h->fbeg_o;
        new->velres = h->fres;
        new->freq0 = s->freq0 + (new->vel0 - s->vel0)/new->velres * s->freqres;
        new->freqn = new->freq0 + (double)(new->nChan - 1)*s->freqres;
        new->freqres = -new->velres/SPEEDOFLIGHT / 2.0*(new->freq0+new->freqn);
	new->lofreq = s->lofreq - s->freq0 + new->freq0;
        for (n=0; n<nChan; n++) {
            new->d[n] = new->e[n] = 0.0;
            n_old = NINT((new->vel0 + (double)n * new->velres - s->vel0)/
                    s->velres);
            if (n_old < 0 || n_old >= s->nChan) continue;
            new->d[n] = s->d[n_old];
            new->e[n] = s->e[n_old];
        }
    } else if (h->type == SHIFT_VEL_WITH_CLP) {
        new->vel0 = h->fbeg_i;
        new->velres = h->fres;
        new->freq0 = s->freq0 + (new->vel0 - s->vel0)/new->velres * s->freqres;
        new->freqn = new->freq0 + (double)(new->nChan - 1)*s->freqres;
        new->freqres = -new->velres/SPEEDOFLIGHT/2.0*(new->freq0+new->freqn);
        /* printf("Aligning %d.%04d %f [%.2f->%.2f].\n", s->scan_no, s->subscan,
               (new->freq0 - s->freq0)/new->freqres, s->vel0, new->vel0); */
	new->lofreq = s->lofreq - s->freq0 + new->freq0;
        for (n=0; n<nChan; n++) {
            new->d[n] = new->e[n] = 0.0;
            n_old = NINT((new->vel0 + (double)n * new->velres - s->vel0)/
                    s->velres);
            if (n_old < 0 || n_old >= s->nChan) continue;
            new->d[n] = s->d[n_old];
            new->e[n] = s->e[n_old];
        }
    } else if (h->type == SHIFT_ODIN) {
        new->vel0  = s->vlsr - (h->val - s->freq0)/s->freqres * s->velres;
        new->freq0 = h->val - (h->ref - new->vel0)/s->velres *s->freqres;
        new->freqn = new->freq0 + (double)(s->nChan - 1)*s->freqres;
        new->vlsr  = h->ref;
	new->lofreq = s->lofreq - s->freq0 + new->freq0;
    }
    
    return new;
}

static scanPtr VelShiftSpe(DataSetPtr d, scanPtr s, shift *h)
{
    double vdiff;
    scanPtr new = NULL;
    
    double Vlsrheldiff(scanPtr);
    
    if (!d || !s || !h) return new;
    
    new = copy_scan(d, s);
    if (!new) return new;
    
    vdiff = Vlsrheldiff(s);
    
    if (h->veltype == LSR_TO_HEL) {
        new->vel0 -= vdiff;
        new->vlsr -= vdiff;
    } else {
        new->vel0 += vdiff;
        new->vlsr += vdiff;
    }
    
    return new;
}

static void SetShiftLabelStrings(StdForm *sf)
{
    Widget val, ref;
    
    if (sf) {
        val = sf->edit[0];
        ref = sf->edit[2];
    } else {
        val = Shift.lval;
        ref = Shift.lref;
    }
    
    if (Shift.type == SHIFT_VEL_TO_FREF) {
        wprintf(val, "%s", "Velocity (km/s) value");
        wprintf(ref, "%s", "at frequency (GHz) reference");
    } else if (Shift.type == SHIFT_FRE_TO_VREF) {
        wprintf(val, "%s", "Frequency (GHz) value");
        wprintf(ref, "%s", "at velocity (km/s) reference");
    } else if (Shift.type == SHIFT_FRE_WITH_PAD) {
        wprintf(val, "%s", "--- Not used ---");
        wprintf(ref, "%s", "--- Not used ---");
    } else if (Shift.type == SHIFT_FRE_WITH_CLP) {
        wprintf(val, "%s", "--- Not used ---");
        wprintf(ref, "%s", "--- Not used ---");
    } else if (Shift.type == SHIFT_ODIN) {
        wprintf(val, "%s", "Line rest frequency (GHz)");
        wprintf(ref, "%s", "Source LSR velocity (km/s)");
    }
}

static void ShiftTypeCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int type = atoi(str);
    
    if (type != Shift.type) {
        Shift.type = atoi(str);
        SetShiftLabelStrings(NULL);
    }
}

static void DoShift(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int n;
    list curr = NULL;
    scanPtr new;
    DataSetPtr dsp;
    shift *s = (shift *)sf->any;

    wdscanf(sf->edit[1], &(s->val));
    wdscanf(sf->edit[3], &(s->ref));

    dsp = new_dataset(get_listlist(), "Shifted", vP->from);
    if (!dsp) {
        PostErrorDialog(w, "Out of memory when allocating dataset.");
        return;
    }

    if ((s->type == SHIFT_FRE_WITH_PAD || s->type == SHIFT_FRE_WITH_CLP) 
                                            && count_scans(vP->from) > 0) {
        FindFreBorders(vP->from, s);
        sprintf(buf, "Outer borders: %f,%f   Inner borders: %f, %f",
                s->fbeg_o, s->fend_o, s->fbeg_i, s->fend_i);
        send_line(buf);
        n = 0;
        while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
            new = ShiftSpe(dsp, (scanPtr)DATA(curr), s);
            if (!new) break;
            n++;
        }
        if (n == 0) {
            PostErrorDialog(w, "Out of memory when allocating scan.");
            DeleteLastDataSet();
            return;
        }
        vP->to = vP->from = dsp;
        vP->s = (scanPtr)DATA(dsp->scanlist);
        sprintf(buf, "Shifted %d spectra.", n);
    } else if ((s->type == SHIFT_VEL_WITH_PAD || s->type == SHIFT_VEL_WITH_CLP) 
                                            && count_scans(vP->from) > 0) {
        FindVelBorders(vP->from, s);
        sprintf(buf, "Outer borders: %f,%f   Inner borders: %f, %f",
                s->fbeg_o, s->fend_o, s->fbeg_i, s->fend_i);
        send_line(buf);
        n = 0;
        while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
            new = ShiftSpe(dsp, (scanPtr)DATA(curr), s);
            if (!new) break;
            n++;
        }
        if (n == 0) {
            PostErrorDialog(w, "Out of memory when allocating scan.");
            DeleteLastDataSet();
            return;
        }
        vP->to = vP->from = dsp;
        vP->s = (scanPtr)DATA(dsp->scanlist);
        sprintf(buf, "Shifted %d spectra in velocity.", n);
    } else if (s->type < SHIFT_FRE_WITH_PAD && vP->mode == SHOW_SPE && vP->s) {
        new = ShiftSpe(dsp, vP->s, s);
        if (!new) {
            PostErrorDialog(w, "Out of memory when allocating scan.");
            DeleteLastDataSet();
            return;
        }
        vP->from = vP->to = dsp;
        vP->s = new;
        n = 1;
        sprintf(buf, "Shifted 1 spectrum with value %10.3e and ref. %10.3e.",
                s->val, s->ref);
    } else if (s->type < SHIFT_FRE_WITH_PAD && count_scans(vP->from) > 0) {
        n = 0;
        while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
            new = ShiftSpe(dsp, (scanPtr)DATA(curr), s);
            if (!new) break;
            n++;
        }
        if (n == 0) {
            PostErrorDialog(w, "Out of memory when allocating scan.");
            DeleteLastDataSet();
            return;
        }
        vP->to = vP->from = dsp;
        vP->s = (scanPtr)DATA(dsp->scanlist);
        sprintf(buf, "Shifted %d spectra of %d with value %10.3e and ref. %10.3e.",
                n, count_scans(vP->from), s->val, s->ref);
    } else if (s->type == SHIFT_ODIN && count_scans(vP->from) > 0) {
        n = 0;
        while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
            new = ShiftSpe(dsp, (scanPtr)DATA(curr), s);
            if (!new) break;
            n++;
        }
        if (n == 0) {
            PostErrorDialog(w, "Out of memory when allocating scan.");
            DeleteLastDataSet();
            return;
        }
        vP->to = vP->from = dsp;
        vP->s = (scanPtr)DATA(dsp->scanlist);
        sprintf(buf, "Shifted %d spectra of %d with value %10.3e and ref. %10.3e.",
                n, count_scans(vP->from), s->val, s->ref);
    } else {
        PostErrorDialog(w, "Couldn't find any spectra to shift.");
        return;
    }

    send_line(buf);
    
    if (s->type == SHIFT_FRE_WITH_PAD) {
        sprintf(dsp->name, "Aligned %s padded [%.3f,%.3f]",
                vP->s->name, s->fbeg_o, s->fend_o);
    } else if (s->type == SHIFT_FRE_WITH_CLP) {
        sprintf(dsp->name, "Aligned %s clipped [%.3f,%.3f]",
                vP->s->name, s->fbeg_i, s->fend_i);
    } else if (s->type == SHIFT_VEL_WITH_PAD) {
        sprintf(dsp->name, "Aligned %s padded [%.3f,%.3f] km/s",
                vP->s->name, s->fbeg_o, s->fend_o);
    } else if (s->type == SHIFT_VEL_WITH_CLP) {
        sprintf(dsp->name, "Aligned %s clipped [%.3f,%.3f] km/s",
                vP->s->name, s->fbeg_i, s->fend_i);
    } else if (s->type == SHIFT_ODIN) {
        sprintf(dsp->name, "%s shifted [%.3f,%.1f]",
                vP->s->name, s->val, s->ref);
    } else {
        sprintf(dsp->name, "Shifted %s [%g,%g]", vP->s->name, s->val, s->ref);
    }

    UpdateData(SCALE_ONLY_X, REDRAW);
}

void PostShiftDialog(Widget wid, char *cmd, XtPointer call_data)
{
    Widget rc, rc2, fr, menu;
    Widget w = wid;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Shift x-scale in spectra",
             BUTT_APPLY, (XtCallbackProc)DoShift, NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, Shift_Help,
	     4, NULL);
    sf->any = (XtPointer)(&Shift);

    rc = XtVaCreateManagedWidget("scalerc", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
                                  
    menu = CreateOptionMenu(rc, &ShiftOptionMenu);
    SetDefaultOptionMenuItem(menu, Shift.type);
				                   
    fr = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rc, 
				 XmNshadowType, XmSHADOW_OUT, NULL);

    rc2 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                  XmNorientation, XmVERTICAL,
				  NULL);
    
    sf->edit[0] = XtCreateManagedWidget("label", xmLabelWidgetClass,
                                        rc2, NULL, 0);
    sf->edit[1] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc2, NULL, 0);
    sf->edit[2] = XtCreateManagedWidget("label", xmLabelWidgetClass,
                                        rc2, NULL, 0);
    sf->edit[3] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc2, NULL, 0);
    
    Shift.lval = sf->edit[0];
    Shift.lref = sf->edit[2];
    
    ArrangeStdFormDialog(sf, rc);

    SetShiftLabelStrings(sf);
    
    wprintf(sf->edit[1], "%f", Shift.val);
    wprintf(sf->edit[3], "%f", Shift.ref);
    
    XtManageChild(menu);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static scanPtr ShiftVelSpe(DataSetPtr dsp, scanPtr s, double v)
{
    double dv, df;
    scanPtr new = NULL;
    
    if (!s) return new;
    
    new = copy_scan(dsp, s);
    if (!new) return new;
    
    dv = (s->vlsr - v);
    df = -dv * s->freqres / s->velres;
    new->freq0 -= df; 
    new->freqn -= df;
    new->vlsr = v;
    
    return new;
}

static scanPtr SetLOFreq(DataSetPtr dsp, scanPtr s, double f)
{
    scanPtr new = NULL;
    
    if (!s) return new;
    
    new = copy_scan(dsp, s);
    if (!new) return new;
    
    new->lofreq = f; 
    
    return new;
}

static void get_vlsr_str(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int n;
    double v;
    list curr = NULL;
    scanPtr new;
    DataSetPtr dsp;

    wdscanf(sf->edit[0], &v);
    sprintf(buf, "Nothing shifted!");
    
    dsp = new_dataset(get_listlist(), "Shifted", vP->from);
    if (!dsp) {
        PostErrorDialog(w, "Out of memory when allocating dataset.");
        return;
    }

    if (vP->mode == SHOW_SPE && vP->s) {
        new = ShiftVelSpe(dsp, vP->s, v);
        if (!new) {
            PostErrorDialog(w, "Out of memory when allocating scan.");
            DeleteLastDataSet();
            return;
        }
        vP->to = vP->from = dsp;
        sprintf(buf, "Shifted 1 spectrum from v=%7.3f to %7.3f.",
                vP->s->vlsr, v);
        vP->s = new;
    } else if (count_scans(vP->from) > 0) {
        n = 0;
        while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
            new = ShiftVelSpe(dsp, (scanPtr)DATA(curr), v);
            if (!new) break;
            n++;
        }
        if (n == 0) {
            PostErrorDialog(w, "Out of memory when allocating scan.");
            DeleteLastDataSet();
            return;
        }
        vP->to = vP->from = dsp;
        vP->s = (scanPtr)DATA(dsp->scanlist);
        sprintf(buf, "Shifted %d spectra of %d to v=%f.",
                n, count_scans(vP->from), v);
    } else {
        PostErrorDialog(w,
            "Couldn't find any spectra to shift the emission velocity in.");
        return;
    }
    send_line(buf);
    sprintf(dsp->name, "Shifted Vlsr to %g", v);

    UpdateData(SCALE_ONLY_X, REDRAW);
}

void PostVLSRDialog(Widget wid, char *cmd, XtPointer call_data)
{
    Widget w = wid, rc;
    double vlsr=0.0;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Set new emission velocity",
             BUTT_APPLY, (XtCallbackProc)get_vlsr_str, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
	     1, NULL);

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation,       XmVERTICAL,
				 NULL);
    XtCreateManagedWidget("New Vlsr:", xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass, rc,
                                        NULL, 0);

    ArrangeStdFormDialog(sf, rc);

    if (vP->s) vlsr = vP->s->vlsr;

    wprintf(sf->edit[0], "%f", vlsr);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void get_lo_str(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int n;
    double f;
    list curr = NULL;
    scanPtr new;
    DataSetPtr dsp;

    wdscanf(sf->edit[0], &f);
    sprintf(buf, "Nothing set!");
    
    dsp = new_dataset(get_listlist(), "LO set", vP->from);
    if (!dsp) {
        PostErrorDialog(w, "Out of memory when allocating dataset.");
        return;
    }

    if (vP->mode == SHOW_SPE && vP->s) {
        new = SetLOFreq(dsp, vP->s, f);
        if (!new) {
            PostErrorDialog(w, "Out of memory when allocating scan.");
            DeleteLastDataSet();
            return;
        }
        vP->to = vP->from = dsp;
        sprintf(buf, "Set LO in 1 spectrum from f=%7.3f to %7.3f.",
                vP->s->vlsr, f);
        vP->s = new;
    } else if (count_scans(vP->from) > 0) {
        n = 0;
        while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
            new = SetLOFreq(dsp, (scanPtr)DATA(curr), f);
            if (!new) break;
            n++;
        }
        if (n == 0) {
            PostErrorDialog(w, "Out of memory when allocating scan.");
            DeleteLastDataSet();
            return;
        }
        vP->to = vP->from = dsp;
        vP->s = (scanPtr)DATA(dsp->scanlist);
        sprintf(buf, "Set LO in %d spectra of %d to f=%f.",
                n, count_scans(vP->from), f);
    } else {
        PostErrorDialog(w,
            "Couldn't find any spectra to set the image frequency in.");
        return;
    }
    send_line(buf);
    sprintf(dsp->name, "Shifted LO to %g", f);

    UpdateData(SCALE_ONLY_X, REDRAW);
}

void PostImageFreqDialog(Widget wid, char *cmd, XtPointer call_data)
{
    Widget w = wid, rc;
    double flo = 0.0;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Set LO to cal. new image freqs.",
             BUTT_APPLY, (XtCallbackProc)get_lo_str, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
	     1, NULL);

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation,       XmVERTICAL,
				 NULL);
    XtCreateManagedWidget("New LO (GHz):", xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass, rc,
                                        NULL, 0);

    ArrangeStdFormDialog(sf, rc);

    if (vP->s) flo = vP->s->lofreq;

    wprintf(sf->edit[0], "%f", flo);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void DoVelShift(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int n;
    list curr = NULL;
    scanPtr new;
    DataSetPtr dsp;
    shift *s = (shift *)sf->any;

    dsp = new_dataset(get_listlist(), "Shifted", vP->from);
    if (!dsp) {
        PostErrorDialog(w, "Out of memory when allocating dataset.");
        return;
    }

    if (vP->mode == SHOW_SPE && vP->s) {
        new = VelShiftSpe(dsp, vP->s, s);
        if (!new) {
            PostErrorDialog(w, "Out of memory when allocating scan.");
            DeleteLastDataSet();
            return;
        }
        vP->from = vP->to = dsp;
        vP->s = new;
        n = 1;
        sprintf(buf, "Changed 1 spectrum.");
    } else if (count_scans(vP->from) > 0) {
        n = 0;
        while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
            new = VelShiftSpe(dsp, (scanPtr)DATA(curr), s);
            if (!new) break;
            n++;
        }
        if (n == 0) {
            PostErrorDialog(w, "Out of memory when allocating scan.");
            DeleteLastDataSet();
            return;
        }
        vP->to = vP->from = dsp;
        vP->s = (scanPtr)DATA(dsp->scanlist);
        sprintf(buf, "Changed %d spectra of %d.", n, count_scans(vP->from));
    } else {
        PostErrorDialog(w, "Couldn't find any spectra to change.");
        return;
    }

    send_line(buf);
    if (s->veltype == LSR_TO_HEL)
        sprintf(dsp->name, "LSR->HEL");
    else
        sprintf(dsp->name, "HEL->LSR");

    UpdateData(SCALE_ONLY_X, REDRAW);
}

static void VelShiftTypeCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int type = atoi(str);
    
    if (type != Shift.veltype) {
        Shift.veltype = atoi(str);
    }
}

void PostVelShiftDialog(Widget wid, char *cmd, XtPointer call_data)
{
    Widget rc, menu;
    Widget w = wid;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Change velocity scale",
             BUTT_APPLY, (XtCallbackProc)DoVelShift, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
	         0, NULL);
    sf->any = (XtPointer)(&Shift);

    rc = XtVaCreateManagedWidget("scalerc", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
                                  
    menu = CreateOptionMenu(rc, &VelShiftOptionMenu);
    SetDefaultOptionMenuItem(menu, Shift.veltype);
				                   
    ArrangeStdFormDialog(sf, rc);
   
    XtManageChild(menu);
    
    ManageDialogCenteredOnPointer(sf->form);
}
