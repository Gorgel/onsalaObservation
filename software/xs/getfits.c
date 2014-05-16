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
#include <time.h>

#include <Xm/Xm.h>
#include <Xm/XmStrDefs.h>
#include <Xm/SelectioB.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>

#include "defines.h"
#include "global_structs.h"
#include "drp.h"
#include "fits.h"
#include "menus.h"

/*** External variables and structures ***/
extern GLOBAL  *gp;
extern VIEW    *vP;
extern FDATA   *fP;

void   ManageDialogCenteredOnPointer(Widget);
void   PostMessageDialog(Widget, char *);
void   PostWarningDialog(Widget, char *);
void   PostErrorDialog(Widget, char *);
Widget PostWaitingDialog(Widget, char *, Widget *, int);
void   SetWaitingScale(Widget, int);
char  *GetRAStr(double), *GetDECStr(double);
char  *GetLongStr(double), *GetLatStr(double);
Widget CreateOptionMenu(Widget, MenuBarItem *);
void   SetDefaultOptionMenuItem(Widget, int);
void   SetDefaultOptionMenuItemString(Widget, MenuItem *, char *);

void   send_line(char *);

int     count_scans(DataSetPtr);
list    scan_iterator(list, DataSetPtr);
scanPtr new_scan(DataSetPtr, int);
scanPtr copy_scan(DataSetPtr, scanPtr);
list   *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);

SCAN OnScan;
XSCAN XScan;

/*** Local variables ***/
static string buf;

static int        ndata;
static short int *sdata;
static long int  *idata;
static float     *fdata;
static char      *cdata;
static double    *ddata;

struct fitskey *readFITSheader(const char *, int);
int  readFITSdata(int, int, void *);
int  LoadBinaryTable(int, char *, int);
int  writeFITSheader(const char *);
int  writeBINTABLEheader(fkey *, int);
int  writeFITSdata(int, int, void *);
void addFITScard(int, struct fitskey *);
void ClearFITSwords(void);
void CloseFITSfile(void);

#define VLSR (0<<2)
#define VHEL (1<<2)
#define VGEO (2<<2)

/* #define IRAMCUBE -1038 */  /* TT Cyg 1-0 */
/* #define IRAMCUBE -5752 */  /* TT Cyg 2-1 */
/* #define IRAMCUBE 206 */    /* S Sct 1-0 */

#ifdef IRAMCUBE
/* #define BVAL(x, y)  ((x) >= IRAMCUBE-5 && (x) <= IRAMCUBE+5) */   /* 2-1 */
#define BVAL(x, y)  ((x) == IRAMCUBE)                    /* 1-0 */
#else
#define BVAL(x, y)  ((x) == (double)(y))
#endif

static struct fitskey card;

#define SORT

#ifdef SORT
static int sort1_type = SORT_TYPE_NONE;
static int sort2_type = SORT_TYPE_NONE;
static int sort_order = SORT_INCREASING;

