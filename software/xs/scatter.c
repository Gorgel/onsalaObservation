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
#include <string.h>

#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/Frame.h>

#include "defines.h"
#include "global_structs.h"
#include "menus.h"
#include "dialogs.h"

#ifdef HAVE_LIBPGPLOT
#include "cpgplot.h"
#endif

#include "scatter.h"

double  *AllocDoubleVector(int);
void     FreeDoubleVector(double *);
int     *AllocIntVector(int);
void     FreeIntVector(int *);

void init_scatter_data()
{
    int n;
    
    scat.m1 = NULL;
    scat.m2 = NULL;
    scat.s  = NULL;
    scat.p  = NULL;
    scat.x0 = 0.0;
    scat.y0 = 0.0;
    scat.epoch = ' ';
    scat.equinox = 0.0;
    scat.nData = 0;
    scat.x  = NULL;
    scat.y  = NULL;
    scat.ex = NULL;
    scat.ey = NULL;
    scat.sp = NULL;
    scat.t  = NULL;
    scat.swapped = 0;
    scat.xtype = XTYPE_SCA_DIST;
    scat.ytype = YTYPE_SCA_INT;
    scat.xmin = 0.0;
    scat.xmax = 0.0;
    scat.ymin = 0.0;
    scat.ymax = 0.0;
    scat.single = 0;
    strcpy(scat.name, "");
    strcpy(scat.molecule, "");
    
    sopt.xRef = 0.0;
    sopt.yRef = 0.0;
    sopt.PA1 = 0.0;
    sopt.PA2 = 360.0;
    sopt.r1  = 0.0;
    sopt.r2  = -1.0;
    sopt.xtype = scat.xtype;
    sopt.ytype = scat.ytype;
    sopt.stat = 0;
    sopt.fit = SCATTER_NO_FIT;
    sopt.join = 0;
    sopt.label = 1;
    sopt.yerror = -1.0;
    sopt.updateError = 0;
    sopt.nPos = 50;
    sopt.Width = 30.0;
    
    for (n=0; n<MAX_PAR; n++) {
        spar[n].p = 0.0;
        spar[n].q = 0.0;
        spar[n].fit = 0;
        spar[n].f = spar[n].e = spar[n].error = NULL;
    }
    
    dotSize = 2;
    dotType = DOT_PLUSMARK;
}

int *GetScatterDotType()
{
    return &dotType;
}

int *GetScatterDotSize()
{
    return &dotSize;
}

char *GetScatterTypeStr(char axis, int type)
{
    int i=0;
    static char *desc;
    
    if (axis == 'x') {
        while ((desc = XTypeMenuData[i].label)) {
            if (type == atoi((char *)XTypeMenuData[i].callback_data)) break;
            i++;
        }
    } else {
        while ((desc = YTypeMenuData[i].label)) {
            if (type == atoi((char *)YTypeMenuData[i].callback_data)) break;
            i++;
        }
    }
    
    return desc;
}

void ZeroScanInAllScatters(scanPtr s)
{
    int n;
    
    void ZeroScanInScatterList(scanPtr);
    
    for (n=0; n<scat.nData; n++) {
        if (scat.sp[n] == s) {
            scat.sp[n] = NULL;
        }
    }
    ZeroScanInScatterList(s);
}

void MakeSingleSpeScatterPlot(Widget w, char *cmd, XtPointer cd)
{
    int n;
    string buf;
    scanPtr s = vP->s;
    int prev_mode = vP->mode;
    
    double chan2xunit();
    void set_scatter_minmax();
    int AllocScatter();
    void SetDefWindow(), draw_main(), SetStdView();

    if (!s) {
        sprintf(buf, "No current spectrum available.");
        PostWarningDialog(NULL, buf);
        return;
    }
    if (AllocScatter(s->nChan)) {
        sprintf(buf, "Couldn't allocate scatter data (%d).", s->nChan);
        PostErrorDialog(NULL, buf);
        return;
    }
    
    scat.x0 = s->x0;
    scat.y0 = s->y0;
    scat.epoch = s->epoch;
    scat.equinox = s->equinox;
    
    strcpy(scat.name, s->name);
    strcpy(scat.molecule, s->molecule);
    scat.date = s->date;
    
    scat.s = s;
    scat.m1 = NULL;
    scat.m2 = NULL;
    scat.p  = NULL;
    
    scat.nData = s->nChan;
    if (vP->xunit == UNIT_FRE) {
        scat.xtype = XTYPE_SCA_FREQ;
    } else if (vP->xunit == UNIT_FOFF) {
        scat.xtype = XTYPE_SCA_FOFF;
    } else if (vP->xunit == UNIT_FMHZ) {
        scat.xtype = XTYPE_SCA_FMHZ;
    } else if (vP->xunit == UNIT_CHA) {
        scat.xtype = XTYPE_SCA_CHAN;
    } else {
        scat.xtype = XTYPE_SCA_VELO;
    }
    scat.ytype = YTYPE_SCA_TEMP;

    for (n=0; n<s->nChan; n++) {
        scat.x[n] = chan2xunit(n);
        scat.y[n] = s->d[n];
        scat.ex[n] = 0.0;
        scat.ey[n] = s->e[n];
        scat.sp[n] = NULL;
        scat.t[n]  = 0;
    }
    
    scat.single = 1;
    
    set_scatter_minmax(&scat);
    
    SetViewMode(SHOW_SCATTER, vP->s, vP->m, &scat);
    
    if (prev_mode == SHOW_POSPOS) SetStdView();
    SetDefWindow(SCALE_BOTH);
    draw_main();
}

