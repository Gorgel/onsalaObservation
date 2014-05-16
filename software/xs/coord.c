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

#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Frame.h>

#include "defines.h"
#include "global_structs.h"
#include "dialogs.h"

#define SECPERREV 1296000.0         /* arcsec per full circle     */

/*** External variables ***/
extern VIEW  *vP;

void PostErrorDialog(Widget, char *);
void PostWarningDialog(Widget, char *);
void PostMessageDialog(Widget, char *);
void ManageDialogCenteredOnPointer(Widget);

void wprintf();
void wsscanf(Widget, char *);
void UpdateData(int, int);
void obtain_map_info(Widget, char *, XtPointer);

list    scan_iterator(list, DataSetPtr);
int     count_scans(DataSetPtr);

/*** Local variables ***/
static int OnlyOffsets=1, ConvertCoordSystem=0;
static double siteLongitude, siteLatitude;

typedef struct { double l,b; } pVector;
typedef double cVector[3];
typedef double rotMatrix[3][3];

static char *std_coord_str[4] = {
    "RA: [AAhBBmCCs]", "Dec.: [AAdBB'CC\"]",
    "RA offset [\"]:", "Dec. offset [\"]:"
};

static char *gal_coord_str[4] = {
    "Gal. long.: [AAA.AAA]", "Gal. lat.: [AA.AAA]",
    "Offset [\"]:", "Offset [\"]:"
};

void SetSiteCoord(double lon, double lat)
{
    siteLongitude = lon;
    siteLatitude  = lat;
}

void GetSiteCoord(double *lon, double *lat)
{
    if (lon) *lon = siteLongitude;
    if (lat) *lat = siteLatitude;
}

/*
 * Calculation of precession matrix
 * input:
 *     Js:                 Julian date of starting epoch
 *     Je:                 Julian date of ending epoch
 * output:
 *     PreMat:             matrix describing precession from Js to Je
 */
static void pre(double Js, double Je, rotMatrix PreMat)
{
   double t,T;
   double zeta,z,theta;
 
   T = (Js-2451545.0)/36525.0;
   t = (Je-Js)/36525.0;
   zeta  = ( (2306.2181+1.39656*T-0.000139*T*T)*t
	   + (0.30188-0.000344*T)*t*t + 0.017998*t*t*t )*(2.0*PI)/SECPERREV;
   z     = ( (2306.2181+1.39656*T-0.000139*T*T)*t
	   + (1.09468+0.000066*T)*t*t + 0.018203*t*t*t )*(2.0*PI)/SECPERREV;
   theta = ( (2004.3109-0.85330*T-0.000217*T*T)*t
	   - (0.42665+0.000217*T)*t*t - 0.041833*t*t*t )*(2.0*PI)/SECPERREV;
   PreMat[0][0] =  cos(zeta)*cos(theta)*cos(z)-sin(zeta)*sin(z);
   PreMat[0][1] = -sin(zeta)*cos(theta)*cos(z)-cos(zeta)*sin(z);
   PreMat[0][2] = -sin(theta)*cos(z);
   PreMat[1][0] =  cos(zeta)*cos(theta)*sin(z)+sin(zeta)*cos(z);
   PreMat[1][1] = -sin(zeta)*cos(theta)*sin(z)+cos(zeta)*cos(z);
   PreMat[1][2] = -sin(theta)*sin(z);
   PreMat[2][0] =  cos(zeta)*sin(theta);
   PreMat[2][1] = -sin(zeta)*sin(theta);
   PreMat[2][2] =  cos(theta);
}
		 
void cuv(pVector *p, cVector v)
{
   v[0] = cos(p->l)*cos(p->b);
   v[1] = sin(p->l)*cos(p->b);
   v[2] = sin(p->b);
}
 
void uvc(cVector v, pVector *p)
{
   if (v[0] == 0.0 && v[1] == 0.0) {
      p->l = 0.0;
      if (v[2] > 0.0) p->b =  PI/2.0;
      else            p->b = -PI/2.0;
   } else {
      p->l = atan2(v[1],v[0]);
      if (p->l < 0.0)  p->l += 2.0*PI;
      p->b = atan(v[2]/sqrt(v[0]*v[0]+v[1]*v[1]));
   }
}
 
