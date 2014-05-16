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

#include <Xm/SelectioB.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/Frame.h>

#include "list.h"
#include "defines.h"
#include "global_structs.h"
#include "dialogs.h"

#define OP_ADD 0
#define OP_SUB 1
#define OP_MUL 2
#define OP_DIV 3
#define OP_AVE 4
#define OP_CHI 5

#define NUMOPS 6
#define SCATTER_SIZE (4*sizeof(double) + sizeof(scanPtr) + sizeof(int))

/* External declarations */
extern VIEW   *vP;
extern DRAW    draw;
extern PSDATA  ps;
extern GLOBAL *gp;

void PostErrorDialog(Widget, char *);
void PostWarningDialog(Widget, char *);
void PostMessageDialog(Widget, char *);
int  PostQuestionDialog(Widget, char *);
void ManageDialogCenteredOnPointer(Widget);

list       scan_iterator(list, DataSetPtr);
scanPtr    copy_scan(DataSetPtr, scanPtr);
list      *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);
void       DeleteLastDataSet();

/* Local declarations */
typedef struct {
    scatter **s;
    Widget w;
    StdForm *sf;
} scatDualWidget;

typedef struct {
    int n;
    StdForm *sf;
} intWidget;

typedef struct {
    scatDualWidget mwL, mwR;
    intWidget iw[NUMOPS];
    scatter *p1, *p2;
    int op;
} ScatAriStruct;

typedef struct {
    int   nL;
    list *pL;
} SubList;

typedef struct {
    SubList sl;
    Widget listD;
    int nBins;
    double x1, x2, dx;
    double y1, y2;
    double tr[6];
} BinData;

list scatterlist;

unsigned long int scatter_bytes = 0;

static char *sop_string[] = {"+", "-", "*", "/", "ave", "chi"};
static int   sop_types[]  = {OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_AVE, OP_CHI};

static char wrongSizeForm[] = {
  "The plots '%s' and '%s' have different dimensions: %d and %d.\n"
};

static char *bin_labels[] = {
  "First x-value",
  "Bin x-width",
  "No. of x-bins",
  "New name"
};

static char *merge_labels[] = {
  "Smallest x-value",
  "Largest x-value",
  "Smallest y-value",
  "Largest y-value",
  "New name"
};

static char *scale_labels[] = {
  "X = ", "x + ", "y + ",
  "Y = ", "x + ", "y + "
};

void init_scatterlist()
{
    status init_list();

    init_list(&scatterlist);
}

double GetScatterMemory()
{
    return (double)scatter_bytes/1024.0/1024.0;
}

list *get_scatterlist()
{
    return &scatterlist;
}

int count_scatters()
{
    int n = 0;
    list curr = NULL;

    list list_iterator();
    bool empty_list();

    if (empty_list(scatterlist) == tRUE)
        return 0;

    while ((curr = list_iterator(scatterlist, curr)) != NULL)
        n++;

    return n;
}

scatter *new_scatter(list *pL, int nData)
{
    scatter *pS = NULL;

    scatter *cleanup_scatter();
    status   insert(list *, generic_ptr);

    pS = (scatter *)malloc(sizeof(scatter));
    if (!pS)
        return cleanup_scatter(pS);

    pS->x  = NULL;
    pS->y  = NULL;
    pS->ex = NULL;
    pS->ey = NULL;
    pS->sp = NULL;
    pS->t  = NULL;
    pS->nData = nData;

    pS->x = (double *)malloc(nData * sizeof(double));
    if (!pS->x)
        return cleanup_scatter(pS);

    pS->y = (double *)malloc(nData * sizeof(double));
    if (!pS->y)
        return cleanup_scatter(pS);

    pS->ex = (double *)malloc(nData * sizeof(double));
    if (!pS->ex)
        return cleanup_scatter(pS);

    pS->ey = (double *)malloc(nData * sizeof(double));
    if (!pS->ey)
        return cleanup_scatter(pS);

    pS->sp = (scanPtr *)calloc(nData, sizeof(scanPtr));
    if (!pS->sp)
        return cleanup_scatter(pS);

    pS->t = (int *)malloc(nData * sizeof(int));
    if (!pS->t)
        return cleanup_scatter(pS);

    if (insert(pL, (generic_ptr)pS) == Error)
        return cleanup_scatter(pS);

    scatter_bytes += nData * SCATTER_SIZE + sizeof(scatter);

    return pS;
}

scatter *cleanup_scatter(scatter *pS)
{
    if (pS) {
        if (pS->x) free(pS->x);
        if (pS->y) free(pS->y);
        if (pS->ex) free(pS->ex);
        if (pS->ey) free(pS->ey);
        if (pS->sp) free(pS->sp);
        if (pS->t) free(pS->t);
       free(pS);
    }
    return NULL;
}

list *delete_scatter(list *pL, list node)
{
    unsigned long int n = 0;
    scatter *pS = NULL;

    status delete_node();
    bool empty_list();
    scatter *cleanup_scatter();

    if (empty_list(*pL) == tRUE || !node)
        return NULL;

    pS = (scatter *)DATA(node);
    
    if (pS) {
        n = pS->nData * SCATTER_SIZE + sizeof(scatter);
        cleanup_scatter(pS);
    }

    if (delete_node(pL, node) == Error)
        return NULL;

    if (n < scatter_bytes)
        scatter_bytes -= n;
    else
        scatter_bytes = 0;

    return pL;
}

scatter *copy_scatter(list *pL, scatter *old)
{
    int n, *t;
    double *x, *y, *ex, *ey;
    scanPtr *s;
    scatter *new = NULL;
    
    if (!old)
        return new;
    
    new = new_scatter(pL, old->nData);
    if (!new)
        return new;    
    
    x  = new->x;
    y  = new->y;
    ex = new->ex;
    ey = new->ey;
    s  = new->sp;
    t  = new->t;
    
    *new = *old;
    
    new->x  = x;
    new->y  = y;
    new->ex = ex;
    new->ey = ey;
    new->sp = s;
    new->t  = t;

    for (n=0; n<old->nData; n++) {
        x[n]  = old->x[n];
        y[n]  = old->y[n];
        ex[n] = old->ex[n];
        ey[n] = old->ey[n];
        s[n]  = old->sp[n];
        t[n]  = old->t[n];
    }
    
    return new;
}

scatter *resize_scatter(list *pL, list node, int newNData)
{
    unsigned long int n = 0;
    scatter *pS = NULL;
    
    bool empty_list();
    list *delete_scatter();
    
    if (empty_list(*pL) == tRUE || !node)
        return pS;
    
    pS = (scatter *)DATA(node);
    
    if (!pS)
        return pS;
        
    if (pS->nData == newNData)
        return pS;
        
     pS->x = realloc(pS->x, newNData * sizeof(double));
     if (!pS->x) {
         delete_scatter(pL, node);
         return NULL;
     }

     pS->y = realloc(pS->y, newNData * sizeof(double));
     if (!pS->y) {
         delete_scatter(pL, node);
         return NULL;
     }
        
     pS->ex = realloc(pS->ex, newNData * sizeof(double));
     if (!pS->ex) {
         delete_scatter(pL, node);
         return NULL;
     }

     pS->ey = realloc(pS->ey, newNData * sizeof(double));
     if (!pS->ey) {
         delete_scatter(pL, node);
         return NULL;
     }

     pS->sp = realloc(pS->sp, newNData * sizeof(scanPtr));
     if (!pS->sp) {
         delete_scatter(pL, node);
         return NULL;
     }

     pS->t = realloc(pS->t, newNData * sizeof(int));
     if (!pS->t) {
         delete_scatter(pL, node);
         return NULL;
     }
     
     n = (newNData - pS->nData) * SCATTER_SIZE;
     scatter_bytes += n;
     
     pS->nData = newNData;
     
     return pS;
}

void destroy_scatterlist()
{
    list curr = scatterlist;

    while (delete_scatter(&curr, curr) != NULL)
        ;
}

list scatter_iterator(list last)
{
    return (last == NULL) ? scatterlist : NEXT(last);
}

void get_scatter_name(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    string buf;
    scatter *pNew, *pOld = (scatter *)sf->any;

    void send_line(), wsscanf();
    scatter *copy_scatter();

    if ((pNew = copy_scatter(&scatterlist, pOld)) == NULL) {
        PostErrorDialog(w, "Couldn't allocate the new scatter plot!");
        return;
    }

    wsscanf(sf->edit[0], pNew->name);

    sprintf(buf, "Stored new scatter plot as %s.\n", pNew->name);
    send_line(buf);
    
    XtDestroyWidget(sf->form);
}