void MakeSpeScatterPlot(DataSetPtr dsp, int xtype, int ytype)
{
    int n = count_scans(dsp);
    string buf;
    list curr = NULL;
    double x, y, dv, c1, c2, RAOff, DecOff;
    scanPtr p;
    
    int AllocScatter();
    void set_scatter_minmax();
    double JulianDay();
    double ModifiedJulianDay();
    double SpecUnitConv();
    void GetEquOffsets(DATE *, double, double, double, double, double, double,
                       double *, double *);
    
    if (AllocScatter(n)) {
        sprintf(buf, "Couldn't allocate scatter data (%d).", n);
        PostErrorDialog(NULL, buf);
        return;
    }
    
    scat.m1 = NULL;
    scat.m2 = NULL;
    scat.p  = NULL;
    
    scat.nData = n;
    scat.dsp   = dsp;
    sopt.xtype = scat.xtype = xtype;
    sopt.ytype = scat.ytype = ytype;
    
    scat.single = 0;

    n = 0;
    while ( (curr = scan_iterator(curr, dsp)) != NULL) {
        p = (scanPtr)DATA(curr);
        if (n == 0) {
            scat.x0 = p->x0;
            scat.y0 = p->y0;
            scat.epoch = p->epoch;
            scat.equinox = p->equinox;

            strcpy(scat.name, p->name);
            strcpy(scat.molecule, p->molecule);
            scat.date = p->date;

            scat.s = p;
        }
        scat.ex[n] = 0.0;
        scat.ey[n] = 0.0;
        scat.sp[n] = p;
        scat.t[n] = 0;
        switch (xtype) {
            case XTYPE_SCA_NO:
                scat.x[n] = (double)(n+1);
                break;
            case XTYPE_SCA_SCAN:
                scat.x[n] = (double)(p->scan_no) +(double)(p->subscan)/10000.0;
                break;
            case XTYPE_SCA_RECT:
                scat.x[n] = p->x0 * RADTOHR;
                break;
            case XTYPE_SCA_DECL:
                scat.x[n] = p->y0 * RADTODEG;
                break;
            case XTYPE_SCA_RA:
                if (dsp->sequence) {
                    scat.x[n] = p->tx - sopt.xRef;
                } else {
                    scat.x[n] = p->xoffset - sopt.xRef;
                }
                break;
            case XTYPE_SCA_DEC:
                if (dsp->sequence) {
                    scat.x[n] = p->ty - sopt.yRef;
                } else {
                    scat.x[n] = p->yoffset - sopt.yRef;
                }
                break;
            case XTYPE_SCA_DIST:
                if (dsp->sequence) {
                    x = p->tx - sopt.xRef;
                    y = p->ty - sopt.yRef;
                } else {
                    x = p->xoffset - sopt.xRef;
                    y = p->yoffset - sopt.yRef;
                }
                scat.x[n] = sqrt(x*x + y*y);
                break;
            case XTYPE_SCA_POSANG:
                if (dsp->sequence) {
                    x = p->tx - sopt.xRef;
                    y = p->ty - sopt.yRef;
                } else {
                    x = p->xoffset - sopt.xRef;
                    y = p->yoffset - sopt.yRef;
                }
                if (x == 0.0 && y == 0.0)
                    scat.x[n] = 0.0;
                else
                    scat.x[n] = RADTODEG * atan2(x, y);
                break;
            case XTYPE_SCA_OSOAZ:
                scat.x[n] = p->az;
                break;
            case XTYPE_SCA_AZ:
                scat.x[n] = p->az + 180.0;
                break;
            case XTYPE_SCA_EL:
                scat.x[n] = p->el;
                break;
            case XTYPE_SCA_AZOFF:
                scat.x[n] = p->aoffset;
                break;
            case XTYPE_SCA_ELOFF:
                scat.x[n] = p->eoffset;
                break;
            case XTYPE_SCA_EQDIST:
                x = p->aoffset - sopt.xRef;
                y = p->eoffset - sopt.yRef;
                scat.x[n] = sqrt(x*x + y*y);
                break;
            case XTYPE_SCA_INT:
                scat.x[n]  = p->mom.iint;
                scat.ex[n] = p->mom.iunc;
                break;
            case XTYPE_SCA_TIME:
                scat.x[n]  = p->int_time;
                scat.ex[n] = 0.1;
                break;
            case XTYPE_SCA_TSQRT:
                scat.x[n]  = 0.0;
		x = 1.0e9 * fabs(p->freqres) * p->int_time;
                if (x > 0.0) scat.x[n]  = p->tsys/sqrt(x);
                scat.ex[n] = 0.1;
                break;
            case XTYPE_SCA_MEAN:
                dv = fabs(p->velres)*(double)(p->mom.nchan);
                scat.x[n] = p->mom.mean;
                if (dv > 0.0) scat.ex[n] = p->mom.iunc/dv;
                break;
            case XTYPE_SCA_SIGMA:
                scat.x[n] = p->mom.sigma;
                break;
            case XTYPE_SCA_TSYS:
                scat.x[n] = p->tsys;
                break;
            case XTYPE_SCA_TAU:
                scat.x[n] = p->tau;
                break;
            case XTYPE_SCA_EXPTAU:
                scat.x[n] = exp(p->tau)-1.0;
                break;
            case XTYPE_SCA_DATE:
                scat.x[n] = (double)(p->date.Year - 1900)*10000.0 +
                            (double)(p->date.Month)*100.0 +
                            (double)(p->date.Day) +
                            (double)(p->date.Hour)/100.0 +
                            (double)(p->date.Min)/10000.0 +
                            (double)(p->date.Sec)/1000000.0;
                break;
            case XTYPE_SCA_UT:
                scat.x[n] = (double)(p->date.Hour) +
                            (double)(p->date.Min)/60.0 +
                            (double)(p->date.Sec)/3600.0;
                break;
            case XTYPE_SCA_JD:
                scat.x[n] = JulianDay(&(p->date));
                break;
            case XTYPE_SCA_MJD:
                scat.x[n] = ModifiedJulianDay(&(p->date));
                break;
            case XTYPE_SCA_GAMP:
                scat.x[n] = p->g.amp;
                scat.ex[n] = p->g.uamp;
                break;
            case XTYPE_SCA_GWID:
                c1 = p->g.cen - p->g.wid/2.0;
                c2 = p->g.cen + p->g.wid/2.0;
                scat.x[n] = fabs(SpecUnitConv(UNIT_VEL, UNIT_CHA, c2)-
                                 SpecUnitConv(UNIT_VEL, UNIT_CHA, c1));
                c1 = p->g.cen - p->g.uwid/2.0;
                c2 = p->g.cen + p->g.uwid/2.0;
                scat.ex[n] = fabs(SpecUnitConv(UNIT_VEL, UNIT_CHA, c2)-
                                  SpecUnitConv(UNIT_VEL, UNIT_CHA, c1));
                break;
            case XTYPE_SCA_GCEN:
                scat.x[n] = SpecUnitConv(UNIT_VEL, UNIT_CHA, p->g.cen);
                c1 = p->g.cen - p->g.ucen/2.0;
                c2 = p->g.cen + p->g.ucen/2.0;
                scat.ex[n] = fabs(SpecUnitConv(UNIT_VEL, UNIT_CHA, c2)-
                                  SpecUnitConv(UNIT_VEL, UNIT_CHA, c1));
                break;
            case XTYPE_SCA_POL0:
                scat.x[n] = p->coeffs[0];
                scat.ex[n] = p->mom.sigma;
                break;
            case XTYPE_SCA_POL1:
                scat.x[n] = p->coeffs[1];
                scat.ex[n] = p->mom.sigma;
                break;
            case XTYPE_SCA_VCENT:
                scat.x[n] = p->mom.vcent;
                scat.ex[n] = p->mom.sigma;
                break;
            case XTYPE_SCA_V2MOM:
                scat.x[n] = p->mom.v2mom;
                scat.ex[n] = p->mom.sigma;
                break;
            case XTYPE_SCA_VELO:
                scat.x[n] = p->vel0 + (double)(p->nChan/2) * p->velres;
                scat.ex[n] = 0.0;
                break;
            case XTYPE_SCA_FREQ:
                scat.x[n] = (p->freq0 + p->freqn)/2.0;
                scat.ex[n] = 0.0;
                break;
            case XTYPE_SCA_VRES:
                scat.x[n] = p->velres;
                scat.ex[n] = 0.0;
                break;
            case XTYPE_SCA_FRES:
                scat.x[n] = p->freqres * 1.0e6;
                scat.ex[n] = 0.0;
                break;
            case XTYPE_SCA_TMIN:
                scat.x[n] = p->mom.TMin;
                scat.ex[n] = p->mom.sigma;
                break;
            case XTYPE_SCA_TMAX:
                scat.x[n] = p->mom.TMax;
                scat.ex[n] = p->mom.sigma;
                break;
            case XTYPE_SCA_POLA_EL:
                GetEquOffsets(&(p->LST), (p->az+180.0)/RADTODEG, p->el/RADTODEG,
                              0.0, 10.0,
                              p->x0, p->y0,
                              &RAOff, &DecOff);
                scat.x[n] = RADTODEG*atan2(-RAOff, DecOff);
                scat.ex[n] = 0.0;
                break;
            case XTYPE_SCA_POLA_AZ:
                GetEquOffsets(&(p->LST), (p->az+180.0)/RADTODEG, p->el/RADTODEG,
                              10.0, 0.0,
                              p->x0, p->y0,
                              &RAOff, &DecOff);
                scat.x[n] = RADTODEG*atan2(-RAOff, DecOff);
                scat.ex[n] = 0.0;
                break;
            case XTYPE_SCA_COSPA:
                GetEquOffsets(&(p->LST), (p->az+180.0)/RADTODEG, p->el/RADTODEG,
                              0.0, 10.0,
                              p->x0, p->y0,
                              &RAOff, &DecOff);
                scat.x[n] = DecOff/10.0;
                scat.ex[n] = 0.0;
                break;
            case XTYPE_SCA_BEFF:
                scat.x[n] = p->beameff;
                scat.ex[n] = p->beameff * 0.1;
                break;
            case XTYPE_SCA_AIRMASS:
	        scat.x[n] = 0.0;
		if (p->el != 0.0) scat.x[n] = 1.0/sin(p->el/RADTODEG);
                scat.ex[n] = 0.0;
                break;
            case XTYPE_SCA_TAIR:
		scat.x[n] = p->tair;
                scat.ex[n] = 0.0;
                break;
            case XTYPE_SCA_PAIR:
		scat.x[n] = p->pair;
                scat.ex[n] = 0.0;
                break;
            case XTYPE_SCA_RAIR:
		scat.x[n] = p->rair;
                scat.ex[n] = 0.0;
                break;
        }
        switch (ytype) {
            case YTYPE_SCA_NO:
                scat.y[n] = (double)(n+1);
                break;
            case YTYPE_SCA_SCAN:
                scat.y[n] = (double)(p->scan_no) + (double)(p->subscan)/10000.0;
                break;
            case YTYPE_SCA_RECT:
                scat.y[n] = p->x0 * RADTOHR;
                break;
            case YTYPE_SCA_DECL:
                scat.y[n] = p->y0 * RADTODEG;
                break;
            case YTYPE_SCA_RA:
                if (dsp->sequence) {
                    scat.y[n] = p->tx - sopt.xRef;
                } else {
                    scat.y[n] = p->xoffset - sopt.xRef;
                }
                break;
            case YTYPE_SCA_DEC:
                if (dsp->sequence) {
                    scat.y[n] = p->ty - sopt.yRef;
                } else {
                    scat.y[n] = p->yoffset - sopt.yRef;
                }
                break;
            case YTYPE_SCA_DIST:
                if (dsp->sequence) {
                    x = p->tx - sopt.xRef;
                    y = p->ty - sopt.yRef;
                } else {
                    x = p->xoffset - sopt.xRef;
                    y = p->yoffset - sopt.yRef;
                }
                scat.y[n] = sqrt(x*x + y*y);
                break;
            case YTYPE_SCA_POSANG:
                if (dsp->sequence) {
                    x = p->tx - sopt.xRef;
                    y = p->ty - sopt.yRef;
                } else {
                    x = p->xoffset - sopt.xRef;
                    y = p->yoffset - sopt.yRef;
                }
                if (x == 0.0 && y == 0.0)
                    scat.y[n] = 0.0;
                else
                    scat.y[n] = RADTODEG * atan2(x, y);
                break;
            case YTYPE_SCA_OSOAZ:
                scat.y[n] = p->az;
                break;
            case YTYPE_SCA_AZ:
                scat.y[n] = p->az + 180.0;
                break;
            case YTYPE_SCA_EL:
                scat.y[n] = p->el;
                break;
            case YTYPE_SCA_AZOFF:
                scat.y[n] = p->aoffset;
                break;
            case YTYPE_SCA_ELOFF:
                scat.y[n] = p->eoffset;
                break;
            case YTYPE_SCA_EQDIST:
                x = p->aoffset - sopt.xRef;
                y = p->eoffset - sopt.yRef;
                scat.y[n] = sqrt(x*x + y*y);
                break;
            case YTYPE_SCA_TIME:
                scat.y[n]  = p->int_time;
                scat.ey[n] = 0.1;
                break;
            case YTYPE_SCA_TSQRT:
                scat.y[n]  = 0.0;
		y = 1.0e9 * fabs(p->freqres) * p->int_time;
                if (y > 0.0) scat.y[n]  = p->tsys/sqrt(y);
                scat.ey[n] = 0.1;
                break;
            case YTYPE_SCA_INT:
                scat.y[n]  = p->mom.iint;
                scat.ey[n] = p->mom.iunc;
                break;
            case YTYPE_SCA_MEAN:
                dv = fabs(p->velres)*(double)(p->mom.nchan);
                scat.y[n] = p->mom.mean;
                if (dv > 0.0) scat.ey[n] = p->mom.iunc/dv;
                break;
            case YTYPE_SCA_SIGMA:
                scat.y[n] = p->mom.sigma;
                break;
            case YTYPE_SCA_TSYS:
                scat.y[n] = p->tsys;
                break;
            case YTYPE_SCA_TAU:
                scat.y[n] = p->tau;
                break;
            case YTYPE_SCA_EXPTAU:
                scat.y[n] = exp(p->tau)-1;
                break;
            case YTYPE_SCA_DATE:
                scat.y[n] = (double)(p->date.Year - 1900)*10000.0 +
                            (double)(p->date.Month)*100.0 +
                            (double)(p->date.Day) +
                            (double)(p->date.Hour)/100.0 +
                            (double)(p->date.Min)/10000.0 +
                            (double)(p->date.Sec)/1000000.0;
                break;
            case YTYPE_SCA_UT:
                scat.y[n] = (double)(p->date.Hour) +
                            (double)(p->date.Min)/60.0 +
                            (double)(p->date.Sec)/3600.0;
                break;
            case YTYPE_SCA_JD:
                scat.y[n] = JulianDay(&(p->date));
                break;
            case YTYPE_SCA_MJD:
                scat.y[n] = ModifiedJulianDay(&(p->date));
                break;
            case YTYPE_SCA_GAMP:
                scat.y[n] = p->g.amp;
                scat.ey[n] = p->g.uamp;
                break;
            case YTYPE_SCA_GWID:
                c1 = p->g.cen - p->g.wid/2.0;
                c2 = p->g.cen + p->g.wid/2.0;
                scat.y[n] = fabs(SpecUnitConv(UNIT_VEL, UNIT_CHA, c2)-
                                 SpecUnitConv(UNIT_VEL, UNIT_CHA, c1));
                c1 = p->g.cen - p->g.uwid/2.0;
                c2 = p->g.cen + p->g.uwid/2.0;
                scat.ey[n] = fabs(SpecUnitConv(UNIT_VEL, UNIT_CHA, c2)-
                                  SpecUnitConv(UNIT_VEL, UNIT_CHA, c1));
                break;
            case YTYPE_SCA_GCEN:
                scat.y[n] = SpecUnitConv(UNIT_VEL, UNIT_CHA, p->g.cen);
                c1 = p->g.cen - p->g.ucen/2.0;
                c2 = p->g.cen + p->g.ucen/2.0;
                scat.ey[n] = fabs(SpecUnitConv(UNIT_VEL, UNIT_CHA, c2)-
                                  SpecUnitConv(UNIT_VEL, UNIT_CHA, c1));
                break;
            case YTYPE_SCA_POL0:
                scat.y[n] = p->coeffs[0];
                scat.ey[n] = p->mom.sigma;
                break;
            case YTYPE_SCA_POL1:
                scat.y[n] = p->coeffs[1];
                scat.ey[n] = p->mom.sigma;
                break;
            case YTYPE_SCA_VCENT:
                scat.y[n] = p->mom.vcent;
                scat.ey[n] = p->mom.sigma;
                break;
            case YTYPE_SCA_V2MOM:
                scat.y[n] = p->mom.v2mom;
                scat.ey[n] = p->mom.sigma;
                break;
            case YTYPE_SCA_VELO:
                scat.y[n] = p->vel0 + (double)(p->nChan/2) * p->velres;
                scat.ey[n] = 0.0;
                break;
            case YTYPE_SCA_FREQ:
                scat.y[n] = (p->freq0 + p->freqn)/2.0;
                scat.ey[n] = 0.0;
                break;
            case YTYPE_SCA_VRES:
                scat.y[n] = p->velres;
                scat.ey[n] = 0.0;
                break;
            case YTYPE_SCA_FRES:
                scat.y[n] = p->freqres * 1.0e6;
                scat.ey[n] = 0.0;
                break;
            case YTYPE_SCA_TMIN:
                scat.y[n] = p->mom.TMin;
                scat.ey[n] = p->mom.sigma;
                break;
            case YTYPE_SCA_TMAX:
                scat.y[n] = p->mom.TMax;
                scat.ey[n] = p->mom.sigma;
                break;
            case YTYPE_SCA_POLA_EL:
                GetEquOffsets(&(p->LST), (p->az+180.0)/RADTODEG, p->el/RADTODEG,
                              0.0, 10.0,
                              p->x0, p->y0,
                              &RAOff, &DecOff);
                scat.y[n] = RADTODEG*atan2(-RAOff, DecOff);
                scat.ey[n] = 0.0;
                break;
            case YTYPE_SCA_POLA_AZ:
                GetEquOffsets(&(p->LST), (p->az+180.0)/RADTODEG, p->el/RADTODEG,
                              10.0, 0.0,
                              p->x0, p->y0,
                              &RAOff, &DecOff);
                scat.y[n] = RADTODEG*atan2(-RAOff, DecOff);
                scat.ey[n] = 0.0;
                break;
            case YTYPE_SCA_COSPA:
                GetEquOffsets(&(p->LST), (p->az+180.0)/RADTODEG, p->el/RADTODEG,
                              0.0, 10.0,
                              p->x0, p->y0,
                              &RAOff, &DecOff);
                scat.y[n] = DecOff/10.0;
                scat.ey[n] = 0.0;
                break;
            case YTYPE_SCA_BEFF:
                scat.y[n] = p->beameff;
                scat.ey[n] = p->beameff * 0.1;
                break;
            case YTYPE_SCA_AIRMASS:
	        scat.y[n] = 0.0;
                if (p->el != 0.0) scat.y[n] = 1.0/sin(p->el/RADTODEG);
                scat.ey[n] = 0.0;
                break;
            case YTYPE_SCA_TAIR:
		scat.y[n] = p->tair;
                scat.ey[n] = 0.0;
                break;
            case YTYPE_SCA_PAIR:
		scat.y[n] = p->pair;
                scat.ey[n] = 0.0;
                break;
            case YTYPE_SCA_RAIR:
		scat.y[n] = p->rair;
                scat.ey[n] = 0.0;
                break;
        }
        n++;
    }
    
    scat.nData = n;
    
    set_scatter_minmax(&scat);
}

