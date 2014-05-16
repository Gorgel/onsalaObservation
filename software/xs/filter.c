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
#ifdef POSIX
#define _INCLUDE_POSIX2_SOURCE
#endif
#include <regex.h>
#include <math.h>

#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Separator.h>

#include "defines.h"
#include "global_structs.h"
#include "drp.h"

#define FE_TYPE_DOUBLE 0
#define FE_TYPE_STRING 1
#define FE_TYPE_STRINT 2
#define FE_TYPE_STRDBL 3
#define FE_TYPE_INT    4

extern VIEW   *vP;

typedef struct {
    Widget toggleButton;
    int    status;
    string description;
    Widget minEdit;
    Widget maxEdit;
    int type;
    double min, max;
    int imin, imax;
    string smin, smax;
} FILTERENTRY;

FILTERENTRY fe[] = {
 {NULL, 0, "Source name",           NULL, NULL, FE_TYPE_STRING, 0.0, 0.0, 0, 0, "", ""},
 {NULL, 0, "Date (YYYY/MM/DD)",     NULL, NULL, FE_TYPE_STRDBL, 0.0, 0.0, 0, 0, "1990/01/01", "2000/12/31"},
 {NULL, 0, "Time (HH:MM:SS)",       NULL, NULL, FE_TYPE_STRDBL, 0.0, 0.0, 0, 0, "00:00:00", "23:59:59"},
 {NULL, 0, "Molecule name",         NULL, NULL, FE_TYPE_STRING, 0.0, 0.0, 0, 0, "", ""},
 {NULL, 0, "Backend name",          NULL, NULL, FE_TYPE_STRING, 0.0, 0.0, 0, 0, "", ""},
 {NULL, 0, "Type of scan",          NULL, NULL, FE_TYPE_INT,    0.0, 0.0, 0, 0, "", ""},
 {NULL, 0, "Polarization (1,2,3)",  NULL, NULL, FE_TYPE_INT,    0.0, 0.0, 0, 0, "", ""},
 {NULL, 0, "Frequency range (MHz)", NULL, NULL, FE_TYPE_DOUBLE, 0.0, 0.0, 0, 0, "", ""},
 {NULL, 0, "Integration time (s)",  NULL, NULL, FE_TYPE_DOUBLE, 0.0, 0.0, 0, 0, "", ""},
 {NULL, 0, "System temp. (K)",      NULL, NULL, FE_TYPE_DOUBLE, 0.0, 0.0, 0, 0, "", ""},
 {NULL, 0, "No of channels",        NULL, NULL, FE_TYPE_INT,    0.0, 0.0, 1, 4096, "", ""},
 {NULL, 0, "Scan number",           NULL, NULL, FE_TYPE_INT,    0.0, 0.0, 1, 9999, "", ""},
 {NULL, 0, "x offset range (\")",   NULL, NULL, FE_TYPE_DOUBLE, 0.0, 0.0, 0, 0, "", ""},
 {NULL, 0, "y offset range (\")",   NULL, NULL, FE_TYPE_DOUBLE, 0.0, 0.0, 0, 0, "", ""}
};

static char *filter_help = "\
                        Filter dialog help\n\
                        ------------------\n\
The two filter fields are used to specify minimum and maximum allowed values,\n\
respectively, for each item. The items for `Source', `Molecule', and\n\
`Backend name' are special in the sense that strings should be supplied.\n\
The filtering for strings works as follows: If only the first field is used\n\
only those scans that match the entered string will be selected. The matching\n\
is determined via standard (?) regular expression syntax (see `man regexp' or\n\
`man regex' on most UNIX systems). If both fields are used the resulting\n\
match is determined by a logical AND.\n\
\n\
Note that the toggle button to the left in the item row must be selected for\n\
the item to be active during the filtering.\n\
\n\
The item `Backend name' will probably only work for data taken at OSO or SEST.\n\
The polarization code works only for OSO 20 m data. 1=LCP, 2=RCP, 3=BOTH";

