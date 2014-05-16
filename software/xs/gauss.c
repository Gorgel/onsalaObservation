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
#include <stdlib.h>
#include <math.h>

#include <Xm/Text.h>
#include <Xm/ScrolledW.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Frame.h>

#include "defines.h"
#include "global_structs.h"
#include "list.h"
#include "menus.h"
#include "dialogs.h"

#ifdef HAVE_LIBPGPLOT
#include "cpgplot.h"
#endif

/*** External variables ***/
extern int      pgplot;
extern int      gau_w, gau_h;

extern int      rgauss_sel, gauss_show;
extern int      nreg, nbox;
extern BOX      regs[MAXBOX];
extern BLINE    bl;

extern VIEW    *vP;
extern Gauss    gau;
extern int      nmark, mark_xunit;
extern MARK     marks[MAXMARK];
extern PSDATA   ps;

extern GLOBAL  *gp;
extern USER    *pP;

void   PostErrorDialog(Widget, char *);
void   PostWarningDialog(Widget, char *);
void   PostMessageDialog(Widget, char *);
int    PostQuestionDialog(Widget, char *);
void   ManageDialogCenteredOnPointer(Widget);
void   SetPGStyle(PSSTY *);
void   UnsetAnyToggle(char *, int);
void   SetAnyToggle(char *, int);
void   send_line(char *);
Widget CreateOptionMenu(Widget, MenuBarItem *);
void   SetDefaultOptionMenuItem(Widget, int);

list    scan_iterator(list, DataSetPtr);

double  *AllocDoubleVector(int);
void     FreeDoubleVector(double *);
double **AllocDoubleArray(int, int);
int     *AllocIntVector(int);

int     Fitter1D(double x[], double y[], double e[], int nData,
                 double p[], int fit[], double q[], int nPar,
                 int nIter, double *chi2, void (*f)());

/*** Local variables ***/
#define GAUSS_ABS	0
#define GAUSS_REL   1

#define GAUSS_ITER 7
#define NSTEPS  301

typedef struct {
  Widget form;
  Widget num_lw;
  Widget sub_tb;
  Widget fre_tw, fre_tb;
  Widget amp_tw, amp_tb;
  Widget wid_tw, wid_tb;
  Widget mol_tw;
  Widget tra_tw;
  Widget ref_tw;
} GAUSSROW;

static Widget   w_xunit, gauss_rc;
static int      ngauss, nGaussChan, gauss_xunit, gauss_mode, gauss_show_errors;
static GAUSSROW gw[MAXGAUSS];
static Gauss    gaussar[MAXGAUSS];
static int      not_fitted[3*MAXGAUSS];
static int      sub_sel[MAXGAUSS];
static double   *parameters, *par_errors;
static double   *xval, *yval, *zval;
static int      nopar, *lista, mfit;

static string XUnitLabel[] = {
 "     Frequency [GHz]    ",
 "     Velocity [km/s]    ",
 "     Channel number     ",
 "     Frequency [MHz]    ",
 "     ---------------    ",
 "     ---------------    ",
 "     Frequency [MHz]    "
};

static char *GaussLabel[] = {
 "No.",
 "Sel ",
 "     Frequency [GHz]    ",
 "      Amplitude [K]     ",
 "       Width [km/s]     ",
 NULL
};

static void GaussModeCallback();
static MenuItem GaussModeData[] = {
   {"Absolute gaussians", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, GaussModeCallback, "0", NULL},
   {"Relative gaussians", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, GaussModeCallback, "1", NULL},
EOI};
static MenuBarItem GaussModeMenu = {
   "Type of Gauss-fitting mode", ' ', True, GaussModeData
};

static void GaussErrorCallback();
static MenuItem GaussErrorData[] = {
   {"No", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, GaussErrorCallback, "0", NULL},
   {"Yes", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, GaussErrorCallback, "1", NULL},
EOI};
static MenuBarItem GaussErrorMenu = {
   "Show 1-sigma errors?", ' ', True, GaussErrorData
};

static char *GaussMode_Help = "\
                     Gaussian fitting help\n\
                     ---------------------\n\
In this dialog you can specify the type of mode used to fit the Gaussians. The\n\
options are:\n\
    Absolute    The Gaussians are treated independantly using center value,\n\
                amplitude, and width (FWHM). They can be selected as fixed\n\
                or to be iterated.\n\
    Relative    The first Gaussian is treated as in absolute mode above. All\n\
                subsequent entered Gaussians are, however, given with respect\n\
                to the first Gaussian in terms of offset from the center of\n\
                the first Gaussian, relative amplitude, and relative width.\n\
                This mode is useful, when fitting a group of Gaussian that\n\
                should have the same width (fix the width for Gaussians 2-) or\n\
                a group of Gaussians with known difference in frequency and/or\n\
                relative amplitude.\n\
";

static char *edit_gauss_labs[] = {
    "Amplitude:", "Width [km/s]", "Centre [km/s]:"
};

typedef struct {
   Gauss g;
   scanPtr s;
   Widget w;
} GaussEdit;

static void make_each_gw(int);

void init_gauss_parameters()
{
    int    i;
    
    int     GetStartingUnit();

    parameters = AllocDoubleVector(3*MAXGAUSS);
    par_errors = AllocDoubleVector(3*MAXGAUSS);
    lista      = AllocIntVector(3*MAXGAUSS);

    gauss_xunit = GetStartingUnit();
    gauss_mode  = GAUSS_ABS;
    gauss_show_errors = 0;

    for (i=0; i<MAXGAUSS; i++)   sub_sel[i]    = False;
    for (i=0; i<3*MAXGAUSS; i++) not_fitted[i] = False;
}

void set_gauss_data(scanPtr s)
{
    int i, j, m;
    static int size=0;
    double x, y, e;
    
    if (!s) return;

    j = 0;
    for (i=0; i<s->nChan; i++) {
        if (nreg == 0) {
            j++;
        } else {
            for (m=0; m<nreg; m++) {
                if (i < regs[m].begin || i > regs[m].end) continue;
                j++;
            }
        }
    }
    nGaussChan = j;

    if (nGaussChan > size) {
        if (size) {
            FreeDoubleVector(xval);
            FreeDoubleVector(yval);
            FreeDoubleVector(zval);
        }
        xval = AllocDoubleVector(nGaussChan);
        yval = AllocDoubleVector(nGaussChan);
        zval = AllocDoubleVector(nGaussChan);
        size = nGaussChan;
    }

    j = 0;
    for (i=0; i<s->nChan; i++) {
        x = (double)i;
        y = s->d[i];
        if (nbox == 0)
            e = s->e[i];
        else
            e = bl.sigma;

        if (nreg == 0) {
            xval[j] = x;
            yval[j] = y;
            zval[j] = e;
            j++;
        } else {
            for (m=0; m<nreg; m++) {
                if (i < regs[m].begin || i > regs[m].end) continue;
                xval[j] = x;
                yval[j] = y;
                zval[j] = e;
                j++;
            }
        }
    }
}

static void assign_gaussar()
{
    int i, j;
  
    for (i=0,j=0; i<nopar; i+=3,j++) {
        if (gauss_mode == GAUSS_ABS) {
            gaussar[j].amp  = parameters[i];
            gaussar[j].uamp = par_errors[i];
            if (j == 0) {
                gaussar[j].cen = parameters[i+1];
            } else {
                gaussar[j].cen = parameters[i+1] + gaussar[0].cen;
            }
            gaussar[j].ucen = par_errors[i+1];
            gaussar[j].wid  = SQALPHA*parameters[i+2];
            gaussar[j].uwid = SQALPHA*par_errors[i+2];
        } else {
            if (j==0) {
                gaussar[j].amp  = parameters[i];
                gaussar[j].uamp = par_errors[i];
                gaussar[j].cen  = parameters[i+1];
                gaussar[j].ucen = par_errors[i+1];
                gaussar[j].wid  = SQALPHA*parameters[i+2];
                gaussar[j].uwid = SQALPHA*par_errors[i+2];
             } else {
                gaussar[j].amp  = parameters[i]*gaussar[0].amp;
                gaussar[j].uamp = par_errors[i]*gaussar[0].amp;
                gaussar[j].cen  = parameters[i+1] + gaussar[0].cen;
                gaussar[j].ucen = par_errors[i+1];
                gaussar[j].wid  = parameters[i+2]*gaussar[0].wid;
                gaussar[j].uwid = par_errors[i+2]*gaussar[0].wid;
            }
        }
    }
}