static int check_scatter_point(double x, double y, double *r, double *t)
{
    double ra, de, radius, theta;
    
    ra = x - sopt.xRef;
    de = y - sopt.yRef;
    radius = sqrt(ra*ra + de*de);
    
    if (ra == 0.0 && de == 0.0)
        theta = 0.0;
    else
        theta = RADTODEG * atan2(ra, de);
    if (theta < 0.0) theta += 360.0;
    if (theta > 360.0) theta -= 360.0;
    
    if (r) *r = radius;
    if (t) *t = theta;
    
    if (sopt.r2 > sopt.r1) {
        if (radius < sopt.r1 || radius > sopt.r2) return 0;
    }
    
    if (sopt.PA2 > sopt.PA1) {
        if (theta < sopt.PA1 || theta > sopt.PA2) return 0;
    }
    
    return 1;
}

static int GetMapGaussValue(MAP *m, Point *p, double width,
                            double *d, double *e)
{
    int i, j, nX, nY;
    double x, y, d2, w;
    double v=0.0, verr=0.0, wSum=0.0;
    
    if (!m || !p || width == 0.0) return 1;
    
    nX = m->i_no;
    nY = m->j_no;
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (m->f[i][j] <= BLANK) continue;
            x = m->xleft  + (double)(i) * m->xspacing - p->x;
            y = m->ylower + (double)(j) * m->yspacing - p->y;
            d2 = x*x + y*y;
            w = exp(-ALPHA*d2/width/width);
            wSum += w;
            v += w * m->d[i][j];
            if (m->e[i][j] != 0.0) {
                verr += w / m->e[i][j] / m->e[i][j];
            }
        }
    }
    
    if (wSum == 0.0) return 2;
    
    v /= wSum;
    if (verr != 0.0) verr = sqrt(wSum/verr);
    
    if (d) *d = v;
    if (e) *e = verr;
    
    return 0;
}

void MakePolyLineCutScatterPlot(MAP *m, PolyLine *pLine)
{
    int n, nSca, nPos = sopt.nPos;
    double d, v, verr, width = sopt.Width;
    string buf;
    Point *p = NULL;
    PolyLine *pl = NULL;
    
    int AllocScatter();
    void set_scatter_minmax();
    PolyLine *GetFirstPolyLine(Point *);
    Point *StepAlongPolyLine(PolyLine *, int, double *, Point *);
    
    if (!m) return;
    
    if (!pLine) {
        pl = GetFirstPolyLine(NULL);
    } else {
        pl = pLine;
    }
    
    if (!pl) {
        PostErrorDialog(NULL, "There are no polylines.");
        return;
    }
    
    nSca = 0;
    while ((p = StepAlongPolyLine(pl, nPos, &d, p)) != NULL && nSca < nPos) {
        if (GetMapGaussValue(m, p, width, NULL, NULL)) continue;
        nSca++;
    }
    
    if (AllocScatter(nSca)) {
        sprintf(buf, "Couldn't allocate scatter data (%d).", nSca);
        PostErrorDialog(NULL, buf);
        return;
    }
    
    n = 0;
    p = NULL;
    while ((p = StepAlongPolyLine(pl, nPos, &d, p)) != NULL && n < nSca) {
        if (GetMapGaussValue(m, p, width, &v, &verr)) continue;
        scat.x[n]  = d;
        scat.ex[n] = 0.0;
        scat.y[n]  = v;
        scat.ey[n] = verr;
        scat.sp[n] = NULL;
        scat.t[n] = 0;
        n++;
    }
    
    switch (m->zType) {
        case ZTYPE_MOMENT:
            scat.ytype = YTYPE_SCA_INT;
            break;
        case ZTYPE_MEAN:
            scat.ytype = YTYPE_SCA_MEAN;
            break;
        case ZTYPE_TRMS:
            scat.ytype = YTYPE_SCA_SIGMA;
            break;
        case ZTYPE_TMIN:
            scat.ytype = YTYPE_SCA_TMIN;
            break;
        case ZTYPE_TMAX:
            scat.ytype = YTYPE_SCA_TMAX;
            break;
        default:
            scat.ytype = YTYPE_SCA_GAMP + m->zType - ZTYPE_GAMP;
            break;
    }
    scat.xtype = XTYPE_SCA_DIST;
    
    scat.x0 = m->x0;
    scat.y0 = m->y0;
    scat.epoch = m->epoch;
    scat.equinox = m->equinox;
    
    strcpy(scat.name, m->name);
    strcpy(scat.molecule, m->molecule);
    scat.date = m->date;
    
    set_scatter_minmax(&scat);
    
    scat.p  = pl;
    scat.m1 = m;
    scat.m2 = NULL;
    scat.s  = NULL;
    
    scat.single  = 0;
}

void MakeMapScatterPlot(MAP *m, char *cmd)
{
    int i, j, nX, nY, n;
    double radius, PA;
    static string prev_cmd = "Distance";
    string buf;
    
    void set_scatter_minmax();
    int AllocScatter();
    
    if (!m) return;
    
    nX = m->i_no;
    nY = m->j_no;
    
    if (cmd) strcpy(prev_cmd, cmd);
    
    n = 0;
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (m->f[i][j] <= BLANK) continue;
            if (!check_scatter_point(m->xleft  + (double)(i) * m->xspacing,
                                     m->ylower + (double)(j) * m->yspacing,
                                     NULL, NULL)) continue;
            n++;
        }
    }
    
    if (AllocScatter(n)) {
        sprintf(buf, "Couldn't allocate scatter data (%d).", n);
        PostErrorDialog(NULL, buf);
        return;
    }
    
    switch (m->zType) {
        case ZTYPE_MOMENT:
            scat.ytype = YTYPE_SCA_INT;
            break;
        case ZTYPE_MEAN:
            scat.ytype = YTYPE_SCA_MEAN;
            break;
        case ZTYPE_TRMS:
            scat.ytype = YTYPE_SCA_SIGMA;
            break;
        case ZTYPE_TMIN:
            scat.ytype = YTYPE_SCA_TMIN;
            break;
        case ZTYPE_TMAX:
            scat.ytype = YTYPE_SCA_TMAX;
            break;
        default:
            scat.ytype = YTYPE_SCA_GAMP + m->zType - ZTYPE_GAMP;
            break;
    }
    
    if (strcmp(prev_cmd, "PosAngle") == 0) {
        scat.xtype = XTYPE_SCA_POSANG;
    } else if (strcmp(prev_cmd, "x-axis") == 0) {
        scat.xtype = XTYPE_SCA_AZ;
    } else if (strcmp(prev_cmd, "y-axis") == 0) {
        scat.xtype = XTYPE_SCA_EL;
    } else {
        scat.xtype = XTYPE_SCA_DIST;
    }
    
    n = 0;
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (m->f[i][j] <= BLANK) continue;
            if (!check_scatter_point(m->xleft  + (double)(i) * m->xspacing,
                                     m->ylower + (double)(j) * m->yspacing,
                                     &radius, &PA)) continue;
            if (scat.xtype == XTYPE_SCA_POSANG)
                scat.x[n] = PA;
            else if (scat.xtype == XTYPE_SCA_AZ)
                scat.x[n] = m->xleft  + (double)(i) * m->xspacing;
            else if (scat.xtype == XTYPE_SCA_EL)
                scat.x[n] = m->ylower + (double)(j) * m->yspacing;
            else
                scat.x[n] = radius;
            scat.ex[n] = 0.0;
            scat.y[n] = m->d[i][j];
            if (m->e[i][j] == UNDEF)
                scat.ey[n] = 0.0;
            else
                scat.ey[n] = m->e[i][j];
            scat.sp[n] = m->sp[i][j];
            scat.t[n] = 0;
            n++;
        }
    }
    
    scat.x0 = sopt.xRef;
    scat.y0 = sopt.yRef;
    scat.epoch = m->epoch;
    scat.equinox = m->equinox;
    
    strcpy(scat.name, m->name);
    strcpy(scat.molecule, m->molecule);
    scat.date = m->date;
    
    set_scatter_minmax(&scat);
    
    scat.m1 = m;
    scat.m2 = NULL;
    scat.s  = NULL;
    scat.p  = NULL;
    
    scat.single  = 0;
}

void ViewCurrentMapAsScatterPlot(Widget wid, char *cmd, XtPointer cd)
{
    int prev_mode = vP->mode;
    string buf;
    
    void SetDefWindow();
    void send_line(), draw_main(), SetStdView();
    
    if (!vP->m) {
        strcpy(buf, "There is no current map to view as a scatter plot.");
        PostErrorDialog(wid, buf);
        return;
    }
    
    if (strcmp(cmd, "PolyLine") == 0) {
        MakePolyLineCutScatterPlot(vP->m, NULL);
    } else {
        MakeMapScatterPlot(vP->m, cmd);
    }
    
    SetViewMode(SHOW_SCATTER, vP->s, vP->m, &scat);
    if (prev_mode == SHOW_POSPOS) SetStdView();
    SetDefWindow(SCALE_BOTH);
    draw_main();
}

void MakeDualMapScatterPlot(MAP *m1, MAP *m2)
{
    int i, j, nX, nY, n;
    string buf;
    
    void set_scatter_minmax();
    int AllocScatter(), FreeScatter();
    
    if (!m1 || !m2) return;
    
    nX = m1->i_no;
    nY = m1->j_no;
    
    if (nX != m2->i_no || nY != m2->j_no) return;
    
    n = 0;
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (m1->f[i][j] <= BLANK || m2->f[i][j] <= BLANK) continue;
            n++;
        }
    }
    
    if (AllocScatter(n)) {
        sprintf(buf, "Couldn't allocate scatter data (%d).", n);
        PostErrorDialog(NULL, buf);
        return;
    }
    
    n = 0;
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (m1->f[i][j] <= BLANK || m2->f[i][j] <= BLANK) continue;
            scat.x[n] = m1->d[i][j];
            scat.ex[n] = m1->e[i][j];
            scat.y[n] = m2->d[i][j];
            scat.ey[n] = m2->e[i][j];
            scat.sp[n] = m1->sp[i][j];
            scat.t[n] = 0;
            n++;
        }
    }
    scat.x0 = m1->x0;
    scat.y0 = m1->y0;
    scat.epoch = m1->epoch;
    scat.equinox = m1->equinox;
    
    strcpy(scat.name, m1->name);
    strcpy(scat.molecule, m1->molecule);
    scat.date = m1->date;
    
    set_scatter_minmax(&scat);
    
    scat.m1 = m1;
    scat.m2 = m2;
    scat.s  = NULL;
    scat.p  = NULL;
    scat.xtype = XTYPE_SCA_INT;
}