static void set_fits_filter(Widget w, Widget parent, XmAnyCallbackStruct *cb)
{
    int n, nent = sizeof(fe)/sizeof(FILTERENTRY);
    Arg arg;
    Boolean set;
    
    void wdscanf(), wsscanf(), wiscanf();
    
    for (n=0; n<nent; n++) {
         XtSetArg(arg, XmNset, &set);
         XtGetValues(fe[n].toggleButton, &arg, 1);
         if (!set) {
             fe[n].status = 0;
             continue;
         }
         fe[n].status = 1;
         if (fe[n].type == FE_TYPE_DOUBLE) {
             wdscanf(fe[n].minEdit, &fe[n].min);
             wdscanf(fe[n].maxEdit, &fe[n].max);
         } else if (fe[n].type == FE_TYPE_INT) {
             wiscanf(fe[n].minEdit, &fe[n].imin);
             wiscanf(fe[n].maxEdit, &fe[n].imax);
         } else {
             wsscanf(fe[n].minEdit, fe[n].smin);
             wsscanf(fe[n].maxEdit, fe[n].smax);
         }
    }
}

void PostFilterDialog(Widget w, char *cmd, XtPointer *cd)
{
    int n, nent = sizeof(fe)/sizeof(FILTERENTRY);
    Widget form, rc, sep, apply, cancel, help;
    Widget parent = w;
    Arg arg, wargs[10];
    
    void wprintf();
    void cancel_dialog(), help_dialog();
    void StdApplyCancel();
    void ManageDialogCenteredOnPointer(Widget);
    
    while (!XtIsWMShell(parent))
        parent = XtParent(parent);
    
    n = 0;
    XtSetArg(wargs[n], XmNtitle, "Filter scans"); n++;
    XtSetArg(wargs[n], XmNautoUnmanage, False); n++;
    form  = XmCreateFormDialog(parent, "form", wargs, n);

    n = 0;
    XtSetArg(wargs[n], XmNorientation,       XmHORIZONTAL); n++;
    XtSetArg(wargs[n], XmNnumColumns,        nent); n++;
    XtSetArg(wargs[n], XmNadjustLast,        FALSE); n++;
    XtSetArg(wargs[n], XmNpacking,           XmPACK_COLUMN); n++;
    rc    = XtCreateManagedWidget("rc", xmRowColumnWidgetClass,
                                  form, wargs, n);

    for (n=0; n<nent; n++) {
        fe[n].toggleButton = XtCreateManagedWidget(fe[n].description,
                                                   xmToggleButtonWidgetClass,
                                                   rc, NULL, 0);
        XtAddCallback(fe[n].toggleButton, XmNvalueChangedCallback,
                      (XtCallbackProc)set_fits_filter, form);
        fe[n].minEdit = XtCreateManagedWidget("minedit",
                                                   xmTextWidgetClass,
                                                   rc, NULL, 0);
        fe[n].maxEdit = XtCreateManagedWidget("maxedit",
                                                   xmTextWidgetClass,
                                                   rc, NULL, 0);
    }

    sep = XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
                                  form, XmNseparatorType,
                                  XmSHADOW_ETCHED_IN, NULL);
    apply  = XtCreateManagedWidget(BUTT_APPLY, xmPushButtonWidgetClass,
                                   form, NULL, 0);
    cancel = XtCreateManagedWidget(BUTT_CANCEL, xmPushButtonWidgetClass,
                                   form, NULL, 0);
    help   = XtCreateManagedWidget(BUTT_HELP, xmPushButtonWidgetClass,
                                   form, NULL, 0);
    XtAddCallback(apply, XmNactivateCallback,
                  (XtCallbackProc)set_fits_filter, form);
    XtAddCallback(cancel, XmNactivateCallback,
                  (XtCallbackProc)cancel_dialog, form);
    XtAddCallback(help, XmNactivateCallback,
                  (XtCallbackProc)help_dialog, filter_help);

    XtVaSetValues(form, XmNdefaultButton, apply, NULL);

    StdApplyCancel(NULL, rc, sep, apply, cancel, help);

    for (n=0; n<nent; n++) {
        XtSetArg(arg, XmNset, fe[n].status);
        XtSetValues(fe[n].toggleButton, &arg, 1);
        if (fe[n].type == FE_TYPE_DOUBLE) {
            wprintf(fe[n].minEdit, "%f", fe[n].min);
            wprintf(fe[n].maxEdit, "%f", fe[n].max);
        } else if (fe[n].type == FE_TYPE_INT) {
            wprintf(fe[n].minEdit, "%d", fe[n].imin);
            wprintf(fe[n].maxEdit, "%d", fe[n].imax);
        } else {
            wprintf(fe[n].minEdit, "%s", fe[n].smin);
            wprintf(fe[n].maxEdit, "%s", fe[n].smax);
        }
    }
    
    ManageDialogCenteredOnPointer(form);
}

