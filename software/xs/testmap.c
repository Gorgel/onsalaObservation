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

#define TESTMAP_TYPE_SQUARE  0
#define TESTMAP_TYPE_URING   1
#define TESTMAP_TYPE_GRING   2

/*** External variables ***/

void   PostErrorDialog(Widget, char *);
Widget PostWaitingDialog(Widget, char *, Widget *, int);
void   SetWaitingScale(Widget, int);
void   ManageDialogCenteredOnPointer(Widget);
void   draw_main();
Widget ThreeHorEdit(Widget, Widget *, Widget *, Widget *);
Widget CreateOptionMenu(Widget, MenuBarItem *);
void   SetDefaultOptionMenuItem(Widget, int);

void   wdscanf(Widget, double *);
void   wsscanf(Widget, char *);

/*** Local variables ***/

typedef struct {
    string  name;
    int     nX, nY, type;
    double  xleft, xright, xspacing;
    double  ylower, yupper, yspacing;
    int     nGauss, nClumps;
    Gauss2D g[3];
    unsigned int seed;
    double  sigma;
    Beam    b;
} TestMap;

static TestMap tm;

static void SetRingTypeCallback();
static MenuItem RingTypeData[] = {
   {"Rectangle", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetRingTypeCallback, "0", NULL},
   {"Uniform ring", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetRingTypeCallback, "1", NULL},
   {"Gaussian ring", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetRingTypeCallback, "2", NULL},
EOI};
static MenuBarItem RingTypeMenu = {
   "Clump distribution", ' ', True, RingTypeData
};

void init_testmap()
{
    strcpy(tm.name, "Test map");
    
    tm.nX = 31;
    tm.nY = 31;
    tm.xleft = 150.0;
    tm.xright = -150.0;
    tm.xspacing = (tm.xright - tm.xleft)/(double)(tm.nX-1);
    tm.ylower = -150.0;
    tm.yupper = 150.0;
    tm.yspacing = (tm.yupper - tm.ylower)/(double)(tm.nY-1);
    
    tm.type = TESTMAP_TYPE_URING;
    tm.nGauss = 3;
    tm.nClumps = 1;
    
    tm.g[0].A = 10.0;
    tm.g[0].x = 0.0;
    tm.g[0].y = 30.0;
    tm.g[0].maj = 40.0;
    tm.g[0].min = 40.0;
    tm.g[0].PA  = 0.0;
    
    tm.g[1].A = 10.0;
    tm.g[1].x = 0.0;
    tm.g[1].y = -30.0;
    tm.g[1].maj = 40.0;
    tm.g[1].min = 40.0;
    tm.g[1].PA  = 0.0;
    
    tm.g[2].A = 0.0;
    tm.g[2].x = 0.0;
    tm.g[2].y = 1.5;
    tm.g[2].maj = 34.7;
    tm.g[2].min = 3.2;
    tm.g[2].PA  = 0.0;
    
    tm.seed = 12345;
    tm.sigma = 1.0;
    
    tm.b.maj = 30.0;
    tm.b.min = 30.0;
    tm.b.PA  = 0.0;
}

static double GetTestMapValue(double x0, double y0, Beam *b,
                              Gauss2D *g, int nGauss,
			      Point *p, int nPoint)
{
    int n;
    double b2, d2, s2, s2m, s2M, z=0.0;
    double x, y, u, v, cp, sp, dtau, tau=0.0;
    
/* Can only handle circular beam */
    if (!p) {
	b2 = b->maj * b->min;
	for (n=0; n<nGauss; n++) {
	   cp = cos(g[n].PA/RADTODEG); sp = sin(g[n].PA/RADTODEG);
	   x = x0 - g[n].x; y = y0 - g[n].y;
	   u = x*cp - y*sp;
	   v = x*sp + y*cp;
	   s2 = g[n].maj*g[n].min;
	   s2M = g[n].maj*g[n].maj;
	   s2m = g[n].min*g[n].min;
	   if (n < 2) {
               d2 = u*u/(s2m + b2) + v*v/(s2M + b2);
               z += g[n].A * s2/(s2 + b2) * exp(-ALPHA * d2);
	   } else {
               d2 = sqrt(u*u + v*v) - g[n].maj;
               z += g[n].A * exp(-ALPHA * d2 * d2 /s2m);
	   }
	}
    } else {
	s2M = g[0].maj*g[0].maj;
        for (n=0; n<nPoint; n++) {
	   d2 = (p[n].x - x0)*(p[n].x - x0) + (p[n].y - y0)*(p[n].y - y0);
	   dtau = g[2].A * exp(-ALPHA * d2/s2M);
	   z += (g[0].A - z)*(1.0 - exp(-dtau));
	   tau += dtau;
	}
    }
    
    return z;
}

