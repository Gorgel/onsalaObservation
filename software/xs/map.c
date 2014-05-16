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
#include <ctype.h>

#include <Xm/Scale.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/Frame.h>
#if XmVersion >= 2000                            
#include <Xm/Notebook.h>
#include <Xm/SpinB.h>
#endif

#include "defines.h"
#include "global_structs.h"
#include "menus.h"

#ifdef HAVE_LIBPGPLOT
#include "cpgplot.h"
#define STRIPING 0.00105
#endif

#include "map.h"
#include "dialogs.h"

void init_map_parameters()
{
    int i;
    
    void init_vp_parameters();
    
    cont.grey = SHADE_NONE;
    cont.grey_inverted = 0;
    cont.grey_res = 0;
    
    cont.markers = atoi(pP->contMarker);
    cont.dot_size = atoi(pP->contMarkerSize);
    
    cont.zmin = cont.zmax = 0.0;
    cont.minmax = CONT_MINMAX;
    set_contour_levels(0.0, 10.0, 7, CONT_LINEAR, 1.0);
    cont.quick = 1;
    cont.relative = 0;
    cont.ndigits = 3;
    cont.pexp  = 1;
    
    cont.intpType = 0;
    cont.intpOrder = 0;
    cont.nCorners = 0;
    
    cont.blank = 0;
    
    Map.saved = 1;
    zType = ZTYPE_MOMENT;
    cType = COORD_TYPE_EQU;
    rType = 0;
    pType = 0;
    fType = 0;
    forcePosAngle = 0;
    posAngle = 0.0;
    xStep = 0.0;
    yStep = 0.0;
    xCentre = 0.0;
    yCentre = 0.0;
    zeroSpacing = ZEROSPACING;
    
    for (i=0; i<5; i++) slider[i] = NULL;
    
    init_vp_parameters();
}

double GetCurrentMapMemory()
{
    return (double)map_bytes/1024.0/1024.0;
}

void SetTinyWindow(int size)
{
    tinyWindow = size;
}

MAP *GetPosPosMap()
{
    return &Map;
}

static void StoreContourState()
{
    tmp_cont = cont;
}

static void RestoreContourState()
{
    cont = tmp_cont;
}

static void BeginOnlyContour()
{
    StoreContourState();
    
    cont.grey = 0;
    if (!cont.quick) cont.quick = 2;
}

static void EndOnlyContour()
{
    RestoreContourState();
}

void set_show_mode(Widget w, char *cmd, XtPointer call_data)
{
    int n;
    string buf;
    /* scanPtr first = (scanPtr)DATA(vP->from->scanlist); */

    void obtain_map_info(), draw_main(), send_line();
    void SetDefWindow(), SetStdView();
    /* void MakeMapScatterPlot(); */
    int SetVelPosShowMode();

    if (strcmp(cmd, "single")==0) {
        SetViewMode(SHOW_SPE, vP->s, vP->m, vP->p);
    } else if (strcmp(cmd, "map")==0) {
        n = count_scans(vP->from);
        if (n < 2) {
            sprintf(buf, "No map data available.");
            PostErrorDialog(w, buf);
            return;
        } else {
            if (n > 250 && tinyWindow == 0) {
                sprintf(buf, "The map consists of %d spectra.\n\
This may take time. Do you want to continue?", n);
                if (!PostQuestionDialog(w, buf))
                    return;
            }
            SetViewMode(SHOW_ALLSPE, vP->s, &Map, vP->p);
            obtain_map_info(NULL, "map", NULL);
        }
    } else if (strcmp(cmd, "contour")==0) {
        if (count_scans(vP->from) < 2) {
            sprintf(buf, "No map data available.");
            PostErrorDialog(w, buf);
            return;
        } else {
            SetViewMode(SHOW_POSPOS, vP->s, &Map, vP->p);
            vP->nMaps = 0;
            vP->M = NULL;
            obtain_map_info(NULL, cmd, NULL);
        }
    } else if (strcmp(cmd, "scatter")==0) {
        if (count_scans(vP->from) < 2) {
/*             sprintf(buf, "No scatter data available.");
            PostErrorDialog(w, buf);
            return; */
            SetViewMode(SHOW_SCATTER, vP->s, vP->m, vP->p);
        } else {
            SetViewMode(SHOW_SCATTER, vP->s, vP->m, vP->p);
            /* obtain_map_info(NULL, cmd, NULL);
            MakeMapScatterPlot(vP->m, NULL); */
        }
    } else if (strcmp(cmd, "velpos")==0) {
        if (SetVelPosShowMode()) return;
        vP->nMaps = 0;
        vP->M = NULL;
    }
    SetStdView();
    SetDefWindow(SCALE_BOTH);
    draw_main();
}

static char *GetMapInfo(MAP *m)
{
    static string s;
    string buf;
    
    sprintf(s, "%s\n", m->name);
    
    if (m->coordType == COORD_TYPE_EQU) {
        sprintf(buf, "RA offset range: %f to %f\n", m->xright, m->xleft);
        strcat(s, buf);
        sprintf(buf, "Dec. offset range: %f to %f\n", m->ylower, m->yupper);
        strcat(s, buf);
    } else if (m->coordType == COORD_TYPE_GAL) {
        sprintf(buf, "Gal. long. offset range: %f to %f\n", m->xleft, m->xright);
        strcat(s, buf);
        sprintf(buf, "Gal. lat. offset range: %f to %f\n", m->ylower, m->yupper);
        strcat(s, buf);
    } else {
        sprintf(buf, "Az. offset range: %f to %f\n", m->xleft, m->xright);
        strcat(s, buf);
        sprintf(buf, "El. offset range: %f to %f\n", m->ylower, m->yupper);
        strcat(s, buf);
    }
    
    sprintf(buf, "Spacing: %f and %f", m->xspacing, m->yspacing);
    strcat(s, buf);
    
    return s;
}

static void SetOffsetsUsingMapGrid(Widget w, char *cmd, XtPointer call_data)
{
    int n=0, i, j;
    DataSetPtr dsp;
    scanPtr s = NULL;
    list curr = NULL;
    string buf;
    
    void send_line();
    DataSetPtr new_dataset(list *, char *, DataSetPtr);
    list *get_listlist();
    void DeleteLastDataSet();
    
    dsp = new_dataset(get_listlist(), "Offsets set", vP->from);
    if (!dsp) {
        PostErrorDialog(w, "Out of memory when allocating dataset.");
        return;
    }
    
    if (vP->m == &Map) {
        while ( (curr = scan_iterator(curr, vP->from)) ) {
            s = copy_scan(dsp, (scanPtr)DATA(curr));
            if (!s) break;
            i = NINT((xmap(s) - Map.xleft)/Map.xspacing);
            j = NINT((ymap(s) - Map.ylower)/Map.yspacing);
            if (cType == COORD_TYPE_EQU || cType == COORD_TYPE_GAL) {
                s->xoffset = Map.xleft + (double)i * Map.xspacing;
                s->yoffset = Map.ylower + (double)j * Map.yspacing;
            } else if (cType == COORD_TYPE_HOR) {
                s->aoffset = Map.xleft + (double)i * Map.xspacing;
                s->eoffset = Map.ylower + (double)j * Map.yspacing;
            }
            /* i = NINT((xmap(s) - xCentre)/Map.xspacing);
            j = NINT((ymap(s) - yCentre)/Map.yspacing);
            if (cType == COORD_TYPE_EQU || cType == COORD_TYPE_GAL) {
                s->xoffset = xCentre + (double)i * Map.xspacing;
                s->yoffset = yCentre + (double)j * Map.yspacing;
            } else if (cType == COORD_TYPE_HOR) {
                s->aoffset = xCentre + (double)i * Map.xspacing;
                s->eoffset = yCentre + (double)j * Map.yspacing;
            } */
            n++;
        }
        sprintf(buf, "Offsets set in %d spectra.", n);
        send_line(buf);
    } else {
        PostWarningDialog(w, "No current map grid.");
        DeleteLastDataSet();
        return;
    }
    if (!s) {
        if (n > 0) {
            PostErrorDialog(w, "Out of memory when allocating scan\n\
Resulting dataset probably incomplete.");
        } else {
            PostErrorDialog(w, "Out of memory when allocating scan\n\
No new data.");
            DeleteLastDataSet();
            return;
        }
    } else {
        sprintf(dsp->name, "%s offsets set (%d)", s->name, n);
    }
    vP->s = (scanPtr)DATA(dsp->scanlist);
    vP->to = vP->from = dsp;
    UpdateData(SCALE_BOTH, REDRAW);
}

void obtain_map_info(Widget w, char *cmd, XtPointer call_data)
{
    int i, j, m, nX, nY, force_centre=0;
    static int alloc_i_size=0, alloc_j_size=0;
    string buf;
    char buf2[512];
    double x, y, v1, v2, tmp;
    double min_xspa=HIGHSPACING, min_yspa=HIGHSPACING;
    double min_x, max_x, min_y, max_y;
    scanPtr s = NULL;
    list curr = NULL, next;

    int CheckCoordType();
    void send_line(), update_map_data();
    void FreeIntArray(), FreeDoubleArray(), FreeScanPtrArray();
    int GetBox();
    double SpecUnitConv();
    int **AllocIntArray();
    double **AllocDoubleArray();
    scanPtr **AllocScanPtrArray();

    if (count_scans(vP->from) < 2) {
        if (!vP->m) {
            PostWarningDialog(w, "There is no map data.");
        } else if (strcmp(cmd, "info")==0) {
            PostMessageDialog(w, GetMapInfo(vP->m));
        }
        return;
    }
    
    if (!vP->s) return;
    
    if (CheckCoordType()) {
        PostWarningDialog(w,
           "Scans have mixed coordinates types (eg. equatorial, galactic).");
        return;
    }

    if (strcmp(cmd, "velpos") == 0) {
        update_map_data();
        return;
    }
    
    if (strcmp(cmd, "map-centre") == 0) force_centre=1;
    
    SetWatchCursor(True);
    
    min_x = max_x = xmap(vP->s);
    min_y = max_y = ymap(vP->s);
    while ( (curr = scan_iterator(curr, vP->from)) ) {
        s = (scanPtr)DATA(curr);
        x = xmap(s);
        y = ymap(s);
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
    }
    
    if (!vP->from->gridded) {
        curr = NULL;
        while ( (curr = scan_iterator(curr, vP->from)) ) {
            s = (scanPtr)DATA(curr);
            next = curr;
            while ( (next = scan_iterator(next, vP->from)) ) {
                x = fabs(xmap(s) - xmap((scanPtr)DATA(next)));
                y = fabs(ymap(s) - ymap((scanPtr)DATA(next)));
                if (x < min_xspa && x >= zeroSpacing) min_xspa = x;
                if (y < min_yspa && y >= zeroSpacing) min_yspa = y;
            }
        }
        if (xStep > 0.0) min_xspa = xStep;
        if (yStep > 0.0) min_yspa = yStep;
        if (min_xspa == HIGHSPACING && min_yspa == HIGHSPACING) {
            PostWarningDialog(w, "Couldn't resolve map spacing.");
            SetWatchCursor(False);
            return;
        } else if (min_xspa == HIGHSPACING) {
            min_xspa = min_yspa;
        } else if (min_yspa == HIGHSPACING) {
            min_yspa = min_xspa;
        }
        vP->from->gridded = 1;
        vP->from->dx = min_xspa;
        vP->from->dy = min_yspa;
    } else {
        min_xspa = fabs(vP->from->dx);
        min_yspa = fabs(vP->from->dy);
    }

    if (strncmp(cmd, "info", 4) == 0) {
        strcpy(buf2, vP->s->name);
        strcat(buf2, "\n");
        
        sprintf(buf, "No. of map spectra: %d", count_scans(vP->from));
        strcat(buf2, buf);
        strcat(buf2, "\n");
        send_line(buf);
        
        if (cType == COORD_TYPE_EQU) {
            sprintf(buf, "RA offset range: %.1f to %.1f", max_x, min_x);
            send_line(buf);
            strcat(buf2, buf);
            strcat(buf2, "\n");

            sprintf(buf, "Dec. offset range: %.1f to %.1f", min_y, max_y);
            send_line(buf);
            strcat(buf2, buf);
            strcat(buf2, "\n");
        } else if (cType == COORD_TYPE_GAL) {
            sprintf(buf, "Gal. long. offset range: %.1f to %.1f", min_x, max_x);
            send_line(buf);
            strcat(buf2, buf);
            strcat(buf2, "\n");

            sprintf(buf, "Gal. lat. offset range: %.1f to %.1f", min_y, max_y);
            send_line(buf);
            strcat(buf2, buf);
            strcat(buf2, "\n");
        } else {
            sprintf(buf, "Az. offset range: %.1f to %.1f", min_x, max_x);
            send_line(buf);
            strcat(buf2, buf);
            strcat(buf2, "\n");

            sprintf(buf, "El. offset range: %.1f to %.1f", min_y, max_y);
            send_line(buf);
            strcat(buf2, buf);
            strcat(buf2, "\n");
        }
        
        sprintf(buf, "Spacing: %.2f and %.2f", min_xspa, min_yspa);
        send_line(buf);
        strcat(buf2, buf);
        
        PostMessageDialog(w, buf2);
        SetWatchCursor(False);
        return;
    }

    strcpy(Map.name, vP->s->name);
    strcpy(Map.molecule, vP->s->molecule);
    Map.epoch = vP->s->epoch;
    Map.equinox = vP->s->equinox;
    Map.x0 = vP->s->x0;
    Map.y0 = vP->s->y0;
    Map.date = vP->s->date;
    Map.type = MAP_POSPOS;
    if (vP->from->sequence)
        Map.sequence = 1;
    else
        Map.sequence = 0;
    Map.ndata = count_scans(vP->from);
    Map.coordType = cType;
    Map.posAngle = vP->from->posAngle;
    if (cType == COORD_TYPE_HOR) {
        if (xCentre != 0.0 || force_centre) {
            i = NINT((min_x - xCentre)/min_xspa);
            Map.xleft = xCentre + (double)i * min_xspa;
            i = NINT((max_x - xCentre)/min_xspa);
            Map.xright = xCentre + (double)i * min_xspa;
            Map.xspacing = min_xspa;
        } else {
            Map.xspacing = min_xspa;
            Map.xleft = min_x;
            Map.xright = max_x;
        }
    } else {
        if (xCentre != 0.0 || force_centre) {
            i = NINT((xCentre - max_x)/min_xspa);
            Map.xleft = xCentre - (double)i * min_xspa;
            i = NINT((xCentre - min_x)/min_xspa);
            Map.xright = xCentre - (double)i * min_xspa;
            Map.xspacing = -min_xspa;
        } else {
            Map.xspacing = -min_xspa;
            Map.xleft = max_x;
            Map.xright = min_x;
        }
    }
    if (yCentre != 0.0 || force_centre) {
        j = NINT((min_y - yCentre)/min_yspa);
        Map.ylower = yCentre + (double)j * min_yspa;
        j = NINT((max_y - yCentre)/min_yspa);
        Map.yupper = yCentre + (double)j * min_yspa;
        Map.yspacing = min_yspa;
    } else {
        Map.yspacing = min_yspa;
        Map.yupper = max_y;
        Map.ylower = min_y;
    }

    Map.i_min = NINT((Map.xleft - xCentre)/Map.xspacing);
    Map.i_max = NINT((Map.xright - xCentre)/Map.xspacing);
    Map.j_min = NINT((Map.ylower - yCentre)/Map.yspacing);
    Map.j_max = NINT((Map.yupper - yCentre)/Map.yspacing);
    Map.i_no = Map.i_max - Map.i_min + 1;
    Map.j_no = Map.j_max - Map.j_min + 1;
    Map.swapped = 0;
    Map.saved = 0;
    Map.original = NULL;
    if (vP->s->b.maj > 0.0) {
        Map.b = vP->s->b;
    } else {
        Map.b.maj = Map.b.min = Map.b.PA = 0.0;
    }

    nX = Map.i_no;
    nY = Map.j_no;
#ifdef DEBUG
    printf("No. of map spectra: %dx%d=%d\n", nX, nY, count_scans(vP->from));
    printf("RA range: %.2f to %.2f\n", Map.xleft, Map.xright);
    printf("Dec. range: %.2f to %.2f\n", Map.ylower, Map.yupper);
    printf("Spacing: %.2f and %.2f\n", Map.xspacing, Map.yspacing);
    printf("i_min=%d i_max=%d\n", Map.i_min, Map.i_max);
    printf("j_min=%d j_max=%d\n", Map.j_min, Map.j_max);
#endif

    if (alloc_i_size != nX || alloc_j_size != nY) {
        if (alloc_i_size > 0 || alloc_j_size > 0) {
            if (gm)    FreeIntArray(gm, alloc_i_size, alloc_j_size);
            if (ddata) FreeDoubleArray(ddata, alloc_i_size, alloc_j_size);
            if (edata) FreeDoubleArray(edata, alloc_i_size, alloc_j_size);
            if (sdata) FreeScanPtrArray(sdata, alloc_i_size, alloc_j_size);
            alloc_i_size = 0;
            alloc_j_size = 0;
            ddata = NULL;
            edata = NULL;
	    sdata = NULL;
        }

        gm = AllocIntArray(nX, nY);
        if (!gm) {
            send_line("obtain_map_info: malloc failure for gm[][].");
            sprintf(buf, "Out of memory in map arrays (nX=%d, nY=%d).", nX, nY);
            PostErrorDialog(w, buf);
            return;
        }
        map_bytes = nX * nY * sizeof(int);
        ddata = AllocDoubleArray(nX, nY);
        if (!ddata) {
            send_line("obtain_map_info: malloc failure for ddata[][].");
            sprintf(buf, "Out of memory in map arrays (nX=%d, nY=%d).", nX, nY);
            PostErrorDialog(w, buf);
            return;
        }
        map_bytes += nX * nY * sizeof(double);
        edata = AllocDoubleArray(nX, nY);
        if (!edata) {
            send_line("obtain_map_info: malloc failure for edata[][].");
            sprintf(buf, "Out of memory in map arrays (nX=%d, nY=%d).", nX, nY);
            PostErrorDialog(w, buf);
            return;
        }
        map_bytes += nX * nY * sizeof(double);
        sdata = AllocScanPtrArray(nX, nY);
        if (!sdata) {
            send_line("obtain_map_info: malloc failure for sdata[][].");
            sprintf(buf, "Out of memory in map arrays (nX=%d, nY=%d).", nX, nY);
            PostErrorDialog(w, buf);
            return;
        }
        map_bytes += nX * nY * sizeof(scanPtr);
        alloc_i_size = nX;
        alloc_j_size = nY;
    }
  
#ifdef DEBUG
    printf("nX=%d (%d)   nY=%d (%d)\n", nX, alloc_i_size, nY, alloc_j_size);
#endif

    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            gm[i][j] = BLANK;
            ddata[i][j] = UNDEF;
            edata[i][j] = UNDEF;
            sdata[i][j] = NULL;
        }
    }

    m = 0;
    curr = NULL;
    while ( (curr = scan_iterator(curr, vP->from)) ) {
        s = (scanPtr)DATA(curr);
        i = NINT((xmap(s)-xCentre)/Map.xspacing) - Map.i_min;
        j = NINT((ymap(s)-yCentre)/Map.yspacing) - Map.j_min;
        if (gm[i][j] != BLANK) {
            if (m < 10) {
                sprintf(buf,
                    "There are doublets at position %3d,%3d (%f,%f).",
                    i, j, xmap(s), ymap(s));
                send_line(buf);
            }
            m++;
            s->doublet = 1;
        } else
            s->doublet = 0;
        s->i = i;
        s->j = j;
        gm[i][j] = 0;
        sdata[i][j] = s;
    }

    if (m > 0) {
        sprintf(buf, "In total there are %d duplicate positions.", m);
        PostMessageDialog(w, buf);
        send_line(buf);
    }

    Map.d  = ddata;
    Map.e  = edata;
    Map.sp = sdata;
    Map.f  = gm;
    
    if (!GetBox(BOX_MOM, 0, &i, &j)) {
        v1 = SpecUnitConv(UNIT_VEL, UNIT_CHA, (double)i);
        v2 = SpecUnitConv(UNIT_VEL, UNIT_CHA, (double)j);
        if (v1 > v2) {tmp = v1; v1 = v2; v2 = tmp;}
    } else {
        v1 = v2 = 0.0;
    }

    Map.v  = (v1 + v2)/2.0;
    Map.dv =  v2 - v1;

    SetWatchCursor(False);
    
    if (strcmp(cmd, "no_update_map_data") != 0)
        update_map_data();
}

