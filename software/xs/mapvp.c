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

#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/Frame.h>
#include <Xm/ToggleB.h>

#include "defines.h"
#include "global_structs.h"
#include "menus.h"
#include "dialogs.h"

/* #define DEBUG */

#define VPTYPE_LIN 0
#define VPTYPE_GAU 1

#define VP_POSAXIS_DIST 0
#define VP_POSAXIS_RA   1
#define VP_POSAXIS_DEC  2

#define VP_POSAXIS_VER 0
#define VP_POSAXIS_HOR 1

/*** External variables ***/
extern VIEW   *vP;
extern GLOBAL *gp;

void   ManageDialogCenteredOnPointer(Widget);
void   PostErrorDialog(Widget, char *);
MAP   *GetPosPosMap();
double xmap(scanPtr);
double ymap(scanPtr);

list    scan_iterator(list, DataSetPtr);
int     count_scans(DataSetPtr);
int     DeleteScan(DataSetPtr, scanPtr);
scanPtr copy_scanheader(DataSetPtr, int, scanPtr);
DataSetPtr get_tmpdataset();

Widget CreateOptionMenu(Widget, MenuBarItem *);
void   SetDefaultOptionMenuItem(Widget, int);
 
/*** Local variables ***/
static MAP vpMap;

static VELPOS     VelPos;

static scanPtr **vpScan;
static int     **vpFlag;
static double  **vpData;
static double  **vpErrorData;
static unsigned long int map_bytes = 0;

static void PosAxisCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem PosAxisData[] = {
  {"Distance", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PosAxisCallback,  "0", NULL},
  {"RA", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PosAxisCallback,  "1", NULL},
  {"Decl.", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PosAxisCallback,  "2", NULL},
EOI};
static MenuBarItem PosAxisMenu = {
   "Values on the position axis:", ' ', True, PosAxisData
};
static void PosAxisDirCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem PosAxisDirData[] = {
  {"Vertical", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PosAxisDirCallback,  "0", NULL},
  {"Horizontal", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PosAxisDirCallback,  "1", NULL},
EOI};
static MenuBarItem PosAxisDirMenu = {
   "Direction of the position axis:", ' ', True, PosAxisDirData
};

void init_vp_parameters()
{
    VelPos.npos = 50;
    VelPos.flag = 0;
    VelPos.type = VPTYPE_LIN;
    VelPos.posaxis = VP_POSAXIS_DIST;
    VelPos.posaxisdir = VP_POSAXIS_VER;
    VelPos.width = 30.0;
    
    vpMap.saved = 1;
}

double GetVelPosMapMemory()
{
    return (double)map_bytes/1024.0/1024.0;
}

MAP *GetVelPosMap()
{
    return &vpMap;
}

