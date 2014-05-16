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
#include <string.h>
#include <math.h>

#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/Frame.h>

#include "defines.h"
#include "global_structs.h"
#include "dialogs.h"

/*** External variables ***/
extern VIEW *vP;

void       ManageDialogCenteredOnPointer(Widget);
int        count_scans(DataSetPtr);
list       scan_iterator(list, DataSetPtr);
scanPtr    copy_scanheader(DataSetPtr, int, scanPtr);
list      *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);
void       DeleteLastDataSet();

/*** Local parameters ***/

#define FOLD_POSNEG 0
#define FOLD_NEGPOS 1

int     fold_type;
double  fold_throw, fold_offset;
char   *fold_str[] = {"Pos-Neg", "Neg-Pos"};

void invert_scans(Widget w, char *cmd, XtPointer call_data)
{
    int i, j;
    double tmp;
    string buf;
    list curr = NULL;
    scanPtr s;
    VIEW *sv;

    void UpdateData(), draw_main(), invert_gaussar(), send_line();
    void invert_boxar_and_regs(), SetWindow();
    VIEW *GetScanView();

    while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
        s = (scanPtr)DATA(curr);
        i = 0;
        j = s->nChan - 1;
        while (i < j) {
            tmp = s->d[i]; s->d[i] = s->d[j]; s->d[j] = tmp;
            tmp = s->e[i]; s->e[i] = s->e[j]; s->e[j] = tmp;
            i++;
            j--;
        }
        tmp = s->freqn;
        s->freqn = s->freq0;
        s->freq0 = tmp;
        s->freqres *= -1.0;

        s->vel0 += (double)(s->nChan - 1)* s->velres;
        s->velres *= -1.0;
        s->saved = 0;
    }
    
    invert_boxar_and_regs(vP->s);
    invert_gaussar();
    
    UpdateData(SCALE_NONE, NO_REDRAW);
    
    if (vP->mode == SHOW_SPE) {
        SetWindow(vP->xright, vP->xleft, vP->ylower, vP->yupper);
        draw_main();
    } else if (vP->mode == SHOW_ALLSPE) {
        sv = GetScanView();
        if (sv) {
            tmp = sv->xleft; sv->xleft = sv->xright; sv->xright = tmp;
        }
        draw_main();
    }
    
    sprintf(buf, "Inverted %d spectra.", count_scans(vP->from));
    send_line(buf);
}

void init_fold_parameters()
{
    fold_offset = 0.0;
    fold_throw = 0.0;
    fold_type = FOLD_POSNEG;
}

static void ToggleFoldType(Widget w, char *str, XtPointer call_data)
{
    int i;
    
    for (i=0; i<2; i++) {
        if (strcmp(str, fold_str[i]) == 0) {
            fold_type = i;
            return;
        }
    }
}

static scanPtr FoldScan(DataSetPtr d, scanPtr s,
                        double fthrow, double foffset, int type)
{
    int n, dn;
    double s1, s2, r1, r2;
    scanPtr new = NULL;

    dn = NINT(fabs(fthrow/s->freqres/2.0));
    
    n = s->nChan - 2*dn;
    if (n <= 0) return new;
    
    new = copy_scanheader(d, s->nChan - 2*dn, s);
    if (!new) return new;
    
    for (n=dn; n<s->nChan - dn; n++) {
        s1 = s->d[n - dn];
        s2 = s->d[n + dn];
        r1 = s->e[n - dn];
        r2 = s->e[n + dn];
        if (type == FOLD_POSNEG) {
            new->d[n - dn] = (s1 - s2)/2.0;
            if (new->fft)
                new->e[n - dn] = (r1 - r2)/2.0;
            else if (r1 != 0.0 && r2 != 0.0)
                new->e[n - dn] = 1.0/sqrt(4.0/r1/r1 + 4.0/r2/r2);
        } else {
            new->d[n - dn] = (s2 - s1)/2.0;
            if (new->fft)
                new->e[n - dn] = (r2 - r1)/2.0;
            else if (r1 != 0.0 && r2 != 0.0)
                new->e[n - dn] = 1.0/sqrt(4.0/r1/r1 + 4.0/r2/r2);
        }
    }
    
    new->freq0  += foffset + fabs(fthrow)/2.0;
    new->freqn   = new->freq0 + (double)(new->nChan - 1)*new->freqres;
    
    new->vel0 += new->velres*(foffset + fabs(fthrow)/2.0)/new->freqres;
    
    return new;
}