void update_map_data()
{
    int i, j, first;
    double x, z, tmin, tmax, xmin, xmax, zmin=0.0, zmax=0.0;
    double d, dmin, dmax;
    string buf;
    scanPtr s;
    list curr = NULL;

    void update_bl_data(), update_mom_data();
    void MakeMapScatterPlot();
    double chan2xunit();

    if (count_scans(vP->from) < 2 || !vP->s || !vP->m) return;
    
    if (!gm || !ddata || !edata || !sdata) {
        sprintf(buf, "Out of memory in map arrays.");
        PostErrorDialog(NULL, buf);
        return;
    }

    s = vP->s;

    tmin = tmax = s->d[0];
    xmin = xmax = chan2xunit(0);
    dmin = dmax = sqrt(s->xoffset * s->xoffset + s->yoffset * s->yoffset);
    while ( (curr = scan_iterator(curr, vP->from)) ) {
        s = (scanPtr)DATA(curr);
        if (s->s_max > tmax) tmax = s->s_max;
        if (s->s_min < tmin) tmin = s->s_min;
        x = chan2xunit(0);
        if (x > xmax) xmax = x;
        if (x < xmin) xmin = x;
        x = chan2xunit(s->nChan - 1);
        if (x > xmax) xmax = x;
        if (x < xmin) xmin = x;
        d = sqrt(s->xoffset * s->xoffset + s->yoffset * s->yoffset);
        if (d < dmin) dmin = d;
        if (d > dmax) dmax = d;
    }

    vP->m->unit   = vP->xunit;
    vP->m->gx_min = xmin;
    vP->m->gx_max = xmax;
    vP->m->gt_min = tmin - 0.1*(tmax - tmin);
    vP->m->gt_max = tmax + 0.1*(tmax - tmin);
    vP->m->d_min  = dmin;
    vP->m->d_max  = dmax;
    vP->m->zType  = zType;

    curr = NULL;
    while ( (curr = scan_iterator(curr, vP->from)) ) {
        s = (scanPtr)DATA(curr);
        update_bl_data(s);
        update_mom_data(s);
        i = s->i;
        j = s->j;
        z = zmap(s);
        if (z != UNDEF) {
            gm[i][j] = 0;
            ddata[i][j] = z;
            edata[i][j] = emap(s);
            sdata[i][j] = s;
        } else {
            gm[i][j] = BLANK;
            ddata[i][j] = UNDEF;
            edata[i][j] = UNDEF;
            sdata[i][j] = NULL;
        }
    }
    
    vP->m->v  = vP->s->mom.v;
    vP->m->dv = vP->s->mom.dv;

    if (vP->mode == SHOW_POSPOS || vP->mode == SHOW_ALLSPE ||
        vP->mode == SHOW_SCATTER) {
        first = 1;
        curr = NULL;
        while ( (curr = scan_iterator(curr, vP->from)) ) {
            s = (scanPtr)DATA(curr);
            z = zmap(s);
            if (z != UNDEF) {
                if (first) {
                    zmin = zmax = z;
                    first = 0;
                } else {
                    if (z > zmax) zmax = z;
                    if (z < zmin) zmin = z;
                }
            }
        }
        vP->m->z_min = zmin;
        vP->m->z_max = zmax;
        cont.zmin = zmin;
        cont.zmax = zmax;
        set_contour_levels(zmin, zmax, cont.nc, cont.spacing, 1.0);
        if (vP->mode == SHOW_SCATTER) {
            MakeMapScatterPlot(vP->m, NULL);
        }
    } else if (vP->mode == SHOW_VELPOS || vP->mode == SHOW_POSVEL) {
        vP->m->z_min = tmin;
        vP->m->z_max = tmax;
        cont.zmin = tmin;
        cont.zmax = tmax;
        set_contour_levels(tmin, tmax, cont.nc, cont.spacing, 1.0);
    }
}

void ZeroScanInAllMaps(scanPtr s)
{
    int i, j;
    
    void ZeroScanInMapList(scanPtr);
    
    if (sdata) {
        for (i=0; i<Map.i_no; i++) {
            for (j=0; j<Map.j_no; j++) {
                if (Map.sp[i][j] == s) Map.sp[i][j] = NULL;
            }
        }
    }
    ZeroScanInMapList(s);
}

void set_map_minmax(MAP *m)
{
    int i, j, nX, nY, first=1;
    double x, y, d, dmin=0.0, dmax=0.0, zmin=0.0, zmax=0.0;

    if (!m)
        return;

    nX = m->i_no;
    nY = m->j_no;

    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (m->f[i][j] <= BLANK) continue;
            x = m->xleft  + (double)i * m->xspacing;
            y = m->ylower + (double)j * m->yspacing;
            d = sqrt(x*x + y*y);
            if (first) {
                zmin = zmax = m->d[i][j];
                dmin = dmax = d;
                first = 0;
            } else {
                if (m->d[i][j] < zmin) zmin = m->d[i][j];
                if (m->d[i][j] > zmax) zmax = m->d[i][j];
                if (d < dmin) dmin = d;
                if (d > dmax) dmax = d;
            }
        }
    }
    m->z_min = zmin;
    m->z_max = zmax;
    m->d_min = dmin;
    m->d_max = dmax;
#ifdef DEBUG
    printf("Map '%s':  zmin=%f  zmax=%f\n", m->name, zmin, zmax);
#endif
    cont.zmin = zmin;
    cont.zmax = zmax;
    set_contour_levels(zmin, zmax, cont.nc, cont.spacing, 1.0);
}

static void draw_spec_map()
{
    int nfound=0, xsize, tiny=0;
#ifdef HAVE_LIBPGPLOT
    PLFLT f_x1, f_x2, f_y1, f_y2, f_xm, f_ym, f_dx, f_dy;
    PLFLT fx_l, fx_r, fy_b, fy_t;
    double x2, y2, x0=0.0, y0=0.0, dx=0.0, dy=0.0;
#endif
    double x1=0.0, y1=0.0;
    VIEW *mview = NULL, *sv;
    list curr = NULL;
    scanPtr s, sTR=NULL, first = vP->s;

    void draw_frame(), draw_ticks(), draw_spectrum(), draw_boxes();
    void draw_markers(), draw_secondary(), EndSubView();
    void SetWindow(), SetDefWindow(), draw_gauss(), draw_poly();
    void DrawRelLabel();
    char *GetLeftLabel();
    char *GetRightLabel();
    VIEW *BeginSubView(), *GetScanView();
    int OutsideView(), SetSubView();
    void UpdateHeaderInfo();

    sv = GetScanView();

    if (vP->special_view_type != VIEW_NONE) {
        while ( (curr = scan_iterator(curr, vP->from)) ) {
            s = (scanPtr)DATA(curr);
            if (OutsideView(xmap(s), ymap(s), vP)) continue;
	    if (!sTR) {
                x1 = xmap(s);
                y1 = ymap(s);
                sTR = s;
	    } else if (vP->special_view_type == VIEW_BOTTOM_RIGHT) {
                if (ymap(s) <= y1 && xmap(s) <= x1) {
                    x1 = xmap(s);
                    y1 = ymap(s);
                    sTR = s;
                }
            } else if (vP->special_view_type == VIEW_BOTTOM_LEFT) {
                if (ymap(s) <= y1 && xmap(s) >= x1) {
                    x1 = xmap(s);
                    y1 = ymap(s);
                    sTR = s;
                }
            } else if (vP->special_view_type == VIEW_TOP_LEFT) {
                if (ymap(s) >= y1 && xmap(s) >= x1) {
                    x1 = xmap(s);
                    y1 = ymap(s);
                    sTR = s;
                }
            } else {
                if (ymap(s) >= y1 && xmap(s) <= x1) {
                    x1 = xmap(s);
                    y1 = ymap(s);
                    sTR = s;
                }
            }
            nfound++;
        }
    }

#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        x0 = vP->xleft  + vP->xspacing/2.0;
        y0 = vP->ylower + vP->yspacing/2.0;
        dx = vP->xright - vP->xleft;
        dy = vP->yupper - vP->ylower;
        cpgqvp(0, &fx_l, &fx_r, &fy_b, &fy_t);
    }
#endif

    mview = BeginSubView(0, 0, vP->m->i_no, vP->m->j_no,
                         vP->subXmarg, vP->subYmarg);
    curr = NULL;
    while ( (curr = scan_iterator(curr, vP->from)) ) {
        s = (scanPtr)DATA(curr);
        if (OutsideView(xmap(s), ymap(s), mview)) continue;
        SetViewMode(SHOW_SUBSPE, s, mview->m, vP->p);
        xsize = SetSubView(s->i, s->j);
        if (xsize < tinyWindow)
            tiny = 1;
        else
            tiny = 0;
        if (vP->use_attached_frame && s->frame.use) {
          SetWindow(s->frame.x1, s->frame.x2, s->frame.y1, s->frame.y2);
        } else {
          SetWindow(sv->xleft, sv->xright, sv->ylower, sv->yupper);
          if (vP->autoscale_x && !vP->autoscale_y)
              SetDefWindow(SCALE_ONLY_X);
          else if (!vP->autoscale_x && vP->autoscale_y)
              SetDefWindow(SCALE_ONLY_Y);
          else if (vP->autoscale_x && vP->autoscale_y)
              SetDefWindow(SCALE_BOTH);
        }
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            x1 = (xmap(s) - x0)/dx;
            x2 = x1 + mview->m->xspacing/dx;
            y1 = (ymap(s) - y0)/dy;
            y2 = y1 + mview->m->yspacing/dy;
            f_x1 = fx_l + (PLFLT)x1 * (fx_r - fx_l);
            f_x2 = fx_l + (PLFLT)x2 * (fx_r - fx_l);
            f_y1 = fy_b + (PLFLT)y1 * (fy_t - fy_b);
            f_y2 = fy_b + (PLFLT)y2 * (fy_t - fy_b);
            if (mview->subXmarg != 0) {
                f_dx = f_x2 - f_x1;
                f_x1 += (PLFLT)mview->subXmarg/100.0 * f_dx;
                f_x2 -= (PLFLT)mview->subXmarg/100.0 * f_dx;
            }
            if (mview->subXmagn > 0.0) {
                f_xm = (f_x2 + f_x1)/2.0;
                f_dx = (f_x2 - f_x1)/2.0;
                f_x1 = f_xm - (PLFLT)mview->subXmagn * f_dx;
                f_x2 = f_xm + (PLFLT)mview->subXmagn * f_dx;
            }
            if (mview->subYmarg != 0) {
                f_dy = f_y2 - f_y1;
                f_y1 += (PLFLT)mview->subYmarg/100.0 * f_dy;
                f_y2 -= (PLFLT)mview->subYmarg/100.0 * f_dy;
            }
            if (mview->subYmagn > 0.0) {
                f_ym = (f_y2 + f_y1)/2.0;
                f_dy = (f_y2 - f_y1)/2.0;
                f_y1 = f_ym - (PLFLT)mview->subYmagn * f_dy;
                f_y2 = f_ym + (PLFLT)mview->subYmagn * f_dy;
            }
#ifdef DEEPDEBUG
            printf("n=%3d  x1=%f x2=%f  y1=%f y2=%f\n", n,
                   x1, x2, y1, y2);
            printf("x=(%f, %f)    y=(%f,%f)\n",
                   (double)f_x1, (double)f_x2, (double)f_y1, (double)f_y2);
#endif
            cpgsvp(f_x1, f_x2, f_y1, f_y2);
        }
#endif
        if (s != first)
            draw_frame(gp->gcFrame[2], 1, s == sTR ? 1 : 0);
        else
            draw_frame(gp->gcMom, 1, s == sTR ? 1 : 0);
        if (s == sTR) {
            if (nfound < 10)
                draw_ticks(gp->gcFrame[1], "Tt", "Rr");
            else
                draw_ticks(gp->gcFrame[1], "t", "r");
        }
        if (tiny) continue;
        if (!s->doublet)
            draw_spectrum(gp->gcLine, gp->gcRms);
        else
            draw_secondary(s, gp->gcSec);
        if (vP->llab_type /* && sv->s->sequence */)
            DrawRelLabel(gp->gcFrame[3], vP->lef_x, vP->lef_y, 0.0,
                         GetLeftLabel());
        if (vP->rlab_type /* && sv->s->sequence */)
            DrawRelLabel(gp->gcFrame[3], vP->rig_x, vP->rig_y, 0.0,
                         GetRightLabel());
        if (draw.boxes)   draw_boxes(gp->gcBox, gp->gcMom);
        if (draw.markers) draw_markers(gp->gcFrame[1], gp->gcGauss, gp->gcTag);
        if (draw.poly)    draw_poly(gp->gcPoly, s);
        if (draw.gsum && s->gaussFit)
                          draw_gauss(gp->gcGaussI, s->g);
    }
    EndSubView();
    UpdateHeaderInfo();
}

static void draw_pos_markers(GC gc, MAP *m)
{
    int i, j;
    Point r, p;
#ifdef HAVE_LIBPGPLOT
    PSSTY tmp_style = ps.marker;
#endif
    Display *dpy = XtDisplay(gp->graph);
    COLOR *c;

    COLOR *GetColorInfo();
    void DrawAbsLabel(), DrawContourDot();
    char *get_numeric_label();
    unsigned long GetContourPixel(double);
    int GetPGci(double);

    if (cont.markers == DOT_NOMARK || !m) return;
    
    c = GetColorInfo();
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        tmp_style = ps.marker;
        ps.marker = ps.posmarker;
        cpgbbuf();
    }
#endif

    for (i=0; i<m->i_no; i++) {
        for (j=0; j<m->j_no; j++) {
            if (m->f[i][j] <= BLANK) continue;
            r.x = m->xleft  + (double)(i) * m->xspacing;
            r.y = m->ylower + (double)(j) * m->yspacing;
            if (cont.grey) {
                XSetForeground(dpy, gc, GetContourPixel(m->d[i][j]));
#ifdef HAVE_LIBPGPLOT
                if (pgplot) ps.marker.ci = GetPGci(m->d[i][j]);
#endif
            }
            if (rType && m->type == MAP_POSPOS) {
                uv2xy(r, forcePosAngle ? posAngle : m->posAngle, &p);
                r = p;
            } else if (pType && m->type == MAP_POSPOS) {
                pt2xy(r.x, r.y, pType, &p);
                r = p;
            }
            if (cont.markers == DOT_VALUEMARK) {
                DrawAbsLabel(gc, r.x, r.y, 0.5,
                             get_numeric_label(m->d[i][j], 0.01, 4));
            } else if (cont.markers == DOT_SQUAREMARK) {
                DrawContourDot(gc, 's', (double)cont.dot_size, r.x, r.y);
            } else if (cont.markers == DOT_CIRCLEMARK) {
                DrawContourDot(gc, 'c', 2.0 * (double)cont.dot_size, r.x, r.y);
            } else if (cont.markers == DOT_CROSSMARK) {
                DrawContourDot(gc, 'x', (double)cont.dot_size, r.x, r.y);
            } else if (cont.markers == DOT_PLUSMARK) {
                DrawContourDot(gc, '+', (double)cont.dot_size, r.x, r.y);
            }
        }
    }
    
    if (cont.grey) XSetForeground(dpy, gc, c->black);
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        cpgebuf();
        ps.marker = tmp_style;
    }
#endif
}

static void FindNxNy(int n, int *nX, int *nY)
{
    int i;
    
    if (!nY) return;
    
    if (!nX) {
        if (n <= 0 || vP->Nx <= 0) return;
        *nY = 1 + (n-1)/vP->Nx;
        return;
    }
    
    *nX = *nY = 0;
    
    if (n <= 0) return;
    
    if (n == 1) {
        *nX = *nY = 1;
        return;
    }
    
    if (n == 2) {
        *nX = 2;
        *nY = 1;
        return;
    }
    
    i = (int)floor(sqrt((double)n));
    
    if (i*i >= n) {
        *nX = *nY = i;
    } else if ((i+1)*i >= n) {
        *nX = i+1;
        *nY = i;
    } else {
        *nX = i+1;
        *nY = i+1;
    }
}

static void find_special(int nX, int nY, int nMax, int *is, int *js)
{
    int i, j, n=0;
    int x=nX/2, y=nY/2;
    
    if (nMax <=1) {
        if (is) *is = -1;
        if (js) *js = -1;
        return;
    }
    
    for (j=nY-1; j>=0; j--) {
        for (i=0; i<nX; i++) {
            if (n >= nMax) break;
            if (vP->special_view_type == VIEW_BOTTOM_RIGHT) {
               if (i >= x && j <= y) {
                   x = i; y = j;
               }
            } else if (vP->special_view_type == VIEW_BOTTOM_LEFT) {
               if (i <= x && j <= y) {
                   x = i; y = j;
               }
            } else if (vP->special_view_type == VIEW_TOP_LEFT) {
               if (i <= x && j >= y) {
                   x = i; y = j;
               }
            } else {
               if (i >= x && j >= y) {
                   x = i; y = j;
               }
            }
            n++;
        }
    }
    
    if (is) *is = x;
    if (js) *js = y;
}