int filter_active()
{
    int n, nent = sizeof(fe)/sizeof(FILTERENTRY);
    
    for (n=0; n<nent; n++) {
        if (fe[n].status) return 1;
    }
    
    return 0;
}

int match(char *str, char *pat)
{
    int i;
    regex_t re;
    string buf;
    
    void send_line();
    
    i = regcomp(&re, pat, REG_EXTENDED | REG_NOSUB);
    if (i != 0) {
        (void)regerror(i, &re, buf, sizeof buf);
        regfree(&re);
        send_line(buf);
        return 0;
    }
    i = regexec(&re, str, (size_t)0, NULL, 0);
    regfree(&re);
    
    if (i != 0) return 0;
    
    return 1;
}

static double GetDateDbl(char *str)
{
    int y, m, d;
    double v;
    
    sscanf(str, "%d/%d/%d", &y, &m, &d);
    
    v = (double)y + (double)m/100.0 + (double)d/10000.0;
    
    return (v);
}

static double GetTimeDbl(char *str)
{
    int h, m, s;
    double v;
    
    sscanf(str, "%d:%d:%d", &h, &m, &s);
    
    v = (double)h + (double)m/60.0 + (double)s/3600.0;
    
    return (v);
}

int allow_fd(FDATA *s)
{
    int n, i=0, nent = sizeof(fe)/sizeof(FILTERENTRY);
    int i_min=0, i_max=0;
    double v=0.0, v_min=0.0, v_max=0.0;
    string str;
    
    void wdscanf(), wsscanf(), wiscanf(), strip_trailing_spaces();
    
    for (n=0; n<nent; n++) {
        if (!fe[n].status) continue;
        switch (n) {
            case 0:
                strcpy(str, s->sname);
                strip_trailing_spaces(str);
                break;
            case 1:
                v = (double)s->date.Year + (double)s->date.Month/100.0 +
                    (double)s->date.Day/10000.0;
                v_min = GetDateDbl(fe[n].smin) - 0.1/10000.0;
                v_max = GetDateDbl(fe[n].smax) + 0.1/10000.0;
                break;
            case 2:
                v = (double)s->date.Hour + (double)s->date.Min/60.0 +
                    (double)s->date.Sec/3600.0;
                v_min = GetTimeDbl(fe[n].smin) - 0.1/3600.0;
                v_max = GetTimeDbl(fe[n].smax) + 0.1/3600.0;
                break;
            case 3:
                strcpy(str, s->molecule);
                strip_trailing_spaces(str);
                break;
            case 4:
                strcpy(str, "");
                strip_trailing_spaces(str);
                break;
            case 5: /* backend */
                i = 0;
                break;
            case 6: /* Polarization filtering */
                i = s->pol;
                break;
            case 7:
                v = (double)(s->f0 + s->fn)*1000.0/2.0;
                break;
            case 8:
                v = (double)s->int_time;
                break;
            case 9:
                v = (double)s->tsys;
                break;
            case 10:
                i = (int)s->n;
                break;
            case 11:
                i = (int)s->sno;
                break;
            case 12:
                v = RADTOSEC*(double)s->xoff;
                break;
            case 13:
                v = RADTOSEC*(double)s->yoff;
                break;
            default:
                return 0;
        }
        if (fe[n].type == FE_TYPE_DOUBLE) {
            if (v < fe[n].min || v > fe[n].max) return 0;
        } else if (fe[n].type == FE_TYPE_INT) {
            if (i < fe[n].imin || i > fe[n].imax) return 0;
        } else if (fe[n].type == FE_TYPE_STRINT) {
            if (i < i_min || i > i_max) return 0;
        } else if (fe[n].type == FE_TYPE_STRDBL) {
            if (v < v_min || v > v_max) return 0;
        } else {
            if (strlen(fe[n].smin) && !match(str, fe[n].smin)) return 0;
            if (strlen(fe[n].smax) && !match(str, fe[n].smax)) return 0;
        }
    }
    
    return 1;
}