static scanPtr GetSpeAtPos(Point *p, int vert)
{
    scanPtr s=NULL, s00=NULL, s01=NULL, s10=NULL, s11=NULL, new=NULL;
    int i, j, nX, nY, n, c;
    int n_no, n_min;
    int n00, n01, n10, n11;
    double *a1=NULL, *a2=NULL, *a3=NULL, *a4=NULL;
    double *e1=NULL, *e2=NULL, *e3=NULL, *e4=NULL;
    double A, x=0.0, y=0.0;
    MAP *m = GetPosPosMap();
    
    if (!m) return NULL;

    
    if (vert) {
        n_no  = vpMap.i_no;
	n_min = vpMap.i_min;
    
    } else {
        n_no  = vpMap.j_no;
	n_min = vpMap.j_min;
    }
    
    nX = m->i_no;
    nY = m->j_no;
    
    i = (int)((p->x - m->xleft)/m->xspacing);
    j = (int)((p->y - m->ylower)/m->yspacing);
    
#ifdef DEBUG
    printf("i=%d(%d)  j=%d(%d) n=", i, nX, j, nY);
#endif

    if (i >= 0 && i < nX && j >= 0 && j < nY)
        n00 = m->f[i][j];
    else
        n00 = BLANK;
    
    if (i >= 0 && i < nX && j+1 >= 0 && j+1 < nY)
        n01 = m->f[i][j+1];
    else
        n01 = BLANK;
        
    if (i+1 >= 0 && i+1 < nX && j >= 0 && j < nY)
        n10 = m->f[i+1][j];
    else
        n10 = BLANK;
        
    if (i+1 >= 0 && i+1 < nX && j+1 >= 0 && j+1 < nY)
        n11 = m->f[i+1][j+1];
    else
        n11 = BLANK;
    
    n = 0;
    if (n11 > BLANK) {
        s = s11 = m->sp[i+1][j+1];
        n++;
    }
    if (n10 > BLANK) {
        s = s10 = m->sp[i+1][j];
        n++;
    }
    if (n01 > BLANK) {
        s = s01 = m->sp[i][j+1];
        n++;
    }
    if (n00 > BLANK) {
        s = s00 = m->sp[i][j];
        n++;
    }
    
#ifdef DEBUG
    printf("%d\n", n);
#endif
    
    if (n == 0) return NULL;
    
    new = copy_scanheader(get_tmpdataset(), n_no, s);
    if (!new) return new;
    
    switch (n) {
        case 1:
            for (c=0; c<new->nChan; c++) {
                new->d[c] = s->d[n_min + c];
                new->e[c] = s->e[n_min + c];
            }
            return new;
        case 2:
            if (n00 > BLANK && n01 > BLANK) {
                a1 = &(s00->d[n_min]);
                a2 = &(s01->d[n_min]);
                e1 = &(s00->e[n_min]);
                e2 = &(s01->e[n_min]);
                x = (p->y - ymap(s00))/(ymap(s01) - ymap(s00));
            } else if (n00 > BLANK && n10 > BLANK) {
                a1 = &(s00->d[n_min]);
                a2 = &(s10->d[n_min]);
                e1 = &(s00->e[n_min]);
                e2 = &(s10->e[n_min]);
                x = (p->x - xmap(s00))/(xmap(s10) - xmap(s00));
            } else if (n00 > BLANK && n11 > BLANK) {
                a1 = &(s00->d[n_min]);
                a2 = &(s11->d[n_min]);
                e1 = &(s00->e[n_min]);
                e2 = &(s11->e[n_min]);
                x = (p->x - xmap(s00))/(xmap(s11) - xmap(s00));
                y = (p->y - ymap(s00))/(ymap(s11) - ymap(s00));
                x = sqrt((x*x + y*y)/2.0);
            } else if (n01 > BLANK && n10 > BLANK) {
                a1 = &(s01->d[n_min]);
                a2 = &(s10->d[n_min]);
                e1 = &(s01->e[n_min]);
                e2 = &(s10->e[n_min]);
                x = (p->x - xmap(s01))/(xmap(s10) - xmap(s01));
                y = (p->y - ymap(s01))/(ymap(s10) - ymap(s01));
                x = sqrt((x*x + y*y)/2.0);
            } else if (n01 > BLANK && n11 > BLANK) {
                a1 = &(s01->d[n_min]);
                a2 = &(s11->d[n_min]);
                e1 = &(s01->e[n_min]);
                e2 = &(s11->e[n_min]);
                x = (p->x - xmap(s01))/(xmap(s11) - xmap(s01));
            } else if (n10 > BLANK && n11 > BLANK) {
                a1 = &(s10->d[n_min]);
                a2 = &(s11->d[n_min]);
                e1 = &(s10->e[n_min]);
                e2 = &(s11->e[n_min]);
                x = (p->y - ymap(s10))/(ymap(s11) - ymap(s10));
            }
#ifdef DEBUG
            printf("2 x=%f\n", x);
#endif
            for (c=0; c<new->nChan; c++) {
                new->d[c] = *a1 + x*(*a2 - *a1);
                new->e[c] = *e1 + x*(*e2 - *e1);
                a1++; a2++; e1++; e2++;
            }
            return new;
        case 3:
            if (n00 <= BLANK) {
                x = (p->x - xmap(s11))/(xmap(s01) - xmap(s11));
                y = (p->y - ymap(s11))/(ymap(s10) - ymap(s11));
                a1 = &(s11->d[n_min]);
                a2 = &(s10->d[n_min]);
                a3 = &(s01->d[n_min]);
                e1 = &(s11->e[n_min]);
                e2 = &(s10->e[n_min]);
                e3 = &(s01->e[n_min]);
#ifdef DEBUG
                printf("3 00 x = %f   y = %f\n", x, y);
#endif
            } else if (n01 <= BLANK) {
                x = (p->x - xmap(s10))/(xmap(s00) - xmap(s10));
                y = (p->y - ymap(s10))/(ymap(s11) - ymap(s10));
                a1 = &(s10->d[n_min]);
                a2 = &(s00->d[n_min]);
                a3 = &(s11->d[n_min]);
                e1 = &(s10->e[n_min]);
                e2 = &(s00->e[n_min]);
                e3 = &(s11->e[n_min]);
#ifdef DEBUG
                printf("3 01 x = %f   y = %f\n", x, y);
#endif
            } else if (n10 <= BLANK) {
                x = (p->x - xmap(s01))/(xmap(s11) - xmap(s01));
                y = (p->y - ymap(s01))/(ymap(s00) - ymap(s01));
                a1 = &(s01->d[n_min]);
                a2 = &(s11->d[n_min]);
                a3 = &(s00->d[n_min]);
                e1 = &(s01->e[n_min]);
                e2 = &(s11->e[n_min]);
                e3 = &(s00->e[n_min]);
#ifdef DEBUG
                printf("3 10 x = %f   y = %f\n", x, y);
#endif
            } else if (n11 <= BLANK) {
                x = (p->x - xmap(s00))/(xmap(s10) - xmap(s00));
                y = (p->y - ymap(s00))/(ymap(s01) - ymap(s00));
                a1 = &(s00->d[n_min]);
                a2 = &(s10->d[n_min]);
                a3 = &(s01->d[n_min]);
                e1 = &(s00->e[n_min]);
                e2 = &(s10->e[n_min]);
                e3 = &(s01->e[n_min]);
#ifdef DEBUG
                printf("3 11 x = %f   y = %f\n", x, y);
#endif
            }
            for (c=0; c<new->nChan; c++) {
                new->d[c] = *a1 + x*(*a2 - *a1) + y*(*a3 - *a1);
                new->e[c] = *e1 + x*(*e2 - *e1) + y*(*e3 - *e1);
                a1++; a2++; a3++; e1++; e2++; e3++;
            }
            return new;
        case 4:
            x = (p->x - xmap(s00))/(xmap(s10) - xmap(s00));
            y = (p->y - ymap(s00))/(ymap(s01) - ymap(s00));
            a1 = &(s00->d[n_min]);
            a3 = &(s01->d[n_min]);
            a2 = &(s10->d[n_min]);
            a4 = &(s11->d[n_min]);
            e1 = &(s00->e[n_min]);
            e3 = &(s01->e[n_min]);
            e2 = &(s10->e[n_min]);
            e4 = &(s11->e[n_min]);
#ifdef DEBUG
            printf("4 x = %f   y = %f\n", x, y);
#endif
            for (c=0; c<new->nChan; c++) {
                A = *a2 + *a3 - *a4;
                new->d[c] = A + (*a2 - A)*x + (*a3 - A)*y +
                                (*a1 - A)*(x-1.0)*(y-1.0);
                A = *e2 + *e3 - *e4;
                new->e[c] = A + (*e2 - A)*x + (*e3 - A)*y +
                           (*e1 - A)*(x-1.0)*(y-1.0);
                a1++; a2++, a3++; a4++; e1++; e2++, e3++; e4++;
            }
            return new;
    }
    return NULL;
}

