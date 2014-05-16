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
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>

#include "list.h"
#include "defines.h"
#include "global_structs.h"
#include "menus.h"
#include "dialogs.h"

#define CONVOLVE_LIM 3.0
#define REGRID_LIM   3.0

/*** External variables ***/
extern VIEW   *vP;
extern DRAW    draw;

void   PostErrorDialog(Widget, char *);
Widget PostWaitingDialog(Widget, char *, Widget *, int);
void   SetWaitingScale(Widget, int);
void   ManageDialogCenteredOnPointer(Widget);
Widget ThreeHorEdit(Widget, Widget *, Widget *, Widget *);
Widget CreateOptionMenu(Widget, MenuBarItem *);
void   SetDefaultOptionMenuItem(Widget, int);
void   draw_main();

int        count_scans(DataSetPtr);
list       scan_iterator(list, DataSetPtr);
scanPtr    copy_scanheader(DataSetPtr, int, scanPtr);
list      *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);
void       DeleteLastDataSet();

/*** Local variables ***/
typedef struct {
    MAP *m;
    int regrid, jansky, spectral;
} ConvStruct;

typedef struct {
    double v1, v2;
} TWODOUBLES;

typedef struct {
  double R, Ve, Vs, Width, Frac;
  double Phi1, Phi2, dPhi, dx;
  double Theta1, Theta2, dTheta, dy;
} PROJCUBE;

static Beam convBeam;
static PROJCUBE projCube;
static int PixelDist=10, CorrNorm=1, CorrXUnit=0;

static void SetCorrNormCallback();
static MenuItem CorrNormData[] = {
   {"No", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetCorrNormCallback, "0", NULL},
   {"Yes", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetCorrNormCallback, "1", NULL},
EOI};
static MenuBarItem CorrNormMenu = {
   "Normalize correlation function?", ' ', True, CorrNormData
};

static void SetCorrUnitCallback();
static MenuItem CorrUnitData[] = {
   {"Pixel", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetCorrUnitCallback, "0", NULL},
   {"Arcsec", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetCorrUnitCallback, "1", NULL},
EOI};
static MenuBarItem CorrUnitMenu = {
   "Unit on x-axis:", ' ', True, CorrUnitData
};

static char *ConvHelp_1 = "\
                            Regrid map help\n\
                            ---------------\n\
Description:\n\
To regrid unevenly spaced map data onto a rectangular grid. The algoritm used\n\
in the regridding is based on interpolation of map data using a elliptical (or\n\
circular) Gaussian. The values at each new grid point are given by a normalized\n\
convolution of the old data points. Thus, the regridded data will be more or\n\
less smoothed depending on the size of the elliptical Gaussian as compared to\n\
the observed resolution (i.e. beam size) as well as the area coverage of the\n\
unevenly spaced observed points.\n\n\
Usage:\n\
  1. A map should be selected from list at top.\n\
  2. In the field below the list a new name can be supplied (the default name\n\
     is 'Regridded <old name>').\n\
  3. Specify the elliptical Gaussian used in the interpolation in the fields\n\
     for Major and minor axes, and position angle.\n\
  4. Specify the extent and grid spacing of the new rectangular map in the\n\
     remaining fields.\
";
static char *ConvHelp_2 = "\
                           Regrid spectra help\n\
                           -------------------\n\
Description:\n\
To regrid unevenly spaced spectra onto a rectangular grid. The algoritm used\n\
in the regridding is based on interpolation of spectral values using a\n\
elliptical (or circular) Gaussian. The spectra at each new grid point are given\n\
by a normalized convolution of the old spectra values. Thus, the regridded data\n\
will be more or less smoothed depending on the size of the elliptical Gaussian\n\
as compared to the observed resolution (i.e. beam size) as well as the area\n\
coverage of the unevenly spaced observed spectra.\n\
The task will use the currently read spectra as input data. Note that these\n\
will be replaced by the regridded spectra. The regridding is quite time\n\
consuming since it will be done for each channel.\n\n\
Usage:\n\
  1. Specify the elliptical Gaussian used in the interpolation in the fields\n\
     for Major and minor axes, and position angle.\n\
  2. Specify the extent and grid spacing of the new rectangular map in the\n\
     remaining fields.\
";
static char *ConvHelp_3 = "\
                            Convolve map help\n\
                            -----------------\n\
Description:\n\
To preform a Gaussian convolution of unevenly spaced map data.\n\n\
Usage:\n\
  1. A map should be selected from list at top.\n\
  2. In the field below the list a new name can be supplied (the default name\n\
     is 'Convolved <old name>').\n\
  3. Specify the elliptical Gaussian used in the convolution in the fields for\n\
     Major axis, Minor axis, and position angle.\n\
  4. Specify the extent and grid spacing of the convolved rectangular map in\n\
     the remaining fields.\
";
static char *ConvHelp_4 = "\
                          Convolve spectra help\n\
                          ---------------------\n\
Description:\n\
To preform a Gaussian convolution of unevenly spaced spectra.\n\
The task will use the currently read spectra as input data. Note that these\n\
will be replaced by the convolved spectra. The convolving is quite time\n\
consuming since it will be done for each channel.\n\n\
Usage:\n\
  1. Specify the elliptical Gaussian used in the convolution in the fields\n\
     for Major and minor axes, and position angle, respectively.\n\
  2. Specify the extent and grid spacing of the new rectangular map in the\n\
     remaining fields.\
";
static char *Scale_Help = "\
                             Scale map help\n\
                             --------------\n\
Description:\n\
To linearly scale axes and data.\n\
The task will use the selected map as input data. The map is selected by\n\
highlighting a map entry in the list at the top of the dialog.\n\n\
Usage:\n\
    The first two entries are to be used to shift the map centre to (x0,x0).\n\
    Moreover, entries 3 and 4 are used to scale the axes with Sx, Sy, and in\n\
    the fifth entry a position angle, PA [degrees], can be entered to rotate\n\
    the map. The entire transformation can be written as\n\
        x' = Sx (x - x0) cos(PA) - Sy (y - y0) sin(PA)\n\
        y' = Sy (y - y0) cos(PA) + Sx (x - x0) sin(PA)\n\n\
    The two last entries are used to scale the map data according to\n\
        z' = c0 + c1 z\n\
    In order to preserve the S/N-ratio the error map will be scaled as\n\
        e' = c1 e\n\n\
    To view the rotation, check 'Rotate with PA' under\n\
    'Graph'->'Contour levels...'\n\
";
static char *Correlate_Help = "\
                             Correlate map help\n\
                             ------------------\n\
Description:\n\
To calculate the correlation (covariance) function (as a function of pixel\n\
distance).\n\
The task will use the selected map as input data. The map is selected by\n\
highlighting a map entry in the list at the top of the dialog.\n\n\
Usage:\n\
    In the entry box the maximum correlation distance (in pixels) should\n\
    be entered. It should not be larger than the maximum distance in a map,\n\
    i.e. the diagonal in a rectangular map.\n\
    Using the non-normalized version is equivalent to obtaining the\n\
    covariance function R(n). The correlation function is simply\n\
                        r(n) = R(n)/R(0)\n\
    There is also the option to specify whether the x-axis unit should be\n\
    pixels or arcsecs.\n\
    The result will become a scatter plot.\n\
    Taking the FFT of the covariance function gives you the power spectrum\n\
    for the spatial frequencies.\n\
";