static void DoTestMap(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int i, j, nX, nY, n, seed;
    double xlef, xrig, xspa, ylow, yupp, yspa, sigma, x, y, val;
    Widget wait=NULL, scale;
    string buf;
    Beam b;
    MAP *m;
    Point *c, *cp;
    TestMap *t = (TestMap *)sf->any;

    void send_line(), wdscanf(), wsscanf(), wiscanf(), SetWatchCursor();
    void MapDraw();
    MAP *new_map();
    list *get_maplist();
    DATE *XS_localtime();
    void XS_RndInit(unsigned int);
    double XS_NormalRnd(double, double);
    double XS_UniformRnd(double, double);
    
    if (!t) return;

    wdscanf(sf->edit[1], &(b.maj));
    wdscanf(sf->edit[2], &(b.min));
    wdscanf(sf->edit[3], &(b.PA));
    /* At the moment we force circular beams, see above */
    b.min = b.maj;
    if (b.min > b.maj) {
        sprintf(buf, "Minor axis of beam error: %f > major axis", b.min);
        PostErrorDialog(w, buf);
        return;
    }
    
    wdscanf(sf->edit[4], &xlef);
    wdscanf(sf->edit[5], &xrig);
    wdscanf(sf->edit[6], &xspa);
    xspa *= -1.0;
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
    
    wdscanf(sf->edit[10], &sigma);
    if (sigma < 0.0) {
        sprintf(buf, "Sigma %f < 0.0", sigma);
        PostErrorDialog(w, buf);
        return;
    }
    
    wiscanf(sf->edit[11], &n);
    if (n < 1) {
        sprintf(buf, "nClumps %d < 0", n);
        PostErrorDialog(w, buf);
        return;
    }
    
    wiscanf(sf->edit[12], &seed);
    if (n < 1) {
        sprintf(buf, "Seed %d < 0", seed);
        PostErrorDialog(w, buf);
        return;
    }
    
    c = (Point *)XtMalloc(n * sizeof(Point));
    if (!c) {
        PostErrorDialog(w, "Out of memory in testmap.");
        return;
    }
    
    t->b = b;
    t->sigma = sigma;
    t->nClumps = n;
    t->seed = seed;

    nX = 1 + NINT((xrig - xlef)/xspa);
    nY = 1 + NINT((yupp - ylow)/yspa);

    m = new_map(get_maplist(), nX, nY);
    
    m->type         = MAP_POSPOS;
    m->coordType    = COORD_TYPE_EQU;
    m->swapped      = 0;
    m->memed        = 0;
    m->interpolated = 0;
    m->original     = NULL;
    m->x0           = 0.0;
    m->y0           = 0.0;
    m->equinox      = 0.0;
    m->epoch        = ' ';
    m->date         = *XS_localtime();
    m->b            = b;
    strcpy(m->molecule, "Test map");
    
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
    
    t->nX = nX;
    t->nY = nY;
    t->xleft = xlef;
    t->xright = xrig;
    t->xspacing = xspa;
    t->ylower = ylow;
    t->yupper = yupp;
    t->yspacing = yspa;

    if (t->nClumps > 50)
        wait = PostWaitingDialog(w, "Creating testmap...", &scale, nX*nY);

    SetWatchCursor(True);
    
    XS_RndInit(t->seed);
        
    /* Generate all clumps */
    if (t->nClumps > 1) {
        for (n=0; n<t->nClumps; n++) {
	    cp = &c[n];
            switch (t->type) {
                case TESTMAP_TYPE_SQUARE:
                    cp->x = XS_UniformRnd(xrig, xlef-xrig);
                    cp->y = XS_UniformRnd(ylow, yupp-ylow);
                    break;
                case TESTMAP_TYPE_URING:
                    {
                        double r = XS_UniformRnd(t->g[2].maj - t->g[2].min/2.0,
                                                 t->g[2].min);
                        double a = XS_UniformRnd(0, 2.0*PI);
                        cp->x = t->g[2].x + r * cos(a);
                        cp->y = t->g[2].y + r * sin(a);
                    }
                    break;
                case TESTMAP_TYPE_GRING:
                    {
                        double r = XS_NormalRnd(t->g[2].maj,
                                                t->g[2].min/sqrt(2.0*ALPHA));
                        double a = XS_UniformRnd(0, 2.0*PI);
                        cp->x = t->g[2].x + r * cos(a);
                        cp->y = t->g[2].y + r * sin(a);
                    }
                    break;
            }
        }
    }
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (wait) SetWaitingScale(scale, j + 1 + i*nY);
            x = xlef + (double)i * xspa;
            y = ylow + (double)j * yspa;
            if (t->nClumps > 1)
            	val = GetTestMapValue(x, y, &b, t->g, 1, c, t->nClumps);
            else
            	val = GetTestMapValue(x, y, &b, t->g, 3, NULL, 0);
            m->f[i][j]  = UNBLANK;
            m->d[i][j]  = XS_NormalRnd(val, sigma);
            m->e[i][j]  = sigma;
            m->sp[i][j] = NULL;
        }
    }
    
    SetWatchCursor(False);
    if (wait) XtDestroyWidget(wait);
    
    XtFree((char *)c);

    wsscanf(sf->edit[0], m->name);
    sprintf(buf, "Test map stored as '%s': %dx%d\n",  m->name, nX, nY);
    send_line(buf);
    
    MapDraw(NULL, m, NULL);
}