static scanPtr GetGaussSpeAtPos(Point *p, double width, int vert)
{
    scanPtr s, new=NULL;
    double x, y, d2, w2, w, wsum;
    int c, first = 1, n_no, n_min;
    list curr = NULL;
    
    w2 = width*width;
    
    if (w2 == 0.0) return NULL;
    
    if (vert) {
        n_no = vpMap.i_no;
	n_min = vpMap.i_min;
    } else {
        n_no = vpMap.j_no;
	n_min = vpMap.j_min;
    }
    
    wsum = 0.0;
    
    while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
        s = (scanPtr)DATA(curr);
        if (first) {
            new = copy_scanheader(get_tmpdataset(), n_no, s);
            if (!new) return new;
            for (c=0; c<new->nChan; c++) new->d[c] = 0.0;
            first = 0;
        }
        x = p->x - xmap(s);
        y = p->y - ymap(s);
        d2 = x*x + y*y;
        w = exp(-ALPHA*d2/w2);
        for (c=0; c<new->nChan; c++) {
            new->d[c] += w * (s->d[n_min+c]);
        }
        wsum += w;
    }
    
    for (c=0; c<new->nChan; c++) new->d[c] /= wsum;
    
    return new;
}

static int MakeVelPosSpe(Point *p, int vert, int nX, int nY, int n)
{
    int m, np;
    string buf;
    scanPtr pSpe = NULL;
    
    void send_line();
    
    if (!p) {
        sprintf(buf, "Error: No position.");
        send_line(buf);
        return 1;
    }
    
    if (vert) {
      np = nY;
    } else {
      np = nX;
    }
    
    if (n >= np) {
        sprintf(buf, "Error: No of pos out of range: %d >= %d.", n, np);
        send_line(buf);
        return 1;
    }
    
    if (VelPos.type == VPTYPE_LIN)
        pSpe = GetSpeAtPos(p, vert);
    else if (VelPos.type == VPTYPE_GAU)
        pSpe = GetGaussSpeAtPos(p, VelPos.width, vert);
    else
        return 1;
    
    if (vert) {
      for (m=0; m<nX; m++) {
        if (pSpe) {
          vpData[m][n] = pSpe->d[m];
          vpErrorData[m][n] = pSpe->e[m];
          vpFlag[m][n] = 1;
        } else {
          sprintf(buf, "Warning: Couldn't interpolate for m=%d.", m);
          send_line(buf);
          vpData[m][n] = UNDEF;
          vpErrorData[m][n] = UNDEF;
          vpFlag[m][n] = BLANK;
        }
        vpScan[m][n] = NULL;
      }
    } else {
#ifdef DEBUG
    printf("hor, nchan=%d\n", nY);
#endif
      for (m=0; m<nY; m++) {
        if (pSpe) {
          vpData[n][m] = pSpe->d[m];
          vpErrorData[n][m] = pSpe->e[m];
          vpFlag[n][m] = 1;
        } else {
          sprintf(buf, "Warning: Couldn't interpolate for m=%d.", m);
          send_line(buf);
          vpData[n][m] = UNDEF;
          vpErrorData[n][m] = UNDEF;
          vpFlag[n][m] = BLANK;
        }
        vpScan[n][m] = NULL;
      }
    }
    
    DeleteScan(get_tmpdataset(), pSpe);
    
    return 0;
}