void draw_map()
{
    int n, i, j, nX, nY, i_special, j_special, special=0;
    GC gc = NULL;
    MAP *m = vP->m;
    scatter *p = vP->p;
#ifdef HAVE_LIBPGPLOT
    PLFLT f_x1, f_x2, f_y1, f_y2, f_dx=0.0, f_dy=0.0, f_xm, f_ym, f_d;
    PLFLT fx_l, fx_r, fy_b, fy_t;
#endif
    PSSTY tmp = ps.line;
    VIEW *v = NULL;

    void draw_ticks(), draw_scatter_plot(), draw_frame(), draw_polylines();
    void draw_all_labels();
    GC   IterateColor();
    int  QueryMEM();
    void set_map_minmax(), set_scatter_minmax();
    VIEW *BeginSubView();
    int SetSubView();
    void EndSubView();
    void SetWindow(), SetDefWindow(), SetStdView();
    void DrawRelLabel();
    char *GetLeftLabel(), *GetRightLabel();

    if (!QueryMEM())
        SetWatchCursor(True);

    if (vP->mode == SHOW_ALLSPE) {
        if (draw.data) draw_spec_map();
    } else if (vP->mode == SHOW_SCATTER) {
        if (vP->nScat > 1 && draw.multiple) {
            if (vP->Nx <= 0) {
                FindNxNy(vP->nScat, &nX, &nY);
            } else {
                FindNxNy(vP->nScat, NULL, &nY);
                nX = vP->Nx;
            }

            SetStdView();
            v = BeginSubView(vP->box_w, vP->box_h, nX, nY,
                             vP->subXmarg, vP->subYmarg);
#ifdef HAVE_LIBPGPLOT
            if (pgplot) {
                cpgqvp(0, &fx_l, &fx_r, &fy_b, &fy_t);
                f_dx = (fx_r - fx_l)/(PLFLT)nX;
                f_dy = (fy_t - fy_b)/(PLFLT)nY;
            }
#endif

            find_special(nX, nY, vP->nScat, &i_special, &j_special);
            
            n = 0;
            for (j=nY-1; j>=0; j--) {
                for (i=0; i<nX; i++) {
                    if (n >= vP->nScat) break;
                    
                    special = (i == i_special && j == j_special);

                    p = vP->P[n];
                    set_scatter_minmax(p);

                    SetViewMode(SHOW_SCATTER, vP->s, vP->m, p);

                    SetSubView(i, j);
                    if (vP->use_attached_frame && p->frame.use) {
                      SetWindow(p->frame.x1, p->frame.x2, p->frame.y1, p->frame.y2);
                    } else {
                        if (vP->autoscale_x && !vP->autoscale_y)
                            SetDefWindow(SCALE_ONLY_X);
                        else if (!vP->autoscale_x && vP->autoscale_y)
                            SetDefWindow(SCALE_ONLY_Y);
                        else if (vP->autoscale_x && vP->autoscale_y)
                            SetDefWindow(SCALE_BOTH);
                    }
/* Store the region (upper left corner and width/height) of this subplot */
                    p->r.x = vP->min_x;
                    p->r.y = vP->min_y - vP->box_h;
                    p->r.width = vP->box_w;
                    p->r.height = vP->box_h;
#ifdef HAVE_LIBPGPLOT
                    if (pgplot) {
                        f_x1 = fx_l + (PLFLT)i * f_dx;
                        f_x2 = f_x1 + f_dx;
                        f_y1 = fy_b + (PLFLT)j * f_dy;
                        f_y2 = f_y1 + f_dy;
                        if (v->subXmarg != 0) {
                            f_x1 += (PLFLT)(v->subXmarg)/100.0 * f_dx;
                            f_x2 -= (PLFLT)(v->subXmarg)/100.0 * f_dx;
                        }
                        if (v->subXmagn > 0.0) {
                            f_xm = (f_x2 + f_x1)/2.0;
                            f_d  = (f_x2 - f_x1)/2.0;
                            f_x1 = f_xm - (PLFLT)v->subXmagn * f_d;
                            f_x2 = f_xm + (PLFLT)v->subXmagn * f_d;
                        }
                        if (v->subYmarg != 0) {
                            f_y1 += (PLFLT)(v->subYmarg)/100.0 * f_dy;
                            f_y2 -= (PLFLT)(v->subYmarg)/100.0 * f_dy;
                        }
                        if (v->subYmagn > 0.0) {
                            f_ym = (f_y2 + f_y1)/2.0;
                            f_d  = (f_y2 - f_y1)/2.0;
                            f_y1 = f_ym - (PLFLT)v->subYmagn * f_d;
                            f_y2 = f_ym + (PLFLT)v->subYmagn * f_d;
                        }
                        cpgsvp(f_x1, f_x2, f_y1, f_y2);
                        cpgswin((PLFLT)vP->xleft, (PLFLT)vP->xright,
                                (PLFLT)vP->ylower, (PLFLT)vP->yupper);
                    }
#endif
                    if (draw.frame) draw_frame(gp->gcFrame[5], 0, special);
                    if (draw.ticks) draw_ticks(gp->gcFrame[2], "Bb", "Ll");
                    if (draw.data)  draw_scatter_plot(gp->gcLine,
                                                      gp->gcGauss, p);
                    if (vP->llab_type /* && sv->s->sequence */)
                        DrawRelLabel(gp->gcFrame[3], vP->lef_x, vP->lef_y,
                                     0.0, GetLeftLabel());
                    if (vP->rlab_type /* && sv->s->sequence */)
                        DrawRelLabel(gp->gcFrame[3], vP->rig_x, vP->rig_y,
                                     0.0, GetRightLabel());
                    n++;
                }
                if (n >= vP->nScat) break;
            }
            EndSubView();
        } else if (vP->nScat > 1 && draw.data) {
            for (n=0; n<vP->nScat; n++) {
                if (n == 0) {
                    tmp = ps.line;
                } else {
                    ps.line = ps.secondary;
                }
                draw_scatter_plot(gp->gcLine, gp->gcGauss, vP->P[n]);
            }
            ps.line = tmp;
        } else if (draw.iterate_color && draw.data) {
            gc = IterateColor();
            draw_scatter_plot(gc, gc, vP->p);
            XFreeGC(XtDisplay(gp->graph), gc);
        } else {
            if (draw.data) draw_scatter_plot(gp->gcLine, gp->gcGauss, vP->p);
        }
    } else if (vP->nMaps > 1) {
        if (vP->Nx <= 0) {
            FindNxNy(vP->nMaps, &nX, &nY);
        } else {
            FindNxNy(vP->nMaps, NULL, &nY);
            nX = vP->Nx;
        }
        
        SetStdView();
        v = BeginSubView(vP->box_w, vP->box_h, nX, nY,
                         vP->subXmarg, vP->subYmarg);
    
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            cpgqvp(0, &fx_l, &fx_r, &fy_b, &fy_t);
            f_dx = (fx_r - fx_l)/(PLFLT)nX;
            f_dy = (fy_t - fy_b)/(PLFLT)nY;
        }
#endif

        find_special(nX, nY, vP->nMaps, &i_special, &j_special);
        
        n = 0;
        for (j=nY-1; j>=0; j--) {
            for (i=0; i<nX; i++) {
                if (n >= vP->nMaps) break;
                
                special = (i == i_special && j == j_special);
                
                m = vP->M[n];
                set_map_minmax(m);
       
                if (m->type == MAP_VELPOS) {
                    SetViewMode(SHOW_VELPOS, vP->s, m, vP->p);
                } else if (m->type == MAP_POSVEL) {
                    SetViewMode(SHOW_POSVEL, vP->s, m, vP->p);
                } else {
                    SetViewMode(SHOW_POSPOS, vP->s, m, vP->p);
                }

                SetSubView(i, j);
                SetDefWindow(SCALE_BOTH);
/* Store the region (upper left corner and width/height) of this subplot */
                m->r.x = vP->min_x;     m->r.y = vP->min_y - vP->box_h;
                m->r.width = vP->box_w; m->r.height = vP->box_h;
#ifdef HAVE_LIBPGPLOT
                if (pgplot) {
                    f_x1 = fx_l + (PLFLT)i * f_dx;
                    f_x2 = f_x1 + f_dx;
                    f_y1 = fy_b + (PLFLT)j * f_dy;
                    f_y2 = f_y1 + f_dy;
                    if (v->subXmarg != 0) {
                        f_x1 += (PLFLT)(v->subXmarg)/100.0 * f_dx;
                        f_x2 -= (PLFLT)(v->subXmarg)/100.0 * f_dx;
                    }
                    if (v->subXmagn > 0.0) {
                        f_xm = (f_x2 + f_x1)/2.0;
                        f_d  = (f_x2 - f_x1)/2.0;
                        f_x1 = f_xm - (PLFLT)v->subXmagn * f_d;
                        f_x2 = f_xm + (PLFLT)v->subXmagn * f_d;
                    }
                    if (v->subYmarg != 0) {
                        f_y1 += (PLFLT)(v->subYmarg)/100.0 * f_dy;
                        f_y2 -= (PLFLT)(v->subYmarg)/100.0 * f_dy;
                    }
                    if (v->subYmagn > 0.0) {
                        f_ym = (f_y2 + f_y1)/2.0;
                        f_d  = (f_y2 - f_y1)/2.0;
                        f_y1 = f_ym - (PLFLT)v->subYmagn * f_d;
                        f_y2 = f_ym + (PLFLT)v->subYmagn * f_d;
                    }
                    cpgsvp(f_x1, f_x2, f_y1, f_y2);
                    cpgwnad((PLFLT)vP->xleft, (PLFLT)vP->xright,
                            (PLFLT)vP->ylower, (PLFLT)vP->yupper);
                 }
#endif
                
                if (draw.data)  draw_contours(gp->gcGrey, gp->gcLine, m);
                if (draw.boxes) draw_polylines(gp->gcMom);
                if (draw.beam)  draw_mapbeam(gp->gcLine, m);
                if (draw.ticks) draw_ticks(gp->gcFrame[2], "Bbts", "Llrs");
                if (draw.frame) draw_frame(gp->gcFrame[5], 0, special);
                if (special && draw.labels && vP->nMaps > 1)
                    draw_all_labels();
                if (vP->mode == SHOW_POSPOS) {
                    draw_pos_markers(gp->gcLine, m);
                }
                if (vP->llab_type /* && sv->s->sequence */)
                    DrawRelLabel(gp->gcFrame[3], vP->lef_x, vP->lef_y, 0.0,
                                 GetLeftLabel());
                if (vP->rlab_type /* && sv->s->sequence */)
                    DrawRelLabel(gp->gcFrame[3], vP->rig_x, vP->rig_y, 0.0,
                                 GetRightLabel());
                
                n++;
            }
            if (n >= vP->nMaps) break;
        }
        EndSubView();
    } else {
        if (vP->nMaps == 1 || vP->nMaps == -2) m = vP->M[0];
        if (draw.data) {
            if (draw.iterate_color) {
                gc = IterateColor();
                draw_contours(gp->gcGrey, gc, m);
                XFreeGC(XtDisplay(gp->graph), gc);
            } else {
                draw_contours(gp->gcGrey, gp->gcLine, m);
            }
        }
        if (draw.projaxes && pType) draw_projaxes(gp->gcLine);
        if (draw.projaxes && pType && draw.projnums) draw_projnums(gp->gcLine);
        if (draw.boxes) draw_polylines(gp->gcMom);
        if (draw.beam)  draw_mapbeam(gp->gcLine, m);
        if (draw.ticks) draw_ticks(gp->gcFrame[2], "Bbts", "Llrs");
        if (draw.frame && cont.grey) draw_frame(gp->gcFrame[5], 0, 0);
        if (vP->mode == SHOW_POSPOS) {
            draw_pos_markers(gp->gcLine, m);
        }
        if (vP->nMaps == -2 && (m = vP->M[1])) {
            tmp = ps.line;
            ps.line = ps.secondary;
            BeginOnlyContour();
            set_map_minmax(m);
            draw_contours(gp->gcGrey, gp->gcSec, m);
            EndOnlyContour();
            ps.line = tmp;
        }
        if (vP->llab_type /* && sv->s->sequence */)
            DrawRelLabel(gp->gcFrame[3], vP->lef_x, vP->lef_y, 0.0,
                         GetLeftLabel());
        if (vP->rlab_type /* && sv->s->sequence */)
            DrawRelLabel(gp->gcFrame[3], vP->rig_x, vP->rig_y, 0.0,
                         GetRightLabel());
    }
    if (!QueryMEM())
        SetWatchCursor(False);
    else
        XFlush(XtDisplay(gp->graph));
}

static scanPtr RADec_to_s(double x, double y)
{
    list curr = NULL;
    scanPtr s, sfound = NULL;

    while ( (curr = scan_iterator(curr, vP->from)) ) {
        s = (scanPtr)DATA(curr);
        if (!s) continue;
        if (fabs(x - xmap(s)) < fabs(vP->m->xspacing/2.0) &&
            fabs(y - ymap(s)) < fabs(vP->m->yspacing/2.0)) {
            sfound = s;
            break;
        }
    }

    return sfound;
}

static scanPtr RADec_to_nearest_s(double x, double y)
{
    double d2, d2min=0.0;
    list curr = NULL;
    scanPtr s, sfound = NULL;

    while ( (curr = scan_iterator(curr, vP->from)) ) {
        s = (scanPtr)DATA(curr);
        d2 = (x-xmap(s))*(x-xmap(s)) + (y-ymap(s))*(y-ymap(s));
        if (!sfound) {
            d2min = d2;
            sfound = s;
        } else {
            if (d2 < d2min) {
                d2min = d2;
                sfound = s;
            }
        }
    }

    return sfound;
}

int GetSpectrumXExtent(double x, double y, XRectangle *r)
{
    
    if (!vP->m || !r) return 0;
    
    r->x      = xunit2x(x - vP->m->xspacing/2.0);
    r->y      = yunit2y(y + vP->m->yspacing/2.0);
    r->width  = xunit2x(x + vP->m->xspacing/2.0) - r->x;
    r->height = yunit2y(y - vP->m->yspacing/2.0) - r->y;
    
    return 1;
}

MAP *GetMapXExtent(int x, int y, XRectangle *r)
{
    int n;
    MAP *m;
    
    if (vP->nMaps <= 1) return 0;
    
    for (n=0; n<vP->nMaps; n++) {
        m = vP->M[n];
        if (!m) continue;
        if (x >= m->r.x && x <= m->r.x + m->r.width &&
            y >= m->r.y && y <= m->r.y + m->r.height) {
            if (r) *r = m->r;
            return m;
        }
    }
        
    return NULL;
}

scatter *GetScatterXExtent(int x, int y, XRectangle *r)
{
    int n;
    scatter *p;
    
    if (vP->nScat <= 1) return 0;
    
    for (n=0; n<vP->nScat; n++) {
        p = vP->P[n];
        if (!p) continue;
        if (x >= p->r.x && x <= p->r.x + p->r.width &&
            y >= p->r.y && y <= p->r.y + p->r.height) {
            if (r) *r = p->r;
            return p;
        }
    }
        
    return NULL;
}

void delete_spectrum(double x, double y, int ask)
{
    int inCurrentSingleView = 0;
    string buf;
    scanPtr sdel;
    XRectangle r;
    
    char *GetSpeDesc(scanPtr, int);
    
    sdel = RADec_to_s(x, y);
    
    if (!sdel) return;
    
    if (count_scans(vP->from) == 1) return;
    
    if (ask) {
        sprintf(buf, "%s\n\nDo you really want to delete it?",
                GetSpeDesc(sdel, 1));
    }
    
    /* Flag if the scan to be deleted is used in the current view */
    if (sdel == vP->s) inCurrentSingleView = 1;
    
    if (ask) {
        if (!PostQuestionDialog(NULL, buf)) return;
    }
    
    if (GetSpectrumXExtent(x, y, &r)) {
        XClearArea(XtDisplay(gp->graph), XtWindow(gp->graph),
                   r.x+1, r.y+1, r.width-2, r.height-2, False);
#ifdef USE_PIXMAP_STORAGE
        XFillRectangle(XtDisplay(gp->graph), gp->pm, gp->gcClear,
                       r.x+1, r.y+1, r.width-2, r.height-2);
#endif
    }
    
    DeleteScan(vP->from, sdel);
    
    /* Make sure we always have a scan pointer in the current view */
    if (inCurrentSingleView)
        vP->s = (scanPtr) DATA(scan_iterator(NULL, vP->from));
    
    obtain_map_info(NULL, "map", NULL);
}

void edit_map_gauss(Point *p)
{
    scanPtr s;
    
    void PostEditGaussDialog(scanPtr);
    
    if (p)
        s = RADec_to_s(p->x, p->y);
    else
        s = vP->s;
    
    if (!s) return;
    
    PostEditGaussDialog(s);
}

void swap_seq_spectra(double x1, double y1, double x2, double y2)
{
    double t1, t2;
    scanPtr s1, s2;
    
    s1 = RADec_to_s(x1, y1);
    s2 = RADec_to_s(x2, y2);
    
    if (!s1) return;
    if (s1 == s2) return;
    if (!vP->from->sequence) return;
    
    if (s2) {
        t1 = xmap(s1);
        t2 = xmap(s2);
        if (cType == COORD_TYPE_EQU || cType == COORD_TYPE_GAL) {
            s1->xoffset = t2;
            s2->xoffset = t1;
        } else {
            s1->aoffset = t2;
            s2->aoffset = t1;
        }
        t1 = ymap(s1);
        t2 = ymap(s2);
        if (cType == COORD_TYPE_EQU || cType == COORD_TYPE_GAL) {
            s1->yoffset = t2;
            s2->yoffset = t1;
        } else {
            s1->eoffset = t2;
            s2->eoffset = t1;
        }
    } else {
        if (cType == COORD_TYPE_EQU || cType == COORD_TYPE_GAL) {
            s1->xoffset = x2;
            s1->yoffset = y2;
        } else {
            s1->aoffset = x2;
            s1->eoffset = y2;
        }
    }
    
    obtain_map_info(NULL, "map", NULL);
    
    if (vP->mode == SHOW_ALLSPE)
        redraw_graph(gp->top, "update", NULL);
}

void swap_map_spectra(double x, double y)
{
    scanPtr sfound;

    sfound = RADec_to_s(x, y);

    if (!sfound) {
        sfound = RADec_to_nearest_s(x, y);
        if (!sfound) return;
    }
    
    SetViewMode(SHOW_SPE, sfound, vP->m, vP->p);

    UpdateData(SCALE_BOTH, REDRAW);
}

void swap_maps(MAP *m1, MAP *m2)
{
    int n, n1=-1, n2=-1;
    MAP *m;

    if (!m1 || !m2 || vP->nMaps <= 1) return;
    
    if (m1 == m2) return;
    
    for (n=0; n<vP->nMaps; n++) {
        if (vP->M[n] == m1) n1 = n;
        if (vP->M[n] == m2) n2 = n;
    }
    
    if (n1 < 0 || n2 < 0) return;
    
    m = vP->M[n1]; vP->M[n1] = vP->M[n2]; vP->M[n2] = m;
    
    redraw_graph(gp->top, "redraw", NULL);
}

void swap_scatters(scatter *p1, scatter *p2)
{
    int n, n1=-1, n2=-1;
    scatter *p;

    if (!p1 || !p2 || vP->nScat <= 1) return;
    
    if (p1 == p2) return;
    
    for (n=0; n<vP->nScat; n++) {
        if (vP->P[n] == p1) n1 = n;
        if (vP->P[n] == p2) n2 = n;
    }
    
    if (n1 < 0 || n2 < 0) return;
    
    p = vP->P[n1]; vP->P[n1] = vP->P[n2]; vP->P[n2] = p;
    
    redraw_graph(gp->top, "redraw", NULL);
}

static scanPtr xy_to_s(int x, int y)
{
    double xval, yval;

    double x2xunit(), y2yunit();

    xval = x2xunit(x);
    yval = y2yunit(y);

    return RADec_to_s(xval, yval);
}

double *xy_to_z(int x, int y)
{
    static double d;
    scanPtr s;

    s = xy_to_s(x, y);

    if (!s) return NULL;

    d = zmap(s);
    
    if (d == UNDEF) return NULL;

    return &d;
}

static double GetZFromShade(double shade)
{
    double z, f=0.0, s = shade;
    
    if (cont.grey_inverted == 0) s = 1.0 - s;
    
    if (s < 0.0) s = 0.0;
    if (s > 1.0) s = 1.0;
    
    if (cont.spacing == CONT_LINEAR) {
        f = s;
    } else if (cont.spacing == CONT_LOGARITHMIC) {
        f = log(s+1.0)/log(2.0);
    } else if (cont.spacing == CONT_EXPONENTIAL) {
        f = (exp(s)-1.0)/(exp(1.0)-1.0);
    } else if (cont.spacing == CONT_SQUAREROOT) {
        f = s*s;
    } else if (cont.spacing == CONT_QUADRATIC) {
        f = sqrt(s);
    } else if (cont.spacing == CONT_VARYING) {
        if (cont.pexp == 0.0)
            f = 0.5;
        else
            f = pow(s, 1.0/cont.pexp);
    }
    
    if (cont.nc > 1)
        z = cont.c[0] + f * (cont.c[cont.nc-1] - cont.c[0]);
    else
        z = f * cont.zmax;
    
    return z;
}

static double GetRawShade(double frac)
{
    double shade=0.0, f = frac;
    
    if (f < 0.0) f = 0.0;
    if (f > 1.0) f = 1.0;
    
    if (cont.spacing == CONT_LINEAR) {
        shade = f;
    } else if (cont.spacing == CONT_LOGARITHMIC) {
        shade = log(f+1.0)/log(2.0);
    } else if (cont.spacing == CONT_EXPONENTIAL) {
        shade = (exp(f)-1.0)/(exp(1.0)-1.0);
    } else if (cont.spacing == CONT_SQUAREROOT) {
        shade = sqrt(f);
    } else if (cont.spacing == CONT_QUADRATIC) {
        shade = f*f;
    } else if (cont.spacing == CONT_VARYING) {
        shade = pow(f, cont.pexp);
    }
    
    return shade;
}

static double GetShade(double z)
{
    double shade, frac;

    if (cont.nc > 1) {
        frac = (z - cont.c[0])/(cont.c[cont.nc-1] - cont.c[0]);
        shade = GetRawShade(frac);
    } else
        shade = GetRawShade(z/cont.zmax);

    if (shade < 0.0) shade = 0.0;
    if (shade > 1.0) shade = 1.0;
    if (cont.grey_inverted == 0) shade = 1.0 - shade;

    return shade;
}

int GetColorIndex(double z)
{
    int i=0;
    COLOR *c;

    COLOR *GetColorInfo();
    int GetPGColorOffset();
    
    if (cont.grey == SHADE_NONE) return 0;
    
    c = GetColorInfo();
    
    if (cont.grey == SHADE_FALSE) { 
        i = (int)(GetShade(z) * (double)(c->nfalse - 1) + 0.5);
    } else if (cont.grey == SHADE_GREY) {
        i = (int)(GetShade(z) * (double)(c->ngreys - 1) + 0.5);
    }
    i += GetPGColorOffset(cont.grey);
    return i;
}

unsigned long GetShadePixel(double z)
{
    int i;
    COLOR *c;

    COLOR *GetColorInfo();
    
    c = GetColorInfo();
    
    if (cont.grey == SHADE_FALSE) { 
        i = (int)(GetShade(z) * (double)(c->nfalse - 1) + 0.5);
        return c->false[i];
    } else if (cont.grey == SHADE_GREY) {
        i = (int)(GetShade(z) * (double)(c->ngreys - 1) + 0.5);
        return c->greys[i];
    } else {
        return c->black;
    }
}

static int GetRawColorIndex(double frac)
{
    int i=0;
    COLOR *c;

    COLOR *GetColorInfo();
    int GetPGColorOffset();
    
    if (cont.grey == SHADE_NONE) return 0;
    
    c = GetColorInfo();
    
    if (cont.grey == SHADE_FALSE) { 
        i = (int)(GetRawShade(frac) * (double)(c->nfalse - 1) + 0.5);
        if (!cont.grey_inverted) i = c->nfalse - 1 - i;
    } else if (cont.grey == SHADE_GREY) {
        i = (int)(GetRawShade(frac) * (double)(c->ngreys - 1) + 0.5);
        if (!cont.grey_inverted) i = c->ngreys - 1 - i;
    }
    i += GetPGColorOffset(cont.grey);
    return i;
}

static unsigned long GetRawShadePixel(double frac)
{
    int i;
    COLOR *c;

    COLOR *GetColorInfo();
    
    c = GetColorInfo();
    
    if (cont.grey == SHADE_FALSE) { 
        i = (int)(GetRawShade(frac) * (double)(c->nfalse - 1) + 0.5);
        if (!cont.grey_inverted) i = c->nfalse - 1 - i;
        return c->false[i];
    } else if (cont.grey == SHADE_GREY) {
        i = (int)(GetRawShade(frac) * (double)(c->ngreys - 1) + 0.5);
        if (!cont.grey_inverted) i = c->ngreys - 1 - i;
        return c->greys[i];
    } else {
        return c->black;
    }
}

unsigned long GetContourPixel(double z)
{
    COLOR *c;

    COLOR *GetColorInfo();
    
    c = GetColorInfo();

    if (cont.grey == SHADE_GREY) {
        if (GetShade(z) < SHADE_SWAP) {
            return (c->swapped) ? c->black : c->white;
        } else {
            return (c->swapped) ? c->white : c->black;
        }
    } else if (cont.grey == SHADE_FALSE) {
        if (GetShade(z) < SHADE_SWAP) {
            return (c->swapped) ? c->white : c->black;
        } else {
            return (c->swapped) ? c->black : c->white;
        }
    } else {
        return c->black;
    }
}

int GetPGci(double z)
{
    int i;
    COLOR *c;

    int GetPGColorOffset();
    COLOR *GetColorInfo();
    
    c = GetColorInfo();

    i = GetPGColorOffset(SHADE_GREY);

    if (cont.grey == SHADE_GREY) {
        if (GetShade(z) < SHADE_SWAP) {
            return (i + c->ngreys - 1);
        } else {
            return i;
        }
    } else if (cont.grey == SHADE_FALSE) {
        if (GetShade(z) < SHADE_SWAP) {
            return i;
        } else {
            return (i + c->ngreys - 1);
        }
    } else {
        return 1;
    }
}

#ifdef HAVE_LIBPGPLOT
static int clip_fill(double *xle, double *xri, double *ylo, double *yup)
{
    if (vP->xleft < vP->xright) {
        if (*xle < vP->xleft)  *xle = vP->xleft;
        if (*xri > vP->xright) *xri = vP->xright;
        if (*xle >= *xri) return 1;
    } else {
        if (*xle > vP->xleft)  *xle = vP->xleft;
        if (*xri < vP->xright) *xri = vP->xright;
        if (*xri >= *xle) return 1;
    }
    if (vP->ylower < vP->yupper) {
        if (*ylo < vP->ylower) *ylo = vP->ylower;
        if (*yup > vP->yupper) *yup = vP->yupper;
        if (*ylo >= *yup) return 1;
    } else {
        if (*ylo > vP->ylower) *ylo = vP->ylower;
        if (*yup < vP->yupper) *yup = vP->yupper;
        if (*yup >= *ylo) return 1;
    }
    
    return 0;
}
#endif

