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

#include <Xm/Xm.h>
#include "list.h"
#include "drp.h"
#include "defines.h"
#include "global_structs.h"
#include "fits.h"

/*** External variables ***/
extern SCAN OnScan;
extern XSCAN XScan;

/*** Local variables ***/
#define BT_UNKNOWN -1
#define BT_2_INT    0
#define BT_4_INT    1
#define BT_ASCII    2
#define BT_4_FLT    3
#define BT_8_FLT    4

#define NULLMAP  {NULL, NULL, 0.0, NULL, NULL}

/* Odin defs */
#define BACK_AC1   1
#define BACK_AC2   2
#define BACK_AOS   3

#define TYPE_SIG   1
#define TYPE_REF   2
#define TYPE_CAL   3
#define TYPE_CMB   4
#define TYPE_DRK   5
#define TYPE_SK1   6
#define TYPE_SK2   7
#define TYPE_SPE   8
#define TYPE_SSB   9
#define TYPE_AVE  10

#define ILINEAR      0x01000000 
#define ISORTED      0x02000000

typedef struct {
    char *type;
    generic_ptr v;
    double c;
    char *form;
    char *unit;
} Mapping;

struct odin {
    short  Version;
    short  Level;
    long   Quality;
    long   STW;
    double MJD;
    double Orbit;
    float  LST;
    char   Source[32];
    short  Discipline;
    short  Topic;
    short  Spectrum;
    short  ObsMode;
    short  Type;
    short  Frontend;
    short  Backend;
    short  SkyBeamHit;
    float  RA2000;
    float  Dec2000;
    float  VSource;
    float  MapXoff;
    float  MapYoff;
    float  MapTilt;
    double Qtarget[4];
    double Qachieved[4];
    double Qerror[3];
    double GPSpos[3];
    double GPSvel[3];
    double SunPos[3];
    double MoonPos[3];
    float  SunZD;
    float  Vgeo;
    float  Vlsr;
    float  Tcal;
    float  Tsys;
    float  SBpath;
    double LOFreq;
    double SkyFreq;
    double RestFreq;
    double MaxSuppression;
    double FreqThrow;
    double FreqRes;
    double FreqCal[4];
    long   IntMode;
    float  IntTime;
    float  EffTime;
    long   Channels;
    float  *data;
};

typedef struct odin OdinScan;

static OdinScan Odin;

static Mapping SEST_bt[] = {
    {"Version ",   NULL,                             1.0, NULL, NULL},
    {"NChannel",   (generic_ptr)(&OnScan.NChannel),  1.0, NULL, NULL},
    {"Ctrl    ",   (generic_ptr)(&OnScan.Ctrl),      1.0, NULL, NULL},
    {"ScanNo  ",   (generic_ptr)(&OnScan.ScanNo),    1.0, NULL, NULL},
    {"SubScan ",   (generic_ptr)(&OnScan.SubScan),   1.0, NULL, NULL},
    {"Year    ",   (generic_ptr)(&OnScan.Year),      1.0, NULL, NULL},
    {"Month   ",   (generic_ptr)(&OnScan.Month),     1.0, NULL, NULL},
    {"Day     ",   (generic_ptr)(&OnScan.Day),       1.0, NULL, NULL},
    {"UTHour  ",   (generic_ptr)(&OnScan.UTHour),    1.0, NULL, NULL},
    {"UTMin   ",   (generic_ptr)(&OnScan.UTMin),     1.0, NULL, NULL},
    {"UTSec   ",   (generic_ptr)(&OnScan.UTSec),     1.0, NULL, NULL},
    {"STHour  ",   (generic_ptr)(&OnScan.STHour),    1.0, NULL, NULL},
    {"STMin   ",   (generic_ptr)(&OnScan.STMin),     1.0, NULL, NULL},
    {"STSec   ",   (generic_ptr)(&OnScan.STSec),     1.0, NULL, NULL},
    {"CSystem ",   (generic_ptr)(&OnScan.CSystem),   1.0, NULL, NULL},
    {"ObsMode ",   (generic_ptr)(&OnScan.ObsMode),   1.0, NULL, NULL},
    {"Backend ",   (generic_ptr)(&OnScan.Backend),   1.0, NULL, NULL},
    {"Frontend",   (generic_ptr)(&OnScan.Frontend),  1.0, NULL, NULL},
    {"MapX    ",   (generic_ptr)(&OnScan.MapX),      1.0, NULL, NULL},
    {"MapY    ",   (generic_ptr)(&OnScan.MapY),      1.0, NULL, NULL},
    {"JulDate ",   (generic_ptr)(&OnScan.JulDate),   1.0, NULL, NULL},
    {"Name    ",   (generic_ptr)(OnScan.Name),       1.0, NULL, NULL},
    {"Project ",   (generic_ptr)(OnScan.Project),    1.0, NULL, NULL},
    {"Observer",   (generic_ptr)(OnScan.Observer),   1.0, NULL, NULL},
    {"Program ",   (generic_ptr)(OnScan.Program),    1.0, NULL, NULL},
    {"Molecule",   (generic_ptr)(OnScan.Molecule),   1.0, NULL, NULL},
    {"Fuellsel_1", NULL,                             1.0, NULL, NULL},
    {"AirTemp ",   (generic_ptr)(&OnScan.AirTemp),   1.0, NULL, NULL},
    {"Pressure",   (generic_ptr)(&OnScan.Pressure),  1.0, NULL, NULL},
    {"Humidity",   (generic_ptr)(&OnScan.Humidity),  1.0, NULL, NULL},
    {"Equinox ",   (generic_ptr)(&OnScan.Equinox),   1.0, NULL, NULL},
    {"EquiNow ",   (generic_ptr)(&OnScan.EquiNow),   1.0, NULL, NULL},
    {"Longitude",  (generic_ptr)(&OnScan.Longitude), 1.0/RADTODEG, NULL, NULL},
    {"Latitude",   (generic_ptr)(&OnScan.Latitude),  1.0/RADTODEG, NULL, NULL},
    {"Long2000",   (generic_ptr)(&OnScan.Long2000),  1.0/RADTODEG, NULL, NULL},
    {"Lat2000 ",   (generic_ptr)(&OnScan.Lat2000),   1.0/RADTODEG, NULL, NULL},
    {"LMapOff ",   (generic_ptr)(&OnScan.LMapOff),   1.0, NULL, NULL},
    {"BMapOff ",   (generic_ptr)(&OnScan.BMapOff),   1.0, NULL, NULL},
    {"Azimuth ",   (generic_ptr)(&OnScan.Azimuth),   1.0/RADTODEG, NULL, NULL},
    {"Elevation",  (generic_ptr)(&OnScan.Elevation), 1.0/RADTODEG, NULL, NULL},
    {"AzOffset",   (generic_ptr)(&OnScan.AzOffset),  1.0/RADTODEG, NULL, NULL},
    {"ElOffset",   (generic_ptr)(&OnScan.ElOffset),  1.0/RADTODEG, NULL, NULL},
    {"AzMapOff",   (generic_ptr)(&OnScan.AzMapOff),  1.0/RADTODEG, NULL, NULL},
    {"ElMapOff",   (generic_ptr)(&OnScan.ElMapOff),  1.0/RADTODEG, NULL, NULL},
    {"AzPointg",   (generic_ptr)(&OnScan.AzPointg),  1.0/RADTODEG, NULL, NULL},
    {"ElPointg",   (generic_ptr)(&OnScan.ElPointg),  1.0/RADTODEG, NULL, NULL},
    {"AzErrAve",   (generic_ptr)(&OnScan.AzErrAve),  1.0/RADTODEG, NULL, NULL},
    {"ElErrAve",   (generic_ptr)(&OnScan.ElErrAve),  1.0/RADTODEG, NULL, NULL},
    {"AzErrRms",   (generic_ptr)(&OnScan.AzErrRms),  1.0/RADTODEG, NULL, NULL},
    {"ElErrRms",   (generic_ptr)(&OnScan.ElErrRms),  1.0/RADTODEG, NULL, NULL},
    {"GalLong ",   (generic_ptr)(&OnScan.GalLong),   1.0/RADTODEG, NULL, NULL},
    {"GalLat  ",   (generic_ptr)(&OnScan.GalLat),    1.0/RADTODEG, NULL, NULL},
    {"VHel    ",   (generic_ptr)(&OnScan.VHel),      1.0e5, NULL, NULL},
    {"VLsr    ",   (generic_ptr)(&OnScan.VLsr),      1.0e5, NULL, NULL},
    {"Axial   ",   (generic_ptr)(&OnScan.Axial),     1.0, NULL, NULL},
    {"Shift   ",   (generic_ptr)(&OnScan.Shift),     1.0, NULL, NULL},
    {"VTilt   ",   (generic_ptr)(&OnScan.VTilt),     1.0, NULL, NULL},
    {"HTilt   ",   (generic_ptr)(&OnScan.HTilt),     1.0, NULL, NULL},
    {"Tcal    ",   (generic_ptr)(&OnScan.Tcal),      1.0, NULL, NULL},
    {"Tsys    ",   (generic_ptr)(&OnScan.Tsys),      1.0, NULL, NULL},
    {"Trec    ",   (generic_ptr)(&OnScan.Trec),      1.0, NULL, NULL},
    {"Tau     ",   (generic_ptr)(&OnScan.Tau),       1.0, NULL, NULL},
    {"dBl     ",   (generic_ptr)(&OnScan.dBl),       1.0, NULL, NULL},
    {"IntTime ",   (generic_ptr)(&OnScan.IntTime),   1.0, NULL, NULL},
    {"RefCorr ",   (generic_ptr)(&OnScan.RefCorr),   1.0, NULL, NULL},
    {"ParAngle",   (generic_ptr)(&OnScan.ParAngle),  1.0/RADTODEG, NULL, NULL},
    {"PosAngle",   (generic_ptr)(&OnScan.PosAngle),  1.0/RADTODEG, NULL, NULL},
    {"StepX   ",   (generic_ptr)(&OnScan.StepX),     1.0/RADTODEG, NULL, NULL},
    {"StepY   ",   (generic_ptr)(&OnScan.StepY),     1.0/RADTODEG, NULL, NULL},
    {"Bandwidth",  (generic_ptr)(&OnScan.Bandwidth), 1.0, NULL, NULL},
    {"RestFreq",   (generic_ptr)(&OnScan.RestFreq),  1.0, NULL, NULL},
    {"SkyFreq ",   (generic_ptr)(&OnScan.SkyFreq),   1.0, NULL, NULL},
    {"FirstIF ",   (generic_ptr)(&OnScan.FirstIF),   1.0, NULL, NULL},
    {"FreqThrow",  (generic_ptr)(&OnScan.FreqThrow), 1.0, NULL, NULL},
    {"FreqRes ",   (generic_ptr)(&OnScan.FreqRes),   1.0, NULL, NULL},
    {"VSource ",   (generic_ptr)(&OnScan.VSource),   1.0e5, NULL, NULL},
    {"VelRes  ",   (generic_ptr)(&OnScan.VelRes),   -1.0e5, NULL, NULL},
    {"work    ",   (generic_ptr)(OnScan.work),       1.0, NULL, NULL},
    {"flag    ",   (generic_ptr)(OnScan.flag),       1.0, NULL, NULL},
    {"Fuellsel_2", NULL,                             1.0, NULL, NULL},
    {"Channels",   NULL, 1.0, NULL, NULL},
NULLMAP
};