static void set_sort1_type(Widget, char *, XmAnyCallbackStruct *);
static MenuItem Sort1Data[] = {
  {"None", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, "-1", NULL},
  {"Scan no", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, " 0", NULL},
  {"Channel no", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, " 1", NULL},
  {"Frequency", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, "10", NULL},
  {"Velocity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, "11", NULL},
  {"Int. time", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, "12", NULL},
  {"Date/Time", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, "13", NULL},
  {"RA", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, "14", NULL},
  {"Dec.", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, "15", NULL},
  {"RA offset", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, "16", NULL},
  {"Dec. offset", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, "17", NULL},
  {"Dist. from (0,0)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, "18", NULL},
  {"Source", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, "30", NULL},
  {"Molecule", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort1_type, "31", NULL},
EOI};
static MenuBarItem Sort1Menu = {
   "Primary sorting", ' ', True, Sort1Data
};

static void set_sort2_type(Widget, char *, XmAnyCallbackStruct *);
static MenuItem Sort2Data[] = {
  {"None", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, "-1", NULL},
  {"Scan no", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, " 0", NULL},
  {"Channel no", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, " 1", NULL},
  {"Frequency", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, "10", NULL},
  {"Velocity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, "11", NULL},
  {"Int. time", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, "12", NULL},
  {"Date/Time", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, "13", NULL},
  {"RA", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, "14", NULL},
  {"Dec.", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, "15", NULL},
  {"RA offset", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, "16", NULL},
  {"Dec. offset", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, "17", NULL},
  {"Dist. from (0,0)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, "18", NULL},
  {"Source", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, "30", NULL},
  {"Molecule", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort2_type, "31", NULL},
EOI};
static MenuBarItem Sort2Menu = {
   "Secondary", ' ', True, Sort2Data
};
static void set_sort_order(Widget, char *, XmAnyCallbackStruct *);
static MenuItem SortOrderData[] = {
  {"Increasing", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort_order, "0", NULL},
  {"Decreasing", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sort_order, "1", NULL},
EOI};
static MenuBarItem SortOrderMenu = {
   "Sort order", ' ', True, SortOrderData
};
#endif

int CheckDataSize(int n)
{
    static int size = -1;
    
    if (n <= 0) return size;
    
    if (size < 0) {
        OnScan.c = (float *)malloc(n * sizeof(float));
        if (!OnScan.c) return 0;
        fP->d = (double *)malloc(n * sizeof(double));
        if (!fP->d) return 0;
        fP->e = (double *)malloc(n * sizeof(double));
        if (!fP->e) return 0;
        size = n;
    } else if (n > size) {
        OnScan.c = (float *)realloc(OnScan.c, n * sizeof(float));
        if (!OnScan.c) return size;
        fP->d = (double *)realloc(fP->d, n * sizeof(double));
        if (!fP->d) return size;
        fP->e = (double *)realloc(fP->e, n * sizeof(double));
        if (!fP->e) return size;
        size = n;
    }
    return size;
}


void VoidCard(int index)
{
    card.vartyp = NVARTYPE;
    addFITScard(index, &card);
}

void BoolCard(int index, int b)
{
    card.val.l = (long)b;
    card.vartyp = BOOLTYPE;
    addFITScard(index, &card);
}

void CharCard(int index, char *s)
{
    int i, len;

    strncpy(card.val.str, s, 16);
    len = strlen(card.val.str);
    if (len < 8) {
        for (i = len; i < 8; i++) card.val.str[i] = ' ';
        card.val.str[8] = '\0';
    }
    card.vartyp = CHARTYPE;
    addFITScard(index, &card);
}

void LongCard(int index, long l)
{
    card.val.l = l;
    card.vartyp = LONGTYPE;
    addFITScard(index, &card);
}

void RealCard(int index, double d)
{
    card.val.d = d;
    card.vartyp = REALTYPE;
    addFITScard(index, &card);
}

static void FreeFITSData()
{
    if (sdata) {
        free(sdata);
        sdata = NULL;
    }
    if (idata) {
        free(idata);
        idata = NULL;
    }
    if (fdata) {
        free(fdata);
        fdata = NULL;
    }
    if (cdata) {
        free(cdata);
        cdata = NULL;
    }
    if (ddata) {
        free(ddata);
        ddata = NULL;
    }
    
    ndata = 0;
}

static void GetFITSScaleFactors(int bitpix, double min, double max,
                                double *bz, double *bs)
{
    if (!bz || !bs) return;
    
    if (bitpix == FITS_TYPE_8BIT) {
        *bz = (double)((max + min)/2.0);
        *bs = (double)((max - *bz)/127.0);
    } else if (bitpix == FITS_TYPE_16BIT) {
        *bz = (double)((max + min)/2.0);
        *bs = (double)((max - *bz)/32767.0);
    } else if (bitpix == FITS_TYPE_32BIT) {
        *bz = (double)((max + min)/2.0);
        *bs = (double)((max - *bz)/2147483647.0);
    } else if (bitpix == FITS_TYPE_FLOAT) {
        *bz = 0.0;
        *bs = 1.0;
    } else if (bitpix == FITS_TYPE_DOUBLE) {
        *bz = 0.0;
        *bs = 1.0;
    } else {
        *bz = (double)((max + min)/2.0);
        *bs = (double)((max - *bz));
    }
    
    return;
}

static long int GetBLANK(int bitpix)
{
    if (bitpix == FITS_TYPE_8BIT) {
        return -128;
    } else if (bitpix == FITS_TYPE_16BIT) {
        return -32768;
    } else if (bitpix == FITS_TYPE_32BIT) {
        return -2147483646;
    } else {
        return 0;
    }
}

int AllocFITSData(int bitpix, int n, int *size, void **d)
{
    int err = 0, s = 0;
    
    if (bitpix == FITS_TYPE_8BIT) {
        s = sizeof(char);
        cdata = (char *)malloc(n * s);
        if (!cdata) err = 1;
        else if (d) *d = (void *)cdata;
    } else if (bitpix == FITS_TYPE_16BIT) {
        s = sizeof(short int);
        sdata = (short int *)malloc(n * s);
        if (!sdata) err = 1;
        else if (d) *d = (void *)sdata;
    } else if (bitpix == FITS_TYPE_32BIT) {
        s = sizeof(long int);
        idata = (long int *)malloc(n * s);
        if (!idata) err = 1;
        else if (d) *d = (void *)idata;
    } else if (bitpix == FITS_TYPE_FLOAT) {
        s = sizeof(float);
        fdata = (float *)malloc(n * s);
        if (!fdata) err = 1;
        else if (d) *d = (void *)fdata;
    } else if (bitpix == FITS_TYPE_DOUBLE) {
        s = sizeof(double);
        ddata = (double *)malloc(n * s);
        if (!ddata) err = 1;
        else if (d) *d = (void *)ddata;
    } else {
        err = 2;
    }
    
    if (err) {
        if (err == 1)
            sprintf(buf, "Out of memory when allocating FITS data, n=%d, Type=%d.",
                    n, bitpix);
        else
            sprintf(buf, "Not a valid BITPIX %d when allocating FITS data.",
                    bitpix);
        send_line(buf);
        CloseFITSfile();
        ndata = 0;
    } else {
        if (size) *size = s;
        ndata = n;
    }
    
    return err;
}


static int AllocAndReadFITS(int bitpix, int n, int close)
{
    int err = 1, size;
    void *d;
    string buf;
    
    if (AllocFITSData(bitpix, n, &size, &d)) return 1;
    
    if (close == -1) return 0; /* For binary tables */
    
    err = readFITSdata(n, size, d);
    
    if (err == 0) {
        sprintf(buf, "Error reading FITS %d data.", bitpix);
        send_line(buf);
        CloseFITSfile();
        FreeFITSData();
        return 1;
    }

#ifdef DEBUG		    
    printf("Read %d bytes of data.\n", size*n);
#endif
    
    if (close) CloseFITSfile();
    
    return 0;
}

static double GetFITSValue(int bitpix, int n)
{
    if (n < 0 || n >= ndata) return 0.0;
    
    if (bitpix == FITS_TYPE_16BIT) {
        return GET2(sdata[n]);
    } else if (bitpix == FITS_TYPE_32BIT) {
        return GET4(idata[n]);
    } else if (bitpix == FITS_TYPE_FLOAT) {
        return GETF(fdata[n]);
    } else if (bitpix == FITS_TYPE_DOUBLE) {
        return GETD(ddata[n]);
    } else if (bitpix == FITS_TYPE_8BIT) {
        return GET1(cdata[n]);
    } else {
        return 0.0;
    }
}

static void SetFITSValue(int bitpix, int n, double val)
{
    if (n < 0 || n >= ndata) return;
    
    if (bitpix == FITS_TYPE_16BIT)
        sdata[n] = GET2((short int)val);
    else if (bitpix == FITS_TYPE_32BIT)
        idata[n] = GET4((long int)val);
    else if (bitpix == FITS_TYPE_FLOAT)
        fdata[n] = GETF((float)val);
    else if (bitpix == FITS_TYPE_DOUBLE)
        ddata[n] = GETD(val);
    else if (bitpix == FITS_TYPE_8BIT)
        cdata[n] = GET1(val);
}

int LoadHolo(char *filename, char *cmd)
{
    int n, m, i, j, ndata, nx, ny, first;
    string buf;
    FILE *fp;
    MAP *a;
    Beam b;
    double *c1, *c2, *c3, *c4, Emin, Emax, span, lim2;
    double X, Y, A, P, d2, wid, w, wsum, sP, cP;
    double dx, dy, x_min, x_max, y_min, y_max;
    double a_min, a_max, p_min, p_max;
    double alambda, samp, dist1, dist2, theta;
    static int size=10000;
    Widget wait=NULL, scale=NULL;
    Holography *h = gp->hp;
    
    list *get_maplist();
    MAP *new_map(), *copy_map();
    void MapDraw();
    Widget PostWaitingDialog(Widget, char *, Widget *, int);
    void   SetWaitingScale(Widget, int);
    DATE *XS_localtime();
    char *StripPath(), *StripSuffix();
    
    b.PA = b.min = b.maj = 0.0;
    
    fp = fopen(filename, "r");
    
    if (!fp) return 1;
    
    c1 = (double *)malloc(size * sizeof(double));
    c2 = (double *)malloc(size * sizeof(double));
    c3 = (double *)malloc(size * sizeof(double));
    c4 = (double *)malloc(size * sizeof(double));
    
    if (!c1 || !c2 || !c3 || !c4) {
        if (c1) free(c1);
        if (c2) free(c2);
        if (c3) free(c3);
        if (c4) free(c4);
        return 1;
    }
    
    n=0;
    while ((fgets(buf, sizeof(buf)-1, fp)) != NULL) {
        m = sscanf(buf, "%lf %lf %lf %lf", &X, &Y, &A, &P);
	if (m != 4) {
	    if (c1) free(c1);
	    if (c2) free(c2);
	    if (c3) free(c3);
	    if (c4) free(c4);
	    return 1;
	}
	if (n >= size) {
	    size += 10000;
	    c1 = (double *)realloc(c1, size * sizeof(double));
	    c2 = (double *)realloc(c2, size * sizeof(double));
	    c3 = (double *)realloc(c3, size * sizeof(double));
	    c4 = (double *)realloc(c4, size * sizeof(double));
	    if (!c1 || !c2 || !c3 || !c4) {
        	if (c1) free(c1);
        	if (c2) free(c2);
        	if (c3) free(c3);
        	if (c4) free(c4);
        	return 1;
	    }
	}
	if (A < 0.0 || A > 1.0e6) continue;
	c1[n] = X; c2[n] = Y; c3[n] = A;
        c4[n] = (M_PI/180.0)*(P*h->Phase_k + h->Phase_m);
	n++;
    }
    
    ndata = n;
    
    fclose(fp);
    
    sprintf(buf, "Read %d entries from %s.", ndata, filename);
    send_line(buf);
    
    x_min=x_max=c1[0];
    y_min=y_max=c2[0];
    a_min=a_max=c3[0];
    p_min=p_max=c4[0];
    for (n=1; n<ndata; n++) {
        if (c1[n] < x_min) x_min = c1[n];
        if (c1[n] > x_max) x_max = c1[n];
        if (c2[n] < y_min) y_min = c2[n];
        if (c2[n] > y_max) y_max = c2[n];
        if (c3[n] < a_min) a_min = c3[n];
        if (c3[n] > a_max) a_max = c3[n];
        if (c4[n] < p_min) p_min = c4[n];
        if (c4[n] > p_max) p_max = c4[n];
    }
    
    sprintf(buf, "x:%f,%f  y:%f,%f  a:%f,%f  p:%f,%f",
            x_min, x_max, y_min, y_max, a_min, a_max, p_min, p_max);
    send_line(buf);
    
    nx = ny = NINT(h->nDim);
    Emax = (double)(nx-1)*h->Sampling/3600./2.;
    Emin = -Emax;
    span = Emax-Emin;
    
    a = new_map(get_maplist(), nx, ny);
    if (!a) {
    	return 1;
    }
    sprintf(a->name, "%s", StripSuffix(StripPath(filename)));
    a->type = MAP_POSPOS;
    a->date = *XS_localtime();
    a->coordType = COORD_TYPE_HOR;
    a->equinox = 0;
    a->sequence = 0;
    a->swapped = 0;
    a->memed = 0;
    a->ndata = nx * ny;
    a->b = b;
    a->x0 = h->Az/RADTODEG;
    a->xspacing = span/(double)(nx-1);
    a->xleft  = Emin;
    a->xright = Emax;
    a->i_no = nx;
    a->i_min = NINT(a->xleft/a->xspacing);
    a->i_max = a->i_min + nx - 1;
    a->y0 = h->El/RADTODEG;
    a->yspacing = span/(double)(ny-1);
    a->ylower = Emin;
    a->yupper = Emax;
    a->j_no = ny;
    a->j_min = NINT(a->ylower/a->yspacing);
    a->j_max = a->j_min + ny - 1;
    a->posAngle = 0.0;
    a->fMHz = h->TransFreq;
    a->fft = -2;
    
    wait = PostWaitingDialog(NULL, "Resample the holo data...",
	                     &scale, nx*ny);
    first = 1;  
    wid = span/(double)(nx-1)/2.0;
    lim2 = 9.0*wid*wid;
    for (i=0; i<nx; i++) {
      X = Emin + (double)i * a->xspacing;
      for (j=0; j<ny; j++) {
        Y = Emin + (double)j * a->yspacing;
	A = sP = cP = wsum = 0.0;
        if (wait) SetWaitingScale(scale, j + i*ny + 1);
	for (n=0; n<ndata; n++) {
	  dx = X - c1[n];
	  dy = Y - c2[n];
	  d2 = dx*dx + dy*dy;
	  if (d2 > lim2) continue;
	  w = exp(-ALPHA*d2/wid/wid);
	  A += w*c3[n];
	  P = c4[n];
	  sP += w*sin(P);
	  cP += w*cos(P);
	  wsum += w;
	}
	if (wsum > 0.0) {
	  A /= wsum;
	  P = atan2(sP, cP);
	  a->d[i][j] = A;
	  a->e[i][j] = P;
	  a->f[i][j] = UNBLANK;
	  if (first) {
	    first=0;
	    a_min = a_max = A;
	    p_min = p_max = P;
	  } else {
            if (A < a_min) a_min = A;
            if (A > a_max) a_max = A;
            if (P < p_min) p_min = P;
            if (P > p_max) p_max = P;
	  }
	} else {
	  a->d[i][j] = 0.0;
	  a->e[i][j] = 0.0;
	  a->f[i][j] = BLANK;
	}
      }
    }
    sprintf(buf, "After resampling: a:%f,%f  p:%f,%f",
            a_min, a_max, p_min, p_max);
    send_line(buf);
    
    if (wait) XtDestroyWidget(wait);
    
    if (c1) free(c1);
    if (c2) free(c2);
    if (c3) free(c3);
    if (c4) free(c4);
    
    alambda = SPEEDOFLIGHT*1.0e3/a->fMHz/1.0e6;
    samp = h->Sampling/3600.0/RADTODEG;
    dist1 = h->Zref;
    dist2 = h->Zseco;
    for (i=0; i<nx; i++) {
      X = (double)i - (double)(nx-1)*0.5;
      for (j=0; j<ny; j++) {
        Y = (double)j - (double)(ny-1)*0.5;
        a->d[i][j] -= h->Ampl_m;
	theta = sqrt(X*X + Y*Y) * samp;
	a->e[i][j] += 2.0*M_PI/alambda*(dist1-dist2)*(1.0 - cos(theta));
      }
    }
    
    MapDraw(NULL, a, NULL);
    
    return 0;
}

int LoadAPEXMap(char *filename, char *cmd)
{
    int n, m, i, j, ndata, nx, ny, first;
    string buf;
    FILE *fp;
    MAP *a, *b;
    Beam beam;
    double *c1, *c2, *c3, *c4, *c5, lim2;
    double X, Y, A, P, T, d2, wid, w, wsum, elevation=0.0;
    double dx, dy, x_min, x_max, y_min, y_max, t_min, t_max;
    double a_min, a_max, p_min, p_max;
    static int size=10000;
    Widget wait=NULL, scale=NULL;
    APEXMap *h = gp->am;
    
    list *get_maplist();
    MAP *new_map(), *copy_map();
    void MapDraw();
    Widget PostWaitingDialog(Widget, char *, Widget *, int);
    void   SetWaitingScale(Widget, int);
    DATE *XS_localtime();
    char *StripPath(), *StripSuffix();
    
    beam.PA = beam.maj = beam.min = 0.0;
    
    fp = fopen(filename, "r");
    
    if (!fp) {
        sprintf(buf, "No such APEX map file '%s'", filename);
	send_line(buf);
        return 1;
    }
    
    c1 = (double *)malloc(size * sizeof(double));
    c2 = (double *)malloc(size * sizeof(double));
    c3 = (double *)malloc(size * sizeof(double));
    c4 = (double *)malloc(size * sizeof(double));
    c5 = (double *)malloc(size * sizeof(double));
    
    if (!c1 || !c2 || !c3 || !c4 || !c5) {
        if (c1) free(c1);
        if (c2) free(c2);
        if (c3) free(c3);
        if (c4) free(c4);
        if (c5) free(c5);
        sprintf(buf, "Not enough memory %d.", size);
	send_line(buf);
        return 1;
    }
    
    n=0;
    while ((fgets(buf, sizeof(buf)-1, fp)) != NULL) {
        if (buf[0] == '!' || buf[0] == '#') continue;
	if (buf[0] == 'E') {
            m = sscanf(buf, "El %lf", &elevation);
	    continue;
	}
        m = sscanf(buf, "%lf %lf %lf %lf %lf", &X, &Y, &A, &P, &T);
	if (m != 5 && m != 4) {
	    if (c1) free(c1);
	    if (c2) free(c2);
	    if (c3) free(c3);
	    if (c4) free(c4);
            if (c5) free(c5);
            sprintf(buf, "%d column data, expected 4 or 5 data cols.", m);
	    send_line(buf);
	    return 1;
	}
	if (m == 4) T = 0.0;
	if (n >= size) {
	    size += 10000;
	    c1 = (double *)realloc(c1, size * sizeof(double));
	    c2 = (double *)realloc(c2, size * sizeof(double));
	    c3 = (double *)realloc(c3, size * sizeof(double));
	    c4 = (double *)realloc(c4, size * sizeof(double));
	    c5 = (double *)realloc(c5, size * sizeof(double));
	    if (!c1 || !c2 || !c3 || !c4) {
        	if (c1) free(c1);
        	if (c2) free(c2);
        	if (c3) free(c3);
        	if (c4) free(c4);
                if (c5) free(c5);
                sprintf(buf, "Not enough memory %d.", size);
                send_line(buf);
        	return 1;
	    }
	}
	if (elevation > 0.0) X *= cos(elevation/RADTODEG);
	c1[n] = X*3600.0; c2[n] = Y*3600.0;
	c3[n] = A; c4[n] = P;
	c5[n] = T;
	n++;
    }
    
    ndata = n;
    
    fclose(fp);
    
    sprintf(buf, "Read %d entries from %s.", ndata, filename);
    send_line(buf);
    
    x_min=x_max=c1[0];
    y_min=y_max=c2[0];
    a_min=a_max=c3[0];
    p_min=p_max=c4[0];
    t_min=t_max=c5[0];
    for (n=1; n<ndata; n++) {
        if (c1[n] < x_min) x_min = c1[n];
        if (c1[n] > x_max) x_max = c1[n];
        if (c2[n] < y_min) y_min = c2[n];
        if (c2[n] > y_max) y_max = c2[n];
        if (c3[n] < a_min) a_min = c3[n];
        if (c3[n] > a_max) a_max = c3[n];
        if (c4[n] < p_min) p_min = c4[n];
        if (c4[n] > p_max) p_max = c4[n];
        if (c5[n] < p_min) p_min = c5[n];
        if (c5[n] > p_max) p_max = c5[n];
    }
    
    sprintf(buf, "x:%f,%f  y:%f,%f  1:%f,%f  2:%f,%f",
            x_min, x_max, y_min, y_max, a_min, a_max, p_min, p_max);
    send_line(buf);    
    
    nx = NINT((h->xright - h->xleft)/h->xspacing) + 1;
    ny = NINT((h->yupper - h->ylower)/h->yspacing) + 1;
    
    sprintf(buf, "Size of new maps: nX=%d nY=%d", nx, ny);
    send_line(buf);    
    
    a = new_map(get_maplist(), nx, ny);
    if (!a) {
    	return 1;
    }
    sprintf(a->name, "%s-1", StripSuffix(StripPath(filename)));
    a->type = MAP_POSPOS;
    a->date = *XS_localtime();
    a->coordType = COORD_TYPE_HOR;
    a->equinox = 0;
    a->sequence = 0;
    a->swapped = 0;
    a->memed = 0;
    a->ndata = nx * ny;
    a->b = beam;
    a->x0 = h->Az/RADTODEG;
    a->xleft  = h->xleft;
    a->xright = h->xright;
    a->xspacing = h->xspacing;
    a->i_no = nx;
    a->i_min = NINT(a->xleft/a->xspacing);
    a->i_max = a->i_min + nx - 1;
    a->y0 = h->El/RADTODEG;
    a->yspacing = h->yspacing;
    a->ylower = h->ylower;
    a->yupper = h->yupper;
    a->j_no = ny;
    a->j_min = NINT(a->ylower/a->yspacing);
    a->j_max = a->j_min + ny - 1;
    a->posAngle = 0.0;
    a->fMHz = 460000.0;
    a->fft = 0;
    b = copy_map(get_maplist(), a);
    if (!b) {
    	return 1;
    }
    sprintf(b->name, "%s-2", StripSuffix(StripPath(filename)));
    b->fMHz = 810000.0;
    
    wait = PostWaitingDialog(NULL, "Resample the APEX map data...",
	                     &scale, nx*ny);
    first = 1;  
    wid = h->width;
    lim2 = 9.0*wid*wid;
    for (i=0; i<nx; i++) {
      X = a->xleft + (double)i * a->xspacing;
      for (j=0; j<ny; j++) {
        Y = a->ylower + (double)j * a->yspacing;
	A = P = wsum = 0.0;
        if (wait) SetWaitingScale(scale, j + i*ny + 1);
	for (n=0; n<ndata; n++) {
	  dx = X - c1[n];
	  dy = Y - c2[n];
	  d2 = dx*dx + dy*dy;
	  if (d2 > lim2) continue;
	  w = exp(-ALPHA*d2/wid/wid);
	  A += w*c3[n];
	  P += w*c4[n];
	  wsum += w;
	}
	if (wsum > 0.0) {
	  A /= wsum;
	  P /= wsum;
	  a->d[i][j] = A;
	  a->e[i][j] = A/20.0;
	  a->f[i][j] = UNBLANK;
	  b->d[i][j] = P;
	  b->e[i][j] = P/20.0;
	  b->f[i][j] = UNBLANK;
	  if (first) {
	    first=0;
	    a_min = a_max = A;
	    p_min = p_max = P;
	  } else {
            if (A < a_min) a_min = A;
            if (A > a_max) a_max = A;
            if (P < p_min) p_min = P;
            if (P > p_max) p_max = P;
	  }
	} else {
	  a->d[i][j] = 0.0;
	  a->e[i][j] = 0.0;
	  a->f[i][j] = BLANK;
	  b->d[i][j] = 0.0;
	  b->e[i][j] = 0.0;
	  b->f[i][j] = BLANK;
	}
      }
    }
    sprintf(buf, "After resampling: 1st col:%f,%f  2nd col:%f,%f",
            a_min, a_max, p_min, p_max);
    send_line(buf);
    
    if (wait) XtDestroyWidget(wait);
    
    if (c1) free(c1);
    if (c2) free(c2);
    if (c3) free(c3);
    if (c4) free(c4);
    if (c5) free(c5);
   
    MapDraw(NULL, a, NULL);
    
    return 0;
}

int LoadFITS(char *filename, char *cmd)
{
    int i, j, k, l, type, cType;
    long int blank=0;
    double bzero=0.0, bscale=1.0, val, equinox;
    double xleft=0.0, xright=0.0, xspacing=0.0;
    double ylower=0.0, yupper=0.0, yspacing=0.0;
    double cc, vlsr=0.0, frest, vscale;
    int n=0, nX=0, nY=0, nC=0;
    int bitpix=0, has_blank=0;
    struct fitskey *keyword;
    DATE date;
    char *ptr;
    Beam b;
    MAP *m;
    scanPtr s, first=NULL, **S;
    DataSetPtr d;
    Widget wait=NULL, scale=NULL;
    
    list *get_maplist();
    MAP *new_map(), *GetPosPosMap();
    void MapDraw();
    scanPtr **AllocScanPtrArray();
    void FreeScanPtrArray();
    void SetCoordType(), SetViewMode();
    void obtain_map_info(Widget, char *, Widget);
    Widget PostWaitingDialog(Widget, char *, Widget *, int);
    void   SetWaitingScale(Widget, int);
    
    if (strcmp(cmd, "array")==0)
        type = FITS_ARRAY;
    else if (strcmp(cmd, "cube")==0)
        type = FITS_CUBE;
    else {
        sprintf(buf, "Internal error: Unknown cmd %s to LoadFITS().", cmd);
        send_line(buf);
        return 1;
    }
    
    ClearFITSwords();
    
    if ((keyword = readFITSheader(filename, 0)) == NULL) return 1;
    
    n = (int)keyword[KW_NAXIS].val.l;
    
    if (n > 0 && keyword[KW_NAXIS1].hit)
        nX = (int)keyword[KW_NAXIS1].val.l;
    if (n > 1 && keyword[KW_NAXIS2].hit)
        nY = (int)keyword[KW_NAXIS2].val.l;
    if (n > 2 && keyword[KW_NAXIS3].hit)
        nC = (int)keyword[KW_NAXIS3].val.l;

    if (type == FITS_ARRAY) {
        if (n < 2 || nX*nY == 0) {
            CloseFITSfile();
            return 1;
        }
        if (!nC) nC = 1;
    } else if (type == FITS_CUBE) {
        if (n < 3 || nX*nY*nC == 0) {
            CloseFITSfile();
            return 1;
        }
    } else {
        sprintf(buf, "Internal error: Unknown type %d in LoadFITS().", type);
        send_line(buf);
        CloseFITSfile();
        return 1;
    }
    
    bitpix = (int)keyword[KW_BITPIX].val.l;
    if (bitpix != FITS_TYPE_16BIT && bitpix != FITS_TYPE_32BIT &&
        bitpix != FITS_TYPE_FLOAT && bitpix != FITS_TYPE_DOUBLE &&
	bitpix != FITS_TYPE_8BIT) {
        sprintf(buf, "Can't read BITPIX=%d type FITS.", bitpix);
        send_line(buf);
        CloseFITSfile();
        return 1;
    }
    
    if (keyword[KW_CTYPE1].hit && keyword[KW_CTYPE2].hit) {
        if (strncmp(keyword[KW_CTYPE1].val.str, "RA", 2)==0 &&
            strncmp(keyword[KW_CTYPE2].val.str, "DEC", 3)==0) {
            cType = COORD_TYPE_EQU;
        } else if (strncmp(keyword[KW_CTYPE1].val.str, "AZ", 2)==0 &&
                   strncmp(keyword[KW_CTYPE2].val.str, "EL", 2)==0) {
            cType = COORD_TYPE_HOR;
        } else if (strncmp(keyword[KW_CTYPE1].val.str, "GLON", 4)==0 &&
                   strncmp(keyword[KW_CTYPE2].val.str, "GLAT", 4)==0) {
            cType = COORD_TYPE_GAL;
        } else {
            cType = COORD_TYPE_UNKNOWN;
            sprintf(buf, "Warning! No known axes (%s,%s) keywords.",
                    keyword[KW_CTYPE1].val.str,
                    keyword[KW_CTYPE2].val.str);
            send_line(buf);
        }
    } else {
        cType = COORD_TYPE_UNKNOWN;
        sprintf(buf, "Warning! No CTYPE keywords at all, assuming pixels");
        send_line(buf);
    }
    
    if (keyword[KW_EQUINOX].hit) {
        equinox = keyword[KW_EQUINOX].val.d;
    } else if (keyword[KW_EPOCH].hit) {
        equinox = keyword[KW_EPOCH].val.d;
    } else {
        equinox = 0.0;
    }
    
    bscale = 1.0; bzero = 0.0;
    if (keyword[KW_BSCALE].hit) bscale = keyword[KW_BSCALE].val.d;
    if (keyword[KW_BZERO].hit) bzero = keyword[KW_BZERO].val.d;
    
    if (keyword[KW_BLANK].hit) has_blank = 1;
    if (has_blank) {
        blank = keyword[KW_BLANK].val.l;
    }
     
    /* Not yet implemented when reading FITS
    if (keyword[KW_TIMESYS].hit) {
    }
    */
    date.Year = date.Month = date.Day = 0;
    date.Hour = date.Min = date.Sec = 0;
    if (keyword[KW_DATE_OBS].hit) {
        if (strchr(keyword[KW_DATE_OBS].val.str, '-')) { /* New DATE format */
            ptr = strtok(keyword[KW_DATE_OBS].val.str, "-\0");
            if (ptr) date.Year = atoi(ptr);
            ptr = strtok(NULL, "-\0");
            if (ptr) date.Month = atoi(ptr);
            ptr = strtok(NULL, "T\0");
            if (ptr) date.Day = atoi(ptr);
            ptr = strtok(NULL, ":\0");
            if (ptr) {
                date.Hour = atoi(ptr);
                ptr = strtok(NULL, ":\0");
                if (ptr) date.Min = atoi(ptr);
                ptr = strtok(NULL, ":\0");
                if (ptr) date.Sec = (int)(atof(ptr)+0.5);
            }
        } else { /* Old 1900-1999 DATE-OBS FITS format */
            ptr = strtok(keyword[KW_DATE_OBS].val.str, "/\0");
            if (ptr) date.Day = atoi(ptr);
            ptr = strtok(NULL, "/\0");
            if (ptr) date.Month = atoi(ptr);
            ptr = strtok(NULL, "/\0");
            if (ptr) {
                date.Year = atoi(ptr);
                if (date.Year < 100) date.Year += 1900;
            }
        }
    }
    if (keyword[KW_UTC].hit || keyword[KW_UT].hit) {
        if (keyword[KW_UT].hit)
            ptr = strtok(keyword[KW_UT].val.str,":\0");
	else
            ptr = strtok(keyword[KW_UTC].val.str,":\0");
        if (ptr) date.Hour = atoi(ptr);
        ptr = strtok(NULL,":\0");
        if (ptr) date.Min = atoi(ptr);
        ptr = strtok(NULL,":\0");
        if (ptr) date.Sec = atoi(ptr);
    }
    
    if (keyword[KW_BMAJ].hit) {
        b.maj = 3600.0*keyword[KW_BMAJ].val.d;
        if (keyword[KW_BMIN].hit) {
            b.min = 3600.0*keyword[KW_BMIN].val.d;
        } else {
            b.min = b.maj;
        }
        if (keyword[KW_BPA].hit) {
            b.PA = keyword[KW_BPA].val.d;
        } else {
            b.PA = 0.0;
        }
    } else {
        b.maj = 0.0;
        b.min = 0.0;
        b.PA  = 0.0;
    }
        
    if (AllocAndReadFITS(bitpix, nX*nY*nC, 1)) return 1;

#ifdef DEBUG    
    printf("nX, nY, nC = %d,%d,%d = %d  bitpix=%d\n", nX, nY, nC, nX*nY*nC, bitpix);
#endif
    
    if (type == FITS_ARRAY) {
        m = new_map(get_maplist(), nX, nY);
        if (!m) {
            FreeFITSData();
            return 1;
        }
        m->type = MAP_POSPOS;
        m->date = date;
        m->coordType = cType;
        m->equinox = equinox;
        m->sequence = 0;
        m->swapped = 0;
        m->memed = 0;
        m->ndata = nX * nY;
        m->b = b;
        if (keyword[KW_OBJECT].hit)
            strcpy(m->name, keyword[KW_OBJECT].val.str);
        else
            strcpy(m->name, "<unknown>");
        if (keyword[KW_LINE].hit)
            strcpy(m->molecule, keyword[KW_LINE].val.str);
        else
            strcpy(m->molecule, "<unknown>");
        if (keyword[KW_CDELT1].hit) {
            xspacing = keyword[KW_CDELT1].val.d*3600.0;
        } else if (keyword[KW_CD11].hit || keyword[KW_CD21].hit) {
            xspacing = 3600.0*sqrt(
                            keyword[KW_CD11].val.d*keyword[KW_CD11].val.d +
                            keyword[KW_CD21].val.d*keyword[KW_CD21].val.d);
        } else {
            xspacing = -1.0;
        }
        if (keyword[KW_CDELT2].hit) {
            yspacing = keyword[KW_CDELT2].val.d*3600.0;
        } else if (keyword[KW_CD22].hit || keyword[KW_CD12].hit) {
            yspacing = 3600.0*sqrt(
                            keyword[KW_CD22].val.d*keyword[KW_CD22].val.d +
                            keyword[KW_CD12].val.d*keyword[KW_CD12].val.d);
        } else {
            yspacing = -1.0;
        }
        if (keyword[KW_CRVAL1].hit) {
            m->x0 = keyword[KW_CRVAL1].val.d/RADTODEG;
            m->xspacing = xspacing;
            m->xleft  = (1.0-keyword[KW_CRPIX1].val.d)*m->xspacing;
            m->xright = m->xleft + (double)(nX-1)*m->xspacing;
            m->i_min = NINT(m->xleft/m->xspacing);
            m->i_max = m->i_min + nX - 1;
        } else {
            m->x0 = 0.0;
            m->xspacing = -1.0;
            m->xleft = (double)nX;
            m->xright = 1.0;
        }
        if (keyword[KW_CRVAL2].hit) {
            m->y0 = keyword[KW_CRVAL2].val.d/RADTODEG;
            m->yspacing = yspacing;
            m->ylower = (1.0-keyword[KW_CRPIX2].val.d)*m->yspacing;
            m->yupper = m->ylower + (double)(nY-1)*m->yspacing;
            m->j_min = NINT(m->ylower/m->yspacing);
            m->j_max = m->j_min + nY - 1;
        } else {
            m->y0 = 0.0;
            m->yspacing = 1.0;
            m->ylower = 1.0;
            m->yupper = (double)nY;
        }
        if (n >= 3 && keyword[KW_CRVAL3].hit) {
            m->v = keyword[KW_CRVAL3].val.d/1.0e3;
            m->dv = keyword[KW_CDELT3].val.d/1.0e3;
        } else {
            m->v = 0.0;
            m->dv = 0.0;
        }
        if (keyword[KW_CROTA2].hit) {
            m->posAngle = keyword[KW_CROTA2].val.d/RADTODEG;
        } else if (keyword[KW_CROTA1].hit) {
            m->posAngle = keyword[KW_CROTA1].val.d/RADTODEG;
        } else {
            m->posAngle = 0.0;
        }
        if (keyword[KW_RESTFREQ].hit) {
            m->fMHz = keyword[KW_RESTFREQ].val.d/1.0e6;
        }
        n = 0;
        for (j=0; j<nY; j++) {
            for (i=0; i<nX; i++) {
                val = GetFITSValue(bitpix, n);
                m->d[i][j] = bzero + bscale*val;
                m->e[i][j] = 0.0;
                if (bitpix == FITS_TYPE_FLOAT)
                    if (isnan(val))
                        m->f[i][j] = BLANK;
                    else
                        m->f[i][j] = UNBLANK;
                else if (has_blank && BVAL((long int)val, blank))
                    m->f[i][j] = BLANK;
                else
                    m->f[i][j] = UNBLANK;
                n++;
            }
        }
        MapDraw(NULL, m, NULL);
        if (m->coordType == COORD_TYPE_GAL) {
            sprintf(buf, "FITS array (%s: %s %s %dx%d) read.",
                m->name, GetLongStr(m->x0), GetLatStr(m->y0), nX, nY);
        } else {
            sprintf(buf, "FITS array (%s: %s %s %dx%d) read.",
                m->name, GetRAStr(m->x0), GetDECStr(m->y0), nX, nY);
        }
    } else {    
        S = AllocScanPtrArray(nX, nY);
        if (!S) {
            sprintf(buf, "Warning! Out of memory at npixels=%dx%d", nX, nY);
            send_line(buf);
            FreeFITSData();
            return 1;
        }

        d = new_dataset(get_listlist(), "FITS cube", NULL);
        if (!d) {
            send_line("Warning! Out of memory when allocating data set.");
            FreeScanPtrArray(S, nX, nY);
            FreeFITSData();
            return 1;
        }
        
        if (nX*nY > 3*WAITSPECTRA)
            wait = PostWaitingDialog(NULL, "Allocating memory for cube...",
                                     &scale, nX*nY);
        
        n = 0;
        for (j=0; j<nY; j++) {
            for (i=0; i<nX; i++) {
                if (wait) SetWaitingScale(scale, i + j*nX + 1);
                val = GetFITSValue(bitpix, n++);
#ifdef DEBUG		
		if (isnan(val)) val = 0;
		if (i==50 && j==10) printf("n=%d (%d,%d) %d %f\n", n, i, j, bitpix, val);
#endif
                if (bitpix == FITS_TYPE_FLOAT && isnan(val)) {
                    S[i][j] = NULL;
                    continue;
                }
                /* if ((long int)val != IRAMCUBE)
                    printf("%d,%d  val=%ld\n", i, j, (long int)val); */
                if (has_blank && BVAL(val, blank)) {
                    S[i][j] = NULL;
                    continue;
                }
                if (!first) {
                    s = first = new_scan(d, nC);
                    if (s) {
                    if (keyword[KW_OBJECT].hit) {
                        strcpy(s->name, keyword[KW_OBJECT].val.str);
                        sprintf(d->name, "FITS cube %s (%dx%dx%d)",
                                keyword[KW_OBJECT].val.str, nX, nY, nC);
                    } else {
                        strcpy(s->name, "<unknown>");
                        sprintf(d->name, "FITS cube (%dx%dx%d)",
                                nX, nY, nC);
                    }
                    if (keyword[KW_LINE].hit)
                        strcpy(s->molecule, keyword[KW_LINE].val.str);
                    else
                        strcpy(s->molecule, "<unknown>");
        	    if (keyword[KW_CDELT1].hit) {
        		xspacing = keyword[KW_CDELT1].val.d*3600.0;
        	    } else if (keyword[KW_CD11].hit || keyword[KW_CD21].hit) {
        		xspacing = -3600.0*sqrt(
                        		keyword[KW_CD11].val.d*keyword[KW_CD11].val.d +
                        		keyword[KW_CD21].val.d*keyword[KW_CD21].val.d);
        	    } else {
        		xspacing = -1.0;
        	    }
                    if (keyword[KW_CRVAL1].hit) {
                        s->x0 = keyword[KW_CRVAL1].val.d/RADTODEG;
/*                        xspacing = keyword[KW_CDELT1].val.d*3600.0; */
                        xleft  = (1.0-keyword[KW_CRPIX1].val.d)*xspacing;
                        xright = xleft + (double)(nX-1)*xspacing;
                    } else {
                        s->x0 = 0.0;
/*                        xspacing = 1.0; */
                        xleft = 1.0;
                        xright = (double)nX;
                    }
        	    if (keyword[KW_CDELT2].hit) {
        		yspacing = keyword[KW_CDELT2].val.d*3600.0;
        	    } else if (keyword[KW_CD22].hit || keyword[KW_CD12].hit) {
        		yspacing = 3600.0*sqrt(
                        		keyword[KW_CD22].val.d*keyword[KW_CD22].val.d +
                        		keyword[KW_CD12].val.d*keyword[KW_CD12].val.d);
        	    } else {
        		yspacing = 1.0;
        	    }
                    if (keyword[KW_CRVAL2].hit) {
                        s->y0 = keyword[KW_CRVAL2].val.d/RADTODEG;
/*                        yspacing = keyword[KW_CDELT2].val.d*3600.0; */
                        ylower = (1.0-keyword[KW_CRPIX2].val.d)*yspacing;
                        yupper = ylower + (double)(nY-1)*yspacing;
                    } else {
                        s->y0 = 0.0;
/*                        yspacing = 1.0; */
                        ylower = 1.0;
                        yupper = (double)nY;
                    }
                    if (keyword[KW_VELO_LSR].hit) {
                        vlsr = keyword[KW_VELO_LSR].val.d/1000.0;
                    }
                    s->coordType = cType;
                    SetCoordType(cType);
                    s->equinox = equinox;
                    s->date = date;
                    s->scan_no = 1000;
                    s->b = b;
                    s->gaussFit = 0;
                    d->sequence = 0;
                    d->gridded = 1;
                    d->dx = xspacing;
                    d->dy = yspacing;

                    if (keyword[KW_CRVAL3].hit) {
                        cc = keyword[KW_CRPIX3].val.d;
                        if (strncmp(keyword[KW_CTYPE3].val.str, "VEL", 3)==0 ||
			    strncmp(keyword[KW_CTYPE3].val.str, "VRAD", 4)==0) {
			    if (strncmp(keyword[KW_CUNIT3].val.str, "km/s", 4) == 0 ||
				strncmp(keyword[KW_CUNIT3].val.str, "KM/S", 4) == 0) {
				vscale = 1.0;
			    } else { /* assume m/s */
			        vscale = 1000.0;
			    }
                            vlsr = keyword[KW_CRVAL3].val.d/vscale;
			    if (keyword[KW_CDELT3].hit) {
        			s->velres = keyword[KW_CDELT2].val.d/vscale;
			    } else if (keyword[KW_CD33].hit) {
        			s->velres = keyword[KW_CD33].val.d/vscale;
			    } else {
        			s->velres = 1.0;
			    }
                            s->vel0 =  vlsr - (cc-1.0)*s->velres;
                            frest = 100.0;
                            if (keyword[KW_RESTFREQ].hit)
                                frest = keyword[KW_RESTFREQ].val.d/1.0e9;
                            s->freqres = -s->velres / SPEEDOFLIGHT * frest;
                            s->freq0 = frest - (cc-1.0)*s->freqres;
                            s->freqn = s->freq0 + (double)(nC-1)*s->freqres;
                        } else {
                            frest = keyword[KW_CRVAL3].val.d/1.0e9;
                            s->freqres = keyword[KW_CDELT3].val.d/1.0e9;
                            s->freq0 =  frest - (cc-1.0)*s->freqres;
                            s->freqn = s->freq0 + (double)(nC-1)*s->freqres;
                            s->velres = -s->freqres/frest * SPEEDOFLIGHT;
                            s->vel0 = vlsr - (cc-1.0)*s->velres;
                        }
		        if (keyword[KW_IMAGFREQ].hit) {
                          s->lofreq = (keyword[KW_IMAGFREQ].val.d/1.0e9 + frest)/2.0;
                        }
                      }
                    }
                } else {
                    s = copy_scan(d, first);
                }
                if (!s) {
                    sprintf(buf, "Warning! Out of memory at %d spectra.",
                            count_scans(d));
                    send_line(buf);
                    FreeFITSData();
                    FreeScanPtrArray(S, nX, nY);
                    if (wait) XtDestroyWidget(wait);
                    vP->from = vP->to = d;
                    vP->s = first;
                    return 1;
                }
                S[i][j] = s;
                s->scan_no = i + j*nX + 1;
                if (cType == COORD_TYPE_HOR) {
                    s->tx = s->aoffset = xleft  + (double)i * xspacing;
                    s->ty = s->eoffset = ylower + (double)j * yspacing;
                } else {
                    s->tx = s->xoffset = xleft  + (double)i * xspacing;
                    s->ty = s->yoffset = ylower + (double)j * yspacing;
                }
            }
        }
        
        if (wait) XtDestroyWidget(wait);
        
        if (nX*nY > 3*WAITSPECTRA)
            wait = PostWaitingDialog(NULL, "Reading cube...", &scale, nC);

        n = l = 0;
        for (k=0; k<nC; k++) {
            if (wait) SetWaitingScale(scale, k + 1);
            for (j=0; j<nY; j++) {
                for (i=0; i<nX; i++) {
                    val = GetFITSValue(bitpix, n++);
#ifdef DEBUG		   
		    if (isnan(val)) val = 0.0;
#endif		
                    if ((s = S[i][j])) {
                        s->d[k] = bzero + bscale*val;
                        s->e[k] = 0.0;
			l++;
                    }
                }
            }
        }
        
        if (wait) XtDestroyWidget(wait);
        
        vP->from = vP->to = d;
        
        FreeScanPtrArray(S, nX, nY);
	
	if (l == 0) {
            sprintf(buf, "Warning! Invalid data cube, only nan/blank positions read.");
            send_line(buf);
            FreeFITSData();
            vP->s = first;
	    return 1;
	}
        
        obtain_map_info(NULL, "no_update_map_data", NULL);
        
        if (count_scans(d) > 1 && count_scans(d) < 226) {
            SetViewMode(SHOW_ALLSPE, first, GetPosPosMap(), vP->p);
        } else {
            SetViewMode(SHOW_SPE, first, vP->m, vP->p);
        }
        if (first->coordType == COORD_TYPE_GAL) {
            sprintf(buf, "FITS cube (%s: %s %s %dx%dx%d) read.",
                first->name, GetLongStr(first->x0), GetLatStr(first->y0),
                nX, nY, nC);
        } else {
            sprintf(buf, "FITS cube (%s: %s %s %dx%dx%d) read.",
                first->name, GetRAStr(first->x0), GetDECStr(first->y0),
                nX, nY, nC);
        }
    }
    
    FreeFITSData();
    
    send_line(buf);
    
    return 0;
}

int get_fits(char *scanname, FDATA *fd)
{
    double bzero, bscale;
    double cType, cc, orbit, val1, rf;
    int i, n, polType=POL_UNKNOWN;
    int bitpix=0, velaxis=0;
    char *ptr;
    struct fitskey *keyword;
    
    void DRP2FD();
    double rta(float);
    
    ClearFITSwords();
    
    if ((keyword = readFITSheader(scanname, 0)) == NULL) return -1;

    if (keyword[KW_XTENSION].hit || keyword[KW_EXTEND].hit) {
        if (strcmp(keyword[KW_XTENSION].val.str, "BINTABLE")==0) {
            send_line("Expecting a standard FITS header---not a binary table.");
            CloseFITSfile();
            ClearFITSwords();
            return -1;
        }
    }
    
    n = (int)keyword[KW_NAXIS].val.l;
    if (keyword[KW_NAXIS].val.l > 1) {
        for (i=2; i<n; i++) {
            if (keyword[KW_NAXIS+i].val.l != 1 && keyword[KW_NAXIS1].val.l > 1) {
                sprintf(buf,
                        "FITS scan has not one axis (NAXIS=%d, NAXIS%d=%d).",
                        n, i, (int)keyword[KW_NAXIS+i].val.l);
                send_line(buf);
                CloseFITSfile();
                ClearFITSwords();
                return -1;
            }
        }
    }
    n = (int)keyword[KW_NAXIS1].val.l;
    if (n == 1 && keyword[KW_NAXIS].val.l > 1)
         n = (int)keyword[KW_NAXIS2].val.l;
    bitpix = (int)keyword[KW_BITPIX].val.l;
#ifdef DEBUG
    printf("Bitpix=%d\n", bitpix);
#endif
    if (bitpix != FITS_TYPE_16BIT && bitpix != FITS_TYPE_32BIT &&
        bitpix != FITS_TYPE_FLOAT && bitpix != FITS_TYPE_DOUBLE) {
        sprintf(buf, "Can't read BITPIX=%d type FITS.", bitpix);
        send_line(buf);
        CloseFITSfile();
        ClearFITSwords();
        return -1;
    }
    
    if (CheckDataSize(n) < n) {
        sprintf(buf, "Can't allocate enough memory for NChan=%d.", n);
        send_line(buf);
        CloseFITSfile();
        ClearFITSwords();
        return -1;
    }

    for (i=0; i<10; i++) OnScan.work[i] = 0.0;
    for (i=0; i<31; i++) OnScan.flag[i] = 0;
    
    OnScan.NChannel = (unsigned short)n;
    XScan.NChannel = n;
#ifdef DEBUG
    printf("NChannel=%d\n", n);
#endif
    if (strncmp(keyword[KW_CTYPE1].val.str, "FREQ", 4) == 0) {
        val1 = 0.0;
	rf = 0.0;
        if (keyword[KW_CRVAL1].hit) val1=keyword[KW_CRVAL1].val.d/1.0e6;
        if (keyword[KW_RESTFREQ].hit) rf=keyword[KW_RESTFREQ].val.d/1.0e6;
        
        if (keyword[KW_INSTRUME].hit) {
            if (strncmp(keyword[KW_INSTRUME].val.str, "Mopra", 5) == 0) {
                OnScan.RestFreq = keyword[KW_CRVAL1].val.d/1.0e6;
            } else {
	        if (val1 == 0.0 || val1 == rf) OnScan.RestFreq = rf;
	        else OnScan.RestFreq = val1 + rf;
            }
        } else {
	    if (val1 == 0.0 || val1 == rf) OnScan.RestFreq = rf;
	    else OnScan.RestFreq = val1 + rf;
        }
        OnScan.FreqRes   = keyword[KW_CDELT1].val.d/1.0e6;
        cc = keyword[KW_CRPIX1].val.d - (double)(CenterCh(&OnScan, &XScan)+1);
        OnScan.RestFreq -= OnScan.FreqRes*cc;
        OnScan.Bandwidth = OnScan.FreqRes*XScan.NChannel;
    } else if (strncmp(keyword[KW_CTYPE1].val.str, "VEL", 3) == 0) {
        OnScan.RestFreq = 1.0e5;
        if (keyword[KW_RESTFREQ].hit)
            OnScan.RestFreq = keyword[KW_RESTFREQ].val.d/1.0e6;
        OnScan.VelRes = keyword[KW_CDELT1].val.d/1.0e3;
	OnScan.VSource = keyword[KW_CRVAL1].val.d/1.0e3;
        OnScan.FreqRes = -OnScan.VelRes/C * OnScan.RestFreq * 1.0e3;
        cc = keyword[KW_CRPIX1].val.d - (double)(CenterCh(&OnScan, &XScan) + 1);
	OnScan.VSource -= OnScan.VelRes * cc;
        OnScan.RestFreq -= OnScan.FreqRes * cc; 
        OnScan.Bandwidth = OnScan.FreqRes*XScan.NChannel;
	velaxis = 1;
    } else {
        sprintf(buf, "Not velocity or frequency on first axis (CTYPE1=%s).",
                keyword[KW_CTYPE1].val.str);
        send_line(buf);
        CloseFITSfile();
        ClearFITSwords();
        return -1;
    }

    if (keyword[KW_IMAGFREQ].hit) {
        XScan.LOFreq = (keyword[KW_IMAGFREQ].val.d/1.0e6 + OnScan.RestFreq)/2.0;
        OnScan.FirstIF = fabs(XScan.LOFreq - OnScan.RestFreq);
    } else {
        if (keyword[KW_TELESCOP].hit) {
	  if (strncmp(keyword[KW_TELESCOP].val.str, "OSO-20M", 7)==0) {
	    OnScan.FirstIF = 4000.0;
	    if (OnScan.RestFreq > 105000.0) {
        	XScan.LOFreq = OnScan.RestFreq - OnScan.FirstIF;
	    } else {
        	XScan.LOFreq = OnScan.RestFreq + OnScan.FirstIF;
	    }
	  } else if (strncmp(keyword[KW_TELESCOP].val.str, "APEX-ASC-1", 10)==0) {
	    OnScan.FirstIF = 6000.0;
	    if (OnScan.RestFreq < 308000.0) {
        	XScan.LOFreq = OnScan.RestFreq + OnScan.FirstIF;
	    } else if (OnScan.RestFreq < 325000.0) {
        	XScan.LOFreq = OnScan.RestFreq - OnScan.FirstIF;
	    } else if (OnScan.RestFreq < 343000.0) {
        	XScan.LOFreq = OnScan.RestFreq + OnScan.FirstIF;
	    } else {
        	XScan.LOFreq = OnScan.RestFreq - OnScan.FirstIF;
	    }
	  } else {
	    OnScan.FirstIF = 4000.0;
	    if (OnScan.RestFreq > 105000.0) {
        	XScan.LOFreq = OnScan.RestFreq - OnScan.FirstIF;
	    } else {
        	XScan.LOFreq = OnScan.RestFreq + OnScan.FirstIF;
	    }
	  }
	} else {
	    OnScan.FirstIF = 4000.0;
	    if (OnScan.RestFreq > 105000.0) {
        	XScan.LOFreq = OnScan.RestFreq - OnScan.FirstIF;
	    } else {
        	XScan.LOFreq = OnScan.RestFreq + OnScan.FirstIF;
	    }
	}
    }
    
    if (keyword[KW_OBSFREQ].hit) {
        OnScan.SkyFreq = keyword[KW_OBSFREQ].val.d/1.0e6;
    } else {
        OnScan.SkyFreq = OnScan.RestFreq;
    }
    
    if (keyword[KW_CTYPE2].hit && keyword[KW_CTYPE3].hit) {
        if (strncmp(keyword[KW_CTYPE2].val.str, "RA", 2)==0 &&
            strncmp(keyword[KW_CTYPE3].val.str, "DEC", 3)==0) {
            cType = COORD_TYPE_EQU;
        } else if (strncmp(keyword[KW_CTYPE2].val.str, "AZ", 2)==0 &&
                   strncmp(keyword[KW_CTYPE3].val.str, "EL", 2)==0) {
            cType = COORD_TYPE_HOR;
        } else if (strncmp(keyword[KW_CTYPE2].val.str, "GLON", 4)==0 &&
                   strncmp(keyword[KW_CTYPE3].val.str, "GLAT", 4)==0) {
            cType = COORD_TYPE_GAL;
        } else if (strncmp(keyword[KW_CTYPE2].val.str, "ANGLE", 5)==0 &&
                   strncmp(keyword[KW_CTYPE3].val.str, "ANGLE", 5)==0) {
            cType = COORD_TYPE_SCAN;
        } else {
            cType = COORD_TYPE_UNKNOWN;
            sprintf(buf, "Warning! Unknown axes (%s,%s) keywords.",
                    keyword[KW_CTYPE2].val.str,
                    keyword[KW_CTYPE3].val.str);
            send_line(buf);
        }
    } else {
        cType = COORD_TYPE_UNKNOWN;
        sprintf(buf, "Warning! No CTYPE keywords at all, assuming pixels.");
        send_line(buf);
    }
    
    if (keyword[KW_OBSTYPE].hit) {
        if (strncmp(keyword[KW_OBSTYPE].val.str, "SK1", 3)==0 ||
            strncmp(keyword[KW_OBSTYPE].val.str, "sk1", 3)==0) {
            OnScan.ObsMode = 3;
        } else if (strncmp(keyword[KW_OBSTYPE].val.str, "SK2", 3)==0 ||
                   strncmp(keyword[KW_OBSTYPE].val.str, "sk2", 3)==0) {
            OnScan.ObsMode = 4;
        } else if (strncmp(keyword[KW_OBSTYPE].val.str, "REF", 3)==0 ||
                   strncmp(keyword[KW_OBSTYPE].val.str, "ref", 3)==0) {
            OnScan.ObsMode = 2;
        } else if (strncmp(keyword[KW_OBSTYPE].val.str, "CAL", 3)==0 ||
                   strncmp(keyword[KW_OBSTYPE].val.str, "cal", 3)==0) {
            OnScan.ObsMode = 1;
        } else {
            OnScan.ObsMode = 0;
        }
    } else {
        OnScan.ObsMode = 0;
    }
    
    OnScan.CSystem = cType;

    OnScan.Ctrl = 0;
    OnScan.Slength = HEADER + 2*XScan.NChannel;
    OnScan.PosAngle = 0.0;
    if (cType == COORD_TYPE_EQU || cType == COORD_TYPE_GAL) {
        OnScan.Longitude = keyword[KW_CRVAL2].val.d*PI/180.0;
        OnScan.LMapOff   = keyword[KW_CDELT2].val.d*PI/180.0;
        if (OnScan.PosAngle == 0.0)
            OnScan.PosAngle = keyword[KW_CROTA2].val.d*PI/180.0;
        if (keyword[KW_MAPTILT].hit)
            OnScan.PosAngle = keyword[KW_MAPTILT].val.d*PI/180.0;
        
        OnScan.Latitude = keyword[KW_CRVAL3].val.d*PI/180.0;
        OnScan.BMapOff  = keyword[KW_CDELT3].val.d*PI/180.0;
        if (OnScan.PosAngle == 0.0)
            OnScan.PosAngle = keyword[KW_CROTA3].val.d*PI/180.0;
        if (keyword[KW_MAPTILT].hit)
            OnScan.PosAngle = keyword[KW_MAPTILT].val.d*PI/180.0;
    } else if (cType == COORD_TYPE_HOR) {
        if (keyword[KW_RA ].hit)
            OnScan.Longitude = keyword[KW_RA ].val.d*PI/180.0;
        else
            OnScan.Longitude = 0.0;
        
        if (keyword[KW_DEC].hit)
            OnScan.Latitude = keyword[KW_DEC].val.d*PI/180.0;
        else
            OnScan.Latitude = 0.0;
    } else {
        if (keyword[KW_CRVAL2].hit)
            OnScan.Longitude = keyword[KW_CRVAL2].val.d;
        else
            OnScan.Longitude = 0.0;
        if (keyword[KW_CDELT2].hit)
            OnScan.LMapOff  = keyword[KW_CDELT2].val.d;
        else
            OnScan.LMapOff  = 0.0;
        
        if (keyword[KW_CRVAL3].hit)
            OnScan.Latitude = keyword[KW_CRVAL3].val.d;
        else
            OnScan.Latitude = 0.0;
        if (keyword[KW_CDELT3].hit)
            OnScan.BMapOff  = keyword[KW_CDELT3].val.d;
        else
            OnScan.BMapOff  = 0.0;
    }
    
    OnScan.SubScan = 0;
    if (keyword[KW_SCAN].hit) {
        n = keyword[KW_SCAN].val.l;
        if (n >= 32767) n %= 10000;
        OnScan.ScanNo = (short)n;
    } else if (keyword[KW_ORBIT].hit) {
        orbit = keyword[KW_ORBIT].val.d;
        OnScan.ScanNo = (int)orbit;
        OnScan.SubScan = NINT(10000.0*(orbit - (double)OnScan.ScanNo));
    }
#ifdef DEBUG
    printf("KW_DATE_OBS...\n");
#endif
    OnScan.UTHour = OnScan.UTMin = OnScan.UTSec = 0;
    /* Not yet implemented when reading FITS
    if (keyword[KW_TIMESYS].hit) {
    }
    */
    if (keyword[KW_DATE_OBS].hit) {
        if (strchr(keyword[KW_DATE_OBS].val.str, '-')) { /* New DATE format */
            ptr = strtok(keyword[KW_DATE_OBS].val.str, "-\0");
            if (ptr) OnScan.Year = atoi(ptr);
            ptr = strtok(NULL, "-\0");
            if (ptr) OnScan.Month = atoi(ptr);
            ptr = strtok(NULL, "T\0");
            if (ptr) OnScan.Day = atoi(ptr);
            ptr = strtok(NULL, ":\0");
            if (ptr) {
                OnScan.UTHour = atoi(ptr);
                ptr = strtok(NULL, ":\0");
                if (ptr) OnScan.UTMin = atoi(ptr);
                ptr = strtok(NULL, ":\0");
                if (ptr) OnScan.UTSec = (int)(atof(ptr)+0.5);
            }
        } else { /* Old 1900-1999 DATE-OBS FITS format */
            ptr = strtok(keyword[KW_DATE_OBS].val.str,"/\0");
            if (ptr) OnScan.Day = atoi(ptr);
            ptr = strtok(NULL,"/\0");
            if (ptr) OnScan.Month = atoi(ptr);
            ptr = strtok(NULL,"/\0");
            if (ptr) {
                OnScan.Year = atoi(ptr);
                if (OnScan.Year < 100) OnScan.Year += 1900;
            }
        }
    } else
        OnScan.Year = OnScan.Month = OnScan.Day = 0;
#ifdef DEBUG
    printf("KW_UTC...\n");
#endif
    if (keyword[KW_UTC].hit || keyword[KW_UT].hit) {
        if (keyword[KW_UT].hit)
            ptr = strtok(keyword[KW_UT].val.str,":\0");
	else
            ptr = strtok(keyword[KW_UTC].val.str,":\0");
        if (ptr) OnScan.UTHour = atoi(ptr);
        ptr = strtok(NULL,":\0");
        if (ptr) OnScan.UTMin = atoi(ptr);
        ptr = strtok(NULL,":\0");
        if (ptr) OnScan.UTSec = atoi(ptr);
    }
#ifdef DEBUG
    printf("KW_LST...\n");
#endif
    if (keyword[KW_LST].hit) {
        ptr = strtok(keyword[KW_LST].val.str,":\0");
        if (ptr) OnScan.STHour = atoi(ptr);
        ptr = strtok(NULL,":\0");
        if (ptr) OnScan.STMin = atoi(ptr);
        ptr = strtok(NULL,":\0");
        if (ptr) OnScan.STSec = atoi(ptr);
    } else
        OnScan.STHour = OnScan.STMin = OnScan.STSec = 0;
    OnScan.Backend = 0;
    OnScan.Frontend = 0;
    OnScan.MapX = 0;
    OnScan.MapY = 0;
    if (keyword[KW_JDATE].hit) {
        OnScan.JulDate = (long)(keyword[KW_JDATE].val.d+0.5);
    } else
        OnScan.JulDate = 0L;
    if (keyword[KW_OBJECT].hit) {
        strncpy(OnScan.Name, keyword[KW_OBJECT].val.str, 11);
    } else
        strncpy(OnScan.Name , "            ", 11);
    strncpy(OnScan.Project ,"    ",4);
    if (keyword[KW_OBSERVER].hit) {
        strncpy(OnScan.Observer, keyword[KW_OBSERVER].val.str, 15);
    } else
        strncpy(OnScan.Observer, "                ", 15);
    if (keyword[KW_LINE    ].hit) {
        strncpy(OnScan.Molecule, keyword[KW_LINE    ].val.str, 17);
    } else
        strncpy(OnScan.Molecule,"                    ",17);
    if (keyword[KW_INSTRUME].hit) {
        strncpy(OnScan.Program, keyword[KW_INSTRUME].val.str, 15);
    } else
        strncpy(OnScan.Program, "                ", 15);
    if (keyword[KW_TOUTSIDE].hit) {
        OnScan.AirTemp  = keyword[KW_TOUTSIDE].val.d;
    } else
        OnScan.AirTemp  = 0.0;
    if (keyword[KW_PRESSURE].hit) {
        OnScan.Pressure = keyword[KW_PRESSURE].val.d/100.0;
    } else
        OnScan.Pressure = 0.0;
    if (keyword[KW_HUMIDITY].hit) {
        OnScan.Humidity = keyword[KW_HUMIDITY].val.d*100.0;
    } else
        OnScan.Humidity = 0.0;
    if (keyword[KW_EQUINOX].hit) {
        OnScan.Equinox = keyword[KW_EQUINOX].val.d;
    } else if (keyword[KW_EPOCH].hit) {
        OnScan.Equinox = keyword[KW_EPOCH].val.d;
    } else
        OnScan.Equinox = 0.0;
    OnScan.EquiNow = 0.0;
    if (keyword[KW_RA].hit) {
        OnScan.Long2000 = keyword[KW_RA].val.d*PI/180.0;
    } else {
        OnScan.Long2000 = 0.0;
    }
    if (keyword[KW_DEC].hit) {
        OnScan.Lat2000 = keyword[KW_DEC].val.d*PI/180.0;
    } else {
        OnScan.Lat2000 = 0.0;
    }
    if (keyword[KW_AZIMUTH].hit) {
        OnScan.Azimuth = keyword[KW_AZIMUTH ].val.d*PI/180.0;
    } else
        OnScan.Azimuth = 0.0;
    /* if (OnScan.Azimuth < 0.0) OnScan.Azimuth += 2.0*PI; */
    if (keyword[KW_ELEVATIO].hit) {
        OnScan.Elevation = keyword[KW_ELEVATIO].val.d*PI/180.0;
    } else
        OnScan.Elevation = 0.0;
    if (keyword[KW_AZPOINT].hit) {
        OnScan.AzPointg = keyword[KW_AZPOINT].val.d*PI/180.0;
    } else
        OnScan.AzPointg = 0.0;
    if (keyword[KW_ELPOINT].hit) {
        OnScan.ElPointg = keyword[KW_ELPOINT].val.d*PI/180.0;
    } else
        OnScan.ElPointg = 0.0;
    if (keyword[KW_AZOFF].hit) {
        OnScan.AzMapOff = keyword[KW_AZOFF].val.d*PI/180.0;
    } else
        OnScan.AzMapOff = 0.0;
    if (keyword[KW_ELOFF].hit) {
        OnScan.ElMapOff = keyword[KW_ELOFF].val.d*PI/180.0;
    } else
        OnScan.ElMapOff = 0.0;
    if (keyword[KW_AZCORR].hit) {
        OnScan.AzOffset = keyword[KW_AZCORR].val.d*PI/180.0;
    } else
        OnScan.AzOffset = 0.0;
    if (keyword[KW_COLLIMAT].hit) {
        OnScan.AzOffset += keyword[KW_COLLIMAT].val.d*PI/180.0
                          /cos(OnScan.Elevation);
    }
    if (keyword[KW_ELCORR].hit) {
        OnScan.ElOffset = keyword[KW_ELCORR].val.d*PI/180.0;
    } else
        OnScan.ElOffset = 0.0;

    OnScan.AzErrAve = 0.0;
    OnScan.ElErrAve = 0.0;
    OnScan.AzErrRms = 0.0;
    OnScan.ElErrRms = 0.0;
    OnScan.GalLong = 0.0;
    OnScan.GalLat = 0.0;
    OnScan.VHel = 0.0;
    OnScan.VLsr = 0.0;
/*
    OnScan.Axial = I.subfocus;
    OnScan.Shift = I.subshiftv;
    OnScan.VTilt = I.subtiltv;
    OnScan.HTilt = I.subtilth;
*/
    OnScan.RefCorr = 1.0;
    if (keyword[KW_BEAMEFF].hit) {
        OnScan.RefCorr = keyword[KW_BEAMEFF].val.d;
    }
    
    if (keyword[KW_POLARIZA].hit) {
        if (strncmp(keyword[KW_POLARIZA].val.str, "LCP+RCP", 6)==0) {
	    polType = POL_BOTH;
        } else if (strncmp(keyword[KW_POLARIZA].val.str, "LCP", 3)==0) {
	    polType = POL_LCP;
        } else if (strncmp(keyword[KW_POLARIZA].val.str, "RCP", 3)==0) {
	    polType = POL_RCP;
	}
/*	printf("Found pol. %s\n", keyword[KW_POLARIZA].val.str); */
    }
    OnScan.flag[0] = polType;
    
    if (keyword[KW_BMAJ].hit) {
        OnScan.StepX = 3600.0*keyword[KW_BMAJ].val.d;
        if (keyword[KW_BMIN].hit) {
            OnScan.StepY = 3600.0*keyword[KW_BMIN].val.d;
        } else {
            OnScan.StepY = OnScan.StepX;
        }
        if (keyword[KW_BPA].hit) {
            OnScan.ParAngle = keyword[KW_BPA].val.d;
        } else {
            OnScan.ParAngle = 0.0;
        }
    } else {
        OnScan.StepX = 0.0;
        OnScan.StepY = 0.0;
        OnScan.ParAngle = 0.0;
    }

    if (keyword[KW_TSYS   ].hit) OnScan.Tsys = keyword[KW_TSYS].val.d;
    else                         OnScan.Tsys = 0.0;
    if (keyword[KW_TREC   ].hit) OnScan.Trec = keyword[KW_TREC].val.d;
    else                         OnScan.Trec = 0.0;
    if (keyword[KW_TCAL   ].hit) OnScan.Tcal = keyword[KW_TCAL].val.d;
    else                         OnScan.Tcal = 0.0;
    if (keyword[KW_TAU    ].hit) OnScan.Tau  = keyword[KW_TAU].val.d;
    else                         OnScan.Tau  = 0.0;
    if (keyword[KW_DBLOAD ].hit) OnScan.dBl  = keyword[KW_DBLOAD].val.d;
    else                         OnScan.Tau  = 0.0;
    if (keyword[KW_OBSTIME].hit) OnScan.IntTime = keyword[KW_OBSTIME].val.d;
    else                         OnScan.IntTime = 0.0;
    
    if (!velaxis) {
	if (keyword[KW_VLSR].hit) {
	    OnScan.VSource = keyword[KW_VLSR].val.d/1.0e3;
	    OnScan.Ctrl |= VLSR;
	} else if (keyword[KW_VELO_LSR].hit) {
	    OnScan.VSource = keyword[KW_VELO_LSR].val.d/1.0e3;
	    OnScan.Ctrl |= VLSR;
	} else if (keyword[KW_VHEL].hit) {
	    OnScan.VSource = keyword[KW_VHEL].val.d/1.0e3;
	    OnScan.Ctrl |= VHEL;
	} else if (keyword[KW_VELO_HEL].hit) {
	    OnScan.VSource = keyword[KW_VELO_HEL].val.d/1.0e3;
	    OnScan.Ctrl |= VHEL;
	} else if (keyword[KW_VELO_GEO].hit) {
	    OnScan.VSource = keyword[KW_VELO_GEO].val.d/1.0e3;
	    OnScan.Ctrl |= VGEO;
	} else
	    OnScan.VSource = 0.0;
	if (keyword[KW_DELTAV].hit) {
	    OnScan.VelRes  = keyword[KW_DELTAV].val.d/1.0e3;
	    /* kludge for taking care of inconsistencies in *.fits */
	    /* we want df*dv < 0 !!! */
	    if (OnScan.VelRes*OnScan.FreqRes > 0.0) 
        	OnScan.FreqRes = -OnScan.FreqRes;
	} else
	    OnScan.VelRes  = -C*OnScan.FreqRes/OnScan.RestFreq/1.0e3;

	OnScan.VSource -= OnScan.VelRes * cc;
    }
    if (cType == COORD_TYPE_SCAN) {
	OnScan.VelRes = keyword[KW_CDELT2].val.d*3600.0;
	OnScan.VSource = (XScan.NChannel/2 - keyword[KW_CRPIX2].val.d)*
	                 OnScan.VelRes;
    }
    
    bzero = 0.0;
    if (keyword[KW_BZERO].hit) bzero  = keyword[KW_BZERO ].val.d;
    bscale = 1.0;
    if (keyword[KW_BZERO].hit) bscale = keyword[KW_BSCALE].val.d;
#ifdef DEBUG
    printf("bzero = %e   bscale = %e\n", bzero, bscale);
#endif

    if (!fd) { /* We only need the header info ! */
        CloseFITSfile();
        ClearFITSwords();
        return 0;
    }

#ifdef DEBUG
    sprintf(buf, "Reading FITS scan from file '%s'.", scanname);
    send_line(buf);
#endif
    
    if (AllocAndReadFITS(bitpix, (int)XScan.NChannel, 1)) return -1;
    
    for (i=0; i<XScan.NChannel; i++)
        OnScan.c[i] = (float)(bzero + bscale*GetFITSValue(bitpix, i));
    
    FreeFITSData();
    ClearFITSwords();
    
    DRP2FD(&OnScan, &XScan, fd);
    
    if (fd->coordType == COORD_TYPE_GAL){
        sprintf(buf, "FITS (%s: %s %s %5.1f,%5.1f) scan %d read.", fd->sname,
            GetLongStr(fd->x0), GetLatStr(fd->y0), fd->xoff, fd->yoff, fd->sno);
    } else {
        sprintf(buf, "FITS (%s: %s %s %5.1f,%5.1f) scan %d read.", fd->sname,
            GetRAStr(fd->x0), GetDECStr(fd->y0), fd->xoff, fd->yoff, fd->sno);
    }
    send_line(buf);
    
    return 0;
}

char *getScanStr(int n)
{
    static string str;
    
    double rta(float);
    
    if (n < 0) {
        if (OnScan.CSystem == COORD_TYPE_GAL) {
            sprintf(str,
      "%5d %-10s %9s %9s %9.1f %7.1f %-12s %-8s %1d %6d %5d (%5.0f,%5.0f)",
            OnScan.ScanNo, OnScan.Name, GetLongStr(OnScan.Longitude),
            GetLatStr(OnScan.Latitude), OnScan.RestFreq,
            OnScan.VSource, OnScan.Molecule, OnScan.Program, OnScan.flag[0],
            (int)XScan.NChannel, (int)(OnScan.IntTime + 0.5),
            rta(OnScan.LMapOff), rta(OnScan.BMapOff));
        } else {
            sprintf(str,
      "%5d %-10s %12s %12s %9.1f %7.1f %-12s %-8s %1d %6d %5d (%5.0f,%5.0f)",
            OnScan.ScanNo, OnScan.Name, GetRAStr(OnScan.Longitude),
            GetDECStr(OnScan.Latitude), OnScan.RestFreq,
            OnScan.VSource, OnScan.Molecule, OnScan.Program, OnScan.flag[0],
            (int)XScan.NChannel, (int)(OnScan.IntTime + 0.5),
            rta(OnScan.LMapOff), rta(OnScan.BMapOff));
        }
    } else {
        if (OnScan.CSystem == COORD_TYPE_GAL) {
            sprintf(str,
    "%4d %5d %-10s %s %s %9.1f %7.1f %-12s %-8s %1d, %6d %5d (%5.0f,%5.0f)",
            n+1,
            OnScan.ScanNo, OnScan.Name, GetLongStr(OnScan.Longitude),
            GetLatStr(OnScan.Latitude), OnScan.RestFreq,
            OnScan.VSource, OnScan.Molecule, OnScan.Program, OnScan.flag[0],
            (int)XScan.NChannel, (int)(OnScan.IntTime + 0.5),
            rta(OnScan.LMapOff), rta(OnScan.BMapOff));
        } else {
            sprintf(str,
    "%4d %5d %-10s %s %s %9.1f %7.1f %-12s %-8s %1d %6d %5d (%5.0f,%5.0f)",
            n+1,
            OnScan.ScanNo, OnScan.Name, GetRAStr(OnScan.Longitude),
            GetDECStr(OnScan.Latitude), OnScan.RestFreq,
            OnScan.VSource, OnScan.Molecule, OnScan.Program, OnScan.flag[0],
            (int)XScan.NChannel, (int)(OnScan.IntTime + 0.5),
            rta(OnScan.LMapOff), rta(OnScan.BMapOff));
        }
    }
    
    return str;
}

static void FreeXSTR(XSTR *x)
{
    int n;
    
    if (!x) return;
    
    n = x->size;
    
    while (n > 0) {
        n--;
        if (x->files) XmStringFree(x->files[n]);
        if (x->descs) XmStringFree(x->descs[n]);
    }
    
    if (x->files) XtFree((char *)x->files);
    if (x->descs) XtFree((char *)x->descs);
    if (x->order) XtFree((char *)x->order);
    
    x->files = NULL;
    x->descs = NULL;
    x->order = NULL;
    
    x->size     = 0;
    x->busy     = 0;
    x->interupt = 0;
}

/* Function argument should be get_fits() or get_drp() */
static XSTR *GetFileList(Widget w, int (*f)(), fsel_struct *fs, int use_old)
{
    int n, nfiles, nfound=0;
    char *txt;
    Widget FS, msg=NULL, scale=NULL;
    XmString *files;
    int (*func)();

    int filter_active(), allow_scan();
    int InitSort(int, XSTR *);
    void AddSort(int, XSTR *, SCAN *);
    void EndSort(XSTR *);
    
    if (use_old) {
        FS = fs->x->fs;
        func = fs->x->f;
    } else {
        FS = w;
        func = f;
    }
    
    if (!FS || !func) return NULL;

    XtVaGetValues(FS, XmNfileListItemCount, &nfiles,
                  XmNfileListItems, &files, NULL);
                  
    if (nfiles < 1) {
        PostWarningDialog(FS, "No files in this directory.");
        return NULL;
    }
    

    if (nfiles > WAITFILES) {    
        msg = PostWaitingDialog(FS,
                                (filter_active()) ?
                                "Scanning entire directory using filter..." :
                                "Scanning entire directory...",
                                &scale, nfiles);
    }

    if (use_old) {
        FreeXSTR(fs->x);
    } else {
        fs->x = (XSTR *) XtMalloc(sizeof(XSTR));
        if (!fs->x) {
            if (msg) XtDestroyWidget(msg);
            PostErrorDialog(FS, "Out of memory when allocating the file list.");
            return NULL;
        }
    }
    
    fs->x->f = func;
    fs->x->fs = FS;
      
    fs->x->files = (XmString *) XtMalloc(nfiles * sizeof(XmString));
    fs->x->descs = (XmString *) XtMalloc(nfiles * sizeof(XmString));
    fs->x->order = (int *) XtMalloc(nfiles * sizeof(int));
    fs->x->sort1_type = sort1_type;
    fs->x->sort2_type = sort2_type;
    fs->x->sort_order = sort_order;
    
    if (!fs->x->files || !fs->x->descs || !fs->x->order) {
        if (msg) XtDestroyWidget(msg);
        PostErrorDialog(FS, "Out of memory when allocating the file list.");
        if (fs->x->files) XtFree((char *)fs->x->files);
        if (fs->x->descs) XtFree((char *)fs->x->descs);
        if (fs->x->order) XtFree((char *)fs->x->order);
        fs->x->files = NULL;
        fs->x->descs = NULL;
        fs->x->order = NULL;
        fs->x->size = 0;
        return fs->x;
    }
    
    fs->x->busy = 1;
    fs->x->interupt = 0;
    
    InitSort(nfiles, fs->x);

    for (n=0; n<nfiles; n++) {
        if (msg) SetWaitingScale(scale, n+1);
        if (fs->x->interupt) {
            PostWarningDialog(FS, "File listing interupted.");
            fs->x->interupt = 0;
            break;
        }
        if (XmStringGetLtoR(files[n], XmSTRING_DEFAULT_CHARSET, &txt)) {
            if (func(txt, NULL) != 0) { /* Failure if != 0 */
                XtFree(txt);
                continue;
            }
            if (filter_active() && !allow_scan(&OnScan)) continue;
            fs->x->files[nfound] = MKSTRING(txt);
            fs->x->descs[nfound] = MKSTRING(getScanStr(-1));
            AddSort(nfound, fs->x, &OnScan);
            XtFree(txt);
            nfound++;
        }
    }
    fs->x->size = nfound;
    fs->x->busy = 0;
    EndSort(fs->x);
    
    if (msg) XtDestroyWidget(msg);
    
    if (fs->x->size < 1) {
        if (!use_old)
            PostWarningDialog(FS, "No files in this directory.\nCheck filter.");
        XtFree((char *)fs->x->files);
        XtFree((char *)fs->x->descs);
        XtFree((char *)fs->x->order);
        fs->x->files = NULL;
        fs->x->descs = NULL;
        fs->x->order = NULL;
        fs->x->size  = 0;
    }
    
    return fs->x;
}

static void destroy_list_dialog(Widget w, fsel_struct *f, XmAnyCallbackStruct *cb)
{
    if (f && f->x) {
        FreeXSTR(f->x);
        XtFree((char *)(f->x));
        f->x = NULL;
    }
}

static void cancel_list_dialog(Widget w, fsel_struct *f, XmAnyCallbackStruct *cb)
{
    if (!f || !f->x) return;
    
    if (f->x->busy) {
        f->x->interupt = 1;
        return;
    }
    
    if (f->x->form)
        XtDestroyWidget(f->x->form);
}

static void reset_list_dialog(Widget w, fsel_struct *f, XmAnyCallbackStruct *cb)
{    
    if (!f || !f->x) return;
    if (f->x->busy) return;
    
    if (f->x->dialog) XmListDeselectAllItems(f->x->dialog);
}

static void all_list_dialog(Widget w, fsel_struct *f, XmAnyCallbackStruct *cb)
{
    int n = 0;
    
    if (!f || !f->x) return;
    if (f->x->busy) return;
    
    reset_list_dialog(w, f, cb);
    
    XtVaSetValues(f->x->dialog, XmNselectionPolicy, XmMULTIPLE_SELECT, NULL);
    
    for (n=1; n<=f->x->size; n++)
        XmListSelectPos(f->x->dialog, n, (n == f->x->size) ? True : False);
    
    XtVaSetValues(f->x->dialog, XmNselectionPolicy, XmEXTENDED_SELECT, NULL);
}

#ifdef RESCAN
static void rescan_list_dialog(Widget w, fsel_struct *f, XmAnyCallbackStruct *cb)
{
    PostMessageDialog(w, "Not yet available.");
}
#endif

#ifdef SORT
static void set_sort1_type(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int i = atoi(str);

    if (i != sort1_type) {
        sort1_type = i;
    }
}
static void set_sort2_type(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int i = atoi(str);

    if (i != sort2_type) {
        sort2_type = i;
    }
}
static void set_sort_order(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int i = atoi(str);

    if (i != sort_order) {
        sort_order = i;
    }
}
#endif

static void invoke_filter(Widget w, fsel_struct *f, XmAnyCallbackStruct *cb)
{
    void PostFilterDialog();
    
    PostFilterDialog(w, "FromList", NULL);
}

static void filter_list(Widget w, fsel_struct *f, XmAnyCallbackStruct *cb)
{
    void wprintf();
    
    if (!f || !f->x) return;
    if (f->x->busy) return;
    
    reset_list_dialog(w, f, cb);
    
    if (!GetFileList(NULL, NULL, f, 1)) {
        cancel_list_dialog(w, f, cb);
        return;
    }
    
    XmListDeleteAllItems(f->x->dialog);
    if (f->x->size)
        XmListAddItems(f->x->dialog, f->x->descs, f->x->size, 0);
        
    wprintf(f->x->label, f->x->fmt, f->x->size);
    
    XtVaSetValues(f->x->dialog,
                  XmNvisibleItemCount,
                  (f->x->size < 30 && f->x->size > 0) ? f->x->size : 30,
                  NULL);
}

static Widget CreateFileListDialog(Widget w, char *title, fsel_struct *f)
{
    int n, vis;
    Arg wargs[10];
    Widget form, label, sep, cancel, all, reset, invoke, filter;
#ifdef RESCAN
    Widget rescan;
#endif
#ifdef SORT
    Widget sort1, sort2, order;
#endif
    Widget dialog;
    
    if (!f || !f->x) return NULL;
    
    n = 0;
    XtSetArg(wargs[n], XmNautoUnmanage, False); n++;
    if (title) {
        XtSetArg(wargs[n], XmNtitle, title); n++;
    }
    form = XmCreateFormDialog(w, "form", wargs, n);
    
    vis = (f->x->size < 30 && f->x->size > 0) ? f->x->size : 30;
    n = 0;
    XtSetArg(wargs[n], XmNitemCount, f->x->size); n++;
    XtSetArg(wargs[n], XmNitems, f->x->descs); n++;
    XtSetArg(wargs[n], XmNvisibleItemCount, vis); n++;
    XtSetArg(wargs[n], XmNfontList, gp->flist10); n++;
    XtSetArg(wargs[n], XmNselectionPolicy, XmEXTENDED_SELECT); n++;
    
    if (f->x->fmt) {
        sprintf(buf, f->x->fmt, f->x->size);
    } else {
        sprintf(buf, "No of files: %d", f->x->size);
    }
    
    dialog = XmCreateScrolledList(form, "list", wargs, n);
    
    label =  XtVaCreateManagedWidget(buf, xmLabelWidgetClass, form, NULL);

#ifdef SORT
    sprintf(buf, "%2d", sort1_type);
    sort1 = CreateOptionMenu(form, &Sort1Menu);
    SetDefaultOptionMenuItemString(sort1, Sort1Data, buf);
    
    sprintf(buf, "%2d", sort2_type);
    sort2 = CreateOptionMenu(form, &Sort2Menu);
    SetDefaultOptionMenuItemString(sort2, Sort2Data, buf);
    
    order = CreateOptionMenu(form, &SortOrderMenu);
    SetDefaultOptionMenuItem(order, sort_order);
#endif

    sep =    XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
				                     form, XmNseparatorType,
				                     XmSHADOW_ETCHED_IN, NULL);
    cancel = XtVaCreateManagedWidget(BUTT_CANCEL, xmPushButtonWidgetClass,
                                     form, NULL);
    all =    XtVaCreateManagedWidget("Select all", xmPushButtonWidgetClass,
                                     form, NULL);
    reset =  XtVaCreateManagedWidget("Unselect all", xmPushButtonWidgetClass,
                                     form, NULL);
#ifdef RESCAN
    rescan = XtVaCreateManagedWidget("Rescan", xmPushButtonWidgetClass,
                                     form, NULL);
#endif
    invoke = XtVaCreateManagedWidget("Set filter...", xmPushButtonWidgetClass,
                                     form, NULL);
    filter = XtVaCreateManagedWidget("Filter/Sort", xmPushButtonWidgetClass,
                                     form, NULL);
    
    f->x->form = form;
    f->x->label = label;
    f->x->dialog = dialog;
    XtAddCallback(cancel, XmNactivateCallback,
                  (XtCallbackProc)cancel_list_dialog, f);
    XtAddCallback(reset, XmNactivateCallback,
                  (XtCallbackProc)reset_list_dialog, f);
    XtAddCallback(all, XmNactivateCallback,
                  (XtCallbackProc)all_list_dialog, f);
#ifdef RESCAN
    XtAddCallback(rescan, XmNactivateCallback,
                  (XtCallbackProc)rescan_list_dialog, f);
#endif
    XtAddCallback(form, XmNdestroyCallback,
                  (XtCallbackProc)destroy_list_dialog, f);
    XtAddCallback(invoke, XmNactivateCallback,
                  (XtCallbackProc)invoke_filter, f);
    XtAddCallback(filter, XmNactivateCallback,
                  (XtCallbackProc)filter_list, f);

    XtVaSetValues(form, XmNdefaultButton, cancel, NULL);
    
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetArg(wargs[n], XmNrightAttachment,   XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,       10); n++;
    XtSetValues(XtParent(dialog), wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         XtParent(dialog)); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetArg(wargs[n], XmNrightAttachment,   XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,       10); n++;
    XtSetValues(label, wargs, n);
#ifdef SORT
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         label); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetValues(sort1, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         label); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,        sort1); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetValues(sort2, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         label); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,        sort2); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetValues(order, wargs, n);
    /* n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         label); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,        order); n++;
    XtSetArg(wargs[n], XmNleftOffset,        20); n++;
    XtSetValues(invoke, wargs, n); */
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         sort1); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,        1); n++;
    XtSetArg(wargs[n], XmNrightAttachment,   XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,       1); n++;
    XtSetValues(sep, wargs, n);