void rotate(rotMatrix m, cVector vo, cVector vn)
{
    cVector v;

    v[0] = m[0][0]*vo[0]+m[0][1]*vo[1]+m[0][2]*vo[2];
    v[1] = m[1][0]*vo[0]+m[1][1]*vo[1]+m[1][2]*vo[2];
    v[2] = m[2][0]*vo[0]+m[2][1]*vo[1]+m[2][2]*vo[2];
    vn[0] = v[0];
    vn[1] = v[1];
    vn[2] = v[2];
}
 
void trc(rotMatrix m, pVector *old, pVector *new)
{
   static double vo[3], vn[3];
 
   cuv(old,vo);
   rotate(m,vo,vn);
   uvc(vn,new);
}

static void PrecessTo1950(scanPtr s, double *ra1950, double *de1950)
{
    double JDref, JD1950;
    rotMatrix rot;
    pVector p, pold;
    
    JDref  = 2451545.0 + (s->equinox - 2000.0)*365.25;
    JD1950 = 2433282.42346;
    
    pold.l = s->x0;
    pold.b = s->y0;
    
    pre(JDref, JD1950, rot);
    trc(rot, &pold, &p);
    
    *ra1950 = p.l;
    *de1950 = p.b;
}

void PrecessSpe(scanPtr s, char *new)
{
    char c;
    double JDref, JDnew, e;
    rotMatrix rot;
    pVector p, pold;
    
    if (s->epoch == 'J' || s->epoch == 'j') {
        JDref  = 2451545.0 + (s->equinox - 2000.0)*365.25;
    } else if (s->epoch == 'B' || s->epoch == 'b') {
        JDref  = 2415020.31352 + (s->equinox - 1900.0)*365.242198781;
    } else { /* we guess that <= 1950 is Besselian */
        if (s->equinox <= 1950.1) {
            JDref  = 2415020.31352 + (s->equinox - 1900.0)*365.242198781;
        } else {
            JDref  = 2451545.0 + (s->equinox - 2000.0)*365.25;
        }
    }
    
    sscanf(new, "%1c%lf", &c, &e);
    
    if (c == 'J' || c == 'j') {
        JDnew =  2451545.0 + (e - 2000.0)*365.25;
    } else {
        JDnew  = 2415020.31352 + (e - 1900.0)*365.242198781;
    }
    
    if (JDref == JDnew) return;
    
    pold.l = s->x0;
    pold.b = s->y0;
    
    pre(JDref, JDnew, rot);
    trc(rot, &pold, &p);
    
    s->x0 = p.l;
    s->y0 = p.b;
    s->epoch = c;
    s->equinox = e;
}

void PrecessMap(MAP *m, char *new)
{
    char c;
    double JDref, JDnew, e;
    rotMatrix rot;
    pVector p, pold;
    
    if (m->epoch == 'J' || m->epoch == 'j') {
        JDref  = 2451545.0 + (m->equinox - 2000.0)*365.25;
    } else if (m->epoch == 'B' || m->epoch == 'b') {
        JDref  = 2415020.31352 + (m->equinox - 1900.0)*365.242198781;
    } else { /* we guess that <= 1950 is Besselian */
        if (m->equinox <= 1950.1) {
            JDref  = 2415020.31352 + (m->equinox - 1900.0)*365.242198781;
        } else {
            JDref  = 2451545.0 + (m->equinox - 2000.0)*365.25;
        }
    }
    
    sscanf(new, "%1c%lf", &c, &e);
    
    if (c == 'J' || c == 'j') {
        JDnew =  2451545.0 + (e - 2000.0)*365.25;
    } else {
        JDnew  = 2415020.31352 + (e - 1900.0)*365.242198781;
    }
    
    if (JDref == JDnew) return;
    
    pold.l = m->x0;
    pold.b = m->y0;
    
    pre(JDref, JDnew, rot);
    trc(rot, &pold, &p);
    
    m->x0 = p.l;
    m->y0 = p.b;
    m->epoch = c;
    m->equinox = e;
}