static Mapping Odin_bt[] = {
    {"Version ",   (generic_ptr)(&Odin.Version),    1.0, "1I      ", NULL},
    {"Level   ",   (generic_ptr)(&Odin.Level),      1.0, "1I      ", NULL},
    {"Quality ",   (generic_ptr)(&Odin.Quality),    1.0, "1J      ", NULL},
    {"STW     ",   (generic_ptr)(&Odin.STW),        1.0, "1J      ", NULL},
    {"MJD     ",   (generic_ptr)(&Odin.MJD),        1.0, "1D      ", NULL},
    {"Orbit   ",   (generic_ptr)(&Odin.Orbit),      1.0, "1D      ", NULL},
    {"LST     ",   (generic_ptr)(&Odin.LST),        1.0, "1E      ", NULL},
    {"Source  ",   (generic_ptr)(Odin.Source),      1.0, "32A     ", NULL},
    {"Discipline", (generic_ptr)(&Odin.Discipline), 1.0, "1I      ", NULL},
    {"Topic   ",   (generic_ptr)(&Odin.Topic),      1.0, "1I      ", NULL},
    {"Spectrum",   (generic_ptr)(&Odin.Spectrum),   1.0, "1I      ", NULL},
    {"ObsMode ",   (generic_ptr)(&Odin.ObsMode),    1.0, "1I      ", NULL},
    {"Type    ",   (generic_ptr)(&Odin.Type),       1.0, "1I      ", NULL},
    {"Frontend",   (generic_ptr)(&Odin.Frontend),   1.0, "1I      ", NULL},
    {"Backend ",   (generic_ptr)(&Odin.Backend),    1.0, "1I      ", NULL},
    {"SkyBeamHit", (generic_ptr)(&Odin.SkyBeamHit), 1.0, "1I      ", NULL},
    {"RA2000  ",   (generic_ptr)(&Odin.RA2000),     1.0, "1E      ", "degrees "},
    {"Dec2000 ",   (generic_ptr)(&Odin.Dec2000),    1.0, "1E      ", "degrees "},
    {"VSource ",   (generic_ptr)(&Odin.VSource),    1.0, "1E      ", "m/s     "},
    {"MapXoff ",   (generic_ptr)(&Odin.MapXoff),    1.0, "1E      ", "degrees "},
    {"MapYoff ",   (generic_ptr)(&Odin.MapYoff),    1.0, "1E      ", "degrees "},
    {"MapTilt ",   (generic_ptr)(&Odin.MapTilt),    1.0, "1E      ", "degrees "},
    {"Qtarget ",   (generic_ptr)(Odin.Qtarget),     1.0, "4D      ", NULL},
    {"Qachieved",  (generic_ptr)(Odin.Qachieved),   1.0, "4D      ", NULL},
    {"Qerror  ",   (generic_ptr)(Odin.Qerror),      1.0, "3D      ", "degrees "},
    {"GPSpos  ",   (generic_ptr)(Odin.GPSpos),      1.0, "3D      ", "m       "},
    {"GPSvel  ",   (generic_ptr)(Odin.GPSvel),      1.0, "3D      ", "m/s     "},
    {"SunPos  ",   (generic_ptr)(Odin.SunPos),      1.0, "3D      ", "m       "},
    {"MoonPos ",   (generic_ptr)(Odin.MoonPos),     1.0, "3D      ", "m       "},
    {"SunZD   ",   (generic_ptr)(&Odin.SunZD),      1.0, "1E      ", "degrees "},
    {"Vgeo    ",   (generic_ptr)(&Odin.Vgeo),       1.0, "1E      ", "m/s     "},
    {"Vlsr    ",   (generic_ptr)(&Odin.Vlsr),       1.0, "1E      ", "m/s     "},
    {"Tcal    ",   (generic_ptr)(&Odin.Tcal),       1.0, "1E      ", "K       "},
    {"Tsys    ",   (generic_ptr)(&Odin.Tsys),       1.0, "1E      ", "K       "},
    {"SBpath  ",   (generic_ptr)(&Odin.SBpath),     1.0, "1E      ", "m       "},
    {"LOFreq  ",   (generic_ptr)(&Odin.LOFreq),     1.0, "1D      ", "Hz      "},
    {"SkyFreq ",   (generic_ptr)(&Odin.SkyFreq),    1.0, "1D      ", "Hz      "},
    {"RestFreq",   (generic_ptr)(&Odin.RestFreq),   1.0, "1D      ", "Hz      "},
    {"MaxSuppression", (generic_ptr)(&Odin.MaxSuppression), 1.0, "1D      ", "Hz      "},
    {"FreqThrow",  (generic_ptr)(&Odin.FreqThrow),  1.0, "1D      ", "Hz      "},
    {"FreqRes ",   (generic_ptr)(&Odin.FreqRes),    1.0, "1D      ", "Hz      "},
    {"FreqCal ",   (generic_ptr)(Odin.FreqCal),     1.0, "4D      ", "Hz      "},
    {"IntMode ",   (generic_ptr)(&Odin.IntMode),    1.0, "1J      ", NULL},
    {"IntTime ",   (generic_ptr)(&Odin.IntTime),    1.0, "1E      ", "s       "},
    {"EffTime ",   (generic_ptr)(&Odin.EffTime),    1.0, "1E      ", "s       "},
    {"Channels",   (generic_ptr)(&Odin.Channels),   1.0, "1J      ", NULL},
    {"data    ",   NULL, 1.0, NULL, NULL},
NULLMAP
};

typedef union {
    short i;
    int j;
    float e;
    double d;
    char *a;
} UNION;

static char FormCodes[] = {'I', 'J', 'A', 'E', 'D'}; 
static int ByteSizes[]  = {2, 4, 1, 4, 8};

static list fieldlist;

void init_fieldlist()
{
    status init_list();

    init_list(&fieldlist);
}

list *get_fieldlist()
{
    return &fieldlist;
}

int count_fields()
{
    int n = 0;
    list curr = NULL;

    list list_iterator();
    bool empty_list();

    if (empty_list(&fieldlist) == tRUE)
        return 0;

    while ((curr = list_iterator(fieldlist, curr)) != NULL)
        n++;

    return n;
}

static field *cleanup_ret(field *f)
{
    if (f) {
        free(f);
    }
    return NULL;
}