#else
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         label); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,        1); n++;
    XtSetArg(wargs[n], XmNrightAttachment,   XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,       1); n++;
    XtSetValues(sep, wargs, n);
#endif
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,         20); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,        30); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,  XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,      10); n++;
    XtSetValues(cancel, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,         20); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,        cancel); n++;
    XtSetArg(wargs[n], XmNleftOffset,        20); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,  XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,      10); n++;
    XtSetValues(reset, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,         20); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,        reset); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,  XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,      10); n++;
    XtSetValues(all, wargs, n);
#ifdef RESCAN
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,         20); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,        all); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,  XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,      10); n++;
    XtSetValues(rescan, wargs, n);
#endif
/* #ifndef SORT */
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,         20); n++;
    XtSetArg(wargs[n], XmNrightAttachment,   XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNrightWidget,       filter); n++;
    XtSetArg(wargs[n], XmNrightOffset,       20); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,  XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,      10); n++;
    XtSetValues(invoke, wargs, n);
/* #endif */
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         sep); n++;
    XtSetArg(wargs[n], XmNtopOffset,         20); n++;
    XtSetArg(wargs[n], XmNrightAttachment,   XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,       30); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,  XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,      10); n++;
    XtSetValues(filter, wargs, n);
    
    XtManageChild(dialog);
