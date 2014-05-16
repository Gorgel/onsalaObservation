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
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <ctype.h>

#include <Xm/SelectioB.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>

#include "drp.h"
#include "defines.h"
#include "global_structs.h"
#include "dfil.h"
#include "fits.h"

/*** External variables ***/
extern GLOBAL *gp;
extern VIEW   *vP;
extern SCAN    OnScan;
extern XSCAN   XScan;

void ManageDialogCenteredOnPointer(Widget);

int        count_scans(DataSetPtr);
list      *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);

/*** Local variables ***/
typedef struct {
   FILE *pdf;
   int  df;
   int  bl_no;
   int  bl_len;
   int  unit;
   int  rec;
   int  nscans;
   char *fname;
   Widget form;
} ANTS;

typedef struct {
  Widget w;
  int n;
} KLUDGE;

static string type_of_seq;

#define NOARGUMENT 1
#define DFILERROR  2
#define NOTFOUND   3
#define ALLOCERR   4
#define OPTIONERR  4

static char   dfilname[48];
static DFWORD indx[DFILBLOCK];
static DFWORD *itwh;

static void hms(float rad, short *hour, short *min, short *sec)
{
    long int seconds;

    seconds = (long int) rad*RADTOSEC/15.0;
    *hour = (DFWORD) seconds/3600;
    *min  = (DFWORD) ((seconds/60)%60);
    *sec  = (DFWORD) (seconds%60);
}

static void swap(char *p, int n)
{
    char swap;
    int i;

    for (i = 0; i < n/2; i++) {
        swap = p[i];
        p[i] = p[n-1-i];
        p[n-1-i] = swap;
    }
}

static double tod(DFWORD i1, DFWORD i2, DFWORD i3, int swapped)
{
    /* DFWORD d[4]; */
    double tmp;
    
    union {
        DFWORD d[4];
        double tmp;
    } b;

    /* d[0]=i1;
    d[1]=i2;
    d[2]=i3;
    d[3]=0;
    tmp = *(double *)d; */
    
    b.d[0] = i1; b.d[1] = i2; b.d[2] = i3; b.d[3] = 0;
    
    tmp = b.tmp;
    
    if (swapped) swap((char *)&tmp, sizeof(double));
    
    return tmp;
}

static void swap_twh()
{
    float *twh = (float *)itwh;
    
    swap((char *)&itwh[0], sizeof(DFWORD));

    swap((char *)&itwh[27], sizeof(DFWORD));
    swap((char *)&itwh[28], sizeof(DFWORD));
    swap((char *)&itwh[29], sizeof(DFWORD));
    swap((char *)&itwh[30], sizeof(DFWORD));
        
    swap((char *)&twh[9],  sizeof(float));
    swap((char *)&twh[11], sizeof(float));
    swap((char *)&twh[12], sizeof(float));
    swap((char *)&twh[24], sizeof(float));
    
    swap((char *)&twh[41], sizeof(float));
    swap((char *)&twh[42], sizeof(float));
    
    swap((char *)&twh[45], sizeof(float));
    swap((char *)&twh[46], sizeof(float));
    swap((char *)&twh[47], sizeof(float));
    swap((char *)&twh[48], sizeof(float));
    swap((char *)&twh[49], sizeof(float));
    swap((char *)&twh[50], sizeof(float));
    swap((char *)&twh[51], sizeof(float));
    swap((char *)&twh[52], sizeof(float));
    swap((char *)&twh[53], sizeof(float));
    swap((char *)&twh[54], sizeof(float));
    swap((char *)&twh[55], sizeof(float));
    swap((char *)&twh[56], sizeof(float));
    swap((char *)&twh[57], sizeof(float));
    
    swap((char *)&twh[77], sizeof(float));
    swap((char *)&twh[78], sizeof(float));
    swap((char *)&twh[79], sizeof(float));
    
    swap((char *)&twh[88], sizeof(float));
    swap((char *)&twh[89], sizeof(float));
    swap((char *)&twh[90], sizeof(float));
    swap((char *)&twh[91], sizeof(float));
    
    swap((char *)&twh[140], sizeof(float));
    swap((char *)&twh[141], sizeof(float));
    swap((char *)&twh[142], sizeof(float));
}

