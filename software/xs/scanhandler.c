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
#include <strings.h>
#include <math.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>

#include "defines.h"
#include "global_structs.h"
#include "dialogs.h"

extern GLOBAL *gp;
extern VIEW   *vP;

void PostWarningDialog(Widget, char *);
void PostErrorDialog(Widget, char *);
void PostMessageDialog(Widget, char *);
int  PostQuestionDialog(Widget, char *);
list *get_action_list(XmListCallbackStruct *, int *, list);
void ManageDialogCenteredOnPointer(Widget);
int  SetViewMode(int, scanPtr, MAP *, scatter *);
void UpdateData(int, int);

/* Local declarations */

status  init_list(list *);
status  insert(list *, generic_ptr);
status  delete_node(list *, list);
list    list_iterator(list, list);
bool    empty_list(list);

scanPtr     new_scan(DataSetPtr, int);
DataSetPtr  new_dataset(list *, char *, DataSetPtr);
static void UpdateDataSetListDialog(Widget);
static void UpdateScanListDialog(Widget);

static list listlist, tmplist;

static DataSetPtr tmpds = NULL, RMSdsptr = NULL;

static scanPtr RefScan=NULL;

unsigned long int scan_bytes = 0;

void init_scanlist()
{
    init_list(&listlist);
    init_list(&tmplist);
    
    tmpds = new_dataset(&tmplist, "tmp", NULL);
}

double GetScanMemory()
{
    return (double)scan_bytes/1024.0/1024.0;
}

list *get_listlist()
{
    return &listlist;
}

list *get_tmplist()
{
    return &tmplist;
}

DataSetPtr get_tmpdataset()
{
    return tmpds;
}

/* ------------------------------------------------------------- */

int count_scans(DataSetPtr ds)
{
    int n = 0;
    list curr = NULL;

    if (!ds) return 0;
    if (!ds->scanlist) return 0;
    
    if (empty_list(listlist) == tRUE)
        return 0;
    
    if (empty_list(ds->scanlist) == tRUE)
        return 0;

    while ((curr = list_iterator(ds->scanlist, curr)) != NULL)
        n++;

    return n;
}

static scanPtr cleanup_ret(scanPtr pS)
{
    if (pS) {
        if (pS->d) free(pS->d);
        if (pS->e) free(pS->e);
        free(pS);
    }
    return NULL;
}

scanPtr new_scan(DataSetPtr ds, int nChan)
{
    scanPtr pS = NULL;
    
    pS = (scan *)calloc(1, sizeof(scan));
    if (!pS)
        return cleanup_ret(pS);

    pS->d = NULL;
    pS->e = NULL;
    pS->nChan = nChan;
    pS->saved = 0;

    pS->d = (double *)malloc(nChan * sizeof(double));
    if (!pS->d)
        return cleanup_ret(pS);

    pS->e = (double *)malloc(nChan * sizeof(double));
    if (!pS->e)
        return cleanup_ret(pS);

    if (insert(&(ds->scanlist), (generic_ptr)pS) == Error)
        return cleanup_ret(pS);

    scan_bytes += 2 * nChan * sizeof(double) + sizeof(scan);
    
    return pS;
}

static list *delete_scan(list *pL, list node)
{
    unsigned long int n = 0;
    scanPtr pS = NULL;

    void ZeroScanInAllMaps(scanPtr s);
    void ZeroScanInAllScatters(scanPtr s);
    
    if (empty_list(*pL) == tRUE || !node)
        return NULL;

    pS = (scanPtr)DATA(node);
    
    if (pS) {
        ZeroScanInAllMaps(pS);
        ZeroScanInAllScatters(pS);
        n = 2 * pS->nChan * sizeof(double) + sizeof(scan);
        cleanup_ret(pS);
    }

    if (delete_node(pL, node) == Error)
        return NULL;

    if (n < scan_bytes)
        scan_bytes -= n;
    else
        scan_bytes = 0;
     
    return pL;
}

int DeleteScan(DataSetPtr ds, scanPtr del)
{
    list curr = NULL;
    
    if (!ds) return 1;
    
    while ( (curr = (curr == NULL) ? ds->scanlist : NEXT(curr)) ) {
        if (del == (scanPtr)DATA(curr)) {
            delete_scan(&(ds->scanlist), curr);
            return 0;
        }
    }
    
    return 1;
}

scanPtr copy_scan(DataSetPtr ds, scanPtr old)
{
    int n;
    double *d, *e;
    scanPtr new = NULL;
    
    if (!old || !ds)
        return new;
    
    new = new_scan(ds, old->nChan);
    if (!new)
        return new;    
    
    d = new->d;
    e = new->e;
    
    *new = *old;
    
    new->d = d;
    new->e = e;

    for (n=0; n<old->nChan; n++) {
        d[n] = old->d[n];
        e[n] = old->e[n];
    }
    
    return new;
}

scanPtr copy_scanheader(DataSetPtr ds, int nChan, scanPtr old)
{
    double *d, *e;
    scanPtr new = NULL;
    
    if (!old)
        return new;
    
    new = new_scan(ds, nChan);
    if (!new)
        return new;    
    
    d = new->d;
    e = new->e;
    
    *new = *old;
    
    new->d = d;
    new->e = e;
    new->nChan = nChan;
    
    return new;
}