static field *new_field()
{
    field *f = NULL;

    status append(list *, generic_ptr);
    
    f = (field *)malloc(sizeof(field));
    if (!f)
        return cleanup_ret(f);

    if (append(&fieldlist, (generic_ptr)f) == Error)
        return cleanup_ret(f);

    f->cnt = 0;

    return f;
}

static list *delete_field(list *pL, list node)
{
    field *f = NULL;

    status delete_node();
    bool empty_list();

    if (empty_list(pL) == tRUE || !node)
        return NULL;

    f = (field *)DATA(node);
    
    if (f) cleanup_ret(f);

    if (delete_node(pL, node) == Error)
        return NULL;

    return pL;
}

list field_iterator(list last)
{
    return (last == NULL) ? fieldlist : NEXT(last);
}

void destroy_fieldlist()
{
    list curr = fieldlist;

    while (delete_field(&curr, curr) != NULL)
        ;
        
    init_fieldlist();
}

long int FieldByteSize()
{
    long int bytes = 0;
    list curr = NULL;
    field *f;
    
    while ((curr = field_iterator(curr)) != NULL) {
        f = (field *)DATA(curr);
        if (!f) continue;
        if (f->n > 0) bytes += (f->n * f->bytes);
    }
    
    return bytes;
}

static int GetMappingIndex(field *f, Mapping *m)
{
    int n = 0;
    Mapping *M;
    
    while ((M = &(m[n]))) {
        if (strcmp(M->type, f->type) == 0)
            return (M->v ? n : -1);
        n++;
    }
    
    return -1;
}

field *NewType(int cnt, char *str)
{
    field *f = new_field();
    
    if (!f) return f;
    
    f->cnt = cnt;
    f->format = BT_UNKNOWN;
    f->n = 0;
    f->bytes = 0;
    
    if (str) {
        strncpy(f->type, str, 16);
        f->type[16] = '\0';
    }
    
    return f;
}

static int GetFormType(char *str)
{
    int i;
    
    if (!str) return BT_UNKNOWN;
    
    /* if (strchr(str, 'I')) return BT_2_INT;
    if (strchr(str, 'J')) return BT_4_INT;
    if (strchr(str, 'A')) return BT_ASCII;
    if (strchr(str, 'E')) return BT_4_FLT;
    if (strchr(str, 'D')) return BT_8_FLT; */
    
    for (i=0; i<=BT_8_FLT; i++) {
        if (strchr(str, (int)FormCodes[i])) return i;
    }
    
    return BT_UNKNOWN;
}

static int GetFormN(char *str, int type)
{
    string tmp;
    char *p, t[2];
    
    if (!str || type < 0 || type > BT_8_FLT) return -1;
    
    strcpy(tmp, str);
    
    t[0] = FormCodes[type];
    t[1] = '\0';
    
    p = strtok(tmp, t);
    if (!p) return -1;
    
    return atoi(p);
}

void AddForm(field *f, char *str)
{
    if (!f) return;
    
    f->format = GetFormType(str);
    if (f->format == BT_UNKNOWN) return;
    
    f->bytes = ByteSizes[f->format]; 
    
    f->n = GetFormN(str, f->format);
}

void AddUnit(field *f, char *str)
{
    if (!f || !str) return;
    
    strncpy(f->unit, str, 16);
    f->unit[16] = '\0';
}


static Mapping *SetupMapping(int BT_type)
{
    list curr = NULL;
    field *f;
    Mapping *m = NULL;
    
    if (BT_type == 0)
        m = SEST_bt;
    else if (BT_type == 1)
        m = Odin_bt;
    else
        return m;
    
    while ((curr = field_iterator(curr)) != NULL) {
        f = (field *)DATA(curr);
        f->nmap = GetMappingIndex(f, m);
    }
    
    return m;
}

void AddDRPValue(field *f, UNION *u, int n, Mapping *mapping, int type)
{
    int     m;
    char   *pa;
    short  *pi;
    long   *pj;
    float  *pe;
    double *pd;
    Mapping *M = NULL;
    
    if (!f || !mapping) return;
    
    if ((m = f->nmap) < 0) return;
    
    M = &(mapping[m]);
    
    switch (f->format) {
        case BT_2_INT:
            pi = (short *)M->v;
            pi[n] = (type == 1 ? GET2(u->i) : AGET2(u->i));
            break;
        case BT_4_INT:
            pj = (long *)M->v;
            pj[n] = (type == 1 ? GET4(u->j) : AGET4(u->j));
            break;
        case BT_ASCII:
            pa = (char *)M->v;
            if (n < 18) {
                strncpy(pa, u->a, n);
                pa[n] = '\0';
            } else {
                strncpy(pa, u->a, 17);
                pa[17] = '\0';
            }
            break;
        case BT_4_FLT:
            pe = (float *)M->v;
            pe[n] = ((float)(M->c)) * (type == 1 ? GETF(u->e) : AGETF(u->e));
            break;
        case BT_8_FLT:
            pd = (double *)M->v;
            pd[n] = (M->c) * (type == 1 ? GETD(u->d) : AGETD(u->d));
            break;
    }
}

int GetBTrowsize(int nchan, int type, int *nf)
{
    int n, size=0, format;
    Mapping *m = NULL;
    
    if (type == 0) { /* SEST otf */
      m = SEST_bt;
    } else if (type == 1) { /* Odin */
      m = Odin_bt;
    } else {
      return 0;
    }
    
    n = 0;
    
    while (m->type) {
      if (m->form) {
        format = GetFormType(m->form);
        if (format == BT_UNKNOWN) return -1;
        size += GetFormN(m->form, format) * ByteSizes[format];
      }
      m++;
      n++;
    }
    
    size += nchan * 4;
    
    if (nf) *nf = n;
    
    return size;
}

void FillInTCards(int type, int nchan, fkey *fk)
{
    int n=0;
    Mapping *m = NULL;
    fkey *t;

    void CharCard(int , char *);
    void VoidCard(int);
    
    if (type == 0) { /* SEST otf */
      m = SEST_bt;
    } else if (type == 1) { /* Odin */
      m = Odin_bt;
    } else {
      return;
    }
    
    t = fk;
    
    while (m->type) {
      n++;
      t->hit = 1;
      sprintf(t->kw, "TTYPE%d  ", n);
      t->vartyp = CHARTYPE;
      strcpy(t->val.str, m->type);
      strcpy(t->c, "type comment...");
      t++;
      if (m->form) {
        t->hit = 1;
        sprintf(t->kw, "TFORM%d  ", n);
        t->vartyp = CHARTYPE;
        strcpy(t->val.str, m->form);
        strcpy(t->c, "form comment...");
      } else {
        t->hit = 0;
      }
      t++;
      if (m->unit) {
        t->hit = 1;
        sprintf(t->kw, "TUNIT%d  ", n);
        t->vartyp = CHARTYPE;
        strcpy(t->val.str, m->unit);
        strcpy(t->c, "unit comment...");
      } else {
        t->hit = 0;
      }
      t++;
      m++;
    }
    /* The form for the data channels */
    t--;
    t--;
    t->hit = 1;
    sprintf(t->kw, "TFORM%d  ", n);
    t->vartyp = CHARTYPE;
    sprintf(t->val.str, "%dE", nchan);
    strcpy(t->c, "form comment...");
    
    
    if (type == 0) {
        CharCard(KW_EXTNAME, "Binary Table");
    } else if (type == 1) {
        CharCard(KW_EXTNAME, "ODIN_Binary");
    }
}

typedef struct {
    double f;
    char tag[18];
} MolTra;

/* Odin line table                                               */
/* Line frequencies in MHz from JPL                              */
/* No more than 18 characters for mol./transition description!!! */
static MolTra Mols[] = {
    {118750.343, "O2 N(J)=1(1)-1(0)"},
    {461040.769, "CO J=4-3"},
    {487249.376, "O2 N(J)=3(3)-1(2)"},
    {488491.133, "H2O 6(2,4)-7(1,7)"},
    {489054.260, "H218O 423-330"},
    {492160.700, "CI"},
    {547676.440, "H218O 110-101"},
    {548830.978, "C18O J=5-4"},
    {550926.366, "13CO J=5-4"},
    {552020.960, "H217O 110-101"},
    {553365.000, "PH"},
    {556936.002, "H2O 1(1,0)-1(0,1)"},
    {572498.068, "NH3 100-001"},
    {576267.922, "CO J=5-4"},
};

static double GetLineFreq(double frest, char *mol)
{
    int n, nmol = sizeof(Mols)/sizeof(MolTra);
    MolTra *m;
    
    /* Accept line if frest is within +/- 250 MHz of lab. freq.*/
    for (n=0; n<nmol; n++) {
        m = &(Mols[n]);
        if (frest > m->f - 250.0 && frest < m->f + 250.0) {
            if (mol) strcpy(mol, m->tag);
            return m->f;
        }
    }
    
    return frest;
}