static int FoldAllScans(DataSetPtr d)
{
    int n = 0;
    list curr = NULL;
    scanPtr s, f;
    
    while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
        s = (scanPtr)DATA(curr);
        f = FoldScan(d, s, fold_throw/1000.0, fold_offset/1000.0, fold_type);
        if (!f) break;
        n++;
    }
    
    return n;
}

void DoFoldAllScans(Widget w, StdForm *sf, XtPointer cd)
{
    int n=0, m = vP->mode;
    string buf;
    scanPtr f = NULL;
    DataSetPtr d;
    
    void wdscanf(), UpdateData(), send_line();
    
    if (sf) {
        wdscanf(sf->edit[0], &fold_throw);
        wdscanf(sf->edit[1], &fold_offset);
    }
    
    sprintf(buf, "Folded %s by %.1f MHz", vP->from->name, fold_throw);
    
    d = new_dataset(get_listlist(), buf, vP->from);
    if (!d) {
        send_line("Out of memory when allocating data set.");
        return;
    }
    
    if (m == SHOW_SPE) {
        f = FoldScan(d, vP->s, fold_throw/1000.0, fold_offset/1000.0, fold_type);
        if (f) {
            vP->from = vP->to = d;
            vP->s = f;
            sprintf(buf, "Folded scan with freq. throw=%f MHz.", fold_throw);
            send_line(buf);
        } else
            DeleteLastDataSet();
    } else {
        n = FoldAllScans(d);
        sprintf(buf, "Folded %d scans with freq. throw=%f MHz.", n, fold_throw);
        send_line(buf);
        if (n > 0) {
            vP->from = vP->to = d;
            vP->s = (scanPtr)DATA(d->scanlist);
        } else
            DeleteLastDataSet();
    }
    
    if (sf && (f || n > 0)) UpdateData(SCALE_BOTH, REDRAW);
    if (sf) XtDestroyWidget(sf->form);
}

void FoldSelect(Widget wid, char *cmd, XtPointer call_data)
{
    int n;
    Arg wargs[10];
    Widget w = wid, rc, fr, rc2, rb, butt;
    StdForm *sf;
    
    void wprintf();

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Fold FSW spectra",
             BUTT_APPLY, (XtCallbackProc)DoFoldAllScans, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 2, NULL);

    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          NULL);

    XtCreateManagedWidget("Order of pos. and neg. peaks:", xmLabelWidgetClass,
                          rc, NULL, 0);
    n = 0;
    XtSetArg(wargs[n], XmNentryClass, xmToggleButtonWidgetClass); n++;
    rb = XmCreateRadioBox(rc, "radiobox", wargs, n);
    for (n=0; n<2; n++) {
        butt = XtVaCreateManagedWidget(fold_str[n], xmToggleButtonWidgetClass,
                                 rb, XmNset, (fold_type == n) ? True : False,
                                 NULL);
        XtAddCallback(butt, XmNarmCallback,
                      (XtCallbackProc)ToggleFoldType, fold_str[n]);
    }
  
    fr  = XtVaCreateWidget("frame", xmFrameWidgetClass,
				           rc, XmNshadowType, XmSHADOW_OUT, NULL);
    rc2 = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, fr,
                           XmNorientation, XmVERTICAL,
                           NULL);
    XtCreateManagedWidget("Frequency throw [MHz]", xmLabelWidgetClass,
                          rc2, NULL, 0);
    sf->edit[0]  = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                         rc2, NULL, 0);
    XtCreateManagedWidget("Frequency offset [MHz]", xmLabelWidgetClass,
                          rc2, NULL, 0);
    sf->edit[1]  = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                         rc2, NULL, 0);
    
    ArrangeStdFormDialog(sf, rc);
    
    XtManageChild(rb);
    XtManageChild(rc2);
    XtManageChild(fr);
    XtManageChild(rc);
    
    wprintf(sf->edit[0], "%f", fold_throw);
    wprintf(sf->edit[1], "%f", fold_offset);
    
    ManageDialogCenteredOnPointer(sf->form);
}