scanPtr resize_scan(list *pL, list node, int newNChan)
{
    scanPtr pS = NULL;
    
    if (empty_list(*pL) == tRUE || !node)
        return pS;
    
    pS = (scanPtr)DATA(node);
    
    if (!pS)
        return pS;
        
    if (pS->nChan == newNChan)
        return pS;
        
     pS->d = realloc(pS->d, newNChan * sizeof(double));
     if (!pS->d) {
         delete_scan(pL, node);
         return NULL;
     }

     pS->e = realloc(pS->e, newNChan * sizeof(double));
     if (!pS->e)
         delete_scan(pL, node);
         
     return pS;
}

list scan_iterator(list last, DataSetPtr ds)
{
    return (last == NULL) ? ds->scanlist : NEXT(last);
}

void setup_previous_scans(DataSetPtr ds)
{
    void setup_prev(list);
    
    setup_prev(ds->scanlist);
}

list previous_scan(list last, DataSetPtr ds)
{
    return (last == NULL) ? ds->scanlist : PREV(last);
}

void destroy_scans(DataSetPtr ds)
{
    list curr = ds->scanlist;

    while (delete_scan(&curr, curr) != NULL)
        ;
}

/* ------------------------------------------------------------- */

int count_dataset()
{
    int n = 0;
    list curr = NULL;

    if (empty_list(listlist) == tRUE)
        return 0;

    while ((curr = list_iterator(listlist, curr)) != NULL)
        n++;

    return n;
}


void attach_boxes_to_dataset(DataSetPtr d, int r, BOX *rms, int m, BOX *mom)
{
    int n;
    
    if (!d) return;
    
    if (r && rms) {
        d->rms = (BOX *)malloc(r * sizeof(BOX));
        if (d->rms) {
            d->r = r;
            for (n=0; n<r; n++) {
                /* HP715 c89 -O kludge for d->rms[n] = rms[n]; */
                d->rms[n].begin = rms[n].begin;
                d->rms[n].end   = rms[n].end;
            }
        } else {
            d->r = 0;
        }
    } else {
        d->r = 0;
        d->rms = NULL;
    }
    
    if (m && mom) {
        d->mom = (BOX *)malloc(m * sizeof(BOX));
        if (d->mom) {
            d->m = m;
            for (n=0; n<m; n++) {
                /* HP715 c89 -O kludge for d->mom[n] = mom[n]; */
                d->mom[n].begin = mom[n].begin;
                d->mom[n].end   = mom[n].end;
            }
        } else {
            d->m = 0;
        }
    } else {
        d->m = 0;
        d->mom = NULL;
    }
    
    scan_bytes += (d->r + d->m) * sizeof(BOX);
}

void detach_boxes_from_dataset(DataSetPtr d)
{
    if (!d) return;
    
    if (d->rms) {
        free((char *)d->rms);
        scan_bytes -= d->r * sizeof(BOX);
    }
    d->rms = NULL;
    d->r = 0;
    
    if (d->mom) {
        free((char *)d->mom);
        scan_bytes -= d->m * sizeof(BOX);
    }
    d->mom = NULL;
    d->m = 0;
}

DataSetPtr new_dataset(list *pList, char *name, DataSetPtr old)
{
    DataSetPtr pD = NULL;
    list *pL = pList;
    
    if (!pL) pL = &listlist;
    
    pD = (DataSetPtr)calloc(1, sizeof(DataSet));
    if (!pD)
        return NULL;

    if (insert(pL, (generic_ptr)pD) == Error) {
        free((char *)pD);
        return NULL;
    }
    
    if (name) strcpy(pD->name, name);
    if (old) {
        pD->gridded = old->gridded;
        pD->sequence = old->sequence;
        pD->dx = old->dx;
        pD->dy = old->dy;
        pD->posAngle = old->posAngle;
        attach_boxes_to_dataset(pD, old->r, old->rms, old->m, old->mom);
    } else {
        pD->sequence = 0;
        pD->gridded = 0;
        pD->dx = pD->dy = 0.0;
        pD->posAngle = 0.0;
        pD->m = pD->r = 0;
        pD->mom = pD->rms = NULL;
    }
    
    
    scan_bytes += sizeof(DataSet);
    
    init_list(&(pD->scanlist));

    return pD;
}

list *delete_dataset(list *pList, list node)
{
    unsigned long int n = 0;
    DataSetPtr pD = NULL;
    list *pL = pList;
    
    if (!pL) pL = &listlist;

    if (empty_list(*pL) == tRUE || !node)
        return NULL;

    pD = (DataSetPtr)DATA(node);
    
    if (pD) {
        n = sizeof(DataSet);
        destroy_scans(pD);
        detach_boxes_from_dataset(pD);
    }

    if (delete_node(pL, node) == Error)
        return NULL;

    if (n < scan_bytes)
        scan_bytes -= n;
    else
        scan_bytes = 0;

    return pL;
}

list dataset_iterator(list last)
{
    return (last == NULL) ? listlist : NEXT(last);
}

void destroy_datasets(list *pL)
{
    list curr = *pL;

    while (delete_dataset(&curr, curr) != NULL)
        ;
}

void DeleteLastDataSet()
{
    if (empty_list(listlist) == tRUE) return;
    
    delete_dataset(&listlist, listlist);
}

void init_datasets()
{
    vP->to = new_dataset(&listlist, "Initial", NULL);
    vP->from = vP->to;
}

void SelectDataSet(Widget w, DataSetPtr d, XmListCallbackStruct *cb)
{
    int n;
    
    void obtain_map_info();
    void SetBoxesFromDataset(DataSetPtr);
    
    vP->to = vP->from = d;
    n = count_scans(d);
    if (n <= 0) return;
    
    if (d && (d->r || d->m)) {
        if (PostQuestionDialog(w, "Use boxes attached to this data set?")) {
            SetBoxesFromDataset(d);
        }
    }
    
    if (n == 1) {
        SetViewMode(SHOW_SPE, (scanPtr)DATA(d->scanlist), vP->m, vP->p);
    } else {
        SetViewMode(vP->mode, (scanPtr)DATA(d->scanlist), vP->m, vP->p);
        obtain_map_info(NULL, "map", NULL);
    } 
    UpdateData(SCALE_BOTH, REDRAW);
}