#ifdef SORT
    XtManageChild(sort1);
    XtManageChild(sort2);
    XtManageChild(order);
#endif
    
    ManageDialogCenteredOnPointer(form);
    
    return dialog;
}

static void GetFileCallback(Widget w, fsel_struct *f, XmListCallbackStruct *cb)
{
    char *txt;
    string foo;
    int n, first = 1, i, err;
    DataSetPtr d = NULL;
    
    void UpdateData(), obtain_map_info();
    int read_file(char *, char *, DataSetPtr);
        
    if (!f || !f->x) return;
    
    if (!vP->to || f->x->app_mode == 0) {
        d = new_dataset(get_listlist(), "Read", NULL);
        if (!d) return;
    } else {
        d = NULL;
    }
        
    sprintf(d ? d->name : vP->to->name,
            "%s %d scans",
            f->x->seq_mode ? "Sequence of" : "Read",
            cb->selected_item_count + count_scans(d ? d : vP->to));
    
    for (n=0; n<cb->selected_item_count; n++) {
        if (XmStringGetLtoR(cb->selected_items[n],
                            XmSTRING_DEFAULT_CHARSET, &txt)) {
            sscanf(txt, "%4d %s", &i, foo);
            XtFree(txt);
            if (XmStringGetLtoR(f->x->files[i-1],
                                XmSTRING_DEFAULT_CHARSET, &txt)) {
                err = read_file(f->x->file_type, txt, first ? d : NULL);
                XtFree(txt);
                if (!err && first) {
                    first = 0;
                } else if (err == 1) { /* Out of memory */
                    break;
                }
            }
        }
    }
    
    if (d) vP->to = d;
    vP->from = vP->to;
    
    if (count_scans(vP->from) > 1) {
        obtain_map_info(NULL, "map", NULL);
    }
    
    if (count_scans(vP->from)) {
        UpdateData(SCALE_BOTH, REDRAW);
    }
}