void StoreCurrentScatter(Widget w, char *cmd, XtPointer call_data)
{
    scatter *pOld = NULL;
    Widget rc;
    StdForm *sf;
    string title;

    void wprintf();

    while (!XtIsWMShell(w))
        w = XtParent(w);

    if (vP->mode == SHOW_SCATTER)
        pOld = vP->p;

    if (!pOld) {
        PostErrorDialog(w, "There is no current scatter plot to store!");
        return;
    }

    sprintf(title, "Store scatter plot '%s'", pOld->name);
    sf = PostStdFormDialog(w, title,
             BUTT_APPLY, (XtCallbackProc)get_scatter_name, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 1, NULL);
    sf->any = pOld;
    
    rc = XtVaCreateManagedWidget("rc", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    XtCreateManagedWidget("Store scatter plot as:", xmLabelWidgetClass,
                          rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass, rc,
                                        NULL, 0);
    
    ArrangeStdFormDialog(sf, rc);

    wprintf(sf->edit[0], "%s",  pOld->name);
    
    ManageDialogCenteredOnPointer(sf->form);
}

void ScatterDraw(Widget w, scatter *p, XmListCallbackStruct *cb)
{
    void set_scatter_minmax(), SetViewMode();
    void SetStdView(), draw_main(), SetDefWindow();

    set_scatter_minmax(p);

    SetViewMode(SHOW_SCATTER, p->s, p->m1, p);

    strcpy(vP->t_label, p->name);
    strcpy(vP->x_label, "x-axis");
    strcpy(vP->y_label, "y-axis");

    SetStdView();
    SetDefWindow(SCALE_BOTH);
    
    vP->nScat = 0;
    vP->P = NULL;
    
    draw_main();
}

static char *ScatterListing(scatter *s, int i)
{
    static string txt;

    if (!s)
        return NULL;

    if (s->swapped)
        sprintf(txt, "%4d Swapped %s(%d)", i+1, s->name, s->nData);
    else 
        sprintf(txt, "%4d %s(%d)", i+1, s->name, s->nData);

    return txt;
}

static XmString *GetScatterListStrings(int *nscats)
{
    int n, ns;
    XmString *xmstr;
    list curr = NULL;
    string buf;
        
    ns = count_scatters();
    if (nscats) *nscats = ns;
    
    if (ns == 0) return NULL;
    
    xmstr = (XmString *) XtMalloc(ns * sizeof(XmString));
    if (!xmstr) return NULL;
    
    n = 0;
    while ( (curr = scatter_iterator(curr)) != NULL) {
        strcpy(buf, ScatterListing((scatter *)DATA(curr), n));
        xmstr[n] = MKSTRING(buf);
        n++;
    }
    
    return xmstr;
}

static void CleanupScatterListStrings(XmString *xmstr, int nscats)
{
    int n;
    
    if (xmstr) {
        n = nscats;
        while (n > 0) XmStringFree(xmstr[--n]);
        
        XtFree((char *)xmstr);
    }
}

void UpdateScatterListDialog(Widget listD)
{
    int nscats;
    XmString *xmstr;
    
    if (!listD) return;
    
    xmstr = GetScatterListStrings(&nscats);
    
    XmListDeselectAllItems(listD);
    XtVaSetValues(listD,
                  XmNitemCount, nscats,
                  XmNitems, xmstr,
                  XmNvisibleItemCount, (nscats > 10 ? 10 : nscats),
                  NULL);
    
    CleanupScatterListStrings(xmstr, nscats);
}

void ZeroScanInScatterList(scanPtr s)
{
    int n;
    scatter *p;
    list curr = NULL;
    
    while ( (curr = scatter_iterator(curr)) != NULL) {
        p = (scatter *)DATA(curr);
        for (n=0; n<p->nData; n++) {
            if (p->sp[n] == s) {
                p->sp[n] = NULL;
            }
        }
    }
}

void manipulate_scats(Widget wid, char *cmd, XtPointer call_data)
{
    Widget w = wid;
    Arg wargs[10], wargs2[10];
    XmString *xmstr;
    int n, nscats;

    void create_scatsarit_dialog();
    void create_scatslist_dialog();
    void create_scatsbin_dialog();
    void create_scatsmerge_dialog();
    void create_scatsscale_dialog();

    while (!XtIsWMShell(w))
        w = XtParent(w);

    xmstr = GetScatterListStrings(&nscats);

    if (!xmstr) {
        PostErrorDialog(w, "There are no stored scatter plots.");
        return;
    }

    n = 0;
    if (nscats > 0) {
        XtSetArg(wargs2[n], XmNitemCount, nscats);
        XtSetArg(wargs[n], XmNitemCount, nscats); n++;
        XtSetArg(wargs2[n], XmNitems, xmstr);
        XtSetArg(wargs[n], XmNitems, xmstr); n++;
        if (nscats <= 10) {
            XtSetArg(wargs2[n], XmNvisibleItemCount, nscats);
            XtSetArg(wargs[n], XmNvisibleItemCount, nscats); n++;
        } else {
            XtSetArg(wargs2[n], XmNvisibleItemCount, 10);
            XtSetArg(wargs[n], XmNvisibleItemCount, 10); n++;
        }
    }
    XtSetArg(wargs2[n], XmNfontList, gp->flist12);
    XtSetArg(wargs[n], XmNfontList, gp->flist12); n++;

    if (strcmp(cmd, "Arithmetic") == 0) {
        XtSetArg(wargs2[n], XmNselectionPolicy, XmSINGLE_SELECT);
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        create_scatsarit_dialog(w, wargs, n, wargs2, n);
    } else {
        XtSetArg(wargs[n], XmNselectionPolicy, XmEXTENDED_SELECT); n++;
        if (strcmp(cmd, "Bin") == 0)
            create_scatsbin_dialog(w, cmd, wargs, n);
        else if (strcmp(cmd, "Merge") == 0)
            create_scatsmerge_dialog(w, cmd, wargs, n);
        else if (strcmp(cmd, "Scale") == 0)
            create_scatsscale_dialog(w, cmd, wargs, n);
        else
            create_scatslist_dialog(w, cmd, wargs, n);
    }
    
    CleanupScatterListStrings(xmstr, nscats);
}

static void ScatterDelete(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, i=0, nL;
    string buf;
    list *pL;

    list *get_action_list();
    void send_line();
    list *delete_scatter();

    if ((pL = get_action_list(cb, &nL, scatterlist)) == NULL)
        return;

    sprintf(buf, "Delete the %d selected scatter plots?", nL);
    
    if (PostQuestionDialog(w, buf)) {
        for (n=0; n<nL; n++) {
            if (vP->p && vP->p == (scatter *)DATA(pL[n])) {
                sprintf(buf,
               "Removed the scatter plot '%s' that was used in current view.", 
                        vP->p->name);
                PostWarningDialog(NULL, buf);
                vP->p = NULL;
            }
            if (!delete_scatter(&scatterlist, pL[n])) {
                sprintf(buf, "Couldn't delete scatter plot %d.\n", i+1);
                PostWarningDialog(NULL, buf);
            } else {
                i++;
            }
        }
    }

    free(pL);

    UpdateScatterListDialog(sf->edit[0]);

    if (i == nL)
        sprintf(buf, "Deleted all %d selected scatter plots.\n", nL);
    else if (i > 0)
        sprintf(buf,
                "Deleted only %d out of %d selected scatter plots.\n",
                i, nL);
    if (i > 0) send_line(buf);
}

#define DSWAP(x, y)  {double t = x; x = y; y = t;}
#define SSWAP(x, y)  {scanPtr t = x; x = y; y = t;}
#define ISWAP(x, y)  {int t = x; x = y; y = t;}

static void DoSortScatter(scatter *s)
{
    int i, j, n;
    
    if (!s) return;
    
    n = s->nData;
    
    for (i=0; i<n-1; i++) {
        for (j=i+1; j<n; j++) {
            if (s->x[i] > s->x[j]) {
                DSWAP(s->x[i],  s->x[j])
                DSWAP(s->y[i],  s->y[j])
                DSWAP(s->ex[i], s->ex[j])
                DSWAP(s->ey[i], s->ey[j])
                SSWAP(s->sp[i], s->sp[j])
                ISWAP(s->t[i],  s->t[j])
            }
        }
    }
}

static void DoDerive(scatter *old, scatter *new)
{
    int n;
    double x, dx, dy, eu, el;
    
    void set_scatter_minmax(scatter *);
    
    strcpy(new->molecule, old->molecule);
    new->date = old->date;
    new->swapped = old->swapped;
    new->x0 = old->x0;
    new->y0 = old->y0;
    new->epoch = old->epoch;
    new->equinox = old->equinox;
    new->m1 = old->m1;
    new->m2 = old->m2;
    new->s  = old->s;
    new->xtype = old->xtype;
    new->ytype = old->ytype;
    
    for (n=0; n<new->nData; n++) {
        x = (old->x[n] + old->x[n+1])/2.0;
        dx = (old->x[n+1] - old->x[n]);
        dy = (old->y[n+1] - old->y[n]);
        new->x[n] = x;
        eu = old->ey[n+1] * old->ey[n+1] + old->ey[n] * old->ey[n];
        el = old->ex[n+1] * old->ex[n+1] + old->ex[n] * old->ex[n];
        new->ex[n] = el/2.0;
        if (dx != 0.0) {
            new->y[n] = dy/dx;
            new->ey[n] = sqrt(eu + dy*dy*el/dx/dx)/dx;
        } else {
            new->y[n] = 0.0;
            new->ey[n] = (old->ey[n] + old->ey[n+1])/2.0;
        }
    }
    
    set_scatter_minmax(new);
}

static void DoYInvert(scatter *old, scatter *new)
{
    int n;
    double y, dy;
    
    void set_scatter_minmax(scatter *);
    
    strcpy(new->molecule, old->molecule);
    new->date = old->date;
    new->swapped = old->swapped;
    new->x0 = old->x0;
    new->y0 = old->y0;
    new->epoch = old->epoch;
    new->equinox = old->equinox;
    new->m1 = old->m1;
    new->m2 = old->m2;
    new->s  = old->s;
    new->xtype = old->xtype;
    new->ytype = old->ytype;
    
    for (n=0; n<new->nData; n++) {
        new->x[n] = old->x[n];
	new->ex[n] = old->ex[n];
        y = old->y[n];
	dy = old->ey[n];
        if (y != 0.0) {
            new->y[n] = 1.0/y;
            new->ey[n] = dy/y/y;
        } else {
            new->y[n] = 0.0;
            new->ey[n] = 0.0;
        }
    }
    
    set_scatter_minmax(new);
}

static void ScatterYInvert(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, i=0, nL;
    string buf;
    list *pL;
    scatter *s, *old;

    list *get_action_list();
    void send_line();
    scatter *copy_scatter();

    if ((pL = get_action_list(cb, &nL, scatterlist)) == NULL)
        return;

    sprintf(buf, "Invert y-values in the %d selected scatter plots?", nL);
    
    if (PostQuestionDialog(w, buf)) {
        for (n=0; n<nL; n++) {
            old = (scatter *)DATA(pL[n]);
            s = copy_scatter(&scatterlist, old);
            if (!s) continue;
            DoYInvert(old, s);
            sprintf(s->name, "%s %s", "1/y", old->name);
            i++;
        }
    }

    free(pL);

    UpdateScatterListDialog(sf->edit[0]);

    if (i == nL)
        sprintf(buf, "Inverting y-values in %d selected scatter plots.\n",
                nL);
    else if (i > 0)
        sprintf(buf,
            "Inverting y-values in %d out of %d selected scatter plots.\n",
                i, nL);

    if (i > 0) send_line(buf);
}

static void ScatterDerivative(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, i=0, nL;
    string buf;
    list *pL;
    scatter *s, *old;

    list *get_action_list();
    void send_line();
    scatter *copy_scatter();

    if ((pL = get_action_list(cb, &nL, scatterlist)) == NULL)
        return;

    sprintf(buf, "Derive the %d selected scatter plots?", nL);
    
    if (PostQuestionDialog(w, buf)) {
        for (n=0; n<nL; n++) {
            old = (scatter *)DATA(pL[n]);
            DoSortScatter(old);
            s = new_scatter(&scatterlist, old->nData-1);
            if (!s) continue;
            DoDerive(old, s);
            sprintf(s->name, "%s %s", "d/dx", old->name);
            i++;
        }
    }

    free(pL);

    UpdateScatterListDialog(sf->edit[0]);

    if (i == nL)
        sprintf(buf, "Taking the derivative all %d selected scatter plots.\n",
                nL);
    else if (i > 0)
        sprintf(buf,
            "Taking the derivative of %d out of %d selected scatter plots.\n",
                i, nL);

    if (i > 0) send_line(buf);
}

static void GetCurrentPointingModel(double A, double E,
                                    double *az_corr, double *el_corr)
{
    double c[18];
    
    double AzPoi2DValue(double x, double y, double *a, int nPar);
    double ElPoi2DValue(double x, double y, double *a, int nPar);
    
    c[0]  = +1487.6000;       /* IA    */
    c[1]  =  +591.8339;       /* IE    */
    c[2]  =   -17.0000;       /* NPAE  */
    c[3]  =    -9.5430;       /* CA    */
    c[4]  =   +14.0000;       /* AN    */
    c[5]  =   -16.6000;       /* AW    */
    c[6]  =   -81.2000;       /* HECE  */
    c[7]  =    -1.8000;       /* HASA  */
    c[8]  =    -0.5000;       /* HACA  */
    c[9]  =    +4.3000;       /* HESE  */
    c[10] =    +0.9000;       /* HESA  */
    c[11] =    -1.2964;       /* HASA2 */
    c[12] =    +4.2444;       /* HACA2 */
    c[13] =    +1.0000;       /* HESA2 */
    c[14] =    -4.0000;       /* HECA2 */
    c[15] =    +1.1604;       /* HECA3 */
    c[16] =    +6.9964;       /* HACA3 */
    c[17] =    +0.0000;       /* HESA3 */
    
    if (az_corr) *az_corr = AzPoi2DValue(A, E, c, 18);
    if (el_corr) *el_corr = ElPoi2DValue(A, E, c, 18);
}

static void DoPointingResiduals(scatter *s, scatter *s1, scatter *s2,
                                scatter *s3, scatter *s4, scatter *s5)
{
    int n;
    double A, dx, dy, px, py;
    
    void set_scatter_minmax(scatter *);
    
    for (n=0; n<s->nData; n++) {
        A = s->x[n];
	if (A < 0.0) A += 360.0;
	if (A > 360.0) A -= 360.0;
        dx = s->x[n] - s->ex[n];
        dx *= cos(s->y[n]/RADTODEG);
        dy = s->y[n] - s->ey[n];
	
        GetCurrentPointingModel(s->x[n], s->y[n], &px, &py);
	dx -= px/3600.0;
	dy -= py/3600.0;
	
	s1->x[n] = A;
	s1->y[n] = dx*3600.0;
	s1->xtype = XTYPE_SCA_AZ; s1->ytype = YTYPE_SCA_AZOFF;
	s2->x[n] = s->y[n];
	s2->y[n] = dx*3600.0;
	s2->xtype = XTYPE_SCA_EL; s2->ytype = YTYPE_SCA_AZOFF;
	s3->x[n] = A;
	s3->y[n] = dy*3600.0;
	s3->xtype = XTYPE_SCA_AZ; s3->ytype = YTYPE_SCA_ELOFF;
	s4->x[n] = s->y[n];
	s4->y[n] = dy*3600.0;
	s4->xtype = XTYPE_SCA_EL; s4->ytype = YTYPE_SCA_ELOFF;
	s5->ex[n] = A;
	s5->ey[n] = s->y[n];
	s5->x[n] = dx*3600.0;
	s5->y[n] = dy*3600.0;
	s5->xtype = XTYPE_SCA_AZOFF; s5->ytype = YTYPE_SCA_ELOFF;
    }
    
    set_scatter_minmax(s1);
    set_scatter_minmax(s2);
    set_scatter_minmax(s3);
    set_scatter_minmax(s4);
    set_scatter_minmax(s5);
}

static void ScatterPointingResiduals(Widget w, StdForm *sf,
                                    XmListCallbackStruct *cb)
{
    int n, i=0, nL;
    string buf;
    list *pL;
    scatter *s1, *s2, *s3, *s4, *s5, *s;

    list *get_action_list();
    void send_line();
    scatter *copy_scatter();

    if ((pL = get_action_list(cb, &nL, scatterlist)) == NULL)
        return;

    sprintf(buf, "Obtain pointing residuals of the %d selected scatter plots?", nL);
    
    if (PostQuestionDialog(w, buf)) {
        for (n=0; n<nL; n++) {
            s = (scatter *)DATA(pL[n]);
            s1 = copy_scatter(&scatterlist, s);
            s2 = copy_scatter(&scatterlist, s);
            s3 = copy_scatter(&scatterlist, s);
            s4 = copy_scatter(&scatterlist, s);
            s5 = copy_scatter(&scatterlist, s);
            if (!s1 || !s2 || !s3 || !s4 || !s5) continue;
            DoPointingResiduals(s, s1, s2, s3, s4, s5);
            sprintf(s1->name, "%s %s", s->name, "\\gdA(A)");
            sprintf(s2->name, "%s %s", s->name, "\\gdA(E)");
            sprintf(s3->name, "%s %s", s->name, "\\gdE(A)");
            sprintf(s4->name, "%s %s", s->name, "\\gdE(E)");
            sprintf(s5->name, "%s %s", s->name, "residuals");
	    sprintf(s5->molecule, "residuals");
            i++;
        }
    }

    free(pL);

    UpdateScatterListDialog(sf->edit[0]);

    if (i == nL)
        sprintf(buf,
	  "Making poi. residuals of all %d selected scatter plots.\n", nL);
    else if (i > 0)
        sprintf(buf,
            "Making poi. residuals of %d out of %d selected scatter plots.\n",
                i, nL);

    if (i > 0) send_line(buf);
}

static void ScatterSort(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, i=0, nL;
    string buf;
    list *pL;
    scatter *s;

    list *get_action_list();
    void send_line();

    if ((pL = get_action_list(cb, &nL, scatterlist)) == NULL)
        return;

    sprintf(buf, "Sort the %d selected scatter plots?", nL);
    
    if (PostQuestionDialog(w, buf)) {
        for (n=0; n<nL; n++) {
            s = (scatter *)DATA(pL[n]);
            DoSortScatter(s);
            sprintf(buf, "%s %s", "Sorted", s->name);
            strcpy(s->name, buf);
            i++;
        }
    }
    
    free(pL);

    UpdateScatterListDialog(sf->edit[0]);

    if (i == nL)
        sprintf(buf, "Sorted all %d selected scatter plots.\n",
                nL);
    else if (i > 0)
        sprintf(buf,
            "Sorted of %d out of %d selected scatter plots.\n",
                i, nL);

    if (i > 0) send_line(buf);
}

static void ScatterCopy(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, i=0, nL;
    string buf;
    list *pL;
    scatter *s, *old;

    list *get_action_list();
    void send_line();
    scatter *copy_scatter();

    if ((pL = get_action_list(cb, &nL, scatterlist)) == NULL)
        return;

    sprintf(buf, "Copy the %d selected scatter plots?", nL);
    
    if (PostQuestionDialog(w, buf)) {
        for (n=0; n<nL; n++) {
            old = (scatter *)DATA(pL[n]);
            s = copy_scatter(&scatterlist, old);
            if (!s) continue;
            sprintf(s->name, "%s %s", "Copy of", old->name);
            i++;
        }
    }

    free(pL);

    UpdateScatterListDialog(sf->edit[0]);

    if (i == nL)
        sprintf(buf, "Copying all %d selected scatter plots.\n", nL);
    else if (i > 0)
        sprintf(buf,
                "Copying only %d out of %d selected scatter plots.\n",
                i, nL);

    if (i> 0) send_line(buf);
}

static void ScatterSwap(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n=0, nL, type;
    string buf;
    list *pL;
    scatter *s;
    double *tmp, ref;

    list *get_action_list();
    void send_line(), set_scatter_minmax(), draw_main();

    if ((pL = get_action_list(cb, &nL, scatterlist)) == NULL)
        return;

    sprintf(buf, "Swap x & y in the %d selected scatter plots?", nL);
    
    if (PostQuestionDialog(w, buf)) {
        for (n=0; n<nL; n++) {
            s = (scatter *)DATA(pL[n]);

            tmp = s->x;
            s->x = s->y;
            s->y = tmp;

            tmp = s->ex;
            s->ex = s->ey;
            s->ey = tmp;

            type = s->xtype;
            s->xtype = s->ytype;
            s->ytype = type;

            ref = s->x0;
            s->x0 = s->y0;
            s->y0 = ref;

            if (s->swapped)
                s->swapped = 0;
            else
                s->swapped = 1;

            set_scatter_minmax(s);
        }
        n = nL;
    }

    free(pL);

    UpdateScatterListDialog(sf->edit[0]);

    if (n > 0) {
        if (vP->mode == SHOW_SCATTER) {
            draw_main();
        }

        sprintf(buf, "Swapping x and y in %d selected scatter plot(s).\n", nL);
        send_line(buf);
    }
}

static void get_new_scatter_name(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    string buf;
    scatter *s = (scatter *)sf->any;

    void send_line(), wsscanf();

    wsscanf(sf->edit[0], s->name);

    sprintf(buf, "Scatter plot renamed as %s.\n", s->name);
    send_line(buf);
    
    XtDestroyWidget(sf->form);
}

static void PostScatterRenameDialog(Widget w, scatter *s)
{
    Widget rc;
    StdForm *sf;
    string title;

    void send_line(), wprintf();

    if (!s) {
        PostErrorDialog(w, "There is no such scatter plot to rename!");
        return;
    }

    sprintf(title, "Rename scatter plot '%s'", s->name);
    sf = PostStdFormDialog(w, "Rename scatter plot",
             BUTT_APPLY, (XtCallbackProc)get_new_scatter_name, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 1, NULL);
    sf->any = (XtPointer)s;
    
    rc = XtVaCreateManagedWidget("rc", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    XtCreateManagedWidget(title, xmLabelWidgetClass,
                          rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);

    ArrangeStdFormDialog(sf, rc);

    wprintf(sf->edit[0], "%s", s->name);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void ScatterRename(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    list *pL;

    list *get_action_list();

    if ((pL = get_action_list(cb, &nL, scatterlist)) == NULL)
        return;

    for (n=0; n<nL; n++)
        PostScatterRenameDialog(gp->top, (scatter *)DATA(pL[n]));

    free(pL);

    XtDestroyWidget(sf->form);
}

static scatter *DoScatterFFT(char how, scatter *old)
{
    scatter *new;
    
    scatter *fft_scatter(char *, scatter *);
    
    if (how == 'f' || how == 'F') {
        new = fft_scatter("p", old);
    } else {
        new = fft_scatter("i", old);
    }
    
    return new;
}

static void ScatterFFT(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, i=0, nL;
    string buf;
    list *pL;
    scatter *o, *s;

    list *get_action_list();
    void send_line();

    if ((pL = get_action_list(cb, &nL, scatterlist)) == NULL)
        return;

    for (n=0; n<nL; n++) {
        o = (scatter *)DATA(pL[n]);
        s = DoScatterFFT('f', o);
        if (s) {
            i++;
            sprintf(s->name, "Forward FFT of %s", o->name);
        } else
            break;
    }
    free(pL);
    
    if (i >0) {
        sprintf(buf, "Calculated forward FFT of %d scatter plots.", i);
        send_line(buf);
    }
}

static void ScatterIFFT(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, i=0, nL;
    string buf;
    list *pL;
    scatter *o, *s;

    list *get_action_list();
    void send_line();

    if ((pL = get_action_list(cb, &nL, scatterlist)) == NULL)
        return;

    for (n=0; n<nL; n++) {
        o = (scatter *)DATA(pL[n]);
        s = DoScatterFFT('i', o);
        if (s) {
            i++;
            sprintf(s->name, "Inverse FFT of %s", o->name);
        } else
            break;
    }
    free(pL);
    
    if (i >0) {
        sprintf(buf, "Calculated inverse FFT of %d scatter plots.", i);
        send_line(buf);
    }
}

void ScatterShow(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    list *pL;
    scatter *p;
    static scatter **scats = NULL;

    list *get_action_list();
    void draw_main(), ScatterDraw(), SetViewMode(), SetStdView();
    void SetDefWindow();

    if ((pL = get_action_list(cb, &nL, scatterlist)) == NULL)
        return;

    if (nL == 1) {
        ScatterDraw(w, (scatter *)DATA(pL[0]), cb);
        if (scats) free(scats);
        scats = NULL;
        free(pL);
        return;
    }
    
    if (scats)
        scats = (scatter **)realloc(scats, nL * sizeof(scatter *));
    else
        scats = (scatter **)malloc(nL * sizeof(scatter *));

    for (n=0; n<nL; n++) scats[n] = (scatter *)DATA(pL[n]);
    
    vP->nScat = nL;
    vP->P = scats;
    
    free(pL);
    
    p = vP->P[0];
    
    SetViewMode(SHOW_SCATTER, p->s, p->m1, p);

    strcpy(vP->t_label, p->name);
    strcpy(vP->x_label, "x-axis");
    strcpy(vP->y_label, "y-axis");

    SetStdView();
    SetDefWindow(SCALE_BOTH);
    
    draw_main();
}

void create_scatslist_dialog(Widget parent, char *cmd, Arg *args, int nargs)
{
    Widget rc;
    string title;
    void (*cb_func)();
    StdForm *sf;
    
    if (strcmp(cmd, "Delete") == 0) {
        sprintf(title, "Delete scatter plot(s)");
        cb_func = ScatterDelete;
    } else if (strcmp(cmd, "Rename") == 0) {
        sprintf(title, "Rename scatter plot(s)");
        cb_func = ScatterRename;
    } else if (strcmp(cmd, "Copy") == 0) {
        sprintf(title, "Copy scatter plot(s)");
        cb_func = ScatterCopy;
    } else if (strcmp(cmd, "Swap") == 0) {
        sprintf(title, "Swap x & y in scatter plot(s)");
        cb_func = ScatterSwap;
    } else if (strcmp(cmd, "Sort") == 0) {
        sprintf(title, "Sort points in scatter plot(s)");
        cb_func = ScatterSort;
    } else if (strcmp(cmd, "d/dx") == 0) {
        sprintf(title, "Derive scatter plot(s)");
        cb_func = ScatterDerivative;
    } else if (strcmp(cmd, "InvertY") == 0) {
        sprintf(title, "Invert y-scale in plot(s)");
        cb_func = ScatterYInvert;
    } else if (strcmp(cmd, "FFT") == 0) {
        sprintf(title, "Forward FFT of scatter plot(s)");
        cb_func = ScatterFFT;
    } else if (strcmp(cmd, "iFFT") == 0) {
        sprintf(title, "Inverse FFT of scatter plot(s)");
        cb_func = ScatterIFFT;
    } else if (strcmp(cmd, "poires") == 0) {
        sprintf(title, "Pointing residulas scatter plot(s)");
        cb_func = ScatterPointingResiduals;
    } else {
        sprintf(title, "Show scatter plot(s)");
        cb_func = ScatterShow;
    }

    sf = PostStdFormDialog(parent, title,
             NULL, NULL, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 1, NULL);

    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    XtCreateManagedWidget(title, xmLabelWidgetClass, rc, NULL, 0);

    sf->edit[0] = XmCreateScrolledList(rc, "scatslist", args, nargs);
    XtManageChild(sf->edit[0]);

    XtAddCallback(sf->edit[0], XmNextendedSelectionCallback,
                  (XtCallbackProc)cb_func, sf);
    
    ArrangeStdFormDialog(sf, rc);

    ManageDialogCenteredOnPointer(sf->form);
}

static char *get_scatarith_name(ScatAriStruct *sas)
{
    string op;
    static string buf;

    strcpy(op, sop_string[sas->op]);

    if (sas->p1 && sas->p2) {
        sprintf(buf, "[%s] %s [%s]", sas->p1->name, op, sas->p2->name);
    } else if (sas->p1) {
        sprintf(buf, "[%s] %s <none>", sas->p1->name, op);
    } else if (sas->p2) {
        sprintf(buf, "<none> %s [%s]", op, sas->p2->name);
    } else {
        sprintf(buf, "<none> %s <none>", op);
    }

    return buf;
}

void fill_scatter(scatter *s, scatter *old)
{
    int n, *t;
    double *x, *y, *ex, *ey;
    scanPtr *p;
    
    x  = s->x;
    y  = s->y;
    ex = s->ex;
    ey = s->ey;
    p  = s->sp;
    t  = s->t;
    n  = s->nData;

    *s = *old;

    s->x  = x;
    s->y  = y;
    s->ex = ex;
    s->ey = ey;
    s->sp = p;
    s->t  = t;
    s->nData = n;
}

static void set_type_of_scatter_op(Widget w, intWidget *iw, XmAnyCallbackStruct *cb)
{
    ScatAriStruct *sas = (ScatAriStruct *)(iw->sf->user);
    
    void wprintf();

    sas->op = iw->n;
    wprintf(iw->sf->edit[0], "%s", get_scatarith_name(sas));
}

static void ScatterGet(Widget w, scatDualWidget *sdw, XmListCallbackStruct *cb)
{
    int      n, nL;
    list    *pL;
    scatter *p = NULL;
    ScatAriStruct *sas = (sdw->sf) ? (ScatAriStruct *)(sdw->sf->user) : NULL;

    list *get_action_list();
    void  wprintf();

    if ((pL = get_action_list(cb, &nL, scatterlist)) == NULL)
        return;

    for (n=0; n<nL; n++)
        p = (scatter *)DATA(pL[n]);

    *(sdw->s) = p;

    if (sdw->w)
        wprintf(sdw->w, "%s", p->name);

    free(pL);
    if (sdw->sf)
        wprintf(sdw->sf->edit[0], "%s", get_scatarith_name(sas));
}

static void ApplyOpOnScatterPlots(scatter *s, ScatAriStruct *sas)
{
    int n;
    double *x, x1, x2;
    double *y, y1, y2;
    double *ex, ex1, ex2;
    double *ey, ey1, ey2;

    for (n=0; n<s->nData; n++) {
        x   = &(s->x[n]);
        y   = &(s->y[n]);
        ex  = &(s->ex[n]);
        ey  = &(s->ey[n]);
        x1  = sas->p1->x[n];
        x2  = sas->p2->x[n];
        y1  = sas->p1->y[n];
        y2  = sas->p2->y[n];
        ex1 = sas->p1->ex[n];
        ex2 = sas->p2->ex[n];
        ey1 = sas->p1->ey[n];
        ey2 = sas->p2->ey[n];
        switch (sas->op) {
            case OP_ADD:
                *y  = y1 + y2;
                *ey = sqrt(ey1*ey1 + ey2*ey2);
                break;
            case OP_SUB:
                *y  = y1 - y2;
                *ey = sqrt(ey1*ey1 + ey2*ey2);
                break;
            case OP_MUL:
                *y  = y1 * y2;
                *ey = sqrt(y2*y2*ey1*ey1 + y1*y1*ey2*ey2);
                break;
            case OP_DIV:
                if (y2 != 0.0) {
                    *y  = y1 / y2;
                    *ey = sqrt(y2*y2*ey1*ey1 + y1*y1*ey2*ey2)/y2/y2;
                } else {
                    *y  = 0.0;
                    *ey = 0.0;
                }
                break;
            case OP_AVE:
                *y = (y1 + y2)/2.0;
                if (ey1 != 0.0 && ey2 != 0.0)
                    *ey = 1.0/sqrt(1.0/ey1/ey1 + 1.0/ey2/ey2);
                else
                    *ey = 0.0;
                break;
            case OP_CHI:
                if (ey1 != 0.0 && s->nData > 1) {
                    *y  = (y1 - y2)*(y1 - y2)/ey1/ey1;
                    *ey = *y/(double)(s->nData - 1);
                } else if (ey2 != 0.0 && s->nData > 1) {
                    *y  = (y1 - y2)*(y1 - y2)/ey2/ey2;
                    *ey = *y/(double)(s->nData - 1);
                } else {
                    *y  = 0.0;
                    *ey = 0.0;
                }
                break;
        }
        *x  = x1;
        *ex = ex1;
	s->sp[n] = NULL;
	s->t[n] = 0;
    }
    sas->p1 = NULL;
    sas->p2 = NULL;
}

static void DoScatterCalc(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    string buf;
    scatter *s;
    ScatAriStruct *sas = (ScatAriStruct *)(sf->user);

    void send_line(), wsscanf();
    void set_scatter_minmax();
    scatter *new_scatter();

    if (!sas->p1) {
        PostErrorDialog(w, "Cannot find a first scatter plot!");
        return;
    }
    if (!sas->p2) {
        PostErrorDialog(w, "Cannot find a second scatter plot!\n");
        return;
    }
    if (sas->p1->nData != sas->p2->nData) {
        sprintf(buf, wrongSizeForm, sas->p1->name, sas->p2->name,
                sas->p1->nData,sas->p2->nData);
        return;
    }
    
    /* DoSortScatter(sas->p1);
    DoSortScatter(sas->p2); */

    s = new_scatter(&scatterlist, sas->p1->nData);

    if (!s) {
        PostErrorDialog(w,
            "Couldn't allocate the space for the new scatter plot.");
        return;
    }

    fill_scatter(s, sas->p1);

    ApplyOpOnScatterPlots(s, sas);

    wsscanf(sf->edit[0], s->name);

    XtDestroyWidget(sf->form);
    
    set_scatter_minmax(s);

    sprintf(buf, "New scatter plot stored as '%s'\n", s->name);
    send_line(buf);
}

void create_scatsarit_dialog(Widget parent, Arg *argsLeft, int nArgsLeft,
                             Arg *argsRight, int nArgsRight)

{
    int n, i;
    Arg wargs[10];
    Widget rc, rcH, rcLeft, rcRight;
    Widget lList, rList;
    Widget radioBox, op[NUMOPS];
    Widget lLab, rLab;
    string lStr="<none>", rStr="<none>", newName;
    StdForm *sf;
    ScatAriStruct *sas;

    void wprintf();

    sas = (ScatAriStruct *)XtMalloc(sizeof(ScatAriStruct));
    if (!sas) {
        PostErrorDialog(parent, "Out of memory in scatter plot arithmetic.");
        return;
    }
    sas->op = OP_ADD;
    sas->p1 = sas->p2 = NULL;

    sprintf(newName, "%s %s %s", lStr, sop_string[sas->op], rStr);

    sf = PostStdFormDialog(parent, "Scatter Plot Arithmetic",
             BUTT_APPLY, (XtCallbackProc)DoScatterCalc, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             1, NULL);
    sf->user = (XtPointer)sas;

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    rcH = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                  XmNorientation, XmHORIZONTAL,
                                  XmNpacking, XmPACK_TIGHT,
                                  NULL);

    rcLeft = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rcH,
                                     XmNorientation, XmVERTICAL,
                                     NULL);

    lList = XmCreateScrolledList(rcLeft,  "leftlist",  argsLeft,  nArgsLeft);
    lLab = XtCreateManagedWidget(lStr, xmLabelWidgetClass, rcLeft,  NULL, 0);

    n = 0;
    XtSetArg(wargs[n], XmNentryClass, xmToggleButtonWidgetClass); n++;
    radioBox = XmCreateRadioBox(rcH, "radiobox", wargs, n);
    for (i=0; i<NUMOPS; i++) {
        if (sas->op == i)
            XtSetArg(wargs[0], XmNset, True);
	else
            XtSetArg(wargs[0], XmNset, False);
        op[i] = XtCreateWidget(sop_string[i], xmToggleButtonWidgetClass,
                               radioBox, wargs, 1);
        sas->iw[i].n = sop_types[i];
        sas->iw[i].sf = sf;
        XtAddCallback(op[i], XmNarmCallback,
                      (XtCallbackProc)set_type_of_scatter_op, &(sas->iw[i]));
    }

    rcRight = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rcH,
                                      XmNorientation, XmVERTICAL,
                                      NULL);

    rList = XmCreateScrolledList(rcRight, "rightlist", argsRight, nArgsRight);
    rLab = XtCreateManagedWidget(rStr, xmLabelWidgetClass, rcRight, NULL, 0);

    XtCreateManagedWidget("Name of new scatter plot:", xmLabelWidgetClass,
                          rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);

    sas->mwL.w  = lLab;
    sas->mwL.sf = sf;
    sas->mwL.s  = &(sas->p1);
    XtAddCallback(lList, XmNsingleSelectionCallback,
                  (XtCallbackProc)ScatterGet, &(sas->mwL));
    sas->mwR.w  = rLab;
    sas->mwR.sf = sf;
    sas->mwR.s  = &(sas->p2);
    XtAddCallback(rList, XmNsingleSelectionCallback,
                  (XtCallbackProc)ScatterGet, &(sas->mwR));
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(lList);
    XtManageChild(rList);
    XtManageChild(radioBox);
    XtManageChildren(op, NUMOPS);

    wprintf(sf->edit[0], "%s", newName);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void FreeBinData(BinData *b)
{
    if (b->sl.pL) free(b->sl.pL);
    b->sl.pL = NULL;
    b->sl.nL = 0;
}

static void ScatsBinCleanUp(XtPointer user)
{
    BinData *b = (BinData *)user;
    if (!b) return;
    
    FreeBinData(b);
}

static void DoBin(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int n, l, m, found, nn, nData, first=1;
    string buf, name;
    scatter *s, *new;
    double x, y, x1, x2, s1, s2;
    BinData *b = (BinData *)sf->user;
    
    void wsscanf(), wdscanf(), wiscanf();
    void send_line();
    DATE *XS_localtime();
    
    if (!(b->sl.pL) || b->sl.nL == 0) {
        PostErrorDialog(w, "There are no scatter plots selected.");
        return;
    }
    
    wdscanf(sf->edit[0], &(b->x1));
    wdscanf(sf->edit[1], &(b->dx));
    if (b->dx <= 0.0) {
        sprintf(buf, "Bin width (%f) cannot be <= 0.", b->dx);
        PostErrorDialog(w, buf);
        return;
    }
    wiscanf(sf->edit[2], &(b->nBins));
    if (b->nBins <= 0) {
        sprintf(buf, "No. of bins (%d) cannot be <= 0.", b->nBins);
        PostErrorDialog(w, buf);
        return;
    }
    wsscanf(sf->edit[3], name);
    
    nData = 0;
    for (n=0; n<b->nBins; n++) {
        x1 = b->x1 + (double)n * b->dx;
        x2 = x1 + b->dx;
        found = 0;
        for (l=0; l<b->sl.nL; l++) {
            s = (scatter *)DATA(b->sl.pL[l]);
            if (!s) continue;
            for (m=0; m<s->nData; m++) {
                x = s->x[m];
                if (x >= x1 && x < x2) {
                    found = 1;
                    break;
                }
            }
            if (found) break;
        }
        if (found) nData++;
    }
    
    if (nData == 0) {
        PostErrorDialog(w,
                      "Couldn't find any data in the selected bins.");
        return;
    }
    
    new = new_scatter(&scatterlist, nData);
    if (!new) {
        PostErrorDialog(w,
                      "Couldn't allocate memory for the new scatter plot.");
        return;
    }
    
    nData = 0;
    for (n=0; n<b->nBins; n++) {
        x1 = b->x1 + (double)n * b->dx;
        x2 = x1 + b->dx;
        nn = 0;
        s1 = s2 = 0.0;
        for (l=0; l<b->sl.nL; l++) {
            s = (scatter *)DATA(b->sl.pL[l]);
            if (!s) continue;
            for (m=0; m<s->nData; m++) {
                x = s->x[m];
                if (x >= x1 && x < x2) {
                    y = s->y[m];
                    if (first) {
                        fill_scatter(new, s);
                        first = 0;
                    }
                    nn++;
                    s1 += y;
                    s2 += y * y;
                }
            }
        }
        if (nn) {
            new->x[nData] = (x1 + x2)/2.0;
            new->ex[nData] = b->dx;
            new->y[nData] = s1/(double)nn;
            if (nn > 1)
                new->ey[nData]=sqrt((s2-s1*s1/((double)nn))/((double)(nn-1)));
            else
                new->ey[nData]=0.0;
            new->sp[nData] = NULL;
	    new->t[nData] = 0;
            nData++;
        }
    }
    
    strcpy(new->name, name);
    strcpy(new->molecule, "");
    new->date = *XS_localtime();
    new->m1 = NULL;
    new->m2 = NULL;
    new->s  = NULL;
    
    ScatterDraw(w, new, NULL);
    
    UpdateScatterListDialog(b->listD);
    
    FreeBinData(b);
    
    sprintf(buf, "New scatter plot (%d) stored as '%s'.\n",
            nData, new->name);
    send_line(buf);
}

static void GetSubList(Widget w, BinData *b, XmListCallbackStruct *cb)
{
    int nL=0;

    list *get_action_list();

    FreeBinData(b);
    b->sl.pL = get_action_list(cb, &nL, scatterlist);
    b->sl.nL = nL;
}

void create_scatsbin_dialog(Widget parent, char *cmd, Arg *args, int nargs)
{
    int n;
    Widget fr, rc, rcH;
    BinData *b;
    StdForm *sf;
    
    void wprintf();

    b = (BinData *)XtMalloc(sizeof(BinData));
    if (!b) {
        PostErrorDialog(parent, "Out of memory in scatter plot binning.");
	return; 
    }
    b->sl.nL = 0;
    b->sl.pL = NULL;
    b->nBins = 10;
    b->x1 = 0.0;
    b->dx = 1.0;
    
    sf = PostStdFormDialog(parent, "Bin scatter plots",
             BUTT_APPLY, (XtCallbackProc)DoBin, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
	     4, ScatsBinCleanUp);
    sf->user = (XtPointer)b;

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);

    b->listD = XmCreateScrolledList(rc, "scatslist", args, nargs);
    
    fr = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rc,
			         XmNshadowType, XmSHADOW_OUT, NULL);
    rcH = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                  XmNorientation, XmHORIZONTAL,
				  XmNnumColumns, 4,
				  XmNadjustLast, False,
				  XmNpacking, XmPACK_COLUMN,
				  NULL);
				    
    for (n=0; n<4; n++) {
        XtCreateManagedWidget(bin_labels[n], xmLabelWidgetClass,
                              rcH, NULL, 0);
        sf->edit[n] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                            rcH, NULL, 0);
    }

    XtAddCallback(b->listD, XmNextendedSelectionCallback,
                  (XtCallbackProc)GetSubList, b);
    
    ArrangeStdFormDialog(sf, rc);

    wprintf(sf->edit[0], "%f", b->x1);
    wprintf(sf->edit[1], "%f", b->dx);
    wprintf(sf->edit[2], "%d", b->nBins);
    wprintf(sf->edit[3], "%s", "Binned");

    XtManageChild(b->listD);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void DoMerge(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int l, m, nData, first=1;
    string buf, name;
    scatter *s, *new;
    double x, y;
    BinData *b = (BinData *)sf->user;
    
    void wsscanf(), wdscanf();
    void send_line();
    DATE *XS_localtime();
    
    if (!(b->sl.pL) || b->sl.nL == 0) {
        PostErrorDialog(w, "There are no scatter plots selected.");
        return;
    }
    
    wdscanf(sf->edit[0], &(b->x1));
    wdscanf(sf->edit[1], &(b->x2));
    if (b->x2 <= b->x1) {
        sprintf(buf, "X-range [%f,%f] cannot be <= 0.", b->x1, b->x2);
        PostErrorDialog(w, buf);
        return;
    }
    wdscanf(sf->edit[2], &(b->y1));
    wdscanf(sf->edit[3], &(b->y2));
    if (b->y2 <= b->y1) {
        sprintf(buf, "Y-range [%f,%f] cannot be <= 0.", b->y1, b->y2);
        PostErrorDialog(w, buf);
        return;
    }
    wsscanf(sf->edit[4], name);
    
    nData = 0;
    for (l=0; l<b->sl.nL; l++) {
        s = (scatter *)DATA(b->sl.pL[l]);
        if (!s) continue;
        for (m=0; m<s->nData; m++) {
            x = s->x[m];
            y = s->y[m];
            if (x >= b->x1 && x <= b->x2 && y >= b->y1 && y <= b->y2) {
                nData++;
            }
        }
    }
    
    if (nData == 0) {
        PostWarningDialog(w,
                      "Couldn't find any data in the selected region.");
        return;
    }
    
    new = new_scatter(&scatterlist, nData);
    if (!new) {
        PostErrorDialog(w,
                      "Couldn't allocate memory for the new scatter plot.");
        return;
    }
    
    nData = 0;
    for (l=0; l<b->sl.nL; l++) {
        s = (scatter *)DATA(b->sl.pL[l]);
        if (!s) continue;
        for (m=0; m<s->nData; m++) {
            x = s->x[m];
            y = s->y[m];
            if (x >= b->x1 && x <= b->x2 && y >= b->y1 && y <= b->y2) {
                if (first) {
                    fill_scatter(new, s);
                    first = 0;
                }
                new->x[nData]  = x;
                new->ex[nData] = s->ex[m];
                new->y[nData]  = y;
                new->ey[nData] = s->ey[m];
                new->sp[nData] = s->sp[m];
		new->t[nData]  = s->t[m];
                nData++;
            }
        }
    }
    
    strcpy(new->name, name);
    strcpy(new->molecule, "");
    new->date = *XS_localtime();
    new->m1 = NULL;
    new->m2 = NULL;
    new->s  = NULL;
    
    ScatterDraw(w, new, NULL);
    
    UpdateScatterListDialog(b->listD);
    
    FreeBinData(b);
    
    sprintf(buf, "New scatter plot (%d) stored as '%s'.\n",
            nData, new->name);
    send_line(buf);
}