static void assign_parameters()
{
    int i, j;
    double rc=0.0, ra=1.0, rw= 1.0;
    
    if (ngauss > 0) rc = gaussar[0].cen;
    
    if (gauss_mode == GAUSS_REL && ngauss > 1) {
        ra = gaussar[0].amp;
        if (ra == 0.0) ra = 1.0;
        rw = gaussar[0].wid;
        if (rw == 0.0) rw = 1.0;
    }

    nopar = ngauss*3;
    for (i=0,j=0; i<nopar; i+=3,j++){
        if (gauss_mode == GAUSS_ABS || j == 0) {
            parameters[i]   = gaussar[j].amp;
            parameters[i+1] = gaussar[j].cen - (j > 0 ? rc : 0.0);
            parameters[i+2] = gaussar[j].wid/SQALPHA;
        } else {
            parameters[i]   = gaussar[j].amp/ra;
            parameters[i+1] = gaussar[j].cen - rc;
            parameters[i+2] = gaussar[j].wid/rw;
        }
    }
}

static void assign_lista()
{
    int i;

    mfit = 0;
    for (i=0; i<nopar; i++) {
        if (!not_fitted[i]) {
            lista[i] = 1;
            mfit++;
        } else {
            lista[i] = 0;
        }
    }
}

static void update_gauss_data()
{
    void wprintf();

    if (gauss_mode == GAUSS_ABS)
        wprintf(gp->TGauss[0], "Absolute mode");
    else
        wprintf(gp->TGauss[0], "Relative mode");
    
    if (ngauss <= 0) {
        wprintf(gp->TGauss[1], "No Gaussians fitted");
    } else if (ngauss == 1) {
        wprintf(gp->TGauss[1], "1 Gaussian fitted");
    } else {
        wprintf(gp->TGauss[1], "%d Gaussians fitted", ngauss);
    }
}

double gauss(double x, double c, double a, double w)
{
    double y=0.0, arg;
    
    if (w != 0.0) {
        arg = (x - c)/w;
        y = a * exp(-ALPHA * arg * arg);
    }
    
    return y;
}
        
void draw_gauss_sum(GC gc)
{
    int i, j, i1, i2;
    int x1, x2, y1, y2;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx[2], fy[2];
#endif
    double c1, c2, dy1, dy2;

    int chan2x(), yunit2y(), x2chan();
    double chan2xunit(), x2dchan(), x2xunit();
    void draw_line();

    if (ngauss <= 0) return;

#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.gauss);
    }
#endif

    i1 = vP->min_x;
    i2 = vP->min_x + vP->box_w;
    x1 = x2chan(i1);
    x2 = x2chan(i2);
    if (x1 < 0) {
        x1 = 0;
        i1 = chan2x(x1);
    }
    if (x2 >= vP->s->nChan) {
        x2 = vP->s->nChan - 1;
        i2 = chan2x(x2);
    }

    if (i2 - i1 > x2 - x1) {
        for (i=i1; i<i2; i++) {
            c1 = x2dchan(i);
            c2 = x2dchan(i+1);
            dy1 = 0.0;
            dy2 = 0.0;
            for (j=0; j<ngauss; j++) {
                dy1 += gauss(c1, gaussar[j].cen, gaussar[j].amp,
                             gaussar[j].wid);
                dy2 += gauss(c2, gaussar[j].cen, gaussar[j].amp,
                             gaussar[j].wid);
            }
            y1 = yunit2y(dy1);
            y2 = yunit2y(dy2);
            draw_line(gc, i, y1, i+1, y2);
#ifdef HAVE_LIBPGPLOT
            if (pgplot) {
                fx[0] = (PLFLT)x2xunit(i);
                fx[1] = (PLFLT)x2xunit(i+1);
                fy[0] = (PLFLT)dy1;
                fy[1] = (PLFLT)dy2;
                cpgline(2, fx, fy);
            }
#endif
        }
    } else {
        for (i=x1; i<x2; i++) {
            i1 = chan2x(i);
            i2 = chan2x(i+1);
            dy1 = 0.0;
            dy2 = 0.0;
            for (j=0; j<ngauss; j++) {
                dy1 += gauss((double)i, gaussar[j].cen,
                             gaussar[j].amp, gaussar[j].wid);
                dy2 += gauss((double)(i+1), gaussar[j].cen,
                             gaussar[j].amp, gaussar[j].wid);
            }
            y1 = yunit2y(dy1);
            y2 = yunit2y(dy2);
            draw_line(gc, i1, y1, i2, y2);
#ifdef HAVE_LIBPGPLOT
            if (pgplot) {
                fx[0] = (PLFLT)chan2xunit(i);
                fx[1] = (PLFLT)chan2xunit(i+1);
                fy[0] = (PLFLT)dy1;
                fy[1] = (PLFLT)dy2;
                cpgline(2, fx, fy);
            }
#endif
        }
    }
}

void draw_gauss(GC gc, Gauss g)
{
    int i, i1, i2;
    int x1, x2, y1, y2;
    double c1, c2;

    double x2dchan();
    int chan2x(), yunit2y();
    void draw_line();

    i1 = NINT(g.cen - 2.0*g.wid);
    i2 = NINT(g.cen + 2.0*g.wid);
    if (i2 < i1) { i = i1; i1 = i2; i2 = i; }

    x1 = chan2x(i1);
    x2 = chan2x(i2);

    if (i2-i1 < x2-x1) {
        for (i=x1; i<x2; i++) {
            c1 = x2dchan(i);
            c2 = x2dchan(i+1);
            y1 = yunit2y(gauss(c1, g.cen, g.amp, g.wid));
            y2 = yunit2y(gauss(c2, g.cen, g.amp, g.wid));
            draw_line(gc, i, y1, i+1, y2);
        }
    } else {
        for (i=i1; i<i2; i++) {
            x1 = chan2x(i);
            x2 = chan2x(i+1);
            y1 = yunit2y(gauss((double)i, g.cen, g.amp, g.wid));
            y2 = yunit2y(gauss((double)i+1.0, g.cen, g.amp, g.wid));
            draw_line(gc, x1, y1, x2, y2);
        }
    }
}

void draw_gauss_ind(GC gc)
{
    int i;
#ifdef HAVE_LIBPGPLOT
    int n;
    PLFLT fx[2], fy[2];
#endif

    double chan2xunit();

    for (i=0; i<ngauss; i++) {
        draw_gauss(gc, gaussar[i]);
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            SetPGStyle(&ps.gauss);
            for (n=0; n<vP->s->nChan-1; n++) {
                fx[0] = (PLFLT)chan2xunit(n);
                fx[1] = (PLFLT)chan2xunit(n+1);
                fy[0] = (PLFLT)gauss((double)n,       gaussar[i].cen,
                                     gaussar[i].amp,  gaussar[i].wid);
                fy[1] = (PLFLT)gauss((double)n + 1.0, gaussar[i].cen,
                                     gaussar[i].amp,  gaussar[i].wid);
                cpgline(2, fx, fy);
            }
        }
#endif
    }
}

double GetAnyErf(double x, double *c, int nPar)
{
    double y=0.0;
    
    if (c[2] != 0.0) {
    	y = c[0] * erfc(SQALPHA * (x-c[1])/c[2]);
    }
    
    return y;
}

void DrawAnyErf(GC gc, double *c, int nPar, double xleft, double xright)
{
    int n, nstep = NSTEPS;
    double x1, x2, dx, y1, y2;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx[2], fy[2];
#endif
    
    int xunit2x(), yunit2y();
    void draw_line();
    
    if (xleft == xright) return;
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.gauss);
    }
#endif
    
    dx = (xright - xleft)/(double)(nstep-1);
    
    for (n=0; n<nstep-1; n++) {
        x1 = xleft + (double)n * dx;
        x2 = x1 + dx;
        y1 = y2 = 0.0;
        if (c[2] != 0.0) {
            y1 = c[0] * erfc(SQALPHA * (x1-c[1])/c[2]);
            y2 = c[0] * erfc(SQALPHA * (x2-c[1])/c[2]);
        }
        draw_line(gc, xunit2x(x1), yunit2y(y1), xunit2x(x2), yunit2y(y2));
        
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx[0] = (PLFLT)x1; fy[0] = (PLFLT)y1;
            fx[1] = (PLFLT)x2; fy[1] = (PLFLT)y2;
            cpgline(2, fx, fy);
        }
#endif
    }
}

double GetAnyExpProf(double x, double *c, int nPar)
{
    double y, a;
    
    y = 0.0;
    if (c[2] != 0.0) {
    	a = (x-c[1])/c[2];
    	if (fabs(a) < 1.0) {
    	    if (c[3] == 0.0)
    		y = c[0];
    	    else
    		y = c[0] * pow(1.0 - a*a, c[3]/2.0);
    	}
    }
    
    return y;
}

