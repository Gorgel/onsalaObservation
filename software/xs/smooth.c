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
extern VIEW   *vP;

void   PostErrorDialog(Widget, char *);
void   ManageDialogCenteredOnPointer(Widget);
Widget PostWaitingDialog(Widget, char *, Widget *, int);
void   SetWaitingScale(Widget, int);
Widget CreateOptionMenu(Widget, MenuBarItem *);
void   SetDefaultOptionMenuItem(Widget, int);

list       scan_iterator(list, DataSetPtr);
int        count_scans(DataSetPtr);
scanPtr    copy_scanheader(DataSetPtr, int, scanPtr);
list      *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);
void       DeleteLastDataSet();

/*** Local variables ***/
static int     smooth_type, smooth_use_rms;
static double  smooth_width;

#define SMOOTH_BOXCAR  0
#define SMOOTH_HANNING 1
#define SMOOTH_GAUSS   2

static void SetSmoothTypeCallback();
static MenuItem SmoothTypeData[] = {
   {"Boxcar", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetSmoothTypeCallback, "0", NULL},
   {"Hanning", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetSmoothTypeCallback, "1", NULL},
   {"Gaussian", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetSmoothTypeCallback, "2", NULL},
EOI};
static MenuBarItem SmoothTypeMenu = {
   "Type of smoothing", ' ', True, SmoothTypeData
};

/* static void set_rms_usage(Widget, Widget, XmAnyCallbackStruct *);
static scanPtr do_boxcar_smoothing(int, DataSetPtr, scanPtr);
static scanPtr do_hanning_smoothing(int, DataSetPtr, scanPtr);
static scanPtr do_gaussian_smoothing(double, DataSetPtr, scanPtr); */

static char *Smoothing_Help = "\
                          Smoothing help\n\
                          --------------\n\
In this dialog you can specify the type of smoothing as well as the smoothing\n\
width (in channels). The options are:\n\
    Boxcar    Will take the average over the size of each box of channels. Each\n\
              box of channels (2,3,4,...) will then be one channel of data in\n\
              the smoothed spectrum. The very last box may not consist of the\n\
              specified number of channels, rather the remainder, if the width\n\
              is not divisble by the total number of channels.\n\
    Hanning   Here the smoothing width must be an odd number (3,5,7,...) of\n\
              channels. A triangular weighting function is applied to the data.\n\
              The total number of channels will remain the same.\n\
    Gaussian  Similar to Hanning-smoothing but using a Gaussian. In this case\n\
              the smoothing width correponds to the FWHM channel width of the\n\
              Gaussian. The width does not have to be an integer, but must be\n\
              larger than the original channel width.\n\n\
There is also an option to use the channel RMS in the weighting. This is only\n\
useful for spectra with varying RMS over the channels.\n\
";

void init_smooth_parameters()
{
    smooth_type = SMOOTH_BOXCAR;
    smooth_width = 3.0;
    smooth_use_rms = 0;
}

static void SetSmoothTypeCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    int n = atoi(s);
    
    if (n != smooth_type) smooth_type = n;
}

static void set_rms_usage(Widget w, Widget butt, XmAnyCallbackStruct *cb)
{
    if (smooth_use_rms)
        smooth_use_rms = 0;
    else
        smooth_use_rms = 1;
}