int DatetoMJDN(DATE *d)
{
    long int jdn, mjdn, a, y, m;
    
    if (!d) return 0;
    
    a = (14 - (long int)d->Month) / 12;
    y = (long int)d->Year + 4800 - a;
    m = (long int)d->Month + 12*a - 3;
    
    jdn = (long int)d->Day;
    jdn += (153*m + 2)/5;
    jdn += 365*y;
    jdn +=  y / 4;
    jdn -=  y / 100;
    jdn +=  y / 400;
    jdn -=  32045;
    
    mjdn = jdn - 2400000;
    
    return (int)mjdn;
}

void MJDNtoDate(DATE *dp, int mjdn)
{
    long int jdn, j, g, dg, c, dc, b, db, a, da, y, m , d;
    long int Y, M, D;
    
    if (!dp) return;
    
    jdn =  mjdn + 2400001;
    
    j = jdn + 32044;
    g = j / 146097;          dg = j % 146097;
    c = dg / 36524;          dc = dg - c * 36524;
    b = dc / 1461;           db = dc % 1461;
    a = ((db/365 + 1)*3)/4 ; da = db - a*365;
    y = g*400 + c*100 + b*4 + a;
    m = (da*5 + 308)/153 - 2;
    d = da - ((m+4)*153)/5 + 122;
    Y = y - 4800 + (m+2)/12;
    M = ((m+2) % 12) + 1;
    D = d + 1;
    
    dp->Year = (int)Y;
    dp->Month = (int)M;
    dp->Day = (int)D;
}

double Vlsrheldiff(scanPtr s)
{
    double vs, ras, des, rad, ded, vrs;

    vs = 20.0;
    ras = 270.5/RADTODEG;
    des = 30.0/RADTODEG;
    if (s->equinox == 1950.0) {
        rad = s->x0;
        ded = s->y0;
    } else {
        PrecessTo1950(s, &rad, &ded);
    }
    vrs = vs * (cos(ras)*cos(des)*cos(rad)*cos(ded) +
                sin(ras)*cos(des)*sin(rad)*cos(ded) +
                sin(des)*sin(ded));
    
    return vrs;
}

/* Conversion from Galactic coordinates (in radians) to equatorial
   coordinates (in radians), epoch J2000 */
static void GalToEqu(double l_rad, double b_rad, double *ra, double *decl)
{
    static rotMatrix rot = {
          { -0.054875539726,  0.494109453312, -0.867666135858 },
          { -0.873437108010, -0.444829589425, -0.198076386122 },
          { -0.483834985808,  0.746982251810,  0.455983795705 } };
    pVector pgal, pequ;
    
    pgal.l = l_rad;
    pgal.b = b_rad;
    
    trc(rot, &pgal, &pequ);
    
    if (ra) *ra = pequ.l;
    if (decl) *decl = pequ.b;
}

static void Gal2EquSpe(scanPtr s)
{
    double x, y, x0, y0;
    double ra, decl;
    
    if (!s) return;
    
    /* Convert the center coordinates */
    x = x0 = s->x0;
    y = y0 = s->y0;
    GalToEqu(x, y, &ra, &decl);
    s->x0 = ra;
    s->y0 = decl;
    s->epoch = 'J';
    s->equinox = 2000.0;
    s->coordType = COORD_TYPE_EQU;
    
    /* Convert also the offsets */
    x = x0 + s->xoffset/3600.0/RADTODEG/cos(y0);
    y = y0 + s->yoffset/3600.0/RADTODEG;
    GalToEqu(x, y, &ra, &decl);
    s->xoffset = (ra - s->x0)*cos(s->y0)*3600.0*RADTODEG;
    s->yoffset = (decl - s->y0)*3600.0*RADTODEG;
}