void init_convolve()
{
    convBeam.maj = 30.0;
    convBeam.min = 30.0;
    convBeam.PA  = 0.0;
    projCube.R = 34.76;
    projCube.Ve = 12.7;
    projCube.Vs = -27.33;
    projCube.Phi1 = -180.0;
    projCube.Phi2 = 180.0;
    projCube.dPhi = 10.0;
    projCube.Theta1 = -90.0;
    projCube.Theta2 = 90.0;
    projCube.dTheta = 10.0;
    
    projCube.Width = 10.0;
    projCube.Frac = 0.075;
    projCube.dx = 0.0;
    projCube.dy = 1.59;
}

static void PrintMapWidgets(StdForm *sf)
{
    ConvStruct *cs;
    MAP *m;

    void wprintf();
    
    if (!sf) return;
    
    cs = (ConvStruct *)sf->user;
    if (!cs) return;
    
    m = cs->m;
    if (!m) return;
    
    if (!cs->spectral) {
        if (cs->regrid)
            wprintf(sf->edit[0], "Regridded %s", m->name);
        else
            wprintf(sf->edit[0], "Convolved %s", m->name);
    }

    wprintf(sf->edit[4], "%f", m->xleft);
    wprintf(sf->edit[5], "%f", m->xright);
    if (m->coordType == COORD_TYPE_EQU || m->coordType == COORD_TYPE_GAL)
        wprintf(sf->edit[6], "%f", -m->xspacing);
    else
        wprintf(sf->edit[6], "%f", m->xspacing);
    wprintf(sf->edit[7], "%f", m->ylower);
    wprintf(sf->edit[8], "%f", m->yupper);
    wprintf(sf->edit[9], "%f", m->yspacing);
}

static void PrintBeamWidgets(StdForm *sf)
{
    ConvStruct *cs;
    MAP *m;

    void wprintf();
    
    cs = (ConvStruct *)sf->user;
    if (!cs) return;
    
    m = cs->m;
    
    if (!m) {
        wprintf(sf->edit[0], "%f", convBeam.maj);
        wprintf(sf->edit[1], "%f", convBeam.min);
        wprintf(sf->edit[2], "%f", convBeam.PA);
        return;
    }

    wprintf(sf->edit[0], "%f", m->b.maj);
    wprintf(sf->edit[1], "%f", m->b.min);
    wprintf(sf->edit[2], "%f", m->b.PA);
}

static void SingleMapGet(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    list *pL;
    list *mL;
    ConvStruct *cs = (ConvStruct *)sf->user;
    MAP *m = NULL;

    list *get_action_list();
    list *get_maplist();

    mL = get_maplist();

    if ((pL = get_action_list(cb, &nL, *mL)) == NULL)
        return;

    for (n=0; n<nL; n++)
        m = (MAP *)DATA(pL[n]);

    cs->m = m;

    if (sf->user_func) sf->user_func(sf);
    
    free(pL);
}

static TWODOUBLES *GetConvolveValue(double x0, double y0, Beam *b,
                                    ConvStruct *cs)
{
    int n, i, j, i1, j1, i2, j2, nX, nY;
    double x, y, u, v, cPA, sPA, r2, w, wTot=0.0, wsum=0.0, wesum=0.0;
    double l = CONVOLVE_LIM, l2;
    double norm, b2;
    static TWODOUBLES val;
    MAP *m = cs->m;
    
    if (!m || b->maj == 0.0 || b->min == 0.0) return NULL;
    
    nX = m->i_no;
    nY = m->j_no;
    
    b2 = b->maj * b->min;
    
    if (cs->regrid)
        l2 = REGRID_LIM * REGRID_LIM;
    else
        l2 = l * l;
    
    if (cs->jansky) {
        norm = fabs(4.0*m->xspacing*m->yspacing/m->b.maj/m->b.min/PI);
    } else {
        norm = GAUSSNORM * fabs(m->xspacing * m->yspacing) / b2;
    }
    
    val.v1 = 0.0;
    val.v2 = 0.0;
    
    sPA = sin(b->PA/RADTODEG);
    cPA = cos(b->PA/RADTODEG);
    
    if (m->coordType == COORD_TYPE_EQU || m->coordType == COORD_TYPE_GAL) {
        i1 = floor((l*b->maj + x0 - m->xleft)/m->xspacing);
        i2 = ceil((-l*b->maj + x0 - m->xleft)/m->xspacing);
    } else {
        i1 = floor((-l*b->maj + x0 - m->xleft)/m->xspacing);
        i2 = ceil((l*b->maj + x0 - m->xleft)/m->xspacing);
    }
    if (i1 > i2) {
        i = i1; i1 = i2; i2 = i;
    }
    if (i1 < 0) i1 = 0;
    if (i2 >= nX) i2 = nX-1;
    
    j1 = floor((-l*b->maj + y0 - m->ylower)/m->yspacing);
    j2 = ceil((l*b->maj + y0 - m->ylower)/m->yspacing);
    if (j1 > j2) {
        j = j1; j1 = j2; j2 = j;
    }
    if (j1 < 0) j1 = 0;
    if (j2 >= nY) j2 = nY-1;
    
    n = 0;
    for (i=i1; i<=i2; i++) {
        for (j=j1; j<=j2; j++) {
            if (m->f[i][j] <= BLANK) continue;
            x = m->xleft  + (double)i * m->xspacing - x0;
            y = m->ylower + (double)j * m->yspacing - y0;
            u = x*cPA - y*sPA;
            v = x*sPA + y*cPA;
            r2 = (u*u/b->min/b->min + v*v/b->maj/b->maj);
            if (r2 > l2) continue;
            n++;
            w = norm * exp(-ALPHA * r2);
            if (cs->regrid && m->e[i][j] != 0.0) {
                wTot = w/m->e[i][j]/m->e[i][j];
            }
            if (cs->regrid) {
                wsum += wTot;
                wesum += w;
                val.v1 += wTot * m->d[i][j];
                if (m->e[i][j] != 0.0) val.v2 += w/m->e[i][j]/m->e[i][j];
            } else {
                val.v1 += w * m->d[i][j];
                val.v2 += w * m->e[i][j];
            }
        }
    }
    
    if (n == 0) return NULL;
    
    if (cs->regrid) {
         val.v1 /= wsum;
         if (val.v2 > 0.0 && wesum > 0.0)
             val.v2 = 1.0/sqrt(val.v2/wesum);
         else
             val.v2 = 0.0;
    }
    
    return &val;
}