static void apply_gauss2d_dialog(Widget parent, StdForm *sf,
                                 XmAnyCallbackStruct cb)
{
    Gauss2D G, *g = (Gauss2D *)sf->any;
    
    void wprintf();
    
    if (!g) return;
    
    wdscanf(sf->edit[0], &G.A);
    if (G.A < 0.0) {
        PostErrorDialog(parent, "Amplitude < 0.0");
        return;
    }
    wdscanf(sf->edit[1], &G.x);
    wdscanf(sf->edit[2], &G.y);
    
    wdscanf(sf->edit[3], &G.maj);
    if (G.maj < 0.0) {
        PostErrorDialog(parent, "Major axis < 0.0");
        return;
    }
    wdscanf(sf->edit[4], &G.min);
    if (G.min > G.maj) {
        PostErrorDialog(parent, "Minor axis > Major axis");
        return;
    }
    if (G.min <= 0.0) G.min = G.maj;
    wdscanf(sf->edit[5], &G.PA);
    if (G.PA < 0.0) G.PA += 360.0;
    if (G.PA >= 360.0) G.PA -= 360.0;
    
    *g = G;
    
    wprintf(sf->edit[0], "%f", g->A);
    wprintf(sf->edit[1], "%f", g->x);
    wprintf(sf->edit[2], "%f", g->y);
    wprintf(sf->edit[3], "%f", g->maj);
    wprintf(sf->edit[4], "%f", g->min);
    wprintf(sf->edit[5], "%f", g->PA);
}

static void apply_gauss2dring_dialog(Widget parent, StdForm *sf,
                                     XmAnyCallbackStruct cb)
{
    Gauss2D G, *g = (Gauss2D *)sf->any;
    
    void wprintf();
    
    if (!g) return;
    
    wdscanf(sf->edit[0], &G.A);
    if (G.A < 0.0) {
        PostErrorDialog(parent, "Amplitude < 0.0");
        return;
    }
    wdscanf(sf->edit[1], &G.x);
    wdscanf(sf->edit[2], &G.y);
    
    wdscanf(sf->edit[3], &G.maj);
    if (G.maj < 0.0) {
        PostErrorDialog(parent, "Major axis < 0.0");
        return;
    }
    wdscanf(sf->edit[4], &G.min);
    if (G.min > G.maj) {
        PostErrorDialog(parent, "Minor axis > Major axis");
        return;
    }
    if (G.min <= 0.0) G.min = G.maj;
    G.PA = 0.0;
    
    *g = G;
    
    wprintf(sf->edit[0], "%f", g->A);
    wprintf(sf->edit[1], "%f", g->x);
    wprintf(sf->edit[2], "%f", g->y);
    wprintf(sf->edit[3], "%f", g->maj);
    wprintf(sf->edit[4], "%f", g->min);
}

static void PostGauss2DDialog(Widget parent, Gauss2D *g, XtPointer cd)
{
    Widget rc;
    StdForm *sf;
    
    void wprintf();
    
   sf = PostStdFormDialog(parent, "2-dimensional Gauss",
             BUTT_APPLY, (XtCallbackProc)apply_gauss2d_dialog, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             6, NULL);
    sf->any = (XtPointer)g;

    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
              NULL);
    
    XtCreateManagedWidget("Amplitude:", xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass, rc, NULL, 0);

    XtCreateManagedWidget("X and Y center offsets [\"]:",
                          xmLabelWidgetClass, rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[1]), &(sf->edit[2]), NULL);

    XtCreateManagedWidget("Major and minor axes [\"], Pos. angle [deg]:",
                          xmLabelWidgetClass, rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[3]), &(sf->edit[4]), &(sf->edit[5]));
    
    ArrangeStdFormDialog(sf, rc);
    
    wprintf(sf->edit[0], "%f", g->A);
    wprintf(sf->edit[1], "%f", g->x);
    wprintf(sf->edit[2], "%f", g->y);
    wprintf(sf->edit[3], "%f", g->maj);
    wprintf(sf->edit[4], "%f", g->min);
    wprintf(sf->edit[5], "%f", g->PA);

    XtManageChild(rc);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void PostGauss2DRingDialog(Widget parent, Gauss2D *g, XtPointer cd)
{
    Widget rc;
    StdForm *sf;
    
    void wprintf();
    
   sf = PostStdFormDialog(parent, "2-dimensional Gaussian ring",
             BUTT_APPLY, (XtCallbackProc)apply_gauss2dring_dialog, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             5, NULL);
    sf->any = (XtPointer)g;

    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
              NULL);
    
    XtCreateManagedWidget("Amplitude:", xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass, rc, NULL, 0);

    XtCreateManagedWidget("X and Y center offsets [\"]:",
                          xmLabelWidgetClass, rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[1]), &(sf->edit[2]), NULL);

    XtCreateManagedWidget("Radius and width of ring [\"]",
                          xmLabelWidgetClass, rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[3]), &(sf->edit[4]), NULL);
    
    ArrangeStdFormDialog(sf, rc);
    
    wprintf(sf->edit[0], "%f", g->A);
    wprintf(sf->edit[1], "%f", g->x);
    wprintf(sf->edit[2], "%f", g->y);
    wprintf(sf->edit[3], "%f", g->maj);
    wprintf(sf->edit[4], "%f", g->min);

    XtManageChild(rc);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void SetRingTypeCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    int n = atoi(s);
    
    if (n != tm.type) {
        tm.type = n;
    }
}