static void DoVelPos(Widget w, StdForm *sf, XtPointer call_data)
{
    int n, nX, nY, np, ver=0;
    static int allocX=0, allocY=0;
    double d, dx, dy;
    Point  p0, *p = NULL;

    void FreeDoubleArray(), FreeIntArray(), send_line(), FreeScanPtrArray();
    void wiscanf(), wdscanf(), set_show_mode();
    int **AllocIntArray();
    double **AllocDoubleArray(), SpecUnitConv();
    scanPtr **AllocScanPtrArray();
    Point *StepAlongPolyLine(PolyLine *, int, double *, Point *);

    wiscanf(sf->edit[0],  &VelPos.npos);
    if (!VelPos.pl) {
        wdscanf(sf->edit[1], &VelPos.x1);
        wdscanf(sf->edit[2], &VelPos.y1);
        wdscanf(sf->edit[3], &VelPos.x2);
        wdscanf(sf->edit[4], &VelPos.y2);
    }
    wdscanf(sf->edit[5], &VelPos.xleft);
    wdscanf(sf->edit[6], &VelPos.xright);
    if (VelPos.type == VPTYPE_GAU) {
        wdscanf(sf->edit[7], &VelPos.width);
    }
#ifdef DEBUG
    printf("prepare_velpos entered: (%f,%f) -> (%f,%f)\n",
           VelPos.x1, VelPos.y1, VelPos.x2, VelPos.y2);
#endif

    VelPos.flag = 0;
    
    if (VelPos.posaxisdir == VP_POSAXIS_VER) ver = 1;

    if (ver) {
      vpMap.j_min = 0;
      vpMap.j_max = VelPos.npos-1;
      nY = vpMap.j_no = VelPos.npos;
      vpMap.i_min = NINT(SpecUnitConv(UNIT_CHA, VelPos.xunit, VelPos.xleft));
      vpMap.i_max = NINT(SpecUnitConv(UNIT_CHA, VelPos.xunit, VelPos.xright));
      nX = vpMap.i_no = vpMap.i_max - vpMap.i_min + 1;
    } else {
      vpMap.i_min = 0;
      vpMap.i_max = VelPos.npos-1;
      nX = vpMap.i_no = VelPos.npos;
      vpMap.j_min = NINT(SpecUnitConv(UNIT_CHA, VelPos.xunit, VelPos.xleft));
      vpMap.j_max = NINT(SpecUnitConv(UNIT_CHA, VelPos.xunit, VelPos.xright));
      nY = vpMap.j_no = vpMap.j_max - vpMap.j_min + 1;
    }

    if (allocX != nX || allocY != nY) {
        if (allocX || allocY) {
            if (vpData)
                FreeDoubleArray(vpData, allocX, allocY);
            if (vpErrorData)
                FreeDoubleArray(vpErrorData, allocX, allocY);
            if (vpFlag)
                FreeIntArray(vpFlag, allocX, allocY);
            if (vpScan)
                FreeScanPtrArray(vpScan, allocX, allocY);
        }
        vpData = AllocDoubleArray(nX, nY);
        if (!vpData) {
            send_line("Error: Couldn't allocate enough memory.");
            return;
        }
        map_bytes = nX * nY * sizeof(double);
        vpErrorData = AllocDoubleArray(nX, nY);
        if (!vpErrorData) {
            FreeDoubleArray(vpData, nX, nY);
            send_line("Error: Couldn't allocate enough memory.");
            return;
        }
        map_bytes += nX * nY * sizeof(double);
        vpFlag = AllocIntArray(nX, nY);
        if (!vpFlag) {
            FreeDoubleArray(vpData, nX, nY);
            FreeDoubleArray(vpErrorData, nX, nY);
            send_line("Error: Couldn't allocate enough memory.");
            return;
        }
        map_bytes += nX * nY * sizeof(int);
        vpScan = AllocScanPtrArray(nX, nY);
        if (!vpScan) {
            FreeDoubleArray(vpData, nX, nY);
            FreeDoubleArray(vpErrorData, nX, nY);
            FreeIntArray(vpFlag, nX, nY);
            send_line("Error: Couldn't allocate enough memory.");
            return;
        }
        map_bytes += nX * nY * sizeof(scanPtr);
        allocX = nX;
        allocY = nY;
    }

#ifdef DEBUG
    printf("vpArrays allocated: nX,nY=%d,%d\n", nX, nY);
#endif

    if (ver) {
      np = nY;
    } else {
      np = nX;
    }
    if (VelPos.pl) { /* Use polyline */
        n = 0;
        while ((p = StepAlongPolyLine(VelPos.pl, np, &d, p)) != NULL) {
#ifdef DEBUG
            printf("position=%d of %d\n", n, np);
#endif
            MakeVelPosSpe(p, ver, nX, nY, n);
	    if (n == 0) {
	       VelPos.x1 = p->x;
	       VelPos.y1 = p->y;
	    } else {
	       VelPos.x2 = p->x;
	       VelPos.y2 = p->y;
	    }
            n++;
        }
    } else {         /* Use the two positions p1 & p2 */
        d = sqrt((VelPos.x2-VelPos.x1)*(VelPos.x2-VelPos.x1) +
                 (VelPos.y2-VelPos.y1)*(VelPos.y2-VelPos.y1));

        dx = (VelPos.x2 - VelPos.x1)/(double)(np - 1);
        dy = (VelPos.y2 - VelPos.y1)/(double)(np - 1);

        for (n=0; n<np; n++) {
            p0.x = VelPos.x1 + (double)n * dx;
            p0.y = VelPos.y1 + (double)n * dy;
            MakeVelPosSpe(&p0, ver, nX, nY, n);
        }
    }

    strcpy(vpMap.name, vP->s->name);
    strcpy(vpMap.molecule, vP->s->molecule);
    vpMap.date = vP->s->date;
    vpMap.ndata = vpMap.i_no * vpMap.j_no;
    if (VelPos.pl) {
        vpMap.x0 = vP->s->x0;
        vpMap.y0 = vP->s->y0;
    } else {
        vpMap.x0 = vP->s->x0 +
                  (VelPos.x1 + VelPos.x2)/2.0/RADTOSEC/cos(vP->s->y0);
        vpMap.y0 = vP->s->y0 + (VelPos.y1 + VelPos.y2)/2.0/RADTOSEC;
    }
    vpMap.epoch   = vP->s->epoch;
    vpMap.equinox = vP->s->equinox;
    vpMap.unit    = VelPos.xunit;
    
    if (ver) {
      vpMap.type = MAP_VELPOS;
      vpMap.xspacing = (VelPos.xright - VelPos.xleft)/(double)(nX - 1);
      vpMap.xleft  = VelPos.xleft;
      vpMap.xright = VelPos.xright;
      if (VelPos.posaxis == VP_POSAXIS_DIST) {
        vpMap.yspacing = d/(double)(nY - 1);
        vpMap.yupper = d;
        vpMap.ylower = 0.0;
      } else if (VelPos.posaxis == VP_POSAXIS_RA) {
        vpMap.yspacing = (VelPos.x2 - VelPos.x1)/(double)(nY - 1);
        vpMap.yupper = VelPos.x2;
        vpMap.ylower = VelPos.x1;
      } else {
        vpMap.yspacing = (VelPos.y2 - VelPos.y1)/(double)(nY - 1);
        vpMap.yupper = VelPos.y2;
        vpMap.ylower = VelPos.y1;
      }
    } else {
      vpMap.type = MAP_POSVEL;
      vpMap.yspacing = (VelPos.xright - VelPos.xleft)/(double)(nY - 1);
      vpMap.ylower = VelPos.xleft;
      vpMap.yupper = VelPos.xright;
      if (VelPos.posaxis == VP_POSAXIS_DIST) {
        vpMap.xspacing = d/(double)(nX - 1);
        vpMap.xright = d;
        vpMap.xleft = 0.0;
      } else if (VelPos.posaxis == VP_POSAXIS_RA) {
        vpMap.xspacing = (VelPos.x2 - VelPos.x1)/(double)(nX - 1);
        vpMap.xright = VelPos.x2;
        vpMap.xleft  = VelPos.x1;
      } else {
        vpMap.xspacing = (VelPos.y2 - VelPos.y1)/(double)(nX - 1);
        vpMap.xright = VelPos.y2;
        vpMap.xleft  = VelPos.y1;
      }
    }
    vpMap.swapped = 0;
    vpMap.memed = 0;
    vpMap.interpolated = 0;
    vpMap.saved = 0;
    vpMap.d = vpData;
    vpMap.e = vpErrorData;
    vpMap.f = vpFlag;
    vpMap.sp = vpScan;

    VelPos.flag = 1;
    set_show_mode(NULL, "velpos", NULL);
}