static double *GetConvolveSpe(int c, double x0, double y0, int nS,
                              Beam *b, ConvStruct *cs)
{
    int n, i, j, i1, j1, i2, j2, nX, nY;
    double x, y, u, v, cPA, sPA, r2, w, wTot, wSum;
    double l = CONVOLVE_LIM, l2;
    double norm, b2;
    static double val;
    scanPtr p = NULL;
    MAP *m = cs->m;
    
    if (!m || b->maj == 0.0 || b->min == 0.0) return NULL;
    
    nX = m->i_no;
    nY = m->j_no;
    
    b2 = b->maj * b->min;
    
    if (cs->regrid)
        l2 = REGRID_LIM * REGRID_LIM;
    else
        l2 = l * l;
    
    if (cs->jansky) {
        norm = fabs(4.0*m->xspacing*m->yspacing/m->b.maj/m->b.min/PI);
    } else {
        norm = GAUSSNORM * fabs(m->xspacing * m->yspacing) / b2;
    }
    
    sPA = sin(b->PA/RADTODEG);
    cPA = cos(b->PA/RADTODEG);
    
    if (m->coordType == COORD_TYPE_EQU || m->coordType == COORD_TYPE_GAL) {
        i1 = floor((l*b->maj + x0 - m->xleft)/m->xspacing);
        i2 = ceil((-l*b->maj + x0 - m->xleft)/m->xspacing);
    } else {
        i1 = floor((-l*b->maj + x0 - m->xleft)/m->xspacing);
        i2 = ceil((l*b->maj + x0 - m->xleft)/m->xspacing);
    }
    if (i1 < 0) i1 = 0;
    if (i2 >= nX) i2 = nX-1;
    
    j1 = floor((-l*b->maj + y0 - m->ylower)/m->yspacing);
    j2 = ceil((l*b->maj + y0 - m->ylower)/m->yspacing);
    if (j1 < 0) j1 = 0;
    if (j2 >= nY) j2 = nY-1;
    
    n = 0;
    val  = 0.0;
    wSum = 0.0;
    for (i=i1; i<=i2; i++) {
        for (j=j1; j<=j2; j++) {
            if (m->f[i][j] <= BLANK) continue;
            p = m->sp[i][j];
            if (!p) continue;
            x = m->xleft  + (double)i * m->xspacing - x0;
            y = m->ylower + (double)j * m->yspacing - y0;
            u = x*cPA - y*sPA;
            v = x*sPA + y*cPA;
            r2 = (u*u/b->min/b->min + v*v/b->maj/b->maj);
            if (r2 > l2) continue;
            w = norm * exp(-ALPHA * r2);
            if (cs->regrid && p->mom.sigma != 0.0) {
                wTot = w/p->mom.sigma/p->mom.sigma;
            } else {
                wTot = w;
            }
            wSum += wTot;
            val += wTot * (p->d[c]);
            n++;
        }
    }
    
    if (n == 0) return NULL;
    
    if (cs->regrid && wSum != 0.0) {
        val /= wSum;
    }
    
    return &val;
}

static double Angle(double l1, double b1, double l2, double b2)
{
    double d;
    
    d = sin(b1)*sin(b2) + cos(b1)*cos(b2)*cos(l1-l2);
    
    return RADTODEG*acos(d);
}

typedef struct {
    double I;
    double R;
    double dR;
} TriDouble;

static TriDouble *GetProjectValue(double x, double y, PROJCUBE *p)
{
    int c, N, n;
    double X=0.0, Y=0.0, V=0.0, Phi, Theta, a, b, b0, RXY, R, ww;
    double w, ws, s1, s2, sW, val, w2;
    static TriDouble td;
    list curr = NULL;
    scanPtr s, tmp = vP->s;
    
    double SpecUnitConv(int, int, double);
    double SpecUnitRes(scanPtr, int);
    
    val = 0.0;
    s1 = s2 = sW = ws = 0.0;
    n = N = 0;
    while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
        s = (scanPtr)DATA(curr);
        X = -((s->xoffset - p->dx)/p->R);
        Y = (s->yoffset - p->dy)/p->R;
        RXY = X*X + Y*Y;
        if (RXY > (1.0 + p->Frac)*(1.0 + p->Frac)) continue;
        vP->s = s;
        b0 = fabs(s->velres)/p->Ve * RADTODEG;
        for (c=0; c<s->nChan; c++) {
            V = -(SpecUnitConv(UNIT_VEL, UNIT_CHA, (double)c) - p->Vs)/p->Ve;
            R = sqrt(RXY + V*V);
            if (R <= 1.0 + p->Frac && R >= 1.0 - p->Frac) {
                Phi = atan2(X, V);
                Theta = PI/2.0 - acos(Y/R);
                a = Angle(Phi, Theta, x/RADTODEG, y/RADTODEG);
                
                b = b0/sqrt(1.0 - (fabs(V) < 0.99*R) ? V*V/R/R : 0.9801);
                w2 = p->Width * p->Width + b * b;
                a = a*a/w2;
                if (a <= 10.0) {
                    w = exp(-ALPHA*a);
                    ww = w * s->d[c];
                    val += ww;
                    ws += w;
                    if (ww > 0.0) {
                        s1 += R*ww;
                        s2 += R*R*ww;
                        sW += ww;
                        n++;
                    }
                    N++;
                }
            }
        }
    }
    vP->s = tmp;
    
    if (N == 0) {
       return NULL;
    }
    td.I = val/ws;
    if (sW > 0.0) {
        td.R = s1/sW;
        if (s2 > td.R * td.R * sW) {
            td.dR = p->R*sqrt(2.0*ALPHA*(s2/sW - td.R * td.R));
        } else {
            td.dR = 0.0;
        }
    }
/* 
    printf("x=%6.1f,y=%6.1f  R=%f,W=%f n(N)=%d(%d)\n", x, y, td.R, td.dR, n, N);
 */  
    return &td;
}