static void GetMergeSubList(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n;
    scatter *s;
    double x1, x2, y1, y2;
    BinData *b = (BinData *)sf->user;

    list *get_action_list();
    void wprintf();

    FreeBinData(b);
    b->sl.pL = get_action_list(cb, &(b->sl.nL), scatterlist);
    
    if (!b->sl.pL || b->sl.nL <= 0) return;
    
    s = (scatter *)DATA(b->sl.pL[0]);
    x1 = s->xmin;
    x2 = s->xmax;
    y1 = s->ymin;
    y2 = s->ymax;
    
    for (n=1; n<b->sl.nL; n++) {
        s = (scatter *)DATA(b->sl.pL[n]);
        if (s->xmin < x1) x1 = s->xmin;
        if (s->xmax > x2) x2 = s->xmax;
        if (s->ymin < y1) y1 = s->ymin;
        if (s->ymax > y2) y2 = s->ymax;
    }
    
    wprintf(sf->edit[0], "%f", x1);
    wprintf(sf->edit[1], "%f", x2);
    wprintf(sf->edit[2], "%f", y1);
    wprintf(sf->edit[3], "%f", y2);
}

void create_scatsmerge_dialog(Widget parent, char *cmd, Arg *args, int nargs)
{
    int n;
    Widget rc, fr, rcH;
    BinData *b;
    StdForm *sf;
    
    void wprintf();

    b = (BinData *)XtMalloc(sizeof(BinData));
    if (!b) {
        PostErrorDialog(parent, "Out of memory in scatter plot merging.");
	return; 
    }
    b->sl.nL = 0;
    b->sl.pL = NULL;
    b->x1 = -999999.0;
    b->x2 = 999999.0;
    b->y1 = b->x1;
    b->y2 = b->x2;
    
    sf = PostStdFormDialog(parent, "Merge & Clip scatter plots",
             BUTT_APPLY, (XtCallbackProc)DoMerge, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
	     5, ScatsBinCleanUp);
    sf->user = (XtPointer)b;

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);

    b->listD = XmCreateScrolledList(rc, "scatslist", args, nargs);
    
    fr = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rc,
		                 XmNshadowType, XmSHADOW_OUT, NULL);
    rcH = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                        	  XmNorientation, XmHORIZONTAL,
				  XmNnumColumns, 5,
				  XmNadjustLast, False,
				  XmNpacking, XmPACK_COLUMN,
				  NULL);
				    
    for (n=0; n<5; n++) {
        XtCreateManagedWidget(merge_labels[n], xmLabelWidgetClass,
                              rcH, NULL, 0);
        sf->edit[n] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                            rcH, NULL, 0);
    }

    XtAddCallback(b->listD, XmNextendedSelectionCallback,
                  (XtCallbackProc)GetMergeSubList, sf);
    
    ArrangeStdFormDialog(sf, rc);

    wprintf(sf->edit[0], "%f", b->x1);
    wprintf(sf->edit[1], "%f", b->x2);
    wprintf(sf->edit[2], "%f", b->y1);
    wprintf(sf->edit[3], "%f", b->y2);
    wprintf(sf->edit[4], "%s", "Merged");

    XtManageChild(b->listD);
    XtManageChild(rc);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void DoScale(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int l, m, nData, first=1;
    string buf, name;
    scatter *s, *new;
    double x, y, ex, ey;
    BinData *b = (BinData *)sf->user;
    
    void wsscanf(), wdscanf();
    void send_line();
    DATE *XS_localtime();
    
    if (!(b->sl.pL) || b->sl.nL == 0) {
        PostErrorDialog(w, "There are no scatter plots selected.");
        return;
    }
    
    wdscanf(sf->edit[0], &(b->tr[0]));
    wdscanf(sf->edit[1], &(b->tr[1]));
    wdscanf(sf->edit[2], &(b->tr[2]));
    wdscanf(sf->edit[3], &(b->tr[3]));
    wdscanf(sf->edit[4], &(b->tr[4]));
    wdscanf(sf->edit[5], &(b->tr[5]));
    
    wsscanf(sf->edit[6], name);
    
    nData = 0;
    for (l=0; l<b->sl.nL; l++) {
        s = (scatter *)DATA(b->sl.pL[l]);
        if (!s) continue;
        nData += s->nData;
    }
    
    if (nData == 0) {
        PostWarningDialog(w,
                      "Couldn't find any data in the selected plots.");
        return;
    }
    
    new = new_scatter(&scatterlist, nData);
    if (!new) {
        PostErrorDialog(w,
                      "Couldn't allocate memory for the new scatter plot.");
        return;
    }
    
    nData = 0;
    for (l=0; l<b->sl.nL; l++) {
        s = (scatter *)DATA(b->sl.pL[l]);
        if (!s) continue;
        for (m=0; m<s->nData; m++) {
            if (first) {
                fill_scatter(new, s);
                first = 0;
            }
            x = s->x[m]; ex = s->ex[m];
            y = s->y[m]; ey = s->ey[m];
            new->x[nData]  = b->tr[0] * x + b->tr[1] * y + b->tr[2];
            new->ex[nData] = sqrt(b->tr[0] * b->tr[0] *ex*ex +
                                  b->tr[1] * b->tr[1] *ey*ey);
            new->y[nData]  = b->tr[3] * x + b->tr[4] * y + b->tr[5];
            new->ey[nData] = sqrt(b->tr[3] * b->tr[3] *ex*ex +
                                  b->tr[4] * b->tr[4] *ey*ey);
            new->sp[nData] = s->sp[m];
            new->t[nData]  = s->t[m];
            nData++;
        }
    }
    
    strcpy(new->name, name);
    strcpy(new->molecule, "");
    new->date = *XS_localtime();
    new->m1 = NULL;
    new->m2 = NULL;
    new->s  = NULL;
    
    ScatterDraw(w, new, NULL);
    
    UpdateScatterListDialog(b->listD);
    
    FreeBinData(b);
    
    sprintf(buf, "New scatter plot (%d) stored as '%s'.\n",
            nData, new->name);
    send_line(buf);
}