int PostFileListDialog(Widget w, fsel_struct *fs)
{
    string title, fmt;
    char ftype[30];
    Widget dialog;
    int (*f)(char *, FDATA *);
    
    int get_fits(char *, FDATA *), get_drp(char *, FDATA *);
    
    if (strcmp(fs->s, "mapfits")==0) {
        strcpy(fmt, "Read multiple FITS files (No of files: %d).");
        strcpy(title, "Read multiple FITS files");
        strcpy(ftype, "fits");
        f = get_fits;
    } else if (strcmp(fs->s, "seqfits")==0) {
        strcpy(fmt, "Read a sequence of FITS files (No of files: %d).");
        strcpy(title, "Read a sequence of FITS files");
        strcpy(ftype, "seqfits");
        f = get_fits;
    } else if (strcmp(fs->s, "appmapfits")==0) {
        strcpy(fmt, "Append multiple FITS files (No of files: %d).");
        strcpy(title, "Append multiple FITS files");
        strcpy(ftype, "fits");
        f = get_fits;
    } else if (strcmp(fs->s, "appseqfits")==0) {
        strcpy(fmt, "Append a sequence FITS files (No of files: %d).");
        strcpy(title, "Append a sequence FITS files");
        strcpy(ftype, "seqfits");
        f = get_fits;
    } else if (strcmp(fs->s, "mapdrp")==0) {
        strcpy(fmt, "Read multiple DRP files (No of files: %d).");
        strcpy(title, "Read multiple DRP filesS");
        strcpy(ftype, "drp");
        f = get_drp;
    } else if (strcmp(fs->s, "seqdrp")==0) {
        strcpy(fmt, "Read a sequence of DRP files (No of files: %d).");
        strcpy(title, "Read a sequence of DRP files");
        strcpy(ftype, "seqdrp");
        f = get_drp;
    } else if (strcmp(fs->s, "appmapdrp")==0) {
        strcpy(fmt, "Append multiple DRP files (No of files: %d).");
        strcpy(title, "Append multiple DRP files");
        strcpy(ftype, "drp");
        f = get_drp;
    } else if (strcmp(fs->s, "appseqdrp")==0) {
        strcpy(fmt, "Append a sequence DRP files (No of files: %d).");
        strcpy(title, "Append a sequence DRP files");
        strcpy(ftype, "seqdrp");
        f = get_drp;
    } else {
        strcpy(fmt, "No of files: %d");
        strcpy(title, "Read files");
        strcpy(ftype, "fits");
        f = get_fits;
    }
    
    if (!(GetFileList(w, f, fs, 0))) return 1;
    
    if (strstr(fs->s, "app"))
        fs->x->app_mode = 1;
    else
        fs->x->app_mode = 0;
    
    if (strstr(fs->s, "seq"))
        fs->x->seq_mode = 1;
    else
        fs->x->seq_mode = 0;
        
    strcpy(fs->x->file_type, ftype);
    strcpy(fs->x->fmt, fmt);
    
    fs->x->busy = fs->x->interupt = 0;
    
    dialog = CreateFileListDialog(w, title, fs);
    
    if (dialog) {
        XtAddCallback(dialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)GetFileCallback, fs);
        XtAddCallback(dialog, XmNmultipleSelectionCallback,
                      (XtCallbackProc)GetFileCallback, fs);
    }
    
    return 0;
}