void PostTestMapDialog(Widget parent, char *cmd, XtPointer cd)
{
    Widget w = parent, rc, b1, b2, b3, menu;
    StdForm *sf;
    TestMap *t = &tm;

    void wprintf();

    while (!XtIsWMShell(w))
        w = XtParent(w);
    
    sf = PostStdFormDialog(w, "Test Map Generator",
             BUTT_APPLY, (XtCallbackProc)DoTestMap, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             12, NULL);
    sf->any = (XtPointer)t;

    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          NULL);

    XtCreateManagedWidget("Name of new test map:", xmLabelWidgetClass,
                          rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);

    
    XtCreateManagedWidget("Beam: (major axis [\"], minor axis [\"], PA [deg]):",
                          xmLabelWidgetClass, rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[1]), &(sf->edit[2]), &(sf->edit[3]));

    XtCreateManagedWidget("X: (Left, right, spacing)", xmLabelWidgetClass,
                          rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[4]), &(sf->edit[5]), &(sf->edit[6]));
    
    XtCreateManagedWidget("Y: (Lower, upper, spacing)", xmLabelWidgetClass,
                          rc, NULL, 0);
    ThreeHorEdit(rc, &(sf->edit[7]), &(sf->edit[8]), &(sf->edit[9]));

    b1 = XtCreateManagedWidget("1st Gaussian source...",
                               xmPushButtonWidgetClass, rc, NULL, 0);
    b2 = XtCreateManagedWidget("2nd Gaussian source...",
                               xmPushButtonWidgetClass, rc, NULL, 0);
    b3 = XtCreateManagedWidget("1st Gaussian ring source...",
                               xmPushButtonWidgetClass, rc, NULL, 0);

    XtCreateManagedWidget("Noise:", xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[10] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                         rc, NULL, 0);
    menu = CreateOptionMenu(rc, &RingTypeMenu);
    SetDefaultOptionMenuItem(menu, t->type);

    XtCreateManagedWidget("No of clumps:", xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[11] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                         rc, NULL, 0);

    XtCreateManagedWidget("Random seed:", xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[12] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                         rc, NULL, 0);
    
    XtAddCallback(b1, XmNactivateCallback,
                  (XtCallbackProc)PostGauss2DDialog, &(t->g[0]));
    XtAddCallback(b2, XmNactivateCallback,
                  (XtCallbackProc)PostGauss2DDialog, &(t->g[1]));
    XtAddCallback(b3, XmNactivateCallback,
                  (XtCallbackProc)PostGauss2DRingDialog, &(t->g[2]));
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(menu);
    XtManageChild(rc);

    wprintf(sf->edit[0], "%s", t->name);
    wprintf(sf->edit[1], "%f", t->b.maj);
    wprintf(sf->edit[2], "%f", t->b.min);
    wprintf(sf->edit[3], "%f", t->b.PA);
    wprintf(sf->edit[4], "%f", t->xleft);
    wprintf(sf->edit[5], "%f", t->xright);
    wprintf(sf->edit[6], "%f", -t->xspacing);
    wprintf(sf->edit[7], "%f", t->ylower);
    wprintf(sf->edit[8], "%f", t->yupper);
    wprintf(sf->edit[9], "%f", t->yspacing);
    wprintf(sf->edit[10], "%f", t->sigma);
    wprintf(sf->edit[11], "%d", t->nClumps);
    wprintf(sf->edit[12], "%d", t->seed);
    
    ManageDialogCenteredOnPointer(sf->form);
}