void create_scatsscale_dialog(Widget parent, char *cmd, Arg *args, int nargs)
{
    int n;
    Widget fr, rc, rcV, rc1, rc2, rc3;
    BinData *b;
    StdForm *sf;
    
    void wprintf();

    b = (BinData *)XtMalloc(sizeof(BinData));
    if (!b) {
        PostErrorDialog(parent, "Out of memory in scatter plot scaling.");
	return; 
    }
    b->sl.nL = 0;
    b->sl.pL = NULL;
    b->tr[0] = 1.0;
    b->tr[1] = 0.0;
    b->tr[2] = 0.0;
    b->tr[3] = 0.0;
    b->tr[4] = 1.0;
    b->tr[5] = 0.0;
    
    sf = PostStdFormDialog(parent, "Scale scatter plots",
             BUTT_APPLY, (XtCallbackProc)DoScale, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
	     7, ScatsBinCleanUp);
    sf->user = (XtPointer)b;

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);

    b->listD = XmCreateScrolledList(rc, "scatslist", args, nargs);
    
    fr = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rc,
				 XmNshadowType, XmSHADOW_OUT, NULL);
				            
    rcV = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                        	  XmNorientation, XmVERTICAL,
				  NULL);
                          
    rc1 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rcV,
                                  XmNorientation, XmHORIZONTAL,
				  NULL);
    for (n=0; n<3; n++) {
        XtCreateManagedWidget(scale_labels[n], xmLabelWidgetClass,
                              rc1, NULL, 0);
        sf->edit[n] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                            rc1, NULL, 0);
    }
    
    rc2 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rcV,
                                  XmNorientation, XmHORIZONTAL,
				  NULL);
    for (n=3; n<6; n++) {
        XtCreateManagedWidget(scale_labels[n], xmLabelWidgetClass,
                              rc2, NULL, 0);
        sf->edit[n] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                            rc2, NULL, 0);
    }
    
    rc3 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rcV,
                                  XmNorientation, XmHORIZONTAL,
				  NULL);
    XtCreateManagedWidget("New name:", xmLabelWidgetClass,
                          rc3, NULL, 0);
    sf->edit[6] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc3, NULL, 0);

    XtAddCallback(b->listD, XmNextendedSelectionCallback,
                  (XtCallbackProc)GetSubList, b);

    ArrangeStdFormDialog(sf, rc);

    wprintf(sf->edit[0], "%f", b->tr[0]);
    wprintf(sf->edit[1], "%f", b->tr[1]);
    wprintf(sf->edit[2], "%f", b->tr[2]);
    wprintf(sf->edit[3], "%f", b->tr[3]);
    wprintf(sf->edit[4], "%f", b->tr[4]);
    wprintf(sf->edit[5], "%f", b->tr[5]);
    wprintf(sf->edit[6], "%s", "Scaled");

    XtManageChild(b->listD);
    
    ManageDialogCenteredOnPointer(sf->form);
}