static void Hor2Equ(double a, double e, double LST, double SiteLat,
                    double *ra, double *decl)
{
    double sinH, cosH, sind;
    
    /* Conversion from Az, El to apparent RA and Dec
       given a site latitude and LST, see The Astronomical Almanac, B58 */
    
    if (ra) {
        sinH = -cos(e)*sin(a);
        cosH = sin(e)*cos(SiteLat) - cos(e)*cos(a)*sin(SiteLat);
        *ra  = LST - atan2(sinH, cosH);
        if (*ra < 0.0) *ra += 2.0*PI;
    }
    if (decl) {
        sind = sin(e)*sin(SiteLat) + cos(e)*cos(a)*cos(SiteLat);
        *decl = asin(sind);
    }
}

/* All absolute angles in radians here, offsets are in arcsecs */
void GetEquOffsets(DATE *d, double Az, double El, double AzOff, double ElOff,
                   double RA, double Dec,
                   double *roff, double *doff)
{
    double LST;
    double SiteLat = siteLatitude/RADTODEG;
    double x, y, x_off, y_off;
    
    /* char *GetRAStr(double), *GetDECStr(double); */
    
    LST = ((double)d->Hour + (double)d->Min/60.0 +
           (double)d->Sec/3600.0)/RADTOHR;

    Hor2Equ(Az, El, LST, SiteLat, &x, &y);
    Hor2Equ(Az + AzOff/cos(El)/RADTOSEC, El + ElOff/RADTOSEC,
            LST, SiteLat, &x_off, &y_off);
            
    /* printf("%s %s     ", GetRAStr(RA), GetDECStr(Dec));
    printf("%s %s\n", GetRAStr(x), GetDECStr(y)); */
    
    if (roff) *roff = (x_off - x)*cos(y)*RADTOSEC;
    if (doff) *doff = (y_off - y)*RADTOSEC;
}

char *GetCoordType(int type)
{
    static string buf;
    
    if (type == COORD_TYPE_EQU) {
        sprintf(buf, "Equatorial");
    } else if (type == COORD_TYPE_HOR) {
        sprintf(buf, "Horizontal");
    } else if (type == COORD_TYPE_GAL) {
        sprintf(buf, "Galactic");
    } else {
        sprintf(buf, "Unknown");
    }
    
    return buf;
}

char *GetEpochStr(char ep, double eq)
{
    static string buf;
    
    sprintf(buf, "%1c%6.1f", ep, eq);
    
    return buf;
}

char *GetLongStr(double rad)
{
    static string buf;
    double d;
    
    d = rad*RADTODEG;
    
    sprintf(buf, "%9.4f", d);
    
    return buf;
}

char *GetLatStr(double rad)
{
    static string buf;
    double d;
    
    d = rad*RADTODEG;
    
    sprintf(buf, "%8.4f", d);
    
    return buf;
}

char *GetRAStr(double rad)
{
    static string buf;
    int hr, min;
    double h, sec;
    
    h = rad*RADTOHR;
    
    hr = (int)floor(h);
    
    h -= (double)hr;
    
    h = h*60.0;
    
    min = (int)floor(h);
    
    h -= (double)min;
    
    sec = h*60.0;
    
    sprintf(buf, "%dh%02dm%05.2fs", hr, min, sec);
    
    return buf;
}

char *GetDECStr(double rad)
{
    static string buf;
    int neg = 0, deg, min;
    double h, sec;
    
    if (rad < 0) {
        h = -rad*RADTODEG;
        neg  = 1;
    } else
        h = rad*RADTODEG;
    
    deg = (int)floor(h);
    
    h -= (double)deg;
    
    h = h*60.0;
    
    min = (int)floor(h);
    
    h -= (double)min;
    
    sec = h*60.0;
    
    if (neg)
        sprintf(buf, "-%dd%02d'%04.1f\"", deg, min, sec);
    else
        sprintf(buf, "%dd%02d'%04.1f\"", deg, min, sec);
    
    return buf;
}

double *DegStr2Rad(char *s)
{
    static double rad;
    
    if (!s) return NULL;
    
    if (sscanf(s, "%lf", &rad) != 1)
        return NULL;
    
    rad /= RADTODEG;
    
    return &rad;
}

