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

#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/Frame.h>

#include "defines.h"
#include "global_structs.h"
#include "menus.h"
#include "dialogs.h"

#ifdef HAVE_LIBPGPLOT
#include "cpgplot.h"
#endif

/* Global declarations */
void   PostErrorDialog(Widget, char *);
void   PostWarningDialog(Widget, char *);
Widget CreateOptionMenu(Widget, MenuBarItem *);
void   SetDefaultOptionMenuItem(Widget, int);
void   ChangeCallbackDataInMenuItems(MenuItem *, char *);
int    QueryOptionMenuItemNumber(Widget, Widget);
void   ManageDialogCenteredOnPointer(Widget);
void   wprintf();

double  *AllocDoubleVector(int);
void     FreeDoubleVector(double *);
    
extern VIEW   *vP;
extern GLOBAL *gp;

/* Local declarations */
typedef struct {
    Widget l;
    Widget f;
    Widget e;
    Widget d;
    int    fit;
} WLIST;

typedef struct {
    Widget   menu;
    Widget   rowcol;
    Widget   error;
    double   sigma;
    int      nPar;
    int      type;
    WLIST   *w;
    Gauss2D  g;
} GaussFit2D;

static char *Gauss2DCircDescs[] = {
    "Amplitude",
    "X value",
    "Y value",
    "Width"
};

static char *Gauss2DDescs[] = {
    "Amplitude",
    "X value",
    "Y value",
    "Major axis",
    "Minor axis",
    "Pos. angle [deg]"
};

static char *Gauss2DRingDescs[] = {
    "Amplitude",
    "X value",
    "Y value",
    "Width",
    "Radius"
};

static char *Gauss2DPlaneDescs[] = {
    "Constant",
    "X value",
    "Y value",
};

static char *Gauss2DQuadDescs[] = {
    "Constant",
    "X value",
    "Y value",
    "X^2 value",
    "Y^2 value",
    "XY value",
};

static void Gauss2DTypeCallback(Widget, char *, XmAnyCallbackStruct *);

MenuItem Gauss2DTypeMenuData[] = {
  {"Circular", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Gauss2DTypeCallback, "0", NULL},
  {"Elliptic", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Gauss2DTypeCallback, "1", NULL},
  {"Ring", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Gauss2DTypeCallback, "2", NULL},
  {"Plane", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Gauss2DTypeCallback, "3", NULL},
  {"Quad. surface", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Gauss2DTypeCallback, "4", NULL},
EOI};

MenuBarItem Gauss2DTypeOptionMenu = {
   "Type of 2 dim. function", ' ', True, Gauss2DTypeMenuData
};

static double Get2DValue(double x, double y, GaussFit2D *gf)
{
    double z=0.0;
    double arg, ax=0.0, ay=0.0;
    double u, v, ua, va, cp, sp;
    Gauss2D  *g;
    
    if (!gf) return 0.0;
    
    g = &(gf->g);
    
    if (gf->type == 0) {
      if (g->maj != 0.0) {
         ax = (x - g->x)/g->maj;
         ay = (y - g->y)/g->maj;
      }
      arg = ALPHA * (ax*ax + ay*ay);
      z = g->A * exp(-arg);
    } else if (gf->type == 1) {
      cp = cos(g->PA); sp = sin(g->PA);
      ax = x - g->x; ay = y - g->y;
      u = (ax*cp - ay*sp);
      v = (ay*cp + ax*sp);
    
      if (g->maj == 0.0 || g->min == 0.0) {
        z = 0.0;
      } else {
        ua = ALPHA * u * u /g->min/g->min;
        va = ALPHA * v * v /g->maj/g->maj;
        z = g->A * exp(-ua-va);
      }
    } else if (gf->type == 2) {
      ax = (x - g->x);
      ay = (y - g->y);
      u = sqrt(ax*ax + ay*ay);
      v = 0.0;
      if (g->maj != 0.0) {
          v = (u - g->min)/g->maj;
      }
      z = g->A * exp(-ALPHA * v * v);
    } else if (gf->type == 3) {
      z = g->A + g->x * x + g->y * y;
    } else if (gf->type == 4) {
      z = g->A + g->x * x + g->y * y;
      z += g->maj *x*x + g->min *y*y + g->PA *x*y;
    }
    
    return z;
}