void set_scatter_minmax(scatter *p)
{
    int n;
    
    if (!p || p->nData <= 0) return;
    
    if (strcmp(p->molecule, "pointing")==0 ||
        strcmp(p->molecule, "residuals")==0) {
      p->xmin = p->xmax = p->x[0];
      p->ymin = p->ymax = p->y[0];
      for (n=1; n<p->nData; n++) {
          if (p->x[n] < p->xmin) p->xmin = p->x[n];
          if (p->x[n] > p->xmax) p->xmax = p->x[n];
          if (p->y[n] < p->ymin) p->ymin = p->y[n];
          if (p->y[n] > p->ymax) p->ymax = p->y[n];
      }
    } else {
      p->xmin = p->x[0] - p->ex[0];
      p->xmax = p->x[0] + p->ex[0];
      p->ymin = p->y[0] - p->ey[0];
      p->ymax = p->y[0] + p->ey[0];

      for (n=1; n<p->nData; n++) {
          if (p->x[n] - p->ex[n] < p->xmin)
              p->xmin = p->x[n] - p->ex[n];
          if (p->x[n] + p->ex[n] > p->xmax)
              p->xmax = p->x[n] + p->ex[n];
          if (p->y[n] - p->ey[n] < p->ymin)
              p->ymin = p->y[n] - p->ey[n];
          if (p->y[n] + p->ey[n] > p->ymax)
              p->ymax = p->y[n] + p->ey[n];
      }
    }
    
    if (p->xmin == p->xmax) {
        p->xmin -= 1.0;
        p->xmax += 1.0;
    }
    
    if (p->ymin == p->ymax) {
        p->ymin -= 1.0;
        p->ymax += 1.0;
    }
}

int AllocScatter(int size)
{
    int FreeScatter();
    
    if (scat.nData > 0) FreeScatter(scat.nData, 0);
    
    scat.x = AllocDoubleVector(size);
    if (!scat.x) return 1;
    
    scat.y = AllocDoubleVector(size);
    if (!scat.y) return FreeScatter(size, 2);
    
    scat.ex = AllocDoubleVector(size);
    if (!scat.ex) return FreeScatter(size, 3);
    
    scat.ey = AllocDoubleVector(size);
    if (!scat.ey) return FreeScatter(size, 4);
    
    scat.sp = (scanPtr *)calloc(size, sizeof(scanPtr));
    if (!scat.sp) return FreeScatter(size, 5);
    
    scat.t  = AllocIntVector(size);
    if (!scat.t) return FreeScatter(size, 6);
    
    scat.nData = size;
    return 0;
}

int FreeScatter(int size, int ret)
{
    if (scat.x && size > 0)
        FreeDoubleVector(scat.x);
    if (scat.y && size > 0)
        FreeDoubleVector(scat.y);
    if (scat.ex && size > 0)
        FreeDoubleVector(scat.ex);
    if (scat.ey && size > 0)
        FreeDoubleVector(scat.ey);
    if (scat.sp && size > 0)
        free((char *)scat.sp);
    if (scat.t && size > 0)
        FreeIntVector(scat.t);
    
    scat.x  = NULL;
    scat.y  = NULL;
    scat.ex = NULL;
    scat.ey = NULL;
    scat.sp = NULL;
    scat.t  = NULL;
    scat.nData = 0;
    scat.m1 = NULL;
    scat.m2 = NULL;
    scat.s  = NULL;
    scat.p  = NULL;
    strcpy(scat.name, "");
    strcpy(scat.molecule, "");
    
    return ret;
}

int ScatterFitter(scatter *p, ScatterPar sp[], int nPar,
                  void (*f)())
{
    int err, nIter = SCATTER_FIT_ITER * nPar;
    int n, fit[MAX_PAR];
    double par[MAX_PAR], unc[MAX_PAR];
    double chi2=0.0;
    
    void wprintf();
    int Fitter1D(double x[], double y[], double e[], int nData,
                 double p[], int fit[], double q[], int nPar,
                 int nIter, double *chi2, void (*f)());
    
    if (!p || nPar > MAX_PAR) return -1;
    
    for (n=0; n<nPar; n++) {
        par[n] = sp[n].p;
        unc[n] = sp[n].q;
        fit[n] = sp[n].fit;
    }
    
    if (sopt.yerror > 0.0) {
        for (n=0; n<p->nData; n++) p->ey[n] = sopt.yerror;
    } else {
        for (n=0; n<p->nData; n++) {
            if (p->ey[n] <= 0.0) p->ey[n] = 1.0;
        }
    }
    
    err = Fitter1D(p->x, p->y, p->ey, p->nData,
                   par, fit, unc, nPar,
                   nIter, &chi2, f);
    
    if (!err && p->nData > 1)
        chi2 /= (double)(p->nData - 1);
    else
        chi2 = -1.0;
    
    for (n=0; n<nPar; n++) {
        sp[n].p = par[n];
        sp[n].q = unc[n];
    }
    
    if (sopt.updateError && sopt.yerror > 0.0 && chi2 > 0.0) {
        sopt.yerror *= sqrt(chi2);
    }
    
    if (chi2 > 0.0) {
        wprintf(gp->TGauss[2], "Chi^2: %11.4e", chi2);
    } else {
        wprintf(gp->TGauss[2], "");
    }
    
    return err;
}

int ScatterFitter2D(scatter *p, ScatterPar sp[], int nPar,
                  void (*f1)(), void (*f2)())
{
    int err, nIter = SCATTER_FIT_ITER * nPar;
    int m, n, fit[MAX_PAR];
    double par[MAX_PAR], unc[MAX_PAR], *ez = NULL;
    double chi2=0.0;
    
    void wprintf();
    int Fitter2D(double x1[], double x2[], double y[], double e[], int nData,
                 double p[], int fit[], double q[], int nPar,
                 int nIter, double *chi2, void (*f)());
    
    if (!p || nPar > MAX_PAR) return -1;
    
    if (strcmp(p->molecule, "residuals")) return -1;
    
    for (n=0; n<nPar; n++) {
        par[n] = sp[n].p;
        unc[n] = sp[n].q;
        fit[n] = sp[n].fit;
    }
    
    if (nPar == 18) {
        ez = (double *)malloc( p->nData * sizeof(double) );
	if (!ez) return -1;
        for (n=0; n<p->nData; n++) ez[n] = 1.0;
    } else if (sopt.yerror > 0.0) {
        for (n=0; n<p->nData; n++) p->ey[n] = sopt.yerror;
    } else {
        for (n=0; n<p->nData; n++) {
            if (p->ey[n] <= 0.0) p->ey[n] = 1.0;
        }
    }
    
    m = 6;
    while (m > 0) {
        for (n=0; n<nPar; n++) fit[n] = sp[n].fit;
	fit[1] = fit[6] = fit[9] = fit[10] = 0;
	fit[13] = fit[14] = fit[15] = fit[17] = 0;
        err = Fitter2D(p->ex, p->ey, p->x, ez, p->nData,
                       par, fit, unc, nPar,
                       nIter, &chi2, f1);
    
        for (n=0; n<nPar; n++) {
	    if (fit[n]) sp[n].q = unc[n];
            fit[n] = sp[n].fit;
        }
	fit[0] = fit[2] = fit[3] = fit[7] = fit[8] = 0;
	fit[11] = fit[12] = fit[16] = 0;
        err = Fitter2D(p->ex, p->ey, p->y, ez, p->nData,
                       par, fit, unc, nPar,
                       nIter, &chi2, f2);
        for (n=0; n<nPar; n++) {
	    if (fit[n]) sp[n].q = unc[n];
            fit[n] = sp[n].fit;
        }
        m--;
    }
    
    if (ez) free(ez);
    
    if (!err && p->nData > 1)
        chi2 /= (double)(p->nData - 1);
    else
        chi2 = -1.0;
    
    for (n=0; n<nPar; n++) {
        sp[n].p = par[n];
        /* sp[n].q = unc[n]; */
    }
    
    if (sopt.updateError && sopt.yerror > 0.0 && chi2 > 0.0) {
        sopt.yerror *= sqrt(chi2);
    }
    
    if (chi2 > 0.0) {
        wprintf(gp->TGauss[2], "Chi^2: %11.4e", chi2);
    } else {
        wprintf(gp->TGauss[2], "");
    }
    
    return err;
}

static int GetCurrentNPar()
{
    int nPar;
    
    if (sopt.fit == SCATTER_NO_FIT) {
        nPar = 0;
    } else if (sopt.fit == SCATTER_LINE_FIT) {
        nPar = 2;
    } else if (sopt.fit == SCATTER_POLY_FIT) {
        nPar = 10;
    } else if (sopt.fit == SCATTER_GAUSS_FIT) {
        nPar = 4;
    } else if (sopt.fit == SCATTER_LORENTZ_FIT) {
        nPar = 4;
    } else if (sopt.fit == SCATTER_EXP_FIT) {
        nPar = 3;
    } else if (sopt.fit == SCATTER_ERFC_FIT) {
        nPar = 3;
    } else if (sopt.fit == SCATTER_EXPPROF1_FIT) {
        nPar = 4;
    } else if (sopt.fit == SCATTER_EXPPROF2_FIT) {
        nPar = 4;
    } else if (sopt.fit == SCATTER_EXPPROF3_FIT) {
        nPar = 5;
    } else if (sopt.fit == SCATTER_COMET_FIT) {
        nPar = 2;
    } else if (sopt.fit == SCATTER_QUAD_FIT) {
        nPar = 6;
    } else if (sopt.fit == SCATTER_FOURIER_FIT) {
        nPar = 10;
    } else if (sopt.fit == SCATTER_INVPOLY_FIT) {
        nPar = 10;
    } else if (sopt.fit == SCATTER_DISGAU_FIT) {
        nPar = 3;
    } else if (sopt.fit == SCATTER_POI_FIT) {
        nPar = 18;
    } else {
        return 0;
    }
    
    return nPar;
}

static void UpdateParameterWidgets()
{
    int n, nPar;
    
    void wprintf();

    if (!(nPar = GetCurrentNPar())) return;    
    
    for (n=0; n<nPar; n++) {
        if (spar[n].e) {
            wprintf(spar[n].e, "%g", spar[n].p);
            if (spar[n].fit)
                wprintf(spar[n].error, "%g", spar[n].q);
            else
                wprintf(spar[n].error, "");
        }
    }
}

static void UpdateScatterWidgets()
{
    void wprintf();

    if (sopt.e[0]) wprintf(sopt.e[0],  "%f", sopt.xRef);
    if (sopt.e[1]) wprintf(sopt.e[1],  "%f", sopt.yRef);
    if (sopt.e[2]) wprintf(sopt.e[2],  "%f", sopt.r1);
    if (sopt.e[3]) wprintf(sopt.e[3],  "%f", sopt.r2);
    if (sopt.e[4]) wprintf(sopt.e[4],  "%f", sopt.PA1);
    if (sopt.e[5]) wprintf(sopt.e[5],  "%f", sopt.PA2);
    if (sopt.e[6]) wprintf(sopt.e[6],  "%f", sopt.yerror);
    if (sopt.e[7]) wprintf(sopt.e[7],  "%d", sopt.nPos);
    if (sopt.e[8]) wprintf(sopt.e[8],  "%f", sopt.Width);
    
    UpdateParameterWidgets();
}

int CountTaggedScatterPoints(scatter *p)
{
    int n, nTagged = 0;
    
    for (n=0; n<p->nData; n++) {
        if (p->t[n]) nTagged++;
    }
    
    return nTagged;
}