int allow_scan(SCAN *s)
{
    int n, i=0, nent = sizeof(fe)/sizeof(FILTERENTRY);
    int i_min=0, i_max=0;
    double v=0.0, v_min=0.0, v_max=0.0;
    string str;
    
    void wdscanf(), wsscanf(), wiscanf(), strip_trailing_spaces();
    
    for (n=0; n<nent; n++) {
        if (!fe[n].status) continue;
        switch (n) {
            case 0:
                strcpy(str, s->Name);
                strip_trailing_spaces(str);
                break;
            case 1:
                v = (double)s->Year + (double)s->Month/100.0 +
                    (double)s->Day/10000.0;
                v_min = GetDateDbl(fe[n].smin) - 0.1/10000.0;
                v_max = GetDateDbl(fe[n].smax) + 0.1/10000.0;
                break;
            case 2:
                v = (double)s->UTHour + (double)s->UTMin/60.0 +
                    (double)s->UTSec/3600.0;
                v_min = GetTimeDbl(fe[n].smin) - 0.1/3600.0;
                v_max = GetTimeDbl(fe[n].smax) + 0.1/3600.0;
                break;
            case 3:
                strcpy(str, s->Molecule);
                strip_trailing_spaces(str);
                break;
            case 4:
                strcpy(str, s->Program);
                strip_trailing_spaces(str);
                break;
            case 5: /* backend */
                i = s->ObsMode;
                break;
            case 6: /* Polarization filtering */
                i = s->flag[0];
                break;
            case 7:
                v = (double)s->RestFreq;
                break;
            case 8:
                v = (double)s->IntTime;
                break;
            case 9:
                v = (double)s->Tsys;
                break;
            case 10:
                i = (int)s->NChannel;
                break;
            case 11:
                i = (int)s->ScanNo;
                break;
            case 12:
                v = RADTOSEC*(double)s->LMapOff;
                break;
            case 13:
                v = RADTOSEC*(double)s->BMapOff;
                break;
            default:
                return 0;
        }
        if (fe[n].type == FE_TYPE_DOUBLE) {
            if (v < fe[n].min || v > fe[n].max) return 0;
        } else if (fe[n].type == FE_TYPE_INT) {
            if (i < fe[n].imin || i > fe[n].imax) return 0;
        } else if (fe[n].type == FE_TYPE_STRINT) {
            if (i < i_min || i > i_max) return 0;
        } else if (fe[n].type == FE_TYPE_STRDBL) {
            if (v < v_min || v > v_max) return 0;
        } else {
            if (strlen(fe[n].smin) && !match(str, fe[n].smin)) return 0;
            if (strlen(fe[n].smax) && !match(str, fe[n].smax)) return 0;
        }
    }
    
    return 1;
}