static int ret_fit(double *x1, double *x2, double *y, double *e, int n, int ret)
{
    if (x1) FreeDoubleVector(x1);
    if (x2) FreeDoubleVector(x2);
    if (y)  FreeDoubleVector(y);
    if (e)  FreeDoubleVector(e);

    return ret;
}

int StdMapFitter(MAP *m, double sigma, PolyLine *pl, double p[], int fit[],
                 double q[], int nPar, void (*f)())
{
    int i, j, nX, nY, nData, n, err, nIter=12;
    double Chi2;
    double *x1, *x2, *y, *e;
    Point p0;
    
    int InsidePolyLine(Point *, PolyLine *);
    int Fitter2D(double x1[], double x2[], double y[], double e[], int nData,
                 double p[], int fit[], double q[], int nPar,
                 int nIter, double *chi2, void (*f)());
    
    if (!m) return 1;
    
    nX = m->i_no;
    nY = m->j_no;
    
    nData = 0;
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (m->f[i][j] <= BLANK) continue;
            p0.x = m->xleft  + (double)i * m->xspacing;
            p0.y = m->ylower + (double)j * m->yspacing;
            if (pl && InsidePolyLine(&p0, pl) < 1) continue;
            nData++;
        }
    }
    
    /* Ok, we need four vectors of size nData */
    
    if (!(x1 = AllocDoubleVector(nData))) {
        PostErrorDialog(NULL,
                         "Out of memory when fitting 2-dim. surface.");
        return ret_fit(NULL, NULL, NULL, NULL, nData, 2);
    }
    if (!(x2 = AllocDoubleVector(nData))) {
        PostErrorDialog(NULL,
                         "Out of memory when fitting 2-dim. surface.");
        return ret_fit(x1, NULL, NULL, NULL, nData, 3);
    }
    if (!(y = AllocDoubleVector(nData))) {
        PostErrorDialog(NULL,
                         "Out of memory when fitting 2-dim. surface.");
        return ret_fit(x1, x2, NULL, NULL, nData, 4);
    }
    if (!(e = AllocDoubleVector(nData))) {
        PostErrorDialog(NULL,
                         "Out of memory when fitting 2-dim. surface.");
        return ret_fit(x1, x2, y, NULL, nData, 5);
    }
   
    n = 0;
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (m->f[i][j] <= BLANK) continue;
            p0.x = m->xleft  + (double)i * m->xspacing;
            p0.y = m->ylower + (double)j * m->yspacing;
            if (pl && InsidePolyLine(&p0, pl) < 1) continue;
            x1[n] = p0.x;
            x2[n] = p0.y;
            y[n]  = m->d[i][j];
            if (sigma > 0.0)
                e[n]  = sigma;
            else
                e[n]  = m->e[i][j];
            n++;
        }
    }
    
    err = Fitter2D(x1, x2, y, e, nData, p, fit, q, nPar,
                   nIter, &Chi2, f);
                        
    if (err) {
        return ret_fit(x1, x2, y, e, nData, 100+err);
    }
    
    if (nData)
        wprintf(gp->TGauss[2], "Chi^2: %11.4e", Chi2/(double)nData);
   
    return ret_fit(x1, x2, y, e, nData, 0);
}

static void ToggleFitCallback(Widget w, WLIST *wl,
                              XmToggleButtonCallbackStruct *cd)
{
    if (cd->set) {
        wl->fit = 1;
        wprintf(wl->f, "%s", "Fitted");
    } else {
        wl->fit = 0;
        wprintf(wl->f, "%s", "Fixed");
    }
}

static void cleanup_2D_dialog(XtPointer user)
{
    GaussFit2D *gf = (GaussFit2D *)user;
    
    if (gf) {
        if (gf->w) XtFree((char *)gf->w);
    }
}

