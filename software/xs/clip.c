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
#include <Xm/PushB.h>

#include "defines.h"
#include "global_structs.h"
#include "dialogs.h"

/*** External variables ***/
extern VIEW *vP;

void   PostWarningDialog(Widget, char *);
void   PostErrorDialog(Widget, char *);
void   ManageDialogCenteredOnPointer(Widget);

int        count_scans(DataSetPtr);
list       scan_iterator(list, DataSetPtr);
scanPtr    copy_scanheader(DataSetPtr, int, scanPtr);
list      *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);
void       DeleteLastDataSet();

/*** Local variables ***/
typedef struct {
    int xunit;
    double xleft, xright;
    double ylower, yupper;
} CLIP;

static CLIP clip;

static char *Clip_Help = "\
                     Clip scan(s) help\n\
                     ---------------------\n\
In this dialog you can specify the left and right values, respectively,\n\
at which the current scan (or all scans of a dataset/map) will be clipped.\n\
If you select a range that is outside the minimum and maximum values of a\n\
scan the part outside the scan will be padded by zeros in the \"clipped\"\n\
scan.\n\
The update button can be used to set the clipping range equal to the current\n\
view.";

void init_clip_parameters()
{
    clip.xunit = UNIT_FRE;
    clip.xleft = clip.xright = 0.0;
}

static scanPtr clip_scan(scanPtr s)
{
    int n1, n2, tmp, n, m, clipY=0;
    string buf;
    scanPtr new = NULL;
    
    void send_line();
    double SpecUnitConv();
    
    vP->s = s;
    
    n1 = NINT(SpecUnitConv(UNIT_CHA, clip.xunit, clip.xleft));
    n2 = NINT(SpecUnitConv(UNIT_CHA, clip.xunit, clip.xright));
    
    if (clip.ylower <= clip.yupper) clipY = 1;
    
    if (n1 == n2) {
        sprintf(buf, "Clip: Left %d = right %d for %s", n1, n2, s->name);
        send_line(buf);
        return new;
    }
    
    if (n1 > n2) {
        tmp = n1;
        n1 = n2;
        n2 = tmp;
    }
    
    n = n2 - n1 + 1;
    new = copy_scanheader(vP->to, n, s);
    if (!new) {
        sprintf(buf, "Clip: Too many channels %d for %s", n, s->name);
        send_line(buf);
        return new;
    }
    
    m = 0;
    for (n=n1; n<=n2; n++) {
        if (n < 0 || n >= s->nChan) {
            new->d[m] = 0.0;
            new->e[m] = 0.0;
        } else {
            new->d[m] = s->d[n];
            new->e[m] = s->e[n];
	    if (clipY) {
	        if (new->d[m] < clip.ylower) new->d[m] = clip.ylower;
	        if (new->d[m] > clip.yupper) new->d[m] = clip.yupper;
	    }
        }
        m++;
    }
    
    new->vel0 = s->vel0 + (double)n1 * s->velres;
    new->freq0 = s->freq0 + (double)n1 * s->freqres;
    new->freqn = new->freq0 + (double)(n2 - n1) * s->freqres;
    
    return new;
}

static void PushButtCallback(Widget w, StdForm *sf,
                               XmPushButtonCallbackStruct *cd)
{
    VIEW *sv;
    
    void wprintf();
    VIEW *GetScanView();
    
    clip.xunit  = vP->xunit;
    clip.xleft  = vP->xleft;
    clip.xright = vP->xright;
    clip.ylower  = vP->ylower;
    clip.yupper = vP->yupper;
    if (vP->mode == SHOW_ALLSPE && (sv = GetScanView())) {
        clip.xunit  = sv->xunit;
        clip.xleft  = sv->xleft;
        clip.xright = sv->xright;
        clip.yupper = sv->ylower;
        clip.ylower = sv->yupper;
    }

    if (sf) {
        wprintf(sf->edit[0], "%f", clip.xleft);
        wprintf(sf->edit[1], "%f", clip.xright);
        wprintf(sf->edit[2], "%f", clip.ylower);
        wprintf(sf->edit[3], "%f", clip.yupper);
    }
}

