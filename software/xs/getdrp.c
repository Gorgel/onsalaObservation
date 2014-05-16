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
#include <Xm/SelectioB.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>

#include "drp.h"
#include "defines.h"
#include "global_structs.h"

/*** External variables and structures ***/
extern SCAN   OnScan;
extern XSCAN  XScan;

/*** Local variables ***/
#ifdef SISYFOS
/* Sisyfos 2x2 array (Apr 2000) offset in arcsecs                  */
/* First entry is the reference: REF, COR1, COR2, COR3, COR4       */
static double SisyfosAz[] = {0.0,  0.0,  46.0,   0.0,  46.0};
static double SisyfosEl[] = {0.0,  0.0,   0.0, -50.0, -50.0};
#endif

int CenterCh(SCAN *S, XSCAN *X)
{
    return (X->NChannel/2);
}
  
double Velocity(int ch, SCAN *S, XSCAN *X)
{
    double Vel;

    if (ch < 0)             ch = 0;
    if (ch > X->NChannel-1) ch = X->NChannel-1;
    Vel = (double)(ch-CenterCh(S, X))*S->VelRes + S->VSource;

    return (Vel);
}
  
double VelOdin(int ch, SCAN *S, XSCAN *X)
{
    double Vel;

    if (ch < 0)             ch = 0;
    if (ch > X->NChannel-1) ch = X->NChannel-1;
    Vel = (double)(ch-CenterCh(S, X))*S->VelRes - S->VLsr + S->VSource;

    return (Vel);
}
  
double Frequency(int ch, SCAN *S, XSCAN *X)
{
    double Freq;

    if (ch < 0)             ch = 0;
    if (ch > X->NChannel-1) ch = X->NChannel-1;
    Freq = (double)(ch-CenterCh(S, X))*S->FreqRes + S->RestFreq;

    return (Freq);
}

void strip_trailing_spaces(char *s2)
{
    int i, n = strlen(s2);
    char c;
    
    for (i=n-1; i>=0; i--) {
        c = s2[i];
        if (c == ' ' || c == '\0' || c == '\n' || c == '\t' || c == '\b')
            s2[i] = '\0';
        else
            return;
    }
}

double rta(float rad)
{
    return (double)((double)rad * RADTOSEC);
}