int AllowScan(scanPtr s)
{
    int n, i=0, nent = sizeof(fe)/sizeof(FILTERENTRY);
    int i_min=0, i_max=0;
    double v=0.0, v_min=0.0, v_max=0.0;
    string str;
    
    void wdscanf(), wsscanf(), wiscanf(), strip_trailing_spaces();
    double xmap(scanPtr), ymap(scanPtr);
    
    if (!s) return 0;
    
    for (n=0; n<nent; n++) {
        if (!fe[n].status) continue;
        switch (n) {
            case 0:
                strcpy(str, s->name);
                strip_trailing_spaces(str);
                break;
            case 1:
                v = (double)s->date.Year + (double)s->date.Month/100.0 +
                    (double)s->date.Day/10000.0;
                v_min = GetDateDbl(fe[n].smin);
                v_max = GetDateDbl(fe[n].smax);
                break;
            case 2:
                v = (double)s->date.Hour + (double)s->date.Min/60.0 +
                    (double)s->date.Sec/3600.0;
                v_min = GetTimeDbl(fe[n].smin);
                v_max = GetTimeDbl(fe[n].smax);
                break;
            case 3:
                strcpy(str, s->molecule);
                strip_trailing_spaces(str);
                break;
            /* case 4:
                strcpy(str, s->backend);
                strip_trailing_spaces(str);
                break; */
	    /* case 5:
	        break; */
            case 6:
                i = s->polarization;
                break;
            case 7:
                v = (s->freq0 + (double)(s->nChan/2 + 1) * s->freqres)*1000.;
                break;
            case 8:
                v =  s->int_time;
                break;
            case 9:
                v =  s->tsys;
                break;
            case 10:
                i =  s->nChan;
                break;
            case 11:
                i = s->scan_no;
                break;
            case 12:
                v = vP->from->sequence ? s->tx : xmap(s);
                break;
            case 13:
                v = vP->from->sequence ? s->ty : ymap(s);
                break;
            default:
                return 0;
        }
        if (fe[n].type == FE_TYPE_DOUBLE) {
            if (v < fe[n].min || v > fe[n].max) return 0;
        } else if (fe[n].type == FE_TYPE_INT) {
            if (i < fe[n].imin || i > fe[n].imax) return 0;
        } else if (fe[n].type == FE_TYPE_STRINT) {
            if (i < i_min || i > i_max) return 0;
        } else if (fe[n].type == FE_TYPE_STRDBL) {
            if (v < v_min || v > v_max) return 0;
        } else {
            if (strlen(fe[n].smin) && !match(str, fe[n].smin)) return 0;
            if (strlen(fe[n].smax) && !match(str, fe[n].smax)) return 0;
        }
    }
    
    return 1;
}

static int    *isort1 = NULL;
static double *dsort1 = NULL;
static string *ssort1 = NULL;
static int    *isort2 = NULL;
static double *dsort2 = NULL;
static string *ssort2 = NULL;

int InitSort(int n, XSTR *x)
{
    if (n <= 0) return 0;
    
    if (!x) return 1;
    
    if (x->sort1_type == SORT_TYPE_NONE) return 0;
    
    if (x->sort1_type < SORT_TYPE_FREQ) {        /* integer sort value */
        isort1 = (int *)XtMalloc(n * sizeof(int));
        if (!isort1) return 1;
    } else if (x->sort1_type < SORT_TYPE_NAME) { /* double sort value */
        dsort1 = (double *)XtMalloc(n * sizeof(double));
        if (!dsort1) return 1;
    } else {                                    /* string sort value */
        ssort1 = (string *)XtMalloc(n * sizeof(string));
        if (!ssort1) return 1;
    }
    
    if (x->sort2_type == SORT_TYPE_NONE) return 0;
    
    if (x->sort2_type < SORT_TYPE_FREQ) {        /* integer sort value */
        isort2 = (int *)XtMalloc(n * sizeof(int));
        if (!isort2) return 1;
    } else if (x->sort2_type < SORT_TYPE_NAME) { /* double sort value */
        dsort2 = (double *)XtMalloc(n * sizeof(double));
        if (!dsort2) return 1;
    } else {                                    /* string sort value */
        ssort2 = (string *)XtMalloc(n * sizeof(string));
        if (!ssort2) return 1;
    }
    
    return 0;
}