static void filltwh(SCAN *scan, XSCAN *xscan, int rec)
{
    int i, swapped = 0;
    float *next;
    float *twh;
    double fact;
	
	int CheckDataSize(int);
	
    twh = (float *)itwh;
    
    fact = twh[57];
    if (fact == 0.0) fact = 1.0;
    
    scan->ScanNo = itwh[0];
    scan->NChannel = (int)(twh[55+rec]/fact + 0.5);
    xscan->NChannel = (int)(twh[55+rec]/fact + 0.5);
    scan->Day   = itwh[28];
    scan->Month = itwh[27];
#ifdef DEBUG
    printf("NChannel=%d\n", xscan->NChannel);
    printf("ScanNo=%d\n", scan->ScanNo);
    printf("fact=%f\n", fact);
#endif

    if (scan->NChannel <= 0 || scan->ScanNo < 0 ||
        scan->Day < 0 || scan->Day > 31 ||
        scan->Month < 0 || scan->Month > 12) {
        swap_twh();
        fact = twh[57];
        if (fact == 0.0) fact = 1.0;
        scan->ScanNo = itwh[0];
        scan->NChannel = (int)(twh[55+rec]/fact + 0.5);
        scan->Day   = itwh[28];
        scan->Month = itwh[27];
#ifdef DEBUG
        printf("Swapped NChannel=%d\n", scan->NChannel);
        printf("        ScanNo=%d\n", scan->ScanNo);
        printf("        fact=%f\n", fact);
#endif
        if (scan->NChannel <= 0 || scan->ScanNo < 0 ||
            scan->Day < 0 || scan->Day > 31 ||
            scan->Month < 0 || scan->Month > 12) return;
        swapped = 1;
    }
    	
	if (CheckDataSize(scan->NChannel) < scan->NChannel) {
	    return;
	}
	
    scan->Year = itwh[29];
    if (scan->Year < 1900) scan->Year += 1900;
    hms(twh[12]/fact, &scan->UTHour, &scan->UTMin, &scan->UTSec);
    hms(twh[11]/fact, &scan->STHour, &scan->STMin, &scan->STSec);
    scan->ObsMode = itwh[30];
    scan->CSystem = 0;
    strncpy(scan->Name, (char *)(itwh+12), 12);
    scan->Name[11] = '\0';
    strncpy(scan->Project, (char *)(itwh+8), 4);
    strncpy(scan->Molecule, (char *)(itwh+260), 18);
    scan->Molecule[17] = '\0';
    scan->JulDate = (int)(twh[9]/fact + 0.5);
    scan->LMapOff = twh[90]/fact;
    scan->BMapOff = twh[91]/fact;
    scan->AzMapOff = twh[88]/fact;
    scan->ElMapOff = twh[89]/fact;
    scan->StepX = twh[140]/fact;
    scan->StepY = twh[141]/fact;
    scan->PosAngle = twh[142]/fact;
    scan->Equinox = twh[24]/fact;
    scan->Tsys = twh[45+rec]/fact;
    scan->Tcal = twh[41+rec]/fact;
    scan->IntTime = twh[47+rec]/fact;
    scan->AirTemp = twh[77]/fact;
    scan->Pressure = twh[78]/fact;
    scan->Humidity = twh[79]/fact;
    scan->VSource = (double)twh[49+rec]/fact;
    scan->VelRes = (double)twh[51+rec]/fact;
    scan->Bandwidth = (double)twh[53+rec]/fact;
    scan->FreqRes = scan->Bandwidth/scan->NChannel;
    next = twh+150;
    if (rec) next += (int)(twh[55]/fact + 0.5);
    for (i=0; i<scan->NChannel; i++) {
        if (swapped) swap((char *)next, sizeof(float));
        scan->c[i] = (*next++)/fact;
    }
    scan->Longitude = tod(itwh[42],itwh[43],itwh[44],swapped)/fact;
    scan->Latitude = tod(itwh[45],itwh[46],itwh[47],swapped)/fact;
    scan->Azimuth = tod(itwh[164],itwh[165],itwh[166],swapped)/fact;
    scan->Elevation = tod(itwh[167],itwh[168],itwh[169],swapped)/fact;
    scan->RestFreq = tod(itwh[70+3*rec],itwh[71+3*rec],itwh[72+3*rec],swapped)/fact;
    scan->SkyFreq = tod(itwh[76+3*rec],itwh[77+3*rec],itwh[78+3*rec],swapped)/fact;
}