static void Update2DimWidgets(GaussFit2D *gf, int update_errs)
{
    if (!gf) return;
    if (!gf->w) return;
    
    wprintf(gf->error,  "%f", gf->sigma);
    wprintf(gf->w[0].d, "%f", gf->g.A);
    wprintf(gf->w[1].d, "%f", gf->g.x);
    wprintf(gf->w[2].d, "%f", gf->g.y);
    wprintf(gf->w[3].d, "%f", gf->g.maj);
    if (gf->nPar >= 5) wprintf(gf->w[4].d, "%f", gf->g.min);
    if (gf->type == 1) wprintf(gf->w[5].d, "%f", gf->g.PA * RADTODEG);
    if (gf->type == 4) wprintf(gf->w[5].d, "%f", gf->g.PA);
    
    if (update_errs == 1) {
        wprintf(gf->w[0].e, "%f", gf->g.uA);
        wprintf(gf->w[1].e, "%f", gf->g.ux);
        wprintf(gf->w[2].e, "%f", gf->g.uy);
        wprintf(gf->w[3].e, "%f", gf->g.umaj);
        if (gf->nPar >= 5) wprintf(gf->w[4].e, "%f", gf->g.umin);
        if (gf->type == 1) wprintf(gf->w[5].e, "%f", gf->g.uPA * RADTODEG);
        if (gf->type == 4) wprintf(gf->w[5].e, "%f", gf->g.uPA);
    } else if (update_errs == -1) {
        wprintf(gf->w[0].e, "%s", "");
        wprintf(gf->w[1].e, "%s", "");
        wprintf(gf->w[2].e, "%s", "");
        wprintf(gf->w[3].e, "%s", "");
        if (gf->nPar >= 5) wprintf(gf->w[4].e, "%s", "");
        if (gf->nPar == 6) wprintf(gf->w[5].e, "%s", "");
    }
}

static void Scan2DimWidgets(GaussFit2D *gf)
{
    void wdscanf();
    
    if (!gf) return;
    if (!gf->w) return;
    
    wdscanf(gf->error, &(gf->sigma));
    wdscanf(gf->w[0].d, &(gf->g.A));
    wdscanf(gf->w[1].d, &(gf->g.x));
    wdscanf(gf->w[2].d, &(gf->g.y));
    wdscanf(gf->w[3].d, &(gf->g.maj));
    if (gf->nPar >= 5) wdscanf(gf->w[4].d, &(gf->g.min));
    if (gf->nPar == 6) {
        wdscanf(gf->w[5].d, &(gf->g.PA));
        if (gf->type == 1) gf->g.PA /= RADTODEG;
    }
}