static void SetSortValue(int n, int type1, int type2, SCAN *s)
{
/* Primary integer sort value */
    switch (type1) {
        case SORT_TYPE_SCAN:
            isort1[n] = (int)s->ScanNo;
            break;
        case SORT_TYPE_NCHA:
            isort1[n] = (int)s->NChannel;
            break;
/* Primary double sort value */
        case SORT_TYPE_FREQ:
            dsort1[n] = (double)s->RestFreq;
            break;
        case SORT_TYPE_VLSR:
            dsort1[n] = (double)s->VSource;
            break;
        case SORT_TYPE_TIME:
            dsort1[n] = (double)s->IntTime;
            break;
        case SORT_TYPE_DATE:
            dsort1[n] = (double)s->Year + (double)s->Month/1.0e2 +
                        (double)s->Day/1.0e4 + (
                        (double)s->UTHour + (double)s->UTMin/60.0 +
                        (double)s->UTSec/3600.0)/1.0e6;
            break;
        case SORT_TYPE_RA:
            dsort1[n] = (double)s->Longitude;
            break;
        case SORT_TYPE_DEC:
            dsort1[n] = (double)s->Latitude;
            break;
        case SORT_TYPE_RAOFF:
            dsort1[n] = (double)s->LMapOff;
            break;
        case SORT_TYPE_DEOFF:
            dsort1[n] = (double)s->BMapOff;
            break;
        case SORT_TYPE_DIST:
            dsort1[n] = (double)(s->LMapOff * s->LMapOff +
                                 s->BMapOff * s->BMapOff);
            break;
/* Primary string sort value */
        case SORT_TYPE_NAME:
            strcpy(ssort1[n], s->Name);
            break;
        case SORT_TYPE_MOLE:
            strcpy(ssort1[n], s->Molecule);
            break;
    }
/* Secondary integer sort value */
    switch (type2) {
        case SORT_TYPE_SCAN:
            isort2[n] = (int)s->ScanNo;
            break;
        case SORT_TYPE_NCHA:
            isort2[n] = (int)s->NChannel;
            break;
/* Secondary double sort value */
        case SORT_TYPE_FREQ:
            dsort2[n] = (double)s->RestFreq;
            break;
        case SORT_TYPE_VLSR:
            dsort2[n] = (double)s->VSource;
            break;
        case SORT_TYPE_TIME:
            dsort2[n] = (double)s->IntTime;
            break;
        case SORT_TYPE_DATE:
            dsort2[n] = (double)s->Year + (double)s->Month/1.0e2 +
                        (double)s->Day/1.0e4 + (
                        (double)s->UTHour + (double)s->UTMin/60.0 +
                        (double)s->UTSec/3600.0)/1.0e6;
            break;
        case SORT_TYPE_RA:
            dsort2[n] = (double)s->Longitude;
            break;
        case SORT_TYPE_DEC:
            dsort2[n] = (double)s->Latitude;
            break;
        case SORT_TYPE_RAOFF:
            dsort2[n] = (double)s->LMapOff;
            break;
        case SORT_TYPE_DEOFF:
            dsort2[n] = (double)s->BMapOff;
            break;
        case SORT_TYPE_DIST:
            dsort2[n] = (double)(s->LMapOff * s->LMapOff +
                                 s->BMapOff * s->BMapOff);
            break;
/* Secondary string sort value */
        case SORT_TYPE_NAME:
            strcpy(ssort2[n], s->Name);
            break;
        case SORT_TYPE_MOLE:
            strcpy(ssort2[n], s->Molecule);
            break;
    }
}