#define AC_XHIRES  1 
#define AC_HIRES   2 
#define AC_MEDRES  3 
#define AC_LOWRES  4 
#define AC_YHIRES  5
#define AC_SPLIT (1<<4)
#define AC_UPPER (1<<5)

#define ADC_SEQ   (1<<8)
#define ADC_SPLIT (1<<9)
#define ADC_UPPER (1<<10)

#define RFCENTER  3900.0e6   /* IF band center in Hz */
#define AOSCENTER 2100.0e6   /* IF band center for AOS in Hz */
#define AOS_FREQRES 0.62e6   /* Nominal AOS freq. resolution */

/* 
   Analyse the correlator mode by calculating a sequence of 16 integers
   whose meaning is a follows:

   n1 ssb1 n2 ssb2 n3 ssb3 ... n8 ssb8

   n1 ... n8 are the numbers of chips that are cascaded to form a band
   ssb1 ... ssb8 are +1 or -1 for USB or SSB, respectively.
   Unused ADCs are represented by zeros.

   examples (the "classical" modes):

   1 band/8 chips  0x00:   8  1  0  0  0  0  0  0  0  0  0  0  0  0  0  0
   2 band/4 chips  0x08:   4  1  0  0  0  0  0  0  4 -1  0  0  0  0  0  0
   4 band/2 chips  0x2A:   2  1  0  0  2  1  0  0  2 -1  0  0  2 -1  0  0
   8 band/1 chips  0x7F:   1  1  1 -1  1  1  1 -1  1 -1  1  1  1 -1  1  1
*/
int *GetACSequence(int mode)
{
    static int seq[16];
    /* the sequence of USB/LSB employed by the correlators */
    static int ssb[8] = { 1, -1, 1, -1, -1, 1, -1, 1 };
    int i, m;

    /* 
       To indicate the new way of storing the mode, 
       they are stored with the ADC_SEQ bit set.
    */
    if (!(mode & ADC_SEQ)) return NULL;

    mode = (mode & 0xff);
    
    m = -1;
    /* reset our sequence */
    for (i = 0; i < 16; i++) seq[i] = 0;

    /* printf("newadc: mode = %02X: ", mode); */
    /*      for (i = 0; i < 8; i++) { */
    /*  	if ((mode >> i) & 1) printf("1"); */
    /*  	else                 printf("0"); */
    /*      } */
    /*      printf("\n"); */
    
    for (i = 0; i < 8; i++) {
	if (mode & 1) m = i;   /* if bit is set, ADC is used */
	seq[2*m]++;            /* count chips                */
	mode >>= 1;            /* move on to next bit        */
    }

    for (i = 0; i < 8; i++) {
	if (seq[2*i]) {
	    if (ssb[i] < 0) seq[2*i+1] = -1;
	    else            seq[2*i+1] =  1;
	}   else            seq[2*i+1] =  0;
    }
    return seq;
}

int ACfrequency(OdinScan *s, double f[])
{
    int j, k, m, n, mode, backend, *seq, adc;
    int upper = 0, split = 0;
    double df, *LO;

    n = s->Channels;
    backend = s->Backend;
    mode = s->IntMode;
    /* for the two correlators test for split mode */
    if (backend == BACK_AC1 || backend == BACK_AC2) {
	if (mode & ADC_SEQ) {
	    if (mode & ADC_SPLIT) split = 1;
	    /* In split mode test for lower or upper half */ 
	    if (split) upper = mode & ADC_UPPER;
	} else {
	    if (mode & AC_SPLIT) split = 1;
	    /* In split mode test for lower or upper half */ 
	    if (split) upper = mode & AC_UPPER;
	}
    }

    /* Here we get if we have an unprocessed spectrum */

    /* n expected to be                   */
    /* 1728 for the AOS                   */
    /*  896 for the AC1/AC2               */
    /*  448 for the AC1/AC2 in split mode */
    /* if (n > MAXCHANNELS || n < 0) return 0; */

    /* keep lowest 4 bits only so we can use it in a switch statement below */

    switch (backend) {
      case BACK_AC1:
      case BACK_AC2:
	LO = s->FreqCal;  /* point to SSB LO frequencies */
	if (s->IntMode & ADC_SEQ) {
	    seq = GetACSequence(s->IntMode);
	    m = 0;
	    for (adc = 0; adc < 8; adc++) {
		if (s->IntMode & ADC_SPLIT) {
		    if (s->IntMode & ADC_UPPER) {
			if (adc == 0) adc += 2;
			if (adc == 4) adc += 2;
		    } else {
			if (adc == 2) adc += 2;
			if (adc == 6) break;
		    }
		}
		if (seq[2*adc]) {
		    k = seq[2*adc]*112;
		    df = 1.0e6/(double)seq[2*adc];
		    if (seq[2*adc+1] < 0) df = -df;
		    /*
      		    printf("newadc: adc[%d] LO[%d] %3d:%3d %10.3f %6.3f\n",
      			   adc, adc/2, m, m+k, LO[adc/2], df/1.0e6);
		    */
		    for (j = 0; j < k; j++) 
			f[m+j] = LO[adc/2] + (double)j*df;
		    m += k;
		}
	    }
	    /* printf("newadc: m = n ? %d = %d\n", m, n); */
	    if (m != n) return 0;
	} else {
	    mode &= 0x000f;
	    switch (mode) {
	      case AC_XHIRES:
	      case AC_YHIRES:
		df = 1.0e6/8.0;
		if (split) return 0; /* not possible in XHIRES mode       */
		for (j = 0; j < n; j++) 
		    f[j] = LO[0] + (double)j*df;
		break;
	      case AC_HIRES:
		df = 1.0e6/4.0;
		if (split) {
		    if (upper) {
			for (j = 0; j < n; j++) 
			    f[j] = LO[1] - (double)(n-1-j)*df;
		    } else {
			for (j = 0; j < n; j++) 
			    f[j] = LO[0] + (double)j*df;
		    }
		} else {
		    for (j = 0; j < n/2; j++) 
			f[j    ] = LO[0] + (double)j*df;
		    for (j = 0; j < n/2; j++) 
			f[j+n/2] = LO[1] - (double)(n/2-1-j)*df;
		}
		break;
	      case AC_MEDRES:
		df = 1.0e6/2.0;
		if (split) {
		    if (upper) {
			for (j = 0; j < n/2; j++) 
			    f[j    ] = LO[3] - (double)(n/2-1-j)*df;
			for (j = 0; j < n/2; j++) 
			    f[j+n/2] = LO[2] + (double)j*df;
		    } else {
			for (j = 0; j < n/2; j++) 
			    f[j    ] = LO[1] - (double)(n/2-1-j)*df;
			for (j = 0; j < n/2; j++) 
			    f[j+n/2] = LO[0] + (double)j*df;
		    }
		} else {
		    for (j = 0; j < n/4; j++) 
			f[j+0*n/4] = LO[1] - (double)(n/4-1-j)*df;
		    for (j = 0; j < n/4; j++) 
			f[j+1*n/4] = LO[0] + (double)j*df;
		    for (j = 0; j < n/4; j++) 
			f[j+2*n/4] = LO[3] - (double)(n/4-1-j)*df;
		    for (j = 0; j < n/4; j++) 
			f[j+3*n/4] = LO[2] + (double)j*df;
		}
		break;
	      case AC_LOWRES:
		df = 1.0e6;
		if (split) {
		    if (upper) {
			for (j = 0; j < n/4; j++) 
			    f[j+0*n/4] = LO[2] - (double)(n/4-1-j)*df;
			for (j = 0; j < n/4; j++) 
			    f[j+1*n/4] = LO[2] + (double)j*df;
			for (j = 0; j < n/4; j++) 
			    f[j+2*n/4] = LO[3] - (double)(n/4-1-j)*df;
			for (j = 0; j < n/4; j++) 
			    f[j+3*n/4] = LO[3] + (double)j*df;
		    } else {
			for (j = 0; j < n/4; j++) 
			    f[j+0*n/4] = LO[0] - (double)(n/4-1-j)*df;
			for (j = 0; j < n/4; j++) 
			    f[j+1*n/4] = LO[0] + (double)j*df;
			for (j = 0; j < n/4; j++) 
			    f[j+2*n/4] = LO[1] - (double)(n/4-1-j)*df;
			for (j = 0; j < n/4; j++) 
			    f[j+3*n/4] = LO[1] + (double)j*df;
		    }
		} else {
		    for (j = 0; j < n/8; j++) 
			f[j+0*n/8] = LO[0] - (double)(n/8-1-j)*df;
		    for (j = 0; j < n/8; j++) 
			f[j+1*n/8] = LO[0] + (double)j*df;
		    for (j = 0; j < n/8; j++) 
			f[j+2*n/8] = LO[1] - (double)(n/8-1-j)*df;
		    for (j = 0; j < n/8; j++) 
			f[j+3*n/8] = LO[1] + (double)j*df;
		    for (j = 0; j < n/8; j++) 
			f[j+4*n/8] = LO[2] - (double)(n/8-1-j)*df;
		    for (j = 0; j < n/8; j++) 
			f[j+5*n/8] = LO[2] + (double)j*df;
		    for (j = 0; j < n/8; j++) 
			f[j+6*n/8] = LO[3] - (double)(n/8-1-j)*df;
		    for (j = 0; j < n/8; j++) 
			f[j+7*n/8] = LO[3] + (double)j*df;
		}
		break;
	      default:
		return 0;
	    }
	}
	break;
      default:
	return 0;
    }

    return n;
}