void DRP2FD(SCAN *s, XSCAN *x, FDATA *fd)
{
    int i, odin=0;
    double *pa, cp, sp, sqrBt;
#ifdef SISYFOS
    int arr_no=0;
    double NorthAz, RAOffset, DecOffset;
#endif
#ifdef ONTHEFLY
    double NorthAz, RAOffset, DecOffset;
#endif
    
    double *CheckPosAngle();
    void SetCoordType();
    int CheckCRVALType();
    void GetEquOffsets(DATE *, double, double, double, double, double, double,
                       double *, double *);
    
    if (strncmp(s->Project, "Odin", 4)==0) odin=1;
    
    strncpy(fd->sname, s->Name, 12);
    fd->sname[12] = '\0';
    strip_trailing_spaces(fd->sname);
    strncpy(fd->molecule, s->Molecule, 18);
    fd->molecule[18] = '\0';
    strip_trailing_spaces(fd->molecule);
    fd->n = (int)x->NChannel;
    fd->sno = (int)s->ScanNo;
    fd->subno = (int)s->SubScan;
    fd->coordType = s->CSystem;
    SetCoordType(s->CSystem);
    if ((pa = CheckPosAngle()))
        fd->posang = (*pa)*PI/180.0;
    else
        fd->posang = (double)s->PosAngle;
    cp = cos(fd->posang);
    sp = sin(fd->posang);
    fd->xoff = rta(s->LMapOff)*cp - rta(s->BMapOff)*sp;
    fd->yoff = rta(s->BMapOff)*cp + rta(s->LMapOff)*sp;
    fd->equinox = s->Equinox;
    if (fd->equinox <= 1950.1) {
        fd->epoch = 'B';
    } else {
        fd->epoch = 'J';
    }
    fd->y0 = s->Latitude;
    fd->x0 = s->Longitude;
    if (CheckCRVALType()) {
        fd->y0 -= s->BMapOff;
        fd->x0 -= s->LMapOff/cos(fd->y0);
    }
    fd->scanType = s->ObsMode;
    fd->tsys = (double)s->Tsys;
    fd->tau  = (double)s->Tau;
    fd->int_time = (double)s->IntTime;
    fd->vlsr = s->VSource;
    fd->date.Year  = s->Year;
    fd->date.Month = s->Month;
    fd->date.Day   = s->Day;
    fd->date.Hour  = s->UTHour;
    fd->date.Min   = s->UTMin;
    fd->date.Sec   = s->UTSec;
    fd->LST = fd->date;
    fd->LST.Hour   = s->STHour;
    fd->LST.Min    = s->STMin;
    fd->LST.Sec    = s->STSec;
    fd->az = s->Azimuth * RADTODEG;
    fd->el = s->Elevation * RADTODEG;
    fd->aoff = rta(s->AzMapOff)*cp - rta(s->ElMapOff)*sp;
    fd->eoff = rta(s->ElMapOff)*cp + rta(s->AzMapOff)*sp;
#ifdef SISYFOS
    if (sscanf(s->Program, "COR%d", &arr_no) == 1) {
        if (arr_no >=1 && arr_no <= 4) {
            fd->aoff += (SisyfosAz[arr_no] - SisyfosAz[0]);
            fd->eoff += (SisyfosEl[arr_no] - SisyfosEl[0]);
            NorthAz = s->Azimuth + PI;
            GetEquOffsets(&(fd->LST), NorthAz, s->Elevation,
                          fd->aoff, fd->eoff,
                          s->Longitude, s->Latitude,
                          &RAOffset, &DecOffset);
            fd->xoff += RAOffset;
            fd->yoff += DecOffset;
        }
    }
#endif
#ifdef ONTHEFLY
    NorthAz = s->Azimuth + PI;
    GetEquOffsets(&(fd->LST), NorthAz, s->Elevation,
                  fd->aoff, fd->eoff,
                  s->Longitude, s->Latitude,
                  &RAOffset, &DecOffset);
    fd->xoff += RAOffset;
    fd->yoff += DecOffset;
#endif
    fd->b.maj = s->StepX;
    fd->b.min = s->StepY;
    fd->b.PA  = s->ParAngle;
    fd->beameff = s->RefCorr;
    fd->pol     = s->flag[0];
    fd->TAir = s->AirTemp; 
    fd->PAir = s->Pressure; 
    fd->RAir = s->Humidity; 
    
    fd->firstIF = s->FirstIF/1000.0;
    if (x) {
        fd->lofreq = x->LOFreq/1000.0;
    } else { /* Assume OSO */
        if (s->RestFreq < 105000.0) {
            fd->lofreq = (s->RestFreq + s->FirstIF)/1000.0;
	} else {
            fd->lofreq = (s->RestFreq - s->FirstIF)/1000.0;
	}
    }
    fd->skyfreq = s->SkyFreq/1000.0;
    
    if (s->FreqRes < 0.0) {
        fd->f0 = Frequency(x->NChannel-1, s, x)/1000.;
        fd->fn = Frequency(0, s, x)/1000.;
/*         printf("f0, fn=%f,%f  RF=%f  res=%f\n", fd->f0, fd->fn, s->RestFreq, s->FreqRes);
 */
        if (odin) {
            fd->v0 = VelOdin(x->NChannel-1, s, x);
        } else {
            fd->v0 = Velocity(x->NChannel-1, s, x);
        }
        fd->fres = -s->FreqRes;
        fd->vres = -s->VelRes;
        sqrBt = sqrt(fabs(fd->fres) * fd->int_time)*1000.0;
        for (i=x->NChannel-1; i>=0; i--) {
            fd->d[x->NChannel-1-i] = (double)(s->c[i]);
            if (sqrBt > 0.0 && fd->tsys > 0.0)
                fd->e[x->NChannel-1-i] = 2.0*fd->tsys/sqrBt;
            else
                fd->e[x->NChannel-1-i] = 1.0;
        }
    } else {
        fd->fn = Frequency(x->NChannel-1, s, x)/1000.;
        fd->f0 = Frequency(0, s, x)/1000.;
        if (odin) {
            fd->v0 = VelOdin(0, s, x);
        } else {
            fd->v0 = Velocity(0, s, x);
        }
        fd->fres = s->FreqRes;
        fd->vres = s->VelRes;
        sqrBt = sqrt(fabs(fd->fres) * fd->int_time)*1000.0;
        for (i=0; i<x->NChannel; i++) {
          fd->d[i] = (double)(s->c[i]);
          if (sqrBt > 0.0 && fd->tsys > 0.0)
              fd->e[i] = 2.0*fd->tsys/sqrBt;
          else
              fd->e[i] = 1.0;
        }
    }
}