void DrawAnyExpProf(GC gc, double *c, int nPar, double xleft, double xright)
{
    int n, nstep = NSTEPS;
    double x1, x2, dx, y1, y2, a1, a2;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx[2], fy[2];
#endif
    
    int xunit2x(), yunit2y();
    void draw_line();
    
    if (xleft == xright) return;
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.gauss);
    }
#endif
    
    dx = (xright - xleft)/(double)(nstep-1);
    
    for (n=0; n<nstep-1; n++) {
        x1 = xleft + (double)n * dx;
        x2 = x1 + dx;
        y1 = 0.0;
        y2 = 0.0;
        if (c[2] != 0.0) {
            a1 = (x1-c[1])/c[2];
            a2 = (x2-c[1])/c[2];
            if (fabs(a1) < 1.0) {
                if (c[3] == 0.0)
                    y1 = c[0];
                else
                    y1 = c[0] * pow(1.0 - a1*a1, c[3]/2.0);
            }
            if (fabs(a2) < 1.0) {
                if (c[3] == 0.0)
                    y2 = c[0];
                else
                    y2 = c[0] * pow(1.0 - a2*a2, c[3]/2.0);
            }
        }
        draw_line(gc, xunit2x(x1), yunit2y(y1), xunit2x(x2), yunit2y(y2));
        
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx[0] = (PLFLT)x1; fy[0] = (PLFLT)y1;
            fx[1] = (PLFLT)x2; fy[1] = (PLFLT)y2;
            cpgline(2, fx, fy);
        }
#endif
    }
}

/* void DrawAnySqrt(GC gc, double *c, int nPar, double xleft, double xright)
{
    int n, nstep = NSTEPS;
    double x1, x2, dx, y1, y2, a1, a2;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx[2], fy[2];
#endif
    
    int xunit2x(), yunit2y();
    void draw_line();
    
    if (xleft == xright) return;
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.gauss);
    }
#endif
    
    dx = (xright - xleft)/(double)(nstep-1);
    
    for (n=0; n<nstep-1; n++) {
        x1 = xleft + (double)n * dx;
        x2 = x1 + dx;
        y1 = 0.0;
        y2 = 0.0;
        if (c[2] != 0.0) {
            a1 = (x1-c[1])/c[2];
            a2 = (x2-c[1])/c[2];
            if (fabs(a1) <= 1.0) y1 = c[0] * sqrt(1.0 - a1*a1);
            if (fabs(a2) <= 1.0) y2 = c[0] * sqrt(1.0 - a2*a2);
        }
        draw_line(gc, xunit2x(x1), yunit2y(y1), xunit2x(x2), yunit2y(y2));
        
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx[0] = (PLFLT)x1; fy[0] = (PLFLT)y1;
            fx[1] = (PLFLT)x2; fy[1] = (PLFLT)y2;
            cpgline(2, fx, fy);
        }
#endif
    }
}

void DrawAnyInvSqrt(GC gc, double *c, int nPar, double xleft, double xright)
{
    int n, nstep = NSTEPS;
    double x1, x2, dx, y1, y2, a1, a2;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx[2], fy[2];
#endif
    
    int xunit2x(), yunit2y();
    void draw_line();
    
    if (xleft == xright) return;
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.gauss);
    }
#endif
    
    dx = (xright - xleft)/(double)(nstep-1);
    
    for (n=0; n<nstep-1; n++) {
        x1 = xleft + (double)n * dx;
        x2 = x1 + dx;
        y1 = 0.0;
        y2 = 0.0;
        if (c[2] != 0.0) {
            a1 = (x1-c[1])/c[2];
            a2 = (x2-c[1])/c[2];
            if (fabs(a1) < 1.0) y1 = c[0] / sqrt(1.0 - a1*a1);
            if (fabs(a2) < 1.0) y2 = c[0] / sqrt(1.0 - a2*a2);
        }
        draw_line(gc, xunit2x(x1), yunit2y(y1), xunit2x(x2), yunit2y(y2));
        
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx[0] = (PLFLT)x1; fy[0] = (PLFLT)y1;
            fx[1] = (PLFLT)x2; fy[1] = (PLFLT)y2;
            cpgline(2, fx, fy);
        }
#endif
    }
} */

double GetAnyExp(double x, double *c, int nPar)
{
    double y;
    
    y = c[0] * exp(c[1] * (x-c[2]));
    
    return y;
}

void DrawAnyExp(GC gc, double *c, int nPar, double xleft, double xright)
{
    int n, nstep = NSTEPS;
    double x1, x2, dx, y1, y2;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx[2], fy[2];
#endif
    
    int xunit2x(), yunit2y();
    void draw_line();
    
    if (xleft == xright) return;
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.gauss);
    }
#endif
    
    dx = (xright - xleft)/(double)(nstep-1);
    
    for (n=0; n<nstep-1; n++) {
        x1 = xleft + (double)n * dx;
        x2 = x1 + dx;
        y1 = c[0] * exp(c[1] * (x1-c[2]));
        y2 = c[0] * exp(c[1] * (x2-c[2]));
        draw_line(gc, xunit2x(x1), yunit2y(y1), xunit2x(x2), yunit2y(y2));
        
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx[0] = (PLFLT)x1; fy[0] = (PLFLT)y1;
            fx[1] = (PLFLT)x2; fy[1] = (PLFLT)y2;
            cpgline(2, fx, fy);
        }
#endif
    }
}

double GetAnyValueFunc(double x, double *c, int nPar, double (*f)())
{
    double y;
    
    y = f(x, c, nPar);
    
    return y;
}

void DrawAnyValueFunc(GC gc, double *c, int nPar,
                      double xleft, double xright, double (*f)())
{
    int n, nstep = NSTEPS;
    double x1, x2, dx, y1, y2;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx[2], fy[2];
#endif
    
    int xunit2x(), yunit2y();
    void draw_line();
    
    if (xleft == xright) return;
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.gauss);
    }
#endif
    
    dx = (xright - xleft)/(double)(nstep-1);
    
    y1 = f(xleft, c, nPar);
    for (n=0; n<nstep-1; n++) {
        x1 = xleft + (double)n * dx;
        x2 = x1 + dx;
        y2 = f(x2, c, nPar);
        draw_line(gc, xunit2x(x1), yunit2y(y1), xunit2x(x2), yunit2y(y2));
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx[0] = (PLFLT)x1; fy[0] = (PLFLT)y1;
            fx[1] = (PLFLT)x2; fy[1] = (PLFLT)y2;
            cpgline(2, fx, fy);
        }
#endif
        y1 = y2;
    }
}

double GetAnyPoly(double x, double *a, int nP)
{
    int m, nPar;
    double y;
    
    if (nP < 0) {
        nPar = -nP;
    } else {
        nPar = nP;
    }
    
    y = a[0];
    for (m=1; m<nPar; m++) {
    	y += a[m] * pow(x, (double)m);
    }
    if (nP < 0 && y != 0.0) y = 1.0/y;
    
    return y;
}

double GetAnyGauss(double x, Gauss *g, double offset)
{
    double y;
    
    y = gauss(x, g->cen, g->amp, g->wid) + offset;
   
    return y;
}

void DrawAnyPoly(GC gc, double *a, int nP, double xleft, double xright)
{
    int n, m, nstep = NSTEPS, nPar;
    double x1, x2, dx, y1, y2;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx[2], fy[2];
#endif
    
    int xunit2x(), yunit2y();
    void draw_line();
    
    if (xleft == xright) return;
    
    if (nP < 0) {
        nPar = -nP;
    } else {
        nPar = nP;
    }
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.gauss);
    }
#endif
    
    dx = (xright - xleft)/(double)(nstep-1);
    
    for (n=0; n<nstep-1; n++) {
        x1 = xleft + (double)n * dx;
        x2 = x1 + dx;
        y1 = y2 = a[0];
        for (m=1; m<nPar; m++) {
            y1 += a[m] * pow(x1, (double)m);
            y2 += a[m] * pow(x2, (double)m);
        }
        if (nP < 0) {
            if (y1 != 0.0) y1 = 1/y1;
            if (y2 != 0.0) y2 = 1/y2;
        }
        draw_line(gc, xunit2x(x1), yunit2y(y1), xunit2x(x2), yunit2y(y2));
        
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx[0] = (PLFLT)x1; fy[0] = (PLFLT)y1;
            fx[1] = (PLFLT)x2; fy[1] = (PLFLT)y2;
            cpgline(2, fx, fy);
        }
#endif
    }
}

void DrawAnyGauss(GC gc, Gauss *g, double offset, double xleft, double xright)
{
    int n, nstep = NSTEPS;
    double x1, x2, dx, y1, y2;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx[2], fy[2];
#endif
    
    int xunit2x(), yunit2y();
    void draw_line();
    
    if (!g || xleft == xright) return;
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.gauss);
    }