/* ------------------------------------------------------------- */

void DataSetDelete(Widget w,StdForm *sf, XmListCallbackStruct *cb)
{
    int n, i, nL;
    string buf;
    list *pL, new;
    DataSetPtr d = NULL;

    void send_line(), UpdateHeaderInfo();

    if ((pL = get_action_list(cb, &nL, listlist)) == NULL)
        return;
	
    if (nL < 1) {
        PostErrorDialog(NULL, "You did not select any datasets!");
        free(pL);
        return;
    }
    
    sprintf(buf, "Delete the %d selected datasets?", nL);
    if (!PostQuestionDialog(w, buf)) {
        free(pL);
	return;
    }

    i = 0;
    for (n=0; n<nL; n++) {
        if (vP->from && vP->from == (DataSetPtr)DATA(pL[n])) {
            sprintf(buf,
               "Removing the data set '%s' that was used in current view.", 
                    vP->from->name);
            PostWarningDialog(NULL, buf);
            if ((new = NEXT(pL[n]))) {
                d = vP->from = (DataSetPtr)DATA(new);
                vP->s = (scanPtr)DATA(d->scanlist);
            } else if ((new = listlist)) {
                d = vP->from = (DataSetPtr)DATA(new);
                vP->s = (scanPtr)DATA(d->scanlist);
            } else {
                d = vP->from = NULL;
                vP->s = NULL;
            }
        }
        if (!delete_dataset(&listlist, pL[n])) {
            sprintf(buf, "Couldn't delete data set %d.\n", i+1);
            PostWarningDialog(NULL, buf);
        } else {
            i++;
        }
    }

    free(pL);

    UpdateDataSetListDialog(sf->edit[0]);

    if (i == nL)
        sprintf(buf, "Deleted all %d selected data sets.", nL);
    else
        sprintf(buf,
                "Deleted only %d out of %d selected data sets.",
                i, nL);
    send_line(buf);
    if (d && vP->s) {
        SelectDataSet(w, d, cb);
    } else {
        UpdateHeaderInfo();
    }
}

static void get_new_dataset_name(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    string buf;
    DataSetPtr d = (DataSetPtr)sf->any;

    void send_line(), wsscanf();

    wsscanf(sf->edit[0], d->name);

    sprintf(buf, "Data set renamed as %s.", d->name);
    send_line(buf);
}