static void DoConvolveSpe(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int i, j, c, n=0, nX, nY, nS, mode=vP->mode, nC=0;
    int err=0;
    double xlef, xrig, xspa, ylow, yupp, yspa, x, y, *v;
    string buf;
    Widget wait=NULL, scale=NULL;
    ConvStruct *cs = (ConvStruct *)sf->user;
    double **A = NULL;
    scanPtr **I = NULL;
    list curr = NULL;
    scanPtr S, first=NULL;
    DataSetPtr dsp;

    void send_line(), wdscanf(), wsscanf(), SetWatchCursor();
    void obtain_map_info(Widget, char *, XtPointer);
    void update_bl_data(scanPtr);
    scanPtr **AllocScanPtrArray();
    double **AllocDoubleArray();
    void FreeScanPtrArray(), FreeDoubleArray();
    void SetStdView();
    int  SetViewMode(int, scanPtr, MAP *, scatter *);
    void UpdateData(int, int);

    if (!cs->m) {
        PostErrorDialog(w, "No data available to be convolved.");
        return;
    }
    if (cs->jansky && (cs->m->b.maj <= 0.0 || cs->m->b.min <= 0.0)) {
        PostErrorDialog(w, "Original map has no beam data.");
        return;
    }
    wdscanf(sf->edit[1], &(convBeam.maj));
    if (convBeam.maj <= 0.0) {
        sprintf(buf, "Major axis of beam error: %f <= 0.0", convBeam.maj);
        PostErrorDialog(w, buf);
        return;
    }
    wdscanf(sf->edit[2], &(convBeam.min));
    if (convBeam.min <= 0.0) convBeam.min = convBeam.maj;
    if (convBeam.min > convBeam.maj) {
        sprintf(buf, "Minor axis of beam error: %f > major axis", convBeam.min);
        PostErrorDialog(w, buf);
        return;
    }
    wdscanf(sf->edit[3], &(convBeam.PA));
    
    wdscanf(sf->edit[4], &xlef);
    wdscanf(sf->edit[5], &xrig);
    wdscanf(sf->edit[6], &xspa);
    if (cs->m->coordType == COORD_TYPE_EQU ||
        cs->m->coordType == COORD_TYPE_GAL) xspa *= -1.0;
    if (xlef == xrig || xspa == 0.0 || (xrig - xlef)/xspa < 0.0) {
        sprintf(buf, "Error in X coords: %f %f %f.", xlef, xrig, xspa);
        PostErrorDialog(w, buf);
        return;
    }
    wdscanf(sf->edit[7], &ylow);
    wdscanf(sf->edit[8], &yupp);
    wdscanf(sf->edit[9], &yspa);
    if (ylow == yupp || yspa == 0.0 || (yupp - ylow)/yspa < 0.0) {
        sprintf(buf, "Error in Y coords: %f %f %f.", ylow, yupp, yspa);
        PostErrorDialog(w, buf);
        return;
    }

    nX = 1 + NINT((xrig - xlef)/xspa);
    nY = 1 + NINT((yupp - ylow)/yspa);
    
    nS = count_scans(vP->from);
        
    A = AllocDoubleArray(nX, nY);
    if (!A) {
        PostErrorDialog(w, "Out of memory when allocating tmp array.");
        XtDestroyWidget(sf->form);
        return;
    }
    
    I = AllocScanPtrArray(nX, nY);
    if (!I) {
        FreeDoubleArray(A, nX, nY);
        PostErrorDialog(w, "Out of memory when allocating tmp array.");
        XtDestroyWidget(sf->form);
        return;
    }
    
    n = 0;
    while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
        S = (scanPtr)DATA(curr);
        if (n == 0) {
            first = S;
            nC = S->nChan;
        } else {
            if (S->nChan != nC) {
                FreeDoubleArray(A, nX, nY);
                FreeScanPtrArray(I, nX, nY);
                PostErrorDialog(w, "Scans have different no of channels.");
                XtDestroyWidget(sf->form);
                return;
            }
        }
        update_bl_data(S);
        n++;
    }
    
    dsp = new_dataset(get_listlist(),
                      cs->regrid ? "Regridded scans" : "Convolved scans", NULL);
    if (!dsp) {
        PostErrorDialog(w, "Out of memory when allocating new dataset.");
        return;
    }
    
    wait = PostWaitingDialog(w,
                cs->regrid ? "Regridding spectra..." : "Convolving spectra...",
                             &scale, nC);
    
    SetWatchCursor(True);
    
    for (c=0; c<nC; c++) {
        if (wait) SetWaitingScale(scale, c+1);
        n = 0;
        for (i=0; i<nX; i++) {
            x = xlef + (double)i * xspa;
            for (j=0; j<nY; j++) {
                y = ylow + (double)j * yspa;
                v = GetConvolveSpe(c, x, y, nS, &convBeam, cs);
                if (v) {
                    if (c == 0) {
                        I[i][j] = copy_scanheader(dsp, nC, first);
                        if (!I[i][j]) err++;
                    }
                    A[i][j] = *v;
                    n++;
                } else {
                    A[i][j] = UNDEF;
                    if (c == 0) I[i][j] = NULL;
                }
            }
        }
        if (err == n) break;
        n = 0;
        for (i=0; i<nX; i++) {
            x = xlef + (double)i * xspa;
            for (j=0; j<nY; j++) {
                if ((S = I[i][j]) == NULL) continue;
                y = ylow + (double)j * yspa;
                S->d[c] = A[i][j];
                S->e[c] = 0.0;
                S->xoffset = x;
                S->yoffset = y;
                S->scan_no = 1000 + n;
                n++;
            }
        }
    }
    
    FreeDoubleArray(A, nX, nY);
    FreeScanPtrArray(I, nX, nY);
    
    SetWatchCursor(False);
    if (wait) XtDestroyWidget(wait);
    
    if (err == n) {
        DeleteLastDataSet();
        PostErrorDialog(w, "Out of memory when allocating scans.");
        return;
    } else {
        if (err) {
            sprintf(buf, "Could only allocate %d scans out of %d.", n-err, n);
            PostErrorDialog(w, buf);
        }
        if (cs->regrid)
            sprintf(buf, "Regridded scans (%d) calculated.", n);
        else
            sprintf(buf, "Convolved scans (%d) calculated.", n);
        send_line(buf);
    }
    
    dsp->gridded = 1;
    dsp->dx = xspa;
    dsp->dy = yspa;
    dsp->sequence = 0;
    
    vP->from = vP->to = dsp;
    
    XtDestroyWidget(sf->form);
    
    first = (scanPtr)DATA(dsp->scanlist);
    
    sprintf(dsp->name, "%s %s (%d scans)", first->name,
            (cs->regrid ? "regridded" : "convolved"), n-err);
    
    if (mode == SHOW_SPE) {
        SetViewMode(SHOW_SPE, first, vP->m, vP->p);
        UpdateData(SCALE_ONLY_Y, REDRAW);
    } else if (mode == SHOW_ALLSPE) {
        SetViewMode(SHOW_ALLSPE, first, vP->m, vP->p);
        obtain_map_info(w, "no_update_map_data", NULL);
        UpdateData(SCALE_BOTH, REDRAW);
    } else {
        SetViewMode(SHOW_ALLSPE, first, vP->m, vP->p);
        obtain_map_info(w, "no_update_map_data", NULL);
        SetStdView();
        UpdateData(SCALE_BOTH, REDRAW);
    }
}

static void DoConvolveMap(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int i, j, nX, nY;
    double xlef, xrig, xspa, ylow, yupp, yspa, x, y;
    TWODOUBLES *val = NULL;
    string buf;
    ConvStruct *cs = (ConvStruct *)sf->user;
    Widget wait=NULL, scale=NULL;
    MAP *m = NULL;

    void send_line(), wdscanf(), wsscanf(), SetWatchCursor();
    void MapDraw();
    MAP *new_map();
    list *get_maplist();

    if (!cs->m) {
        PostErrorDialog(w, "No map selected to be convolved.");
        return;
    }
    if (cs->jansky && (cs->m->b.maj <= 0.0 || cs->m->b.min <= 0.0)) {
        PostErrorDialog(w, "Original data has no beam data.");
        return;
    }
    wdscanf(sf->edit[1], &(convBeam.maj));
    if (convBeam.maj <= 0.0) {
        sprintf(buf, "Major axis of beam error: %f <= 0.0", convBeam.maj);
        PostErrorDialog(w, buf);
        return;
    }
    wdscanf(sf->edit[2], &(convBeam.min));
    if (convBeam.min <= 0.0) convBeam.min = convBeam.maj;
    if (convBeam.min > convBeam.maj) {
        sprintf(buf, "Minor axis of beam error: %f > major axis", convBeam.min);
        PostErrorDialog(w, buf);
        return;
    }
    wdscanf(sf->edit[3], &(convBeam.PA));
    
    wdscanf(sf->edit[4], &xlef);
    wdscanf(sf->edit[5], &xrig);
    wdscanf(sf->edit[6], &xspa);
    if (cs->m->coordType == COORD_TYPE_EQU ||
        cs->m->coordType == COORD_TYPE_GAL) xspa *= -1.0;
    if (xlef == xrig || xspa == 0.0 || (xrig - xlef)/xspa < 0.0) {
        sprintf(buf, "Error in X coords: %f %f %f.", xlef, xrig, xspa);
        PostErrorDialog(w, buf);
        return;
    }
    wdscanf(sf->edit[7], &ylow);
    wdscanf(sf->edit[8], &yupp);
    wdscanf(sf->edit[9], &yspa);
    if (ylow == yupp || yspa == 0.0 || (yupp - ylow)/yspa < 0.0) {
        sprintf(buf, "Error in Y coords: %f %f %f.", ylow, yupp, yspa);
        PostErrorDialog(w, buf);
        return;
    }

    nX = 1 + NINT((xrig - xlef)/xspa);
    nY = 1 + NINT((yupp - ylow)/yspa);

    m = new_map(get_maplist(), nX, nY);
    
    m->type         = cs->m->type;
    m->coordType    = cs->m->coordType;
    m->swapped      = cs->m->swapped;
    m->memed        = cs->m->memed;
    m->interpolated = cs->m->interpolated;
    m->original     = cs->m->original;
    m->x0           = cs->m->x0;
    m->y0           = cs->m->y0;
    m->date         = cs->m->date;
    m->b            = convBeam;
    strcpy(m->molecule, cs->m->molecule);
    
    m->ndata    = nX * nY;
    m->i_min    = NINT(xlef/xspa);
    m->i_max    = NINT(xrig/xspa);
    m->j_min    = NINT(ylow/yspa);
    m->j_max    = NINT(yupp/yspa);
    m->xleft    = xlef;
    m->xright   = xrig;
    m->xspacing = xspa;
    m->ylower   = ylow;
    m->yupper   = yupp;
    m->yspacing = yspa;
    
    if (nX*nY > 100) wait = PostWaitingDialog(w,
                        cs->regrid ? "Regridding map..." : "Convolving map...",
                                              &scale, nX*nY);
    SetWatchCursor(True);
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (wait) SetWaitingScale(scale, j + 1 + i*nY);
            x = xlef + (double)i * xspa;
            y = ylow + (double)j * yspa;
            val = GetConvolveValue(x, y, &convBeam, cs);
            if (val) {
                m->f[i][j] = UNBLANK;
                m->d[i][j] = val->v1;
                m->e[i][j] = val->v2;
            } else {
                m->f[i][j] = BLANK;
                m->d[i][j] = UNDEF;
                m->e[i][j] = UNDEF;
            }
        }
    }
    
    SetWatchCursor(False);
    if (wait) XtDestroyWidget(wait);

    wsscanf(sf->edit[0], m->name);
    if (cs->regrid)
        sprintf(buf, "Regridded map stored as '%s': %dx%d\n",  m->name, nX, nY);
    else
        sprintf(buf, "Convolved map stored as '%s': %dx%d\n",  m->name, nX, nY);
    send_line(buf);
    
    XtDestroyWidget(sf->form);
    MapDraw(NULL, m, NULL);
}