#endif
    
    dx = (xright - xleft)/(double)(nstep-1);
    
    for (n=0; n<nstep-1; n++) {
        x1 = xleft + (double)n * dx;
        x2 = x1 + dx;
        y1 = gauss(x1, g->cen, g->amp, g->wid) + offset;
        y2 = gauss(x2, g->cen, g->amp, g->wid) + offset;
        draw_line(gc, xunit2x(x1), yunit2y(y1), xunit2x(x2), yunit2y(y2));
        
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx[0] = (PLFLT)x1; fy[0] = (PLFLT)y1;
            fx[1] = (PLFLT)x2; fy[1] = (PLFLT)y2;
            cpgline(2, fx, fy);
        }
#endif
    }
}

static void local_math_gaussian(char *cmd, Gauss *gPtr)
{
    int i, use_local_gaussian = 0;
    Gauss *g = NULL;
    scanPtr s;
    list curr = NULL;
    
    double gauss();
    
    if (!gPtr) {
        use_local_gaussian = 1;
    } else {
        g = gPtr;
    }
    if (vP->mode == SHOW_ALLSPE) {
        while ( (curr = scan_iterator(curr, vP->from)) ) {
            s = (scanPtr)DATA(curr);
            if (use_local_gaussian) {
                if (!s->gaussFit) continue;
                g = &(s->g);
            }
	        for (i=0; i<s->nChan; i++) {
                if (strcmp(cmd, "add")==0)
        	        s->d[i] += gauss((double)i, g->cen, g->amp, g->wid);
                else if (strcmp(cmd, "sub")==0)
        	        s->d[i] -= gauss((double)i, g->cen, g->amp, g->wid);
	        }
        }
    } else {
        s = vP->s;
        if (use_local_gaussian) {
            g = &(s->g);
            if (!s->gaussFit) return;
        }
	    for (i=0; i<s->nChan; i++) {
            if (strcmp(cmd, "add")==0)
                s->d[i] += gauss((double)i, g->cen, g->amp, g->wid);
            else if (strcmp(cmd, "sub")==0)
                s->d[i] -= gauss((double)i, g->cen, g->amp, g->wid);
	    }
    }
}

static void RemoveGaussian(int nfound)
{
    int n;

    if (nfound < 0 || nfound >= ngauss)
        return;

    for (n=ngauss-1; n>=nfound; n--) XtDestroyWidget(gw[n].form);
    for (n=nfound+1; n<ngauss; n++) {
        not_fitted[3*n-3] = not_fitted[3*n];
        not_fitted[3*n-2] = not_fitted[3*n+1];
        not_fitted[3*n-1] = not_fitted[3*n+2];
        sub_sel[n-1] = sub_sel[n];
        gaussar[n-1] = gaussar[n];
    }
    ngauss--;
    if (ngauss >= 0) {
        sub_sel[ngauss] = 0;
        not_fitted[3*ngauss] = 0;
        not_fitted[3*ngauss+1] = 0;
        not_fitted[3*ngauss+2] = 0;
    }
    for (n=nfound; n<ngauss; n++) make_each_gw(n);
#if XmVersion <= 1100
    XFlush(XtDisplay(gp->gaussTop));
#endif
}

void MathSelectedGaussian(Widget w, char *cmd, XtPointer call_data)
{
    int n, nsub = 0;
    string buf;
    
    void UpdateData();
    
    if (strcmp(cmd, "mapadd")==0) {
        local_math_gaussian("add", NULL);
    } else if (strcmp(cmd, "mapsub")==0) {
        local_math_gaussian("sub", NULL);
    } else if (!gp->gaussTop) {
        PostErrorDialog(w, "You must start the Gaussian viewer first.");
        return;
    }
    
    if (strcmp(cmd, "rem")==0) {
        while (1) {
            n = 0;
            while (n < ngauss && !sub_sel[n]) n++;
            if (n == ngauss) break;
            nsub++;
            RemoveGaussian(n);
        }
    } else {
        for (n=0; n<ngauss; n++) {
            if (sub_sel[n]) {
                local_math_gaussian(cmd, &gaussar[n]);
                nsub++;
            }
        }
    }
    
    if (strcmp(cmd, "add") == 0)
        sprintf(buf, "Added %d Gaussians to spectrum.\n", nsub);
    else if (strcmp(cmd, "sub") == 0)
        sprintf(buf, "Subtracted %d Gaussians from spectrum.\n", nsub);
    else if (strcmp(cmd, "mapadd") == 0)
        sprintf(buf, "Added Gaussians to all spectra.\n");
    else if (strcmp(cmd, "mapsub") == 0)
        sprintf(buf, "Subtracted Gaussians from all spectra.\n");
    else
        sprintf(buf, "Deleted %d Gaussians.\n", nsub);
        
    send_line(buf);
    
    UpdateData(SCALE_NONE, REDRAW);
}

void invert_gaussar()
{
    int n;
    
    if (ngauss <= 0) return;
    
    for (n=0; n<ngauss; n++)
        gaussar[n].cen = (double)(vP->s->nChan - 1) - gaussar[n].cen;
}

void gauss_reset(Widget w, char *cmd, XtPointer call_data)
{
    int n;
    list curr = NULL;
    scanPtr s;
    
    void draw_main();
    
    if (strcmp(cmd, "mapall")==0) {
        while ( (curr = scan_iterator(curr, vP->from)) ) {
            s = (scanPtr)DATA(curr);
            s->gaussFit = 0;
        }
        draw_main();
        return;
    }
    
    if (!gp->gaussTop) return;
    
    if (strncmp(cmd, "all", 3) == 0) {
        for (n=ngauss-1; n>=0; n--) {
            not_fitted[3*n]   = 0;
            not_fitted[3*n+1] = 0;
            not_fitted[3*n+2] = 0;
            sub_sel[n] = 0;
            XtDestroyWidget(gw[n].form);
        }
        ngauss = 0;
    } else if (strncmp(cmd, "cursor", 6) == 0) {
        if (rgauss_sel == 0) rgauss_sel = 1;
        return;
    } else {
        ngauss--;
        if (ngauss < 0) {
            ngauss = 0;
        } else {
            not_fitted[3*ngauss]   = 0;
            not_fitted[3*ngauss+1] = 0;
            not_fitted[3*ngauss+2] = 0;
            sub_sel[ngauss] = 0;
            XtDestroyWidget(gw[ngauss].form);
        }
    }
    update_gauss_data();
    draw_main();
}

void remove_gauss(int chan, double ampl)
{
    int n, nfound=-1;
    double arange, crange, c1, c2;
    double adist, cdist, dist, mindist=1.e10;

    void draw_main();
    double SpecUnitConv();

    if (ngauss <= 0) return;

    c1 = SpecUnitConv(UNIT_CHA, vP->xunit, vP->xleft);
    c2 = SpecUnitConv(UNIT_CHA, vP->xunit, vP->xright);
    crange = fabs(c2 - c1 + 1.0);
    arange = fabs(vP->yrange);
    for (n=0; n<ngauss; n++) {
        cdist = fabs(gaussar[n].cen - (double)chan)/crange;
        adist = fabs(gaussar[n].amp - ampl)/arange;
        dist = cdist + adist;
        if (dist < mindist) {
            mindist = dist;
            nfound = n;
        }
    }
    if (nfound < 0 || mindist >= 0.1) return;

    RemoveGaussian(nfound);

    update_gauss_data();
    draw_main();
}

static void find_initial_gauss_parameters(double *p, int np, scanPtr s)
{
    int n;
    double tmp;
    double xmin = xval[0];
    double xmax = xval[nGaussChan-1];
    
    if (xmin > xmax) {
        tmp = xmin; xmin = xmax; xmax = tmp;
    }
    
    p[0] = yval[0];
    for (n=0; n<nGaussChan; n++)
        if (yval[n] > p[0]) p[0] = yval[n];
    
    p[1] = (xmax - xmin)/4.0;
    if (s->mom.vcent >= xmin && s->mom.vcent <= xmax) {
        p[2] = s->mom.vcent;
    } else {
        p[2] = (xmin + xmax)/2.0;
    }
}

static int OutsideFittingRange(double *p, double *q, int np, scanPtr s)
{
    double tmp;
    double xmin = xval[0];
    double xmax = xval[nGaussChan-1];
    
    if (xmin > xmax) {
        tmp = xmin; xmin = xmax; xmax = tmp;
    }
    if (p[2] < xmin || p[2] > xmax) return 1;
    if (q[2] > xmax-xmin) return 1;
    if (fabs(p[0]) < 3.0*q[0]) return 1;
    if (fabs(p[1]) < 3.0*q[1]) return 1;
    
    return 0;
}