void PostDataSetRenameDialog(Widget w, DataSetPtr s)
{
    Widget rc;
    StdForm *sf;

    void send_line(), wprintf();

    if (!s) {
        PostErrorDialog(w, "There is no such data set to rename!");
        return;
    }

    sf = PostStdFormDialog(w, "Rename data set",
             BUTT_APPLY, (XtCallbackProc)get_new_dataset_name, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 1, NULL);
    sf->any = (XtPointer)s;
     
    rc = XtVaCreateManagedWidget("rc", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    XtCreateManagedWidget("Name data set as:", xmLabelWidgetClass,
                          rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);
    
    ArrangeStdFormDialog(sf, rc);

    wprintf(sf->edit[0], "%s",  s->name);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static DataSetPtr filter_dataset(DataSetPtr old)
{
    string buf;
    DataSetPtr new = NULL;
    scanPtr s;
    list curr = NULL;
    
    list      *get_listlist();
    DataSetPtr new_dataset(list *, char *, DataSetPtr);
    int        filter_active(), AllowScan(scanPtr);
    int        count_scans(DataSetPtr);
    
    sprintf(buf, "Filtered %s", old->name);
    new = new_dataset(get_listlist(), buf, old);
    
    if (!new) return NULL;
    
    while ( (curr = scan_iterator(curr, old)) != NULL ) {
        s = (scanPtr)DATA(curr);
	if (filter_active() && AllowScan(s)) copy_scan(new, s);
    }
    
    sprintf(buf, "%s (%d->%d)", new->name, count_scans(old), count_scans(new));
    strcpy(new->name, buf);
    
    return new;
}

void DataSetFilter(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    string buf;
    list *pL;
    DataSetPtr d;

    void send_line();
    int count_scans(DataSetPtr);
    void DeleteLastDataSet();

    if ((pL = get_action_list(cb, &nL, listlist)) == NULL)
        return;

    for (n=0; n<nL; n++) {
	d = filter_dataset((DataSetPtr)DATA(pL[n]));
	if (!d) continue;
	if (count_scans(d) == 0) {
	    DeleteLastDataSet();
	} else {
            sprintf(buf, "%s", d->name);
            send_line(buf);
        }
    }

    free(pL);

    XtDestroyWidget(sf->form);

    sprintf(buf, "Filtered %d selected datasets to new datasets.", nL);
    send_line(buf);
}

void DataSetRename(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    string buf;
    list *pL;

    void send_line();

    if ((pL = get_action_list(cb, &nL, listlist)) == NULL)
        return;

    for (n=0; n<nL; n++)
        PostDataSetRenameDialog(gp->top, (DataSetPtr)DATA(pL[n]));

    free(pL);

    XtDestroyWidget(sf->form);

    sprintf(buf, "Renamed %d selected data sets.", nL);
    send_line(buf);
}

void DataSetShow(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int nL;
    list *pL;

    if ((pL = get_action_list(cb, &nL, listlist)) == NULL)
        return;

    if (nL == 1) {
        SelectDataSet(w, (DataSetPtr)DATA(pL[0]), cb);
        free(pL);
        return;
    }
    
    /* Handle multiple data sets here */
    free(pL);

    return;
}

void ScanShow(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int nL;
    list *pL;

    if (!vP->from) return;
    
    if ((pL = get_action_list(cb, &nL, vP->from->scanlist)) == NULL)
        return;
    
    if (nL == 1) {
        SetViewMode(SHOW_SPE, (scanPtr)DATA(pL[0]), vP->m, vP->p);
        free(pL);
        UpdateData(SCALE_BOTH, REDRAW);
        return;
    }
    free(pL);

    return;
}

void ScanSelect(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    string buf;
    list *pL;
    DataSetPtr d = vP->from, new;
    scanPtr s;
    
    list      *get_listlist();
    DataSetPtr new_dataset(list *, char *, DataSetPtr);
    scanPtr    copy_scan(DataSetPtr, scanPtr);
    void       send_line();

    if (!d) return;
    
    if ((pL = get_action_list(cb, &nL, d->scanlist)) == NULL)
        return;
    
    if (nL == 0) {
        free(pL);
        return;
    }
    
    if (nL == count_scans(d)) {
        free(pL);
        sprintf(buf,
                "You have selected all %d scans.", nL);
        PostMessageDialog(w, buf);
        return;
    }
    
    sprintf(buf, "Selected scans from %s", d->name);
    new = new_dataset(get_listlist(), buf, d);
    
    if (!new) {
        sprintf(buf, "Couldn't allocate new dataset for scans from %s.",
	        d->name);
	send_line(buf);
        return;
    }
    for (n=0; n<nL; n++) {
	s = copy_scan(new, (scanPtr)DATA(pL[n]));
    }
    
    free(pL);
    
    vP->to = new;
    
    sprintf(buf, "Copied %d scans to '%s'.", nL, new->name);
    PostMessageDialog(w, buf);
    
    if (count_scans(new) == 1) {
        SetViewMode(SHOW_SPE, (scanPtr)DATA(new->scanlist),
                    vP->m, vP->p);
    } else {
        SetViewMode(vP->mode, (scanPtr)DATA(new->scanlist),
                    vP->m, vP->p);
    }
    UpdateData(SCALE_BOTH, REDRAW);
    
    UpdateScanListDialog(sf->edit[0]);

    return;
}

void ScanDelete(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    string buf;
    list *pL;
    DataSetPtr d = vP->from;

    if (!d) return;
    
    if ((pL = get_action_list(cb, &nL, d->scanlist)) == NULL)
        return;
    
    if (nL == 0) {
        free(pL);
        return;
    }
    
    if (nL == count_scans(d)) {
        free(pL);
        sprintf(buf,
                "Delete the data set '%s', instead of deleting all %d scans.",
                d->name, nL);
        PostMessageDialog(w, buf);
        return;
    }
    
    sprintf(buf, "Do you want to delete %d scans from data set '%s'?",
            nL, d->name);
    if (!PostQuestionDialog(w, buf)) {
        free(pL);
        return;
    }
    
    for (n=0; n<nL; n++) {
        DeleteScan(d, (scanPtr)DATA(pL[n]));
    }
    
    free(pL);
    
    sprintf(buf, "Deleted %d scans from data set '%s'.", nL, d->name);
    PostMessageDialog(w, buf);
    
    if (count_scans(d) == 1) {
        SetViewMode(SHOW_SPE, (scanPtr)DATA(d->scanlist),
                    vP->m, vP->p);
    } else {
        SetViewMode(vP->mode, (scanPtr)DATA(d->scanlist),
                    vP->m, vP->p);
    }
    UpdateData(SCALE_BOTH, REDRAW);
    
    UpdateScanListDialog(sf->edit[0]);

    return;
}

void MakeDataSetIntoSeq(DataSetPtr d)
{
    int i;
    scanPtr s;
    list curr = NULL;
    
    i = 0;
    while ( (curr = scan_iterator(curr, d)) != NULL ) {
        s = (scanPtr)DATA(curr);
        s->xoffset = -(double)(i % SEQ_NSPEC);
        s->yoffset = -(double)(i / SEQ_NSPEC);
        i++;
    }
    d->sequence = 1;
    d->gridded = 1;
    d->dx = -1.0;
    d->dy = 1.0;
}

void MakeDataSetIntoUnseq(DataSetPtr d)
{
    scanPtr s;
    list curr = NULL;
    
    while ( (curr = scan_iterator(curr, d)) != NULL ) {
        s = (scanPtr)DATA(curr);
        s->xoffset = s->tx;
        s->yoffset = s->ty;
    }
    d->sequence = d->gridded = 0;
}

void DataSetSeq(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    string buf;
    list *pL;
    DataSetPtr d;

    void send_line();

    if ((pL = get_action_list(cb, &nL, listlist)) == NULL)
        return;

    for (n=0; n<nL; n++) {
        d = (DataSetPtr)DATA(pL[n]);
        if (d->sequence) continue; /* Already sequence */
        MakeDataSetIntoSeq(d);
        sprintf(buf, "Data set '%s' made into sequence.", d->name);
        send_line(buf);
    }

    free(pL);
    UpdateDataSetListDialog(sf->edit[0]);
    
    return;
}

void DataSetUnseq(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    string buf;
    list *pL;
    DataSetPtr d;

    void send_line();

    if ((pL = get_action_list(cb, &nL, listlist)) == NULL)
        return;

    for (n=0; n<nL; n++) {
        d = (DataSetPtr)DATA(pL[n]);
        if (!d->sequence) continue; /* Not sequence */
        MakeDataSetIntoUnseq(d);
        sprintf(buf, "Data set '%s' is not a sequence any longer.", d->name);
        send_line(buf);
    }
    
    free(pL);
    UpdateDataSetListDialog(sf->edit[0]);

    return;
}

static void DataSetSubtract(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL, i, c=0;
    list *pL, curr;
    DataSetPtr d, dnew=NULL;
    scanPtr new;
    string buf;

    
    if (!RefScan) {
        PostErrorDialog(NULL, "Couldn't find any reference data to use!");
        return;
    }
    
    if ((pL = get_action_list(cb, &nL, listlist)) == NULL)
        return;
    
    for (n=0; n<nL; n++) {
        d = (DataSetPtr)DATA(pL[n]);
        if (!d) continue;
        if (!dnew) {
            sprintf(buf, "Ref. subtr. %s", d->name);
            dnew = new_dataset(get_listlist(), buf, d);
            if (!dnew) {
                free(pL);
                return;
            }
        }
        curr = NULL;
        while ( (curr = scan_iterator(curr, d)) != NULL) {
            new = copy_scan(dnew, (scanPtr)DATA(curr));
            if (!new) break;
            c++;
            for (i=0; i<new->nChan; i++) {
                if (i >= RefScan->nChan) break;
                new->d[i] -= RefScan->d[i];
                new->e[i] = sqrt(new->e[i] * new->e[i] +
                                  RefScan->e[i] * RefScan->e[i]);
            }
        }
    }
    
    if (c == 0) {
        PostErrorDialog(NULL, "Couldn't create any new data!");
    } else {
        SelectDataSet(w, dnew, cb);
    }
    
    free(pL);
    RefScan = NULL;
    return;
}

static void DataSetSubDiv(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL, i, c=0;
    list *pL, curr;
    DataSetPtr d, dnew=NULL;
    scanPtr new;
    string buf;

    
    if (!RefScan) {
        PostErrorDialog(NULL, "Couldn't find any reference data to use!");
        return;
    }
    
    if ((pL = get_action_list(cb, &nL, listlist)) == NULL)
        return;
    
    for (n=0; n<nL; n++) {
        d = (DataSetPtr)DATA(pL[n]);
        if (!d) continue;
        if (!dnew) {
            sprintf(buf, "Ref. sub/div %s", d->name);
            dnew = new_dataset(get_listlist(), buf, d);
            if (!dnew) {
                free(pL);
                return;
            }
        }
        curr = NULL;
        while ( (curr = scan_iterator(curr, d)) != NULL) {
            new = copy_scan(dnew, (scanPtr)DATA(curr));
            if (!new) break;
            c++;
            for (i=0; i<new->nChan; i++) {
                if (i >= RefScan->nChan) break;
		if (RefScan->d[i] != 0.0)
                    new->d[i] = (new->d[i]/RefScan->d[i] - 1.0);
		else
		    new->d[i] = 0.0;
                new->e[i] = sqrt(new->e[i] * new->e[i] +
                                  RefScan->e[i] * RefScan->e[i]);
            }
        }
    }
    
    if (c == 0) {
        PostErrorDialog(NULL, "Couldn't create any new data!");
    } else {
        SelectDataSet(w, dnew, cb);
    }
    
    free(pL);
    RefScan = NULL;
    return;
}

static void DataSetRMSMerge(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL, i, c=0;
    list *pL, curr, curr1;
    DataSetPtr d, d1=RMSdsptr, dnew=NULL;
    scanPtr new, rms;
    string buf;

    
    if (!d1) {
        PostErrorDialog(NULL, "Couldn't find any RMS data to use!");
        return;
    }
    
    if ((pL = get_action_list(cb, &nL, listlist)) == NULL)
        return;
    
    for (n=0; n<nL; n++) {
        d = (DataSetPtr)DATA(pL[n]);
        if (!d) continue;
        if (!dnew) {
            sprintf(buf, "RMS merge %s/%s", d->name, d1->name);
            dnew = new_dataset(get_listlist(), buf, d);
            if (!dnew) {
                free(pL);
                return;
            }
        }
        curr = curr1 = NULL;
        while ( (curr = scan_iterator(curr, d)) != NULL &&
	        (curr1 = scan_iterator(curr1, d1)) != NULL ) {
            new = copy_scan(dnew, (scanPtr)DATA(curr));
	    rms = (scanPtr)DATA(curr1);
            if (!new) break;
            c++;
            for (i=0; i<new->nChan; i++) {
                if (i >= rms->nChan) break;
                new->e[i] = rms->d[i];
            }
        }
    }
    
    if (c == 0) {
        PostErrorDialog(NULL, "Couldn't create any new data!");
    } else {
        SelectDataSet(w, dnew, cb);
    }
    
    free(pL);
    RMSdsptr = NULL;
    return;
}

static void DataSetRMS(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int nL;
    list *pL;
    DataSetPtr d;

    if ((pL = get_action_list(cb, &nL, listlist)) == NULL)
        return;
        
    if (nL == 1) {
        d = (DataSetPtr)DATA(pL[0]);
        if (d) {
	    RMSdsptr = d;
            free(pL);
            return;
        }
    }
    
    PostErrorDialog(NULL, "Couldn't find any RMS data set!");
    RMSdsptr = NULL;
    free(pL);
    
    return;
}

static void DataSetReference(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int nL;
    list *pL, curr=NULL;
    DataSetPtr d;

    if ((pL = get_action_list(cb, &nL, listlist)) == NULL)
        return;
        
    if (nL == 1) {
        d = (DataSetPtr)DATA(pL[0]);
        if (d) {
            curr = scan_iterator(curr, d);
            if (curr) {
                RefScan = (scanPtr)DATA(curr);
                free(pL);
                return;
            }
        }
    }
    
    PostErrorDialog(NULL, "Couldn't find any reference data!");
    RefScan = NULL;
    free(pL);
    
    return;
}

void DataSetMerge(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL, c, ctot=0, any_seq = 0;
    list *pL, curr;
    string buf;
    DataSetPtr d, new;
    scanPtr s;

    void send_line();

    if ((pL = get_action_list(cb, &nL, listlist)) == NULL)
        return;

    if (nL <= 1) {
        sprintf(buf, "Only %d selected data sets. Not enough to merge.\n", nL);
        PostErrorDialog(NULL, buf);
        free(pL);
        return;
    }
    
    sprintf(buf, "Merge the %d selected datasets?", nL);
    if (!PostQuestionDialog(w, buf)) {
        free(pL);
	    return;
    }
    
    d = (DataSetPtr)DATA(pL[0]);
    sprintf(buf, "Merged %s...", d->name);
    new = new_dataset(get_listlist(), buf, d);
    if (!new) {
        free(pL);
        return;
    }
    new->gridded = 0;
    for (n=0; n<nL; n++) {
        d = (DataSetPtr)DATA(pL[n]);
        if (d->sequence) any_seq = 1;
        curr = NULL;
        c = 0;
        while ( (curr = scan_iterator(curr, d)) != NULL) {
            s = copy_scan(new, (scanPtr)DATA(curr));
            if (!s) {
                break;
            } else {
                ctot++;
                c++;
            }
        }
        sprintf(buf, "Merged %d scans from %s. Total no of scans: %d",
                c, d->name, ctot);
        send_line(buf);
    }
    
    free(pL);
    
    if (ctot == 0) {
        DeleteLastDataSet();
        PostErrorDialog(NULL, "No data in selected scans (Out of memory?)");
        return;
    }
    
    new->sequence = any_seq;
    
    UpdateDataSetListDialog(sf->edit[0]);
    
    SelectDataSet(w, new, cb);
}

static void create_datasetlist_dialog(Widget parent, char *cmd, Arg *args, int nargs)
{
    Widget rc;
    StdForm *sf;
    string title;
    void (*cb_func)();

    if (strcmp(cmd, "Delete") == 0) {
        strcpy(title, "Select data sets from the list\nto be deleted.");
        cb_func = DataSetDelete;
    } else if (strcmp(cmd, "Rename") == 0) {
        strcpy(title, "Select data sets from the list\nto be renamed.");
        cb_func = DataSetRename;
    } else if (strcmp(cmd, "Filter") == 0) {
        strcpy(title, "Select data sets from the list\nto be filtered.");
        cb_func = DataSetFilter;
    } else if (strcmp(cmd, "Merge") == 0) {
        strcpy(title,
            "Select two or more data sets from\nthe list to be merged.");
        cb_func = DataSetMerge;
    } else if (strcmp(cmd, "Seq") == 0) {
        strcpy(title,
            "Select data sets from the list\nto be made into sequences.");
        cb_func = DataSetSeq;
    } else if (strcmp(cmd, "Unseq") == 0) {
        strcpy(title,
            "Select data sets from the list\nto be unsequential (map scans).");
        cb_func = DataSetUnseq;
    } else {
        strcpy(title, "Select a data set from the list\nto be displayed.");
        cb_func = DataSetShow;
    }

    sf = PostStdFormDialog(parent, "Data sets",
             NULL, NULL, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 1, NULL);
     
    rc = XtVaCreateManagedWidget("rc", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);

    XtCreateManagedWidget(title, xmLabelWidgetClass, rc, NULL, 0);

    sf->edit[0] = XmCreateScrolledList(rc, "list", args, nargs);

    XtAddCallback(sf->edit[0], XmNextendedSelectionCallback,
                  (XtCallbackProc)cb_func, sf);

    ArrangeStdFormDialog(sf, rc);
    
    XtManageChild(sf->edit[0]);

    ManageDialogCenteredOnPointer(sf->form);
}

static void create_dualdatasetlist_dialog(Widget parent, char *cmd,
                                          Arg *args, Arg *args2, int nargs)
{
    Widget rc, rc1, rc2;
    StdForm *sf;
    string title, title2;
    void (*cb_func)();
    void (*cb_func2)();

    if (strcmp(cmd, "Subtract") == 0) {
        strcpy(title, "Select data sets from the list.");
        strcpy(title2, "Select reference data set.");
        cb_func = DataSetSubtract;
        cb_func2 = DataSetReference;
    } else if (strcmp(cmd, "Sub+div") == 0) {
        strcpy(title, "Select data sets from the list.");
        strcpy(title2, "Select reference data set.");
        cb_func = DataSetSubDiv;
        cb_func2 = DataSetReference;
    } else if (strcmp(cmd, "RMSmerge") == 0) {
        strcpy(title, "Select data sets from the list.");
        strcpy(title2, "Select rms data set.");
        cb_func = DataSetRMSMerge;
        cb_func2 = DataSetRMS;
    } else {
        return;
    }

    sf = PostStdFormDialog(parent, "Data sets",
             NULL, NULL, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 2, NULL);
     
    rc = XtVaCreateManagedWidget("rc", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmHORIZONTAL,
                                 NULL);
    rc2 = XtVaCreateManagedWidget("rc", xmRowColumnWidgetClass, rc,
                                  XmNorientation, XmVERTICAL,
                                  NULL);

    XtCreateManagedWidget(title2, xmLabelWidgetClass, rc2, NULL, 0);

    sf->edit[1] = XmCreateScrolledList(rc, "list", args2, nargs);

    XtAddCallback(sf->edit[1], XmNsingleSelectionCallback,
                  (XtCallbackProc)cb_func2, sf);
                  
    rc1 = XtVaCreateManagedWidget("rc", xmRowColumnWidgetClass, rc,
                                 XmNorientation, XmVERTICAL,
                                 NULL);

    XtCreateManagedWidget(title, xmLabelWidgetClass, rc1, NULL, 0);

    sf->edit[0] = XmCreateScrolledList(rc, "list", args, nargs);

    XtAddCallback(sf->edit[0], XmNextendedSelectionCallback,
                  (XtCallbackProc)cb_func, sf);

                  
    ArrangeStdFormDialog(sf, rc);
    
    XtManageChild(sf->edit[0]);
    XtManageChild(sf->edit[1]);

    ManageDialogCenteredOnPointer(sf->form);
}

static char *std_scan_list_title =
" No. Scan  Source name        RA         Declination (  xoff,  yoff) \
  Vlsr  Molecule   Cen. fre. Chan  Tsys   Time";

static char *gal_scan_list_title =
" No. Scan  Source name        Gal. long. Gal. lat.   (  xoff,  yoff) \
  Vlsr  Molecule   Cen. fre. Chan  Tsys   Time";

static void create_scanlist_dialog(Widget parent, char *cmd,
                                   Arg *args, int nargs)
{
    int type = COORD_TYPE_EQU;
    string title;
    Widget rc;
    StdForm *sf;

    if (vP->from)
        sprintf(title, "Scan listing of '%s'", vP->from->name);
    else
        sprintf(title, "Scan listing");
    
    sf = PostStdFormDialog(parent, title,
             NULL, NULL, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 1, NULL);

    rc = XtVaCreateManagedWidget("rc", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);

    if (vP->s) {
        if (vP->s->coordType == COORD_TYPE_GAL) type = COORD_TYPE_GAL;
    }
    if (type == COORD_TYPE_GAL) {
        XtVaCreateManagedWidget(gal_scan_list_title, xmLabelWidgetClass, rc,
                                XmNfontList, gp->flist10, NULL);
    } else {
        XtVaCreateManagedWidget(std_scan_list_title, xmLabelWidgetClass, rc,
                                XmNfontList, gp->flist10, NULL);
    }

    sf->edit[0] = XmCreateScrolledList(rc, "list", args, nargs);

    if (strcmp(cmd, "ScanShow") == 0)
        XtAddCallback(sf->edit[0], XmNextendedSelectionCallback,
                      (XtCallbackProc)ScanShow, sf);
    else if (strcmp(cmd, "ScanDelete") == 0)
        XtAddCallback(sf->edit[0], XmNextendedSelectionCallback,
                      (XtCallbackProc)ScanDelete, sf);
    else if (strcmp(cmd, "ScanSelect") == 0)
        XtAddCallback(sf->edit[0], XmNextendedSelectionCallback,
                      (XtCallbackProc)ScanSelect, sf);

    ArrangeStdFormDialog(sf, rc);

    XtManageChild(sf->edit[0]);

    ManageDialogCenteredOnPointer(sf->form);
}

static char *DataSetListing(DataSetPtr s, int i)
{
    static string txt;

    if (!s)
        return NULL;

    if (s->sequence) {
        sprintf(txt, "%4d %s sequence (%d)", i+1, s->name, count_scans(s));
    } else if (s->gridded) {
        sprintf(txt, "%4d %s gridded (%d)", i+1, s->name, count_scans(s));
    } else {
        sprintf(txt, "%4d %s (%d)", i+1, s->name, count_scans(s));
    }
    
    return txt;
}

/*
static char *scan_list_title =
" No. Scan  Source name        RA           Dec          (  xoff,  yoff) \
 Vlsr  Molecule   Cen. fre. Chan Tsys   Time";
"1234 12345 123456789012345678 123456789012 123456789012 (123456,123456) \
123456 1234567890 123456789 1234 123456 123456"
*/
static char *ScanListing(DataSetPtr d, scanPtr s, int i)
{
    static string txt;
    
    double  xmap(scanPtr), ymap(scanPtr);
    char   *GetRAStr(double), *GetDECStr(double);
    char   *GetLongStr(double), *GetLatStr(double);

    if (!s)
        return NULL;

    if (s->coordType == COORD_TYPE_GAL) {
    sprintf(txt,
"%4d %5d %-18s %s %s (%6.1f,%6.1f) %6.1f %-10s %9.1f %4d %6.1f %6.1f",
            i+1, s->scan_no,
            s->name, GetLongStr(s->x0), GetLatStr(s->y0),
            (d->sequence) ? s->tx: xmap(s), (d->sequence) ? s->ty : ymap(s),
            s->vlsr, s->molecule, 500.0*(s->freq0 + s->freqn),
            s->nChan, s->tsys, s->int_time);
    } else {
    sprintf(txt,
"%4d %5d %-18s %s %s (%6.1f,%6.1f) %6.1f %-10s %9.1f %4d %6.1f %6.1f",
            i+1, s->scan_no,
            s->name, GetRAStr(s->x0), GetDECStr(s->y0),
            (d->sequence) ? s->tx: xmap(s), (d->sequence) ? s->ty : ymap(s),
            s->vlsr, s->molecule, 500.0*(s->freq0 + s->freqn),
            s->nChan, s->tsys, s->int_time);
    }
    
    return txt;
}

static XmString *GetDataSetListStrings(int *nset)
{
    int n, ns;
    XmString *xmstr;
    list curr = NULL;
    string buf;
        
    ns = count_dataset();
    if (nset) *nset = ns;
    
    if (ns == 0) return NULL;
    
    xmstr = (XmString *) XtMalloc(ns * sizeof(XmString));
    if (!xmstr) return NULL;
    
    n = 0;
    while ( (curr = dataset_iterator(curr)) != NULL) {
        strcpy(buf, DataSetListing((DataSetPtr)DATA(curr), n));
        xmstr[n] = MKSTRING(buf);
        n++;
    }
    
    return xmstr;
}

static XmString *GetScanListStrings(DataSetPtr d, int *nset)
{
    int n, ns;
    XmString *xmstr;
    list curr = NULL;
    string buf;
        
    ns = count_scans(d);
    if (nset) *nset = ns;
    
    if (ns == 0) return NULL;
    
    xmstr = (XmString *) XtMalloc(ns * sizeof(XmString));
    if (!xmstr) return NULL;
    
    n = 0;
    while ( (curr = scan_iterator(curr, d)) != NULL) {
        strcpy(buf, ScanListing(d, (scanPtr)DATA(curr), n));
        xmstr[n] = MKSTRING(buf);
        n++;
    }
    
    return xmstr;
}

static void CleanupDataSetListStrings(XmString *xmstr, int nset)
{
    int n;
    
    if (xmstr) {
        n = nset;
        while (n > 0) XmStringFree(xmstr[--n]);
        
        XtFree((char *)xmstr);
    }
}

static void UpdateDataSetListDialog(Widget listD)
{
    int nset;
    XmString *xmstr;
    
    if (!listD) return;
    
    XmListDeselectAllItems(listD);
    
    xmstr = GetDataSetListStrings(&nset);
    
    XtVaSetValues(listD,
                  XmNitemCount, nset,
                  XmNitems, xmstr,
                  XmNvisibleItemCount, (nset > 20 ? 20 : nset),
                  NULL);
    
    CleanupDataSetListStrings(xmstr, nset);
}

static void UpdateScanListDialog(Widget listD)
{
    int nset;
    XmString *xmstr;
    
    if (!listD) return;
    
    XmListDeselectAllItems(listD);
    
    xmstr = GetScanListStrings(vP->from, &nset);
    
    XtVaSetValues(listD,
                  XmNitemCount, nset,
                  XmNitems, xmstr,
                  XmNvisibleItemCount, (nset > 20 ? 20 : nset),
                  NULL);
    
    CleanupDataSetListStrings(xmstr, nset);
}

void manipulate_datasets(Widget wid, char *cmd, XtPointer call_data)
{
    Widget w = wid;
    Arg wargs[10], wargs2[10];
    XmString *xmstr;
    int n, nset, dual = 0, use_scans = 0;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    if (strncmp(cmd, "Scan", 4) == 0) {
        use_scans = 1;
        xmstr = GetScanListStrings(vP->from, &nset);
    } else {
        xmstr = GetDataSetListStrings(&nset);
        if (strncmp(cmd, "Subtract", 8) == 0) dual = 1;
        if (strncmp(cmd, "Sub+div", 7) == 0) dual = 1;
        if (strncmp(cmd, "RMSmerge", 8) == 0) dual = 1;
    }

    if (!xmstr) {
        PostErrorDialog(w, use_scans ? "There are no scans in this data set." :
                        "There are no stored data sets.");
        return;
    }

    n = 0;
    if (nset > 0) {
        XtSetArg(wargs2[n], XmNitemCount, nset);
        XtSetArg(wargs[n], XmNitemCount, nset); n++;
        XtSetArg(wargs2[n], XmNitems, xmstr);
        XtSetArg(wargs[n], XmNitems, xmstr); n++;
        if (nset <= 20) {
            XtSetArg(wargs2[n], XmNvisibleItemCount, nset);
            XtSetArg(wargs[n], XmNvisibleItemCount, nset); n++;
        } else {
            XtSetArg(wargs2[n], XmNvisibleItemCount, 20);
            XtSetArg(wargs[n], XmNvisibleItemCount, 20); n++;
        }
    }
    XtSetArg(wargs2[n], XmNfontList, use_scans ? gp->flist10 : gp->flist12);
    XtSetArg(wargs[n], XmNfontList, use_scans ? gp->flist10 : gp->flist12); n++;

    if (use_scans) {
        XtSetArg(wargs[n], XmNselectionPolicy, XmEXTENDED_SELECT); n++;
    } else {
        if (dual) {
            XtSetArg(wargs2[n], XmNselectionPolicy, XmSINGLE_SELECT);
            XtSetArg(wargs[n], XmNselectionPolicy, XmEXTENDED_SELECT); n++;
        } else {
            XtSetArg(wargs2[n], XmNselectionPolicy, XmEXTENDED_SELECT);
            XtSetArg(wargs[n], XmNselectionPolicy, XmEXTENDED_SELECT); n++;
        }
    }
    
    if (use_scans)
        create_scanlist_dialog(w, cmd, wargs, n);
    else if (dual)
        create_dualdatasetlist_dialog(w, cmd, wargs, wargs2, n);
    else
        create_datasetlist_dialog(w, cmd, wargs, n);
    
    CleanupDataSetListStrings(xmstr, nset);
}