static int SortTrue(int n, int i, XSTR *x)
{
    int a;
    
    if (x->sort1_type < SORT_TYPE_FREQ) {        /* integer sort value */
        if (isort1[n] == isort1[i] && x->sort2_type != SORT_TYPE_NONE)
            a = -1;
        else if (x->sort_order == SORT_INCREASING)
            a = isort1[n] >= isort1[i];
        else
            a = isort1[n] < isort1[i]; 
    } else if (x->sort1_type < SORT_TYPE_NAME) { /* double sort value */
        if (dsort1[n] == dsort1[i] && x->sort2_type != SORT_TYPE_NONE)
            a = -1;
        else if (x->sort_order == SORT_INCREASING)
            a = dsort1[n] >= dsort1[i];
        else
            a = dsort1[n] < dsort1[i]; 
    } else {                                     /* string sort value */
        a = strcoll(ssort1[n], ssort1[i]);
        if (a == 0 && x->sort2_type != SORT_TYPE_NONE)
            a = -1;
        else if (x->sort_order == SORT_INCREASING) {
            if (a > 0) a = 1;
            else a = 0;
        } else {
            if (a < 0) a = 1;
            else a = 0;
        } 
    }
    
    if (a != -1) return a;
    
    if (x->sort2_type < SORT_TYPE_FREQ) {        /* integer sort value */
        if (x->sort_order == SORT_INCREASING)
            a = isort2[n] >= isort2[i];
        else
            a = isort2[n] < isort2[i]; 
    } else if (x->sort2_type < SORT_TYPE_NAME) { /* double sort value */
        if (x->sort_order == SORT_INCREASING)
            a = dsort2[n] >= dsort2[i];
        else
            a = dsort2[n] < dsort2[i]; 
    } else {                                     /* string sort value */
        a = strcoll(ssort1[n], ssort1[i]);
        if (x->sort_order == SORT_INCREASING) {
            if (a > 0) a = 1;
            else a = 0;
        } else {
            if (a < 0) a = 1;
            else a = 0;
        } 
    }
    
    return a;
}

void AddSort(int n, XSTR *x, SCAN *s)
{
    int i, j;
    
    if (x->sort1_type == SORT_TYPE_NONE) {
        x->order[n] = n;
        return;
    }
    
    SetSortValue(n, x->sort1_type, x->sort2_type, s);
    
    j = 0;
    for (i=0; i<n; i++) {
        if (SortTrue(n, i, x)) {
            j++;
        } else {
            x->order[i] = x->order[i] + 1;
        }
    }
    x->order[n] = j;
}

static void swap_xstr(XSTR *x, int i, int j)
{
    int tmp;
    XmString s;
    
    tmp = x->order[i]; x->order[i] = x->order[j]; x->order[j] = tmp;
    
    s = XmStringCopy(x->files[i]);           XmStringFree(x->files[i]);
    x->files[i] = XmStringCopy(x->files[j]); XmStringFree(x->files[j]);
    x->files[j] = XmStringCopy(s);           XmStringFree(s);
    
    s = XmStringCopy(x->descs[i]);           XmStringFree(x->descs[i]);
    x->descs[i] = XmStringCopy(x->descs[j]); XmStringFree(x->descs[j]);
    x->descs[j] = XmStringCopy(s);           XmStringFree(s);
}

void EndSort(XSTR *x)
{
    int i, j;
    string no;
    XmString s, sno;

    if (x->sort1_type != SORT_TYPE_NONE) {
        if (isort1) XtFree((char *)isort1);
        if (dsort1) XtFree((char *)dsort1);
        if (ssort1) XtFree((char *)ssort1);
        isort1 = NULL;
        dsort1 = NULL;
        ssort1 = NULL;
        if (isort2) XtFree((char *)isort2);
        if (dsort2) XtFree((char *)dsort2);
        if (ssort2) XtFree((char *)ssort2);
        isort2 = NULL;
        dsort2 = NULL;
        ssort2 = NULL;

        for (i=0; i<x->size-1; i++) {
            for (j=i+1; j<x->size; j++) {
                if (x->order[i] > x->order[j]) {
                    swap_xstr(x, i, j);
                }
            }
        }
    }
    for (i=0; i<x->size; i++) {
        sprintf(no, "%4d ", x->order[i] + 1);
        sno = MKSTRING(no);
        s = XmStringConcat(sno, x->descs[i]);
        XmStringFree(sno);
        XmStringFree(x->descs[i]);
        x->descs[i] = XmStringCopy(s);
        XmStringFree(s);
    }
}