static void fitone(scanPtr s)
{
    int fit[3], err;
    double p[3], q[3], chi2;
    
    void set_gauss_data();
    void lm_Gauss3();
    
    set_gauss_data(s);
    p[0] = s->g.amp; fit[0] = 1;
    p[1] = s->g.wid; fit[1] = 1;
    p[2] = s->g.cen; fit[2] = 1;
    if (!s->gaussFit) find_initial_gauss_parameters(p, 3, s);
    err = Fitter1D(xval, yval, zval, nGaussChan,
                   p, fit, q, 3,
                   GAUSS_ITER, &chi2, lm_Gauss3);
    if (err || OutsideFittingRange(p, q, 3, s)) {
        s->gaussFit = 0;
        return;
    }
    s->g.amp = p[0];
    s->g.wid = p[1];
    s->g.cen = p[2];
    s->g.uamp = q[0];
    s->g.uwid = q[1];
    s->g.ucen = q[2];
    s->gaussFit = 1;
}

void FitAllGaussians(Widget w, char *cmd, XtPointer cd)
{
    list curr = NULL;
    
    void SetWatchCursor();
    
    SetWatchCursor(True);
    
    if (vP->mode == SHOW_SPE) {
        fitone(vP->s);
    } else {
        while ( (curr = scan_iterator(curr, vP->from)) ) {
            fitone((scanPtr)DATA(curr));
        }
    }
    
    SetWatchCursor(False);
    
    SetAnyToggle("gsum", 1);
}

static void set_gauss_strings(int n)
{
    int n1, n2, n3;
    double f0, rc=0.0, ra=1.0, rw=1.0, ufre;

    void wprintf();
    double SpecUnitConv();

    f0 = vP->s->freq0 + (double)(vP->s->nChan - 1)*vP->s->freqres/2.0;

    n1 = 3*n + 1;
    n2 = 3*n;
    n3 = 3*n + 2;
    
    if (gauss_mode == GAUSS_REL && n > 0) {
        rc = gaussar[0].cen;
        ra = gaussar[0].amp;
        if (ra == 0.0) ra = 1.0;
        rw = gaussar[0].wid/f0*SPEEDOFLIGHT*fabs(vP->s->freqres);
        if (rw == 0.0) rw = 1.0;
    }

    if (gauss_mode == GAUSS_ABS || n == 0) {
        if (!not_fitted[n1])
            wprintf(gw[n].fre_tw, "%f",
                    SpecUnitConv(vP->xunit, UNIT_CHA, gaussar[n].cen));
        if (!not_fitted[n2])
            wprintf(gw[n].amp_tw, "%f", gaussar[n].amp);
        if (!not_fitted[n3])
            wprintf(gw[n].wid_tw, "%f",
                    gaussar[n].wid/f0*SPEEDOFLIGHT*fabs(vP->s->freqres));
    } else {
        if (!not_fitted[n1])
            wprintf(gw[n].fre_tw, "%f",
                    SpecUnitConv(vP->xunit, UNIT_CHA, gaussar[n].cen) -
                    SpecUnitConv(vP->xunit, UNIT_CHA, rc));
        if (!not_fitted[n2])
            wprintf(gw[n].amp_tw, "%f", gaussar[n].amp/ra);
        if (!not_fitted[n3])
            wprintf(gw[n].wid_tw, "%f",
                    gaussar[n].wid/f0*SPEEDOFLIGHT*fabs(vP->s->freqres)/rw);
    }
    
    if (gauss_show_errors) {
        if (gauss_xunit == UNIT_FRE) {
            ufre = gaussar[n].ucen*fabs(vP->s->freqres);
        } else if (gauss_xunit == UNIT_FOFF) {
            ufre = gaussar[n].ucen*fabs(vP->s->freqres)*1000.0;
        } else if (gauss_xunit == UNIT_FMHZ) {
            ufre = gaussar[n].ucen*fabs(vP->s->freqres)*1000.0;
        } else if (gauss_xunit == UNIT_VEL) {
            ufre = gaussar[n].ucen*fabs(vP->s->velres);
        } else {
            ufre = gaussar[n].ucen;
        }    
        wprintf(gw[n].mol_tw, "%f", ufre);
        wprintf(gw[n].tra_tw, "%f", gaussar[n].uamp);
        wprintf(gw[n].ref_tw, "%f", gaussar[n].uwid*fabs(vP->s->velres));
    }
}

static void set_all_gauss_strings()
{
    int n;

    for (n=0; n<ngauss; n++)
        set_gauss_strings(n);
}

static void set_toggle_buttons(int n)
{
    int j;
    Widget w=NULL;

    void wprintf();

    for (j=0; j<3; j++) {
        if (j==0) w = gw[n].amp_tb;
        if (j==1) w = gw[n].fre_tb;
        if (j==2) w = gw[n].wid_tb;
        if (!w) continue;
        if (not_fitted[3*n + j]) {
            wprintf(w, "%s", "F");
        } else {
            wprintf(w, "%s", "I");
        }
    }

    if (sub_sel[n])
        wprintf(gw[n].sub_tb, "%s", "S");
    else
        wprintf(gw[n].sub_tb, " ");
}

static void get_all_gauss_strings()
{
    int n;
    double val, f0, rc=0.0, ra=1.0, rw=1.0;

    double SpecUnitConv();
    void wdscanf();

    f0 = vP->s->freq0 + (double)(vP->s->nChan - 1)*vP->s->freqres/2.0;

    for (n=0; n<ngauss; n++) {
        if (gauss_mode == GAUSS_ABS || n == 0) {
            wdscanf(gw[n].fre_tw, &val);
            gaussar[n].cen = SpecUnitConv(UNIT_CHA, vP->xunit, val);
            if (n == 0) rc = val;
            wdscanf(gw[n].amp_tw, &val);
            gaussar[n].amp = val;
            if (n == 0) ra = val;
            wdscanf(gw[n].wid_tw, &val);
            gaussar[n].wid = val*f0/(SPEEDOFLIGHT * fabs(vP->s->freqres));
            if (n == 0) rw = gaussar[0].wid;
        } else {
            wdscanf(gw[n].fre_tw, &val);
            gaussar[n].cen = SpecUnitConv(UNIT_CHA, vP->xunit, val + rc);
            wdscanf(gw[n].amp_tw, &val);
            gaussar[n].amp = val * ra;
            wdscanf(gw[n].wid_tw, &val);
            gaussar[n].wid = val * rw; 
        }
    }
}

void do_fit(Widget w, char *cmd, XtPointer cd)
{
    int err;
    double chisq;
    string tbuf;
    
    void (*f)();

    void lm_AbsGauss(), lm_RelGauss();
    void wprintf(), SetWatchCursor();
    
    if (!gp->gaussTop) {
        PostErrorDialog(w, "You must start the Gaussian viewer first.");
        return;
    }

    if (ngauss <= 0) {
        PostErrorDialog(w, "No Gaussian curves have been specified.");
        return;
    }
    
    if (gauss_mode == GAUSS_ABS)
        f = lm_AbsGauss;
    else if (gauss_mode == GAUSS_REL)
        f = lm_RelGauss;
    else {
        PostErrorDialog(w, "Internal error: Unknown Gaussian fitting mode.");
        return;
    }

    SetWatchCursor(True);

    get_all_gauss_strings();

    assign_parameters();
    assign_lista();
    
    wprintf(gp->TGauss[2], "");
    
    err = Fitter1D(xval, yval, zval, nGaussChan,
                   parameters, lista, par_errors, nopar,
                   GAUSS_ITER, &chisq, f);
    if (err != 0) {
        SetWatchCursor(False);
        sprintf(tbuf, "No convergence for the Gaussian fitting.\n\n\
Detected in Fitter1D() (error=%d).\n", err);
        PostWarningDialog(w, tbuf);
        return;
    }
    
    wprintf(gp->TGauss[2], "Chi^2: %11.4e", chisq/(double)nGaussChan);
    assign_gaussar();

    set_all_gauss_strings();
    SetWatchCursor(False);

    SetAnyToggle("gsum", 0);
    UnsetAnyToggle("gind", 1);
}

void new_gaussian(Widget w, char *cmd, XtPointer cd)
{
    void GaussViewer();
    
    if (ngauss < MAXGAUSS) {
        if (!gp->gaussTop) GaussViewer(NULL, "open", NULL);
        gauss_xunit = vP->xunit;
        gaussar[ngauss] = gau;
        not_fitted[3*ngauss]   = 0;
        not_fitted[3*ngauss+1] = 0;
        not_fitted[3*ngauss+2] = 0;
        sub_sel[ngauss] = 0;
        ngauss++;
        make_each_gw(ngauss-1);
        update_gauss_data();
        if (strcmp(cmd, "draw") == 0) {
            UnsetAnyToggle("gsum", 0);
            SetAnyToggle("gind", 1);
        }
#if XmVersion < 1200
        XFlush(XtDisplay(gp->gaussTop));
#endif
    } else {
        PostErrorDialog(w, "Too many Gaussian curves!");
    }
}

