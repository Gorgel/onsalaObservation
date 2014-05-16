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

#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Frame.h>

#include "defines.h"
#include "global_structs.h"
#include "menus.h"
#include "dialogs.h"

#ifdef HAVE_LIBPGPLOT
#include "cpgplot.h"
#endif

/*** External variables ***/
extern BOX     boxar[MAXBOX], regs[MAXBOX];
extern PSDATA  ps;
extern int     nbox, nreg, pgplot;
extern VIEW   *vP;
extern GLOBAL *gp;
extern USER   *pP;

void   PostErrorDialog(Widget, char *);
Widget PostWaitingDialog(Widget, char *, Widget *, int);
void   SetWaitingScale(Widget, int);
void   ManageDialogCenteredOnPointer(Widget);
void   SetPGStyle(PSSTY *);
Widget CreateOptionMenu(Widget, MenuBarItem *);
void   SetDefaultOptionMenuItem(Widget, int);

int        count_scans(DataSetPtr);
list       scan_iterator(list, DataSetPtr);
scanPtr    copy_scan(DataSetPtr, scanPtr);
list      *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);

double  *AllocDoubleVector(int);
void     FreeDoubleVector(double *);
double **AllocDoubleArray(int, int);
void     FreeDoubleArray(double **, int, int);
int     *AllocIntVector(int);

/*** Local variables ***/
BLINE           bl;
static double  *bl_parameters, *bl_errors, **bl_alpha, **bl_covar;
static int     *bl_lista, noblchan = 0;
static double  *xblval, *yblval, *zblval;

#define POLTYPE_STD  0
#define POLTYPE_CHE  1
#define POLTYPE_LIN  2
#define POLTYPE_SIN  3
#define POLTYPE_SINC 4

static char *PolType[] = {"Std polynomial",
"Chebyshev pol.", "Std pol.", "Pol. + sin()", "Pol. + sinc()"};