void SetCSystemFromDRP(SCAN *s)
{
    if (!s) return;
    
    switch (s->CSystem) { /* Convert CSystem from DRP value to XSpec value */
        case 1: /* EQU 2000.0     */
        case 2: /* EQU s->Equinox */
        case 3: /* EQU (mean)     */
        case 4: /* EQU (true)     */
            s->CSystem = COORD_TYPE_EQU;
            break;
        case 5:
            s->CSystem = COORD_TYPE_GAL;
            break;
        case 6:
            s->CSystem = COORD_TYPE_HOR;
            break;
        default:
            s->CSystem = COORD_TYPE_EQU;
            break;
    }
}

static void swapbytes(char *p, int n)
{
    char swap;
    int i;

    for (i = 0; i < n/2; i++) {
        swap = p[i];
        p[i] = p[n-1-i];
        p[n-1-i] = swap;
    }
}

static void swap_header(SCAN *s)
{
    int i;
    
    /* Swapping of Slength, NChannel, and ScanNo is already done
       in get_drp() */

    /* swapbytes((char *)&(s->Slength), sizeof(short)); */
    /* swapbytes((char *)&(s->NChannel), sizeof(short)); */
    swapbytes((char *)&(s->Ctrl), sizeof(short));
    /* swapbytes((char *)&(s->ScanNo), sizeof(short)); */
    swapbytes((char *)&(s->SubScan), sizeof(short));
    swapbytes((char *)&(s->Year), sizeof(short));
    swapbytes((char *)&(s->Month), sizeof(short));
    swapbytes((char *)&(s->Day), sizeof(short));
    swapbytes((char *)&(s->UTHour), sizeof(short));
    swapbytes((char *)&(s->UTMin), sizeof(short));
    swapbytes((char *)&(s->UTSec), sizeof(short));
    swapbytes((char *)&(s->STHour), sizeof(short));
    swapbytes((char *)&(s->STMin), sizeof(short));
    swapbytes((char *)&(s->STSec), sizeof(short));
    swapbytes((char *)&(s->CSystem), sizeof(short));
    swapbytes((char *)&(s->ObsMode), sizeof(short));
    swapbytes((char *)&(s->Backend), sizeof(short));
    swapbytes((char *)&(s->Frontend), sizeof(short));
    swapbytes((char *)&(s->MapX), sizeof(short));
    swapbytes((char *)&(s->MapY), sizeof(short));
    
    swapbytes((char *)&(s->JulDate), sizeof(long));

    swapbytes((char *)&(s->AirTemp), sizeof(float));
    swapbytes((char *)&(s->Pressure), sizeof(float));
    swapbytes((char *)&(s->Humidity), sizeof(float));
    swapbytes((char *)&(s->Equinox), sizeof(float));
    swapbytes((char *)&(s->EquiNow), sizeof(float));
    swapbytes((char *)&(s->Longitude), sizeof(float));
    swapbytes((char *)&(s->Latitude), sizeof(float));
    swapbytes((char *)&(s->Long2000), sizeof(float));
    swapbytes((char *)&(s->Lat2000), sizeof(float));
    swapbytes((char *)&(s->LMapOff), sizeof(float));
    swapbytes((char *)&(s->BMapOff), sizeof(float));
    swapbytes((char *)&(s->Azimuth), sizeof(float));
    swapbytes((char *)&(s->Elevation), sizeof(float));
    swapbytes((char *)&(s->AzOffset), sizeof(float));
    swapbytes((char *)&(s->ElOffset), sizeof(float));
    swapbytes((char *)&(s->AzMapOff), sizeof(float));
    swapbytes((char *)&(s->ElMapOff), sizeof(float));
    swapbytes((char *)&(s->AzPointg), sizeof(float));
    swapbytes((char *)&(s->ElPointg), sizeof(float));
    swapbytes((char *)&(s->AzErrAve), sizeof(float));
    swapbytes((char *)&(s->ElErrAve), sizeof(float));
    swapbytes((char *)&(s->AzErrRms), sizeof(float));
    swapbytes((char *)&(s->ElErrRms), sizeof(float));
    swapbytes((char *)&(s->GalLong), sizeof(float));
    swapbytes((char *)&(s->GalLat), sizeof(float));
    swapbytes((char *)&(s->VHel), sizeof(float));
    swapbytes((char *)&(s->VLsr), sizeof(float));
    swapbytes((char *)&(s->Axial), sizeof(float));
    swapbytes((char *)&(s->Shift), sizeof(float));
    swapbytes((char *)&(s->VTilt), sizeof(float));
    swapbytes((char *)&(s->HTilt), sizeof(float));
    swapbytes((char *)&(s->Tcal), sizeof(float));
    swapbytes((char *)&(s->Tsys), sizeof(float));
    swapbytes((char *)&(s->Trec), sizeof(float));
    swapbytes((char *)&(s->Tau), sizeof(float));
    swapbytes((char *)&(s->dBl), sizeof(float));
    swapbytes((char *)&(s->IntTime), sizeof(float));
    swapbytes((char *)&(s->RefCorr), sizeof(float));
    swapbytes((char *)&(s->ParAngle), sizeof(float));
    swapbytes((char *)&(s->PosAngle), sizeof(float));
    swapbytes((char *)&(s->StepX), sizeof(float));
    swapbytes((char *)&(s->StepY), sizeof(float));
    
    swapbytes((char *)&(s->Bandwidth), sizeof(double));
    swapbytes((char *)&(s->RestFreq), sizeof(double));
    swapbytes((char *)&(s->SkyFreq), sizeof(double));
    swapbytes((char *)&(s->FirstIF), sizeof(double));
    swapbytes((char *)&(s->FreqThrow), sizeof(double));
    swapbytes((char *)&(s->FreqRes), sizeof(double));
    swapbytes((char *)&(s->VSource), sizeof(double));
    swapbytes((char *)&(s->VelRes), sizeof(double));
    
    for (i=0; i<10; i++) swapbytes((char *)&(s->work[i]), sizeof(float));
    
    for (i=0; i<31; i++) swapbytes((char *)&(s->flag[i]), sizeof(short));
}