static void Do2DimFit(Widget w, StdForm *sf, XmAnyCallbackStruct *cd)
{
    int n, err, fit[6];
    double p[6], q[6];
    string buf;
    PolyLine *pL;
    GaussFit2D *gf;
    Beam b;
    
    extern void lm_Gauss2D(), lm_Gauss2DCirc(), lm_Gauss2DRing();
    extern void lm_Plane2D(), lm_Quad2D();
    void draw_beam(GC, double, double, Beam *);
    PolyLine *GetFirstClosedPolyLine(Point *);
    
    if (!sf) return;
    gf = (GaussFit2D *)sf->user;
    if (!gf) return;
    
    if (!vP->m) {
        sprintf(buf, "No current map.");
        PostErrorDialog(w, buf);
        return;
    }
    
    pL = GetFirstClosedPolyLine(NULL);
    
    Scan2DimWidgets(gf);
    
    p[0] = gf->g.A;
    p[1] = gf->g.x;
    p[2] = gf->g.y;
    if (gf->nPar >= 4) p[3] = gf->g.maj;
    if (gf->nPar >= 5) p[4] = gf->g.min;
    if (gf->nPar == 6) p[5] = gf->g.PA;
    
    for (n=0; n<gf->nPar; n++) fit[n] = gf->w[n].fit;
    
    if (gf->nPar == 4) {
        err = StdMapFitter(vP->m, gf->sigma, pL, p, fit, q, 4,
                           lm_Gauss2DCirc);
    } else if (gf->nPar == 3) {
        err = StdMapFitter(vP->m, gf->sigma, pL, p, fit, q, 3,
                           lm_Plane2D);
    } else if (gf->nPar == 5) {
        err = StdMapFitter(vP->m, gf->sigma, pL, p, fit, q, 5,
                           lm_Gauss2DRing);
    } else if (gf->type == 4) {
        err = StdMapFitter(vP->m, gf->sigma, pL, p, fit, q, 6,
                           lm_Quad2D);
    } else {
        err = StdMapFitter(vP->m, gf->sigma, pL, p, fit, q, 6,
                           lm_Gauss2D);
    }
    
    if (err) {
        sprintf(buf, "Error %d in StdMapFitter.", err);
        PostErrorDialog(w, buf);
        return;
    }
    
    gf->g.A = p[0];
    gf->g.x = p[1];
    gf->g.y = p[2];
    if (gf->nPar >= 4) gf->g.maj = p[3];
    if (gf->nPar >= 5) gf->g.min = p[4];
    if (gf->type == 1) gf->g.PA  = fmod(p[5], 2.0*PI);
    if (gf->type == 4) gf->g.PA  = p[5];
    gf->g.uA = q[0];
    gf->g.ux = q[1];
    gf->g.uy = q[2];
    if (gf->nPar >= 4) gf->g.umaj = q[3];
    if (gf->nPar >= 5) gf->g.umin = q[4];
    if (gf->nPar == 6) gf->g.uPA  = q[5];

    if (gf->nPar == 4) {
        b.maj = b.min = p[3];
        b.PA = 0.0;
        draw_beam(gp->gcGauss, p[1], p[2], &b);
    } else if (gf->type == 3 || gf->type == 4) {
    /* do nothing for plane */
    } else if (gf->nPar == 5) {
        b.maj = b.min = 2.0*p[4];
        b.PA = 0.0;
        draw_beam(gp->gcGauss, p[1], p[2], &b);
        b.maj = b.min = 2.0*(p[4] - p[3]/2.0);
        b.PA = 0.0;
        draw_beam(gp->gcGauss, p[1], p[2], &b);
        b.maj = b.min = 2.0*(p[4] + p[3]/2.0);
        b.PA = 0.0;
        draw_beam(gp->gcGauss, p[1], p[2], &b);
    } else {
        b.maj = p[3]; b.min = p[4];
        b.PA = RADTODEG*fmod(p[5], 2.0*PI);
        draw_beam(gp->gcGauss, p[1], p[2], &b);
    }
    
    Update2DimWidgets(gf, 1);
}

static void save2DFitasMap(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int nX, nY, i, j;
    double x, y;
    MAP *m;
    GaussFit2D *gf = NULL;
    string buf;
    
    list *get_maplist();
    MAP *new_map(), *copy_map();

    if (!sf) return;
    
    if (!vP->m) {
        sprintf(buf, "No current map.");
        PostErrorDialog(w, buf);
        return;
    }
    
    gf = (GaussFit2D *)sf->user;
    Scan2DimWidgets(gf);
    
    m = copy_map(get_maplist(), vP->m);
    
    nX = m->i_no;
    nY = m->j_no;
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            x = m->xleft  + (double)i * m->xspacing;
            y = m->ylower + (double)j * m->yspacing;
	    m->d[i][j] = Get2DValue(x, y, gf);
	    m->e[i][j] = 0.0;
	    m->f[i][j] = UNBLANK;
        }
    }
    
    sprintf(m->name, "Fitted %s", vP->m->name);
}