static void SetBlPolTypeCallback();
static MenuItem BlPolTypeData[] = {
   {"Std polynomial", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolTypeCallback, "0", NULL},
   {"Chebyshev polynomial", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolTypeCallback, "1", NULL},
   {"Std pol. (linear fit)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolTypeCallback, "2", NULL},
   {"Std pol. + sin()", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolTypeCallback, "3", NULL},
   {"Std pol. + sinc()", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolTypeCallback, "4", NULL},
EOI};
static MenuBarItem BlPolTypeMenu = {
   "Type of function", ' ', True, BlPolTypeData
};

static void SetBlPolOrderCallback();
static MenuItem BlPolOrderData[] = {
   {"0", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "0", NULL},
   {"1", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "1", NULL},
   {"2", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "2", NULL},
   {"3", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "3", NULL},
   {"4", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "4", NULL},
   {"5", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "5", NULL},
   {"6", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "6", NULL},
   {"7", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "7", NULL},
   {"8", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "8", NULL},
   {"9", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "9", NULL},
   {"10", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "10", NULL},
   {"11", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "11", NULL},
   {"12", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "12", NULL},
   {"13", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "13", NULL},
   {"14", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetBlPolOrderCallback, "14", NULL},
EOI};
static MenuBarItem BlPolOrderMenu = {
   "Polynomial order", ' ', True, BlPolOrderData
};

static char *BlPolType_Help = "\
                     Baseline fitting help\n\
                     ---------------------\n\
In this dialog you can specify the type of function used to fit a baseline. The\n\
options are:\n\
    Std polynomial    Normal polynomial (order <= 15). Non-linear fit.\n\
    Chenbyshev pol.   A Chebyshev polynomial. Non-linear fit.\n\
    Std pol.          A normal polynomial but using a linear fit (faster than\n\
                      the non-linear fit.\n\
    Std Pol + sin()   A normal polynomial + a sinus function (amplitude, period\n\
                      and phase). Non-linear fit. This option is useful when\n\
                      removing standing-wave patterns.\n\n\
    Std Pol + sinc()  A normal polynomial + a sinc function (amplitude, offset\n\
                      and width). Non-linear fit. This option is useful when\n\
                      removing interference patterns in the ODIN AOS spectra.\n\
                      The sinc(x)=sinx/x used is defined as\n\
                            A sin((x-o)/w) / (x-o)/w\n\
                      where x goes from -1 to +1 over the entire spectrum.\n\n\
The polynomial order can also be given here.\n\
";

void init_baseline_parameters()
{  
    bl_alpha      = AllocDoubleArray(MAXORDER+4, MAXORDER+4);
    bl_covar      = AllocDoubleArray(MAXORDER+4, MAXORDER+4);
    bl_parameters = AllocDoubleVector(MAXORDER+4);
    bl_errors     = AllocDoubleVector(MAXORDER+4);
    bl_lista      = AllocIntVector(MAXORDER+4);
    
    bl.norder = atoi(pP->polOrder);
    bl.pol_type = POLTYPE_STD;
}

static int ret_llbf(double **a, double *b, int size, int ret_val)
{
    if (a) FreeDoubleArray(a, size, size);
    if (b) FreeDoubleVector(b);
    
    a = NULL;
    b = NULL;
    
    return ret_val;
}

static int do_local_linear_baseline_fit(scanPtr s)
{
    int i, j, n, err, ncoeffs = bl.norder + 1;
    static int size = 0;
    static double **a, *b;
    
    int GaussJ(double **, int, double *, int);
    void update_bl_data();
    
    if (ncoeffs != size) {
        if (size > 0) {
            ret_llbf(a, b, size, 0);
            size = 0;
        }
        a = AllocDoubleArray(ncoeffs, ncoeffs);
        if (!a) {
            return ret_llbf(NULL, NULL, ncoeffs, 1);
        }
        b = AllocDoubleVector(ncoeffs);
        if (!b) {
            return ret_llbf(a, NULL, ncoeffs, 2);
        }
        size = ncoeffs;
    }
    
    update_bl_data(s);
    
    for (i=0; i<ncoeffs; i++) {
        b[i] = 0.0;
        for (n=0; n<noblchan; n++) {
            b[i] += pow(xblval[n], (double)i) * yblval[n];
        }
        for (j=i; j<ncoeffs; j++) {
            a[i][j] = 0.0;
            for (n=0; n<noblchan; n++) {
                a[i][j] += pow(xblval[n], (double)(i+j));
            }
            if (i != j) a[j][i] = a[i][j];
        }
    }
    
    if ((err = GaussJ(a, ncoeffs, b, 0))) {
        return 3;
    }
    
    for (i=0; i<ncoeffs; i++)
        s->coeffs[i] = b[i];
    
    return 0;
}

static int do_local_baseline_fit(scanPtr s, int report)
{
    int i, isin, mfit, err;
    double chisq;

    double SpecUnitConv(int, int, double);
    void update_bl_data(), lm_poly(), lm_cheby(), lm_polysin(), lm_polysinc();
    void send_line(), wprintf();
    void (*fpoly)();
    int Fitter1D(double x[], double y[], double e[], int nData,
                 double p[], int fit[], double q[], int nPar,
                 int nIter, double *chi2, void (*f)());

    if (report) wprintf(gp->TBaseline[4], "");
    
    update_bl_data(s);

    fpoly = lm_poly;
    if (bl.pol_type == POLTYPE_CHE) fpoly = lm_cheby;
    if (bl.pol_type == POLTYPE_SIN) fpoly = lm_polysin;
    if (bl.pol_type == POLTYPE_SINC) fpoly = lm_polysinc;

    mfit = bl.norder + 1;
    if (bl.pol_type == POLTYPE_SIN ||
        bl.pol_type == POLTYPE_SINC) mfit += 3;

    for (i=0; i<mfit; i++) {
        isin = i-bl.norder-1;
        if (isin >= 0) {
            if (s->scoeffs[isin] == 0.0) {
                bl_parameters[i] = 1.0;
            } else {
                bl_parameters[i] = s->scoeffs[isin];
            }
        } else {
            bl_parameters[i] = s->coeffs[i];
        }
        bl_lista[i] = 1;
    }

    err = Fitter1D(xblval, yblval, zblval, noblchan,
                   bl_parameters, bl_lista, bl_errors, mfit,
                   5, &chisq, fpoly);
    if (err != 0) return err;

    for (i=0; i<mfit; i++) {
        isin = i-bl.norder-1;
        if (isin >= 0) {
            if (isin == 2 && bl.pol_type == POLTYPE_SIN) {
                s->scoeffs[isin] = fmod(bl_parameters[i], 2.0*PI);
            } else {
                s->scoeffs[isin] = bl_parameters[i];
            }
        } else {
            s->coeffs[i] = bl_parameters[i];
        }
    }

    if (report && noblchan) {
        if (bl.pol_type == POLTYPE_SIN) {
            wprintf(gp->TBaseline[1], "Pol.[%d]+sin(T=%g)", bl.norder,
                    1.0/s->scoeffs[1]/2.0*
                    fabs(SpecUnitConv(vP->xunit, UNIT_CHA, 0) -
                         SpecUnitConv(vP->xunit, UNIT_CHA, s->nChan-1)));
        } else if (bl.pol_type == POLTYPE_SINC) {
            wprintf(gp->TBaseline[1], "Pol.[%d]+sinc(1=%g)", bl.norder,
                    s->scoeffs[1]);
        }
        wprintf(gp->TBaseline[4], "Chi^2: %11.4e",
                chisq/(double)noblchan);
    }
    
    return 0;
}

void do_baseline_fit(Widget w, char *cmd, XtPointer cd)
{
    Widget wait=NULL, scale;
    string buf;
    int err=0, lerr = 0, n;
    list curr = NULL;
    scanPtr s;

    void send_line(), SetWatchCursor(), SetAnyToggle();

    if (bl.norder < 0) {
        PostErrorDialog(w, "Polynomial order must be >= 0!");
        return;
    }
    if (bl.norder > MAXORDER) {
        sprintf(buf, "Polynomial order must be <= %d!", MAXORDER);
        PostErrorDialog(w, buf);
        return;
    }
    if (bl.pol_type < POLTYPE_STD || bl.pol_type > POLTYPE_SINC) {
        sprintf(buf, "Unknown polynomial type: %d", bl.pol_type);
        PostErrorDialog(w, buf);
        return;
    }
    if (nbox <= 0) {
        PostErrorDialog(w, "No baseline boxes selected!");
        return;
    }

    SetWatchCursor(True);

    if (vP->mode == SHOW_SPE) {
        if (bl.pol_type == POLTYPE_LIN) {
            err = do_local_linear_baseline_fit(vP->s);
            if (err) sprintf(buf, "Polynomial fit failed (Error=%d).", err);
        } else {
            err = do_local_baseline_fit(vP->s, 1);
            if (err) sprintf(buf, "Polynomial fit failed (Error=%d).", err);
        }
        if (err) {
            SetWatchCursor(False);
            PostErrorDialog(w, buf);
            return;
        }
    } else {
        if (count_scans(vP->from) < 2) {
            SetWatchCursor(False);
            return;
        }
        if (count_scans(vP->from) > WAITSPECTRA) {
            wait = PostWaitingDialog(w, "Fitting baselines...",
                                     &scale, count_scans(vP->from));
        }
        n = err = 0;
        while ( (curr = scan_iterator(curr, vP->from)) ) {
            s = (scanPtr)DATA(curr);
            if (wait) SetWaitingScale(scale, n+1);
            if (bl.pol_type == POLTYPE_LIN) {
                err = do_local_linear_baseline_fit(s);
            } else {
                err = do_local_baseline_fit(s, (n == 0) ? 1 : 0);
            }
            if (err != 0) {
                lerr = 1;
                sprintf(buf, "Warning: No baseline fit for %s (n=%d, Error=%d)",
                        s->name, n, err);
                send_line(buf);
            }
            n++;
        }
        if (lerr) PostErrorDialog(w,
            "Polynomial fit failed for one or more spectra.\nSee message log.");
    }
    
    if (wait) XtDestroyWidget(wait);

    SetWatchCursor(False);

    if (cmd && strcmp(cmd, "Macro")==0)
        SetAnyToggle("poly", 0);
    else
        SetAnyToggle("poly", 1);
}

static double chan2poly(scanPtr s, int chan)
{
    if (s->nChan <= 1) return UNDEF;
    
    return (-1.0 + (double)(2*chan)/(double)(s->nChan - 1));
}

static double bl_poly(scanPtr s, int chan)
{
    int i;
    double x, val = s->coeffs[0];
    double cheby_poly(), sinc();

    x = chan2poly(s, chan);
    for (i=1; i<=bl.norder; i++) {
        if (bl.pol_type == POLTYPE_CHE) {
            val += s->coeffs[i]*cheby_poly(i, x);
        } else {
            val += s->coeffs[i]*pow(x, (double)i);
        }
    }
    
    if (bl.pol_type == POLTYPE_SIN) {
        val += s->scoeffs[0] *
               sin(2.0*PI*(s->scoeffs[1])*x + s->scoeffs[2]);
    } else if (bl.pol_type == POLTYPE_SINC) {
        val += s->scoeffs[0] *
               sinc((x-s->scoeffs[1])/s->scoeffs[2]);
    }
    
    return val;
}

void draw_poly(GC gc, scanPtr s)
{
   int i;
   int x1, x2, y1, y2;
#ifdef HAVE_LIBPGPLOT
   PLFLT fx[2], fy[2];
#endif
   
   int chan2x(), yunit2y();
   double chan2xunit();
   void draw_line();
   
#ifdef HAVE_LIBPGPLOT
   if (pgplot) {
       SetPGStyle(&ps.poly);
   }
#endif
   for (i=0; i<s->nChan-1; i++) {
       x1 = chan2x(i);
       x2 = chan2x(i+1);
       y1 = yunit2y(bl_poly(s, i));
       y2 = yunit2y(bl_poly(s, i+1));
       draw_line(gc, x1, y1, x2, y2);
#ifdef HAVE_LIBPGPLOT
       if (pgplot) {
           fx[0] = (PLFLT)chan2xunit(i);
           fx[1] = (PLFLT)chan2xunit(i+1);
           fy[0] = (PLFLT)bl_poly(s, i);
           fy[1] = (PLFLT)bl_poly(s, i+1);
           cpgline(2, fx, fy);
       }
#endif
   }
}

static void PrintBaselineWidgets(int nb, int nc, double *m, double *s)
{
    string bstr, cstr;
    
    void wprintf();
    
    if (nb == 1)
        strcpy(bstr, "box");
    else
        strcpy(bstr, "boxes");
    
    if (nc == 1)
        strcpy(cstr, "channel");
    else
        strcpy(cstr, "channels");
    
    if (nb > 0 && nc > 0)
        wprintf(gp->TBaseline[0],  "%d %s, %d %s", nb, bstr, nc, cstr);
    else if (nb > 0 && nc == 0)
        wprintf(gp->TBaseline[0],  "%d %s, no %s", nb, bstr, cstr);
    else if (nb == 0 && nc > 0)
        wprintf(gp->TBaseline[0],  "No %s, %d %s", bstr, nc, cstr);
    else if (nb == 0 && nc == 0)
        wprintf(gp->TBaseline[0],  "No %s, no %s", bstr, cstr);
            
    wprintf(gp->TBaseline[1], "%s of order %d",
            PolType[bl.pol_type], bl.norder);

    if (!m)
        wprintf(gp->TBaseline[2],  "Mean: *");
    else if (fabs(*m) < 1.e-3)
        wprintf(gp->TBaseline[2],  "Mean: %10.3e K", *m);
    else if (fabs(*m) < 0.1)
        wprintf(gp->TBaseline[2],  "Mean: %7.3f mK", (*m) * 1000.0);
    else if (fabs(*m) < 10.0)
        wprintf(gp->TBaseline[2],  "Mean: %7.3f K", *m);
    else if (fabs(*m) < 1000.0)
        wprintf(gp->TBaseline[2],  "Mean: %7.1f K", *m);
    else
        wprintf(gp->TBaseline[2],  "Mean: %10.3e K", *m);
    
    if (!s)
        wprintf(gp->TBaseline[3],  "Sigma: *");
    else if (fabs(*s) < 1.e-3)
        wprintf(gp->TBaseline[3],  "Sigma: %10.3e K", *s);
    else if (fabs(*s) < 0.1)
        wprintf(gp->TBaseline[3],  "Sigma: %7.3f mK", (*s) * 1000.0);
    else if (fabs(*s) < 10.0)
        wprintf(gp->TBaseline[3],  "Sigma: %7.3f K", *s);
    else if (fabs(*s) < 1000.0)
        wprintf(gp->TBaseline[3],  "Sigma: %7.1f K", *s);
    else
        wprintf(gp->TBaseline[3],  "Sigma: %10.3e K", *s);
}

void update_bl_data(scanPtr s)
{
    int n, i, j, x1, x2, zeroRMS=0;
    static int alloc_size=0;
    double s0, s1=0.0, s2=0.0, s3=0.0;
    string buf;

    void send_line();
                                         /* Malloc if larger size is needed */
    if (s->nChan > alloc_size && nbox > 0) {
        if (alloc_size > 0) {            /* but free first if necessary */
            FreeDoubleVector(xblval);
            FreeDoubleVector(yblval);
            FreeDoubleVector(zblval);
        }
        xblval = AllocDoubleVector(s->nChan);
        yblval = AllocDoubleVector(s->nChan);
        zblval = AllocDoubleVector(s->nChan);
        if (!xblval || !yblval || !zblval) {
            if (xblval) FreeDoubleVector(xblval);
            if (yblval) FreeDoubleVector(yblval);
            if (zblval) FreeDoubleVector(zblval);
            alloc_size = 0;
            return;
        } else {
            alloc_size = s->nChan;           /* Ok, keep the new vector size */
        }
    }

    j = 0;
    for (n=0; n<nbox; n++) {
        x1 = boxar[n].begin;
        x2 = boxar[n].end;
        for (i=x1; i<=x2; i++) {
            if (i < 0 || i >= s->nChan) continue;
            s0 = s->d[i];
            if (j >= alloc_size) {
                sprintf(buf, "update_bl_data: j (%d) > alloc_size (%d)\n",
                        j, alloc_size);
                send_line(buf);
                continue;
            }
            xblval[j] = chan2poly(s, i);
            yblval[j] = s0;
            zblval[j] = s->e[i];
            if (zblval[j] <= 0.0) zeroRMS = 1;
            s1 += s0;
            s2 += s0*s0;
            s3 += zblval[j];
            j++;
        }
    }

    if (j > 1) {
        noblchan = j;
        bl.mean = s1/(double)j;
        bl.sigma = sqrt((s2 - s1*s1/(double)j)/(double)(j-1));
        s->mom.sigma = bl.sigma;
        bl.iint = s1*fabs(s->velres);
        bl.iunc = s3/sqrt((double)j)*fabs(s->velres);
        if (s == vP->s) {
            PrintBaselineWidgets(nbox, j, &bl.mean, &bl.sigma);
        }
        if (zeroRMS) {
            for (i=0; i<noblchan; i++) zblval[i] = bl.sigma;
        }
    } else if (j == 1) {
        bl.mean = s1;
        s->mom.sigma = bl.sigma = 0.0;
        bl.iint = s1*fabs(s->velres);
        bl.iunc = s3*fabs(s->velres);
        if (s == vP->s) {
            PrintBaselineWidgets(nbox, j, &bl.mean, NULL);
        }
    } else {
        bl.mean = 0.0;
        s->mom.sigma = bl.sigma = 0.0;
        bl.iint = 0.0;
        bl.iunc = 0.0;
        noblchan = 0;
        if (s == vP->s) {
            PrintBaselineWidgets(nbox, j, NULL, NULL);
        }
    }
}

void reset_bl_parameters(Widget w, char *client_data, XtPointer call_data)
{
    int i;
    list curr = NULL;
    scanPtr s;

    while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
        s = (scanPtr)DATA(curr);
        for (i=0; i<=MAXORDER; i++) s->coeffs[i] = 0.0;
        for (i=0; i<3; i++) s->scoeffs[i] = 0.0;
    }
}