int get_bintab(FDATA *fd)
{
    void DRP2FD();
    
    DRP2FD(&OnScan, &XScan, fd);
    
    return 0;
}

int open_binary_table(Widget w, char *fname, char *cmd)
{
    int n, n1, n2, nsca, bitpix=0, err=0, BT_type=0;
    struct fitskey *keyword;
    void *data;
    DataSetPtr d;
    
    long int FieldByteSize();
    int read_file(char *, char *, DataSetPtr);
    void UpdateData();
    void obtain_map_info();
    void SetCSystemFromDRP();
    void DeleteLastDataSet();
           
    ClearFITSwords();
    
    if ((keyword = readFITSheader(fname, 0)) == NULL) {
        PostErrorDialog(NULL, "Error while reading binary table FITS header.");
        ClearFITSwords();
        return -1;
    }
    
    /* If no KW_XTENSION was find in the first record, but only a KW_EXTEND
       we must start to read at the second record, i.e. skip first KW_END by a
       special call to readFITSheader.
     */
    if (!keyword[KW_XTENSION].hit && keyword[KW_EXTEND].hit) {
        if ((keyword = readFITSheader(fname, 1)) == NULL) {
            PostErrorDialog(NULL,
                            "Error while reading binary table FITS header.");
            ClearFITSwords();
            return -1;
        }
    }
    
    if (!keyword[KW_XTENSION].hit) {
        sprintf(buf, "File '%s' has no specified extension.", fname);
        PostErrorDialog(NULL, buf);
        CloseFITSfile();
        ClearFITSwords();
        return -1;
    }
    if (strcmp(keyword[KW_XTENSION].val.str, "BINTABLE")) {
        sprintf(buf, "File '%s' is not a binary table.", fname);
        PostErrorDialog(NULL, buf);
        CloseFITSfile();
        ClearFITSwords();
        return -1;
    }
    if (!keyword[KW_BITPIX].hit) {
        sprintf(buf, "File '%s' has no specified BITPIX.", fname);
        PostErrorDialog(NULL, buf);
        CloseFITSfile();
        ClearFITSwords();
        return -1;
    }
    bitpix = (int)keyword[KW_BITPIX].val.l;
    if (bitpix != FITS_TYPE_8BIT) {
        sprintf(buf, "Only accepting BITPIX=8 (not %d) for binary tables.",
                bitpix);
        PostErrorDialog(NULL, buf);
        CloseFITSfile();
        ClearFITSwords();
        return -1;
    }
    
    n = (int)keyword[KW_NAXIS].val.l;
    if (n != 2) {
        sprintf(buf, "File '%s' has not dimension 2 (%d).", fname, n);
        PostErrorDialog(NULL, buf);
        CloseFITSfile();
        ClearFITSwords();
        return -1;
    }
    n1 = (int)keyword[KW_NAXIS1].val.l;
    if (keyword[KW_NAXIS1].val.l != FieldByteSize()) {
        sprintf(buf, "File '%s' has inconsistent size: %ld vs %ld.",
                fname, keyword[KW_NAXIS1].val.l, FieldByteSize());
        PostErrorDialog(NULL, buf);
        CloseFITSfile();
        ClearFITSwords();
        return -1;
    }
    
    if (keyword[KW_EXTNAME].hit) {
        sprintf(buf, "Found proper binary table with extension name '%s'.",
                keyword[KW_EXTNAME].val.str);
        send_line(buf);
        if (strcmp(keyword[KW_EXTNAME].val.str, "ODIN_Binary") == 0) {
            BT_type = 1;
        } else if (strcmp(keyword[KW_EXTNAME].val.str, "Binary Table") == 0) {
            BT_type = 0;
        } else {
            sprintf(buf, "Extension name %s is not supported.\n\
The supported FITS binary table formats are:\n\
Odin binary tables (ext. name 'ODIN_Binary')\n\
SEST on-the-fly binary tables (ext. name 'Binary Table')",
                    keyword[KW_EXTNAME].val.str);
            PostErrorDialog(NULL, buf);
            CloseFITSfile();
            ClearFITSwords();
            return -1;
        }
    } else {
        PostErrorDialog(NULL, "No extension name found in file.");
        CloseFITSfile();
        ClearFITSwords();
        return -1;
    }
    
    n2 = (int)keyword[KW_NAXIS2].val.l;
    
    d = new_dataset(get_listlist(), "Binary table", NULL);
    if (!d) {
        PostErrorDialog(NULL, "Out of memory when allocating new data set.");
        CloseFITSfile();
        ClearFITSwords();
        return -1;
    }
    
    if (AllocFITSData(bitpix, n1, NULL, &data)) {
        sprintf(buf, "\
Out of memory when reading binary table '%s'.\n\
Number of spectra: %d, Row size=%d, Bits/pixel=%d", fname, n2, n1, bitpix);
        PostErrorDialog(NULL, buf);
        CloseFITSfile();
        ClearFITSwords();
        return -1;
    }
    
    nsca = 0;
    for (n=0; n<n2; n++) {
        err = readFITSdata(n1, (int)sizeof(char), data);
        if (!err) {
            err = 1;
            sprintf(buf, "Error when reading row %d (n=%d) from '%s'.",
                    n+1, n1, fname);
            send_line(buf);
            PostErrorDialog(NULL, buf);
            break;
        }
        err = LoadBinaryTable(n, data, BT_type);
        if (err) {
	    err = 0;
	    continue;
	}
        
        SetCSystemFromDRP(&OnScan);
        
        err = read_file(cmd, fname, (nsca == 0) ? d : NULL);
        if (!err) {
            nsca++;
        } else if (err == 1) { /* Out of memory */
            break;
        }
    }
    
    if (nsca != n2)
        sprintf(buf, "Read only %d scans out of %d.", nsca, n2);
    else
        sprintf(buf, "Read %d scans.", nsca);
    send_line(buf);
    
    FreeFITSData();
    CloseFITSfile();
    ClearFITSwords();
     
    if (nsca == 0) {
        DeleteLastDataSet();
        PostErrorDialog(NULL, "No scans found. Data set not created.");
        return -1;
    }
    
    vP->from = vP->to = d;
    vP->s = (scanPtr)DATA(d->scanlist);
    
    if (keyword[KW_OBJECT].hit)
        sprintf(d->name, "Bin. table %s (%d scans)",
                keyword[KW_OBJECT].val.str, nsca);
    else if (BT_type == 1)
        sprintf(d->name, "%s %d (%d scans)", vP->s->name, vP->s->scan_no, nsca);
    else
        sprintf(d->name, "%s (%d scans)", vP->s->name, nsca);
   
    if (count_scans(vP->from) > 1) {
        obtain_map_info(NULL, "map", NULL);
    }
    UpdateData(SCALE_BOTH, REDRAW);
    
    return 0;
}