static scanPtr do_boxcar_smoothing(int width, DataSetPtr d, scanPtr sp)
{
    int i, j, e_width;
    double y, z, beg;
    string buf;
    scanPtr new;

    double SpecUnitConv();
    void send_line();

    if (width <= 1) {
        sprintf(buf,
          "Width must be 2,3,4,... for boxcar smoothing. You used %d.", width);
        PostErrorDialog(NULL, buf);
        return NULL;
    }
    
    j = 0;
    e_width = width;
    for (i=0; i<sp->nChan; i++) {
        if (i == sp->nChan - 1) {
            e_width = width - ((i + 1) % width);
        } else if ((i + 1) % width == 0) {
            e_width = width;
        } else {
            e_width = 0;
        }
        if (e_width) j++;
    }

    new = copy_scanheader(d, j, sp);
    if (!new) return new;
    
    j = 0;
    y = z = 0.0;
    e_width = width;
    for (i=0; i<sp->nChan; i++) {
        if (smooth_use_rms && sp->e[i] != 0.0 && !sp->fft)
            y += sp->d[i]/sp->e[i]/sp->e[i];
        else
            y += sp->d[i];
        if (sp->fft)
            z += sp->e[i];
        else if (sp->e[i] != 0.0)
            z += 1.0/sp->e[i]/sp->e[i];

        if (i == sp->nChan - 1) {
            e_width = width - ((i + 1) % width);
        } else if ((i + 1) % width == 0) {
            e_width = width;
        } else {
            e_width = 0;
        }

        if (e_width) {
            if (smooth_use_rms && sp->e[i] != 0.0 && !sp->fft)
                new->d[j] = y/(double)e_width/z;
            else
                new->d[j] = y/(double)e_width;
            if (sp->fft)
                new->e[j] = z/(double)e_width;
            else if (z != 0.0)
                new->e[j] = 1.0/sqrt(z*(double)e_width);
            else
                new->e[j] = UNDEF;
            y = z = 0.0; 
            j++;
        }
    }
    beg = (double)(width-1)/2.0;
    new->freqres *= (double)width;
    new->velres *= (double)width;
    new->freq0 = sp->freq0 + beg * sp->freqres;
    new->freqn = new->freq0 + (double)(new->nChan-1)*new->freqres;
    new->vel0  = sp->vel0 + beg * sp->velres;
    vP->s = new;
    
    return new;
}

static double hanning_weight(int n, int width)
{
   int m = (width + 1)/2;

   if (n > m || n < -m) return 0.0;

   return (double)(m + 1 - abs(n));
}

static double gauss_weight(int n, double width)
{
   double gauss();

   return gauss((double)n, 0.0, 1.0, width);
}

static scanPtr do_hanning_smoothing(int width, DataSetPtr d, scanPtr sp)
{
    int i, j, k, span = (width + 1)/2;
    double y, z, w, wsum;
    string buf;
    scanPtr new = NULL;

    void send_line();

    if (width <= 1) {
        sprintf(buf,
                "Width must be 3,5,7,... for Hanning smoothing. You used %d.",
                width);
        PostErrorDialog(NULL, buf);
        return NULL;
    }

    if (width % 2 == 0) {
        sprintf(buf,
                "Width must be 3,5,7,... for Hanning smoothing. You used %d.",
                width);
        PostErrorDialog(NULL, buf);
        return NULL;
    }

    new = copy_scanheader(d, sp->nChan, sp);
    if (!new) return new;
    for (i=0; i<sp->nChan; i++) {
        y = z = 0.0;
        wsum = 0.0;
        for (j=-span; j<=span; j++) {
            k = i + j;
            if (k < 0 || k >= sp->nChan) continue;
            w = hanning_weight(j, width);
            if (smooth_use_rms && sp->e[k] != 0.0 && !sp->fft)
                w /= (sp->e[k]*sp->e[k]);
            wsum += w;
            y += w*sp->d[k];
            if (sp->fft)
                z += w*sp->e[k];
            else if (sp->e[k] != 0.0)
                z += w/sp->e[k]/sp->e[k];
        }
        if (wsum == 0.0) {
            new->d[i] = UNDEF;
            new->e[i] = UNDEF;
        } else {
            new->d[i] = y/wsum;
            if (sp->fft)
                new->e[i] = z/wsum;
            else if (z != 0.0)
                new->e[i] = sqrt(wsum/z);
            else
                new->e[i] = UNDEF;
        }
    }
    
    return new;
}