static void Gauss2DTypeCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int menuNo, n;
    GaussFit2D *gf;
    WLIST *wl;
    
    if (!str) return;
    gf = (GaussFit2D *)str; /* Cast the pointer into the correct type */
    
    if (!gf->menu) return;
   
    menuNo = QueryOptionMenuItemNumber(gf->menu, w);
    
    gf->type = menuNo;
    
    if (menuNo == 0) { /* Circular Gaussian */
        gf->nPar = 4;
        wprintf(gf->w[0].l, "%s", Gauss2DCircDescs[0]);
        wprintf(gf->w[1].l, "%s", Gauss2DCircDescs[1]);
        wprintf(gf->w[2].l, "%s", Gauss2DCircDescs[2]);
        wprintf(gf->w[3].l, "%s", Gauss2DCircDescs[3]);
        XtVaSetValues(gf->rowcol, XmNnumColumns, gf->nPar+1, NULL);
        for (n=4; n<6; n++) {
            wl = &(gf->w[n]);
            XtUnmanageChild(wl->l);
            XtUnmanageChild(wl->f);
            XtUnmanageChild(wl->d);
            XtUnmanageChild(wl->e);
        }
   } else if (menuNo == 1) {      /* Elliptic Gaussian */
        gf->nPar = 6;
        wprintf(gf->w[0].l, "%s", Gauss2DDescs[0]);
        wprintf(gf->w[1].l, "%s", Gauss2DDescs[1]);
        wprintf(gf->w[2].l, "%s", Gauss2DDescs[2]);
        wprintf(gf->w[3].l, "%s", Gauss2DDescs[3]);
        wprintf(gf->w[4].l, "%s", Gauss2DDescs[4]);
        XtVaSetValues(gf->rowcol, XmNnumColumns, gf->nPar+1, NULL);
        for (n=4; n<6; n++) {
            wl = &(gf->w[n]);
            XtManageChild(wl->l);
            XtManageChild(wl->f);
            XtManageChild(wl->d);
            XtManageChild(wl->e);
        }
   } else if (menuNo == 2) {      /* Ring Gaussian */
        gf->nPar = 5;
        wprintf(gf->w[0].l, "%s", Gauss2DRingDescs[0]);
        wprintf(gf->w[1].l, "%s", Gauss2DRingDescs[1]);
        wprintf(gf->w[2].l, "%s", Gauss2DRingDescs[2]);
        wprintf(gf->w[3].l, "%s", Gauss2DRingDescs[3]);
        wprintf(gf->w[4].l, "%s", Gauss2DRingDescs[4]);
       /* wprintf(gf->w[5].l, "%s", Gauss2DRingDescs[5]); */
        XtVaSetValues(gf->rowcol, XmNnumColumns, gf->nPar+1, NULL);
        for (n=4; n<6; n++) {
            wl = &(gf->w[n]);
            if (n==4) {
                XtManageChild(wl->l);
                XtManageChild(wl->f);
                XtManageChild(wl->d);
                XtManageChild(wl->e);
            } else {
                XtUnmanageChild(wl->l);
                XtUnmanageChild(wl->f);
                XtUnmanageChild(wl->d);
                XtUnmanageChild(wl->e);
            }
        }
    } else if (menuNo == 3) {      /* Plane */
        gf->nPar = 3;
        wprintf(gf->w[0].l, "%s", Gauss2DPlaneDescs[0]);
        wprintf(gf->w[1].l, "%s", Gauss2DPlaneDescs[1]);
        wprintf(gf->w[2].l, "%s", Gauss2DPlaneDescs[2]);
        XtVaSetValues(gf->rowcol, XmNnumColumns, gf->nPar+1, NULL);
        for (n=3; n<6; n++) {
            wl = &(gf->w[n]);
            XtUnmanageChild(wl->l);
            XtUnmanageChild(wl->f);
            XtUnmanageChild(wl->d);
            XtUnmanageChild(wl->e);
        }
    } else if (menuNo == 4) {      /* Quadratic surface */
        gf->nPar = 6;
        wprintf(gf->w[0].l, "%s", Gauss2DQuadDescs[0]);
        wprintf(gf->w[1].l, "%s", Gauss2DQuadDescs[1]);
        wprintf(gf->w[2].l, "%s", Gauss2DQuadDescs[2]);
        wprintf(gf->w[3].l, "%s", Gauss2DQuadDescs[3]);
        wprintf(gf->w[4].l, "%s", Gauss2DQuadDescs[4]);
        wprintf(gf->w[5].l, "%s", Gauss2DQuadDescs[5]);
        XtVaSetValues(gf->rowcol, XmNnumColumns, gf->nPar+1, NULL);
        for (n=4; n<6; n++) {
            wl = &(gf->w[n]);
            XtManageChild(wl->l);
            XtManageChild(wl->f);
            XtManageChild(wl->d);
            XtManageChild(wl->e);
        }
    }
}