static void ToggleButtCallback(Widget w, StdForm *sf,
                               XmToggleButtonCallbackStruct *cd)
{
    if (cd->set) {
        VelPos.type = VPTYPE_GAU;
    } else {
        VelPos.type = VPTYPE_LIN;
    }
}

static void PosAxisCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str);
    
    if (n != VelPos.posaxis) {
        VelPos.posaxis = n;
    }
}

static void PosAxisDirCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str);
    
    if (n != VelPos.posaxisdir) {
        VelPos.posaxisdir = n;
    }
}

void PostVelPosDialog(Widget parent, double x1, double y1, double x2, double y2,
                      PolyLine *pl)
{
    Widget rc, fr[3], rcv[3], butt, intpButt, menuPosAxis, menuPosAxisDir;
    VIEW *sv;
    string buf;
    StdForm *sf;

    void wprintf();
    VIEW *GetScanView();
    double chan2xunit();
    void SetIntpOrder(int);
    void PostInterpolationDialog(Widget, char *, XtPointer);
    
    rcv[0] = rcv[1] = rcv[2] = NULL;

    if (!vP->s) return;
    
    sv = GetScanView();
    if (!sv) {
        VelPos.xleft  = chan2xunit(0);
        VelPos.xright = chan2xunit(vP->s->nChan - 1);
        VelPos.xunit  = vP->xunit;
    } else {
        VelPos.xleft  = sv->xleft;
        VelPos.xright = sv->xright;
        VelPos.xunit  = sv->xunit;
    }
    VelPos.x1 = x1;
    VelPos.x2 = x2;
    VelPos.y1 = y1;
    VelPos.y2 = y2;
    VelPos.pl = pl;
    
    /* We set the interpolation order to zero, since usually we
       do not wish to interpolate velocity-position maps. A high-
       order interpolation may take forever to perform. */
    SetIntpOrder(0);

    sf = PostStdFormDialog(parent, "Velocity-Position selection",
             BUTT_APPLY, (XtCallbackProc)DoVelPos, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             8, NULL);

    rc  = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                           XmNorientation, XmVERTICAL,
                           XmNpacking, XmPACK_TIGHT,
                           NULL);

    fr[0]  = XtVaCreateWidget("frame", xmFrameWidgetClass,
                              rc, XmNshadowType, XmSHADOW_OUT, NULL);
    rcv[0] = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, fr[0],
                              XmNorientation, XmHORIZONTAL,
                              NULL);
    XtVaCreateManagedWidget("No of positions:", xmLabelWidgetClass,
                            rcv[0], NULL);
    sf->edit[0] = XtVaCreateManagedWidget("npos", xmTextWidgetClass,
                                          rcv[0], NULL);

    if (!pl) {
        fr[1]  = XtVaCreateWidget("frame", xmFrameWidgetClass,
                                  rc, XmNshadowType, XmSHADOW_OUT, NULL);
        rcv[1] = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, fr[1],
                                  XmNorientation, XmHORIZONTAL,
                                  XmNpacking, XmPACK_COLUMN,
                                  XmNnumColumns, 2,
                                  NULL);
        XtVaCreateManagedWidget("First position:", xmLabelWidgetClass,
                                rcv[1], NULL);
        sf->edit[1] = XtVaCreateManagedWidget("x1", xmTextWidgetClass,
                                                rcv[1], NULL);
        sf->edit[2] = XtVaCreateManagedWidget("y1", xmTextWidgetClass,
                                                rcv[1], NULL);
        XtVaCreateManagedWidget("Second position:", xmLabelWidgetClass,
                                rcv[1], NULL);
        sf->edit[3] = XtVaCreateManagedWidget("x2", xmTextWidgetClass,
                                              rcv[1], NULL);
        sf->edit[4] = XtVaCreateManagedWidget("y2", xmTextWidgetClass,
                                              rcv[1], NULL);
    }

    if (VelPos.xunit == UNIT_FRE) {
        strcpy(buf, "Freq. range (GHz):");
    } else if (VelPos.xunit == UNIT_FOFF) {
        strcpy(buf, "Freq. offset range:");
    } else if (VelPos.xunit == UNIT_FMHZ) {
        strcpy(buf, "Freq. range (MHz):");
    } else if (VelPos.xunit == UNIT_VEL) {
        strcpy(buf, "Velocity range:");
    } else {
        strcpy(buf, "Channel range:");
    }
    fr[2]  = XtVaCreateWidget("frame", xmFrameWidgetClass,
                              rc, XmNshadowType, XmSHADOW_OUT, NULL);
    rcv[2] = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, fr[2],
                              XmNorientation, XmHORIZONTAL,
                              NULL);
    XtVaCreateManagedWidget(buf, xmLabelWidgetClass,
                            rcv[2], NULL);
    sf->edit[5] = XtVaCreateManagedWidget("v1", xmTextWidgetClass,
                                            rcv[2], NULL);
    sf->edit[6] = XtVaCreateManagedWidget("v2", xmTextWidgetClass,
                                            rcv[2], NULL);
                         
    menuPosAxis = CreateOptionMenu(rc, &PosAxisMenu);
    SetDefaultOptionMenuItem(menuPosAxis, VelPos.posaxis);
                         
    menuPosAxisDir = CreateOptionMenu(rc, &PosAxisDirMenu);
    SetDefaultOptionMenuItem(menuPosAxisDir, VelPos.posaxisdir);
    
    butt = XtVaCreateManagedWidget(
                    "Use Gaussian interpolation with weighting distance:",
                                   xmToggleButtonWidgetClass, rc,
                                   XmNset, VelPos.type ? True : False,
                                   NULL);
    sf->edit[7] = XtVaCreateManagedWidget("Gaussian FWHM:", xmTextWidgetClass,
                                           rc, NULL);
    intpButt = XtVaCreateManagedWidget("Interpolation...",
                                       xmPushButtonWidgetClass, rc, NULL);
    XtAddCallback(intpButt, XmNactivateCallback,
                  (XtCallbackProc)PostInterpolationDialog, "contour");

    XtAddCallback(butt, XmNvalueChangedCallback,
                  (XtCallbackProc)ToggleButtCallback, sf);
    
    ArrangeStdFormDialog(sf, rc);
    
    XtManageChild(rcv[0]);
    if (!pl) XtManageChild(rcv[1]);
    XtManageChild(rcv[2]);
    if (!pl) {
        XtManageChildren(fr, 3);
    } else {
        XtManageChild(fr[0]);
        XtManageChild(fr[2]);
    }
    XtManageChild(menuPosAxis);
    XtManageChild(menuPosAxisDir);
    XtManageChild(rc);

    wprintf(sf->edit[0],  "%d", VelPos.npos);
    if (!pl) {
        wprintf(sf->edit[1], "%f", VelPos.x1);
        wprintf(sf->edit[2], "%f", VelPos.y1);
        wprintf(sf->edit[3], "%f", VelPos.x2);
        wprintf(sf->edit[4], "%f", VelPos.y2);
    }
    wprintf(sf->edit[5], "%f", VelPos.xleft);
    wprintf(sf->edit[6], "%f", VelPos.xright);
    wprintf(sf->edit[7],  "%f", VelPos.width);
    
    ManageDialogCenteredOnPointer(sf->form);
}