void CreateConvolveDialog(Widget parent, char *cmd, Arg *args, int nArgs)
{
    Widget rc, maps=NULL;
    char *hptr = NULL;
    string title;
    StdForm *sf;
    ConvStruct *cs;

    void wprintf();
    
    cs = (ConvStruct *) XtMalloc(sizeof(ConvStruct));
    if (!cs) return;
    
    cs->spectral = cs->jansky = cs->regrid = 0;
    cs->m = NULL;
    
    if (strncmp(cmd, "s_", 2)==0) cs->spectral = 1;
    
    if (cs->spectral && (!vP->m || count_scans(vP->from) < 2)) {
        PostErrorDialog(parent, "No data available!");
        return;
    }
    
    if (strcmp(cmd, "regrid")==0 || strcmp(cmd, "s_regrid")==0) cs->regrid = 1;
        
    if (strcmp(cmd, "Jansky")==0 || strcmp(cmd, "s_Jansky")==0) cs->jansky = 1;

    sprintf(title, "%s %s", cs->spectral ? "Data" : "Map",
            cs->regrid ? "regridding" : "convolution");
            
    if (cs->regrid) {
        if (!cs->spectral) {
            hptr = ConvHelp_1;
        } else {
            hptr = ConvHelp_2;
        }
    } else {
        if (!cs->spectral) {
            hptr = ConvHelp_3;
        } else {
            hptr = ConvHelp_4;
        }
    }
    
    sf = PostStdFormDialog(parent, title,
             BUTT_APPLY,
   cs->spectral ? (XtCallbackProc)DoConvolveSpe : (XtCallbackProc)DoConvolveMap,
             NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, hptr,
             10, NULL);

    sf->user = (XtPointer)cs;
    
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          XmNpacking, XmPACK_TIGHT,
                          NULL);
    
    if (!cs->spectral) {
        maps  = XmCreateScrolledList(rc,  "list",  args,  nArgs);

        XtCreateManagedWidget("Name of new map:", xmLabelWidgetClass,
                              rc, NULL, 0);
        sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                            rc, NULL, 0);
    }

    
    XtCreateManagedWidget("Major axis:",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[1] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                    rc, NULL, 0);
    XtCreateManagedWidget("Minor axis:",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[2] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                    rc, NULL, 0);
    XtCreateManagedWidget("Position angle [deg]:",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[3] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                    rc, NULL, 0);

    XtCreateManagedWidget("X: (Left, right, spacing)", xmLabelWidgetClass,
                          rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[4]), &(sf->edit[5]), &(sf->edit[6]));

    XtCreateManagedWidget("Y: (Lower, upper, spacing)", xmLabelWidgetClass,
                          rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[7]), &(sf->edit[8]), &(sf->edit[9]));

    if (cs->spectral) {
        cs->m = vP->m;
    } else {
        sf->user_func = PrintMapWidgets;
        XtAddCallback(maps, XmNsingleSelectionCallback,
                      (XtCallbackProc)SingleMapGet, sf);
    }
    
    ArrangeStdFormDialog(sf, rc);
    
    if (!cs->spectral) XtManageChild(maps);
    XtManageChild(rc);

    wprintf(sf->edit[1], "%f", convBeam.maj);
    wprintf(sf->edit[2], "%f", convBeam.min);
    wprintf(sf->edit[3], "%f", convBeam.PA);
    if (cs->spectral) {
        PrintMapWidgets(sf);
    } else {
        wprintf(sf->edit[0], "<none>");
        wprintf(sf->edit[4], "%f", 0.0);
        wprintf(sf->edit[5], "%f", 0.0);
        wprintf(sf->edit[6], "%f", 0.0);
        wprintf(sf->edit[7], "%f", 0.0);
        wprintf(sf->edit[8], "%f", 0.0);
        wprintf(sf->edit[9], "%f", 0.0);
    }
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void MapBeamEdit(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    double maj, min, PA;
    string buf;
    ConvStruct *cs;
    Beam *b = NULL;
    
    void wdscanf(), send_line();
    
    if (!sf) return;
    
    cs = (ConvStruct *)sf->user;
    
    if (sf->edit) {
        wdscanf(sf->edit[0], &maj);
        wdscanf(sf->edit[1], &min);
        wdscanf(sf->edit[2], &PA);
    }
    if (cs->m) {
        b = &(cs->m->b);
        if (maj == 0.0) {
            PostErrorDialog(w, "Major axis value cannot be zero.");
            return;
        }
        b->maj = fabs(maj);
        if (min > 0.0) {
            if (min > b->maj) {
                PostErrorDialog(w, "Minor axis > major axis.");
                return;
            }
            b->min = min;
        } else {
            b->min = maj;
        }
        
        if (b->maj == b->min)
            b->PA = 0.0;
        else
            b->PA = PA;
            
        sprintf(buf, "Saved beam (%fx%f %f) to map '%s'.\n",
                b->maj, b->min, b->PA, cs->m->name);
        send_line(buf);
        if (vP->mode == SHOW_POSPOS && vP->m == cs->m &&
            draw.beam) draw_main();
    }
}