static int ret_open_ants(XmString *xmstr, int nstr, int err)
{
    int n = nstr;
    
    while (n > 0)
        XmStringFree(xmstr[--n]);
        
    if (xmstr) XtFree((char *)xmstr);
        
    return err;
}

static void cancel_pops_dialog(Widget w, ANTS *a, XmAnyCallbackStruct *cb)
{
    if (!a) return;
    if (a->form) XtDestroyWidget(a->form);
    if (a->pdf) fclose(a->pdf);
}

static void reset_pops_dialog(Widget w, KLUDGE *shit, XmAnyCallbackStruct *cb)
{
    if (shit && shit->w) XmListDeselectAllItems(shit->w);
}

static void all_pops_dialog(Widget w, KLUDGE *shit, XmAnyCallbackStruct *cb)
{
    int n = 0;
    Arg wargs[2];
    
    reset_pops_dialog(w, shit, cb);
    
    XtSetArg(wargs[n], XmNselectionPolicy, XmMULTIPLE_SELECT); n++;
    XtSetValues(shit->w, wargs, n);
    
    for (n=1; n<=shit->n; n++)
        XmListSelectPos(shit->w, n, (n == shit->n) ? True : False);
    
    n = 0;
    XtSetArg(wargs[n], XmNselectionPolicy, XmEXTENDED_SELECT); n++;
    XtSetValues(shit->w, wargs, n);
}

static Widget create_list_dialog(Widget parent, char *fname, ANTS *a,
                                 Arg *args, int nargs)
{
    int n;
    Arg wargs[10];
    Widget form, label, listDialog, sep, cancel, all, reset;
    static KLUDGE shit;
    
    n = 0;
    XtSetArg(wargs[n], XmNautoUnmanage, FALSE); n++;
    XtSetArg(wargs[n], XmNtitle, "Read POPS files"); n++;
    form = XmCreateFormDialog(parent, "form", wargs, n);
    a->form = form;
    
    label = XtCreateManagedWidget(fname, xmLabelWidgetClass,
                                  form, NULL, 0);
    
    listDialog = XmCreateScrolledList(form, "list", args, nargs);
    XtManageChild(listDialog);

    sep = XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
				                  form, XmNseparatorType,
				                  XmSHADOW_ETCHED_IN, NULL);
    cancel = XtCreateManagedWidget(BUTT_CANCEL, xmPushButtonWidgetClass,
                                   form, NULL, 0);

    XtVaSetValues(form, XmNdefaultButton, cancel, NULL);
    
    all  = XtCreateManagedWidget("Select all", xmPushButtonWidgetClass,
                                 form, NULL, 0);
    shit.w = listDialog;
    shit.n = a->nscans;
    
    reset  = XtCreateManagedWidget("Unselect all", xmPushButtonWidgetClass,
                                   form, NULL, 0);
    XtAddCallback(cancel, XmNactivateCallback,
                  (XtCallbackProc)cancel_pops_dialog, a);
    XtAddCallback(reset, XmNactivateCallback,
                  (XtCallbackProc)reset_pops_dialog, &shit);
    XtAddCallback(all, XmNactivateCallback,
                  (XtCallbackProc)all_pops_dialog, &shit);
    
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetArg(wargs[n], XmNrightAttachment,   XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,       10); n++;
    XtSetValues(XtParent(listDialog), wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         XtParent(listDialog)); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,        10); n++;
    XtSetArg(wargs[n], XmNrightAttachment,   XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,       10); n++;
    XtSetValues(label, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,     XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,         label); n++;
    XtSetArg(wargs[n], XmNtopOffset,         10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,        1); n++;
    XtSetArg(wargs[n], XmNrightAttachment,   XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,       1); n++;
    XtSetValues(sep, wargs, n);
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
    
    ManageDialogCenteredOnPointer(form);

    return listDialog;
}