/* The following structure and function are used by 'freqsort' below */
struct xs_xaxis {
    int index;
    int weight;
    double freq;
};

typedef struct xs_xaxis XAxis;

int freqcmp(XAxis *one, XAxis *two)
{
    if (one->freq < two->freq) return -1;
    if (one->freq > two->freq) return  1;
    else                       return  0;
}

int ACdrop(OdinScan *s, SCAN *S)
{
    int m, n, i, j, jm;
    int backend, mode, nbands=1, swapped, dead[8];
    double fmin, fmax, df=0.0;
    double *f;
    float *dp, dswap;
    double *fp, fswap;

    double  *AllocDoubleVector(int);
    void     FreeDoubleVector(double *);
    
    n = s->Channels;
    
    f = AllocDoubleVector(n);
    if (!f) {
        if (f) FreeDoubleVector(f);
        return 0;
    }
    
    /* call the frequency routine first */
    ACfrequency(s, f);

    backend = s->Backend;
  
    /* check if this is already done */
    if (s->Quality & ISORTED) return n;

    mode = s->IntMode;
    if (s->IntMode & ADC_SEQ) {
	/* new mode descriptor */
	mode &= 0x00ff;
	switch (mode) {
	  case 0x01:
	    df = 1.0e6/8.0;
	    nbands = 1;
	    break;
	  case 0x11:
	    df = 1.0e6/4.0;
	    nbands = 2;
	    break;
	  case 0x55:
	    df = 1.0e6/2.0;
	    nbands = 4;
	    break;
	  case 0xff:
	    df = 1.0e6;
	    nbands = 8;
	    break;
	  default:
	    /* here we get for modes which use different number of
	       chips per band */
	    /* ODINwarning("not yet implemented"); */
	    return 0;
	}
	if (s->IntMode & ADC_SPLIT) nbands /= 2;
    } else {
	/* old mode descriptor */
	mode &= 0x000f;

	switch (mode) {
	  case AC_XHIRES:
	  case AC_YHIRES:
	    df = 1.0e6/8.0;
	    nbands = 1;
	    break;
	  case AC_HIRES:
	    df = 1.0e6/4.0;
	    nbands = 2;
	    break;
	  case AC_MEDRES:
	    df = 1.0e6/2.0;
	    nbands = 4;
	    break;
	  case AC_LOWRES:
	    df = 1.0e6;
	    nbands = 8;
	    break;
	}
	if (s->IntMode & AC_SPLIT) nbands /= 2;
    }

    m = n/nbands;

    /* turn inverted bands around */
    /* this may be needed for new mode descriptions */
    for (j = 0; j < nbands; j++) {
	if (f[j*m] > f[(j+1)*m-1]) {
	    /* ODINwarning("turning band %d (%f,%f) around\n", 
			j+1,f[j*m], f[(j+1)*m-1]); */
	    dp = &S->c[j*m];
	    fp = &f[j*m];
	    for (i = 0; i < m/2; i++) {
		dswap = dp[i];
		dp[i] = dp[m-1-i];
		dp[m-1-i] = dswap;
		fswap = fp[i];
		fp[i] = fp[m-1-i];
		fp[m-1-i] = fswap;
	    }
	}
    }

    if (nbands > 1) {
	/* bring bands into frequency order */
	do {
	    swapped = 0;
	    for (j = 0; j < nbands-1; j++) {
		if (f[j*m] > f[(j+1)*m]) {
		    dp = &S->c[j*m];
		    fp = &f[j*m];
		    for (i = 0; i < m; i++) {
			dswap = dp[i];
			dp[i] = dp[i+m];
			dp[i+m] = dswap;
			fswap = fp[i];
			fp[i] = fp[i+m];
			fp[i+m] = fswap;
		    }
		    swapped = 1;
		}
	    }
	} while (swapped);

	/* check for dead bands */
	for (j = 0; j < nbands; j++) {
	    dead[j] = 1;
	    for (i = 0; i < m; i++) {
		if (S->c[j*m+i] != 0.0) {
		    dead[j] = 0;
		    break;
		}
	    }
	}

	/*      for (j = 0; j < nbands; j++) { */
	/*        if (dead[j]) { */
	/*  	printf("band %d is dead\n", j); */
    	/* if there are any more bands after the dead one, move them. */
	/*  	if (j < nbands-1) { */
	/*  	  for (i = 0; i < (nbands-1-j)*m; i++) { */
	/*  	    k = j*m+i; */
	/*  	    s->data[k] = s->data[k+m]; */
	/*  	    f[k] = f[k+m]; */
	/*  	  } */
	/*  	} */
  	/* adjust new number of channels and bands */
	/*  	n -= m; */
	/*  	nbands--; */
	/*  	j--; */
	/*  	printf("number of bands now %d (%d)\n", nbands, n); */
	/*        } */
	/*      } */

	for (j = 1; j < nbands; j++) {
	    if (dead[j-1] ^ dead[j]) {
		if (dead[j-1]) {
		    jm = (j-1)*m;
		    for (i = 0; i < m; i++) f[jm+i] = 0.0;
		} else if (dead[j]) {
		    jm = j*m;
		    for (i = 0; i < m; i++) f[jm+i] = 0.0;
		}
	    } else {
		jm = j*m;
		/* fmin is beginning of band j, fmax is end of band j-1 */
		fmin = f[jm];
		fmax = f[jm-1];
		/* if (fmin-fmax > df) { */
		/*     ODINwarning("non-contiguous bands"); */
		/* } */
		i = 0;
		while (fmax >= fmin) {
		    f[jm+i] = 0.0;
		    fmin = f[jm+i+1];
		    if (fmax >= fmin) {
			f[jm-i-1] = 0.0;
			fmax = f[jm-i-2];
			i++;
		    }
		}
	    }
	}
    
	j = 0;
	while (j < n) {
	    if (f[j] == 0.0) {
		/* remove channels with zero frequencies */
		m = 1;
		while (m+j < n && f[j+m] == 0.0) m++;
		/* ODINwarning("remove %d zero channels", m); */
		for (i = j+m; i < n; i++) {
		    f[i-m] = f[i];
		    S->c[i-m] = S->c[i];
		}
		n -= m;
	    } 
	    j++;
	} 

	j = 1;
	while (j < n) {
	    if (f[j]-f[j-1] > df) {
		/* fill in gaps with zero data channels */
		m = 1;
		while (f[j]-f[j-1] > df*(m+1)) m++;
		/* ODINwarning("gap of %d channels", m); */
		/* if (n+m > MAXCHANNELS) {
		    ODINwarning("maximum number of channels exceeded");
		    return 0;
		} */
		for (i = n-1; i >= j; i--) {
		    f[i+m] = f[i];
		    S->c[i+m] = S->c[i];
		}
		n += m;
		for (i = 0; i < m; i++) {
		    f[j] = f[j-1]+df;
		    S->c[j] = 0.0;
		    j++;
		}
	    } else {
		j++;
	    }
	}
    }

    s->Channels = n;
    s->FreqCal[0] = f[0] + df*((n-1)/2);
    s->FreqCal[1] = df;
    s->FreqCal[2] = 0.0;
    s->FreqCal[3] = 0.0;
    if (s->SkyFreq > s->LOFreq) {
	s->SkyFreq = s->LOFreq+f[n/2];
	s->MaxSuppression  = s->SkyFreq - 2.0*RFCENTER;
    } else {
	s->SkyFreq = s->LOFreq-f[n/2];
	s->MaxSuppression  = s->SkyFreq + 2.0*RFCENTER;
    }
    s->RestFreq = s->SkyFreq/(1.0-(s->VSource+s->Vlsr)/SPEEDOFLIGHT/1.0e3);
    s->FreqRes = df;
    if (s->SkyFreq < s->LOFreq) s->FreqRes = -df;

    s->Quality |= ISORTED;
    s->Quality |= ILINEAR;

    return n;
}