double *RAStr2Rad(char *s)
{
    double hr, min, sec;
    static double rad;
    
    if (!s) return NULL;
    
    if (sscanf(s, "%lfh%lfm%lfs", &hr, &min, &sec) != 3)
        return NULL;
    
    if (hr < 0.0 || hr >= 24.0) return NULL;
    if (min < 0.0 || min >= 60.0) return NULL;
    if (sec < 0.0 || sec >= 60.0) return NULL;
    
    rad = (hr + min/60.0 + sec/3600.0)/RADTOHR;
    
    return &rad;
}

double *DECStr2Rad(char *s)
{
    double deg, min, sec, sign=1.0;
    static double rad;
    
    if (!s) return NULL;
    
    if (sscanf(s, "%lfd%lf'%lf\"", &deg, &min, &sec) != 3)
        return NULL;
    
    if (deg < 0.0) {
        sign = -1.0;
        deg = -deg;
    }
    
    if (sign == 1.0 && min < 0.0) {
        sign = -1.0;
        min = -min;
    }
    
    if (sign == 1.0 && sec < 0.0) {
        sign = -1.0;
        sec = -sec;
    }
    
    if (deg < 0.0 || min < 0.0 || sec < 0.0) return NULL;
    
    if (deg >  90.0) return NULL;
    if (min >= 60.0) return NULL;
    if (sec >= 60.0) return NULL;
    
    rad = sign*(deg + min/60.0 + sec/3600.0)/RADTODEG;
    
    return &rad;
}

double *ASECStr2Rad(char *s)
{
    double asec;
    static double rad;
    
    if (!s) return NULL;
    
    if (sscanf(s, "%lf\"", &asec) != 1) return NULL;
    
    rad = asec/3600.0/RADTODEG;
    
    return &rad;
}

static void DoChangeCoord(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int cType;
    string tmp, buf;
    double *val, x=0.0, y=0.0, x_off, y_off;
    double new_x0, new_y0, new_xoffset, new_yoffset;
    list curr = NULL;
    scanPtr s = vP->s;
    
    if (!s) return;
    cType = s->coordType;
    
    if (!OnlyOffsets) {
        wsscanf(sf->edit[0], buf);
        if (cType == COORD_TYPE_GAL) {
            val = DegStr2Rad(buf);
            if (!val) {
                sprintf(tmp, "The string %s doesn't conform to AAA.AAA", buf);
                PostErrorDialog(w, tmp);
                return;
            }
        } else {
            val = RAStr2Rad(buf);
            if (!val) {
                sprintf(tmp, "The string %s doesn't conform to AAhBBmCCs", buf);
                PostErrorDialog(w, tmp);
                return;
            }
        }
        x = *val;

        wsscanf(sf->edit[1], buf);
        if (cType == COORD_TYPE_GAL) {
            val = DegStr2Rad(buf);
            if (!val) {
                sprintf(tmp, "The string %s doesn't conform to AAA.AAA", buf);
                PostErrorDialog(w, tmp);
                return;
            }
        } else {
            val = DECStr2Rad(buf);
            if (!val) {
                sprintf(tmp, "The string %s doesn't conform to AAdBB'CC\"",
                        buf);
                PostErrorDialog(w, tmp);
                return;
            }
        }
        y = *val;
    }
    
    wsscanf(sf->edit[2], buf);
    val = ASECStr2Rad(buf);
    if (!val) {
        sprintf(tmp, "The string %s doesn't conform to A\"", buf);
        PostErrorDialog(w, tmp);
        return;
    }
    x_off = *val;
    
    wsscanf(sf->edit[3], buf);
    val = ASECStr2Rad(buf);
    if (!val) {
        sprintf(tmp, "The string %s doesn't conform to A\"", buf);
        PostErrorDialog(w, tmp);
        return;
    }
    y_off = *val;
    
    
    new_x0 = x + x_off/cos(y);
    new_y0 = y + y_off;
        
    if (vP->mode == SHOW_SPE) {
        if (!s) {
            PostErrorDialog(w, "No current data!");
            return;
        }
        if (OnlyOffsets) {
            new_x0 = s->x0 + x_off/cos(s->y0);
            new_y0 = s->y0 + y_off;
        }
        new_xoffset = (s->x0-new_x0)*cos(s->y0)*3600.0*RADTODEG + s->xoffset;
        new_yoffset = (s->y0-new_y0)*3600.0*RADTODEG + s->yoffset;
        s->x0 = new_x0;
        s->y0 = new_y0;
        s->xoffset = new_xoffset;
        s->yoffset = new_yoffset;
    } else {
        while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
            s = (scanPtr)DATA(curr);
            if (OnlyOffsets) {
                new_x0 = s->x0 + x_off/cos(s->y0);
                new_y0 = s->y0 + y_off;
            }
            new_xoffset = (s->x0-new_x0)*cos(s->y0)*3600.0*RADTODEG +
                          s->xoffset;
            new_yoffset = (s->y0-new_y0)*3600.0*RADTODEG + s->yoffset;
            s->x0 = new_x0;
            s->y0 = new_y0;
            s->xoffset = new_xoffset;
            s->yoffset = new_yoffset;
        }
    }
    
    obtain_map_info(w, "no_update_map_data", NULL);
    
    UpdateData(SCALE_BOTH, REDRAW);
    
    if (vP->s && !OnlyOffsets) {
        if (cType == COORD_TYPE_GAL) {
            wprintf(sf->edit[0], "%s", GetLongStr(vP->s->x0));
            wprintf(sf->edit[1], "%s", GetLatStr(vP->s->y0));
        } else {
            wprintf(sf->edit[0], "%s", GetRAStr(vP->s->x0));
            wprintf(sf->edit[1], "%s", GetDECStr(vP->s->y0));
        }
    }
    wprintf(sf->edit[2], "%f\"", 0.0);
    wprintf(sf->edit[3], "%f\"", 0.0);
}