int SaveGaussFile(char *fname, char *type)
{
    int n;
    string buf;
    double val, fre, ufre, amp, uamp, wid, uwid;
    FILE *fp;
    
    void wdscanf();
    char *GetStringFromUnit();
    
    if (!gp->gaussTop) {
        PostErrorDialog(NULL, "You must start the Gaussian viewer first.");
        return 1;
    }

    if (ngauss <= 0 || !fname || !type) {
        PostErrorDialog(NULL, "No Gaussian curves to save!");
        return 1;
    }

    if (strcmp(type, "a") == 0) {
        if ((fp = fopen(fname, type)) == NULL) {
            sprintf(buf, "Unable to open %s for appending.\n", fname);
            PostWarningDialog(NULL, buf);
            return 1;
        }
    } else {
        if ((fp = fopen(fname, "w")) == NULL) {
            sprintf(buf, "Unable to open %s for writing.\n", fname);
            PostWarningDialog(NULL, buf);
            return 1;
        }
    }
    fprintf(fp, "UNIT: %s\n", GetStringFromUnit(gauss_xunit));
    sprintf(buf, "Writing %d Gaussians to '%s' (%s).\n", ngauss,
            fname, GetStringFromUnit(gauss_xunit));
    send_line(buf);
    for (n=0; n<ngauss; n++) {
        wdscanf(gw[n].fre_tw, &val);
        fre  = val;
        if (gauss_xunit == UNIT_FRE) {
            ufre = gaussar[n].ucen*fabs(vP->s->freqres);
        } else if (gauss_xunit == UNIT_FOFF) {
            ufre = gaussar[n].ucen*fabs(vP->s->freqres)*1000.0;
        } else if (gauss_xunit == UNIT_FMHZ) {
            ufre = gaussar[n].ucen*fabs(vP->s->freqres)*1000.0;
        } else if (gauss_xunit == UNIT_VEL) {
            ufre = gaussar[n].ucen*fabs(vP->s->velres);
        } else {
            ufre = gaussar[n].ucen;
        }
        wdscanf(gw[n].amp_tw, &val);
        amp  = val;
        uamp = gaussar[n].uamp;
        wdscanf(gw[n].wid_tw, &val);
        if (gauss_xunit == UNIT_FRE) {
            wid = val;
            uwid = gaussar[n].uwid*fabs(vP->s->velres);
        } else if (gauss_xunit == UNIT_FOFF) {
            wid = val;
            uwid = gaussar[n].uwid*fabs(vP->s->velres);
        } else if (gauss_xunit == UNIT_FMHZ) {
            wid = val;
            uwid = gaussar[n].uwid*fabs(vP->s->velres);
        } else if (gauss_xunit == UNIT_VEL) {
            wid  = val;
            uwid = gaussar[n].uwid*fabs(vP->s->velres);
        } else {
            wid  = val / fabs(vP->s->velres);
            uwid = gaussar[n].uwid;
        }
        sprintf(buf, "%2d  %f(%f)  %f(%f)  %f(%f)\n", n+1,
                fre, ufre, amp, uamp, wid, uwid);
        send_line(buf);
        fprintf(fp, "%f (%f)  %f (%f)  %f (%f)\n", 
                fre, ufre, amp, uamp, wid, uwid);
    }
    fclose(fp);
    
    return 0;
}

int ReadGaussFile(char *fname)
{
    int n, saved_xunit=UNIT_FRE;
    string foo;
    char buf[MAXBUFSIZE];
    FILE *fp;
    double fre, ufre, amp, uamp, wid, uwid;
    
    double SpecUnitConv();
    int GetUnitFromString();
    char *GetStringFromUnit();
    
    fp = fopen(fname, "r");
    if (!fp) {
        sprintf(foo, "Couldn't open Gauss file '%s'.", fname);
        PostErrorDialog(NULL, foo);
        return 1;
    }
    
    n = 0;
    sprintf(foo, "Loading Gaussians from file '%s'...\n", fname);
    send_line(foo);
    while (fgets(buf, MAXBUFSIZE, fp) != NULL) {
        if (buf[0] == '%' || buf[0] == '!' || buf[0] == '#') continue;
        if (strncmp(buf, "UNIT:", 5) == 0) {
            if (sscanf(buf, "UNIT: %s", foo) == 1)
                saved_xunit = GetUnitFromString(foo);
            continue;
        }
        sscanf(buf, "%lf (%lf)  %lf (%lf)  %lf (%lf)", 
               &fre, &ufre, &amp, &uamp, &wid, &uwid);
        if (ngauss >= MAXGAUSS) {
            PostWarningDialog(NULL, "Couldn't load all Gaussians.");
            break;
        }
        gau.amp  = amp;
        gau.uamp = uamp;
        switch (saved_xunit) {
            case UNIT_FRE:
                gau.cen  = SpecUnitConv(UNIT_CHA, UNIT_FRE, fre);
                gau.ucen = ufre / fabs(vP->s->freqres);
                gau.wid  = wid  / fabs(vP->s->velres);
                gau.uwid = uwid / fabs(vP->s->velres);
                break;
            case UNIT_FOFF:
                gau.cen  = SpecUnitConv(UNIT_CHA, UNIT_FOFF, fre);
                gau.ucen = ufre / fabs(vP->s->freqres) / 1000.0;
                gau.wid  = wid  / fabs(vP->s->velres);
                gau.uwid = uwid / fabs(vP->s->velres);
                break;
            case UNIT_FMHZ:
                gau.cen  = SpecUnitConv(UNIT_CHA, UNIT_FMHZ, fre);
                gau.ucen = ufre / fabs(vP->s->freqres) / 1000.0;
                gau.wid  = wid  / fabs(vP->s->velres);
                gau.uwid = uwid / fabs(vP->s->velres);
                break;
            case UNIT_VEL:
                gau.cen  = SpecUnitConv(UNIT_CHA, UNIT_VEL, fre);
                gau.ucen = ufre / fabs(vP->s->velres);
                gau.wid  = wid  / fabs(vP->s->velres);
                gau.uwid = uwid / fabs(vP->s->velres);
                break;
            case UNIT_CHA:
                gau.cen  = fre;
                gau.ucen = ufre;
                gau.wid  = wid;
                gau.uwid = uwid;
                break;
        }
        new_gaussian(NULL, "nodraw", NULL);
        n++;
    }
    fclose(fp);
    if (n > 0) {
        sprintf(foo, "Added %d Gaussians.\n", n);
        send_line(foo);
        UnsetAnyToggle("gind", 0);
        SetAnyToggle("gsum", 1);
    } else {
        sprintf(foo, "No Gaussians added.\n");
        send_line(foo);
    }
    
    return 0;
}

void GaussiansToMarkers(Widget w, char *cmd, XtPointer call_data)
{
    int n;
    double val;

    void wdscanf(), wsscanf();
    
    if (!gp->gaussTop) {
        PostErrorDialog(w, "You must start the Gaussian viewer first.");
        return;
    }

    if (ngauss <= 0) {
        PostErrorDialog(w, "No Gaussian curves at all!");
        return;
    }

    if (ngauss + nmark >= MAXMARK) return;
    mark_xunit = gauss_xunit;
    for (n=0; n<ngauss; n++) {
        marks[n+nmark].mode = 0;
        wdscanf(gw[n].fre_tw, &val);
        marks[n+nmark].x = val;
        wdscanf(gw[n].amp_tw, &val);
        marks[n+nmark].y = 1.05*val;
        marks[n+nmark].type = MARK_TYPE_ARROW;
        marks[n+nmark].dir  = MARK_DIR_DOWN;
        marks[n+nmark].xlength = 20;
        marks[n+nmark].ylength = 20;
        marks[n+nmark].align = 0.5;
        marks[n+nmark].tagged = 0;
        wsscanf(gw[n].mol_tw, marks[n+nmark].label);
    }
    nmark += ngauss;
    SetAnyToggle("markers", 1);
}

int ViewGaussFile(char *fname, int sort)
{
    string buf, foo;
    
    char *GetTmpFile();
    void XS_system();

    if (sort) {
        strcpy(foo, GetTmpFile("gauss"));
        sprintf(buf, "%s %s > %s", pP->unixSortCmd, fname, foo);
        XS_system(buf, 1);
        sprintf(buf, "%s %s %s", pP->unixMvCmd, foo, fname);
        XS_system(buf, 1);
    }
    sprintf(buf, "%s %s &", pP->editor, fname);
    XS_system(buf, 1);
    
    return 0;
}