void CreateMapBeamEditDialog(Widget parent, char *cmd, Arg *args, int nArgs)
{
    Widget rc, maps;
    StdForm *sf;
    ConvStruct *cs;

    void wprintf();
     
    cs = (ConvStruct *) XtMalloc(sizeof(ConvStruct));
    if (!cs) return;
    cs->m = NULL;
    
    sf = PostStdFormDialog(parent, "Edit beam of map",
             BUTT_APPLY, (XtCallbackProc)MapBeamEdit, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 3, NULL);

    sf->user = (XtPointer)cs;
    
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          XmNpacking, XmPACK_TIGHT,
                          NULL);

    maps  = XmCreateScrolledList(rc,  "list",  args,  nArgs);

    XtCreateManagedWidget("Major axis (FWHM):", xmLabelWidgetClass,
                          rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                    rc, NULL, 0);
    XtCreateManagedWidget("Minor axis (FWHM):", xmLabelWidgetClass,
                          rc, NULL, 0);
    sf->edit[1] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                    rc, NULL, 0);
    XtCreateManagedWidget("Position angle [deg]", xmLabelWidgetClass,
                                    rc, NULL, 0);
    sf->edit[2] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                    rc, NULL, 0);

    sf->user_func = PrintBeamWidgets;
    XtAddCallback(maps, XmNsingleSelectionCallback,
                  (XtCallbackProc)SingleMapGet, sf);

    ArrangeStdFormDialog(sf, rc);

    XtManageChild(maps);
    XtManageChild(rc);

    PrintBeamWidgets(sf);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void DoProjectCube(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int i, j, nX, nY;
    double p1, p2, dp, t1, t2, dt, x, y, dx, dy;
    TriDouble *v;
    string buf;
    MAP *m = NULL, *mR = NULL, *mW = NULL;
    scanPtr s = NULL;
    Widget wait=NULL, scale=NULL;

    list *get_maplist();
    void send_line(), wdscanf(), wsscanf(), SetWatchCursor();
    void MapDraw();
    MAP *new_map();
    MAP *copy_map();

    wdscanf(sf->edit[0], &(projCube.R));
    if (projCube.R <= 0.0) {
        sprintf(buf, "Radius error: %f <= 0.0", projCube.R);
        PostErrorDialog(w, buf);
        return;
    }
    wdscanf(sf->edit[1], &(projCube.Ve));
    if (projCube.Ve == 0.0) {
        sprintf(buf, "Exp. vel. error: %f is zero.", projCube.Ve);
        PostErrorDialog(w, buf);
        return;
    }
    wdscanf(sf->edit[2], &projCube.Vs);
    wdscanf(sf->edit[3], &projCube.Width);
    wdscanf(sf->edit[4], &projCube.Frac);
    
    wdscanf(sf->edit[5], &p1);
    wdscanf(sf->edit[6], &p2);
    wdscanf(sf->edit[7], &dp);
    wdscanf(sf->edit[8], &dx);
    if (p1 == p2 || dp == 0.0 || (p2 - p1)/dp < 0.0) {
        sprintf(buf, "Error in Phi coords: %f %f %f.", p1, p2, dp);
        PostErrorDialog(w, buf);
        return;
    }
    projCube.Phi1 = p1; projCube.Phi2 = p2;
    projCube.dPhi = dp; projCube.dx = dx;
    wdscanf(sf->edit[9], &t1);
    wdscanf(sf->edit[10], &t2);
    wdscanf(sf->edit[11], &dt);
    wdscanf(sf->edit[12], &dy);
    if (t1 == t2 || dt == 0.0 || (t2 - t1)/dt < 0.0) {
        sprintf(buf, "Error in Theta coords: %f %f %f.", t1, t2, dt);
        PostErrorDialog(w, buf);
        return;
    }
    projCube.Theta1 = t1; projCube.Theta2 = t2;
    projCube.dTheta = dt; projCube.dy = dy;

    nX = 1 + NINT((p2 - p1)/dp);
    nY = 1 + NINT((t2 - t1)/dt);

    m = new_map(get_maplist(), nX, nY);
    if (!m) return;
    
    s = (scanPtr)DATA(vP->from->scanlist);
    
    m->type         = MAP_POSPOS;
    m->coordType    = s->coordType;
    m->swapped      = 0;
    m->memed        = 0;
    m->interpolated = 0;
    m->original     = NULL;
    m->x0           = s->x0;
    m->y0           = s->y0;
    m->date         = s->date;
    m->b            = convBeam;
    strcpy(m->molecule, s->molecule);
    
    m->ndata    = nX * nY;
    m->i_min    = NINT(p1/dp);
    m->i_max    = NINT(p2/dp);
    m->j_min    = NINT(t1/dt);
    m->j_max    = NINT(t2/dt);
    m->xleft    = p1;
    m->xright   = p2;
    m->xspacing = dp;
    m->ylower   = t1;
    m->yupper   = t2;
    m->yspacing = dt;
    
    mR = copy_map(get_maplist(), m);
    mW = copy_map(get_maplist(), m);
        
    wait = PostWaitingDialog(w, "Projecting cube...",
                             &scale, nX*nY);
    SetWatchCursor(True);
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (wait) SetWaitingScale(scale, j + 1 + i*nY);
            x = m->xleft  + (double)i * m->xspacing;
            y = m->ylower + (double)j * m->yspacing;
            v = GetProjectValue(x, y, &projCube);
            if (v) {
                m->f[i][j] = UNBLANK;
                m->d[i][j] = v->I;
                m->e[i][j] = 0.0;
                if (mR) {
                    mR->f[i][j] = UNBLANK;
                    mR->d[i][j] = v->R;
                    mR->e[i][j] = 0.0;
                }
                if (mW) {
                    mW->f[i][j] = UNBLANK;
                    mW->d[i][j] = v->dR;
                    mW->e[i][j] = 0.0;
                }
            } else {
                m->f[i][j] = BLANK;
                m->d[i][j] = UNDEF;
                m->e[i][j] = 0.0;
                if (mR) {
                    mR->f[i][j] = BLANK;
                    mR->d[i][j] = UNDEF;
                    mR->e[i][j] = 0.0;
                }
                if (mW) {
                    mW->f[i][j] = BLANK;
                    mW->d[i][j] = UNDEF;
                    mW->e[i][j] = 0.0;
                }
            }
        }
    }
    
    SetWatchCursor(False);
    if (wait) XtDestroyWidget(wait);

    sprintf(m->name, "Proj %s", s->name);
    if (mR) sprintf(mR->name, "Radius %s", s->name);
    if (mW) sprintf(mW->name, "Width %s", s->name);
    sprintf(buf, "Projected map stored as '%s': %dx%d\n",  m->name, nX, nY);
    send_line(buf);
    
    XtDestroyWidget(sf->form);
    MapDraw(NULL, m, NULL);
}