void draw_scatter_plot(GC gc1, GC gc2, scatter *p)
{
    int n, err=0, nPar, tagged = 0, nvis=0;
    int x0, y0, yup, ylo, xle, xri, size;
    double x, y, e, yU=0.0, yL=0.0, xL=0.0, xR=0.0;
    double sx1=0.0, sx2=0.0;
    double sy1=0.0, sy2=0.0;
    double c[MAX_PAR];
#ifdef HAVE_LIBPGPLOT
    PSSTY tmp_style = ps.marker;
    PLFLT fa[2], fb[2], fc[1];
#endif
    string buf;
    char label[1024];
    Gauss g;
    GC gc = gc1;
    
    int xunit2x(), yunit2y();
    int ScatterFitter(), ScatterFitter2D();
    void SetWatchCursor();
    void draw_line(), DrawAnyGauss();
    int point_is_inside(int, int);
    void DrawAbsLabel(), DrawContourDot(), DrawRelLabel();
    char *get_numeric_label();
    void DrawAnyPoly(), DrawAnyExp(), DrawAnyErf(), DrawAnyExpProf();
    void DrawAnyValueFunc();
    /* extern void GaussFunc4(), ExpFunc3(), LineFunc2(), ErfcFunc3(), inv_poly();
    extern void poly_std(), ExpProfile1(), ExpProfile3(), LorentzFunc4();
    extern void ExpProfile2(), CometFunc(), QuadrupoleFunc(), FourierFunc(); */
    extern double CometValue(), ExpProfileValue(), QuadrupoleValue();
    extern double ExpProfileValue3(), LorentzValue(), FourierValue();
    extern double DiscGaussValue();
    extern void lm_poly(), lm_line2(), lm_Lorentz4(), lm_Gauss4(), lm_exp3();
    extern void lm_erfc3(), lm_epro1(), lm_epro2(), lm_epro3(), lm_comet();
    extern void lm_invpoly(), lm_Fourier(), lm_Quadrupole(), lm_discgauss();
    extern void lm_apexpoiA(), lm_apexpoiE();
    
    if (!p) return;
    
    SetWatchCursor(True);
    
    nPar = GetCurrentNPar();
    
    sopt.p = p;
    
    if (sopt.fit == SCATTER_LINE_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_line2);
        if (err) {
            strcpy(label, "Couldn't fit straight line properly");
        } else {
            if (spar[0].p < 0.0)
                sprintf(label, "y = %fx - %f", spar[1].p, -spar[0].p);
            else
                sprintf(label, "y = %fx + %f", spar[1].p, spar[0].p);
        }
    } else if (sopt.fit == SCATTER_POLY_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_poly);
        if (err) {
            sprintf(label, "Couldn't fit %dth order polynomial properly",
                    nPar-1);
        } else {
            strcpy(label, "y=");
            for (n=0; n<=9; n++) {
                x = spar[n].p;
                if (x == 0.0) continue;
                if (n == 0) {
                    sprintf(buf, "%f", x);
                } else if (n == 1) {
                    if (x < 0.0)
                        sprintf(buf, "-%gx", -x);
                    else
                        sprintf(buf, "+%gx", x);
                } else {
                    if (x < 0.0)
                        sprintf(buf, "-%gx\\u%d\\d", -x, n);
                    else
                        sprintf(buf, "+%gx\\u%d\\d", x, n);
                }
                strcat(label, buf);
            }
        }
    } else if (sopt.fit == SCATTER_GAUSS_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_Gauss4);
        if (err) {
            sprintf(label, "Couldn't fit %d par. Gaussian properly", nPar);
        } else {
            if (spar[2].p < 0.0)
                sprintf(label, "y = %f + %f Exp[-4ln2((x+%f)/%f)\\u2\\d]",
                        spar[3].p, spar[0].p, -spar[2].p, spar[1].p);
            else
                sprintf(label, "y = %f + %f Exp[-4ln2((x-%f)/%f)\\u2\\d]",
                        spar[3].p, spar[0].p, spar[2].p, spar[1].p);
        }
    } else if (sopt.fit == SCATTER_LORENTZ_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_Lorentz4);
        if (err) {
            sprintf(label, "Couldn't fit %d par. Lorentz properly", nPar);
        } else {
            if (spar[2].p < 0.0)
                sprintf(label, "y = %f + %f/[1 + (2[x+%f]/%f)\\u2\\d]",
                        spar[3].p, spar[0].p, -spar[2].p, spar[1].p);
            else
                sprintf(label, "y = %f + %f/[1 + (2[x-%f]/%f)\\u2\\d]",
                        spar[3].p, spar[0].p, spar[2].p, spar[1].p);
        }
    } else if (sopt.fit == SCATTER_EXP_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_exp3);
        if (err) {
            sprintf(label, "Couldn't fit %d par. exponential properly", nPar);
        } else {
            if (spar[2].p < 0.0)
                sprintf(label, "y = %f e\\u%f(x-%f)\\d",
                        spar[0].p, spar[1].p, -spar[2].p);
            else
                sprintf(label, "y = %f e\\u%f(x+%f)\\d",
                        spar[0].p, spar[1].p, spar[2].p);
        }
    } else if (sopt.fit == SCATTER_ERFC_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_erfc3);
        if (err) {
            sprintf(label, "Couldn't fit %d par. erfc properly", nPar);
        } else {
            sprintf(label, "y = %f Erfc[sqrt(4ln2)(x-%f)/%f]",
                    spar[0].p, spar[1].p, spar[2].p);
        }
    } else if (sopt.fit == SCATTER_EXPPROF1_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_epro1);
        if (err) {
            sprintf(label, "Couldn't fit %d par. (1-x^2)^(g/2) properly", nPar);
        } else {
            if (spar[1].p < 0.0)
                sprintf(label, "y = %f {1-[(x+%f)/%f]\\u2\\d}\\u%f/2\\d",
                        spar[0].p, -spar[1].p, spar[2].p, spar[3].p);
            else
                sprintf(label, "y = %f {1-[(x-%f)/%f]\\u2\\d}\\u%f/2\\d",
                        spar[0].p, spar[1].p, spar[2].p, spar[3].p);
        }
    } else if (sopt.fit == SCATTER_EXPPROF2_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_epro2);
        if (err) {
            sprintf(label, "Couldn't fit %d par. exp. profile properly", nPar);
        } else {
            sprintf(label, "y = %f f(v\\d0\\u=%f, v\\de\\u=%f, v\\dt\\u=%f)",
                    spar[0].p, spar[1].p, spar[2].p, spar[3].p);
        }
    } else if (sopt.fit == SCATTER_EXPPROF3_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_epro3);
        if (err) {
            sprintf(label, "Couldn't fit %d par. 1-exp(...) properly", nPar);
        } else {
            sprintf(label, "y = %f [1- f(\\gt=%f, v\\d0\\u=%f, v\\de\\u=%f, v\\dt\\u=%f)]",
                    spar[0].p, spar[4].p, spar[1].p, spar[2].p, spar[3].p);
        }
    } else if (sopt.fit == SCATTER_COMET_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_comet);
        if (err) {
            sprintf(label, "Couldn't fit %d par. comet func. properly", nPar);
        } else {
            sprintf(label, "y = %f f(\\gg=%f)",
                    spar[0].p, spar[1].p);
        }
    } else if (sopt.fit == SCATTER_QUAD_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_Quadrupole);
        if (err) {
            sprintf(label, "Couldn't fit %d quadr. func. properly", nPar);
        } else {
            sprintf(label,
"A\\d1\\u=%f, r\\d3\\u=%f, r\\d5\\u=%f, \\gn=%f, eQq=%f, \\gDv=%f",
                    spar[0].p, spar[1].p, spar[2].p,
                    spar[3].p, spar[4].p, spar[5].p);
        }
    } else if (sopt.fit == SCATTER_FOURIER_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_Fourier);
        if (err) {
            sprintf(label, "Couldn't fit %d Fourier sum properly", nPar);
        } else {
            sprintf(label, "A=2\\gpx/%f, y=%f/2", spar[0].p, spar[1].p);
            for (n=2; n<=9; n++) {
                x = spar[n].p;
                if (x == 0.0) continue;
                if (n % 2 == 0) {
                    if (x < 0.0)
                        sprintf(buf, "-%gcos%dA", -x, n/2);
                    else
                        sprintf(buf, "+%gcos%dA", x, n/2);
                } else {
                    if (x < 0.0)
                        sprintf(buf, "-%gsin%dA", -x, n/2);
                    else
                        sprintf(buf, "+%gsin%dA", x, n/2);
                }
                strcat(label, buf);
            }
        }
    } else if (sopt.fit == SCATTER_INVPOLY_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_invpoly);
        if (err) {
            sprintf(label, "Couldn't fit %dth order inverse poly. properly",
                    nPar-1);
        } else {
            strcpy(label, "y=1/(");
            for (n=0; n<=9; n++) {
                x = spar[n].p;
                if (x == 0.0) continue;
                if (n == 0) {
                    sprintf(buf, "%f", x);
                } else if (n == 1) {
                    if (x < 0.0)
                        sprintf(buf, "-%gx", -x);
                    else
                        sprintf(buf, "+%gx", x);
                } else {
                    if (x < 0.0)
                        sprintf(buf, "-%gx\\u%d\\d", -x, n);
                    else
                        sprintf(buf, "+%gx\\u%d\\d", x, n);
                }
                strcat(label, buf);
            }
            strcat(label, ")");
        }
    } else if (sopt.fit == SCATTER_DISGAU_FIT) {
        err = ScatterFitter(p, spar, nPar, lm_discgauss);
        if (err) {
            sprintf(label,
	      "Couldn't fit %d par. gaussian conv. disc properly", nPar);
        } else {
            sprintf(label, "A=%f R=%f B=%f",
                    spar[0].p, spar[1].p, spar[2].p);
        }
    } else if (sopt.fit == SCATTER_POI_FIT) {
        err = ScatterFitter2D(p, spar, nPar, lm_apexpoiA, lm_apexpoiE);
        if (err) {
            sprintf(label,
	      "Couldn't fit %d par. apex pointing model properly", nPar);
        } else {
            sprintf(label, "IA=%f IE=%f CA=%f",
                    spar[0].p, spar[1].p, spar[3].p);
        }
    }
    
    tagged = CountTaggedScatterPoints(p);
    if (!tagged) {
        gc = gc1;
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            SetPGStyle(&ps.marker);
        }
#endif
    }
    
    for (n=0; n<p->nData; n++) {
        x  = p->x[n];
        e  = p->ex[n];
        xL = x - e;
        xR = x + e;
        
        y  = p->y[n];
        e  = p->ey[n];
        yU = y + e;
        yL = y - e;
        
        x0  = xunit2x(x);
        xle = xunit2x(xL);
        xri = xunit2x(xR);
        
        y0  = yunit2y(y);
        yup = yunit2y(yU);
        ylo = yunit2y(yL);
        
        if (sopt.stat == 1 || (sopt.stat == 2 && point_is_inside(x0, y0))) {
            nvis++;
            sx1 += x;
            sx2 += x*x;
            sy1 += y;
            sy2 += y*y;
        }

        if (tagged) { /* Only alter the gc and style if tagged points exist */
            if (p->t[n]) { /* tagged scatter point */
                gc = gc2;
#ifdef HAVE_LIBPGPLOT
                if (pgplot) {
                    tmp_style = ps.marker;
                    ps.marker = ps.gauss;
                    SetPGStyle(&ps.marker);
                }
#endif
            } else {
                gc = gc1;
#ifdef HAVE_LIBPGPLOT
                if (pgplot) {
                    SetPGStyle(&ps.marker);
                }
#endif
            }
        }
        
        if (dotType == DOT_SQUAREMARK) {
            DrawContourDot(gc, 's', (double)dotSize, x, y);
        } else if (dotType == DOT_CIRCLEMARK) {
            DrawContourDot(gc, 'c', 2.0 * (double)dotSize, x, y);
        } else if (dotType == DOT_CROSSMARK) {
            DrawContourDot(gc, 'x', (double)dotSize, x, y);
        } else if (dotType == DOT_PLUSMARK) {
            DrawContourDot(gc, '+', (double)dotSize, x, y);
        } else if (dotType == DOT_VALUEMARK) {
            DrawAbsLabel(gc, x, y, 0.5,
            		 get_numeric_label(y, 0.01, 4));
        }
#ifdef HAVE_LIBPGPLOT
        if (tagged && pgplot) {
            if (p->t[n]) ps.marker = tmp_style;
        }
#endif
        size = 2*dotSize;
        if (draw.xebars) {
            draw_line(gc, xle, y0, xri, y0);
            draw_line(gc, xle, y0-size, xle, y0+size);
            draw_line(gc, xri, y0-size, xri, y0+size);
#ifdef HAVE_LIBPGPLOT
            if (pgplot) {
                fa[0] = (PLFLT)y;
                fb[0] = (PLFLT)xL;
                fc[0] = (PLFLT)xR;
                cpgerrx(1, fb, fc, fa, (PLFLT)1.0);
            }
#endif
        }
        if (draw.yebars) {
            draw_line(gc, x0, yup, x0, ylo);
            draw_line(gc, x0-size, yup, x0+size, yup);
            draw_line(gc, x0-size, ylo, x0+size, ylo);
#ifdef HAVE_LIBPGPLOT
            if (pgplot) {
                fa[0] = (PLFLT)x;
                fb[0] = (PLFLT)yL;
                fc[0] = (PLFLT)yU;
                cpgerry(1, fa, fb, fc, (PLFLT)1.0);
            }
#endif
        }
    }
    
    if (sopt.stat && nvis) {
        x = sopt.xmean = sx1/(double)nvis;
        y = sopt.ymean = sy1/(double)nvis;
        if (nvis > 1) {
            sopt.xsigma = sqrt((sx2 - sx1*sx1/(double)nvis)/(double)(nvis-1));
            sopt.ysigma = sqrt((sy2 - sy1*sy1/(double)nvis)/(double)(nvis-1));
        } else {
            sopt.xsigma = 0.0;
            sopt.ysigma = 0.0;
        }
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            tmp_style = ps.marker;
            ps.marker = ps.gauss;
            SetPGStyle(&ps.marker);
        }