static void destroy_gaussian_viewer(Widget w, Widget g, XmAnyCallbackStruct *cb)
{
    if (g) {
        gauss_reset(w, "all", NULL);
        w_xunit = NULL;
        gp->gaussTop = NULL;
    }
}

static Widget make_gauss_sw(Widget on_top)
{
    int n;
    Dimension p_x, p_y, p_w, p_h, o_w, o_h;
    Arg wargs[10];
    string buf;
    Widget g, scroll_w;
    COLOR *c;
    static Pixmap iconPixmap = 0;

    COLOR *GetColorInfo();

    XtVaGetValues(gp->top, XmNx, &p_x, XmNy, &p_y, XmNwidth, &p_w,
                  XmNheight, &p_h, NULL);
    XtVaGetValues(on_top, XmNwidth, &o_w, XmNheight, &o_h, NULL);

    sprintf(buf, "%s Gaussian Fits", PKGNAME);
    
    c = GetColorInfo();
    
    n = 0;
    XtSetArg(wargs[n], XmNwidth,     o_w); n++;
    XtSetArg(wargs[n], XmNheight,    gau_h); n++;
    XtSetArg(wargs[n], XmNtitle,     buf); n++;
    XtSetArg(wargs[n], XmNiconName,  "Gauss"); n++;
    XtSetArg(wargs[n], XmNx,         p_x); n++;
    XtSetArg(wargs[n], XmNy,         p_y + p_h + 20); n++;
    if (gp->privateColors) {
        XtSetArg(wargs[n], XmNcolormap, c->cmap); n++;
    }

    g = XtAppCreateShell("Gauss", "", topLevelShellWidgetClass,
                         XtDisplay(gp->top), wargs, n);
    if (iconPixmap == 0) {
        iconPixmap = XmGetPixmap(XtScreen(g), pP->gauss_xpm,
                                 c->black, c->white);
    }
    XtVaSetValues(g, XmNiconPixmap, iconPixmap, NULL);
    n = 0;
    XtSetArg(wargs[n], XmNscrollingPolicy, XmAUTOMATIC); n++;
    scroll_w = XtCreateManagedWidget("scroller", xmScrolledWindowWidgetClass,
                                     g, wargs, n);

    n = 0;
    XtSetArg(wargs[n], XmNorientation,    XmVERTICAL); n++;
    /* XtSetArg(wargs[n], XmNpacking,        XmPACK_TIGHT); n++; 
    XtSetArg(wargs[n], XmNnumColumns,     1); n++;*/
    XtSetArg(wargs[n], XmNmarginHeight,   0); n++;
    XtSetArg(wargs[n], XmNmarginWidth,    0); n++;
    gauss_rc = XtCreateManagedWidget("rowcol", xmRowColumnWidgetClass,
                                     scroll_w, wargs, n);
    XtAddCallback(g, XmNdestroyCallback,
                  (XtCallbackProc)destroy_gaussian_viewer, g);

    make_each_gw(-1);

    XtRealizeWidget(g);

    return g;
}

void GaussViewer(Widget w, char *cmd, XtPointer call_data)
{
    if (strcmp(cmd, "open")==0) {
        if (gp->gaussTop) return;
        gp->gaussTop = make_gauss_sw(gp->form);
    } else if (strcmp(cmd, "close")==0) {
        if (!gp->gaussTop) return;

        if (ngauss > 0 && !PostQuestionDialog(w,
"There are fitted Gaussians.\n\
Are you sure you want to close the viewer?")) return;

        gauss_reset(w, "all", NULL);
        XtDestroyWidget(gp->gaussTop);
        w_xunit = NULL;
        gp->gaussTop = NULL;
    }
}

static void toggle_button_cb(Widget w, int *n,
                             XmToggleButtonCallbackStruct *cb)
{
    void wprintf();

    *n = cb->set;

    if (*n) {
        wprintf(w, "%s", "F");
    } else {
        wprintf(w, "%s", "I");
    }
}

static void toggle_sub_cb(Widget w, int *n,
                          XmToggleButtonCallbackStruct *cb)
{
    void wprintf();

    *n = cb->set;

    if (*n) {
        wprintf(w, "%s", "S");
    } else {
        wprintf(w, "%s", " ");
    }
}

static void create_parameter_widget(int m, Widget rc,
                                    Widget *tb, Widget *tw, Widget *lw)
{
    Widget f, h;
    
    f = XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
				                 rc, XmNshadowType, XmSHADOW_OUT, NULL);
    h = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, f,
                          XmNorientation, XmHORIZONTAL,
                          XmNpacking, XmPACK_TIGHT,
                          XmNmarginHeight, 0,
                          XmNmarginWidth, 0,
                          NULL);
    *tb = XtVaCreateManagedWidget("F", xmToggleButtonWidgetClass, h,
                                  XmNset, not_fitted[m] ? True : False,
                                  XmNrecomputeSize, False,
                                  NULL);
    *tw = XtVaCreateManagedWidget("text", xmTextWidgetClass, h,
                                  XmNwidth, 100,
                                  XmNrecomputeSize, False,
                                  NULL);
    *lw = XtVaCreateManagedWidget("", xmLabelWidgetClass, h,
                                  XmNwidth, 100,
                                  NULL);
}

static void make_each_gw(int n)
{
    int j;
    string str;
    Widget rc;

    if (n+1 > MAXGAUSS) return;

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, gauss_rc,
                          XmNorientation, XmHORIZONTAL,
                          XmNpacking, XmPACK_TIGHT,
                          XmNmarginHeight, 0,
                          XmNmarginWidth, 0,
                          NULL);

    if (n == -1) {
        j = 0;
        while (GaussLabel[j]) {
            if (j == 2) {
                w_xunit = XtCreateManagedWidget(XUnitLabel[gauss_xunit],
                                                xmLabelWidgetClass,
                                                rc, NULL, 0);
            } else {
                XtCreateManagedWidget(GaussLabel[j],
                                      xmLabelWidgetClass,
                                      rc, NULL, 0);
            }
            j++;
        }
        return;
    }

    gw[n].form = rc;
    sprintf(str, "%2d", n+1);
    gw[n].num_lw = XtVaCreateManagedWidget(str, xmLabelWidgetClass,
                                  rc, NULL);
    gw[n].sub_tb = XtVaCreateManagedWidget("S", xmToggleButtonWidgetClass,
                                  rc, XmNset,
                                  sub_sel[n] ? True : False,
                                  XmNrecomputeSize, False,
                                  NULL);
    create_parameter_widget(3*n+1, rc, &(gw[n].fre_tb), &(gw[n].fre_tw),
                            &(gw[n].mol_tw));
    create_parameter_widget(3*n  , rc, &(gw[n].amp_tb), &(gw[n].amp_tw),
                            &(gw[n].tra_tw));
    create_parameter_widget(3*n+2, rc, &(gw[n].wid_tb), &(gw[n].wid_tw),
                            &(gw[n].ref_tw));

    XtAddCallback(gw[n].sub_tb, XmNvalueChangedCallback,
                  (XtCallbackProc)toggle_sub_cb, &sub_sel[n]);
    XtAddCallback(gw[n].fre_tb, XmNvalueChangedCallback,
                  (XtCallbackProc)toggle_button_cb, &not_fitted[3*n+1]);
    XtAddCallback(gw[n].amp_tb, XmNvalueChangedCallback,
                  (XtCallbackProc)toggle_button_cb, &not_fitted[3*n]);
    XtAddCallback(gw[n].wid_tb, XmNvalueChangedCallback,
                  (XtCallbackProc)toggle_button_cb, &not_fitted[3*n+2]);

    set_gauss_strings(n);
    set_toggle_buttons(n);
}

static void ChangeModeInGaussar(int newMode)
{
    int n;
    double rc, ra, rw, val;
    
    double SpecUnitConv();
    void wdscanf(), wprintf();
    
    if (ngauss < 2) return;
    
    wdscanf(gw[0].fre_tw, &rc);
    wdscanf(gw[0].amp_tw, &ra);
    wdscanf(gw[0].wid_tw, &rw);
    
    if (newMode == GAUSS_ABS) {
        for (n=1; n<ngauss; n++) {
            wdscanf(gw[n].fre_tw, &val);
            wprintf(gw[n].fre_tw, "%f", rc+val);
            wdscanf(gw[n].amp_tw, &val);
            wprintf(gw[n].amp_tw, "%f", val*ra);
            wdscanf(gw[n].wid_tw, &val);
            wprintf(gw[n].wid_tw, "%f", val*rw);
        }
    } else {
        if (ra == 0.0) ra = 1.0;
        if (rw == 0.0) rw = 1.0;
        for (n=1; n<ngauss; n++) {
            wdscanf(gw[n].fre_tw, &val);
            wprintf(gw[n].fre_tw, "%f", val-rc);
            wdscanf(gw[n].amp_tw, &val);
            wprintf(gw[n].amp_tw, "%f", val/ra);
            wdscanf(gw[n].wid_tw, &val);
            wprintf(gw[n].wid_tw, "%f", val/rw);
        }
    }
}