static int open_ants_block(char *txt, ANTS *a)
{
    char cbl[5];
    string buf;
    int i;

    void send_line();

    for (i=0; i<4; i++) cbl[i] = txt[i];
    cbl[4] = '\0';
    sscanf(cbl, "%d", &(a->bl_no));
    if (a->bl_no < 1 || a->bl_no > 10000) {
        sprintf(buf, "Strange block (%d) selected in %s.\n",
                a->bl_no, a->fname);
        send_line(buf);
        return 1;
    }
    sprintf(buf, "Block no. %d is selected in %s.\n", a->bl_no, a->fname);
    send_line(buf);
    
    if (GetDfilBlock(a->pdf, a->bl_no, a->bl_len, itwh) != 0) return 1;
    
    filltwh(&OnScan, &XScan, a->rec); 
    OnScan.Slength = HEADER+OnScan.NChannel*2;
    
    return 0;
}

static void selectCB(Widget w, ANTS *a, XmListCallbackStruct *cb)
{
    char *txt;
    int n, is_seq = 0, is_app = 0, err=0, first = 1;
    DataSetPtr d = NULL;
    
    void UpdateData(), obtain_map_info();
    int read_file(char *, char *, DataSetPtr);
    
    if (cb->reason == XmCR_BROWSE_SELECT) {
        XmStringGetLtoR(cb->item, XmSTRING_DEFAULT_CHARSET, &txt);
        if (open_ants_block(txt, a) == 0) {
            d = new_dataset(get_listlist(), "Single POPS", NULL);
            if (d && !read_file("ants", a->fname, d)) {
                vP->from = vP->to = d;
                strcpy(d->name, vP->s->name);
            }
        }
        XtFree(txt);
    } else {
        if (strncmp(type_of_seq, "seqants", 7) == 0 ||
            strncmp(type_of_seq, "appseqants", 10)==0) is_seq = 1;
        if (strncmp(type_of_seq, "app", 3)==0) is_app = 1;
        
        if (!vP->to || !is_app) {
            d = new_dataset(get_listlist(), "POPS", NULL);
            if (!d) return;
        } else {
            d = NULL;
        }
        
        sprintf(d ? d->name : vP->to->name,
                "%s %d scans",
                is_seq ? "Sequence of" : "Read",
                cb->selected_item_count + count_scans(d));
        
        for (n=0; n<cb->selected_item_count; n++) {
            if (XmStringGetLtoR(cb->selected_items[n],
                                XmSTRING_DEFAULT_CHARSET, &txt)) {
                if (open_ants_block(txt, a) == 0) {
                    err = read_file(is_seq ? "seqants" : "ants",
                                    a->fname,
                                    first ? d : NULL);
                    if (!err && first) {
                        first = 0;
                    } else if (err == 1) { /* Out of memory */
                        XtFree(txt);
                        break;
                    }
                }
                XtFree(txt);
            }
        }
        if (d) vP->to = d;
        vP->from = vP->to;
        
        if (count_scans(vP->from) > 1)
            obtain_map_info(NULL, "map", NULL);
    }
    UpdateData(SCALE_BOTH, REDRAW);
}