static double *zPlane(MAP *m, Point *p, int X[], int Y[])
{
    int i;
    double x[3], y[3], z[3], vx[2], vy[2], vz[2], v[3];
    static double zp;

    if (!m || !p) return NULL;
    
    for (i=0; i<3; i++) {
        x[i] = m->xleft  + (double)X[i] * m->xspacing;
        y[i] = m->ylower + (double)Y[i] * m->yspacing;
        z[i] = m->d[X[i]][Y[i]];
        if (i > 0) {
            vx[i-1] = x[i]-x[0];
            vy[i-1] = y[i]-y[0];
            vz[i-1] = z[i]-z[0];
        }
    }

    v[2] = vx[0]*vy[1] - vy[0]*vx[1];
    if (v[2] == 0.0) return NULL;

    v[0] = (vy[0]*vz[1] - vz[0]*vy[1])/v[2];
    v[1] = (vz[0]*vx[1] - vx[0]*vz[1])/v[2];

    zp = z[0] + v[0]*(x[0] - p->x) + v[1]*(y[0] - p->y);

    return &zp;
} 

double *GetMapValue(MAP *m, Point *p)
{
    int f1=1, f2=1, f3=1;
    int i, j, nX, nY;
    double x, y, d, d1=0.0, d2=0.0, d3=0.0;
    int X[3], Y[3];
    double *pz;
    static double z;
    
    X[0] = X[1] = X[2] = 0;
    Y[0] = Y[1] = Y[2] = 0;
    
    if (!m || !p) return NULL;
    
    nX = m->i_no; nY = m->j_no;
    for (i=0; i<nX; i++) {
        x = m->xleft + (double)i * m->xspacing; 
        for (j=0; j<nY; j++) {
            if (m->f[i][j] <= BLANK) continue;
            y = m->ylower + (double)j * m->yspacing;
            d = (x - p->x)*(x - p->x) + (y - p->y)*(y - p->y);
            if (f1) {
                d1 = d;
                X[0] = i; Y[0] = j;
                f1 = 0;
            } else if (f2) {
                if (d < d1) {
                    d2 = d1;
                    X[1] = X[0]; Y[1] = Y[0];
                    d1 = d;
                    X[0] = i; Y[0] = j;
                } else {
                    d2 = d;
                    X[1] = i; Y[1] = j;
                }
                f2 = 0;
            } else if (f3) {
                if (d < d1) {
                    d3 = d2;
                    X[2] = X[1]; Y[2] = Y[1];
                    d2 = d1;
                    X[1] = X[0]; Y[1] = Y[0];
                    d1 = d;
                    X[0] = i; Y[0] = j;
                } else if (d < d2) {
                    d3 = d2;
                    X[2] = X[1]; Y[2] = Y[1];
                    d2 = d;
                    X[1] = i; Y[1] = j;
                } else {
                    d3 = d;
                    X[2] = i; Y[2] = j;
                }
                f3 = 0;
            } else {
                if (d < d1) {
                    d3 = d2;
                    X[2] = X[1]; Y[2] = Y[1];
                    d2 = d1;
                    X[1] = X[0]; Y[1] = Y[0];
                    d1 = d;
                    X[0] = i; Y[0] = j;
                } else if (d < d2) {
                    d3 = d2;
                    X[2] = X[1]; Y[2] = Y[1];
                    d2 = d;
                    X[1] = i; Y[1] = j;
                } else if (d < d3) {
                    d3 = d;
                    X[2] = i; Y[2] = j;
                }
            }
        }
    }
    
    if (f1) return NULL; /* No points at all */
    
    if (d1 >= m->xspacing*m->xspacing + m->yspacing*m->yspacing) return NULL;
    
    if (f2) {        /* Only one point */
        z = m->d[X[0]][Y[0]];
    } else if (f3) { /* Only two points */
        if (d1 == 0.0 && d2 == 0.0) {
            z = m->d[X[0]][Y[0]];
        } else {
            z = (m->d[X[0]][Y[0]]*d2 + m->d[X[1]][Y[1]]*d1)/(d1 + d2);
        }
    } else {         /* All three points */
        if (d1 == 0.0 && d2 == 0.0 && d3 == 0.0) {
            z = m->d[X[0]][Y[0]];
        } else {
            if ((pz = zPlane(m, p, X, Y))) return pz;
            
            z = (m->d[X[0]][Y[0]]*d2*d3 +
                 m->d[X[1]][Y[1]]*d1*d3 +
                 m->d[X[2]][Y[2]]*d1*d2) / (d2*d3 + d1*d3 + d1*d2);
        }
    }
    
    return &z;
}

static PAIR *find_pair(double level, double **a, int *nx, int *ny, MAP *pMap)
{
    int i, j, found, np=0;
    static PAIR pair;
    double X, Y, t;
    double x[4], y[4], z[4];

    for (i=0; i<3; i++) {
        x[i] = pMap->xleft + (double)(*nx) * pMap->xspacing;
        y[i] = pMap->ylower + (double)(*ny) * pMap->yspacing;
        z[i] = a[*nx][*ny];
        nx++;
        ny++;
    }
    x[3]=x[0]; y[3]=y[0]; z[3]=z[0];

    for (i=0; i<3; i++) {
        if (z[i+1] != z[i]) {
            t = (level - z[i])/(z[i+1] - z[i]);
            if (t < 0.0 || t > 1.0) continue;
        } else {
            if (level != z[i]) continue;
            t = 0.5;
        }
        X = x[i] + t*(x[i+1] - x[i]);
        Y = y[i] + t*(y[i+1] - y[i]);
        found = 0;
        for (j=0; j<np; j++) {
            if (pair.x[j] == X && pair.y[j] == Y) {
                found=1;
                break;
            }
        }
        if (found) continue;
        if (np > 1) return NULL;
        pair.x[np] = X;
        pair.y[np] = Y;
        np++;
    }
    if (np != 2) return NULL;
    
    return (&pair);
}

int CheckWedge()
{
    if (!ANYMAP(vP->mode)) return 0;
    if (draw.wedge && cont.grey) return 1;
    
    return 0;
}

int GetWedgePos()
{
    return draw.wedgepos;
}

#define NASTEPS 361

void draw_beam(GC gc, double x0, double y0, Beam *b)
{
    int n, X, Y, Xp=0, Yp=0;
    double sp, cp, x, y, u, v, cv, sv, r;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx[NASTEPS], fy[NASTEPS];
#endif

    void draw_line();
    
    if (!b) return;
    
    sp = sin(b->PA/RADTODEG);
    cp = cos(b->PA/RADTODEG);

    for (n=0; n<=360; n++) {
        v = (double)n/RADTODEG;
        sv = sin(v); cv = cos(v);
        x = b->maj*cv;
        y = b->min*sv;
        r = b->maj*b->min/sqrt(x*x + y*y)/2.0;
        u = r * cv;
        v = r * sv;
        x = x0 + u*cp + v*sp;
        y = y0 - u*sp + v*cp;
        X = xunit2x(x);
        Y = yunit2y(y);
        if (n > 1) {
            draw_line(gc, Xp, Yp, X, Y);
        }
        Xp = X;
        Yp = Y;
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx[n] = (PLFLT)x;
            fy[n] = (PLFLT)y;
        }
#endif
    }
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        cpgpoly(361, fx, fy);
    }
#endif
}

static void draw_mapbeam(GC gc, MAP *m)
{
    int n, X, Y, Xp=0, Yp=0;
    double dx, dy, x0, y0, xoff, yoff, xmin=0.0, xmax=0.0, ymin=0.0, ymax=0.0;
    double x, y, u, v, cp, sp, cv, sv, r;
/* #ifdef HAVE_LIBPGPLOT
    PLFLT fx[NASTEPS], fy[NASTEPS];
#endif */

    void draw_line(), draw_xbox();
    
    if (!m) return;
    if (m->b.maj == 0.0 || m->b.min == 0.0) return;
    
    dx = vP->xleft - vP->xright;
    dy = vP->yupper - vP->ylower;
    
    sp = sin(m->b.PA/RADTODEG);
    cp = cos(m->b.PA/RADTODEG);
    
    for (n=0; n<NASTEPS-1; n++) {
        v = (double)n/RADTODEG;
        sv = sin(v); cv = cos(v);
        x = m->b.maj*cv;
        y = m->b.min*sv;
        r = m->b.maj*m->b.min/sqrt(x*x + y*y)/2.0;
        u = r * cv;
        v = r * sv;
        x = u*cp + v*sp;
        y = -u*sp + v*cp;
        if (n == 0) {
            xmin = xmax = x;
            ymin = ymax = y;
        } else {
            if (x < xmin) xmin = x;
            if (y < ymin) ymin = y;
            if (x > xmax) xmax = x;
            if (y > ymax) ymax = y;
        }
    }
    
    xoff = (xmax-xmin)/2.0 + dx/15.0;
    yoff = (ymax-ymin)/2.0 + dx/15.0;
    
    if (draw.beam == MAP_BEAM_LL) {
       x0 = vP->xleft - xoff;
       y0 = vP->ylower + yoff;
    } else if (draw.beam == MAP_BEAM_LR) {
       x0 = vP->xright + xoff;
       y0 = vP->ylower + yoff;
    } else if (draw.beam == MAP_BEAM_UL) {
       x0 = vP->xleft - xoff;
       y0 = vP->yupper - yoff;
    } else if (draw.beam == MAP_BEAM_UR) {
       x0 = vP->xright + xoff;
       y0 = vP->yupper - yoff;
    } else {
       x0 = 0.0;
       y0 = 0.0;
    }
    
    X  = xunit2x(x0 + xmin);
    Y  = yunit2y(y0 + ymin);
    Xp = xunit2x(x0 + xmax);
    Yp = yunit2y(y0 + ymax);
    draw_xbox(gc, X, Y, Xp, Yp);
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.beambox);
        cpgrect((PLFLT)(x0 + xmin), (PLFLT)(x0 + xmax),
                (PLFLT)(y0 + ymin), (PLFLT)(y0 + ymax));
        SetPGStyle(&ps.beam);
    }
#endif

    draw_beam(gc, x0, y0, &(m->b));
    
    /* for (n=0; n<=360; n++) {
        v = (double)n/RADTODEG;
        sv = sin(v); cv = cos(v);
        x = m->b.maj*cv;
        y = m->b.min*sv;
        r = m->b.maj*m->b.min/sqrt(x*x + y*y)/2.0;
        u = r * cv;
        v = r * sv;
        x = x0 + u*cp + v*sp;
        y = y0 - u*sp + v*cp;
        X = xunit2x(x);
        Y = yunit2y(y);
        if (n > 1) {
            draw_line(gc, Xp, Yp, X, Y);
        }
        Xp = X;
        Yp = Y;
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx[n] = (PLFLT)x;
            fy[n] = (PLFLT)y;
        }
#endif
    }
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        cpgpoly(361, fx, fy);
    }
#endif */
}

static void draw_projnums(GC gc)
{
    string lab;
    double Phi = -180.0, Theta=-60.0;
    Point r, r1;
    
    void DrawAbsLabel();
    
    if (pType == 0) return;
    
    while (Phi <= 180.0) {
        sprintf(lab, "%d", NINT(Phi));
        if (pType <= 5) {
            if (Phi < 0.0) {
                r.x = Phi + 5.0;
                r.y = -7.0;
            } else {
                r.x = Phi - 5.0;
                r.y = 5.0;
            }    
            DrawAbsLabel(gc, r.x, r.y, 0.5, lab);
        } else if (pType == 6) {
            if (fabs(Phi) <= 60.0 || fabs(Phi) >= 120.0) {
                pt2xy(Phi, 0.0, pType, &r);
                if (Phi == -180.0) {
                    r.x += 9.0;
                } else if (Phi == 180.0) {
                    r.x -= 9.0;
                }
                r.y -= 7.0;
                DrawAbsLabel(gc, r.x, r.y, 0.5, lab);
            }
        } else if (pType == 8) {
            if (fabs(Phi) >= 30.0 && fabs(Phi) <= 150.0) {
                pt2xy(Phi, 0.0, pType, &r);
                r.y -= 7.0;
                DrawAbsLabel(gc, r.x, r.y, 0.5, lab);
            }
        } else if (pType == 7) {
            if (Phi != 90.0) {
                pt2xy(Phi, 0.001, pType, &r);
                pt2xy(Phi, 20.0, pType, &r1);
                r.x += r.x - r1.x;
                r.y += r.y - r1.y;
                if (Phi == -180.0) r.x -= 9.0;
                if (Phi ==  180.0) r.x += 9.0;
                DrawAbsLabel(gc, r.x, r.y, 0.5, lab);
            }
            if (Phi != -90.0) {
                pt2xy(Phi, -0.001, pType, &r);
                pt2xy(Phi, -20.0, pType, &r1);
                r.x += r.x - r1.x;
                r.y += r.y - r1.y;
                if (Phi == -180.0) r.x -= 9.0;
                if (Phi ==  180.0) r.x += 9.0;
                DrawAbsLabel(gc, r.x, r.y, 0.5, lab);
            }
        }
        Phi += 30.0;
    }
    
    while (Theta <= 60.0) {
        sprintf(lab, "%d", NINT(Theta));
        if (pType <= 5) {
            pt2xy(-180.0, Theta, pType, &r);
            r.x -= 5.0;
            if (Theta < 0.0) {
                r.y -= 3.0;
            } else {
                r.y += 0.0;
            }
            DrawAbsLabel(gc, r.x, r.y, 1.0, lab);
        } else if (pType == 6 || pType == 8) {
            pt2xy((pType==6 ? -90.0 : -180.0), Theta, pType, &r);
            r.x -= 5.0;
            if (Theta < 0.0) {
                r.y -= 3.0;
            } else {
                r.y += 0.0;
            }
            DrawAbsLabel(gc, r.x, r.y, 1.0, lab);
        } else if (pType == 7) {
            if (Theta < 0.0) {
                pt2xy(-80.0, Theta, pType, &r);
                DrawAbsLabel(gc, r.x, r.y, 0.0, lab);
            } else if (Theta > 0.0) {
                pt2xy(-100.0, Theta, pType, &r);
                DrawAbsLabel(gc, r.x, r.y, 0.0, lab);
            }
        }
        Theta += 30.0;
    }
}

static void draw_projaxes(GC gc)
{
    int j, n, ix[2], iy[2];
    double Phi, Theta, t1, t2, p1, p2;
#ifdef HAVE_LIBPGPLOT
    Point r;
    PLFLT fx[NASTEPS], fy[NASTEPS];
    PLFLT fxx[NASTEPS], fyy[NASTEPS];
#endif

    void draw_line();
    
    j = -180;
    while (j <= 180) {
        Phi = (double)j;
        if (j == -1) {
            Phi = -90.0001;
        } else if (j == 1) {
            Phi = 90.0001;
        } else if (j == 2) {
            Phi =  0.0001;
        }
        for (n=0; n < NASTEPS-1; n++) {
            t1 = -90.0 + (double)n *89.9999/(double)(NASTEPS-1);
            t2 = t1 + 89.9999/(double)(NASTEPS-1);
            ptunit2xy(Phi, t1, pType, &ix[0], &iy[0]);
            ptunit2xy(Phi, t2, pType, &ix[1], &iy[1]);
            draw_line(gc, ix[0], iy[0], ix[1], iy[1]);
#ifdef HAVE_LIBPGPLOT
            if (pgplot) {
                pt2xy(Phi, t1, pType, &r);
                fx[n] = (PLFLT)r.x; fy[n] = (PLFLT)r.y;
                r.x = r.y = 0.0;
                if (n == NASTEPS-2) {
                    pt2xy(Phi, t2, pType, &r);
                    fx[n+1] = (PLFLT)r.x; fy[n+1] = (PLFLT)r.y;
                }
            }
#endif
        }
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            SetPGStyle(&ps.subbox.style);
            cpgline(NASTEPS, fx, fy);
        }
#endif
        for (n=0; n < NASTEPS-1; n++) {
            t1 = (double)n *90.0/(double)(NASTEPS-1);
            t2 = t1 + 90.0/(double)(NASTEPS-1);
            ptunit2xy(Phi, t1, pType, &ix[0], &iy[0]);
            ptunit2xy(Phi, t2, pType, &ix[1], &iy[1]);
            draw_line(gc, ix[0], iy[0], ix[1], iy[1]);
#ifdef HAVE_LIBPGPLOT
            if (pgplot) {
                pt2xy(Phi, t1, pType, &r);
                fx[n] = (PLFLT)r.x; fy[n] = (PLFLT)r.y;
                if (n == NASTEPS-2) {
                    pt2xy(Phi, t2, pType, &r);
                    fx[n+1] = (PLFLT)r.x; fy[n+1] = (PLFLT)r.y;
                }
            }
#endif
        }
#ifdef HAVE_LIBPGPLOT
        if (pgplot) cpgline(NASTEPS, fx, fy);
#endif
        if (j == -90) {
            j = -1;
        } else if (j == 90) {
            j = 1;
        } else if (j == 0) {
            j = 2;
        } else if (j == -1) {
            j = -60;
        } else if (j == 1) {
            j = 120;
        } else if (j == 2) {
            j = 30;
        } else {
            j += 30;
        }
    }
    
    j = -90;
    while (j <= 90) {
        Theta = (double)j;
        if (j == 1) {
            Theta = -0.00001;
        }
        for (n=0; n < NASTEPS-1; n++) {
            p1 = -180.0 + (double)n *89.9999/(double)(NASTEPS-1);
            p2 = p1 + 89.9999/(double)(NASTEPS-1);
            ptunit2xy(p1, Theta, pType, &ix[0], &iy[0]);
            ptunit2xy(p2, Theta, pType, &ix[1], &iy[1]);
            draw_line(gc, ix[0], iy[0], ix[1], iy[1]);
#ifdef HAVE_LIBPGPLOT
            if (pgplot) {
                pt2xy(p1, Theta, pType, &r);
                fx[n] = (PLFLT)r.x; fy[n] = (PLFLT)r.y;
                if (n == NASTEPS-2) {
                    pt2xy(p2, Theta, pType, &r);
                    fx[n+1] = (PLFLT)r.x; fy[n+1] = (PLFLT)r.y;
                }
            }
#endif
            ptunit2xy(-p1, Theta, pType, &ix[0], &iy[0]);
            ptunit2xy(-p2, Theta, pType, &ix[1], &iy[1]);
            draw_line(gc, ix[0], iy[0], ix[1], iy[1]);
#ifdef HAVE_LIBPGPLOT
            if (pgplot) {
                pt2xy(-p1, Theta, pType, &r);
                fxx[n] = (PLFLT)r.x; fyy[n] = (PLFLT)r.y;
                if (n == NASTEPS-2) {
                    pt2xy(-p2, Theta, pType, &r);
                    fxx[n+1] = (PLFLT)r.x; fyy[n+1] = (PLFLT)r.y;
                }
            }
#endif
        }
#ifdef HAVE_LIBPGPLOT
        if (pgplot) cpgline(NASTEPS, fx, fy);
        if (pgplot) cpgline(NASTEPS, fxx, fyy);
#endif
        for (n=0; n<NASTEPS-1; n++) {
            p1 = -90.0 + (double)n *89.9999/(double)(NASTEPS-1);
            p2 = p1 + 89.9999/(double)(NASTEPS-1);
            ptunit2xy(p1, Theta, pType, &ix[0], &iy[0]);
            ptunit2xy(p2, Theta, pType, &ix[1], &iy[1]);
            draw_line(gc, ix[0], iy[0], ix[1], iy[1]);
#ifdef HAVE_LIBPGPLOT
            if (pgplot) {
                pt2xy(p1, Theta, pType, &r);
                fx[n] = (PLFLT)r.x; fy[n] = (PLFLT)r.y;
                if (n == NASTEPS-2) {
                    pt2xy(p2, Theta, pType, &r);
                    fx[n+1] = (PLFLT)r.x; fy[n+1] = (PLFLT)r.y;
                }
            }
#endif
            ptunit2xy(-p1, Theta, pType, &ix[0], &iy[0]);
            ptunit2xy(-p2, Theta, pType, &ix[1], &iy[1]);
            draw_line(gc, ix[0], iy[0], ix[1], iy[1]);
#ifdef HAVE_LIBPGPLOT
            if (pgplot) {
                pt2xy(-p1, Theta, pType, &r);
                fxx[n] = (PLFLT)r.x; fyy[n] = (PLFLT)r.y;
                if (n == NASTEPS-2) {
                    pt2xy(-p2, Theta, pType, &r);
                    fxx[n+1] = (PLFLT)r.x; fyy[n+1] = (PLFLT)r.y;
                }
            }
#endif
        }
#ifdef HAVE_LIBPGPLOT
        if (pgplot) cpgline(NASTEPS, fx, fy);
        if (pgplot) cpgline(NASTEPS, fxx, fyy);
#endif
        if (j == 0) {
            j = 1;
        } else if (j == 1) {
            j = 30;
        } else {
            j += 30;
        }
    }
}

void draw_polyline(GC gc, PolyLine *pl)
{
    int n, proj;
    int x1, x2, y1, y2;
    Point P1, P2, *p1, *p2;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx[2], fy[2];
#endif
    
    void draw_line();
    
    if (!pl) return;
    
    proj = (pType && vP->mode == SHOW_POSPOS) ? 1 : 0;
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.mobox);
    }