/*
 * this routine will turn a spectrum into one contiguous band,
 * dropping overlapping frequencies and possibly filling in dummy values
 * at intermediate frequencies which weren't covered.
 *
 * usage:
 *
 * double f[MAXCHANNELS];
 * struct OdinScan odinscan;
 *
 * if (freqsort(&odinscan, f)) {
 *     ...
 * }
 *
 * 'freqsort' will call routine 'frequency' first.
 * The function will return the (probably new) number of channels 
 * or 0 in case of an error.
 */
int ACfreqsort(OdinScan *s, SCAN *S)
{
    int adc, m, n, i, j, k, *seq;
    int backend, mode, split = 0, upper = 0;
    double ip, frac;
    double fmin, fmax, df;
    double *f, *data;
    XAxis *xaxis;

    double  *AllocDoubleVector(int);
    void     FreeDoubleVector(double *);
    
    n = s->Channels;
    
    f = AllocDoubleVector(n);
    data = AllocDoubleVector(n);
    xaxis = (XAxis *)malloc(n * sizeof(XAxis));
    if (!f || !data || !xaxis) {
        if (f) FreeDoubleVector(f);
	if (data) FreeDoubleVector(data);
	if (xaxis) free(xaxis);
        return 0;
    }
    
    /* Call the frequency routine first. */
    ACfrequency(s, f);

    if (s->Quality & ISORTED) return n;

    df = 0.0;
    backend = s->Backend;

    if (backend != BACK_AC1 && backend != BACK_AC2) return 0;

    mode = s->IntMode;
    if (mode & ADC_SEQ) {
	/* for the two correlators test for split mode */
	if (mode & ADC_SPLIT) split = 1;
	/* In split mode test for lower or upper half */ 
	if (split) upper = mode & ADC_UPPER;
	/* keep lowest 4 bits only for the switch statement below */
	mode &= 0x000f;
    } else {
	/* for the two correlators test for split mode */
	if (mode & AC_SPLIT) split = 1;
	/* In split mode test for lower or upper half */ 
	if (split) upper = mode & AC_UPPER;
	/* keep lowest 4 bits only for the switch statement below */
	mode &= 0x000f;
    }

    for (i = 0; i < n; i++) {
	xaxis[i].index = i;
	xaxis[i].freq = f[i];
    }

    /* 
       We set up a weight for each channel which will allow us to decide
       which channels to keep from two bands which overlap.
       The weights as a function of channel number look like this: /\/\/\/\
       with one ramp (/ or \) per band.
       The weights have their highest values close to the SSB local oscillators.
    */
    if (s->IntMode & ADC_SEQ) {
	if (s->IntMode & ADC_SPLIT) {
	    /* ODINwarning("not yet implemented"); */
	    return n;
	}
	seq = GetACSequence(s->IntMode);
	m = 0;
	for (adc = 0; adc < 8; adc++) {
	    if (seq[2*adc]) {
		k = seq[2*adc]*112;
		df = 1.0e6/(double)seq[2*adc];
		for (j = 0; j < k; j++) xaxis[m+j].weight = k-j;
		m += k;
	    }
	}
	if (m != n) {
	    /* ODINwarning("number of channel mismatch"); */
	    return 0;
	}
    } else {
	switch (mode) {
	  case AC_XHIRES:
	  case AC_YHIRES:
	    df = 1.0e6/8.0;
	    for (i = 0; i < n; i++) {
		xaxis[i].weight = n-i;
	    }
	    break;
	  case AC_HIRES:
	    df = 1.0e6/4.0;
	    if (split) {
		for (i = 0; i < n; i++) {
		    xaxis[i].weight     = n-i;
		}
	    } else {
		for (i = 0; i < n/2; i++) {
		    xaxis[i].weight     = n/2-i;
		    xaxis[n-1-i].weight = n/2-i;
		}
	    }
	    break;
	  case AC_MEDRES:
	    df = 1.0e6/2.0;
	    if (split) {
		for (i = 0; i < n/2; i++) {
		    xaxis[i].weight     = i;
		    xaxis[n-1-i].weight = i;
		}
	    } else {
		for (i = 0; i < n/4; i++) {
		    xaxis[1*n/4-1-i].weight = n/4-i;
		    xaxis[1*n/4+i].weight   = n/4-i;
		    xaxis[3*n/4-1-i].weight = n/4-i;
		    xaxis[3*n/4+i].weight   = n/4-i;
		}
	    }
	    break;
	  case AC_LOWRES:
	    df = 1.0e6;
	    if (split) {
		for (i = 0; i < n/4; i++) {
		    xaxis[1*n/4-1-i].weight = n/4-i;
		    xaxis[1*n/4+i].weight   = n/4-i;
		    xaxis[3*n/4-1-i].weight = n/4-i;
		    xaxis[3*n/4+i].weight   = n/4-i;
		}
	    } else {
		for (i = 0; i < n/8; i++) {
		    xaxis[1*n/8-1-i].weight = n/8-i;
		    xaxis[1*n/8+i].weight   = n/8-i;
		    xaxis[3*n/8-1-i].weight = n/8-i;
		    xaxis[3*n/8+i].weight   = n/8-i;
		    xaxis[5*n/8-1-i].weight = n/8-i;
		    xaxis[5*n/8+i].weight   = n/8-i;
		    xaxis[7*n/8-1-i].weight = n/8-i;
		    xaxis[7*n/8+i].weight   = n/8-i;
		}
	    }
	    break;
	}
    }
    qsort(xaxis, n, sizeof(XAxis), (void *)freqcmp);

    /* frequencies are now sorted in structure xaxis, member freq */
    fmin = xaxis[0].freq;
    fmax = xaxis[n-1].freq;

    /* the frequency range needs to be divisible by the resolution */
    if (df == 0.0) return 0;
    frac = modf((double)((fmax - fmin)/df), &ip);
    if (frac != 0.0) return 0;

    m = (int)ip+1;
    if (m > n) {
	/* ODINwarning("maximum number of channels exceeded"); */
	return 0;
    }

    j = 0;
    for (i = 0; i < m; i++) {
	/* generate and look up next frequency */
	f[i] = fmin + df*i;
	/* do we already have the right index j? */
	if (f[i] != xaxis[j].freq) {
	    /* search for the frequency in the array */
	    for (j = 0; j < n; j++) {
		if (xaxis[j].freq == f[i]) break;
	    }
	}
	/* if frequency not found, set channel to zero */
	if (j == n) data[i] = 0.0;
	else {
	    /* the index has the data channel */
	    k = xaxis[j].index;
	    /* do we have a frequency covered by two bands? */
	    if (f[i] == xaxis[j+1].freq) {
		/* take the channel with the higher weight */
		if (xaxis[j+1].weight > xaxis[j].weight) {
		    k = xaxis[j+1].index;
		}
	    }
	    data[i] = S->c[k];
	    j++;
	}
    }

    for (i = 0; i < m; i++) S->c[i] = data[i];

    s->Channels = m;
    s->FreqCal[0] = fmin + df*((m-1)/2);
    s->FreqCal[1] = df;
    s->FreqCal[2] = 0.0;
    s->FreqCal[3] = 0.0;
    
    if (s->SkyFreq > s->LOFreq) {
	s->SkyFreq = s->LOFreq+f[m/2];
	s->MaxSuppression  = s->SkyFreq - 2.0*RFCENTER;
    } else {
	s->SkyFreq = s->LOFreq-f[m/2];
	s->MaxSuppression  = s->SkyFreq + 2.0*RFCENTER;
    }
    s->RestFreq = s->SkyFreq/(1.0-(s->VSource+s->Vlsr)/SPEEDOFLIGHT/1.0e3);
    s->FreqRes = df;
    if (s->SkyFreq < s->LOFreq) s->FreqRes = -df;

    s->FreqRes = df;
    if (s->SkyFreq < s->LOFreq) s->FreqRes = -df;

    s->Quality |= ISORTED;
    s->Quality |= ILINEAR;
    
    FreeDoubleVector(f);
    FreeDoubleVector(data);
    free(xaxis);
    
    return m;
}

static double Odin_IF(int i, OdinScan *s)
{
    int n = s->Channels;
    double *c = s->FreqCal, x, f;
    
    x = (double)(i-n/2);
    f = c[0] + (c[1] + (c[2] + c[3]*x)*x)*x;
    
    if (s->Quality & ISORTED) return f;
    
    f = RFCENTER + (AOSCENTER - f);

    return f;
}