void CreateProjectDialog(Widget parent, char *cmd)
{
    int n;
    Widget rc;
    StdForm *sf;

    void wprintf();
    
    if (count_scans(vP->from) < 2) {
        PostErrorDialog(parent, "No data available!");
        return;
    }
    
    sf = PostStdFormDialog(parent, "Cube projection",
             BUTT_APPLY, (XtCallbackProc)DoProjectCube, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 13, NULL);

    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          XmNpacking, XmPACK_TIGHT,
                          NULL);
    
    XtCreateManagedWidget("Radius [arcsec]:",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);
    XtCreateManagedWidget("Exp. velocity and Vstar [km/s]:",
                          xmLabelWidgetClass, rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[1]), &(sf->edit[2]), NULL);

    XtCreateManagedWidget("Gaussian smearing width [deg]:",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[3] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);
    XtCreateManagedWidget("Fraction around radius:",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[4] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);

    XtCreateManagedWidget("Phi: (Left, right, spacing)",
                          xmLabelWidgetClass,
                          rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[5]), &(sf->edit[6]), &(sf->edit[7]));

    XtCreateManagedWidget("Theta: (Lower, upper, spacing)",
                          xmLabelWidgetClass,
                          rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[9]), &(sf->edit[10]), &(sf->edit[11]));

    XtCreateManagedWidget("Offsets: (RA-offset, Dec-offset)",
                          xmLabelWidgetClass,
                          rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[8]), &(sf->edit[12]), NULL);
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(rc);

    n = 0;
    wprintf(sf->edit[n++], "%f", projCube.R);
    wprintf(sf->edit[n++], "%f", projCube.Ve);
    wprintf(sf->edit[n++], "%f", projCube.Vs);
    wprintf(sf->edit[n++], "%f", projCube.Width);
    wprintf(sf->edit[n++], "%f", projCube.Frac);
    
    wprintf(sf->edit[n++], "%f", projCube.Phi1);
    wprintf(sf->edit[n++], "%f", projCube.Phi2);
    wprintf(sf->edit[n++], "%f", projCube.dPhi);
    wprintf(sf->edit[n++], "%f", projCube.dx);
    
    wprintf(sf->edit[n++], "%f", projCube.Theta1);
    wprintf(sf->edit[n++], "%f", projCube.Theta2);
    wprintf(sf->edit[n++], "%f", projCube.dTheta);
    wprintf(sf->edit[n++], "%f", projCube.dy);
    
    ManageDialogCenteredOnPointer(sf->form);
}

void manipulate_spectra(Widget wid, char *cmd, XtPointer cd)
{
    Widget w = wid;

    void CreateConvolveDialog();

    while (!XtIsWMShell(w))
        w = XtParent(w);
    
    if (strcmp(cmd, "ConvolveTant") == 0) {
        CreateConvolveDialog(w, "s_Tant", NULL, 0);
    } else if (strcmp(cmd, "ConvolveJansky") == 0) {
        CreateConvolveDialog(w, "s_Jansky", NULL, 0);
    } else if (strcmp(cmd, "Regrid") == 0) {
        CreateConvolveDialog(w, "s_regrid", NULL, 0);
    } else if (strcmp(cmd, "Project") == 0) {
        CreateProjectDialog(w, "project");
    }
}

static void DoScaleMap(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int i, j, nX, nY;
    double c0, c1, x, y;
    double x0, y0, sx, sy, pa;
    double xspa, xrig, xlef, yspa, yupp, ylow;
    string buf;
    ConvStruct *cs = (ConvStruct *)sf->user;
    MAP *m = NULL;

    void send_line(), wdscanf(), wsscanf(), SetWatchCursor();
    void MapDraw();
    MAP *copy_map();
    list *get_maplist();

    if (!cs->m) {
        PostErrorDialog(w, "No map selected to be scaled.");
        return;
    }
    wdscanf(sf->edit[0], &x0);
    wdscanf(sf->edit[1], &y0);
    wdscanf(sf->edit[2], &sx);
    wdscanf(sf->edit[3], &sy);
    wdscanf(sf->edit[4], &pa);
    if (sx == 0.0) {
        sprintf(buf, "Impossible to scale x-axis: Sx = 0.0");
        PostErrorDialog(w, buf);
        return;
    }
    if (sy == 0.0) {
        sprintf(buf, "Impossible to scale y-axis: Sy = 0.0");
        PostErrorDialog(w, buf);
        return;
    }
    wdscanf(sf->edit[5], &c0);
    wdscanf(sf->edit[6], &c1);
    
    nX   = cs->m->i_no;
    nY   = cs->m->j_no;
    xlef = cs->m->xleft;
    xrig = cs->m->xright;
    xspa = cs->m->xspacing;
    ylow = cs->m->ylower;
    yupp = cs->m->yupper;
    yspa = cs->m->yspacing;

    m = copy_map(get_maplist(), cs->m);
        
    SetWatchCursor(True);
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            x = xlef + (double)i * xspa;
            y = ylow + (double)j * yspa;
            m->f[i][j]  = cs->m->f[i][j];
            m->sp[i][j] = cs->m->sp[i][j];
            if (m->f[i][j] <= BLANK) {
                m->d[i][j] = UNDEF;
                m->e[i][j] = UNDEF;
            } else {
                m->d[i][j] = c0 + c1*(cs->m->d[i][j]);
                /* The noise should only scale linearly */
                m->e[i][j] = c1*(cs->m->e[i][j]);
            }
        }
    }
    
    SetWatchCursor(False);
    
    xlef = sx*(xlef - x0);
    xrig = sx*(xrig - x0);
    xspa = sx*xspa;
    ylow = sy*(ylow - y0);
    yupp = sy*(yupp - y0);
    yspa = sy*yspa;
    
    m->i_min    = NINT(xlef/xspa);
    m->i_max    = NINT(xrig/xspa);
    m->j_min    = NINT(ylow/yspa);
    m->j_max    = NINT(yupp/yspa);
    m->xleft    = xlef;
    m->xright   = xrig;
    m->xspacing = xspa;
    m->ylower   = ylow;
    m->yupper   = yupp;
    m->yspacing = yspa;
/*  m->posAngle = pa/RADTODEG; */
    m->posAngle = pa;
    
    /* New centre coordinates */
    m->x0 = cs->m->x0;
    m->y0 = cs->m->y0;
    
    sprintf(buf, "Scaled %s", cs->m->name);
    strcpy(m->name, buf);

    sprintf(buf, "Scaled map stored as '%s': %dx%d\n",  m->name, nX, nY);
    send_line(buf);
    
    XtDestroyWidget(sf->form);
    MapDraw(NULL, m, NULL);
}

void CreateMapScaleDialog(Widget parent, char *cmd, Arg *args, int nArgs)
{
    Widget rc, rch, maps;
    StdForm *sf;
    ConvStruct *cs;

    void wprintf();
    
    cs = (ConvStruct *) XtMalloc(sizeof(ConvStruct));
    if (!cs) return;
    cs->m = NULL;
     
    sf = PostStdFormDialog(parent, "Scale map",
             BUTT_APPLY, (XtCallbackProc)DoScaleMap, NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, Scale_Help,
             7, NULL);

    sf->user = (XtPointer)cs;
    
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          XmNpacking, XmPACK_TIGHT,
                          NULL);

    maps  = XmCreateScrolledList(rc,  "list",  args,  nArgs);

    XtCreateManagedWidget("New center offsets (x0, y0):",
                          xmLabelWidgetClass, rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[0]), &(sf->edit[1]), NULL);

    XtCreateManagedWidget("New scale factors (Sx, Sy):",
                          xmLabelWidgetClass, rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[2]), &(sf->edit[3]), NULL);
    rch = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, rc,
                           XmNorientation, XmHORIZONTAL,
                           NULL);
    XtCreateManagedWidget("Rotation [deg]:", xmLabelWidgetClass, rch, NULL, 0);
    sf->edit[4] = XtVaCreateManagedWidget("edit", xmTextWidgetClass, rch,
                                          NULL);
    
    XtCreateManagedWidget("Scale z-data: z' = c0 + c1 z",
                          xmLabelWidgetClass, rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[5]), &(sf->edit[6]), NULL);

    XtAddCallback(maps, XmNsingleSelectionCallback,
                  (XtCallbackProc)SingleMapGet, sf);
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(maps);
    XtManageChild(rch);
    XtManageChild(rc);

    wprintf(sf->edit[0], "%.1f", 0.0);
    wprintf(sf->edit[1], "%.1f", 0.0);

    wprintf(sf->edit[2], "%.1f", 1.0);
    wprintf(sf->edit[3], "%.1f", 1.0);

    wprintf(sf->edit[4], "%.1f", 0.0);
    
    wprintf(sf->edit[5], "%.1f", 0.0);
    wprintf(sf->edit[6], "%.1f", 1.0);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void AddCorrelations(MAP *m, int iPix, int jPix, double mean,
                            double *sum, int *freq, int maxDist)
{
    int i, j, n, nX, nY;
    long int iDist=0, jDist=0, d2, D2 = (long int)maxDist;
    double diff;
    
    D2 *= D2;
    diff = m->d[iPix][jPix] - mean;
    
    nX = iPix + maxDist;
    if (nX >= m->i_no) nX = m->i_no-1;
    
    nY = jPix + maxDist;
    if (nY >= m->j_no) nY = m->j_no-1;
    
    for (i=iPix; i<=nX; i++) {
        iDist = i - iPix;
        for (j=jPix; j<=nY; j++) {
            if (m->f[i][j] <= BLANK) continue;
            jDist = j - jPix;
            d2 = iDist*iDist + jDist*jDist;
            if (d2 > D2) break;
            n = NINT(sqrt((double)d2));
            freq[n]++;
            sum[n] += (m->d[i][j] - mean)*diff;
        }
    }
} 