#endif
    for (n=0; n<pl->n; n++) {
        p1 = &(pl->p[n]);
        if (n == pl->n - 1) {
            if (pl->type != 1) break;
            p2 = &(pl->p[0]);
        } else {
            p2 = &(pl->p[n+1]);
        }
        if (proj) {
            pt2xy(p1->x, p1->y, pType, &P1);
            pt2xy(p2->x, p2->y, pType, &P2);
            p1 = &P1; p2 = &P2;
        }
        x1 = xunit2x(p1->x);
        y1 = yunit2y(p1->y);
        x2 = xunit2x(p2->x);
        y2 = yunit2y(p2->y);
        draw_line(gc, x1, y1, x2, y2);
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            fx[0] = (PLFLT)p1->x; fx[1] = (PLFLT)p2->x;
            fy[0] = (PLFLT)p1->y; fy[1] = (PLFLT)p2->y;
            cpgline(2, fx, fy);
        }
#endif
    }
}

static void draw_contours(GC g_gc, GC l_gc, MAP *m)
{
    int i, j, k, n, np, not_drawn=0, err, nl, rot=0, proj=0;
    char xticks[10], yticks[10];
    static int arrX[5], arrY[5];
    static int istep[] = {0, 1, 1, 0, 0};
    static int jstep[] = {0, 0, 1, 1, 0};
    int x1=0, x2=0, y1=0, y2=0;
    int xr[4], yr[4];
    int nX, nY, nC;
    double xs, ys, I=0.0, I2=0.0, Area=0.0, AreaPixel=0.0, Sigma=0.0, PA=0.0;
    double X1, X2, Y1, Y2, c_inc;
    Point p, P, P0, P1;
    PAIR *pair;
    XPAIR tmp, xpair[MAXPAIRS];
#ifdef HAVE_LIBPGPLOT
    PLFLT f, fx[2], fy[2], fxr[4], fyr[4];
    PLPAIR pltmp, plpair[MAXPAIRS];
    double xle=0.0, xri=0.0, ylo=0.0, yup=0.0, angle=0.0, delta=0.0;
    Point R, dr[4];
#endif
    int **L;
    double **A;
    string buf;
    Display *dpy = XtDisplay(gp->graph);
    COLOR *c;
    unsigned long *cols;
    VIEW    vtmp;

    int point_is_inside();
    double y2yunit(), x2xunit();
    void draw_line(), draw_numeric_label(), send_line();
    void draw_filled_xbox(), draw_frame(), draw_ticks();
    void draw_filled_xpol(), draw_all_labels();
    void SetWindow();
    char *GetNumericLabel();
    COLOR *GetColorInfo();

    int FillHolesInArray(), InterpolateArray();
    void FreeIntArray(), FreeDoubleArray();
    int **AllocIntArray();
    double **AllocDoubleArray();

    if (!m) return;

    nX = m->i_no;
    nY = m->j_no;
    xs = m->xspacing;
    ys = m->yspacing;
    if (forcePosAngle) {
        PA = posAngle;
    } else {
        PA = m->posAngle;
    }
    
    rot = (rType && m->type == MAP_POSPOS) ? 1 : 0;
    proj = (!rot && pType && m->type == MAP_POSPOS) ? 1 : 0;
    
    c = GetColorInfo();

    A = AllocDoubleArray(nX, nY);
    if (!A) {
        PostErrorDialog(NULL, "draw_contour: Out of memory.");
        return;
    }
    L = AllocIntArray(nX, nY);
    if (!L) {
        FreeDoubleArray(A, nX, nY);
        PostErrorDialog(NULL, "draw_contour: Out of memory.");
        return;
    }
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            A[i][j] = m->d[i][j];
            if (m->f[i][j] <= BLANK)
                L[i][j] = EMPTY;
            else
                L[i][j] = FILLED;
        }
    }
    
    if (m->use_attached_cont) cont = m->c;

    if (cont.nCorners > 0)
        err = FillHolesInArray(A, L, nX, nY, cont.nCorners);

    if ((i = cont.intpOrder)) {
        while (i) {
            if (nX > MAXARRAYSIZE || nY > MAXARRAYSIZE) {
                sprintf(buf, "Current array size in interpolation: %dx%d\n\
This may take time. Continue interpolating?", nX, nY);
                if (!PostQuestionDialog(NULL, buf)) {
                    cont.intpOrder = i;
                    break;
                }
            }
            err = InterpolateArray(&A, &L, &nX, &nY, cont.intpType);
            if (err != 0) {
                sprintf(buf, "Couldn't interpolate %dx%d array. Error=%d.",
                        nX, nY, err);
                send_line(buf);
                if (A) FreeDoubleArray(A, nX, nY);
                if (L) FreeIntArray(L, nX, nY);
                PostErrorDialog(NULL, buf);
                return;
            }
            m->xspacing /= 2.0;
            m->yspacing /= 2.0;
            i--;
            if (cont.nCorners > 0)
                err = FillHolesInArray(A, L, nX, nY, cont.nCorners);
        }
    }

#ifdef HAVE_LIBPGPLOT
    if (pgplot) cpgbbuf();
#endif
    
    if (cont.grey) {
        I = I2 = Area = 0.0;
        AreaPixel = fabs(m->xspacing * m->yspacing);
        np = 0;
        for (i=0; i<nX; i++) {
            for (j=0; j<nY; j++) {
                if (!L[i][j]) continue;
                p.x = m->xleft  + (double)(i) * m->xspacing;
                p.y = m->ylower + (double)(j) * m->yspacing;
                if (InsidePolyLines(&p)) {
                    Area += AreaPixel;
                    I += A[i][j] * AreaPixel;
                    I2 += A[i][j] * A[i][j] * AreaPixel * AreaPixel;
                    np++;
                    if (cont.blank) continue;
                }
                X1 = p.x - m->xspacing/2.0; Y1 = p.y - m->yspacing/2.0;
                X2 = p.x + m->xspacing/2.0; Y2 = p.y + m->yspacing/2.0;
                if (rot) {
                    P.x = X1; P.y = Y1; uvunit2xy(P, PA, &xr[0], &yr[0]);
                    P.x = X1; P.y = Y2; uvunit2xy(P, PA, &xr[1], &yr[1]);
                    P.x = X2; P.y = Y2; uvunit2xy(P, PA, &xr[2], &yr[2]);
                    P.x = X2; P.y = Y1; uvunit2xy(P, PA, &xr[3], &yr[3]);
                } else if (proj) {
                    ptunit4xy(X1, X2, Y1, Y2, pType, xr, yr);
                } else {
                    x1 = xunit2x(X1); x2 = xunit2x(X2);
                    if (x1 > x2) {
                        k = x1; x1 = x2; x2 = k;
                    }
                    y1 = yunit2y(Y1); y2 = yunit2y(Y2);
                }
                XSetForeground(dpy, g_gc, GetShadePixel(A[i][j]));
                if (rot || proj) {
                    draw_filled_xpol(g_gc, xr, yr, 4);
                } else {
                    draw_filled_xbox(g_gc, x1, y1, x2, y2);
                }

#ifdef HAVE_LIBPGPLOT
                if (pgplot) {
		    if (vP->xleft > vP->xright) {
		      if (m->xleft > m->xright) {
                        xle = X1; xri = X2;
		      } else {
                        xle = X2; xri = X1;
		      }
		    } else {
		      if (m->xleft > m->xright) {
                        xle = X2; xri = X1;
		      } else {
                        xle = X1; xri = X2;
		      }
		    }
		    if (vP->ylower < vP->yupper) {
		      if (m->ylower < m->yupper) {
                        ylo = Y1; yup = Y2;
		      } else {
                        ylo = Y2; yup = Y1;
		      }
		    } else {
		      if (m->ylower < m->yupper) {
                        ylo = Y2; yup = Y1;
		      } else {
                        ylo = Y1; yup = Y2;
		      }
		    }
                    if (rot) {
                        P.x = xle; P.y = ylo; uv2xy(P, PA, &R);
                        fxr[0] = (PLFLT)R.x; fyr[0] = (PLFLT)R.y;
                        P.x = xle; P.y = yup; uv2xy(P, PA, &R);
                        fxr[1] = (PLFLT)R.x; fyr[1] = (PLFLT)R.y;
                        P.x = xri; P.y = yup; uv2xy(P, PA, &R);
                        fxr[2] = (PLFLT)R.x; fyr[2] = (PLFLT)R.y;
                        P.x = xri; P.y = ylo; uv2xy(P, PA, &R);
                        fxr[3] = (PLFLT)R.x; fyr[3] = (PLFLT)R.y;
                    } else if (proj) {
                        pt4xy(xle, xri, ylo, yup, pType, dr);
                        for (n=0; n<4; n++) {
                           fxr[n] = (PLFLT)dr[n].x;
                           fyr[n] = (PLFLT)dr[n].y;
                        }
                    } else {
                        if (clip_fill(&xle, &xri, &ylo, &yup)) continue;
                    }
                    cpgsfs(1);
                    cpgsci(GetColorIndex(A[i][j]));
                    if (rot || proj)
                        cpgpoly(4, fxr, fyr);
                    else {
/* Avoid striping on screen */
		        if (vP->xleft < vP->xright) {
			    delta = vP->xright - vP->xleft;
			    xle -= STRIPING*delta;
			    xri += STRIPING*delta;
			} else {
			    delta = vP->xleft - vP->xright;
			    xle += STRIPING*delta;
			    xri -= STRIPING*delta;
			}
			if (vP->ylower < vP->yupper) {
			    delta = vP->yupper - vP->ylower;
			    ylo -= STRIPING*delta;
			    yup += STRIPING*delta;
			} else {
			    delta = vP->ylower - vP->yupper;
			    ylo += STRIPING*delta;
			    yup -= STRIPING*delta;
			}
                        cpgrect((PLFLT)xle, (PLFLT)xri, (PLFLT)ylo, (PLFLT)yup);
		    }
                }
#endif
            }
        }
        if (draw.wedge) {
            if (cont.grey == SHADE_FALSE) {
                nC = c->nfalse;
                cols = c->false;
            } else {
                nC = c->ngreys;
                cols = c->greys;
            }
            if (nC > 1) {
                vtmp = *vP;  /* Save original view */
                vP->mode = SHOW_WEDGE;
                vP->fixed_y = 0;
                vP->fixed_z = 0;
                vP->fixed_x = 0;
                switch (draw.wedgepos) {
                    case POS_RIGHT:
                        x1 = vP->min_x + vP->box_w + vP->box_w/20;
                        x2 = x1 + vP->box_w/15;
                        vP->min_x = x1;
                        vP->box_w = vP->box_w/15;
                        strcpy(xticks, "");
                        strcpy(yticks, "Rri");
                        break;
                    case POS_LEFT:
                        x1 = vP->min_x - vP->box_w/10;
                        x2 = x1 + vP->box_w/15;
                        vP->min_x = x1;
                        vP->box_w = vP->box_w/15;
                        strcpy(xticks, "");
                        strcpy(yticks, "Lli");
                        break;
                    case POS_ABOVE:
                        y1 = vP->min_y - vP->box_h - vP->box_h/20;
                        y2 = y1 - vP->box_h/15;
                        vP->min_y = y1;
                        vP->box_h = vP->box_h/15;
                        strcpy(xticks, "Tti");
                        strcpy(yticks, "");
                        break;
                    case POS_BELOW:
                        y1 = vP->min_y + vP->box_h/10;
                        y2 = y1 - vP->box_h/15;
                        vP->min_y = y1;
                        vP->box_h = vP->box_h/15;
                        strcpy(xticks, "Bbi");
                        strcpy(yticks, "");
                        break;
                }
#ifdef HAVE_LIBPGPLOT
                if (pgplot) {
                    cpgqvp(0, &fx[0], &fx[1], &fy[0], &fy[1]);
                    xle = fx[0]; xri = fx[1];
                    ylo = fy[0]; yup = fy[1];
                    switch (draw.wedgepos) {
                        case POS_RIGHT:
                            xle = fx[1] + (fx[1]-fx[0])/20.0;
                            xri = xle + (fx[1]-fx[0])/15.0;
                            break;
                        case POS_LEFT:
                            xle = fx[0] - (fx[1]-fx[0])/10.0;
                            xri = xle + (fx[1]-fx[0])/15.0;
                            break;
                        case POS_ABOVE:
                            ylo = fy[1] + (fy[1]-fy[0])/20.0;
                            yup = ylo + (fy[1]-fy[0])/15.0;
                            break;
                        case POS_BELOW:
                            ylo = fy[0] - (fy[1]-fy[0])/10.0;
                            yup = ylo + (fy[1]-fy[0])/15.0;
                            break;
                    }
                    cpgsvp(xle, xri, ylo, yup);
                    xle = vP->xleft;
                    xri = vP->xright;
                    ylo = vP->ylower;
                    yup = vP->yupper;
                }
#endif
                if (draw.wedgepos == POS_RIGHT || draw.wedgepos == POS_LEFT) {
                    vP->ylower = GetZFromShade(0.0);
                    vP->yupper = GetZFromShade(1.0);
                    if (vP->ylower > vP->yupper) {
                        vP->yupper = vP->ylower;
                        vP->ylower = GetZFromShade(1.0);
                    }
                } else {
                    vP->xleft  = GetZFromShade(0.0);
                    vP->xright = GetZFromShade(1.0);
                    if (vP->xleft > vP->xright) {
                        vP->xright = vP->xleft;
                        vP->xleft = GetZFromShade(1.0);
                    }
                }
                SetWindow(vP->xleft, vP->xright, vP->ylower, vP->yupper);
#ifdef HAVE_LIBPGPLOT
                if (pgplot) {
                    cpgswin((PLFLT)vP->xleft, (PLFLT)vP->xright,
                            (PLFLT)vP->ylower, (PLFLT)vP->yupper);
                }
#endif
                for (i=0; i<nC; i++) {
                    if (draw.wedgepos == POS_RIGHT || draw.wedgepos == POS_LEFT) {
                        y1 = vP->min_y - ((2*i-1)*vP->box_h)/(nC-1)/2;
                        y2 = vP->min_y - ((2*i+1)*vP->box_h)/(nC-1)/2;
                    } else {
                        x1 = vP->min_x + ((2*i-1)*vP->box_w)/(nC-1)/2;
                        x2 = vP->min_x + ((2*i+1)*vP->box_w)/(nC-1)/2;
                    }
                    XSetForeground(dpy, g_gc,
                                 GetRawShadePixel((double)(i)/(double)(nC-1)));
                    draw_filled_xbox(g_gc, x1, y1, x2, y2);
#ifdef HAVE_LIBPGPLOT
                    if (pgplot) {
                       if (draw.wedgepos == POS_RIGHT ||
                           draw.wedgepos == POS_LEFT) {
                           ylo = y2yunit(y1);
                           yup = y2yunit(y2);
                       } else {
                           xle = x2xunit(x1);
                           xri = x2xunit(x2);
                       }
                       cpgsfs(1);
                       cpgsci(GetRawColorIndex((double)(i)/(double)(nC-1)));
		       if (vP->ylower < vP->yupper) {
		           delta = vP->yupper - vP->ylower;
/* To avoid striping in contour wedge */
			   ylo -= STRIPING*delta;
			   yup += STRIPING*delta;
		       } else {
		           delta = vP->ylower - vP->yupper;
/* To avoid striping in contour wedge */
			   ylo += STRIPING*delta;
			   yup -= STRIPING*delta;
		       }
                       cpgrect((PLFLT)xle, (PLFLT)xri, (PLFLT)ylo, (PLFLT)yup);
                    }
#endif
                }
                draw_frame(gp->gcFrame[5], 1, 0);
                if (draw.wticks) draw_ticks(gp->gcFrame[2], xticks, yticks);
                if (draw.wlabels) draw_all_labels();
                *vP = vtmp;   /* Restore to original view */
#ifdef HAVE_LIBPGPLOT
                if (pgplot) {
                    cpgsvp(fx[0], fx[1], fy[0], fy[1]);
                    cpgswin((PLFLT)vP->xleft, (PLFLT)vP->xright,
                            (PLFLT)vP->ylower, (PLFLT)vP->yupper);
                }
#endif
            }
        }
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            cpgsci(1);
            cpgsfs(2);
        }
#endif
        if (np) {
            wprintf(gp->TBaseline[2], "Mean: %10.3e K km/s", I/Area);
            if (np > 1 && AreaPixel != 0.0) {
                Sigma = sqrt((I2 - I*I/(double)np)/(double)(np-1))/AreaPixel;
                wprintf(gp->TBaseline[3], "Sigma: %10.3e K km/s", Sigma);
            } else {
                wprintf(gp->TBaseline[3], "Sigma: *");
            }
            wprintf(gp->TPolyline[1], "%d map pixels", np);
            wprintf(gp->TPolyline[2], "Mean: %10.3e K km/s", I/Area);
            wprintf(gp->TPolyline[3], "I: %10.3e K km/s \"^2", I);
        } else {
            wprintf(gp->TPolyline[1], "");
            wprintf(gp->TPolyline[2], "");
            wprintf(gp->TPolyline[3], "");
        }
    }
    
    if (cont.quick > 0) {
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            SetPGStyle(&ps.line);
        }