static void swap_data(SCAN *s)
{
    int i;
    
    for (i=0; i<s->NChannel; i++) {
        swapbytes((char *)&(s->c[i]), sizeof(float));
    }
}

int get_drp(char *scanname, FDATA *fd)
{
    int h, d, swapped=0;
    SCAN *s = &OnScan;
    XSCAN *x = &XScan;
    FILE *fp;
    string buf;
    
    void send_line();
    double rta(float);
    char *GetRAStr(double), *GetDECStr(double);
    char *GetLongStr(double), *GetLatStr(double);
    int CheckDataSize(int);

    fp = fopen(scanname, "r");
    if (!fp) {
        sprintf(buf, "Couldn't find DRP file '%s'.", scanname);
        send_line(buf);
        return -1;
    }
#ifdef DEBUG
    printf("File %s opened.\n", scanname);
#endif

    h = fread((char *)s, 1, HEADER, fp);
    if (h != HEADER) {
        sprintf(buf, "Error while reading header of DRP file '%s'.", scanname);
        send_line(buf);
        fclose(fp);
        return -1;
    }
#ifdef DEBUG
    printf("Header (%d) read.\n", h);
    printf("NChannel=%d\n", s->NChannel);
    printf("ScanNo=%d\n", s->ScanNo);
    printf("SLength=%d\n", s->Slength);
#endif
    
    if (s->NChannel <= 0 || s->ScanNo < 0) {
        sprintf(buf, "Strange header: ScanNo=%d, NChan=%d, Slength=%d.",
                s->ScanNo, s->NChannel, s->Slength);
        send_line(buf);
        swapbytes((char *)&(s->NChannel), sizeof(short));
        swapbytes((char *)&(s->ScanNo), sizeof(short));
        swapbytes((char *)&(s->Slength), sizeof(short));
#ifdef DEBUG
        printf("Byte swapped values:\n");
        printf("NChannel=%d\n", s->NChannel);
        printf("ScanNo=%d\n", s->ScanNo);
        printf("SLength=%d\n", s->Slength);
#endif
        sprintf(buf, "Let's try byte swapping: NChannel now becomes %d.",
                s->NChannel);
        send_line(buf);
        if (s->NChannel > 0 && s->ScanNo >= 0 && s->Slength > 0) {
                  /* Let's try byteswapping */
            send_line("This looks better. Let's go with swapped bytes.");
            swapped = 1;
            swap_header(s);
        } else {
                  /* Give up on this scan */
            send_line("It doesn't look better. Can't read this.");
            fclose(fp);
            return -1;
        }
    }
#ifdef DEBUG
    printf("Need swapping? (1=yes) %d.\n", swapped);
#endif
    
    if (CheckDataSize(s->NChannel) < s->NChannel) {
        sprintf(buf, "Error while allocating memory, NChan=%d.", s->NChannel);
        send_line(buf);
        fclose(fp);
        return -1;
    }
#ifdef DEBUG
    printf("NChannel=%d.\n", s->NChannel);
#endif
    
    if (s->Slength != (short)DRPVERSION) {
	    if (s->Slength == HEADER + 2*s->NChannel) {
	        /* old DRP scan, correct position for offsets */
	        s->Longitude -= s->LMapOff/cos(s->Latitude);
	        s->Latitude -= s->BMapOff;
	    }
	    s->Slength = DRPVERSION;  /* indicate DRP version */
        sprintf(buf, "Detecting old DRP format in '%s' -- converting.",
                scanname);
        send_line(buf);
    }
    
    s->Name[11]     = '\0';
    s->Project[3]   = '\0';
    s->Observer[15] = '\0';
    s->Program[15]  = '\0';
    s->Molecule[17] = '\0';

    SetCSystemFromDRP(s);
    
    if (!fd) {
        fclose(fp);
        return 0;
    }

    d = fread((char *)s->c, sizeof(float), s->NChannel, fp);
    if (d != s->NChannel) {
        sprintf(buf, "Error (%d != %d) while reading data of DRP file '%s'.",
                d, s->NChannel, scanname);
        send_line(buf);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    
    if (swapped) swap_data(s);
    
    x->NChannel = s->NChannel;
    DRP2FD(s, x, fd);
    
    if (fd->coordType == COORD_TYPE_GAL) {
        sprintf(buf, "DRP (%s: %s %s %5.1f,%5.1f) scan %d read.\n", fd->sname,
            GetLongStr(fd->x0), GetLatStr(fd->y0), fd->xoff, fd->yoff, fd->sno);
    } else {
        sprintf(buf, "DRP (%s: %s %s %5.1f,%5.1f) scan %d read.\n", fd->sname,
            GetRAStr(fd->x0), GetDECStr(fd->y0), fd->xoff, fd->yoff, fd->sno);
    }
    send_line(buf);
    
    return 0;
}