static void DoCorrelateMap(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int i, j, nX, nY, nP=0;
    string buf;
    ConvStruct *cs = (ConvStruct *)sf->user;
    scatter *s = NULL;
    MAP *m = NULL;
    int    *freq;
    double *sum, mean=0.0;
    Widget wait=NULL, scale=NULL;

    void wiscanf(), SetWatchCursor();
    scatter *new_scatter();
    list *get_scatterlist();
    void ScatterDraw(Widget, scatter *, XmListCallbackStruct *);
    DATE *XS_localtime();

    if (!cs->m) {
        PostErrorDialog(w, "No map selected to be correlated.");
        return;
    }
    m = cs->m;
    nX = m->i_no;
    nY = m->j_no;
    
    wiscanf(sf->edit[0], &PixelDist);
    if (PixelDist <= 0) {
        sprintf(buf, "Impossible to use a pixel distance <= 0 (%d)", PixelDist);
        PostErrorDialog(w, buf);
        return;
    }

    /* Allocate and zero the covariance sum vector */
    sum = (double *)calloc(PixelDist+1, sizeof(double));
    if (!sum) {
        PostErrorDialog(w, "Out of memory in correlate map.");
        return;
    }
    /* Allocate and zero the frequency vector:
          freq[0] will contain no of pixels (that are non-blank)
          freq[n] holds the no of pixel pairs with pixel distance=n */
    freq = (int *)calloc(PixelDist+1, sizeof(int));
    if (!freq) {
        free((char *)sum);
        PostErrorDialog(w, "Out of memory in correlate map.");
        return;
    }
    /* Get a new scatter to hold the correlation function */
    s = new_scatter(get_scatterlist(), PixelDist+1);
    if (!s) {
        free((char *)sum);
        free((char *)freq);
        PostErrorDialog(w, "Out of memory in correlate map.");
        return;
    }
    
    /* Find the mean intensity of the map */
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (m->f[i][j] <= BLANK) continue;
            mean += m->d[i][j];
            nP++;
        }
    }
    if (nP > 0) mean /= (double)nP;
        
    wait = PostWaitingDialog(w, "Correlating map...",
                             &scale, nX*nY);
    SetWatchCursor(True);
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (wait) SetWaitingScale(scale, j + 1 + i*nY);
            if (m->f[i][j] <= BLANK) continue;
            AddCorrelations(m, i, j, mean, sum, freq, PixelDist);
        }
    }

    SetWatchCursor(False);
    
    if (wait) XtDestroyWidget(wait);
    
    /* Setup the scatter plot data */
    for (i=0; i<s->nData; i++) {
        if (CorrXUnit)
            s->x[i] = (double)i * fabs(m->xspacing);
        else
            s->x[i] = (double)i;
        s->y[i] = 0.0;
        if (freq[i] > 0) s->y[i] = sum[i]/(double)freq[i];
        s->ex[i] = 0.0;
        s->ey[i] = 0.0;
        s->sp[i] = NULL;
        s->t[i]  = 0;
    }
    
    /* Free the memory allocated by the temporary vectors sum[] and freq[] */
    free((void *)sum);
    free((void *)freq);
    
    /* Normalize R[n]/R[0] */
    if (s->y[0] != 0.0 && CorrNorm) {
        for (i=s->nData-1; i>=0; i--) {
            s->y[i] /= s->y[0];
        }
    }
    
    sprintf(s->name, "Correlated %s (%d)", m->name, PixelDist);
    strcpy(s->molecule, m->molecule);
    s->x0   = m->x0;
    s->y0   = m->y0;
    s->date = *XS_localtime();
    if (CorrXUnit)
        s->xtype = XTYPE_SCA_DIST;
    else
        s->xtype = XTYPE_SCA_PDIST;
    s->ytype = YTYPE_SCA_CORR;
    s->m1   = NULL;
    s->m2   = NULL;
    s->s    = NULL;
    s->p    = NULL;
    s->dsp  = NULL;
    s->swapped = 0;
    
    ScatterDraw(w, s, NULL);
}

static void SetCorrNormCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    int n = atoi(s);
    
    if (n != CorrNorm) {
        CorrNorm = n;
    }
}

static void SetCorrUnitCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    int n = atoi(s);
    
    if (n != CorrXUnit) {
        CorrXUnit = n;
    }
}

void CreateMapCorrelationDialog(Widget parent, char *cmd, Arg *args, int nArgs)
{
    Widget rc, maps, menuN, menuU;
    StdForm *sf;
    ConvStruct *cs;

    void wprintf();
    
    cs = (ConvStruct *) XtMalloc(sizeof(ConvStruct));
    if (!cs) return;
    cs->m = NULL;
     
    sf = PostStdFormDialog(parent, "Correlate map",
             BUTT_APPLY, (XtCallbackProc)DoCorrelateMap, NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, Correlate_Help,
             1, NULL);

    sf->user = (XtPointer)cs;
    
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          XmNpacking, XmPACK_TIGHT,
                          NULL);

    maps  = XmCreateScrolledList(rc,  "list",  args,  nArgs);

    XtCreateManagedWidget("Maxium correlation distance (in pixels):",
                          xmLabelWidgetClass, rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[0]), NULL, NULL);
    
    menuN = CreateOptionMenu(rc, &CorrNormMenu);
    SetDefaultOptionMenuItem(menuN, CorrNorm);
    
    menuU = CreateOptionMenu(rc, &CorrUnitMenu);
    SetDefaultOptionMenuItem(menuU, CorrXUnit);

    XtAddCallback(maps, XmNsingleSelectionCallback,
                  (XtCallbackProc)SingleMapGet, sf);
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(maps);
    XtManageChild(menuN);
    XtManageChild(menuU);
    XtManageChild(rc);

    wprintf(sf->edit[0], "%d", PixelDist);
    
    ManageDialogCenteredOnPointer(sf->form);
}