static void do_clip(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    string buf;
    list curr = NULL;
    int err = 0;
    scanPtr new;
    DataSetPtr dsp;

    void send_line(), wdscanf(), UpdateData(), SetWatchCursor();

    if (!sf) return;

    wdscanf(sf->edit[0], &clip.xleft);
    wdscanf(sf->edit[1], &clip.xright);
    wdscanf(sf->edit[2], &clip.ylower);
    wdscanf(sf->edit[3], &clip.yupper);

    SetWatchCursor(True);
    
    dsp = new_dataset(get_listlist(), "Clipped scans", vP->from);
    if (!dsp) {
        PostErrorDialog(w, "Out of memory when allocating new dataset.");
        return;
    }
    vP->to = dsp;
    
    while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
        new = clip_scan((scanPtr)DATA(curr));
        if (!new)
            err += 1;
    }
    
    SetWatchCursor(False);
    
    sprintf(dsp->name, "Clipped %s", vP->from->name);
    
    vP->from = vP->to;
    vP->s = (scanPtr)DATA(dsp->scanlist);
    
    
    if (err)
        PostWarningDialog(w,
"One or more scans may not have been clipped (or padded) properly.\n\
Check the message log for more info.");
    else {
        if (clip.ylower > clip.yupper) {
            sprintf(buf, "All scans clipped in X: [%f, %f].",
	            clip.xleft, clip.xright);
	} else {
            sprintf(buf, "All scans clipped: X: [%f, %f] and Y: [%f, %f].",
	            clip.xleft, clip.xright, clip.ylower, clip.yupper);
	}  
        send_line(buf);
    }
    
    if (err < count_scans(vP->from))
        UpdateData(SCALE_ONLY_X, REDRAW);
}

void DoClipScans(Widget wid, char *cmd, XtPointer call_data)
{
    static int first = 1;
    string str, unit;
    Widget rc, w = wid, rc2, butt;
    StdForm *sf;

    void wprintf();
    char *GetUnitString();

    while (!XtIsWMShell(w))
        w = XtParent(w);

    if (first) {
        PushButtCallback(NULL, NULL, NULL);
        first = 0;
    }

    sprintf(unit, "[%s]", GetUnitString(clip.xunit));

    sf = PostStdFormDialog(w, "Clip scans",
             BUTT_APPLY, (XtCallbackProc)do_clip, NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, (XtPointer)Clip_Help,
             4, NULL);

    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          NULL);

    strcpy(str, "Begin at ");
    strcat(str, unit);
    XtVaCreateManagedWidget(str, xmLabelWidgetClass, rc, NULL);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);
    strcpy(str, "End at ");
    strcat(str, unit);
    XtVaCreateManagedWidget(str, xmLabelWidgetClass, rc, NULL);
    sf->edit[1] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);
    strcpy(str, "Below");
    XtVaCreateManagedWidget(str, xmLabelWidgetClass, rc, NULL);
    sf->edit[2] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);
    strcpy(str, "Above");
    XtVaCreateManagedWidget(str, xmLabelWidgetClass, rc, NULL);
    sf->edit[3] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);
    rc2 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                  XmNorientation, XmHORIZONTAL,
                                  NULL);
    XtVaCreateManagedWidget("Use current view:", xmLabelWidgetClass, rc2, NULL);
    butt = XtCreateManagedWidget("Update", xmPushButtonWidgetClass,
                                 rc2, NULL, 0);
    XtAddCallback(butt, XmNactivateCallback,
                  (XtCallbackProc)PushButtCallback, sf);
    
    ArrangeStdFormDialog(sf, rc);

    wprintf(sf->edit[0], "%f", clip.xleft);
    wprintf(sf->edit[1], "%f", clip.xright);
    wprintf(sf->edit[2], "%f", clip.ylower);
    wprintf(sf->edit[3], "%f", clip.yupper);
    
    XtManageChild(rc);
    
    ManageDialogCenteredOnPointer(sf->form);
}