void SetRmsFromBoxes(Widget w, char *cmd, XtPointer cd)
{
    int i;
    list curr = NULL;
    scanPtr s;
    
    void UpdateData();
    void SetAnyToggle();

    if (vP->mode == SHOW_SPE && vP->s) {
        if (vP->s->mom.sigma > 0.0) {
            for (i=0; i<vP->s->nChan; i++) {
                vP->s->e[i] = vP->s->mom.sigma;
            }
        }
    } else {
        while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
            s = (scanPtr)DATA(curr);
            if (s->mom.sigma > 0.0) {
                for (i=0; i<s->nChan; i++) {
                    s->e[i] = s->mom.sigma;
                }
            }
        }
    }
    
    UpdateData(SCALE_NONE, NO_REDRAW);
    
    if (strcmp(cmd, "Macro")==0)
        SetAnyToggle("rms", 0);
    else
        SetAnyToggle("rms", 1);
}

static void local_remove_poly(scanPtr s)
{
    int i;
   
    if (!s) return;
    
    for (i=0; i<s->nChan; i++) s->d[i] -= bl_poly(s, i);
    
    s->saved = 0;
}

void remove_poly(Widget w, char *cmd, XtPointer cd)
{
    list curr = NULL;
    
    void UpdateData(), UnsetAnyToggle(), SetWatchCursor();
  
    SetWatchCursor(True);
    
    if (vP->mode == SHOW_SPE) {
        local_remove_poly(vP->s);
    } else {
        while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
            local_remove_poly((scanPtr)DATA(curr));
        }
    }
    
    UpdateData(SCALE_ONLY_Y, NO_REDRAW);
        
    SetWatchCursor(False);
    
    if (cmd && strcmp(cmd, "Macro")==0)
        UnsetAnyToggle("poly", 0);
    else
        UnsetAnyToggle("poly", 1);
}