#endif
        if (dotType == DOT_SQUAREMARK) {
            DrawContourDot(gc2, 's', (double)dotSize, x, y);
        } else if (dotType == DOT_CIRCLEMARK) {
            DrawContourDot(gc2, 'c', 2.0*(double)dotSize, x, y);
        } else if (dotType == DOT_CROSSMARK) {
            DrawContourDot(gc2, 'x', (double)dotSize, x, y);
        } else if (dotType == DOT_PLUSMARK) {
            DrawContourDot(gc2, '+', (double)dotSize, x, y);
        } else if (dotType == DOT_VALUEMARK) {
            sprintf(buf, "(%f +/- %f, %f +/- %f)", x, sopt.xsigma,
                                                   y, sopt.ysigma);
            DrawAbsLabel(gc2, x, y, 0.5, buf);
        }
        x0  = xunit2x(x);
        xle = xunit2x(x - sopt.xsigma);
        xri = xunit2x(x + sopt.xsigma);
        y0  = yunit2y(y);
        yup = yunit2y(y + sopt.ysigma);
        ylo = yunit2y(y - sopt.ysigma);
        size = 2*dotSize;
        draw_line(gc2, xle, y0, xri, y0);
        draw_line(gc2, xle, y0-size, xle, y0+size);
        draw_line(gc2, xri, y0-size, xri, y0+size);
        draw_line(gc2, x0, yup, x0, ylo);
        draw_line(gc2, x0-size, yup, x0+size, yup);
        draw_line(gc2, x0-size, ylo, x0+size, ylo);
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fa[0] = (PLFLT)y;
            fb[0] = (PLFLT)(x - sopt.xsigma);
            fc[0] = (PLFLT)(x + sopt.xsigma);
            cpgerrx(1, fb, fc, fa, (PLFLT)1.0);
            fa[0] = (PLFLT)x;
            fb[0] = (PLFLT)(y - sopt.ysigma);
            fc[0] = (PLFLT)(y + sopt.ysigma);
            cpgerry(1, fa, fb, fc, (PLFLT)1.0);
            SetPGStyle(&ps.marker);
        }
#endif
        sprintf(buf, "%d pts (%f +/- %f, %f +/- %f)", nvis,
                x, sopt.xsigma, y, sopt.ysigma);
        DrawRelLabel(gc2, 0.95, 0.92, 1.0, buf);
    }
    
    if (sopt.join) {
#ifdef HAVE_LIBPGPLOT
        if (pgplot) SetPGStyle(&ps.line);
#endif
        for (n=0; n<p->nData-1; n++) {
            draw_line(gc1, xunit2x(p->x[n]),   yunit2y(p->y[n]),
                          xunit2x(p->x[n+1]), yunit2y(p->y[n+1]));
#ifdef HAVE_LIBPGPLOT
            if (pgplot) {
                fa[0] = (PLFLT)p->x[n];
                fb[0] = (PLFLT)p->y[n];
                fa[1] = (PLFLT)p->x[n+1];
                fb[1] = (PLFLT)p->y[n+1];
                cpgline(2, fa, fb);
            }
#endif
        }
    }
    if (sopt.fit > SCATTER_NO_FIT) {
#ifdef HAVE_LIBPGPLOT
        if (pgplot) SetPGStyle(&ps.gauss);
#endif
        if (!err) {
            xL = p->xmin;
            xR = p->xmax;
            UpdateScatterWidgets();
            for (n=0; n<nPar; n++) c[n] = spar[n].p;
            if (sopt.fit == SCATTER_LINE_FIT || sopt.fit == SCATTER_POLY_FIT) {
                DrawAnyPoly(gc2, c, nPar, xL, xR);
            } else if (sopt.fit == SCATTER_EXP_FIT) {
                DrawAnyExp(gc2, c, nPar, xL, xR);
            } else if (sopt.fit == SCATTER_ERFC_FIT) {
                DrawAnyErf(gc2, c, nPar, xL, xR);
            } else if (sopt.fit == SCATTER_EXPPROF1_FIT) {
                DrawAnyExpProf(gc2, c, nPar, xL, xR);
            } else if (sopt.fit == SCATTER_EXPPROF2_FIT) {
                DrawAnyValueFunc(gc2, c, nPar, xL, xR, ExpProfileValue);
            } else if (sopt.fit == SCATTER_EXPPROF3_FIT) {
                DrawAnyValueFunc(gc2, c, nPar, xL, xR, ExpProfileValue3);
            } else if (sopt.fit == SCATTER_COMET_FIT) {
                DrawAnyValueFunc(gc2, c, nPar, xL, xR, CometValue);
            } else if (sopt.fit == SCATTER_QUAD_FIT) {
                DrawAnyValueFunc(gc2, c, nPar, xL, xR, QuadrupoleValue);
            } else if (sopt.fit == SCATTER_LORENTZ_FIT) {
                DrawAnyValueFunc(gc2, c, nPar, xL, xR, LorentzValue);
            } else if (sopt.fit == SCATTER_FOURIER_FIT) {
                DrawAnyValueFunc(gc2, c, nPar, xL, xR, FourierValue);
            } else if (sopt.fit == SCATTER_INVPOLY_FIT) {
                DrawAnyPoly(gc2, c, -nPar, xL, xR);
            } else if (sopt.fit == SCATTER_DISGAU_FIT) {
                DrawAnyValueFunc(gc2, c, nPar, xL, xR, DiscGaussValue);
            } else if (sopt.fit == SCATTER_POI_FIT) {
            } else {
                g.amp = spar[0].p;
                g.wid = spar[1].p;
                g.cen = spar[2].p;
                DrawAnyGauss(gc2, &g, spar[3].p, xL, xR);
            }
        }
        if (sopt.label) DrawRelLabel(gc2, 0.95, 0.92, 1.0, label);
    }
    
    SetWatchCursor(False);
}

static void UpdateScatterPars()
{
    int n, nPar;
    
    void wdscanf();

    if (!(nPar = GetCurrentNPar())) return;
    
    for (n=0; n<nPar; n++) {
        if (spar[n].e) wdscanf(spar[n].e, &spar[n].p);
    }
}

static void UpdateScatterOpts(Widget w, StdForm *sf, XmAnyCallbackStruct *cd)
{
    string buf;
    
    void wdscanf(), wiscanf(), draw_main();
    
    wdscanf(sopt.e[0], &sopt.xRef);
    wdscanf(sopt.e[1], &sopt.yRef);
    wdscanf(sopt.e[2], &sopt.r1);
    wdscanf(sopt.e[3], &sopt.r2);
    wdscanf(sopt.e[4], &sopt.PA1);
    wdscanf(sopt.e[5], &sopt.PA2);
    wdscanf(sopt.e[6], &sopt.yerror);
    wiscanf(sopt.e[7], &sopt.nPos);
    wdscanf(sopt.e[8], &sopt.Width);
    
    UpdateScatterPars();
    
    if (vP->p && vP->p->m1 && vP->p->p)
        MakePolyLineCutScatterPlot(vP->p->m1, vP->p->p);
    else if (vP->p && vP->p->m1)
        MakeMapScatterPlot(vP->p->m1, NULL);
    else if (vP->p && vP->p->s && !vP->p->single)
        MakeSpeScatterPlot(vP->p->dsp, vP->p->xtype, vP->p->ytype);
    else if (vP->p)
        ;
    else if (vP->m)
        MakeMapScatterPlot(vP->m, NULL);
    else {
        strcpy(buf, "Couldn't find a reference to redo the scatter plot.");
        PostErrorDialog(w, buf);
        return;
    }    
    
    if (vP->mode == SHOW_SCATTER) {
        draw_main();
    }
}

static void cancel_parameter_dialog(Widget w, StdForm *sf,
                                    XmAnyCallbackStruct *cd)
{
    int n;
    
    for (n=0; n<MAX_PAR; n++) {
        spar[n].f = spar[n].e = spar[n].error = NULL;
    }
    if (sf && sf->form) XtDestroyWidget(sf->form);
}

static void ToggleFitCallback(Widget w, ScatterPar *sp,
                              XmToggleButtonCallbackStruct *cd)
{
    void wprintf();
    
    if (!cd) {
        sp->fit = 0;
        if (sp->f) wprintf(sp->f, "%s", "Fixed");
        return;
    }
    
    if (cd->set) {
        sp->fit = 1;
        wprintf(sp->f, "%s", "Fitted");
    } else {
        sp->fit = 0;
        wprintf(sp->f, "%s", "Fixed");
    }
}