static void ToggleButtCallback(Widget w, StdForm *sf,
                               XmToggleButtonCallbackStruct *cd)
{
    if (cd->set) {
        OnlyOffsets = 1;
        wprintf(sf->edit[0], "%s", "");
        wprintf(sf->edit[1], "%s", "");
    } else {
        OnlyOffsets = 0;
        if (vP->s) {
            if (vP->s->coordType == COORD_TYPE_GAL) {
                wprintf(sf->edit[0], "%s", GetLongStr(vP->s->x0));
                wprintf(sf->edit[1], "%s", GetLatStr(vP->s->y0));
            } else {
                wprintf(sf->edit[0], "%s", GetRAStr(vP->s->x0));
                wprintf(sf->edit[1], "%s", GetDECStr(vP->s->y0));
            }
        }
    }
}

void PostCoordDialog(Widget wid, char *cmd, XtPointer call_data)
{
    int n;
    Widget fr, rc1, rc2, butt;
    Widget w = wid;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "New coordinates",
             BUTT_APPLY, (XtCallbackProc)DoChangeCoord, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             4, NULL);
    
    fr = XtVaCreateWidget("frame", xmFrameWidgetClass, sf->form,
                          XmNshadowType, XmSHADOW_OUT,
                          NULL);
    rc1 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                  XmNorientation, XmVERTICAL,
                                  NULL);
    XtCreateManagedWidget("New centre coordinates:", xmLabelWidgetClass,
                          rc1, NULL, 0);

    rc2 = XtVaCreateManagedWidget("velrc", xmRowColumnWidgetClass, rc1,
                                  XmNorientation, XmVERTICAL,
                                  XmNnumColumns, 2,
                                  XmNadjustLast, False,
                                  XmNpacking, XmPACK_COLUMN,
                                  NULL);
    
    for (n=0; n<4; n++) {
        if (vP->s->coordType == COORD_TYPE_GAL) {
            XtCreateManagedWidget(gal_coord_str[n], xmLabelWidgetClass,
                                  rc2, NULL, 0);
        } else {
            XtCreateManagedWidget(std_coord_str[n], xmLabelWidgetClass,
                                  rc2, NULL, 0);
        }
        sf->edit[n] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc2, NULL, 0);
    }
    
    butt = XtVaCreateManagedWidget("Use only offsets",
                                   xmToggleButtonWidgetClass, rc1,
                                   XmNset, OnlyOffsets ? True : False,
                                   NULL);
    XtAddCallback(butt, XmNvalueChangedCallback,
                  (XtCallbackProc)ToggleButtCallback, sf);
    
    ArrangeStdFormDialog(sf, fr);
    
    XtManageChild(fr);
    
    if (vP->s && !OnlyOffsets) {
        if (vP->s->coordType == COORD_TYPE_GAL) {
            wprintf(sf->edit[0], "%s", GetLongStr(vP->s->x0));
            wprintf(sf->edit[1], "%s", GetLatStr(vP->s->y0));
        } else {
            wprintf(sf->edit[0], "%s", GetRAStr(vP->s->x0));
            wprintf(sf->edit[1], "%s", GetDECStr(vP->s->y0));
        }
    } else {
        wprintf(sf->edit[0], "%s", "");
        wprintf(sf->edit[1], "%s", "");
    }
    wprintf(sf->edit[2], "%f\"", 0.0);
    wprintf(sf->edit[3], "%f\"", 0.0);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void DoPreCoord(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    char c;
    string epo, buf;
    double e;
    list curr = NULL;
    scanPtr s = vP->s;
    
    if (!s) {
        PostErrorDialog(w, "No current data!");
        return;
    }
    
    wsscanf(sf->edit[0], epo);
    if (sscanf(epo, "%1c%lf", &c, &e) != 2) {
        sprintf(buf, "'%s' does not conform to JXXXX.x or BXXXX.x", epo);
        PostErrorDialog(w, buf);
        return;
    }
    
    if (c != 'J' && c != 'j' && c != 'B' && c != 'b') {
        sprintf(buf, "'%s' does not conform to the Julian or Besselian epochs",
                epo);
        PostErrorDialog(w, buf);
        return;
    }
        
    if (vP->mode == SHOW_SPE) {
        if (s->coordType == COORD_TYPE_GAL && ConvertCoordSystem)
	   Gal2EquSpe(s);
        PrecessSpe(s, epo);
    } else {
        while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
            s = (scanPtr)DATA(curr);
            if (s->coordType == COORD_TYPE_GAL && ConvertCoordSystem)
	        Gal2EquSpe(s);
            PrecessSpe(s, epo);
        }
    }
    
    obtain_map_info(w, "no_update_map_data", NULL);
    
    UpdateData(SCALE_BOTH, REDRAW);
    
    wprintf(sf->edit[0], "%s", epo);
}