static void local_add_poly(scanPtr s)
{
    int i;
    
    if (!s) return;

    for (i=0; i<s->nChan; i++) s->d[i] += bl_poly(s, i);
    
    s->saved = 0;
}

void add_poly(Widget w, char *cmd, XtPointer cd)
{
    list curr = NULL;
    
    void UpdateData(), SetAnyToggle(), SetWatchCursor();
  
    SetWatchCursor(True);
  
    if (vP->mode == SHOW_SPE) {
        local_add_poly(vP->s);
    } else {
        while ( (curr = scan_iterator(curr, vP->from)) ) {
            local_add_poly((scanPtr)DATA(curr));
        }
    }
    
    UpdateData(SCALE_NONE, NO_REDRAW);
        
    SetWatchCursor(False);
    
    if (strcmp(cmd, "Macro")==0)
        SetAnyToggle("poly", 1);
    else
        SetAnyToggle("poly", 1);
}

static void local_interpolate(scanPtr s)
{
    int n, c;
    double x;
    
    double XS_NormalRnd(double, double);
    double XS_UniformRnd(double, double);
   
    if (!s) return;
    
    /* for (n=0; n<nreg; n++) {
        c1 = regs[n].begin;
        c2 = regs[n].end;
        s0 = 0.0;
        nc = c2 - c1 + 1;
        for (c=c1; c<=c2; c++) {
            if (c < 0 || c >= s->nChan) continue;
            x = s->d[c] - bl_poly(s,c);
            s0 += x*x/(double)nc;
        }
        if (sqrt(s0) > 3.0*bl.sigma) {
            for (c=c1; c<=c2; c++) {
                if (c < 0 || c >= s->nChan) continue;
                s->d[c] = XS_NormalRnd(bl_poly(s, c), bl.sigma);
            }
        }
    } */
    
    for (n=0; n<nreg; n++) {
        for (c=regs[n].begin; c<=regs[n].end; c++) {
            if (c < 0 || c >= s->nChan) continue;
	    x = bl_poly(s,c);
	    if (fabs(s->d[c] - x) < 3.0*bl.sigma) continue;
            s->d[c] = XS_NormalRnd(x, bl.sigma);
	    if (s->fft == 2)
	      s->e[c] = XS_UniformRnd(0.0, 2.0*PI);
	    else
	      s->e[c] = bl.sigma;
        }
    }
    
    s->saved = 0;
}