static void saveFitasScatter(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int i, j, nX, nY, nPar;
    double A, E, Aoff, Eoff, p[MAX_PAR];
    scatter *fit;
    Gauss g;
    MAP *mA, *mE;
    
    scatter *copy_scatter();
    list *get_scatterlist();
    MAP *new_map(), *copy_map();
    list *get_maplist();
    double AzPoi2DValue(double x, double y, double *a, int nPar);
    double ElPoi2DValue(double x, double y, double *a, int nPar);
    double GetAnyPoly(double x, double *a, int nP);
    double GetAnyExp(double x, double *a, int nP);
    double GetAnyErf(double x, double *a, int nP);
    double GetAnyExpProf(double x, double *a, int nP);
    double GetAnyPoly(double x, double *a, int nP);
    double GetAnyGauss(double, Gauss *, double);
    double GetAnyValueFunc();
    extern double CometValue(), ExpProfileValue(), QuadrupoleValue();
    extern double ExpProfileValue3(), LorentzValue(), FourierValue();
    
    nPar = GetCurrentNPar();
    
    fit = copy_scatter(get_scatterlist(), sopt.p);
    if (!fit) return;
    
    for (i=0; i<nPar; i++) p[i] = spar[i].p;
    
    for (i=0; i<fit->nData; i++) {
        if (sopt.fit == SCATTER_LINE_FIT || sopt.fit == SCATTER_POLY_FIT) {
	    fit->y[i] = GetAnyPoly(fit->x[i], p, nPar);
        } else if (sopt.fit == SCATTER_EXP_FIT) {
            fit->y[i] = GetAnyExp(fit->x[i], p, nPar);
        } else if (sopt.fit == SCATTER_ERFC_FIT) {
            fit->y[i] = GetAnyErf(fit->x[i], p, nPar);
        } else if (sopt.fit == SCATTER_EXPPROF1_FIT) {
            fit->y[i] = GetAnyExpProf(fit->x[i], p, nPar);
        } else if (sopt.fit == SCATTER_EXPPROF2_FIT) {
            fit->y[i] = GetAnyValueFunc(fit->x[i], p, nPar, ExpProfileValue);
        } else if (sopt.fit == SCATTER_EXPPROF3_FIT) {
            fit->y[i] = GetAnyValueFunc(fit->x[i], p, nPar, ExpProfileValue3);
        } else if (sopt.fit == SCATTER_COMET_FIT) {
            fit->y[i] = GetAnyValueFunc(fit->x[i], p, nPar, CometValue);
        } else if (sopt.fit == SCATTER_QUAD_FIT) {
            fit->y[i] = GetAnyValueFunc(fit->x[i], p, nPar, QuadrupoleValue);
        } else if (sopt.fit == SCATTER_LORENTZ_FIT) {
            fit->y[i] = GetAnyValueFunc(fit->x[i], p, nPar, LorentzValue);
        } else if (sopt.fit == SCATTER_FOURIER_FIT) {
            fit->y[i] = GetAnyValueFunc(fit->x[i], p, nPar, FourierValue);
	} else if (sopt.fit == SCATTER_INVPOLY_FIT) {
	    fit->y[i] = GetAnyPoly(fit->x[i], p, -nPar);
	} else if (sopt.fit == SCATTER_POI_FIT) {
            A = fit->ex[i]; E = fit->ey[i];
            Aoff = AzPoi2DValue(A, E, p, nPar);
            Eoff = ElPoi2DValue(A, E, p, nPar);
            fit->x[i] = sopt.p->x[i] - Aoff;
	    fit->y[i] = sopt.p->y[i] - Eoff;
	} else if (sopt.fit == SCATTER_GAUSS_FIT) {
	    g.amp = spar[0].p;
            g.wid = spar[1].p;
            g.cen = spar[2].p;
            fit->y[i] = GetAnyGauss(fit->x[i], &g, spar[3].p);
	}
    }
    sprintf(fit->name, "Fitted %s", sopt.p->name);
    
    if (sopt.fit != SCATTER_POI_FIT) return;
    
    nX = 100;
    nY = 50;
    mA = new_map(get_maplist(), nX, nY);
    if (!mA) return;
    sprintf(mA->name, "Az corr %s", sopt.p->name);
    mA->type = MAP_POSPOS;
    mA->coordType = COORD_TYPE_HOR;
    mA->i_no = nX;
    mA->j_no = nY;
    mA->x0 = mA->y0 = 0.0;
    mA->posAngle = 0.0;
    mA->date =  sopt.p->date;
    mA->xleft = -180.0;
    mA->xright = 180.0;
    mA->xspacing = (mA->xright - mA->xleft)/(double)(nX-1);
    mA->ylower = 0.0;
    mA->yupper = 90.0;
    mA->yspacing = (mA->yupper - mA->ylower)/(double)(nY-1);
    mA->i_min = NINT((mA->xleft)/mA->xspacing);
    mA->i_max = NINT((mA->xright)/mA->xspacing);
    mA->j_min = NINT((mA->ylower)/mA->yspacing);
    mA->j_max = NINT((mA->yupper)/mA->yspacing);
    
    mE = copy_map(get_maplist(), mA);
    if (!mE) return;
    sprintf(mE->name, "El corr %s", sopt.p->name);
    
    for (i=0; i<nX; i++) {
        A = mA->xleft + (double)i * mA->xspacing;
        for (j=0; j<nY; j++) {
            E = mA->ylower + (double)j * mA->yspacing;
	    mA->d[i][j] = AzPoi2DValue(A, E, p, nPar);
	    mA->e[i][j] = 0.0;
	    mA->f[i][j] = UNBLANK;
	    mE->d[i][j] = ElPoi2DValue(A, E, p, nPar);
	    mE->e[i][j] = 0.0;
	    mE->f[i][j] = UNBLANK;
	}
    }
}

static void PostScatterParameterDialog(Widget w, Widget *sform, XtPointer cd)
{
    Widget parent = w, rc0, rc, fr, pb;
    int n, nPar;
    char *desc = NULL;
    StdForm *sf;
        
    while (!XtIsWMShell(parent))
        parent = XtParent(parent);

    if (!(nPar = GetCurrentNPar())) {
        PostErrorDialog(*sform, "Select a valid type of fit first.");
        return;
    }
    
    sf = PostStdFormDialog(parent, "Scatter fit parameters",
             BUTT_APPLY, (XtCallbackProc)UpdateScatterOpts, NULL,
             BUTT_CANCEL, (XtCallbackProc)cancel_parameter_dialog, NULL,
             NULL, NULL, NULL, 0, NULL);

    rc0 = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmVERTICAL,
                          NULL);
    fr   = XtVaCreateWidget("frame", xmFrameWidgetClass, rc0,
				            XmNshadowType, XmSHADOW_OUT, NULL);
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, fr,
                          XmNorientation, XmHORIZONTAL,
                          XmNnumColumns, nPar+1,
                          XmNadjustLast, False,
                          XmNpacking, XmPACK_COLUMN,
                          NULL);
				    
    XtCreateManagedWidget("Parameter", xmLabelWidgetClass,
                          rc, NULL, 0);
    XtCreateManagedWidget("Fitted/fixed", xmLabelWidgetClass,
                          rc, NULL, 0);
    XtCreateManagedWidget("Value", xmLabelWidgetClass,
                          rc, NULL, 0);
    XtCreateManagedWidget("Uncertainty", xmLabelWidgetClass,
                          rc, NULL, 0);
    for (n=0; n<nPar; n++) {
        if (sopt.fit == SCATTER_LINE_FIT) {
            desc = scatterLineDescs[n];
        } else if (sopt.fit == SCATTER_POLY_FIT) {
            desc = scatterPolyDescs[n];
        } else if (sopt.fit == SCATTER_GAUSS_FIT) {
            desc = scatterGaussDescs[n];
        } else if (sopt.fit == SCATTER_LORENTZ_FIT) {
            desc = scatterLorentzDescs[n];
        } else if (sopt.fit == SCATTER_EXP_FIT) {
            desc = scatterExpDescs[n];
        } else if (sopt.fit == SCATTER_ERFC_FIT) {
            desc = scatterErfcDescs[n];
        } else if (sopt.fit == SCATTER_EXPPROF1_FIT) {
            desc = scatterExpProf1Descs[n];
        } else if (sopt.fit == SCATTER_EXPPROF2_FIT) {
            desc = scatterExpProf2Descs[n];
        } else if (sopt.fit == SCATTER_EXPPROF3_FIT) {
            desc = scatterExpProf3Descs[n];
        } else if (sopt.fit == SCATTER_COMET_FIT) {
            desc = scatterCometDescs[n];
        } else if (sopt.fit == SCATTER_QUAD_FIT) {
            desc = scatterQuadDescs[n];
        } else if (sopt.fit == SCATTER_FOURIER_FIT) {
            desc = scatterFourierDescs[n];
        } else if (sopt.fit == SCATTER_INVPOLY_FIT) {
            desc = scatterInvPolyDescs[n];
        } else if (sopt.fit == SCATTER_DISGAU_FIT) {
            desc = scatterDisGauDescs[n];
        } else if (sopt.fit == SCATTER_POI_FIT) {
            desc = scatterPointingDescs[n];
        }
        XtCreateManagedWidget(desc, xmLabelWidgetClass,
                              rc, NULL, 0);
        spar[n].f = XtVaCreateManagedWidget(spar[n].fit ? "Fitted" : "Fixed",
                                            xmToggleButtonWidgetClass,
                                            rc, XmNset,
                                            spar[n].fit ? True : False,
                                            NULL);
        spar[n].e = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                          rc, NULL, 0);
        spar[n].error = XtCreateManagedWidget("label", xmLabelWidgetClass,
                                              rc, NULL, 0);
        XtAddCallback(spar[n].f, XmNvalueChangedCallback,
                      (XtCallbackProc)ToggleFitCallback, &(spar[n]));
    }
    
    pb = XtVaCreateManagedWidget("Save current fit as scatter",
                                 xmPushButtonWidgetClass, rc0, NULL);
    XtAddCallback(pb, XmNactivateCallback,
                  (XtCallbackProc)saveFitasScatter, sf);
    ArrangeStdFormDialog(sf, fr);

    XtManageChild(rc);
    XtManageChild(fr);
    XtManageChild(rc0);
    
    ManageDialogCenteredOnPointer(sf->form);
    
    UpdateParameterWidgets();
}

static void ToggleUpdateCallback(Widget w, XtPointer xp,
                                 XmToggleButtonCallbackStruct *cd)
{
    if (cd->set) {
        sopt.updateError = 1;
    } else {
        sopt.updateError = 0;
    }
}

static void cancel_scatter_option_dialog(Widget w, StdForm *sf,
                                         XmAnyCallbackStruct *cd)
{
    int n;
    
    for (n=0; n<nSODs; n++) {
        sopt.e[n] = NULL;
    }
    for (n=0; n<MAX_PAR; n++) {
        spar[n].f = spar[n].e = spar[n].error = NULL;
    }
    if (sf && sf->form) XtDestroyWidget(sf->form);
}

void PostScatterOptionDialog(Widget w, char *cmd, XtPointer cd)
{
    int n;
    Widget parent=w, rc, rc1, rcH, fr, menu, par, update, join, label, stat;
    StdForm *sf;
    
    while (!XtIsWMShell(parent))
        parent = XtParent(parent);
    
    sf = PostStdFormDialog(parent, "Scatter plot options",
             BUTT_APPLY, (XtCallbackProc)UpdateScatterOpts, NULL,
             BUTT_CANCEL, (XtCallbackProc)cancel_scatter_option_dialog, NULL,
             BUTT_HELP, NULL, plotopt_help,
             0, NULL);

    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                           XmNorientation, XmVERTICAL,
                           NULL);
    fr = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rc,
				                 XmNshadowType, XmSHADOW_OUT, NULL);
    rc1 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                  XmNorientation, XmHORIZONTAL,
                                  XmNnumColumns, nSODs,
                                  XmNadjustLast, False,
                                  XmNpacking, XmPACK_COLUMN,
                                  NULL);
				    
    for (n=0; n<nSODs; n++) {
        XtCreateManagedWidget(scatterOptDescs[n], xmLabelWidgetClass,
                              rc1, NULL, 0);
        sopt.e[n] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                          rc1, NULL, 0);
    }
    
    rcH = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                  XmNorientation, XmHORIZONTAL,
                                  NULL);
    join = CreateOptionMenu(rcH, &JoinOptionMenu);
    SetDefaultOptionMenuItem(join, sopt.join);

    label = CreateOptionMenu(rcH, &LabelOptionMenu);
    SetDefaultOptionMenuItem(label, sopt.label);
    
    stat = CreateOptionMenu(rc, &StatOptionMenu);
    SetDefaultOptionMenuItem(stat, sopt.stat);
    
    menu = CreateOptionMenu(rc, &FitOptionMenu);
    SetDefaultOptionMenuItem(menu, sopt.fit);

    par = XtCreateManagedWidget("Set fitting parameters...",
                                xmPushButtonWidgetClass,
                                rc, NULL, 0);
    XtAddCallback(par, XmNactivateCallback,
                  (XtCallbackProc)PostScatterParameterDialog, sf->form);
 
    update = XtVaCreateManagedWidget("Update y errors using fit",
                                     xmToggleButtonWidgetClass, rc,
                                     XmNset, sopt.updateError ? True : False,
                                     NULL);
    XtAddCallback(update, XmNvalueChangedCallback,
                  (XtCallbackProc)ToggleUpdateCallback, NULL);
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(stat);
    XtManageChild(menu);
    XtManageChild(join);
    XtManageChild(label);
    XtManageChild(rc);
    
    ManageDialogCenteredOnPointer(sf->form);
    
    UpdateScatterWidgets();
}