static void AOSredres(OdinScan *s, SCAN *S, double df)
{
    int i, k, n, cc;
    double p, q, sig, a, b, h;
    double *f, *data, *u;

    double  *AllocDoubleVector(int);
    void     FreeDoubleVector(double *);

    n = s->Channels;
    cc = (n-1)/2;
    
    data = AllocDoubleVector(n);
    f = AllocDoubleVector(n);
    u = AllocDoubleVector(n);
    /* tmp = AllocDoubleVector(n); */
    if (!data || !f || !u) {
        if (data) FreeDoubleVector(data);
        if (f) FreeDoubleVector(f);
        if (u) FreeDoubleVector(u);
	return;
    }
    
    data[0] = u[0] = 0.0;
    
    if (!(s->Quality & ISORTED)) {
       s->FreqCal[0] = RFCENTER + AOSCENTER - s->FreqCal[0];
       s->FreqCal[2] = -s->FreqCal[2];
       i = 0; k = s->Channels - 1;
       while (i < k) {
           p = OnScan.c[k];
           OnScan.c[k] = OnScan.c[i];
           OnScan.c[i] = p;
           i++; k--;
       }
       s->Quality |= ISORTED;
    }
    
    for (i = 0; i < n; i++) f[i] = Odin_IF(i, s);
    /* for (i = 0; i < n; i++) tmp[i] = S->c[i]; */
    
    for (i = 1; i <= n-2; i++) {
	sig = (f[i]-f[i-1])/(f[i+1]-f[i-1]);
	p = sig*data[i-1]+2.0;
	data[i] = (sig-1.0)/p;
	u[i] = (S->c[i+1]-S->c[i])/(f[i+1]-f[i]) 
	    - (S->c[i]-S->c[i-1])/(f[i]-f[i-1]);
	u[i] = (6.0*u[i]/(f[i+1]-f[i-1])-sig*u[i-1])/p;
    }

    data[n-1] = 0.0;

    for (i = n-2; i >= 0; i--) data[i] = data[i]*data[i+1]+u[i];

    for (i = 0; i < n; i++) u[i] = S->c[i];
  
    k = 0;
    for (i = 0; i < n; i++) {
	p = f[cc] + (i - cc)*df;
	if ((p < f[0]) || (p > f[n-1])) {
	    S->c[i] = 0.0;
	    continue;
	}
	q = f[k];
	while (p > f[k+1]) k++;
    
	h = f[k+1] - f[k];
	a = (f[k+1] - p)/h;
	b = (p - f[k])/h;
	S->c[i] = a*u[k]+b*u[k+1]
	    + ((a*a*a-a)*data[k] + (b*b*b-b)*data[k+1])*(h*h)/6.0;
    }
    s->FreqRes = s->FreqCal[1] = df;
    if (s->SkyFreq < s->LOFreq) s->FreqRes = -df;

    s->FreqCal[2] = s->FreqCal[3] = 0.0;
    s->Quality |= ILINEAR;
    
    /* {
    int i_old, i_new;
    double old_max, new_max;
    
    old_max = tmp[0]; new_max = S->c[0];
    i_old = i_new = 0;
    for (i=1; i<n; i++) {
        if (S->c[i] > new_max) {
	    new_max = S->c[i]; i_new = i;
	}
        if (tmp[i] > old_max) {
	    old_max = tmp[i]; i_old = i;
	}
    }
    
    printf("Old: %4d %f   New: %4d %f\n", i_old, old_max, i_new, new_max);
    printf("%.2f %.2f  %.2f     [%.2f,%.2f]\n",
    (f[1]-f[0])*1.0e-3, (f[n/2]-f[n/2-1])*1.0e-3, (f[n-1]-f[n-2])*1.0e-3,
    f[0]*1.0e-6, f[n-1]*1.0e-6);
    } */
    
    FreeDoubleVector(data);
    FreeDoubleVector(f);
    FreeDoubleVector(u);
    /* FreeDoubleVector(tmp); */
}

static int ConvertOdinScan()
{
    int MJD, n, m=0, spe_type=0;
    /* int nmax=0;
    double Fre, Vel, Tmax=0.0; */
    string buf, back_end;
    double t, fracUT, fline, vdiff;
    
    void ParseMJD(int, short int *, short int *, short int *);
    void send_line();
    int filter_active(), allow_scan();
    
    OnScan.NChannel = (short int)Odin.Channels;
    XScan.NChannel = Odin.Channels;
    
    OnScan.ScanNo = (short int)Odin.Orbit;
    OnScan.SubScan = (short int)(10000.0*(Odin.Orbit - (double)OnScan.ScanNo));
    
    strncpy(OnScan.Name, Odin.Source, 12);
    OnScan.Name[11]= '\0';
    strcpy(OnScan.Project, "Odi");
    strcpy(OnScan.Molecule, "<unknown>");
    
    OnScan.Equinox = 2000.0;
    OnScan.Longitude = Odin.RA2000/RADTODEG;
    OnScan.Latitude  = Odin.Dec2000/RADTODEG;
    OnScan.LMapOff   = Odin.MapXoff/RADTODEG;
    OnScan.BMapOff   = Odin.MapYoff/RADTODEG;
    
    /* subtract offsets from actual coordinates */
    OnScan.Latitude  -= OnScan.BMapOff;
    OnScan.Longitude -= OnScan.LMapOff/cos(OnScan.Latitude);
    
    OnScan.PosAngle  = 0.0;
    
    OnScan.Tau       = sqrt(Odin.Qerror[0]*Odin.Qerror[0] +
                            Odin.Qerror[1]*Odin.Qerror[1] +
                            Odin.Qerror[2]*Odin.Qerror[2]);

    OnScan.Tsys      = Odin.Tsys;
    OnScan.Tcal      = Odin.Tcal;
    
    OnScan.JulDate   = (long int)(Odin.MJD + 2400000.5);
    
    MJD    = (int)Odin.MJD;
    fracUT = Odin.MJD - MJD;
    
    ParseMJD(MJD, &(OnScan.Year), &(OnScan.Month), &(OnScan.Day));
    
    OnScan.UTHour    = (short int)(24.0 * fracUT);
    OnScan.UTMin     = (short int)(60.0*(24.0*fracUT - OnScan.UTHour));
    OnScan.UTSec     = (short int)(60.0*(24.0*60.0*fracUT - 60.0*OnScan.UTHour
                                         - OnScan.UTMin));
    
    OnScan.STHour    = (short int)(Odin.LST/3600.0);
    OnScan.STMin     = (short int)((Odin.LST - 3600.0*OnScan.STHour)/60.0);
    OnScan.STSec     = (short int)(Odin.LST - 3600.0*OnScan.STHour -
                                    60.0*OnScan.STMin);
                                    
    OnScan.IntTime  = Odin.IntTime;
    OnScan.Backend  = Odin.Backend;
    OnScan.Frontend = Odin.Frontend;
    
    if (Odin.Backend == BACK_AC1) {
        strcpy(back_end, "AC1");
    } else if (Odin.Backend == BACK_AC2) {
        strcpy(back_end, "AC2");
    } else if (Odin.Backend == BACK_AOS) {
        strcpy(back_end, "AOS");
    } else {
        strcpy(back_end, "<unknown>");
    }
    
    if (Odin.Type == TYPE_CAL) {
        sprintf(buf,
	  "Warning! Odin %3s scan %4d.%04d (f=%.2f) is a cal. scan. Skipping!",
	        back_end, OnScan.ScanNo, OnScan.SubScan, Odin.RestFreq*1.0e-6);
	send_line(buf);
        spe_type = 1;
	return spe_type;
    }
    
    fline = GetLineFreq(Odin.RestFreq*1.0e-6, OnScan.Molecule);

    if (Odin.Backend == BACK_AOS && !(Odin.Quality & ILINEAR)) {
        sprintf(buf,
	  "Odin %3s scan %4d.%04d (f=%.2f) is not linearized (%.1f->%.1f kHz).",
	        back_end, OnScan.ScanNo, OnScan.SubScan, OnScan.RestFreq,
		Odin.FreqRes*1.0e-3, AOS_FREQRES*1.0e-3);
	send_line(buf);
	AOSredres(&Odin, &OnScan, AOS_FREQRES);
    }
    
    if (!(Odin.Quality & ISORTED)) {
	if (Odin.Backend == BACK_AOS) {
	    n = 0; m = XScan.NChannel - 1;
	    while (n < m) {
	        t =  OnScan.c[m];
	        OnScan.c[m] = OnScan.c[n];
	        OnScan.c[n] = t;
	        n++; m--;
	    }
            sprintf(buf,
    "Odin %3s scan %4d.%04d (f=%.2f) is not freq. sorted. Sorting.",
	        back_end, OnScan.ScanNo, OnScan.SubScan, Odin.RestFreq*1.0e-6);
	} else {
            /* sprintf(buf,
    "Odin %3s scan %4d.%04d (f=%.2f) is not freq. sorted. Sorting (%d->%d).",
	        back_end, OnScan.ScanNo, OnScan.SubScan, Odin.RestFreq*1.0e-6,
		(int)Odin.Channels, m);
	    printf("%s\n", buf); */
	    m = ACdrop(&Odin, &OnScan);
	    /* Odin.RestFreq += 13.2e6; */
            sprintf(buf,
    "Odin %3s scan %4d.%04d (f=%.2f) is not freq. sorted. Sorting (%d->%d).",
	        back_end, OnScan.ScanNo, OnScan.SubScan, Odin.RestFreq*1.0e-6,
		(int)Odin.Channels, m);
	    /* printf("%s\n", buf); */
	}
	send_line(buf);
    }
    
    OnScan.RestFreq  = Odin.RestFreq  * 1.0e-6;
    OnScan.SkyFreq   = Odin.SkyFreq   * 1.0e-6;
    OnScan.FreqThrow = Odin.FreqThrow * 1.0e-6;

    XScan.LOFreq     = Odin.LOFreq * 1.0e-6;
    XScan.s	     = &(OnScan);

    OnScan.FirstIF   = RFCENTER * 1.0e-6;

    OnScan.FreqRes   = Odin.FreqRes * 1.0e-6;
    OnScan.VelRes    = -OnScan.FreqRes/OnScan.RestFreq * SPEEDOFLIGHT;
    OnScan.VSource   = Odin.VSource/1.0e3;

    vdiff = SPEEDOFLIGHT * (1.0 - fline/OnScan.RestFreq);
    OnScan.VLsr      = vdiff;
    
    /* printf("%12s %4d.%04d %6.2f %6.2f  %.2f  %.2f  %.2f %6.2f %.0f\n", OnScan.Name,
           OnScan.ScanNo, OnScan.SubScan,
           Odin.Vlsr/1.0e3, Odin.VSource/1.0e3, OnScan.RestFreq,
           OnScan.FreqRes, fline, vdiff, OnScan.Tsys); */
    
    /* if (OnScan.Tsys > 2000.0 && OnScan.Tsys < 5000.0) {
       for (n=0; n<XScan.NChannel ; n++) {
          t = OnScan.c[XScan.NChannel - 1 - n];
	  if (n == 0 || t > Tmax) {
	     Tmax = t;
	     nmax = n;
	  }
       }
       n = nmax - (XScan.NChannel/2);
       Fre = OnScan.RestFreq + (double)n * OnScan.FreqRes;
       Vel = OnScan.VSource - OnScan.VLsr + (double)n * OnScan.VelRes;
       printf("%4d  %.1f  %.2f  %.2f\n", nmax, Tmax, Fre, Vel);
    } */
    
    if (filter_active() && !allow_scan(&OnScan)) {
        sprintf(buf,
	  "Odin %3s scan %4d.%04d (f=%.2f) is filtered. Skipping!",
	        back_end, OnScan.ScanNo, OnScan.SubScan, Odin.RestFreq*1.0e-6);
	send_line(buf);
        spe_type = 1;
    }  
    
    return spe_type;
}