int save_binary_table(char *tname, DataSetPtr d, int type)
{
    int nscans, rowsize, nchan=-1, nfields, bytes=0, err=0, n=0;
    list curr = NULL;
    scanPtr s;
    void *data, *dptr=NULL;
    fkey *t;
    
    int  GetTypeOfFITS();
    int  GetBTrowsize(int, int, int *);
    void FillInTCards(int, int, fkey *);
    int FillBinaryTable(scanPtr, char *, int, int);
    
    if (!d) d = vP->from;
    
    if (!d) {
        sprintf(buf, "No dataset?");
        send_line(buf);
        return 1;
    }
    
    if (!tname) {
        sprintf(buf, "No filename!");
        send_line(buf);
        return 1;
    }
    
    /* Check scans to see whether they can be put into a BINTABLE */
    nscans = count_scans(d);
    if (!nscans) {
        sprintf(buf, "No scans found in %s.", d->name);
        send_line(buf);
        return 1;
    }
    
    while ( (curr = scan_iterator(curr, d)) != NULL) {
        s = (scanPtr)DATA(curr);
        if (nchan == -1) {
            nchan = s->nChan;
        } else {
          if (s->nChan != nchan) {
             sprintf(buf, "Scans of different lengths found in %s.", d->name);
             send_line(buf);
             return 1;
          }
        }
    }

    ClearFITSwords();
    
    BoolCard(KW_SIMPLE, TRUE);
    LongCard(KW_BITPIX, 8);
    LongCard(KW_NAXIS,  0);
    BoolCard(KW_EXTEND, TRUE);
    VoidCard(KW_END);
    
    if (writeFITSheader(tname) != 0) {
        sprintf(buf, "Error in writeFITSheader(%s).", tname);
        send_line(buf);
        return 1;
    }
    
    ClearFITSwords();
    
    nscans = count_scans(d);
    rowsize = GetBTrowsize(nchan, type, &nfields);
    
    /* printf("save_binary_table 5: nscans=%d nfields=%d rowsize=%d\n",
           nscans, nfields, rowsize); */
    
    if (!rowsize) {
        sprintf(buf, "Strange rowsize (%d) in bytes.", rowsize);
        send_line(buf);
        CloseFITSfile();
        return 1;
    }
    
    t = (fkey *)malloc(3 * nfields * sizeof(fkey));
    if (!t) {
        sprintf(buf, "Out of memory.");
        send_line(buf);
        CloseFITSfile();
        return 1;
    }
    
    CharCard(KW_XTENSION, "BINTABLE");
    LongCard(KW_BITPIX,   8);
    LongCard(KW_NAXIS,    2);
    LongCard(KW_NAXIS1,   rowsize);
    LongCard(KW_NAXIS2,   nscans);
    LongCard(KW_PCOUNT,   0);
    LongCard(KW_GCOUNT,   1);
    LongCard(KW_TFIELDS,  nfields);
    
    FillInTCards(type, nchan, t);
    
    VoidCard(KW_END);
    
    if (writeBINTABLEheader(t, nfields) != 0) {
        sprintf(buf, "Error in extended writeBINTABLEheader(%s).", tname);
        send_line(buf);
        if (t) free(t);
        return 1;
    }
    
    if (t) free(t);
    
    data = (void *)malloc(nscans * rowsize * sizeof(char));
    if (!data) {
        sprintf(buf, "Out of memory.");
        send_line(buf);
        CloseFITSfile();
        return 1;
    }
    
    curr = NULL; n=0;
    dptr = data;
    while ( (curr = scan_iterator(curr, d)) != NULL) {
        s = (scanPtr)DATA(curr);
	err = FillBinaryTable(s, (char *)dptr, rowsize, type);
	dptr += rowsize;
	n++;
    }
    
    bytes = writeFITSdata(1, nscans*rowsize, data);
    if (bytes != rowsize*nscans) {
      err += 1;
      sprintf(buf, "Error in writing FITS data. %d != %d*%d",
    	      bytes, nscans, rowsize);
      send_line(buf);
    }
     
    if (data) free(data);
    
    if (err == 0) {
	sprintf(buf, "Wrote %d scans (%.3f Mb) into Odin binary table %s",
   	        nscans, (double)(bytes)/1024.0/1024.0, tname);
	send_line(buf);
    } else {
	sprintf(buf, "Warning! Wrote only %d scans of %d, each of %d bytes, into Odin binary table %s",
   	        n, nscans, rowsize, tname);
	send_line(buf);
    }
    
    CloseFITSfile();
    
    return 0;
}

int MakeFITSCubeOfMaps(const char *cubename, MAP **maps, int nmaps)
{
    int i, j, k, first, n, bitpix_out, size;
    double max=0.0, min=0.0, Y, bs, bz;
    double val;
    void *data;
    struct tm *now;
    time_t clock;
    MAP *m;
    
    int GetTypeOfFITS();
    
    ClearFITSwords();
    
    if (!maps) return 1;
    
    m = maps[0];
    
    if ((bitpix_out = GetTypeOfFITS()) == FITS_TYPE_UNKNOWN) {
        sprintf(buf, "Cannot handle FITS type=%d.", bitpix_out);
        send_line(buf);
        return 1;
    }
    
    BoolCard(KW_SIMPLE, TRUE);
    LongCard(KW_BITPIX, bitpix_out);
    LongCard(KW_NAXIS,  3);
    
    LongCard(KW_NAXIS1, m->i_no);
    LongCard(KW_NAXIS2, m->j_no);
    LongCard(KW_NAXIS3, nmaps);
    
    if (m->coordType == COORD_TYPE_EQU) {
        CharCard(KW_CTYPE1, "RA---GLS");
    } else if (m->coordType == COORD_TYPE_GAL) {
        CharCard(KW_CTYPE1, "GLON-GLS");
    } else if (m->coordType == COORD_TYPE_HOR) {
        CharCard(KW_CTYPE1, "AZ---GLS");
    } else {
        CharCard(KW_CTYPE1, "PIX--GLS");
    }
    
    if (m->xspacing != 0.0)
        RealCard(KW_CRPIX1, 1.0 + (double)NINT(-m->xleft/m->xspacing));
    else
        RealCard(KW_CRPIX1, 1.0);
    RealCard(KW_CRVAL1, m->x0*RADTODEG);
    RealCard(KW_CDELT1, m->xspacing/3600.0);
    RealCard(KW_CROTA1, 0.0*RADTODEG);
    
    if (m->coordType == COORD_TYPE_EQU) {
        CharCard(KW_CTYPE2, "DEC--GLS");
    } else if (m->coordType == COORD_TYPE_GAL) {
        CharCard(KW_CTYPE2, "GLAT-GLS");
    } else if (m->coordType == COORD_TYPE_HOR) {
        CharCard(KW_CTYPE2, "EL---GLS");
    } else {
        CharCard(KW_CTYPE2, "PIX--GLS");
    }
    if (m->yspacing != 0.0)
        RealCard(KW_CRPIX2, 1.0 + (double)NINT(-m->ylower/m->yspacing));
    else
        RealCard(KW_CRPIX2, 1.0);
    RealCard(KW_CRVAL2, m->y0*RADTODEG);
    RealCard(KW_CDELT2, m->yspacing/3600.0);
    RealCard(KW_CROTA2, 0.0*RADTODEG);
    
    CharCard(KW_CTYPE3, "VELO");
    RealCard(KW_CRPIX3, 1.0);
    RealCard(KW_CRVAL3, m->v*1.0e3);
    RealCard(KW_CDELT3, m->dv*1.0e3);
    RealCard(KW_CROTA3, 0.0);
    
    CharCard(KW_BUNIT, "K km/s");

    first = 1;
    for (n=0; n<nmaps; n++) {
        m = maps[n];
        for (i=0; i<m->i_no; i++) {
            for (j=0; j<m->j_no; j++) {
                if (m->f[i][j] >= 0) {
                    Y = m->d[i][j];
                    if (first) {
                        min = max = Y;
                        first = 0;
                    } else {
                        if (Y > max) max = Y;
                        if (Y < min) min = Y;
                    }
                }
            }
        }
    }
    m = maps[0];
    
    GetFITSScaleFactors(bitpix_out, min, max, &bz, &bs);

    RealCard(KW_BSCALE,  bs);
    RealCard(KW_BZERO,   bz);
    RealCard(KW_DATAMAX, max);
    RealCard(KW_DATAMIN, min);
    LongCard(KW_BLANK,   GetBLANK(bitpix_out));

    if (m->date.Year >= 1900 && m->date.Year < 2000)
        sprintf(buf, "%02d/%02d/%02d",
                m->date.Day, m->date.Month, m->date.Year % 100);
    else {
        CharCard(KW_TIMESYS, "UTC");
        sprintf(buf, "%4d-%02d-%02dT%02d:%02d:%02d",
                m->date.Year, m->date.Month, m->date.Day,
                m->date.Hour, m->date.Min, m->date.Sec);
    }
    CharCard(KW_DATE_OBS, buf);
    
    time(&clock);
    now = gmtime(&clock);
    if (now->tm_year >= 0 && now->tm_year < 100) {
        sprintf(buf,"%02d/%02d/%02d",
                now->tm_mday, now->tm_mon+1, now->tm_year);
    } else {
        sprintf(buf, "%4d-%02d-%02dT%02d:%02d:%02d",
                now->tm_year+1900, now->tm_mon+1, now->tm_mday,
                now->tm_hour, now->tm_min, now->tm_sec);
    }
    CharCard(KW_DATE, buf);
    
    sprintf(buf, "%02d:%02d:%02d",
            m->date.Hour, m->date.Min, m->date.Sec);
    CharCard(KW_UTC, buf);
    
    sprintf(buf, "%02d:%02d:%02d",
            OnScan.STHour, OnScan.STMin, OnScan.STSec);
    CharCard(KW_LST, buf);
    
    strncpy(buf, m->name, 12); buf[12] = '\0';
    CharCard(KW_OBJECT, buf);
    sprintf(buf, "%s %.1f.%d", PKGNAME, XS_VERSION, XS_PATCH); buf[12] = '\0';
    CharCard(KW_ORIGIN, buf);
    RealCard(KW_BMAJ, m->b.maj/3600.0);
    RealCard(KW_BMIN, m->b.min/3600.0);
    RealCard(KW_BPA,  m->b.PA);
    RealCard(KW_EQUINOX, (double)OnScan.Equinox);
    RealCard(KW_AZIMUTH, (double)OnScan.Azimuth*RADTODEG);
    RealCard(KW_ELEVATIO, (double)OnScan.Elevation*RADTODEG);
    RealCard(KW_AZOFF, (double)OnScan.AzMapOff*RADTODEG);
    RealCard(KW_ELOFF, (double)OnScan.ElMapOff*RADTODEG);
    RealCard(KW_AZCORR, (double)OnScan.AzOffset*RADTODEG);
    RealCard(KW_ELCORR, (double)OnScan.ElOffset*RADTODEG);
    RealCard(KW_RESTFREQ, OnScan.RestFreq*1.0e6);
    RealCard(KW_IMAGFREQ, (2.0*XScan.LOFreq - OnScan.RestFreq)*1.0e6);
    RealCard(KW_VELO_LSR, (m->v + (double)(nmaps/2)*m->dv)*1.0e3);
    RealCard(KW_DELTAV, m->dv*1.0e3);
    RealCard(KW_TSYS, (double)OnScan.Tsys);
    RealCard(KW_OBSTIME, (double)OnScan.IntTime);
    LongCard(KW_SCAN, (long)OnScan.ScanNo);
    RealCard(KW_TAU, (double)OnScan.Tau);
    RealCard(KW_DBLOAD, (double)OnScan.dBl);
    strncpy(buf, OnScan.Observer, 16); buf[16] = '\0';
    CharCard(KW_OBSERVER, buf);
    strncpy(buf, OnScan.Molecule, 16); buf[16] = '\0';
    CharCard(KW_LINE, buf);
    RealCard(KW_TOUTSIDE, (double)OnScan.AirTemp);
    RealCard(KW_PRESSURE, (double)OnScan.Pressure*100.0);
    RealCard(KW_HUMIDITY, (double)OnScan.Humidity/100.0);
    VoidCard(KW_END);

    if (writeFITSheader(cubename) != 0) {
        sprintf(buf, "Error in writeFITSheader().");
        send_line(buf);
        return 1;
    }
    
    m = maps[0];
    n = m->i_no * m->j_no * nmaps;
    
    if (AllocFITSData(bitpix_out, n, &size, &data)) return 1;
    
    n = 0;
    for (k=0; k<nmaps; k++) {
        m = maps[k];
        for (j=0; j<m->j_no; j++) {
            for (i=0; i<m->i_no; i++) {
                if (m->f[i][j] >= 0)
                    val = (m->d[i][j] - bz)/bs;
                else
                    val = (double)GetBLANK(bitpix_out);
                SetFITSValue(bitpix_out, n, val);
                n++;
            }
        }
    }
    
    i = writeFITSdata(n, size, data);
    
    CloseFITSfile();
    
    FreeFITSData();
    
    if (i != n*size) {
        sprintf(buf, "Cube written to file '%s' but only %d bytes of %d.",
                cubename, i, n*size);
    } else {
        sprintf(buf, "Cube saved as '%s'.", cubename);
    }
    send_line(buf);

    return 0;
}