static void UpdateScatterTypePlot(int xtype, int ytype, int scale)
{    
    int prev_mode = vP->mode;
    
    void SetDefWindow(), draw_main(), SetStdView();
    
    MakeSpeScatterPlot(vP->from, xtype, ytype);
    
    SetViewMode(SHOW_SCATTER, vP->s, vP->m, &scat);
    if (prev_mode == SHOW_POSPOS) SetStdView();
    SetDefWindow(scale);
    draw_main();
}

void PostScatterTypeDialog(Widget w, char *cmd, XtPointer cd)
{
    Widget parent = w, rc, menux, menuy;
    string item_str;
    StdForm *sf;
    
    while (!XtIsWMShell(parent))
        parent = XtParent(parent);
    
    sf = PostStdFormDialog(parent, "Scatter parameter options",
             NULL, NULL, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 0, NULL);
    
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmHORIZONTAL,
                          XmNnumColumns, 2,
                          XmNadjustLast, False,
                          XmNpacking, XmPACK_COLUMN,
                          NULL);
    
    menux = CreateOptionMenu(rc, &XTypeOptionMenu);
    menuy = CreateOptionMenu(rc, &YTypeOptionMenu);
    /* SetDefaultOptionMenuItem(menux, sopt.xtype); */
    sprintf(item_str, "%d", sopt.xtype);
    SetDefaultOptionMenuItemString(menux, XTypeMenuData, item_str);
    /* SetDefaultOptionMenuItem(menuy, sopt.ytype); */
    sprintf(item_str, "%d", sopt.ytype);
    SetDefaultOptionMenuItemString(menuy, YTypeMenuData, item_str);
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(menux);
    XtManageChild(menuy);
    XtManageChild(rc);
    
    ManageDialogCenteredOnPointer(sf->form);
    
    UpdateScatterTypePlot(sopt.xtype, sopt.ytype, SCALE_BOTH);
}

static void JoinScatterCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str);

    void draw_main();
    
    if (n != sopt.join) {
        sopt.join = n;
        if (vP->mode == SHOW_SCATTER) draw_main();
    }
}

static void LabelScatterCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str);

    void draw_main();
    
    if (n != sopt.label) {
        sopt.label = n;
        if (vP->mode == SHOW_SCATTER) draw_main();
    }
}

static void FitScatterCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str);

    void draw_main();
    
    if (n != sopt.fit) {
        sopt.fit = n;
        for (n=0; n<MAX_PAR; n++) ToggleFitCallback(w, &spar[n], NULL);
        
        if (vP->mode == SHOW_SCATTER) draw_main();
    }
}

static void StatScatterCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str);

    void draw_main();
    
    if (n != sopt.stat) {
        sopt.stat = n;
        if (vP->mode == SHOW_SCATTER) draw_main();
    }
}

static void XTypeOfScatterCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str);
    
    UpdateScatterTypePlot(n, sopt.ytype, SCALE_ONLY_X);
    scat.xtype = sopt.xtype = n;
}

static void YTypeOfScatterCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str);
    
    UpdateScatterTypePlot(sopt.xtype, n, SCALE_ONLY_Y);
    scat.ytype = sopt.ytype = n;
}

void SaveAndViewScatterTable(char *file)
{
    int i, n;
    double x, y, ex, ey;
    char *xstr, *ystr;
    string buf;
    FILE *fp;
    scatter *p = vP->p;
    scanPtr s = NULL;
    
    char *GetRAStr();
    char *GetDECStr();
    void  XS_system();
    
    if (!p) {
        PostWarningDialog(NULL,
             "There is no current scatter plot to make a table of.");
        return;
    }
    
    fp = fopen(file, "w");
    
    if (!fp) {
        sprintf(buf, "Couldn't open file '%s' for writing table.", file);
        PostWarningDialog(NULL, buf);
        return;
    }
    
    n = p->nData;
    
    fprintf(fp, "# Source: %s   Size: %d\n", p->name, n);
    fprintf(fp, "# Centre coordinates: %s %s\n",
            GetRAStr(p->x0), GetDECStr(p->y0));
    fprintf(fp, "# Molecule: %s\n", p->molecule);
    xstr = GetScatterTypeStr('x', p->xtype);
    ystr = GetScatterTypeStr('y', p->ytype);
    fprintf(fp, "# X type: %s   Y type: %s\n", xstr, ystr);
    fprintf(fp, "#  n        X         Y\n");
    for (i=0; i<n; i++) {
        x = p->x[i];
        y = p->y[i];
        ex = p->ex[i];
        ey = p->ey[i];
	s = p->sp[i];
	if (s) {
            fprintf(fp, "%5d %12.5e (%12.5e) %12.5e (%12.5e) %4d %s\n",
                    i+1, x, ex, y, ey, s->scan_no, s->name);
	} else {
            fprintf(fp, "%5d %12.5e (%12.5e) %12.5e (%12.5e)\n",
                    i+1, x, ex, y, ey);
        }
    }
    
    fclose(fp);
    
    sprintf(buf, "%s %s &", pP->editor, file);
    XS_system(buf, 1);
    
    return;
}

int SaveScatterFile(char *file)
{
    int i, n;
    double x, y, ex, ey;
    string buf;
    FILE *fp;
    scatter *p = vP->p;
    
    if (!p) {
        PostWarningDialog(NULL,
             "There is no current scatter plot to save.");
        return 1;
    }
    
    fp = fopen(file, "w");
    
    if (!fp) {
        sprintf(buf, "Couldn't open file '%s' for writing.", file);
        PostWarningDialog(NULL, buf);
        return 1;
    }
    
    n = p->nData;
    
    fprintf(fp, "@Source %s\n", p->name);
    fprintf(fp, "@X0 %f\n", p->x0);
    fprintf(fp, "@Y0 %f\n", p->y0);
    fprintf(fp, "@Molecule %s\n", p->molecule);
    fprintf(fp, "@NData %d\n", n);
    fprintf(fp, "@XType %d\n", p->xtype);
    fprintf(fp, "@YType %d\n", p->ytype);
    for (i=0; i<n; i++) {
        x = p->x[i];
        y = p->y[i];
        ex = p->ex[i];
        ey = p->ey[i];
        fprintf(fp, "%12.5e %12.5e %12.5e %12.5e\n", x, y, ex, ey);
    }
    
    fclose(fp);
    
    return 0;
}

static int ret_RSF(double *x, double *y, double *ey, double *ex, int ret)
{
    if (x) free(x);
    if (y) free(y);
    if (ex) free(ex);
    if (ey) free(ey);
    
    return ret;
}

int ReadScatterFile(char *file)
{
    int n, nstep=500, nsize=500, prev_mode, np;
    int xtype=-1, ytype=-1;
    string buf, name, molecule;
    FILE *fp;
    double *x, *y, *ex, *ey, X, Y, EX, EY;
    
    void SetStdView();
    void keyword_strcpy(char *, char *);
    char *GetFileType(const char *);
    char *StripSuffix(const char *), *StripPath(const char *);
    
    fp = fopen(file, "r");
    
    if (!fp) {
        sprintf(buf, "Couldn't open file '%s' for reading.", file);
        PostWarningDialog(NULL, buf);
        return 1;
    }
    
    x  = (double *)malloc(nsize * sizeof(double));
    if (!x) return 1;
    y  = (double *)malloc(nsize * sizeof(double));
    if (!y) return ret_RSF(x, NULL, NULL, NULL, 1);
    ex = (double *)malloc(nsize * sizeof(double));
    if (!ex) return ret_RSF(x, y, NULL, NULL, 1);
    ey = (double *)malloc(nsize * sizeof(double));
    if (!ey) return ret_RSF(x, y, ex, NULL, 1);
    
    strcpy(name, "<empty>");
    strcpy(molecule, "");
    n = 0;
    while ((fgets(buf, sizeof(buf)-1, fp)) != NULL) {
        if (buf[0] == '@') {                         /* keyword */
            if (strncmp(buf, "@Source", 7) == 0) {
                keyword_strcpy(name, &buf[8]);
            } else if (strncmp(buf, "@Molecule", 9) == 0) {
                keyword_strcpy(molecule, &buf[10]);
            } else if (sscanf(buf, "@XType %d", &np) == 1) {
                xtype = np;
            } else if (sscanf(buf, "@YType %d", &np) == 1) {
                ytype = np;
            }
            continue;
        } else if (buf[0] == '#' || buf[0] == '!') { /* comment */
            continue;
        } else {                                     /* data    */
            if (n == nsize) {
                nsize += nstep;
                x  = (double *)realloc(x,  nsize * sizeof(double));
                y  = (double *)realloc(y,  nsize * sizeof(double));
                ex = (double *)realloc(ex, nsize * sizeof(double));
                ey = (double *)realloc(ey, nsize * sizeof(double));
                if (!x || !y || !ex || !ey) {
                    fclose(fp);
                    return ret_RSF(x, y, ex, ey, 1);
                }
            }
            np = sscanf(buf, "%lf , %lf , %lf , %lf", &X, &Y, &EX, &EY);
	    if (np != 4 && np != 2)
                np = sscanf(buf, "%lf %lf %lf %lf", &X, &Y, &EX, &EY);
            if (np == 4) {
                x[n]  = X;   y[n] = Y;
                ex[n] = EX; ey[n] = EY;
                n++;
            } else if (np == 2) {
                x[n]  = X;    y[n] = Y;
                ex[n] = 0.0; ey[n] = 0.0;
                n++;
	    }
        }
    }
    
    fclose(fp);
    
    if (AllocScatter(n)) {
        sprintf(buf, "Couldn't allocate scatter data (%d).", n);
        PostErrorDialog(NULL, buf);
        return ret_RSF(x, y, ex, ey, 1);
    }
    if (GetFileType(file) && strcmp(GetFileType(file), "pointing")==0) {
        sprintf(name, "%s", StripSuffix(StripPath(file)));
	strcpy(molecule, "pointing");
        xtype = XTYPE_SCA_AZ;
	ytype = YTYPE_SCA_EL;
    }
    
    strcpy(scat.name, name);
    strcpy(scat.molecule, molecule);
    scat.xtype = xtype;
    scat.ytype = ytype;

    for (n=0; n<scat.nData; n++) {
        scat.x[n] = x[n];
        scat.y[n] = y[n];
        scat.ex[n] = ex[n];
        scat.ey[n] = ey[n];
        scat.sp[n] = NULL;
        scat.t[n] = 0;
    }
    
    set_scatter_minmax(&scat);
    
    prev_mode = vP->mode;
    SetViewMode(SHOW_SCATTER, vP->s, vP->m, &scat);
    if (prev_mode == SHOW_POSPOS) SetStdView();
    
    UpdateData(SCALE_BOTH, REDRAW);
    
    /* SetDefWindow(SCALE_BOTH);
    draw_main(); */
    
    return ret_RSF(x, y, ex, ey, 0);
}

void scatter_pnts_handling(Widget w, char *cmd, Point p1, Point p2)
{
    int n, inside = 0;
    double tmp;
    scatter *p = vP->p;
    
    void draw_main();
    
    if (vP->mode != SHOW_SCATTER || !p) return;
    
    if (p1.x > p2.x) {
        tmp = p1.x; p1.x = p2.x; p2.x = tmp;
    }
    if (p1.y > p2.y) {
        tmp = p1.y; p1.y = p2.y; p2.y = tmp;
    }
    
    for (n=0; n<p->nData; n++) {
        if (p->x[n] >= p1.x && p->x[n] <= p2.x &&
            p->y[n] >= p1.y && p->y[n] <= p2.y) inside = 1;
        else
            inside = 0.0;
        if (strcmp(cmd, "tag_inside")==0) {
            if (inside) p->t[n] = 1;
        } else if (strcmp(cmd, "untag_inside")==0) {
            if (inside) p->t[n] = 0;
        } else if (strcmp(cmd, "tag_outside")==0) {
            if (!inside) p->t[n] = 1;
        } else if (strcmp(cmd, "untag_outside")==0) {
            if (!inside) p->t[n] = 0;
        }
    }
    
    draw_main();
}