int GetVelPos(double *x1, double *y1, double *x2, double *y2)
{
    *x1 = VelPos.x1;
    *x2 = VelPos.x2;
    *y1 = VelPos.y1;
    *y2 = VelPos.y2;

    return VelPos.flag;
}

int SetVelPosShowMode()
{
    PolyLine *pl;
    int smode;
    
    void obtain_map_info(), SetViewMode();
    PolyLine *GetFirstPolyLine(Point *);

    if (count_scans(vP->from) < 2) {
        PostErrorDialog(NULL, "No velocity position data available.");
        return 1;
    }
    if (!VelPos.flag) {
        pl = GetFirstPolyLine(NULL);
        if (!pl) {
            PostErrorDialog(NULL, "Couldn't find a polyline.");
            return 1;
        }
        PostVelPosDialog(gp->graph, 0.0, 0.0, 0.0, 0.0, pl);
        return 0;
    }
    if (VelPos.posaxisdir == VP_POSAXIS_VER) {
        smode = SHOW_VELPOS;
    } else {
        smode = SHOW_POSVEL;
    }
    SetViewMode(smode, vP->s, &vpMap, vP->p);
    obtain_map_info(NULL, "velpos", NULL);
    strcpy(vP->t_label, vP->m->name);
    
    return 0;
}