void Post2DimFitDialog(Widget w, Gauss2D *g2, XtPointer cd)
{
    int n, nPar=6;
    Widget parent = w;
    Widget rc, err, rcp, fr, menu, pb, rch;
    GaussFit2D *gf;
    WLIST *wl;
    StdForm *sf;
    
    while (!XtIsWMShell(parent)) parent = XtParent(parent);
    
    if (!vP->m) {
        PostErrorDialog(w, "There is no map to fit 2-dim Gaussian to!\n");
        return;
    }
    
    gf = (GaussFit2D *)XtMalloc(sizeof(GaussFit2D));
    if (!gf) return;
    gf->w = NULL;
    
    gf->w = (WLIST *)XtMalloc(nPar * sizeof(WLIST));
    if(!gf->w) {
        XtFree((char *)gf);
        return;
    }
    
    gf->nPar = nPar;
    gf->type = 1;
    if (g2) {
        gf->g = *g2;
    } else {
        gf->g.A = gf->g.x = gf->g.y = 0.0;
        gf->g.maj = gf->g.min = gf->g.PA = 0.0;
    }
    
    gf->sigma = -1.0;
    
    sf = PostStdFormDialog(parent, "Gaussian 2D fit parameters",
             BUTT_APPLY, (XtCallbackProc)Do2DimFit, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             0, cleanup_2D_dialog);
    sf->user = (XtPointer)gf;
    
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          NULL);

    XtCreateManagedWidget("Map sigma (use if > 0.0)", xmLabelWidgetClass,
                          rc, NULL, 0);
    err = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                rc, NULL, 0);
    gf->error = err;

    fr   = XtVaCreateWidget("frame", xmFrameWidgetClass,
				            rc, XmNshadowType, XmSHADOW_OUT, NULL);
    rcp  = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, fr,
                            XmNorientation, XmHORIZONTAL,
                            XmNnumColumns, nPar+1,
                            XmNadjustLast, False,
                            XmNpacking, XmPACK_COLUMN,
                            NULL);
    gf->rowcol = rcp;
    
    XtCreateManagedWidget("Parameter", xmLabelWidgetClass,
                          rcp, NULL, 0);
    XtCreateManagedWidget("Fitted/fixed", xmLabelWidgetClass,
                          rcp, NULL, 0);
    XtCreateManagedWidget("Value", xmLabelWidgetClass,
                          rcp, NULL, 0);
    XtCreateManagedWidget("Uncertainty", xmLabelWidgetClass,
                          rcp, NULL, 0);

    for (n=0; n<nPar; n++) {
        wl = &(gf->w[n]);
	if (nPar == 3)
            wl->l = XtCreateManagedWidget(Gauss2DPlaneDescs[n],
	                                  xmLabelWidgetClass,
                                          rcp, NULL, 0);
	else
            wl->l = XtCreateManagedWidget((nPar == 4) ? Gauss2DCircDescs[n] :
                                          Gauss2DDescs[n], xmLabelWidgetClass,
                                          rcp, NULL, 0);
        wl->fit = 0;
        wl->f = XtVaCreateManagedWidget(wl->fit ? "Fitted" : "Fixed",
                                        xmToggleButtonWidgetClass,
                                        rcp, XmNset,
                                        wl->fit ? True : False,
                                        NULL);
        wl->d = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                      rcp, NULL, 0);
        wl->e = XtCreateManagedWidget("label", xmLabelWidgetClass,
                                      rcp, NULL, 0);
        XtAddCallback(wl->f, XmNvalueChangedCallback,
                      (XtCallbackProc)ToggleFitCallback, wl);
    }
                                  
    ChangeCallbackDataInMenuItems(Gauss2DTypeMenuData, (char *)gf);
    menu = CreateOptionMenu(rc, &Gauss2DTypeOptionMenu);
    SetDefaultOptionMenuItem(menu, 1);
    gf->menu = menu;

    rch = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                          XmNorientation, XmHORIZONTAL,
                          NULL);
    pb = XtVaCreateManagedWidget("Save current fit as map",
                                 xmPushButtonWidgetClass, rch, NULL);
    XtAddCallback(pb, XmNactivateCallback,
                  (XtCallbackProc)save2DFitasMap, sf);
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(rcp);
    XtManageChild(fr);
    XtManageChild(menu);
    XtManageChild(rc);
    
    ManageDialogCenteredOnPointer(sf->form);
    
    Update2DimWidgets(gf, -1);
}