void interpolate_from_poly(Widget w, char *cmd, XtPointer cd)
{
    list curr = NULL;
    DataSetPtr d;
    scanPtr s, first=NULL;
    
    void UpdateData(), SetAnyToggle(), SetWatchCursor();
    void XS_RndInitClock();
  
    d = new_dataset(get_listlist(), "Interpolated", vP->from);
    if (!d) return;
    
    XS_RndInitClock();
    
    SetWatchCursor(True);
  
    if (vP->mode == SHOW_SPE) {
        first = s = copy_scan(d, vP->s);
        local_interpolate(s);
    } else {
        while ( (curr = scan_iterator(curr, vP->from)) ) {
            s = (scanPtr)DATA(curr);
            if (s == vP->s) first = s;
            s = copy_scan(d, s);
            local_interpolate(s);
        }
    }
    
    sprintf(d->name, "Interpolated %s", vP->from->name);
    
    vP->from = vP->to = d;
    vP->s = first;
    
    UpdateData(SCALE_ONLY_Y, NO_REDRAW);
        
    SetWatchCursor(False);
    
    if (strcmp(cmd, "Macro")==0)
        SetAnyToggle("poly", 1);
    else
        SetAnyToggle("poly", 1);
}

static void SetBlPolTypeCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    int n = atoi(s);
    
    void wprintf();
    
    if (n != bl.pol_type) {
        bl.pol_type = n;
        wprintf(gp->TBaseline[1], "%s of order %d",
                PolType[bl.pol_type], bl.norder);
    }
}