#endif
        for (nl=0; nl<cont.nc; nl++) {
            if (nl > 0)
                c_inc = cont.c[nl] - cont.c[nl-1];
            else if (cont.nc > 1)
                c_inc = cont.c[1] - cont.c[0];
            else
                c_inc = 1.0;
            if (cont.relative && cont.zmax != 0.0)
                c_inc /= cont.zmax;
            np = 0;
            for (i=0; i<nX-1; i++) {
                for (j=0; j<nY-1; j++) {
                    n = 0;
                    for (k=0; k<4; k++) {
                        if (L[i+istep[k]][j+jstep[k]]) {
                            arrX[n] = i + istep[k];
                            arrY[n] = j + jstep[k];
                            n++;
                        }
                    }
                    if (n < 3) continue;
                    arrX[n] = arrX[0];
                    arrY[n] = arrY[0];
                    for (k=0; k<=n/2; k+=2) {
                        pair = find_pair(cont.c[nl], A, &arrX[k], &arrY[k], m);
                        if (pair == NULL) continue;
                        P0.x = pair->x[0]; P0.y = pair->y[0];
                        P1.x = pair->x[1]; P1.y = pair->y[1];
                        if (rot) {
                            uvunit2xy(P0, PA, &x1, &y1);
                            uvunit2xy(P1, PA, &x2, &y2);
                        } else if (proj) {
                            ptunit2xy(pair->x[0], pair->y[0], pType, &x1, &y1);
                            ptunit2xy(pair->x[1], pair->y[1], pType, &x2, &y2);
                        } else {
                            x1 = xunit2x(pair->x[0]);
                            x2 = xunit2x(pair->x[1]);
                            y1 = yunit2y(pair->y[0]);
                            y2 = yunit2y(pair->y[1]);
                        }
                        if (x1 == x2 && y1 == y2) continue;
#ifdef HAVE_LIBPGPLOT
                        if (pgplot) {
                             if (rot) {
                                 uv2xy(P0, PA, &p);
                                 pltmp.x[0] = (PLFLT)p.x;
                                 pltmp.y[0] = (PLFLT)p.y;
                                 uv2xy(P1, PA, &p);
                                 pltmp.x[1] = (PLFLT)p.x;
                                 pltmp.y[1] = (PLFLT)p.y;
                             } else if (proj) {
                                 pt2xy(pair->x[0], pair->y[0], pType, &p);
                                 pltmp.x[0] = (PLFLT)p.x;
                                 pltmp.y[0] = (PLFLT)p.y;
                                 pt2xy(pair->x[1], pair->y[1], pType, &p);
                                 pltmp.x[1] = (PLFLT)p.x;
                                 pltmp.y[1] = (PLFLT)p.y;
                             } else {
                                 pltmp.x[0] = (PLFLT) pair->x[0];
                                 pltmp.y[0] = (PLFLT) pair->y[0];
                                 pltmp.x[1] = (PLFLT) pair->x[1];
                                 pltmp.y[1] = (PLFLT) pair->y[1];
                             }
                             pltmp.type = PAIR_UNDEF;
                        }
#endif
                        if (cont.quick == 1) {
                            if (cont.grey) XSetForeground(dpy, l_gc,
                                               GetContourPixel(cont.c[nl]));
                            draw_line(l_gc, x1, y1, x2, y2);
#ifdef HAVE_LIBPGPLOT
                            if (pgplot) {
                                if (cont.grey) cpgsci(GetPGci(cont.c[nl]));
                                cpgline(2, pltmp.x, pltmp.y);
                            }
#endif
                        } else {
                            if (np < MAXPAIRS) {
                                xpair[np].x[0] = x1;
                                xpair[np].x[1] = x2;
                                xpair[np].y[0] = y1;
                                xpair[np].y[1] = y2;
                                xpair[np].type = PAIR_UNDEF;
#ifdef HAVE_LIBPGPLOT
                                if (pgplot) plpair[np] = pltmp;
#endif
                                np++;
                            }
                        }
                    }
                }
            }

            if (cont.quick == 1) {
                if (cont.grey) {
#ifdef HAVE_LIBPGPLOT
                    if (pgplot) cpgsci(1);
#endif
                    XSetForeground(dpy, l_gc, c->black);
                }
                continue;
            }

            for (i=0; i<np; i++) {
                if (xpair[i].type == PAIR_UNDEF) xpair[i].type = PAIR_HEAD;
                j = i + 1;
                while (j < np) {
                    if (xpair[j].type != PAIR_UNDEF) {
                        j++;
                        continue;
                    }
                    if (xpair[i].x[0] == xpair[j].x[1] &&
                        xpair[i].y[0] == xpair[j].y[1]) {
                        xpair[j].type = PAIR_HEAD;
                        xpair[i].type = PAIR_TAIL;
                        tmp = xpair[j];
                        n = j;
                        while (n > i) {
                            xpair[n] = xpair[n-1];
                            n--;
                        }
                        xpair[i] = tmp;
                        j = i+1;
                    } else if (xpair[i].x[0] == xpair[j].x[0] &&
                               xpair[i].y[0] == xpair[j].y[0]) {
                        n = xpair[j].x[0];
                        xpair[j].x[0] = xpair[j].x[1];
                        xpair[j].x[1] = n;
                        n = xpair[j].y[0];
                        xpair[j].y[0] = xpair[j].y[1];
                        xpair[j].y[1] = n;
                        xpair[j].type = PAIR_HEAD;
                        xpair[i].type = PAIR_TAIL;
                        tmp = xpair[j];
                        n = j;
                        while (n > i) {
                            xpair[n] = xpair[n-1];
                            n--;
                        }
                        xpair[i] = tmp;
                        j = i+1;
                    } else if (xpair[i].x[1] == xpair[j].x[0] &&
                               xpair[i].y[1] == xpair[j].y[0]) {
                        xpair[j].type = PAIR_TAIL;
                        tmp = xpair[j];
                        xpair[j] = xpair[i+1];
                        xpair[i+1] = tmp;
                    } else if (xpair[i].x[1] == xpair[j].x[1] &&
                               xpair[i].y[1] == xpair[j].y[1]) {
                        xpair[j].type = PAIR_TAIL;
                        n = xpair[j].x[0];
                        xpair[j].x[0] = xpair[j].x[1];
                        xpair[j].x[1] = n;
                        n = xpair[j].y[0];
                        xpair[j].y[0] = xpair[j].y[1];
                        xpair[j].y[1] = n;
                        tmp = xpair[j];
                        xpair[j] = xpair[i+1];
                        xpair[i+1] = tmp;
                    }
                    j++;
                }
            }

            for (n=0; n<np; n++) {
                if (xpair[n].type == PAIR_HEAD) {
                    not_drawn = 1;
                    i = 0;
                }
                if (cont.grey) XSetForeground(dpy, l_gc,
                                              GetContourPixel(cont.c[nl]));
                x1 = (xpair[n].x[0] + xpair[n].x[1])/2;
                y1 = (xpair[n].y[0] + xpair[n].y[1])/2;
                if (not_drawn && i >= CONT_LABLIMIT*(cont.intpOrder+1) &&
                    point_is_inside(x1, y1)) {
                    draw_numeric_label(l_gc, 'o', x1, y1,
                             (cont.relative && cont.zmax != 0.0) ?
                         GetNumericLabel(cont.c[nl]/cont.zmax, 0, cont.ndigits, 3) :
                         GetNumericLabel(cont.c[nl], 0, cont.ndigits, 3));
                    not_drawn = 0;
                }
                draw_line(l_gc, xpair[n].x[0], xpair[n].y[0],
                                xpair[n].x[1], xpair[n].y[1]);
                i++;
            }

            if (cont.grey)
                XSetForeground(dpy, l_gc, c->black);

            if (!pgplot) continue;

#ifdef HAVE_LIBPGPLOT
            for (i=0; i<np; i++) {
                if (plpair[i].type == PAIR_UNDEF) plpair[i].type = PAIR_HEAD;
                j = i + 1;
                while (j < np) {
                    if (plpair[j].type != PAIR_UNDEF) {
                        j++;
                        continue;
                    }
                    if (PLEQU(plpair[i].x[0], plpair[j].x[1]) &&
                        PLEQU(plpair[i].y[0], plpair[j].y[1])) {
                        plpair[j].type = PAIR_HEAD;
                        plpair[i].type = PAIR_TAIL;
                        pltmp = plpair[j];
                        n = j;
                        while (n > i) {
                            plpair[n] = plpair[n-1];
                            n--;
                        }
                        plpair[i] = pltmp;
                        j = i+1;
                    } else if (PLEQU(plpair[i].x[0], plpair[j].x[0]) &&
                               PLEQU(plpair[i].y[0], plpair[j].y[0])) {
                        f = plpair[j].x[0];
                        plpair[j].x[0] = plpair[j].x[1];
                        plpair[j].x[1] = f;
                        f = plpair[j].y[0];
                        plpair[j].y[0] = plpair[j].y[1];
                        plpair[j].y[1] = f;
                        plpair[j].type = PAIR_HEAD;
                        plpair[i].type = PAIR_TAIL;
                        pltmp = plpair[j];
                        n = j;
                        while (n > i) {
                            plpair[n] = plpair[n-1];
                            n--;
                        }
                        plpair[i] = pltmp;
                        j = i+1;
                    } else if (PLEQU(plpair[i].x[1], plpair[j].x[0]) &&
                               PLEQU(plpair[i].y[1], plpair[j].y[0])) {
                        plpair[j].type = PAIR_TAIL;
                        pltmp = plpair[j];
                        plpair[j] = plpair[i+1];
                        plpair[i+1] = pltmp;
                    } else if (PLEQU(plpair[i].x[1], plpair[j].x[1]) &&
                               PLEQU(plpair[i].y[1], plpair[j].y[1])) {
                        plpair[j].type = PAIR_TAIL;
                        f = plpair[j].x[0];
                        plpair[j].x[0] = plpair[j].x[1];
                        plpair[j].x[1] = f;
                        f = plpair[j].y[0];
                        plpair[j].y[0] = plpair[j].y[1];
                        plpair[j].y[1] = f;
                        pltmp = plpair[j];
                        plpair[j] = plpair[i+1];
                        plpair[i+1] = pltmp;
                    }
                    j++;
                }
            }

            for (n=0; n<np; n++) {
                if (xpair[n].type == PAIR_HEAD) {
                    not_drawn = 1;
                    i = 0;
                }
                if (cont.grey) cpgsci(GetPGci(cont.c[nl]));
                fx[0] = (plpair[n].x[0] + plpair[n].x[1])/2.0;
                fy[0] = (plpair[n].y[0] + plpair[n].y[1])/2.0;
                x1 = xunit2x((double)fx[0]);
                y1 = yunit2y((double)fy[0]);
                if (not_drawn && i >= CONT_LABLIMIT*(cont.intpOrder+1) &&
                    point_is_inside(x1, y1)) {
                    fx[1] = plpair[n].x[0] - plpair[n].x[1];
                    fy[1] = plpair[n].y[1] - plpair[n].y[0];
                    angle = RADTODEG*atan2((double)fy[1], (double)fx[1]);
                    if (angle > 90.0) angle -= 180.0;
                    if (angle < -90.0) angle += 180.0;
                    cpgptxt(fx[0], fy[0], (PLFLT)angle, 0.5,
                            (cont.relative && cont.zmax != 0.0) ?
                      GetNumericLabel(cont.c[nl]/cont.zmax, 0, cont.ndigits, 3) :
                      GetNumericLabel(cont.c[nl], 0, cont.ndigits, 3));
                    not_drawn = 0;
                }
                cpgline(2, plpair[n].x, plpair[n].y);
                i++;
            }
            if (cont.grey) cpgsci(1);
#endif
        } /* End of contour level loop */
    } /* End of cont.quick test */
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) cpgebuf();
#endif

    FreeDoubleArray(A, nX, nY);
    FreeIntArray(L, nX, nY);

    m->xspacing = xs;
    m->yspacing = ys;
}

void FindMapExtent(MAP *m, double *xle, double *xri, double *ylo, double *yup)
{
    int i, j, n, first=1;
    double xmin=0.0, xmax=0.0, ymin=0.0, ymax=0.0, PA=0.0;
    double u, v, u1, u2, v1, v2;
    Point p, r[4];
    
    if (!m) return;
    
    if (rType && m->type == MAP_POSPOS) {
        if (forcePosAngle) {
            PA = posAngle;
        } else {
            PA = m->posAngle;
        }
        for (i=0; i<m->i_no; i++) {
            for (j=0; j<m->j_no; j++) {
                if (m->f[i][j] <= BLANK) continue;
                u = m->xleft  + (double)(i) * m->xspacing;
                v = m->ylower + (double)(j) * m->yspacing;
                u1 = u - m->xspacing/2.0;
                u2 = u + m->xspacing/2.0;
                v1 = v - m->yspacing/2.0;
                v2 = v + m->yspacing/2.0;
                p.x = u1; p.y = v1; uv2xy(p, PA, &r[0]);
                p.x = u1; p.y = v2; uv2xy(p, PA, &r[1]);
                p.x = u2; p.y = v2; uv2xy(p, PA, &r[2]);
                p.x = u2; p.y = v1; uv2xy(p, PA, &r[3]);
                for (n=0; n<4; n++) {
                    if (first) {
                        xmin = xmax = r[n].x;
                        ymin = ymax = r[n].y;
                        first = 0;
                    } else {
                        if (r[n].x < xmin) xmin = r[n].x;
                        if (r[n].x > xmax) xmax = r[n].x;
                        if (r[n].y < ymin) ymin = r[n].y;
                        if (r[n].y > ymax) ymax = r[n].y;
                    }
                }
            }
        }
        if (xle) *xle = xmax;
        if (xri) *xri = xmin;
        if (ylo) *ylo = ymin;
        if (yup) *yup = ymax;
    } else {
        if (xle) *xle = m->xleft  - m->xspacing/2.0;
        if (xri) *xri = m->xright + m->xspacing/2.0;
        if (ylo) *ylo = m->ylower - m->yspacing/2.0;
        if (yup) *yup = m->yupper + m->yspacing/2.0;
    }
}

double xmap(scanPtr s)
{
    if (!s) return UNDEF;

    if (cType == COORD_TYPE_EQU || cType == COORD_TYPE_GAL) {
        return s->xoffset;
    } else if (cType == COORD_TYPE_HOR) {
        return s->aoffset;
    } else
        return UNDEF;
}

double ymap(scanPtr s)
{
    if (!s) return UNDEF;

    if (cType == COORD_TYPE_EQU || cType == COORD_TYPE_GAL) {
        return s->yoffset;
    } else if (cType == COORD_TYPE_HOR) {
        return s->eoffset;
    } else
        return UNDEF;
}

double zmap(scanPtr s)
{
    double c1, c2;
    
    double SpecUnitConv();
    
    if (!s) return UNDEF;
    
    if (zType == ZTYPE_MOMENT)
        return s->mom.iint;
    else if (zType == ZTYPE_MEAN)
        return s->mom.mean;
    else if (zType == ZTYPE_GAMP) {
        if (!(s->gaussFit)) return UNDEF;
        return s->g.amp;
    } else if (zType == ZTYPE_GCEN) {
         if (!(s->gaussFit)) return UNDEF;
         return SpecUnitConv(UNIT_VEL, UNIT_CHA, s->g.cen);
    } else if (zType == ZTYPE_GWID) {
        if (!(s->gaussFit)) return UNDEF;
        c1 = s->g.cen - s->g.wid/2.0;
        c2 = s->g.cen + s->g.wid/2.0;
        return fabs(SpecUnitConv(UNIT_VEL, UNIT_CHA, c2)-
                    SpecUnitConv(UNIT_VEL, UNIT_CHA, c1));
    } else if (zType == ZTYPE_POL0)
        return s->coeffs[0];
    else if (zType == ZTYPE_POL1)
        return s->coeffs[1];
    else if (zType == ZTYPE_VCENT)
        return s->mom.vcent;
    else if (zType == ZTYPE_V2MOM)
        return s->mom.v2mom;
    else if (zType == ZTYPE_TMAX)
        return s->mom.TMax;
    else if (zType == ZTYPE_TMIN)
        return s->mom.TMin;
    else if (zType == ZTYPE_XTMAX)
        return s->mom.xTMax;
    else if (zType == ZTYPE_XTMIN)
        return s->mom.xTMin;
    else if (zType == ZTYPE_TRMS)
        return s->mom.sigma;
    else
        return UNDEF;
}

void GetCurrentXUnitStr(char *unit)
{
    if (!unit) return;
    
    if (vP->xunit == UNIT_FRE) 
        strcpy(unit, "GHz");
    else if (vP->xunit == UNIT_VEL)
        strcpy(unit, "km/s");
    else if (vP->xunit == UNIT_CHA)
        strcpy(unit, "channel");
    else
        strcpy(unit, "MHz");
}

char *GetZType(char *unit)
{
    static string label;

    switch (zType) {
        case ZTYPE_MOMENT:
            if (unit) strcpy(unit, "K km/s");
            strcpy(label, "Integrated intensity");
            break;
        case ZTYPE_MEAN:
            if (unit) strcpy(unit, "K");
            strcpy(label, "Mean intensity");
            break;
        case ZTYPE_GAMP:
            if (unit) strcpy(unit, "K");
            strcpy(label, "Amplitude");
            break;
        case ZTYPE_GCEN:
            if (unit) strcpy(unit, "km/s");
            strcpy(label, "Centre vel.");
            break;
        case ZTYPE_GWID:
            if (unit) strcpy(unit, "km/s");
            strcpy(label, "Vel. width");
            break;
        case ZTYPE_POL0:
            if (unit) strcpy(unit, "K");
            strcpy(label, "Fitted offset");
            break;
        case ZTYPE_POL1:
            if (unit) strcpy(unit, "");
            strcpy(label, "Fitted slope");
            break;
        case ZTYPE_VCENT:
            if (unit) strcpy(unit, "km/s");
            strcpy(label, "Centroid vel.");
            break;
        case ZTYPE_V2MOM:
            if (unit) strcpy(unit, "km/s");
            strcpy(label, "2nd vel. mom.");
            break;
        case ZTYPE_TMAX:
            if (unit) strcpy(unit, "K");
            strcpy(label, "Maximum intensity");
            break;
        case ZTYPE_TMIN:
            if (unit) strcpy(unit, "K");
            strcpy(label, "Minimum intensity");
            break;
        case ZTYPE_XTMIN:
            if (unit) GetCurrentXUnitStr(unit);
            strcpy(label, "x unit at minimum intensity");
            break;
        case ZTYPE_XTMAX:
            if (unit) GetCurrentXUnitStr(unit);
            strcpy(label, "x unit at maximum intensity");
            break;
        case ZTYPE_TRMS:
            if (unit) strcpy(unit, "K");
            strcpy(label, "Intensity RMS");
            break;
        default:
            if (unit) strcpy(unit, "");
            strcpy(label, "");
            break;
    }
    return label;
}

double emap(scanPtr s)
{
    double c1, c2;
    
    double SpecUnitConv();
    
    if (!s) return UNDEF;
    
    if (zType == ZTYPE_MOMENT)
        return s->mom.iunc;
    else if (zType == ZTYPE_MEAN) {
        if (s->mom.iint != 0.0)
            return s->mom.iunc*s->mom.mean/s->mom.iint;
        else
            return s->mom.sigma/sqrt((double)s->mom.nchan);
    } else if (zType == ZTYPE_GAMP)
        return s->g.uamp;
    else if (zType == ZTYPE_GCEN) {
        c1 = s->g.cen-s->g.ucen/2.0;
        c2 = s->g.cen+s->g.ucen/2.0;
        return fabs(SpecUnitConv(UNIT_VEL, UNIT_CHA, c2)-
                    SpecUnitConv(UNIT_VEL, UNIT_CHA, c1));
    } else if (zType == ZTYPE_GWID) {
        c1 = s->g.wid-s->g.uwid/2.0;
        c2 = s->g.wid+s->g.uwid/2.0;
        return fabs(SpecUnitConv(UNIT_VEL, UNIT_CHA, c2)-
                    SpecUnitConv(UNIT_VEL, UNIT_CHA, c1));
    } else if (zType == ZTYPE_POL0)  /* Not correct */
        return s->mom.sigma;
    else if (zType == ZTYPE_POL1)  /* Not correct */
        return s->mom.sigma;
    else if (zType == ZTYPE_VCENT)
        return s->mom.ucent;
    else if (zType == ZTYPE_V2MOM)
        return s->mom.u2mom;
    else if (zType == ZTYPE_TMAX)
        return s->mom.sigma;
    else if (zType == ZTYPE_TMIN)
        return s->mom.sigma;
    else if (zType == ZTYPE_XTMAX)
        return 0.0;
    else if (zType == ZTYPE_XTMIN)
        return 0.0;
    else if (zType == ZTYPE_TRMS)
        return 0.0;
    else
        return 0.0;
}

void set_contour_levels(double zmin, double zmax, int nc, int spacing,
                        double pexp)
{
    int n;
    double range;

    if (nc >= MAXCONTOURS || nc < 0 || vP->fixed_z) return;

    if (zmin == zmax) {
        zmax = cont.zmax;
    }

    if (zmin > zmax) {
        range = zmin;
        zmin = zmax;
        zmax = range;
    }

    if (cont.minmax == CONT_NULLMAX) {
        zmin = 0.0;
    }
    
    if (pexp <= 0.0) pexp = 1.0;

    if (nc == 0) {
        cont.nc = 0;
        return;
    } else if (nc == 1) {
        cont.nc = 1;
        cont.spacing = spacing;
        cont.c[0] = zmin;
        cont.pexp = pexp;
        return;
    }

    if (zmin == zmax) return;

    if (spacing == CONT_LINEAR) {
        range = (zmax-zmin)/(double)(nc-1);
    } else if (spacing == CONT_LOGARITHMIC) {
        range = log10(zmax-zmin)/(double)(nc-1);
    } else if (spacing == CONT_EXPONENTIAL) {
        range = pow(10.0, zmax-zmin)/(double)(nc-1);
    } else if (spacing == CONT_SQUAREROOT) {
        range = sqrt(zmax-zmin)/(double)(nc-1);
    } else if (spacing == CONT_QUADRATIC) {
        range = pow(zmax-zmin, 2.0)/(double)(nc-1);
    } else if (spacing == CONT_VARYING) {
        range = pow(zmax-zmin, pexp)/(double)(nc-1);
    } else {
        return;
    }

    cont.c[0] = zmin;
    for (n=1; n<nc; n++) {
        if (spacing == CONT_LINEAR)
            cont.c[n] = zmin + (double)n*range;
        else if (spacing == CONT_LOGARITHMIC)
            cont.c[n] = zmin + pow(10.0, (double)n*range);
        else if (spacing == CONT_EXPONENTIAL)
            cont.c[n] = zmin + log10((double)n*range);
        else if (spacing == CONT_SQUAREROOT)
            cont.c[n] = zmin + pow((double)n*range, 2.0);
        else if (spacing == CONT_QUADRATIC)
            cont.c[n] = zmin + sqrt((double)n*range);
        else if (spacing == CONT_VARYING)
            cont.c[n] = zmin + pow((double)n*range, 1./pexp);
    }

    cont.nc = nc;
    cont.spacing = spacing;
    cont.pexp = pexp;
}

static void GetContourStrs(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int nc=1;
    double first, last, step;
    string buf;
    
    void wdscanf(), send_line();
    
    wdscanf(sf->edit[0], &first);
    wdscanf(sf->edit[1], &last);
    wdscanf(sf->edit[2], &step);
    
    if (cont.relative && cont.zmax != 0.0) {
        first *= cont.zmax;
        last  *= cont.zmax;
        step  *= cont.zmax;
    }
    
    if (last < first) {
        sprintf(buf, "Invalid contour levels: first > last (%f > %f).",
                first, last);
        send_line(buf);
        PostErrorDialog(w, buf);
        return;
    } else if (last == first) {
        nc = 1;
    } else if (step > 0.0) {
        nc = 1 + (int)((last - first)/step + 0.5);
    } else {
        sprintf(buf, "Nonpositive increment: %f.", step);
        send_line(buf);
        PostErrorDialog(w, buf);
        return;
    }
    
    set_contour_levels(first, last, nc, CONT_LINEAR, 0.0);
    
    if (cont.nc <= 0) {
        first = 0.0;
        last = 10.0;
        step = 1.0;
    } else if (cont.nc == 1) {
        first = last = cont.c[0];
        step = 0.0;
    } else {
        first = cont.c[0];
        last = cont.c[cont.nc - 1];
        step = (last - first)/(double)(cont.nc - 1);
    }
    
    if (cont.relative && cont.zmax != 0.0) {
        first /= cont.zmax;
        last  /= cont.zmax;
        step  /= cont.zmax;
    }

    wprintf(sf->edit[0], "%f", first);
    wprintf(sf->edit[1], "%f", last);
    wprintf(sf->edit[2], "%f", step);
    wprintf(sf->edit[3], "No of contours: %d", cont.nc);
    wprintf(sf->edit[4], "Min map value: %f", cont.zmin);
    wprintf(sf->edit[5], "Max map value: %f", cont.zmax);

    if (cont.quick && ANYMAP(vP->mode))
        draw_main();
}

static char *GetNumericContourString()
{
    int n;
    string tmp;
    char *str;
    
    str = (char *)malloc(20*MAXCONTOURS*sizeof(char));
    if (!str) return NULL;
    
    strcpy(str, "<none>");
    
    for (n=0; n<cont.nc; n++) {
        sprintf(tmp, "%2d: %f\n", n+1, (cont.relative && cont.zmax != 0.0) ?
                cont.c[n]/cont.zmax : cont.c[n]);
        if (n == 0)
            strcpy(str, tmp);
        else
            strcat(str, tmp);
    }
    
    return str;
}