static scanPtr do_gaussian_smoothing(double width, DataSetPtr d, scanPtr sp)
{
    int i, j, k, span;
    double y, z, w, wsum;
    string buf;
    scanPtr new = NULL;

    if (width <= 1.0) {
        sprintf(buf,
                "Width must be > 1.0 for Gaussian smoothing. You used %f.",
                width);
        PostErrorDialog(NULL, buf);
        return NULL;
    }

    span = NINT(2.0*width);

    new = copy_scanheader(d, sp->nChan, sp);
    if (!new) return new;
    
    for (i=0; i<sp->nChan; i++) {
        y = z = 0.0;
        wsum = 0.0;
        for (j=-span; j<=span; j++) {
            k = i + j;
            if (k < 0 || k >= sp->nChan) continue;
            w = gauss_weight(j, width);
            if (smooth_use_rms && sp->e[k] != 0.0 && !sp->fft)
                w /= (sp->e[k]*sp->e[k]);
            wsum += w;
            y += w*sp->d[k];
            if (sp->fft)
                z += w*sp->e[k];
            else if (sp->e[k] != 0.0)
                z += w/sp->e[k]/sp->e[k];
        }
        if (wsum == 0.0) {
            new->d[i] = UNDEF;
            new->e[i] = UNDEF;
        } else {
            new->d[i] = y/wsum;
            if (sp->fft)
                new->e[i] = z/wsum;
            else if (z != 0.0)
                new->e[i] = sqrt(wsum/z);
            else
                new->e[i] = UNDEF;
        }
    }
    
    return new;
}