static void SetBlPolOrderCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    int order = atoi(s);
    string buf;
    
    void wprintf();
    
    if (order > MAXORDER || order < 0) {
        sprintf(buf, "Polynomial order out of range: %d > %d",
                order, MAXORDER);
        PostErrorDialog(w, buf);
    } else {
        bl.norder = order;
        wprintf(gp->TBaseline[1], "%s of order %d",
                PolType[bl.pol_type], bl.norder);
    }
}

void PolynomialOrderDialog(Widget wid, char *cmd, XtPointer call_data)
{
    Widget rc, menuT, menuO;
    StdForm *sf;
    Widget w = wid;

    void wprintf();

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Baseline polynomial",
             NULL, NULL, NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, (XtPointer)BlPolType_Help,
             0, NULL);
    
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          NULL);

    menuT = CreateOptionMenu(rc, &BlPolTypeMenu);
    SetDefaultOptionMenuItem(menuT, bl.pol_type);
    
    menuO = CreateOptionMenu(rc, &BlPolOrderMenu);
    SetDefaultOptionMenuItem(menuO, bl.norder);
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(menuT);
    XtManageChild(menuO);
    XtManageChild(rc);
    
    ManageDialogCenteredOnPointer(sf->form);
}

typedef struct {
    int np, ns;
    Widget *p;
    Widget *s;
    scanPtr sp;
    DataSetPtr dsp;
} BL_WL;