void ScatterPointOps(Widget w, char *cmd, XtPointer cd)
{
    int n, nT, m=0;
    scatter *s = NULL, *old = vP->p;
    DataSetPtr d = NULL;
    scanPtr sp;
    
    int CountTaggedScatterPoints(scatter *);
    void MakeDataSetIntoSeq(DataSetPtr);
    void MakeSpeScatterPlot(DataSetPtr, int, int);
    void draw_main();
    
    if (vP->mode != SHOW_SCATTER || !old) return;
    
    nT = CountTaggedScatterPoints(old);
    
    if (strcmp(cmd, "select_tagged") == 0) {
        s = new_scatter(&scatterlist, nT);
        if (!s) {
            PostErrorDialog(w, "Out of memory when allocating new scatter plot");
            return;
        }
        fill_scatter(s, old);
        for (n=0; n<old->nData; n++) {
            if (old->t[n]) {
                s->x[m]  = old->x[n];
                s->y[m]  = old->y[n];
                s->ex[m] = old->ex[n];
                s->ey[m] = old->ey[n];
                s->sp[m] = old->sp[n];
                s->t[m] = 0;
                m++;
            }
        }
        sprintf(s->name, "%s [%d->%d]", old->name, old->nData, s->nData);
        ScatterDraw(w, s, NULL);
    } else if (strcmp(cmd, "select_untagged") == 0) {
        s = new_scatter(&scatterlist, old->nData-nT);
        if (!s) {
            PostErrorDialog(w, "Out of memory when allocating new scatter plot");
            return;
        }
        fill_scatter(s, old);
        for (n=0; n<old->nData; n++) {
            if (!old->t[n]) {
                s->x[m]  = old->x[n];
                s->y[m]  = old->y[n];
                s->ex[m] = old->ex[n];
                s->ey[m] = old->ey[n];
                s->sp[m] = old->sp[n];
                s->t[m] = 0;
                m++;
            }
        }
        sprintf(s->name, "%s [%d->%d]", old->name, old->nData, s->nData);
        ScatterDraw(w, s, NULL);
    } else if (strcmp(cmd, "untag") == 0) {
        if (!nT) return;
        for (n=0; n<old->nData; n++) old->t[n] = 0;
        draw_main();
    } else if (strcmp(cmd, "invert") == 0) {
        for (n=0; n<old->nData; n++) {
            old->t[n] = (old->t[n]) ? 0 : 1;
        }
        draw_main();
    } else if (strcmp(cmd, "select_tagged_as_scans") == 0 ||
               strcmp(cmd, "select_untagged_as_scans") == 0) {
        d = new_dataset(get_listlist(), "", NULL);
        if (!d) {
            PostErrorDialog(w, "Out of memory when allocating new data set");
            return;
        }
        m = 0;
        for (n=0; n<old->nData; n++) {
          /* Skip those not tagged / untagged         */
            if (strcmp(cmd, "select_tagged_as_scans") == 0) {
                if (!old->t[n]) continue;
            } else {
                if (old->t[n]) continue;
            }
            if (!old->sp[n]) continue; /* Skip pnts lacking scan pointer */
            sp = copy_scan(d, old->sp[n]);
            if (!sp) {
                PostErrorDialog(w, "Out of memory when allocating new scan");
                break;
            }
            m++;
        }
        if (!m) { /* No scans selected */
            DeleteLastDataSet();
            PostWarningDialog(w, "No scans could be selected");
            return;
        }
        MakeDataSetIntoSeq(d);
        sprintf(d->name, "%s [%d->%d]", old->name, old->nData, m);
        vP->from = vP->to = d;
        vP->s = (scanPtr)DATA(scan_iterator(NULL, d));
        MakeSpeScatterPlot(d, old->xtype, old->ytype);
        draw_main();
    } else {
       return;
    }
}