int put_fits(char *scanname, FDATA *fd, int type)
{
    int i, j, cc, first, n=0, k, bitpix_out, size;
    float max=0.0, min=0.0, Y;
    double bs, bz, cp, sp;
    double val;
    void *data;
    struct tm *now;
    time_t clock;
    scanPtr s;
    
    int GetTypeOfFITS();
    int CheckCRVALType();
    
    if (fd) {    
        strcpy(OnScan.Name, fd->sname);
        strcpy(OnScan.Molecule, fd->molecule);
        OnScan.NChannel = (short)fd->n;
	XScan.NChannel = fd->n;
        OnScan.ScanNo   = (short)fd->sno;
        OnScan.PosAngle = (float)fd->posang;
        cp = cos(fd->posang);
        sp = sin(fd->posang);
        OnScan.Equinox   = fd->equinox;
        OnScan.Longitude = fd->x0;
        OnScan.Latitude  = fd->y0;
        OnScan.CSystem = fd->coordType;
        OnScan.Year  = fd->date.Year;
        OnScan.Month = fd->date.Month;
        OnScan.Day   = fd->date.Day;
        OnScan.UTHour = fd->date.Hour;
        OnScan.UTMin  = fd->date.Min;
        OnScan.UTSec  = fd->date.Sec;
        OnScan.StepX    = fd->b.maj;
        OnScan.StepY    = fd->b.min;
        OnScan.ParAngle = fd->b.PA;
        if (type == FITS_VECTOR) {
            OnScan.LMapOff = (fd->xoff * cp + fd->yoff * sp)/RADTOSEC;
            OnScan.BMapOff = (fd->yoff * cp - fd->xoff * sp)/RADTOSEC;
            if (CheckCRVALType()) {
                OnScan.Longitude += OnScan.LMapOff/cos(fd->y0);
                OnScan.Latitude  += OnScan.BMapOff;
            }
        }
        OnScan.AzMapOff = (fd->aoff * cp + fd->eoff * sp)/RADTOSEC;
        OnScan.ElMapOff = (fd->aoff * cp - fd->eoff * sp)/RADTOSEC;
        OnScan.Tsys = (float)fd->tsys;
        OnScan.Tau = (float)fd->tau;
        OnScan.IntTime = (float)fd->int_time;
        cc = CenterCh(&OnScan, &XScan);
        OnScan.FreqRes  = fd->fres;
        OnScan.VelRes   = fd->vres;
        OnScan.RestFreq = 1000.0*fd->f0 + (double)cc * fd->fres;
        OnScan.VSource  = (float)(fd->v0 + (double)cc * fd->vres);
	OnScan.FirstIF = fd->firstIF * 1000.0;
	OnScan.flag[0] = fd->pol;
	XScan.LOFreq   = fd->lofreq  * 1000.0;
	OnScan.AirTemp = fd->TAir;
	OnScan.Pressure = fd->PAir;
	OnScan.Humidity = fd->RAir;
        for (i=0; i<fd->n; i++)
            OnScan.c[i] = (float)fd->d[i];
    }

    ClearFITSwords();
    
    if ((bitpix_out = GetTypeOfFITS()) == FITS_TYPE_UNKNOWN) {
        sprintf(buf, "Cannot handle FITS type=%d.", bitpix_out);
        send_line(buf);
        return 1;
    }
    
    BoolCard(KW_SIMPLE, TRUE);
    LongCard(KW_BITPIX, bitpix_out);
    LongCard(KW_NAXIS, 3);
    
    if (type == FITS_VECTOR) {
        LongCard(KW_NAXIS1, XScan.NChannel);
        LongCard(KW_NAXIS2, 1);
        LongCard(KW_NAXIS3, 1);
    } else if (type == FITS_ARRAY) {
        LongCard(KW_NAXIS1, fd->nX);
        LongCard(KW_NAXIS2, fd->nY);
        LongCard(KW_NAXIS3, 1);
    } else {
        LongCard(KW_NAXIS1, fd->nX);
        LongCard(KW_NAXIS2, fd->nY);
        LongCard(KW_NAXIS3, XScan.NChannel);
    }
    
    if (type == FITS_VECTOR) {
        CharCard(KW_CTYPE1, "FREQ");
        RealCard(KW_CRPIX1, (double)(CenterCh(&OnScan, &XScan)+1));
        RealCard(KW_CRVAL1, OnScan.RestFreq*1.0e6);
/*        RealCard(KW_CRVAL1, 0.0); */
        RealCard(KW_CDELT1, OnScan.FreqRes*1.0e6);
        if (OnScan.CSystem == COORD_TYPE_EQU) {
            CharCard(KW_CTYPE2, "RA---GLS");
            CharCard(KW_CTYPE3, "DEC--GLS");
        } else if (OnScan.CSystem == COORD_TYPE_HOR) {
            CharCard(KW_CTYPE2, "AZ---GLS");
            CharCard(KW_CTYPE3, "EL---GLS");
        } else if (OnScan.CSystem == COORD_TYPE_GAL) {
            CharCard(KW_CTYPE2, "GLON-GLS");
            CharCard(KW_CTYPE3, "GLAT-GLS");
        } else {
            CharCard(KW_CTYPE2, "RA---GLS");
            CharCard(KW_CTYPE3, "DEC--GLS");
        }
        RealCard(KW_CRPIX2, 0.0);
        RealCard(KW_CRVAL2, OnScan.Longitude*RADTODEG);
        RealCard(KW_CDELT2, OnScan.LMapOff*RADTODEG);
        RealCard(KW_CROTA2, 0.0*RADTODEG);
        RealCard(KW_CRPIX3, 0.0);
        RealCard(KW_CRVAL3, OnScan.Latitude*RADTODEG);
        RealCard(KW_CDELT3, OnScan.BMapOff*RADTODEG);
        RealCard(KW_CROTA3, 0.0*RADTODEG);
        CharCard(KW_BUNIT, "K");
    } else {
        if (OnScan.CSystem == COORD_TYPE_EQU) {
            CharCard(KW_CTYPE1, "RA---GLS");
            CharCard(KW_CTYPE2, "DEC--GLS");
        } else if (OnScan.CSystem == COORD_TYPE_HOR) {
            CharCard(KW_CTYPE1, "AZ---GLS");
            CharCard(KW_CTYPE2, "EL---GLS");
        } else if (OnScan.CSystem == COORD_TYPE_GAL) {
            CharCard(KW_CTYPE1, "GLON-GLS");
            CharCard(KW_CTYPE2, "GLAT-GLS");
        } else {
            CharCard(KW_CTYPE1, "RA---GLS");
            CharCard(KW_CTYPE2, "DEC--GLS");
        }
        RealCard(KW_CRPIX1, fd->refX);
        RealCard(KW_CRVAL1, fd->x0*RADTODEG);
        RealCard(KW_CDELT1, fd->xspa/3600.0);
        RealCard(KW_CROTA1, 0.0*RADTODEG);
        RealCard(KW_CRPIX2, fd->refY);
        RealCard(KW_CRVAL2, fd->y0*RADTODEG);
        RealCard(KW_CDELT2, fd->yspa/3600.0);
        RealCard(KW_CROTA2, fd->posang*RADTODEG);
        
        CharCard(KW_CTYPE3, "FREQ");
        if (type == FITS_ARRAY) {
            RealCard(KW_CRPIX3, 1);
        } else {
            RealCard(KW_CRPIX3, (double)(CenterCh(&OnScan, &XScan)+1));
        }
        RealCard(KW_CRVAL3, OnScan.RestFreq*1.0e6);
        RealCard(KW_CDELT3, OnScan.FreqRes*1.0e6);
        RealCard(KW_CROTA3, 0.0);
        if (type == FITS_ARRAY) {
            CharCard(KW_BUNIT, "K km/s");
        } else {
            CharCard(KW_BUNIT, "K");
        }
    }
    

    if (type == FITS_VECTOR) {
        max = min = OnScan.c[0];
        for (i = 1; i < (int)XScan.NChannel; i++) {
            Y = OnScan.c[i];
            if (Y > max) max = Y;
            if (Y < min) min = Y;
        }
    } else if (type == FITS_ARRAY) {
        first = 1;
        for (i=0; i<fd->nX; i++) {
            for (j=0; j<fd->nY; j++) {
                if (fd->flag[i][j] >= 0) {
                    if (first) {
                        min = max = fd->data[i][j];
                        first = 0;
                    } else {
                        Y = fd->data[i][j];
                        if (Y > max) max = Y;
                        if (Y < min) min = Y;
                    }
                }
            }
        }
    } else if (type == FITS_CUBE) {
        first = 1;
        for (n = 0; n < (int)XScan.NChannel; n++) {
            for (i=0; i<fd->nX; i++) {
                for (j=0; j<fd->nY; j++) {
                    k = fd->flag[i][j];
                    s = fd->sp[i][j];
                    if (k >= 0 && s) {
                        if (first) {
                            min = max = s->d[n];
                            first = 0;
                        } else {
                            Y = s->d[n];
                            if (Y > max) max = Y;
                            if (Y < min) min = Y;
                        }
                    }
                }
            }
        }
    }
    
    GetFITSScaleFactors(bitpix_out, min, max, &bz, &bs);

    RealCard(KW_BSCALE, bs);
    RealCard(KW_BZERO, bz);
    RealCard(KW_DATAMAX, (double)max);
    RealCard(KW_DATAMIN, (double)min);
    LongCard(KW_BLANK, GetBLANK(bitpix_out));

    if (OnScan.Year >= 1900 && OnScan.Year < 2000)
        sprintf(buf, "%02d/%02d/%02d",
                OnScan.Day, OnScan.Month, OnScan.Year%100);
    else {
        CharCard(KW_TIMESYS, "UTC");
        sprintf(buf, "%4d-%02d-%02dT%02d:%02d:%02d",
                OnScan.Year, OnScan.Month, OnScan.Day,
                OnScan.UTHour, OnScan.UTMin, OnScan.UTSec);
    }
    CharCard(KW_DATE_OBS, buf);
    time(&clock);
    now = gmtime(&clock);
    if (now->tm_year >= 0 && now->tm_year < 100) {
        sprintf(buf,"%02d/%02d/%02d",
                now->tm_mday, now->tm_mon+1, now->tm_year);
    } else {
        sprintf(buf, "%4d-%02d-%02dT%02d:%02d:%02d",
                now->tm_year+1900, now->tm_mon+1, now->tm_mday,
                now->tm_hour, now->tm_min, now->tm_sec);
    }
    CharCard(KW_DATE, buf);
    sprintf(buf, "%02d:%02d:%02d",
            OnScan.UTHour, OnScan.UTMin, OnScan.UTSec);
    CharCard(KW_UTC, buf);
    sprintf(buf, "%02d:%02d:%02d",
            OnScan.STHour, OnScan.STMin, OnScan.STSec);
    CharCard(KW_LST, buf);

    strncpy(buf, OnScan.Name, 12); buf[12] = '\0';
    CharCard(KW_OBJECT, buf);
    sprintf(buf, "%s %.1f.%d", PKGNAME, XS_VERSION, XS_PATCH); buf[12] = '\0';
    CharCard(KW_ORIGIN, buf);
    RealCard(KW_BMAJ, fd->b.maj/3600.0); 
    RealCard(KW_BMIN, fd->b.min/3600.0); 
    RealCard(KW_BPA,  fd->b.PA); 
    RealCard(KW_EQUINOX, (double)OnScan.Equinox);
    RealCard(KW_AZIMUTH, (double)OnScan.Azimuth*RADTODEG);
    RealCard(KW_ELEVATIO, (double)OnScan.Elevation*RADTODEG);
    RealCard(KW_AZOFF, (double)OnScan.AzMapOff*RADTODEG);
    RealCard(KW_ELOFF, (double)OnScan.ElMapOff*RADTODEG);
    RealCard(KW_AZCORR, (double)OnScan.AzOffset*RADTODEG);
    RealCard(KW_ELCORR, (double)OnScan.ElOffset*RADTODEG);
    RealCard(KW_RESTFREQ, OnScan.RestFreq*1.0e6);
    RealCard(KW_IMAGFREQ, (2.0*XScan.LOFreq-OnScan.RestFreq)*1.0e6);
/*
    switch ((OnScan.Ctrl&VELSYS) >> 2) {
      case 0:
        RealCard(KW_VELO_LSR, OnScan.VSource*1.0e3);
        break;
      case 1:
        RealCard(KW_VELO_HEL, OnScan.VSource*1.0e3);
        break;
      case 2:
        RealCard(KW_VELO_GEO, OnScan.VSource*1.0e3);
        break;
      default:
 */
        RealCard(KW_VELO_LSR, OnScan.VSource*1.0e3);
/*
        break;
    }
 */
    RealCard(KW_DELTAV, OnScan.VelRes*1.0e3);
    RealCard(KW_TSYS, (double)OnScan.Tsys);
    RealCard(KW_OBSTIME, (double)OnScan.IntTime);
    LongCard(KW_SCAN, (long)OnScan.ScanNo);
    RealCard(KW_TAU, (double)OnScan.Tau);
    RealCard(KW_DBLOAD, (double)OnScan.dBl);
    if (OnScan.flag[0] == POL_BOTH) {
        CharCard(KW_POLARIZA, "LCP+RCP");
    } else if (OnScan.flag[0] == POL_LCP) {
        CharCard(KW_POLARIZA, "LCP");
    } else if (OnScan.flag[0] == POL_RCP) {
        CharCard(KW_POLARIZA, "RCP");
    } else {
        CharCard(KW_POLARIZA, "UNKNOWN");
    }
    strncpy(buf, OnScan.Observer, 16); buf[16] = '\0';
    CharCard(KW_OBSERVER, buf);
    strncpy(buf, OnScan.Molecule, 18); buf[18] = '\0';
    CharCard(KW_LINE, buf);
    RealCard(KW_MAPTILT, OnScan.PosAngle*RADTODEG);
    RealCard(KW_TOUTSIDE, (double)OnScan.AirTemp);
    RealCard(KW_PRESSURE, (double)OnScan.Pressure*100.0);
    RealCard(KW_HUMIDITY, (double)OnScan.Humidity/100.0);
    VoidCard(KW_END);

    i = writeFITSheader(scanname);
    if (i != 0) {
        sprintf(buf, "Error in writeFITSheader().");
        send_line(buf);
        return 1;
    }
    
    if (type == FITS_VECTOR)
        n = XScan.NChannel;
    else if (type == FITS_ARRAY)
        n = fd->nX * fd->nY;
    else if (type == FITS_CUBE)
        n = fd->nX * fd->nY * XScan.NChannel;
    
    if (AllocFITSData(bitpix_out, n, &size, &data)) return 1;
    
    if (type == FITS_VECTOR) {
        for (i = 0; i < n; i++) {
            val = ((double)OnScan.c[i] - bz)/bs;
            SetFITSValue(bitpix_out, i, val);
        }
    } else if (type == FITS_ARRAY) {
        n = 0;
        for (j=0; j<fd->nY; j++) {
            for (i=0; i<fd->nX; i++) {
                if (fd->flag[i][j] >= UNBLANK) {
                    val = (fd->data[i][j] - bz)/bs;
                } else {
                    val = (double)GetBLANK(bitpix_out);
                }
                SetFITSValue(bitpix_out, n, val);
                n++;
            }
        }
    } else if (type == FITS_CUBE) {
        n = 0;
        for (k = 0; k < (int)XScan.NChannel; k++) {
            for (j=0; j<fd->nY; j++) {
                for (i=0; i<fd->nX; i++) {
                    cc = fd->flag[i][j];
                    s = fd->sp[i][j];
                    if (cc >= UNBLANK && s) {
                        val = (s->d[k] - bz)/bs;
                    } else {
                        val = (double)GetBLANK(bitpix_out);
                    }
                    SetFITSValue(bitpix_out, n, val);
                    n++;
                }
            }
        }
    }
    
    i = writeFITSdata(n, size, data);
    
    CloseFITSfile();
    FreeFITSData();
    
    if (i != n*size) {
        sprintf(buf, "Data written to file '%s' but only %d bytes of %d.",
                scanname, i, n*size);
    } else {
        if (type == FITS_VECTOR)
            sprintf(buf, "Scan saved as '%s'.", scanname);
        else if (type == FITS_ARRAY)
            sprintf(buf, "Map %dx%d saved as '%s'.", fd->nX, fd->nY, scanname);
        else
            sprintf(buf, "Cube %dx%dx%d saved as '%s'.",
                    fd->nX, fd->nY, (int)XScan.NChannel, scanname);
    }
    send_line(buf);

    return 0;
}