static void GetBLParameters(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int n;
    double d;
    scanPtr s, a;
    DataSetPtr dsp;
    BL_WL *wl = NULL;
    list curr = NULL;
    
    void wdscanf();
    
    if (!sf) return;
    if (!sf->user) return;
    
    wl = (BL_WL *)sf->user;
    
    s = wl->sp;
    dsp = wl->dsp;
    if (!s) return;
    
    for (n=0; n<wl->np; n++) {
        wdscanf(wl->p[n], &d);
        s->coeffs[n] = d;
    }
    for (n=0; n<wl->ns; n++) {
        wdscanf(wl->s[n], &d);
        s->scoeffs[n] = d;
    }
    if (dsp) { /* Also set coeffs for all other scans in data set */
        while ( (curr = scan_iterator(curr, dsp)) != NULL ) {
            a = (scanPtr)DATA(curr);
            if (a == s) continue;
            for (n=0; n<wl->np; n++) {
                a->coeffs[n] = s->coeffs[n];
            }
            for (n=0; n<wl->ns; n++) {
                a->scoeffs[n] = s->scoeffs[n];
            }
        }
    }
}

static void BL_WL_cleanup(XtPointer user)
{
    BL_WL *wl;
    
    if (!user) return;
    
    wl = (BL_WL *)user;
    
    if (wl->np > 0) XtFree((char *)(wl->p));
    if (wl->ns > 0) XtFree((char *)(wl->s));
}