static void FillOdinStruct(scanPtr s)
{
    static int size = -1;
    double d, r;
    int n;
    
    double ModifiedJulianDay(DATE *);
    
    Odin.Channels = GET4((long)(s->nChan));
    
    Odin.Orbit = GETD((double)(s->scan_no));
    
    strcpy(Odin.Source, s->name);

    d = s->y0 + s->yoffset/RADTOSEC;
    r = s->x0 + s->xoffset/RADTOSEC * cos(d);
    d *= RADTODEG;
    r *= RADTODEG;
    
    Odin.Dec2000 = GETF((float)d);
    Odin.RA2000  = GETF((float)r);
    
    Odin.MapXoff = GETF((float)(s->xoffset/3600.0));
    Odin.MapYoff = GETF((float)(s->yoffset/3600.0));
    Odin.MapTilt = GETF((float)0.0);
    
    Odin.RestFreq = GETD((double)(
                1.0e9*(s->freq0 + (s->nChan/2) * s->freqres)));
    Odin.FreqRes  = GETD((double)(s->freqres * 1.0e9));
    Odin.SkyFreq  = GETD((double)0.0);
    Odin.LOFreq   = GETD((double)0.0);
    Odin.FreqThrow = GETD((double)0.0);
   
    Odin.VSource = GETF((float)((s->vel0 + (s->nChan/2)*s->velres)*1.0e3));
    Odin.Vlsr = GETF((float)(s->vlsr));
    
    Odin.Qerror[0] = GETD((double)s->tau);
    Odin.Qerror[1] = Odin.Qerror[2] = GETD((double)0.0);
    
    Odin.Tsys = GETF((float)(s->tsys));
    Odin.Tcal = GETF((float)-1.0);
    
    Odin.MJD = GETD((double)ModifiedJulianDay(&(s->date)));
    Odin.LST = GETF((float)(3600.0 * s->LST.Hour +
                            60.0 * s->LST.Min + s->LST.Sec));
                                    
    Odin.IntTime = GETF((float)(s->int_time));
    
    Odin.Type = GET2((short)TYPE_SPE);
    Odin.Quality = GET4((long)(ISORTED | ILINEAR));
    
    Odin.Version = Odin.Level = 0;
    Odin.STW = 0;
    Odin.Discipline = Odin.Topic = Odin.Spectrum = Odin.ObsMode = 0;
    Odin.Frontend = Odin.Backend = Odin.SkyBeamHit = 0;
    Odin.Qtarget[0] = Odin.Qtarget[1] = Odin.Qtarget[2] = Odin.Qtarget[3] = 0.0;
    Odin.Qachieved[0] = Odin.Qachieved[1] = 0.0;
    Odin.Qachieved[2] = Odin.Qachieved[3] = 0.0;
    Odin.GPSpos[0] = Odin.GPSpos[1] = Odin.GPSpos[2] = 0.0;
    Odin.GPSvel[0] = Odin.GPSvel[1] = Odin.GPSvel[2] = 0.0;
    Odin.SunPos[0] = Odin.SunPos[1] = Odin.SunPos[2] = 0.0;
    Odin.MoonPos[0] = Odin.MoonPos[1] = Odin.MoonPos[2] = 0.0;
    Odin.SunZD = 0.0;
    
    Odin.Vgeo = 0.0;
    Odin.SBpath = 0.0;
    Odin.MaxSuppression = 0.0;
    Odin.FreqCal[0] = Odin.FreqCal[1] = Odin.FreqCal[2] = Odin.FreqCal[3] = 0.0;
    Odin.IntMode = 0.0;
    Odin.EffTime = 0.0;
    
    if (size == -1) {
       Odin.data = (float *)malloc(s->nChan * sizeof(float));
       size = s->nChan;
    } else if (size != s->nChan) {
       Odin.data = (float *)realloc(Odin.data, s->nChan * sizeof(float));
       size = s->nChan;
    }
    
    if (!Odin.data) return;
    
    for (n=0; n<s->nChan; n++) {
        Odin.data[n] = GETF((float)(s->d[n]));
    }
}

int FillBinaryTable(scanPtr s, char *data, int size, int type)
{
    char *d;
    int osize = sizeof(struct odin);
    
    FillOdinStruct(s);
    
    if (!data) return 1;
    if (!Odin.data) return 2;
    
    d = data;
    memcpy(d, (char *)(&Odin), osize-4);
    d += (osize - 4);
    memcpy(d, (char *)(Odin.data), s->nChan * 4);
    
    return 0;
}

int LoadBinaryTable(int nscan, void *data, int type)
{
    string buf;
    UNION val;
    int n, nc=0, spe_type=0;
    char *p = (char *)data;
    list curr = NULL;
    field *f;
    static Mapping *m = NULL;
	
    int  CheckDataSize(int);
    
    if (nscan == 0) m = SetupMapping(type);
    
    if (!m) return 1;
    
    while ((curr = field_iterator(curr)) != NULL) {
        f = (field *)DATA(curr);
        if (type == 0 && strncmp(f->type, "Channels", 8) == 0) {
            nc = OnScan.NChannel;
	    XScan.NChannel = nc;
            if (nc <= 0 || nc > f->n) return nc;
            if (nc < f->n) f->n = nc;
            if (CheckDataSize(nc) < nc) {
		        return 3;
            }
            m[80].v = (generic_ptr)(OnScan.c);
            f->nmap = 80;
        }
        if (type == 1 && strncmp(f->type, "data", 4) == 0) {
            nc = Odin.Channels;
	    XScan.NChannel = nc;
            if (nc <= 0 || nc > f->n) return nc;
            if (nc < f->n) f->n = nc;
            if (CheckDataSize(nc) < nc) {
		        return 3;
            }
            m[46].v = (generic_ptr)(OnScan.c);
            f->nmap = 46;
        }
        if (f->format == BT_ASCII) {
            strncpy(buf, p, f->n);
            buf[f->n] = '\0';
            p += f->n;
            val.a = buf;
            AddDRPValue(f, &val, f->n, m, type);
        } else {
            for (n=0; n<f->n; n++) {
                bcopy((void *)p, &val, (size_t)f->bytes);
                p += f->bytes;
                AddDRPValue(f, &val, n, m, type);
            }
        }
    }
    
    if (type == 1) {
        spe_type = ConvertOdinScan();
    }
    
    return spe_type;
}
