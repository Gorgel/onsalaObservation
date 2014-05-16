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

#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Frame.h>
#include <Xm/ToggleB.h>

#include "defines.h"
#include "global_structs.h"
#include "dialogs.h"

/*** External variables ***/
extern VIEW  *vP;

void ManageDialogCenteredOnPointer(Widget);
void PostErrorDialog(Widget, char *);

list       scan_iterator(list, DataSetPtr);
scanPtr    copy_scanheader(DataSetPtr, int, scanPtr);
list      *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);
void       DeleteLastDataSet();

/*** Local variables ***/
typedef struct {
    double k, m;
    int beam_eff;
} LINE;

static LINE scale;

void init_scale_parameters()
{
    scale.k = 1.0;
    scale.m = 0.0;
    scale.beam_eff = 0;
}

static scanPtr scale_spectrum(DataSetPtr d, scanPtr s, LINE *L)
{
    int n;
    scanPtr new = NULL;
    
    new = copy_scanheader(d, s->nChan, s);
    if (!new) return new;
    
    for (n=0; n<new->nChan; n++) {
        if (L->beam_eff) {
            if (s->beameff > 0.0 && s->beameff <= 1.0) {
                new->d[n] = s->d[n]/s->beameff;
                new->e[n] = s->e[n]/s->beameff;
            }
        } else {
            new->d[n] = L->k * s->d[n] + L->m;
            if (new->fft)
                new->e[n] = L->k * s->e[n] + L->m;
            else
                new->e[n] = L->k * s->e[n];
        }
    }
    
    return new;
}

static void get_scale_strs(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int n = 0;
    string buf, name;
    list curr = NULL;
    scanPtr s, new;
    DataSetPtr dsp;

    void UpdateData(), send_line(), wdscanf();

    wdscanf(sf->edit[0], &scale.k);
    wdscanf(sf->edit[1], &scale.m);

    if (scale.beam_eff) {
        sprintf(buf, "Dividing all spectra with the main beam efficiency.");
        sprintf(name, "Scaled (1/eta_mb)");
    } else {
        if (scale.m >= 0.0) {
            if (scale.m == 0.0) {
                sprintf(buf, "Scaling all spectra with y' = %g*y",
                        scale.k);
                sprintf(name, "Scaled (%g*y)", scale.k);
            } else {
                sprintf(buf, "Scaling all spectra with y' = %g*y + %g",
                        scale.k, scale.m);
                sprintf(name, "Scaled (%g*y+%g)", scale.k, scale.m);
            }
        } else {
            sprintf(buf, "Scaling all spectra with y' = %g*y - %g",
                    scale.k, -scale.m);
            sprintf(name, "Scaled (%g*y-%g)", scale.k, -scale.m);
        }
    }
    send_line(buf);
    
    dsp = new_dataset(get_listlist(), name, vP->from);
    if (!dsp) return;

    if (vP->mode == SHOW_SPE) {
        new = scale_spectrum(dsp, vP->s, &scale);
        if (!new) {
            PostErrorDialog(w, "Out of memory when allocating new scan.");
            DeleteLastDataSet();
        } else {
            vP->to = vP->from = dsp;
            vP->s = new;
            UpdateData(SCALE_ONLY_Y, REDRAW);
        }
        return;
    }
    
    n = 0;
    while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
        s = (scanPtr)DATA(curr);
        new = scale_spectrum(dsp, s, &scale);
        if (!new) break;
        n++;
    }
    
    if (n == 0) {
        PostErrorDialog(w, "Out of memory when allocating new scan.");
        DeleteLastDataSet();
        return;
    } else {
        vP->to = vP->from = dsp;
        vP->s = (scanPtr)DATA(dsp->scanlist);
    }

    UpdateData(SCALE_ONLY_Y, REDRAW);
}

static void set_beff_active(Widget w, char *str,
                            XmToggleButtonCallbackStruct *cb)
{
    if (cb->set) {
        scale.beam_eff = 1;
    } else {
        scale.beam_eff = 0;
    }
}

void scale_scans(Widget wid, char *cmd, XtPointer call_data)
{
    int n;
    Widget rc, rcV, fr, beffButt;
    Widget w = wid;
    Arg wargs[10];
    StdForm *sf;

    void wprintf();

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Scale spectra",
             BUTT_APPLY, (XtCallbackProc)get_scale_strs, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             2, NULL);

    rcV = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                  XmNorientation,       XmVERTICAL,
                                  NULL);

    fr = XtVaCreateWidget("frame", xmFrameWidgetClass, rcV,
				          XmNshadowType, XmSHADOW_OUT, NULL);
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                 XmNorientation,       XmHORIZONTAL,
                                 NULL);
    XtCreateManagedWidget("y = ", xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);
    XtCreateManagedWidget("x + ", xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[1] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);
    
    n = 0;
    if (scale.beam_eff == 1) {
        XtSetArg(wargs[n], XmNset, True); n++;
    }
    beffButt = XtCreateWidget("Divide by main beam efficiency",
                              xmToggleButtonWidgetClass,
                              rcV, wargs, n);
    XtAddCallback(beffButt, XmNvalueChangedCallback,
                  (XtCallbackProc)set_beff_active, NULL);
    ArrangeStdFormDialog(sf, fr);

    wprintf(sf->edit[0], "%f", scale.k);
    wprintf(sf->edit[1], "%f", scale.m);
    
    XtManageChild(beffButt);
    XtManageChild(fr);
    
    ManageDialogCenteredOnPointer(sf->form);
}