static void edit_baseline_parameters(Widget wid, DataSetPtr d, scanPtr s)
{
    int n, npol=0, nsin=0;
    Widget rc, row, tmp;
    Widget w = wid;
    StdForm *sf;
    BL_WL *wl;
    
    void wprintf();

    if (!s) return;
    
    while (!XtIsWMShell(w))
        w = XtParent(w);
        
    npol = bl.norder + 1;
    if (bl.pol_type == POLTYPE_SIN ||
        bl.pol_type == POLTYPE_SINC) nsin = 3;    
    
    wl = (BL_WL *) XtMalloc(sizeof(BL_WL));
    if (!wl) return;
    
    wl->p = (Widget *) XtMalloc(npol * sizeof(Widget));
    if (!wl->p) {
        XtFree((char *)wl);
        return;
    }
    
    if (nsin > 0) {
        wl->s = (Widget *) XtMalloc(nsin * sizeof(Widget));
        if (!wl->s) {
            XtFree((char *)(wl->p));
            XtFree((char *)wl);
            return;
        }
    } else {
        wl->s = NULL;
    }
    
    wl->np  = npol;
    wl->ns  = nsin;
    wl->sp  = s;
    wl->dsp = d;
    
    sf = PostStdFormDialog(w, "Polynomial data",
             BUTT_APPLY, (XtCallbackProc)GetBLParameters, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 0, BL_WL_cleanup);
             
    sf->user = (XtPointer)wl;
    
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          NULL);
    
    for (n=0; n<npol; n++) {
        row = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                      XmNorientation, XmHORIZONTAL, NULL);
        
        tmp = XtVaCreateManagedWidget("label", xmLabelWidgetClass, row, NULL);
        wprintf(tmp, "a[%d]:", n);
        wl->p[n] = XtVaCreateManagedWidget("edit", xmTextWidgetClass, row, NULL);
        wprintf(wl->p[n], "%g", s->coeffs[n]);
    }
    
    if (nsin) {
        for (n=0; n<nsin; n++) {
            row = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                          XmNorientation, XmHORIZONTAL, NULL);
        
            tmp = XtVaCreateManagedWidget("label", xmLabelWidgetClass, row,
                                          NULL);
            if (n == 0) {
                wprintf(tmp, "Ampl.");
            } else if (n == 1) {
                if (bl.pol_type == POLTYPE_SIN)
                    wprintf(tmp, "1/T  ");
                else
                    wprintf(tmp, "Center");
            } else {
                if (bl.pol_type == POLTYPE_SIN)
                    wprintf(tmp, "Phase");
                else
                    wprintf(tmp, "Width");
            }
            wl->s[n] = XtVaCreateManagedWidget("edit", xmTextWidgetClass,
                                              row, NULL);
            wprintf(wl->s[n], "%g", s->scoeffs[n]);
        }
    }
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(rc);
    
    ManageDialogCenteredOnPointer(sf->form);
}

void EditCurrentBaselineParameters(Widget w, char *cmd, XtPointer cd)
{
    Widget wid = w;
    
    if (!wid) wid = gp->top;
    
    if (vP->mode == SHOW_SPE)
        edit_baseline_parameters(wid, NULL, vP->s);
    else
        edit_baseline_parameters(wid, vP->from, vP->s);
} 