static void GetNumLevCallback(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int n, m, k;
    string txt;
    double level, value;
    char *p;
    
    void wsscanf();
    
    if (!sf) return;
    
    wsscanf(sf->edit[0], txt);
    
    n = 0;
    p = strtok(txt, "\n");
    while (p && n < MAXCONTOURS) {
        m = sscanf(p, "%lf: %lf", &level, &value);
        if (m == 2) {
            k = (int)(level + 0.5) - 1;
            if (k >= 0 && k < MAXCONTOURS) {
                cont.c[k] = (cont.relative && cont.zmax != 0.0) ?
                            value * cont.zmax : value;
            }
        } else if (m == 1) {
            cont.c[n] = (cont.relative && cont.zmax != 0.0) ?
                        atof(p) * cont.zmax : atof(p);
        } else {
            p = strtok(NULL, "\n");
            continue;
        }
        p = strtok(NULL, "\n");
        n++;
    }
    
    cont.nc = n;
    
    if (cont.quick && ANYMAP(vP->mode))
        draw_main();
}

void PostManualContourDialog(Widget wid, char *cmd, XtPointer call_data)
{
    Widget w = wid;
    StdForm *sf;
    char *txt;
    
    while (!XtIsWMShell(w))
        w = XtParent(w);
                
    sf = PostStdFormDialog(w, "Manual contours",
             BUTT_APPLY, (XtCallbackProc)GetNumLevCallback, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 1, NULL);
    
    txt = GetNumericContourString();
    
    sf->edit[0] = XtVaCreateManagedWidget("text", xmTextWidgetClass, sf->form,
                                   XmNeditMode, XmMULTI_LINE_EDIT,
                                   XmNcolumns, 25,
                                   XmNrows, (cont.nc > 15) ? cont.nc : 15,
                                   XmNvalue, txt,
                                   NULL);
                          
    if (txt) free(txt);

    ArrangeStdFormDialog(sf,  sf->edit[0]);
                
    ManageDialogCenteredOnPointer(sf->form);
}

void PostRangeStepContourDialog(Widget wid, char *cmd, XtPointer call_data)
{
    double first, last, step;
    Widget w = wid, fr, rc, rc1, rc2;
    StdForm *sf;
    
    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Contour range and increment",
             BUTT_APPLY, (XtCallbackProc)GetContourStrs, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 6, NULL);
    
    rc    = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                    XmNorientation, XmVERTICAL,
                                    NULL);
    
    fr    = XtVaCreateWidget("frame", xmFrameWidgetClass,
				             rc, XmNshadowType, XmSHADOW_OUT, NULL);
    rc1   = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                    XmNorientation, XmVERTICAL,
                                    NULL);
    XtCreateManagedWidget("First contour", xmLabelWidgetClass,
                                  rc1, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("firstc", xmTextWidgetClass,
                                  rc1, NULL, 0);
    XtCreateManagedWidget("Last contour", xmLabelWidgetClass,
                                  rc1, NULL, 0);
    sf->edit[1] = XtCreateManagedWidget("lastc", xmTextWidgetClass,
                                  rc1, NULL, 0);
    XtCreateManagedWidget("Contour increment", xmLabelWidgetClass,
                                  rc1, NULL, 0);
    sf->edit[2] = XtCreateManagedWidget("cincrement", xmTextWidgetClass,
                                  rc1, NULL, 0);

    rc2   = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                    XmNorientation, XmVERTICAL,
                                    NULL);
    sf->edit[3] = XtCreateManagedWidget("", xmLabelWidgetClass,
                                  rc2, NULL, 0);
    sf->edit[4] = XtCreateManagedWidget("", xmLabelWidgetClass,
                                  rc2, NULL, 0);
    sf->edit[5] = XtCreateManagedWidget("", xmLabelWidgetClass,
                                  rc2, NULL, 0);

    ArrangeStdFormDialog(sf, rc);
  
    if (cont.nc <= 0) {
        first = 0.0;
        last = 10.0;
        step = 1.0;
    } else if (cont.nc == 1) {
        first = last = cont.c[0];
        step = 0.0;
    } else {
        first = cont.c[0];
        last = cont.c[cont.nc - 1];
        step = (last - first)/(double)(cont.nc - 1);
    }
    
    if (cont.relative && cont.zmax != 0.0) {
        first /= cont.zmax;
        last  /= cont.zmax;
        step  /= cont.zmax;
    }
    
    wprintf(sf->edit[0], "%f", first);
    wprintf(sf->edit[1], "%f", last);
    wprintf(sf->edit[2], "%f", step);
    wprintf(sf->edit[3], "No of contours: %d", cont.nc);
    wprintf(sf->edit[4], "Min map value: %f", cont.zmin);
    wprintf(sf->edit[5], "Max map value: %f", cont.zmax);
    
    XtManageChild(fr);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void SetSubMapValues(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int nx, n=0;
    int mx, my;
    double x, y;
    string str;
    list curr = NULL;
    scanPtr s;
    
    void wiscanf();
    void wdscanf();
    
    if (!sf) return;
    
    wiscanf(sf->edit[0], &nx);
    wiscanf(sf->edit[1], &mx);
    wiscanf(sf->edit[2], &my);
    wdscanf(sf->edit[3], &x);
    wdscanf(sf->edit[4], &y);
    
    if (nx < 0) {
        sprintf(str, "Size (nX=%d) must be greater than 0.", nx);
        PostErrorDialog(w, str);
        return;
    }
/*     if (mx < 0) {
        sprintf(str, "X margin value (%d) must be >= 0.", mx);
        PostErrorDialog(w, str);
        return;
    }
    if (my < 0) {
        sprintf(str, "Y margin value (%d) must be >= 0.", my);
        PostErrorDialog(w, str);
        return;
    } */
    
    if (x <= 0.0) {
        sprintf(str, "X magnification value (%f) must be > 0.", x);
        PostErrorDialog(w, str);
    }
    if (y <= 0.0) {
        sprintf(str, "Y magnification value (%f) must be > 0.", y);
        PostErrorDialog(w, str);
    }
    
    if (vP->mode == SHOW_ALLSPE) {
        if (nx != 0 && vP->from->sequence) {
            while ( (curr = scan_iterator(curr, vP->from)) ) {
                s = (scanPtr)DATA(curr);
                s->xoffset = -(double)(n % nx);
                s->yoffset = -(double)(n / nx);
                n++;
            }
        }
        n = count_scans(vP->from);
    } else if (vP->mode == SHOW_SCATTER) {
        n = vP->nScat;
    } else {
        n = vP->nMaps;
    }
    vP->Nx = nx;
    if (nx == 0)
        vP->Ny = 0;
    else
        vP->Ny = (n-1)/nx + 1;
    
    vP->subXmarg = mx;
    vP->subYmarg = my;
    vP->subXmagn = x;
    vP->subYmagn = y;
    
    if (vP->mode == SHOW_ALLSPE) obtain_map_info(NULL, "map", NULL);
    
    redraw_graph(gp->top, "update", NULL);
}

static void UpdateMagnifyEdits(Widget *x, Widget *y)
{
    static Widget *xEdit;
    static Widget *yEdit;
    
    if (x) xEdit = x;
    if (y) yEdit = y;
    
    if (xEdit) wprintf(*xEdit, "%f", vP->subXmagn);
    if (yEdit) wprintf(*yEdit, "%f", vP->subYmagn);
}

static void set_view_type(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int type = atoi(str);
    
    if (type != vP->special_view_type) {
        vP->special_view_type = type;
        obtain_map_info(NULL, "map", NULL);
        if (vP->mode == SHOW_ALLSPE)
            redraw_graph(gp->top, "update", NULL);
    }
}

static void MagnXCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int m = vP->mode;
    double f = atof(str);
    
    if (f != vP->subXmagn) {
        vP->subXmagn = f;
        UpdateMagnifyEdits(NULL, NULL);
        obtain_map_info(NULL, "map", NULL);
        if (m == SHOW_ALLSPE)
            redraw_graph(gp->top, "update", NULL);
    }
}

static void MagnYCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int m = vP->mode;
    double f = atof(str);
    
    if (f != vP->subYmagn) {
        vP->subYmagn = f;
        UpdateMagnifyEdits(NULL, NULL);
        obtain_map_info(NULL, "map", NULL);
        if (m == SHOW_ALLSPE)
            redraw_graph(gp->top, "update", NULL);
    }
}

void PostSubMapDialog(Widget wid, char *cmd, XtPointer call_data)
{
    int n;
    Widget w = wid, rc, rcx, rcy, frx, fry;
    Widget menuV, menuX, menuY;
    string str;
    StdForm *sf;
    Arg wargs[10];
    
    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Multiple plot size",
             BUTT_APPLY,(XtCallbackProc)SetSubMapValues, NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, (XtPointer)IntpOrder_Help, 5, NULL);
    
    n = 0;
    XtSetArg(wargs[n], XmNorientation,       XmVERTICAL); n++;
    XtSetArg(wargs[n], XmNnumColumns,        1); n++;
    XtSetArg(wargs[n], XmNadjustLast,        FALSE); n++;
    XtSetArg(wargs[n], XmNpacking,           XmPACK_TIGHT); n++;
    rc = XtCreateManagedWidget("rowcol", xmRowColumnWidgetClass,
                               sf->form, wargs, n);
    
    XtCreateManagedWidget("No. of plots in x-direction",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("Nx", xmTextWidgetClass, rc, NULL, 0);
    
    XtCreateManagedWidget("Margin in x-direction:",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[1] = XtCreateManagedWidget("xmargin", xmTextWidgetClass, rc, NULL, 0);
    
    XtCreateManagedWidget("Margin in y-direction:",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[2] = XtCreateManagedWidget("ymargin", xmTextWidgetClass, rc, NULL, 0);
    
    menuV = CreateOptionMenu(rc, &SpecialViewTypeMenu);
    SetDefaultOptionMenuItem(menuV, vP->special_view_type);

    frx = XtVaCreateWidget("frame", xmFrameWidgetClass,
				           rc, XmNshadowType, XmSHADOW_ETCHED_IN, NULL);
    n = 0;
    XtSetArg(wargs[n], XmNorientation,       XmVERTICAL); n++;
    XtSetArg(wargs[n], XmNnumColumns,        1); n++;
    XtSetArg(wargs[n], XmNadjustLast,        FALSE); n++;
    XtSetArg(wargs[n], XmNpacking,           XmPACK_TIGHT); n++;
    rcx = XtCreateManagedWidget("rowcol", xmRowColumnWidgetClass,
                                frx, wargs, n);

    menuX = CreateOptionMenu(rcx, &MagnXMenu);
    SetDefaultOptionMenuItem(menuX, 1);
    sprintf(str, "%.1f", vP->subXmagn);
    SetDefaultOptionMenuItemString(menuX, MagnXData, str);
    
    XtCreateManagedWidget("Magnification factor in x-direction:",
                          xmLabelWidgetClass, rcx, NULL, 0);
    sf->edit[3] = XtCreateManagedWidget("xmagn", xmTextWidgetClass, rcx, NULL, 0);
    
    fry = XtVaCreateWidget("frame", xmFrameWidgetClass,
				           rc, XmNshadowType, XmSHADOW_ETCHED_IN, NULL);
    n = 0;
    XtSetArg(wargs[n], XmNorientation,       XmVERTICAL); n++;
    XtSetArg(wargs[n], XmNnumColumns,        1); n++;
    XtSetArg(wargs[n], XmNadjustLast,        FALSE); n++;
    XtSetArg(wargs[n], XmNpacking,           XmPACK_TIGHT); n++;
    rcy = XtCreateManagedWidget("rowcol", xmRowColumnWidgetClass,
                                fry, wargs, n);
    
    menuY = CreateOptionMenu(rcy, &MagnYMenu);
    SetDefaultOptionMenuItem(menuY, 1);
    sprintf(str, "%.1f", vP->subYmagn);
    SetDefaultOptionMenuItemString(menuY, MagnYData, str);
    
    XtCreateManagedWidget("Magnification factor in y-direction:",
                          xmLabelWidgetClass, rcy, NULL, 0);
    sf->edit[4] = XtCreateManagedWidget("ymagn", xmTextWidgetClass, rcy, NULL, 0);
    
    ArrangeStdFormDialog(sf, rc);
    
    wprintf(sf->edit[0], "%d", vP->Nx);
    wprintf(sf->edit[1], "%d", vP->subXmarg);
    wprintf(sf->edit[2], "%d", vP->subYmarg);
    
    UpdateMagnifyEdits(&(sf->edit[3]), &(sf->edit[4]));
    
    XtManageChild(menuV);
    XtManageChild(menuX);
    XtManageChild(menuY);
    XtManageChild(frx);
    XtManageChild(fry);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void set_type_of_cont_mode(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int i;

    for (i=0; i<3; i++) {
        if (strcmp(str, radio_cont_mode[i]) == 0) {
            cont.quick = i;
            if (ANYMAP(vP->mode))
                draw_main();
            break;
        }
    }
}

static void set_type_of_grey_mode(Widget w, char *str,
                                  XmToggleButtonCallbackStruct *cb)
{
    int i;

    for (i=0; i<3; i++) {
        if (strcmp(str, radio_grey_mode[i]) == 0) {
            cont.grey = i;
            if (ANYMAP(vP->mode))
                draw_main();
            break;
        }
    }
}

static void set_relative_levels(Widget w, char *str,
                                XmToggleButtonCallbackStruct *cb)
{
    if (cb->set) {
        cont.relative = 1;
    } else {
        cont.relative = 0;
    }
    if (cont.quick && ANYMAP(vP->mode))
        draw_main();
}

#if XmVersion >= 2000
static void set_no_of_contours(Widget w, char *txt,
                                XmSpinBoxCallbackStruct *cb)
{
    if (cont.nc == cb->position + 1) return;
    set_contour_levels(cont.zmin, cont.zmax, cb->position + 1,
                       cont.spacing, cont.pexp);
    
    if (cont.quick && ANYMAP(vP->mode))
        draw_main();
}
#endif

static void set_grey_inverted(Widget w, char *str,
                              XmToggleButtonCallbackStruct *cb)
{
    if (cb->set) {
        cont.grey_inverted = 1;
    } else {
        cont.grey_inverted = 0;
    }
    if (cont.grey && ANYMAP(vP->mode))
        draw_main();
}

static void set_wedge(Widget w, char *str, XmToggleButtonCallbackStruct *cb)
{
    if (cb->set) {
        draw.wedge = 1;
    } else {
        draw.wedge = 0;
    }
    if (cont.grey && ANYMAP(vP->mode))
        draw_main();
}

static void set_rotate(Widget w, char *str, XmToggleButtonCallbackStruct *cb)
{
    if (cb->set) {
        rType = 1;
    } else {
        rType = 0;
    }
    if (vP->mode == SHOW_POSPOS)
        draw_main();
}

static void set_blank_inside(Widget w, char *str,
                             XmToggleButtonCallbackStruct *cb)
{
    if (cb->set) {
        cont.blank = 1;
    } else {
        cont.blank = 0;
    }
    if (ANYMAP(vP->mode))
        draw_main();
}

static void set_slider_values(Widget w, int i)
{
    int pos=0;

    if (!w) return;

    if (i==0) {
        pos = 0;
        if (cont.zmax != 0.0)
            pos = NINT((double)slider_100[0]*cont.zmin/cont.zmax);
        if (pos < slider_min[0]) pos = slider_min[0]; 
    } else if (i==1) {
        pos = slider_100[1];
    } else if (i==2) {
        pos = cont.nc;
        if (pos == 0) pos = 1;
    } else if (i==3) {
        pos = 0;
    }
    XtVaSetValues(w, XmNvalue, pos, NULL);
}

static void slider_moved(Widget w, char *s, XmScaleCallbackStruct *cb)
{
    static int last_val[5]={UNDEF, UNDEF, UNDEF, UNDEF, UNDEF};
    int i, nc, val, sp;
    double z1, z2, pexp;
    double v100 = (double)slider_100[1];

/*
    if (cont.grey == 1 && call_data->reason == XmCR_DRAG) return;
 */

    for (i=0; i<4; i++) {
        if (strcmp(s, slider_labels[i]) == 0) {
            val = cb->value;
            if (val == last_val[i]) return;
            last_val[i] = val;
            nc = cont.nc;
            sp = cont.spacing;
            z1 = cont.c[0];
            z2 = cont.c[nc-1];
            pexp = cont.pexp;
            if (i==0) {
                z1 = 0.0 + (double)val*(cont.zmax)/v100;
                if (z1 >= z2) {
                    XtVaSetValues(slider[1], XmNvalue, val+1, NULL);
                    z2 = 0.0 + (double)(val+1)*(cont.zmax)/v100;
                }
            } else if (i==1) {
                z2 = 0.0 + (double)val*(cont.zmax)/v100;
                if (z2 <= z1) {
                    XtVaSetValues(slider[0], XmNvalue, val-1, NULL);
                    z1 = 0.0 + (double)(val-1)*(cont.zmax)/v100;
                }
            } else if (i==2) {
                nc = val;
            } else if (i==3) {
                pexp = exp(-(double)val/15.0);
            }
            set_contour_levels(z1, z2, nc, sp, pexp);
            draw_main();
            break;
        }
    }
}

static Widget create_control(Widget parent, char *name, char *label,
                             int min, int max, char *str, int dec)
{
    int n;
    Arg wargs[10];
    Widget w;
    XmString xstr = MKSTRING(label);

    n = 0;
    XtSetArg(wargs[n], XmNminimum, min); n++;
    XtSetArg(wargs[n], XmNmaximum, max); n++;
    XtSetArg(wargs[n], XmNtitleString, xstr); n++;
    XtSetArg(wargs[n], XmNshowValue, True); n++;
    if (dec > 0) {
        XtSetArg(wargs[n], XmNdecimalPoints, dec); n++;
    }
    XtSetArg(wargs[n], XmNorientation, XmHORIZONTAL); n++;
    XtSetArg(wargs[n], XmNscaleWidth, 500); n++;
    w = XtCreateManagedWidget(name, xmScaleWidgetClass, parent, wargs, n);

    XtAddCallback(w, XmNvalueChangedCallback,
                  (XtCallbackProc)slider_moved, str);
    XtAddCallback(w, XmNdragCallback,
                  (XtCallbackProc)slider_moved, str);

    XmStringFree(xstr);

    return (w);
}

static Widget make_controller(char *name, int min, int max, Widget parent,
                              char *str, int dec)
{
    Widget rc, w;

    rc = XtCreateManagedWidget("rc", xmRowColumnWidgetClass, parent, NULL, 0);
    w = create_control(rc, "control", name, min, max, str, dec);
    
    return (w);
}

void LoadMapInfo(FDATA *fd, MAP *m)
{
    strcpy(fd->sname, m->name);
    strcpy(fd->molecule, m->molecule);
    fd->date = m->date;
    
    fd->nX = m->i_no;
    fd->nY = m->j_no;
    fd->data = m->d;
    fd->flag = m->f;
    fd->sp = m->sp;
    fd->x0 = m->x0;
    fd->y0 = m->y0;
    fd->epoch = m->epoch;
    fd->equinox = m->equinox;
    fd->xspa = m->xspacing;
    fd->yspa = m->yspacing;
    fd->posang = m->posAngle/RADTODEG;
    fd->b = m->b;
    fd->coordType = m->coordType;

    if (fd->xspa != 0.0)
        fd->refX = 1.0 + (double)NINT(-m->xleft/fd->xspa);
    else
        fd->refX = 1.0;

    if (fd->yspa != 0.0)
        fd->refY = 1.0 + (double)NINT(-m->ylower/fd->yspa);
    else
        fd->refY = 1.0;
}

void SetMapInfo(FDATA *fd)
{
    SetViewMode(vP->mode, vP->s, &Map, vP->p); 
    obtain_map_info(NULL, "map", NULL);
    LoadMapInfo(fd, vP->m);
}

int *GetMapDotSize()
{
    return &(cont.dot_size);
}

int *GetMapDotType()
{
    return &(cont.markers);
}

int GetIntpType()
{
    return cont.intpType;
}

int GetIntpOrder()
{
    return cont.intpOrder;
}

int GetIntpCorners()
{
    return cont.nCorners;
}

void SetIntpOrder(int order)
{
    cont.intpOrder = order;
}

void PostInterpolationDialog(Widget wid, char *cmd, XtPointer cd)
{
    Widget rc, fr, menuT, menuO, menuF, w = wid;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Interpolation options",
             NULL, NULL, NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, (XtPointer)IntpOrder_Help, 0, NULL);
    
    fr  = XtVaCreateWidget("frame", xmFrameWidgetClass,
				           sf->form, XmNshadowType, XmSHADOW_IN, NULL);
    
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, fr,
                          XmNorientation, XmVERTICAL,
                          XmNnumColumns, 3, NULL);
        
    menuO = CreateOptionMenu(rc, &IntpOrderMenu);
    SetDefaultOptionMenuItem(menuO, cont.intpOrder);
    
    menuT = CreateOptionMenu(rc, &IntpTypeMenu);
    SetDefaultOptionMenuItem(menuT, cont.intpType);

    menuF = CreateOptionMenu(rc, &IntpFillMenu);
    SetDefaultOptionMenuItem(menuF, cont.nCorners);
    
    ArrangeStdFormDialog(sf, fr);

    XtManageChild(menuO);
    XtManageChild(menuT);
    XtManageChild(menuF);
    XtManageChild(rc);
    XtManageChild(fr);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void IntpOrderCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str), m = vP->mode;
    
    if (n != cont.intpOrder) {
        cont.intpOrder = n;
        if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL)
            draw_main();
    }
}

static void IntpTypeCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str), m = vP->mode;
    
    if (n != cont.intpType) {
        cont.intpType = n;
        if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL)
            draw_main();
    }
}

static void IntpFillCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str), m = vP->mode;
    
    if (n != cont.nCorners) {
        cont.nCorners = n;
        if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL)
            draw_main();
    }
}