static void GaussModeCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    int n = atoi(s);
    
    if (n != gauss_mode) {
        gauss_mode = n;
        ChangeModeInGaussar(n);
        update_gauss_data();
    }
}

static void GaussErrorCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    int i, n = atoi(s);
    
    void wprintf();
    
    if (n != gauss_show_errors) {
        gauss_show_errors = n;
        if (n) {
            set_all_gauss_strings();
        } else {
            for (i=0; i<ngauss; i++) {
                wprintf(gw[i].mol_tw, "");
                wprintf(gw[i].tra_tw, "");
                wprintf(gw[i].ref_tw, "");
            }
        }
    }
}

void GaussModeDialog(Widget wid, char *cmd, XtPointer call_data)
{
    Widget rc, menu, menuE;
    Widget w = wid;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Gaussian fitting mode",
             NULL, NULL, NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, GaussMode_Help, 0, NULL);
    
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          NULL);

    menu = CreateOptionMenu(rc, &GaussModeMenu);
    SetDefaultOptionMenuItem(menu, gauss_mode);

    menuE = CreateOptionMenu(rc, &GaussErrorMenu);
    SetDefaultOptionMenuItem(menuE, gauss_show_errors);
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(menu);
    XtManageChild(menuE);
    XtManageChild(rc);
    
    ManageDialogCenteredOnPointer(sf->form);
}

void ChangeUnitInGaussar(int newUnit)
{
    int n;
    double val, rc_old=0.0, rc_new=0.0, ufre=0.0;
    
    double SpecUnitConv();
    void wdscanf(), wprintf();
    
    if (newUnit == gauss_xunit)
        return;
    
    for (n=0; n<ngauss; n++) {
        wdscanf(gw[n].fre_tw, &val);
        if (gauss_mode == GAUSS_ABS || n == 0) {
            if (n == 0) rc_old = val;
            val = SpecUnitConv(newUnit, gauss_xunit, val);
            if (n == 0) rc_new = val;
        } else {
            val = SpecUnitConv(newUnit, gauss_xunit, rc_old + val);
            val = val - rc_new;
        }
        wprintf(gw[n].fre_tw, "%f", val);
        if (gauss_show_errors) {
            if (newUnit == UNIT_FRE) {
                ufre = gaussar[n].ucen*fabs(vP->s->freqres);
            } else if (newUnit == UNIT_FMHZ) {
                ufre = gaussar[n].ucen*fabs(vP->s->freqres)*1000.0;
            } else if (newUnit == UNIT_FOFF) {
                ufre = gaussar[n].ucen*fabs(vP->s->freqres)*1000.0;
            } else if (newUnit == UNIT_VEL) {
                ufre = gaussar[n].ucen*fabs(vP->s->velres);
            } else {
                ufre = gaussar[n].ucen;
            }
            wprintf(gw[n].mol_tw, "%g", ufre);
        }
    }
    gauss_xunit = newUnit;
    if (w_xunit)
        wprintf(w_xunit, "%s", XUnitLabel[gauss_xunit]);
}

static void UpdateGaussEditWidgets(StdForm *sf)
{
    double v1, v2;
    GaussEdit *ge = (GaussEdit *)sf->user;
    
    void wprintf();
    double SpecUnitConv();
    
    if (!ge) return;
    
    wprintf(sf->edit[0], "%f", ge->g.amp);
    v1 = SpecUnitConv(UNIT_VEL, UNIT_CHA, ge->g.cen - ge->g.wid/2.0);
    v2 = SpecUnitConv(UNIT_VEL, UNIT_CHA, ge->g.cen + ge->g.wid/2.0);
    wprintf(sf->edit[1], "%f", fabs(v1 - v2));
    wprintf(sf->edit[2], "%f", SpecUnitConv(UNIT_VEL, UNIT_CHA, ge->g.cen));
}

static void GetNewGaussPars(Widget wid, StdForm *sf, XmAnyCallbackStruct *cb)
{
    double a, w, c, c1, c2;
    GaussEdit *ge = (GaussEdit *)sf->user;
    
    void wdscanf(), wprintf();
    double SpecUnitConv();
    
    if (!ge) return;
    
    wdscanf(sf->edit[0], &a);
    wdscanf(sf->edit[1], &w);
    wdscanf(sf->edit[2], &c);
    
    ge->g.amp = a;
    ge->g.cen = SpecUnitConv(UNIT_CHA, UNIT_VEL, c);
    
    c1 = SpecUnitConv(UNIT_CHA, UNIT_VEL, c - w/2.0);
    c2 = SpecUnitConv(UNIT_CHA, UNIT_VEL, c + w/2.0);
    ge->g.wid = fabs(c2 - c1);
    
    if (ge->s) ge->s->g = ge->g;
    
    FitAllGaussians(NULL, NULL, NULL);
    
    if (ge->s) ge->g = ge->s->g;
    
    if (ge->s->gaussFit) {
        XtVaSetValues(ge->w, XmNset, True, NULL);
        wprintf(ge->w, "%s", "Valid fit");
    } else {
        XtVaSetValues(ge->w, XmNset, False, NULL);
        wprintf(ge->w, "%s", "No fit");
    }
    
    UpdateGaussEditWidgets(sf);
}

static void toggle_gaussFit(Widget w, int *n,
                            XmToggleButtonCallbackStruct *cb)
{
    void wprintf();

    *n = cb->set;

    if (*n) {
        wprintf(w, "%s", "Valid fit");
    } else {
        wprintf(w, "%s", "No fit");
    }
}

static void cb_useDrawnGauss(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    GaussEdit *ge = (GaussEdit *)sf->user;
    
    if (!ge) return;
    
    ge->g = gau;
    
    UpdateGaussEditWidgets(sf);
}

void PostEditGaussDialog(scanPtr s)
{
    int n;
    Widget fr, rc, rc2, rc3, tb, pb;
    StdForm *sf;
    GaussEdit *ge;
    
    char *GetSpeDesc(scanPtr, int);
    
    if (!s) return;
    
    ge = (GaussEdit *) XtMalloc(sizeof(GaussEdit));
    if (!ge) return;
    
    ge->g = s->g;
    ge->s = s;
    
    sf = PostStdFormDialog(gp->top, "Edit Gaussian fit parameters",
             BUTT_APPLY, (XtCallbackProc)GetNewGaussPars, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 3, NULL);

    sf->user = (XtPointer)ge;
        
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    XtVaCreateManagedWidget(GetSpeDesc(s, 1), xmLabelWidgetClass, rc,
                            XmNfontList, gp->flist10,
                            NULL);
    
    fr  = XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
				                  rc, XmNshadowType, XmSHADOW_OUT, NULL);
	                          
    rc2 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                  XmNorientation, XmHORIZONTAL,
                                  XmNnumColumns, 3,
                                  XmNadjustLast, False,
                                  XmNpacking, XmPACK_COLUMN,
                                  NULL);
    for (n=0; n<3; n++) {
        XtCreateManagedWidget(edit_gauss_labs[n], xmLabelWidgetClass,
                              rc2, NULL, 0);
        sf->edit[n] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                            rc2, NULL, 0);
    }
    rc3 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                  XmNorientation, XmHORIZONTAL,
                                  NULL);
    tb = XtVaCreateManagedWidget(s->gaussFit ? "Valid fit" : "No fit",
                                 xmToggleButtonWidgetClass, rc3,
                                 XmNset, s->gaussFit ? True : False,
                                 /* XmNrecomputeSize, False, */
                                 NULL);
    XtAddCallback(tb, XmNvalueChangedCallback,
                  (XtCallbackProc)toggle_gaussFit, &(s->gaussFit));
    ge->w = tb;
    pb = XtVaCreateManagedWidget("Use values from drawn Gaussian",
                                 xmPushButtonWidgetClass, rc3, NULL);
    XtAddCallback(pb, XmNactivateCallback,
                  (XtCallbackProc)cb_useDrawnGauss, sf);
    
    ArrangeStdFormDialog(sf, rc);
    
    UpdateGaussEditWidgets(sf);
    
    ManageDialogCenteredOnPointer(sf->form);
}