int open_ants(Widget w, char *fn, char *cmd)
{
    Widget list;
    Arg wargs[10];
    XmString *xmstr;
    char *p;
    string stmp, buf, user;
    int i, n, nindx, df, rec=0, bl_len;
    int err;
    size_t size;
    FILE *pdf;
    static ANTS ants;
    
    void send_line();
    char *getScanStr();
    
    strcpy(dfilname, fn);
    strcpy(stmp, fn);
    
    p = strtok(stmp, "/");
    while (p) {
        strcpy(buf, p);
        p = strtok(NULL, "/");
    }
    
    sscanf(buf, "DF%2d%4d.%s", &df, &bl_len, user);
    
    if (bl_len == 0) bl_len = 3;

    pdf = OpenDfil(df, bl_len, user, dfilname);
    if (!pdf) {
        sprintf(buf, "Open POPS file: cannot open dfile %s.", dfilname);
        send_line(buf);
        return 1;
    }
    
    nindx = ReadDfilIndex(pdf, indx);
    if (nindx <= 0) {
        if (nindx == 0) {
            sprintf(buf, "Open POPS file: dfile %s is empty.", dfilname);
        } else {
            sprintf(buf, "Open POPS file: dfile %s is corrupt.", dfilname);
        }
        send_line(buf);
        fclose(pdf);
        return 1;
    }
    
    i = 0;
    while (i < 500 && indx[i] != 0) {
        i++;
    }
    nindx = i;
    
    sprintf(buf, "POPS dfile %s (%d scans) is opened.", dfilname, nindx);
    send_line(buf);
    
    size = (size_t)(bl_len*DFILBLOCK)*sizeof(DFWORD);
    if (!itwh)
        itwh = (DFWORD *)malloc(size);
    else
        itwh = (DFWORD *)realloc(itwh, size);
    if (!itwh) {
        sprintf(buf, "Open POPS -- can't allocate itwh[%d]",
                bl_len*DFILBLOCK);
        send_line(buf);
        fclose(pdf);
        return 1;
    }
    
    strcpy(type_of_seq, cmd);
    
    if (strcmp(cmd, "mapants2") == 0 ||
        strcmp(cmd, "ants2")    == 0 ||
        strcmp(cmd, "seqants2") == 0 ||
        strcmp(cmd, "appmapants2") == 0 ||
        strcmp(cmd, "appseqants2") == 0)
        rec = 1;

    ants.pdf    = pdf;
    ants.bl_len = bl_len;
    ants.df     = df;
    ants.rec    = rec;
    ants.fname  = dfilname;
    ants.nscans = nindx;
    
    xmstr = (XmString *) XtMalloc(nindx * sizeof(XmString));
    if (!xmstr) {
        sprintf(buf, "Open POPS -- can't allocate xmstr[%d]", nindx);
        send_line(buf);
        fclose(pdf);
        return 1;
    }

    for (i=0; i<nindx; i++) {
        err = GetDfilBlock(pdf, i+1, bl_len, itwh);
        if (err != 0) {
            sprintf(buf, "Open POPS -- can't read read block %d", i+1);
            send_line(buf);
            fclose(pdf);
            return ret_open_ants(xmstr, nindx, err);
        }
        filltwh(&OnScan, &XScan, rec); 
        OnScan.Slength = HEADER + 2*OnScan.NChannel;
        xmstr[i] = MKSTRING(getScanStr(i));
    }

    n = 0;
    if (nindx > 0) {
        XtSetArg(wargs[n], XmNitemCount, nindx); n++;
        XtSetArg(wargs[n], XmNitems, xmstr); n++;
        if (nindx <= 30) {
            XtSetArg(wargs[n], XmNvisibleItemCount, nindx); n++;
        } else {
            XtSetArg(wargs[n], XmNvisibleItemCount, 30); n++;
        }
    }
    XtSetArg(wargs[n], XmNfontList, gp->flist10); n++;
    if (strncmp(cmd, "mapants", 7)==0 ||
        strncmp(cmd, "seqants", 7)==0 ||
        strncmp(cmd, "appmapants", 10)==0 ||
        strncmp(cmd, "appseqants", 10)==0) {
        XtSetArg(wargs[n], XmNselectionPolicy, XmEXTENDED_SELECT); n++;
    }
    
    list = create_list_dialog(w, dfilname, &ants, wargs, n);

    if (strncmp(cmd, "mapants", 7)==0 ||
        strncmp(cmd, "seqants", 7)==0 ||
        strncmp(cmd, "appmapants", 10)==0 ||
        strncmp(cmd, "appseqants", 10)==0) {
        XtAddCallback(list, XmNextendedSelectionCallback,
                      (XtCallbackProc)selectCB, &ants);
        XtAddCallback(list, XmNmultipleSelectionCallback,
                      (XtCallbackProc)selectCB, &ants);
    } else
        XtAddCallback(list, XmNbrowseSelectionCallback,
                      (XtCallbackProc)selectCB, &ants);

    return ret_open_ants(xmstr, nindx, 0);
}

int get_ants(FDATA *fd)
{
    string buf;
    
    void send_line(), DRP2FD();
    
    DRP2FD(&OnScan, &XScan, fd);
    
    sprintf(buf, "POPS scan read successfully.");
    send_line(buf);
    
    return 0;
}