static void do_smoothing(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int n=0;
    string buf;
    list curr = NULL;
    scanPtr old, new=NULL;
    DataSetPtr d, dold;
    Widget wait=NULL, scale;

    void wdscanf(), UpdateData(), SetWatchCursor(), send_line();

    wdscanf(sf->edit[0], &smooth_width);
    
    if (vP->mode != SHOW_SPE) n = count_scans(vP->from);
    
    d = new_dataset(get_listlist(), "Smoothed", vP->from);
    if (!d) {
        PostErrorDialog(w, "Out of memory when allocating new dataset.");
        return;
    }
    dold = vP->from;
    
    SetWatchCursor(True);

    switch (smooth_type)
    {
        case SMOOTH_BOXCAR:
            sprintf(buf, "Doing boxcar smoothing over %d channels.\n",
                    NINT(smooth_width));
            if (vP->mode == SHOW_SPE) {
                new = do_boxcar_smoothing(NINT(smooth_width), d, vP->s);
                if (!new) {
                    PostErrorDialog(w,
                      "Out of memory when allocating new scan.");
                    DeleteLastDataSet();
                    if (wait) XtDestroyWidget(wait);
                    SetWatchCursor(False);
                    return;
                }
                vP->to = vP->from = d;
                vP->s = new;
            } else {
                if (n > WAITSPECTRA)
                    wait = PostWaitingDialog(NULL, "Smoothing (Boxcar) spectra...",
                                             &scale, n);
                n = 0;
                while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
                    if (wait) SetWaitingScale(scale, n+1);
                    old = (scanPtr)DATA(curr);
                    new = do_boxcar_smoothing(NINT(smooth_width), d, old);
                    if (!new) break;
                    if (n == 0) {
                        sprintf(buf, "Smoothing from %d channels to %d.\n",
                                old->nChan, new->nChan);
                        send_line(buf);
                    }
                    n++;
                }
                if (n == 0) {
                    PostErrorDialog(w,
                      "Out of memory when allocating new scan.");
                    DeleteLastDataSet();
                    if (wait) XtDestroyWidget(wait);
                    SetWatchCursor(False);
                    return;
                }
                vP->to = vP->from = d;
                vP->s = (scanPtr)DATA(d->scanlist);
            }
            sprintf(d->name, "Smoothed %s (Boxcar %d)",
	            dold->name, NINT(smooth_width));
            break;
        case SMOOTH_HANNING:
            sprintf(buf, "Doing Hanning smoothing over %d channels.\n",
                    NINT(smooth_width));
            if (vP->mode == SHOW_SPE) {
                new = do_hanning_smoothing(NINT(smooth_width), d, vP->s);
                if (!new) {
                    PostErrorDialog(w,
                      "Out of memory when allocating new scan.");
                    DeleteLastDataSet();
                    if (wait) XtDestroyWidget(wait);
                    SetWatchCursor(False);
                    return;
                }
                vP->to = vP->from = d;
                vP->s = new;
            } else {
                if (n > WAITSPECTRA)
                    wait = PostWaitingDialog(NULL, "Smoothing (Hanning) spectra...",
                                             &scale, n);
                n = 0;
                while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
                    if (wait) SetWaitingScale(scale, n+1);
                    old = (scanPtr)DATA(curr);
                    new = do_hanning_smoothing(NINT(smooth_width), d, old);
                    if (!new) break;
                    n++;
                }
                if (n == 0) {
                    PostErrorDialog(w,
                      "Out of memory when allocating new scans.");
                    DeleteLastDataSet();
                    if (wait) XtDestroyWidget(wait);
                    SetWatchCursor(False);
                    return;
                }
                vP->to = vP->from = d;
                vP->s = (scanPtr)DATA(d->scanlist);
            }
            sprintf(d->name, "Smoothed %s (Hanning %d)",
	            dold->name, NINT(smooth_width));
            break;
        case SMOOTH_GAUSS:
            sprintf(buf, "Doing Gaussian smoothing with %f channels.\n",
                    smooth_width);
            if (vP->mode == SHOW_SPE) {
                new = do_gaussian_smoothing(NINT(smooth_width), d, vP->s);
                if (!new) {
                    PostErrorDialog(w,
                      "Out of memory when allocating new scans.");
                    DeleteLastDataSet();
                    if (wait) XtDestroyWidget(wait);
                    SetWatchCursor(False);
                    return;
                }
                vP->to = vP->from = d;
                vP->s = new;
            } else {
                if (n > WAITSPECTRA)
                    wait = PostWaitingDialog(NULL,
                             "Smoothing (Gaussian) spectra...", &scale, n);
                n = 0;
                while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
                    if (wait) SetWaitingScale(scale, n+1);
                    old = (scanPtr)DATA(curr);
                    new = do_gaussian_smoothing(smooth_width, d, old);
                    if (!new) break;
                    n++;
                }
                if (n == 0) {
                    PostErrorDialog(w,
                      "Out of memory when allocating new scans.");
                    DeleteLastDataSet();
                    if (wait) XtDestroyWidget(wait);
                    SetWatchCursor(False);
                    return;
                }
                vP->to = vP->from = d;
                vP->s = (scanPtr)DATA(d->scanlist);
            }
            sprintf(d->name, "Smoothed %s (Gaussian %g)",
	            dold->name, smooth_width);
            break;
    }
    
    if (wait) XtDestroyWidget(wait);
    SetWatchCursor(False);
    
    send_line(buf);
    
    if (smooth_type == SMOOTH_BOXCAR)
        UpdateData(SCALE_ONLY_X, REDRAW);
    else 
        UpdateData(SCALE_NONE, REDRAW);
}

void smooth_scans(Widget wid, char *cmd, XtPointer call_data)
{
    Widget rc, menu, w = wid, rms_butt;
    StdForm *sf;

    void wprintf();

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Smoothing spectra",
             BUTT_APPLY, (XtCallbackProc)do_smoothing, NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, Smoothing_Help,
	     1, NULL);

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
			         NULL);

    menu = CreateOptionMenu(rc, &SmoothTypeMenu);
    SetDefaultOptionMenuItem(menu, smooth_type);

    rms_butt = XtVaCreateManagedWidget("RMS usage?", xmToggleButtonWidgetClass,
                                rc, XmNset, smooth_use_rms ? True : False,
                                NULL);
    XtAddCallback(rms_butt, XmNvalueChangedCallback,
                  (XtCallbackProc)set_rms_usage, rms_butt);

    XtCreateManagedWidget("Width in channels:", xmLabelWidgetClass,
                          rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(menu);

    wprintf(sf->edit[0], "%f", smooth_width);
    
    ManageDialogCenteredOnPointer(sf->form);
}