static void ContSpacingCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str), m = vP->mode;
    
    if (n != cont.spacing) {
        cont.spacing = n;
        set_contour_levels(cont.zmin, cont.zmax, cont.nc, n, cont.pexp);
        if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL)
            draw_main();
    }
}

static void ContMinMaxCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str), m = vP->mode;
    
    if (n != cont.minmax) {
        cont.minmax = n;
        set_contour_levels(cont.zmin, cont.zmax, cont.nc,
                           cont.spacing, cont.pexp);
        if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL)
            draw_main();
    }
}

static void NDigitsCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str), m = vP->mode;
    
    if (n != cont.ndigits) {
        cont.ndigits = n;
        if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL)
            draw_main();
    }
}

static void WedgeCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str), m = vP->mode;
    
    if (n != draw.wedgepos) {
        draw.wedgepos = n;
        update_map_data();
        if (m == SHOW_POSPOS)
            draw_main();
    }
}

static void BeamCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str), m = vP->mode;
    
    if (n != draw.beam) {
        draw.beam = n;
        update_map_data();
        if (m == SHOW_POSPOS)
            draw_main();
    }
}

static void ProjCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str), m = vP->mode;
    
    if (n != pType) {
        pType = n;
        if (m == SHOW_POSPOS)
            draw_main();
    }
}

static void cancel_contour_dialog(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    XtDestroyWidget(sf->form);
    slider[4] = NULL;
}

void PostContourDialog(Widget wid, char *cmd, XtPointer cd)
{
    int i, n;
    Widget rc, rc1, rc1_l, rc2, rc2M, rc2R, rc4, fr1, fr2, fr4;
    Widget intpButt, invButt, wedgeButt, rotButt, menuBeam;
    Widget menuSpacing, menuMinMax, relButt, menuProj, menuDigit, menuWedge;
    Widget blankButt;
    Widget rbQuick, rbShade;
    Widget quickButt[3], shadeButt[3];
    Widget w = wid;
    StdForm *sf;
    Arg wargs[10];
#if XmVersion >= 2000
    Widget sb_rc, sb, sb_txt;
#endif

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Contour Selection",
             NULL, NULL, NULL,
             BUTT_CANCEL, (XtCallbackProc)cancel_contour_dialog, NULL,
             BUTT_HELP, NULL, (XtPointer)ContSel_Help, 0, NULL);

#if XmVersion >= 2000
    rc = XtVaCreateManagedWidget("rowcol", xmNotebookWidgetClass,
                                 sf->form,
                                 XmNorientation, XmVERTICAL,
                                 XmNbackPagePlacement, XmTOP_RIGHT,
                                 XmNbindingType, XmNONE,
                                 NULL);
#else
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass,
                                 sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
#endif
    
    fr1  = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rc,
                                   XmNshadowType, XmSHADOW_ETCHED_IN, NULL);
#if XmVersion >= 1200
    XtVaCreateManagedWidget(" Contour level options ",
                            xmLabelWidgetClass, fr1,
#if XmVersion >= 2000
                            XmNframeChildType, XmFRAME_TITLE_CHILD,
#else
                            XmNchildType, XmFRAME_TITLE_CHILD,
#endif
                            NULL);
#endif
#if XmVersion >= 2000
    XtVaCreateManagedWidget("Contours",
                            xmPushButtonWidgetClass, rc,
                            XmNnotebookChildType, XmMAJOR_TAB,
                            NULL);
#endif
    rc1 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr1,
                                  XmNorientation, XmHORIZONTAL,
                                  NULL);
    
    rc1_l = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc1,
                                    XmNorientation, XmVERTICAL,
                                    NULL);

    menuSpacing = CreateOptionMenu(rc1_l, &ContSpacingMenu);
    SetDefaultOptionMenuItem(menuSpacing, cont.spacing);

    menuMinMax = CreateOptionMenu(rc1_l, &ContMinMaxMenu);
    SetDefaultOptionMenuItem(menuMinMax, cont.minmax);

#if XmVersion >= 2000
    sb_rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc1_l,
                                  XmNorientation, XmHORIZONTAL,
                                  NULL);
    XtVaCreateManagedWidget("No. of contours:",
                            xmLabelWidgetClass, sb_rc, NULL);
    sb = XtVaCreateManagedWidget("spinbox", xmSpinBoxWidgetClass, sb_rc,
                                 XmNarrowLayout, XmARROWS_SPLIT,
                                 NULL);
    sb_txt = XtVaCreateManagedWidget("text", xmTextWidgetClass, sb,
                                     XmNwidth, 40,
                                     XmNspinBoxChildType, XmNUMERIC,
                                     XmNminimumValue, 1,
                                     XmNmaximumValue, MAXCONTOURS,
                                     XmNposition, cont.nc-1,
                                     NULL);
    XtAddCallback(sb, XmNvalueChangedCallback,
                  (XtCallbackProc)set_no_of_contours, NULL);
#endif
    
    n = 0;
    XtSetArg(wargs[n], XmNset, cont.relative ? True : False); n++;
    relButt = XtCreateWidget("Relative levels", xmToggleButtonWidgetClass,
                             rc1_l, wargs, n);
    XtAddCallback(relButt, XmNvalueChangedCallback,
                  (XtCallbackProc)set_relative_levels, NULL);

    menuDigit = CreateOptionMenu(rc1_l, &NDigitsMenu);
    SetDefaultOptionMenuItem(menuDigit, cont.ndigits-1);

    n = 0;
    XtSetArg(wargs[n], XmNentryClass, xmToggleButtonWidgetClass); n++;
    rbQuick = XmCreateRadioBox(rc1, "radiobox", wargs, n);
    for (i=0; i<3; i++) {
        n = 0;
        if (cont.quick == i) {
            XtSetArg(wargs[n], XmNset, True); n++;
        } else {
            XtSetArg(wargs[n], XmNset, False); n++;
        }
        quickButt[i] = XtCreateWidget(radio_cont_mode[i],
                                  xmToggleButtonWidgetClass,
                                  rbQuick, wargs, n);
        XtAddCallback(quickButt[i], XmNarmCallback,
                      (XtCallbackProc)set_type_of_cont_mode,
                      radio_cont_mode[i]);
    }
    
    fr2  = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rc,
                                   XmNshadowType, XmSHADOW_ETCHED_IN, NULL);
#if XmVersion >= 1200
    XtVaCreateManagedWidget(" Grey scale, interpolation, and projection options ",
                            xmLabelWidgetClass, fr2,
#if XmVersion >= 2000
                            XmNframeChildType, XmFRAME_TITLE_CHILD,
#else
                            XmNchildType, XmFRAME_TITLE_CHILD,
#endif
                            NULL);
#endif
#if XmVersion >= 2000
    XtVaCreateManagedWidget("Colour",
                            xmPushButtonWidgetClass, rc,
                            XmNnotebookChildType, XmMAJOR_TAB,
                            NULL);
#endif
    rc2 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr2,
                                  XmNorientation, XmHORIZONTAL,
                                  NULL);

    n = 0;
    XtSetArg(wargs[n], XmNentryClass, xmToggleButtonWidgetClass); n++;
    rbShade = XmCreateRadioBox(rc2, "radiobox", wargs, n);
    for (i=0; i<3; i++) {
        n = 0;
        if (cont.grey == i) {
            XtSetArg(wargs[n], XmNset, True); n++;
        } else {
            XtSetArg(wargs[n], XmNset, False); n++;
        }
        shadeButt[i] = XtCreateWidget(radio_grey_mode[i],
                                      xmToggleButtonWidgetClass,
                                      rbShade, wargs, n);
        XtAddCallback(shadeButt[i], XmNarmCallback,
                      (XtCallbackProc)set_type_of_grey_mode,
                      radio_grey_mode[i]);
    }
    
    rc2M = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc2,
                                   XmNorientation, XmVERTICAL,
                                   NULL);
    n = 0;
    if (cont.grey_inverted == 1) {
        XtSetArg(wargs[n], XmNset, True); n++;
    }
    invButt = XtCreateWidget("Invert", xmToggleButtonWidgetClass,
                             rc2M, wargs, n);
    XtAddCallback(invButt, XmNvalueChangedCallback,
                  (XtCallbackProc)set_grey_inverted, NULL);
  
    n = 0;
    if (draw.wedge == 1) {
        XtSetArg(wargs[n], XmNset, True); n++;
    }
    wedgeButt = XtCreateWidget("Show wedge", xmToggleButtonWidgetClass,
                               rc2M, wargs, n);
    XtAddCallback(wedgeButt, XmNvalueChangedCallback,
                  (XtCallbackProc)set_wedge, NULL);
    
    n = 0;
    if (rType == 1) {
        XtSetArg(wargs[n], XmNset, True); n++;
    }
    rotButt = XtCreateWidget("Rotate with PA", xmToggleButtonWidgetClass,
                             rc2M, wargs, n);
    XtAddCallback(rotButt, XmNvalueChangedCallback,
                  (XtCallbackProc)set_rotate, NULL);
    n = 0;
    if (cont.blank == 1) {
        XtSetArg(wargs[n], XmNset, True); n++;
    }
    blankButt = XtCreateWidget("",
                             xmToggleButtonWidgetClass,
                             rc2M, wargs, n);
    wprintf(blankButt, "Blank pixels\ninside polylines");
    XtAddCallback(blankButt, XmNvalueChangedCallback,
                  (XtCallbackProc)set_blank_inside, NULL);
        
    rc2R = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc2,
                                   XmNorientation, XmVERTICAL,
                                   NULL);
    intpButt = XtCreateWidget("Interpolation...", xmPushButtonWidgetClass,
                              rc2R, NULL, 0);
    XtAddCallback(intpButt, XmNactivateCallback,
                  (XtCallbackProc)PostInterpolationDialog, "contour");

    menuWedge = CreateOptionMenu(rc2R, &WedgeMenu);
    SetDefaultOptionMenuItem(menuWedge, draw.wedgepos);
    
    menuBeam = CreateOptionMenu(rc2R, &BeamMenu);
    SetDefaultOptionMenuItem(menuBeam, draw.beam);

    menuProj = CreateOptionMenu(rc2R, &ProjMenu);
    SetDefaultOptionMenuItem(menuProj, pType);

    fr4  = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rc,
                                   XmNshadowType, XmSHADOW_ETCHED_IN, NULL);
#if XmVersion >= 1200
    XtVaCreateManagedWidget(" Contour sliders ",
                            xmLabelWidgetClass, fr4,
#if XmVersion >= 2000
                            XmNframeChildType, XmFRAME_TITLE_CHILD,
#else
                            XmNchildType, XmFRAME_TITLE_CHILD,
#endif
                            NULL);
#endif

#if XmVersion >= 2000
    XtVaCreateManagedWidget("Sliders",
                            xmPushButtonWidgetClass, rc,
                            XmNnotebookChildType, XmMAJOR_TAB,
                            NULL);
#endif
    rc4 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr4,
                                  XmNorientation, XmVERTICAL,
                                  XmNnumColumns, 3,
                                  NULL);
    for (i=0; i<4; i++) {
        slider[i] = make_controller(slider_labels[i],
                                    slider_min[i], slider_max[i],
                                    rc4, slider_labels[i], slider_dec[i]);
    }

    ArrangeStdFormDialog(sf, rc);

    XtManageChild(menuSpacing);
    XtManageChild(menuMinMax);
    XtManageChild(menuDigit);
    XtManageChild(rbQuick);
    XtManageChild(rbShade);
    XtManageChildren(quickButt, 3);
    XtManageChildren(shadeButt, 3);
    XtManageChild(relButt);
    XtManageChild(intpButt);
    XtManageChild(invButt);
    XtManageChild(wedgeButt);
    XtManageChild(rotButt);
    XtManageChild(blankButt);
    XtManageChild(menuWedge);
    XtManageChild(menuBeam);
    XtManageChild(menuProj);

#ifdef DEBUG
    printf("pcd: set_slider_values()...\n");
#endif

    for (i=0; i<4; i++) {
        set_slider_values(slider[i], i);
    }
    
    ManageDialogCenteredOnPointer(sf->form);
}

void SaveAndViewMapTable(char *file)
{
    int i, j, nX, nY;
    double x, y, v1, v2;
    string buf;
    FILE *fp;
    MAP *m = vP->m;
    
    char *GetRAStr();
    char *GetDECStr();
    char *GetEpochStr(char, double);
    void  XS_system();
    
    if (!m) {
        PostWarningDialog(NULL, "There is no current map to make a table of.");
        return;
    }
    
    fp = fopen(file, "w");
    
    if (!fp) {
        sprintf(buf, "Couldn't open file '%s' for writing table.", file);
        PostWarningDialog(NULL, buf);
        return;
    }
    
    nX = m->i_no;
    nY = m->j_no;
    v1 = m->v - m->dv/2.0;
    v2 = m->v + m->dv/2.0;
    
    fprintf(fp, "# Source: %s   Size: %dx%d\n", m->name, nX, nY);
    fprintf(fp, "# Centre coordinates: %s %s (%s)\n",
         GetRAStr(m->x0), GetDECStr(m->y0), GetEpochStr(m->epoch, m->equinox));
    fprintf(fp, "# Map spacing: %7.2f %7.2f\n", m->xspacing, m->yspacing);
    fprintf(fp, "# Molecule: %s\n", m->molecule);
    fprintf(fp, "# Velocity interval: [%8.3f, %8.3f]\n", v1, v2);
    
    fprintf(fp, "# X [\"]   Y [\"]    Data      Error\n");
    for (j=0; j<nY; j++) {
        y = m->ylower + (double)j * m->yspacing;
        for (i=0; i<nX; i++) {
            x = m->xleft + (double)i * m->xspacing;
            if (m->f[i][j] > BLANK) {
                fprintf(fp, "%7.2f %7.2f %10.3e %10.3e\n",
                        x, y, m->d[i][j], m->e[i][j]);
            }
        }
    }
    
    fclose(fp);
    
    sprintf(buf, "%s %s &", pP->editor, file);
    XS_system(buf, 1);
    
    return;
}

void AttachContToMap(Widget wid, char *cmd, XtPointer cd)
{
    MAP *m = vP->m;
    string buf;
    
    if (!m) {
        sprintf(buf, "No current map to %s contour/grey data to.", cmd);
        PostWarningDialog(wid, buf);
        return;
    }
    
    if (strcmp(cmd, "attach")==0) {
        m->use_attached_cont = 1;
        m->c = cont;
        sprintf(buf, "Current contour/grey data attached to map %s.",
                m->name);
    } else if (strcmp(cmd, "deattach")==0 && m->use_attached_cont == 1) {
        m->use_attached_cont = 0;
        sprintf(buf, "Contour/grey data deattached from map %s.", m->name);
    } else {
        return;
    }
    
    PostMessageDialog(wid, buf);
}

static void ZTypeCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str), m = vP->mode;
    
    if (n != zType) {
        zType = n;
        if (m != SHOW_POSPOS)
            UpdateData(SCALE_NONE, NO_REDRAW);
        else
            UpdateData(SCALE_NONE, REDRAW);
    }
}

void SetCoordType(int n)
{
    if (n < COORD_TYPE_UNKNOWN || n > COORD_TYPE_GAL) return;
    
    cType = n;
}

static void CTypeCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str), m = vP->mode;
    
    if (n != cType) {
        cType = n;
        obtain_map_info(NULL, "map", NULL);
        if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL ||
            m == SHOW_ALLSPE)
            draw_main();
    }
}

static void FTypeCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str), m = vP->mode;
    
    if (n != fType) {
        fType = n;
        obtain_map_info(NULL, "map", NULL);
        if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL ||
            m == SHOW_ALLSPE)
            draw_main();
    }
}

int CheckCRVALType()
{
    return fType;
}

double *CheckPosAngle()
{
    if (!forcePosAngle) return NULL;
    
    return &posAngle;
}

static void set_pos_angle(Widget w, StdForm *sf,
                          XmToggleButtonCallbackStruct *cb)
{
    double pa;
    string buf;
    
    void wdscanf();

    wdscanf(sf->edit[0], &pa);
    
    if (cb->set) {
        forcePosAngle = 1;
        sprintf(buf, "You must reread the map data from disk,\n\
to use %.1f degrees as position angle.", pa);
        posAngle = pa;
    } else {
        forcePosAngle = 0;
        sprintf(buf, "You must reread the map data from disk,\n\
to use the position angle in the data header.");
    }

    PostWarningDialog(w, buf);
}

static void ScanPAandCancel(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    double pa;
    
    void wdscanf();
    void cancel_dialog();
    
    wdscanf(sf->edit[0], &pa);
    
    posAngle = pa;
    
    cancel_dialog(w, sf->form, cb);
}

static void ApplyMapType(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    double pa, xs, ys, zs, xc, yc;
    
    void wdscanf();
    
    wdscanf(sf->edit[0], &pa);
    posAngle = pa;
    
    wdscanf(sf->edit[1], &xc);
    wdscanf(sf->edit[2], &yc);
    wdscanf(sf->edit[3], &xs);
    wdscanf(sf->edit[4], &ys);
    wdscanf(sf->edit[5], &zs);
    
    xCentre = xc;
    yCentre = yc;
    xStep = xs;
    yStep = ys;
    if (zs > 0.0) zeroSpacing = zs;
}

static void RegridData(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    ApplyMapType(w, sf, cb);
    if (vP->from) {
        vP->from->gridded = 0;
        obtain_map_info(NULL, "map-centre", NULL);
    }
}

void PostMapTypeDialog(Widget wid, char *cmd, XtPointer cd)
{
    Widget rc, fr, rc2, rcH, PAButt, regridButt, gridButt;
    Widget menuZType, menuCType, menuFType;
    Widget w = wid;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

                
    sf = PostStdFormDialog(w, "Type of map",
             BUTT_APPLY, (XtCallbackProc)ApplyMapType, NULL,
             BUTT_CANCEL, (XtCallbackProc)ScanPAandCancel, NULL,
             BUTT_HELP, NULL, (XtPointer)TypeOfMap_Help,
             6, NULL);
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);

    menuZType = CreateOptionMenu(rc, &ZTypeMenu);
    SetDefaultOptionMenuItem(menuZType, zType);

    menuCType = CreateOptionMenu(rc, &CTypeMenu);
    SetDefaultOptionMenuItem(menuCType, cType);

    menuFType = CreateOptionMenu(rc, &FTypeMenu);
    SetDefaultOptionMenuItem(menuFType, fType);
    
    fr = XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
                                 rc, XmNshadowType, XmSHADOW_OUT, NULL);
    rc2 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                  XmNorientation, XmVERTICAL,
                                  NULL);
    XtCreateManagedWidget("Centre grid on offsets:",
                          xmLabelWidgetClass, rc2, NULL, 0);
    sf->edit[1] = XtVaCreateManagedWidget("edit", xmTextWidgetClass, rc2, NULL);
    sf->edit[2] = XtVaCreateManagedWidget("edit", xmTextWidgetClass, rc2, NULL);
    XtCreateManagedWidget("Force map spacing [\"]:",
                          xmLabelWidgetClass, rc2, NULL, 0);
    sf->edit[3] = XtVaCreateManagedWidget("edit", xmTextWidgetClass, rc2, NULL);
    sf->edit[4] = XtVaCreateManagedWidget("edit", xmTextWidgetClass, rc2, NULL);
    regridButt = XtVaCreateManagedWidget(
       "Regrid current data using the values above",
                                     xmPushButtonWidgetClass,
                                     rc2,
                                     NULL);
    XtAddCallback(regridButt, XmNactivateCallback,
                  (XtCallbackProc)RegridData, sf);
    gridButt = XtVaCreateManagedWidget(
       "Set offsets in scans to the current grid values",
                                     xmPushButtonWidgetClass,
                                     rc2,
                                     NULL);
    XtAddCallback(gridButt, XmNactivateCallback,
                  (XtCallbackProc)SetOffsetsUsingMapGrid, NULL);

    fr = XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
                                 rc, XmNshadowType, XmSHADOW_OUT, NULL);
    rcH = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                  XmNorientation, XmHORIZONTAL,
                                  NULL);
    XtCreateManagedWidget("Minimum map spacing [\"]:",
                          xmLabelWidgetClass, rcH, NULL, 0);
    sf->edit[5] = XtVaCreateManagedWidget("edit", xmTextWidgetClass, rcH, NULL);
    
    fr = XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
                                 rc, XmNshadowType, XmSHADOW_OUT, NULL);
    rc2 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                  XmNorientation, XmVERTICAL,
                                  NULL);
    XtVaCreateManagedWidget("Map position angle [deg]",
                            xmLabelWidgetClass, rc2, NULL);
    sf->edit[0] = XtVaCreateManagedWidget("edit", xmTextWidgetClass, rc2, NULL);
    PAButt = XtVaCreateManagedWidget("Force this PA",
                                     xmToggleButtonWidgetClass,
                                     rc2,
                                     XmNset, forcePosAngle ? TRUE : FALSE,
                                     NULL);

    XtAddCallback(PAButt, XmNvalueChangedCallback,
                  (XtCallbackProc)set_pos_angle, sf);
    ArrangeStdFormDialog(sf, rc);
    
    XtManageChild(menuZType);
    XtManageChild(menuCType);
    XtManageChild(menuFType);
    
    wprintf(sf->edit[0], "%f", posAngle);
    wprintf(sf->edit[1], "%f", xCentre);
    wprintf(sf->edit[2], "%f", yCentre);
    wprintf(sf->edit[3], "%f", xStep);
    wprintf(sf->edit[4], "%f", yStep);
    wprintf(sf->edit[5], "%f", zeroSpacing);
    
    ManageDialogCenteredOnPointer(sf->form);
}