static void ToggleCoordSystemCallback(Widget w, StdForm *sf,
                               XmToggleButtonCallbackStruct *cd)
{
    if (cd->set) {
        ConvertCoordSystem = 1;
    } else {
        ConvertCoordSystem = 0;
    }
}

void PostPreDialog(Widget wid, char *cmd, XtPointer call_data)
{
    Widget fr, rc1, butt;
    Widget w = wid;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "New epoch",
             BUTT_APPLY, (XtCallbackProc)DoPreCoord, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             1, NULL);
    
    fr = XtVaCreateWidget("frame", xmFrameWidgetClass, sf->form,
                          XmNshadowType, XmSHADOW_OUT,
                          NULL);
    rc1 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                  XmNorientation, XmVERTICAL,
                                  NULL);
    
    butt = XtVaCreateManagedWidget("Galactic to equatorial conversion",
                                   xmToggleButtonWidgetClass, rc1,
                                   XmNset, ConvertCoordSystem ? True : False,
                                   NULL);
    XtAddCallback(butt, XmNvalueChangedCallback,
                  (XtCallbackProc)ToggleCoordSystemCallback, sf);

    XtCreateManagedWidget("New epoch (JXXXX.X/BYYYY.Y):", xmLabelWidgetClass,
                          rc1, NULL, 0);
    
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc1, NULL, 0);
    
    ArrangeStdFormDialog(sf, fr);
    
    XtManageChild(fr);
    
    wprintf(sf->edit[0], "%1c%6.1f", vP->s->epoch, vP->s->equinox);
    
    ManageDialogCenteredOnPointer(sf->form);
}
