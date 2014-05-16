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

#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/Frame.h>

#include "defines.h"
#include "global_structs.h"
#include "menus.h"
#include "dialogs.h"

#ifdef HAVE_LIBPGPLOT
#include "cpgplot.h"
#endif

/* Global declarations */
void   SetPGStyle(PSSTY *);
void   PostErrorDialog(Widget, char *);
void   PostWarningDialog(Widget, char *);
void   PostMessageDialog(Widget, char *);
int    PostQuestionDialog(Widget, char *);
void   ManageDialogCenteredOnPointer(Widget);
Widget CreateOptionMenu(Widget, MenuBarItem *);
void   SetDefaultOptionMenuItem(Widget, int);
void   SetAnyToggle(char *, int);
void   wprintf();
void   wdscanf(Widget, double *);
void   wiscanf(Widget, int *);
void   draw_main();

extern int    pgplot;

extern PSDATA  ps;
extern GLOBAL *gp;

/* Local declarations */
static list polylist;
static PolyLine *currPL = NULL;

static void PolyLineTypeCallback(Widget, char *, XmAnyCallbackStruct *);

MenuItem PolyLineTypeMenuData[] = {
  {"Open", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PolyLineTypeCallback, "0", NULL},
  {"Closed", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PolyLineTypeCallback, "1", NULL},
EOI};

MenuBarItem PolyLineTypeOptionMenu = {
   "Type of polyline", ' ', True, PolyLineTypeMenuData
};

void init_polylist()
{
    status init_list(list *);
    
    init_list(&polylist);
}

list *get_polylist()
{
    return &polylist;
}

int count_polyline()
{
    int n = 0;
    list curr = NULL;

    list list_iterator();
    bool empty_list();

    if (empty_list(polylist) == tRUE)
        return 0;

    while ( (curr = list_iterator(polylist, curr)) != NULL) n++;

    return n;
}

void UpdatePolylineInfo()
{
    int n = count_polyline();
    string buf;
    
    if (n == 0) {
        sprintf(buf, "No polylines");
    } else if (n == 1) {
        sprintf(buf, "No of polylines: 1");
    } else {
        sprintf(buf, "No of polylines: %d", n);
    }
    wprintf(gp->TPolyline[0], buf);
}

static PolyLine *del_polyline(PolyLine *pPoly)
{
    if (pPoly) {
        if (pPoly->p) free(pPoly->p);
        free(pPoly);
    }
    return NULL;
}

PolyLine *new_polyline(list *p_L, int n)
{
    PolyLine *pPoly;

    status   insert(list *, generic_ptr);

    pPoly = (PolyLine *)calloc(1, sizeof(PolyLine));
    if (pPoly == NULL)
        return NULL;

    pPoly->n = n;
    pPoly->type = 0;
    pPoly->p = (Point *)calloc(n, sizeof(Point));
    if (pPoly->p == NULL)
        return del_polyline(pPoly);

    if (insert(p_L, (generic_ptr)pPoly) == Error)
        return del_polyline(pPoly);
        
    return pPoly;
}

list *polyline_delete(list *pL, list node)
{
    PolyLine *pPoly;

    status delete_node();
    bool empty_list();

    if (empty_list(*pL) == tRUE || !node)
        return NULL;

    pPoly = (PolyLine *)DATA(node);
    
    if (pPoly) pPoly = del_polyline(pPoly);

    if (delete_node(pL, node) == Error)
        return NULL;

    return pL;
}

list polyline_iterator(list last)
{
    return (last == NULL) ? polylist : NEXT(last);
}

void destroy_polylist()
{
    list curr = polylist;

    while (polyline_delete(&curr, curr) != NULL)
        ;
}

PolyLine *copy_polyline(list *pL, PolyLine *old)
{
    int i;
    Point *p;
    PolyLine *new = NULL;
    
    if (!old)
        return new;
        
    new = new_polyline(pL, old->n);
    if (!new)
        return new;

    p = new->p;

    *new = *old;

    new->p = p;

    for (i=0; i<old->n; i++) {
        p[i] = old->p[i];
    }
    return new;
}

PolyLine *resize_polyline(PolyLine *old, int n)
{
    Point *p;
    
    if (!old || n < 0) return NULL;
    
    if (n == old->n) return old;
    
    p = (Point *)realloc(old->p, n * sizeof(Point));
    
    if (!p) return NULL;

    old->p = p;
    old->n = n;
        
    return old;
}

PolyLine *InitPoint(Point p)
{
    PolyLine *new;
    
    new = new_polyline(&polylist, 1);
    if (!new) return NULL;
    
    new->p[0] = p;
    new->type = 0;
    
    UpdatePolylineInfo();
    
    return new;
}

PolyLine *AddPoint(Point p, PolyLine *pl)
{
    PolyLine *new;
    
    if (!pl) return NULL;
    
    new = resize_polyline(pl, pl->n + 1);
    if (!new) return NULL;
    
    new->p[new->n - 1] = p;
    
    return new;
}

PolyLine *RemovePoint(int n, PolyLine *pl)
{
    int i = n, j;
    PolyLine *new;
    
    if (!pl) return NULL;
    if (pl->n == 0) return NULL;
    if (i < 0) i = pl->n - 1; /* Remove last point if n < 0 */
    if (i >= pl->n) return NULL;
    
    for (j=i; j<pl->n-1; j++) {
        pl->p[j] = pl->p[j+1];
    }
    
    new = resize_polyline(pl, pl->n - 1);
    if (!new) return NULL;
    
    return new;
}

static int DeltaResidue(Point *p1, Point *p2, Point *p0, double *res)
{
    double A, B, C, D, t, E, F;
    
    A = p1->x - p0->x;
    B = p2->x - p1->x;
    C = p1->y - p0->y;
    D = p2->y - p1->y;
    
    E = A*D - B*C;
    
    if (E == 0.0) { /* p0 possibly on the line p1 -> p2 */
        if (B != 0.0) {
            t = A/B;
        } else if (D != 0.0) {
            t = C/D;
        } else { /* p1 == p2 */
            return -1;
        }
        if (t >= 0.0 && t <= 1.0) { /* p0 on the line p1 -> p2 */
            return 1;
        }
    }
    
/* Ok, calculate the contribution to the residue */
    F = A*(A+B) + C*(C+D);
    *res = atan2(E, F)/2.0/PI;
    
    return 0;
}

#define REPS 1.0e-7

int InsidePolyLine(Point *p0, PolyLine *pl)
{
    int n, nr, err;
    Point *p1, *p2;
    double r, dr;
    
    if (!pl || !p0) return -1;
    if (pl->n < 3) return -2;     /* Too few points to be closed */
    
    r = 0.0;
    nr = 0;
    for (n=0; n<pl->n; n++) {
        p1 = &(pl->p[n]);
        if (n == pl->n - 1)
            p2 = &(pl->p[0]);
        else
            p2 = &(pl->p[n+1]);
        err = DeltaResidue(p1, p2, p0, &dr);
        if (err == -1) { /* p1 == p2 */
            continue;
        } else if (err == 1) { /* p0 on the line p1 -> p2*/
            return 2;
        }
        r += dr;
        nr++;
    }
    if (nr < 3) return -3;
    
    r = fabs(r);
    if (r < REPS)
        return 0;
    else
        return 1;
}

int InsidePolyLines(Point *p)
{
    list curr = NULL;
    PolyLine *PL;
    
    while ((curr = polyline_iterator(curr)) != NULL) {
        PL = (PolyLine *)DATA(curr);
        if (PL->type != 1) continue; /* Skip open poly lines */
        if (InsidePolyLine(p, PL) >= 1) return 1;
    }
    
    return 0;
}

double PointDist(Point *p1, Point *p2)
{
    double dx, dy;
    
    if (!p1 || !p2) return 0.0;
    
    dx = p1->x - p2->x;
    dy = p1->y - p2->y;
    
    return sqrt(dx*dx+dy*dy);
}

double GetPolyLineLength(PolyLine *pl)
{
    int n, end;
    double d=0.0;
    Point *p1, *p2;
    
    if (pl->type)
        end = pl->n;
    else
        end = pl->n - 1;
    
    for (n=0; n<end; n++) {
        p1 = &(pl->p[n]);
        if (n == pl->n - 1)
            p2 = &(pl->p[0]);
        else
            p2 = &(pl->p[n+1]);
        
        d += PointDist(p1, p2);
    }
    
    return d;
}

static Point *GetPolyLinePoint(PolyLine *pl, double frac)
{
    int n;
    double ds, length, f1, f2, s = 0.0;
    Point *next, *prev;
    static Point p0;

    if (!pl) return NULL;
    
    if (frac < 0.0 || frac > 1.0) return NULL;
    if (pl->n < 2) return NULL;
    
    if (frac == 0.0) return pl->p;
    if (frac == 1.0) {
        if (pl->type)
            return pl->p;
        else
            return &pl->p[pl->n - 1];
    }
    
    length = GetPolyLineLength(pl);
    if (length <= 0.0) return NULL;
    
    for (n=1; n< (pl->type ? pl->n + 1 : pl->n); n++) {
        prev = &pl->p[n-1];
        if (n == pl->n)
            next = &pl->p[0];
        else
            next = &pl->p[n];
        ds = PointDist(next, prev);
        f1 = s/length;
        f2 = (s + ds)/length;
        if (f2 <= f1) continue;
        if (frac >= f1 && frac <= f2) {
            p0.x = prev->x + (frac - f1)*(next->x - prev->x)/(f2-f1);
            p0.y = prev->y + (frac - f1)*(next->y - prev->y)/(f2-f1);
            return &p0;
        }
        s += ds;
    }
    
    return NULL;
}

Point *StepAlongPolyLine(PolyLine *pl, int nPoints, double *dist, Point *p)
{
    static int n, nP;
    static double length;
    double f;
    
    if (!pl) return NULL;
    
    if (!p) { /* Initialize StepAlongPolyLine */
        if (nPoints < 2 || pl->n < 1) return NULL;
        n = 0;
        nP = nPoints;
        length = GetPolyLineLength(pl);
        if (dist) *dist = 0.0;
        return pl->p;
    } else if (n >= nP-1)
        return NULL;
    
    n++;
    f = (double)(n)/(double)(nP - 1);
    if (dist) *dist = f*length;
    
    return GetPolyLinePoint(pl, f);
}

void TogglePolyLines(Point *p0, int type)
{
    list curr = NULL;
    PolyLine *p;
    int changed = 0;
    
    while ((curr = polyline_iterator(curr)) != NULL) {
        p = (PolyLine *)DATA(curr);
        if (p->type == type) continue;
        if (InsidePolyLine(p0, p) >= 1) {
            p->type = type;
            changed = 1;
        }
    }
    if (changed) draw_main();
}

void draw_polylines(GC gc)
{
    list curr = NULL;
    PolyLine *p;
    
    void draw_polyline(GC, PolyLine *);
    
    while ((curr = polyline_iterator(curr)) != NULL) {
        p = (PolyLine *)DATA(curr);
        draw_polyline(gc, p);
    }
}

static void mark_poly_point(GC gc, PolyLine *p, int n)
{
    void DrawContourDot();
    
    if (!p) return;
    
    if (n < 0 || n >= p->n) return;
    
    DrawContourDot(gc, 'c', 4.0, p->p[n].x, p->p[n].y);
}

static void mark_poly_line(GC gc, PolyLine *p)
{
    int n;
    
    if (!p) return;
    
    for (n=0; n<p->n; n++)
        mark_poly_point(gc, p, n);
}

typedef struct {
    int       c_point;
    PolyLine *p;
} PolyData;

static void cancel_polyline(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    draw_main();
    XtDestroyWidget(sf->form);
}

static void update_polyline(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    Point *p0;
    PolyData *p = (PolyData *)sf->user;
    
    p0 = &(p->p->p[p->c_point]);
    
    wdscanf(sf->edit[1], &(p0->x));
    wdscanf(sf->edit[2], &(p0->y));
    wprintf(sf->edit[3], "Total length %f", GetPolyLineLength(p->p));
    
    draw_main();
    mark_poly_point(gp->gcGauss, p->p, p->c_point);
}

static void edit_next_point(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    PolyData *p = (PolyData *)sf->user;
    int n = p->c_point + 1;
    
    if (n >= p->p->n) n = 0;
    p->c_point = n;
    
    wprintf(sf->edit[0], "Point %d out of %d.", n+1, p->p->n);
    wprintf(sf->edit[1], "%f", p->p->p[p->c_point].x);
    wprintf(sf->edit[2], "%f", p->p->p[p->c_point].y);
    wprintf(sf->edit[3], "Total length %f", GetPolyLineLength(p->p));
    
    draw_main();
    mark_poly_point(gp->gcGauss, p->p, p->c_point);
}

static void PolyLineTypeCallback(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int n = atoi(str);
    
    if (!currPL) return;
    currPL->type = n;
}

static void edit_prev_point(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    PolyData *p = (PolyData *)sf->user;
    int n = p->c_point - 1;
    
    if (n <= 0) n = p->p->n - 1;
    p->c_point = n;
    
    wprintf(sf->edit[0], "Point %d out of %d.", n+1, p->p->n);
    wprintf(sf->edit[1], "%f", p->p->p[p->c_point].x);
    wprintf(sf->edit[2], "%f", p->p->p[p->c_point].y);
    wprintf(sf->edit[3], "Total length %f", GetPolyLineLength(p->p));
    
    draw_main();
    mark_poly_point(gp->gcGauss, p->p, p->c_point);
}

static void EditPolyLineDialog(Widget w, PolyLine *p)
{
    Widget rc, menu, prev, next, rch;
    PolyData *PD;
    StdForm *sf;

    if (!p) return;
    
    if (!w) {
        w = gp->top;
    } else {
        while (!XtIsWMShell(w)) w = XtParent(w);
    }
    
    PD = (PolyData *)XtMalloc(sizeof(PolyData));
    if (!PD) {
        PostErrorDialog(w, "Out of memory in polyline dialog.");
        return;
    }
    
    sf = PostStdFormDialog(w, "Edit polyline",
             BUTT_APPLY, (XtCallbackProc)update_polyline, NULL,
             BUTT_CANCEL, (XtCallbackProc)cancel_polyline, NULL,
             NULL, NULL, NULL,
             4, NULL);
    sf->user = (XtPointer)PD;
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
                                  
    menu = CreateOptionMenu(rc, &PolyLineTypeOptionMenu);
    SetDefaultOptionMenuItem(menu, p->type);
    
    sf->edit[0] = XtCreateManagedWidget("     ", xmLabelWidgetClass, rc,
                                        NULL, 0);
    sf->edit[1] = XtCreateManagedWidget("entry", xmTextWidgetClass, rc,
                                        NULL, 0);
    sf->edit[2] = XtCreateManagedWidget("entry", xmTextWidgetClass, rc,
                                        NULL, 0);
    sf->edit[3] = XtCreateManagedWidget("     ", xmLabelWidgetClass, rc,
                                        NULL, 0);
   
    rch = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                 XmNorientation,       XmHORIZONTAL,
                                 NULL);
    next   = XtCreateManagedWidget("next", xmPushButtonWidgetClass,
                                   rch, NULL, 0);
    prev   = XtCreateManagedWidget("previous", xmPushButtonWidgetClass,
                                   rch, NULL, 0);
                                   
    PD->p       = p;
    PD->c_point = 0;
    
    XtAddCallback(next, XmNactivateCallback,
                  (XtCallbackProc)edit_next_point, sf);                        
    XtAddCallback(prev, XmNactivateCallback,
                  (XtCallbackProc)edit_prev_point, sf);                        
    
    ArrangeStdFormDialog(sf, rc);
    
    wprintf(sf->edit[0], "Point 1 out of %d.", p->n);
    wprintf(sf->edit[1], "%f", p->p[0].x);
    wprintf(sf->edit[2], "%f", p->p[0].y);
    wprintf(sf->edit[3], "Total length %f", GetPolyLineLength(p));
    
    mark_poly_point(gp->gcGauss, p, 0);
    
    XtManageChild(menu);
    
    ManageDialogCenteredOnPointer(sf->form);
}

void EditPolyLines(Widget w, Point *p0)
{
    list curr = NULL;
    PolyLine *p;
    
    while ((curr = polyline_iterator(curr)) != NULL) {
        p = (PolyLine *)DATA(curr);
        if (!p0 || InsidePolyLine(p0, p) >= 1 || p->n < 3) {
            currPL = p;
            EditPolyLineDialog(w, p);
        }
    }
}

void DeletePolyLines(Widget w, Point *p0)
{
    list curr = NULL;
    PolyLine *p;
    string s;
    
    while ((curr = polyline_iterator(curr)) != NULL) {
        p = (PolyLine *)DATA(curr);
        if (!p0 || InsidePolyLine(p0, p) >= 1 || p->n < 3) {
            sprintf(s, "Want to delete %s polyline (n=%d)?",
                       p->type ? "closed" : "open", p->n);
            mark_poly_line(gp->gcGauss, p);
            if (PostQuestionDialog(w, s)) {
                if (polyline_delete(&polylist, curr)) {
                    PostMessageDialog(w, "Polyline deleted.");
                } else {
                    PostErrorDialog(w, "Polyline couldn't be deleted.");
                }
                UpdatePolylineInfo();
                draw_main();
                break;
            }
            draw_main();
        }
    }
}

PolyLine *GetFirstPolyLine(Point *p0)
{
    list curr = NULL;
    PolyLine *p;
    
    while ((curr = polyline_iterator(curr)) != NULL) {
        p = (PolyLine *)DATA(curr);
        if (!p0 || InsidePolyLine(p0, p) >= 1 || p->n < 3) {
            return p;
        }
    }
    
    return NULL;
}

PolyLine *GetFirstClosedPolyLine(Point *p0)
{
    list curr = NULL;
    PolyLine *p;
    
    while ((curr = polyline_iterator(curr)) != NULL) {
        p = (PolyLine *)DATA(curr);
        if (!p->type || p->n < 3) continue; 
        if (!p0 || InsidePolyLine(p0, p) >= 1) return p;
    }
    
    return NULL;
}

static void DoPolyCircle(Widget w, StdForm *sf, XtPointer cd)
{
    int n, i;
    string buf;
    double x, y, r, phi;
    PolyLine *p;
    
    wdscanf(sf->edit[0], &x);
    wdscanf(sf->edit[1], &y);
    wdscanf(sf->edit[2], &r);
    if (r <= 0.0) {
        sprintf(buf, "Radius (%f) is <= 0.0.", r);
        PostErrorDialog(NULL, buf);
        return;
    }
    
    wiscanf(sf->edit[3], &n);
    if (n < 3) {
        sprintf(buf, "Number of points (%d) is < 3.", n);
        PostErrorDialog(NULL, buf);
        return;
    }
    
    p = new_polyline(&polylist, n);
    if (!p) {
        PostErrorDialog(NULL, "Couldn't allocate memory for circle polyline.");
        return;
    }
    
    for (i=0; i<n; i++) {
        phi = (double)i * 2.0 * PI / (double)n;
        p->p[i].x = x + r*cos(phi);
        p->p[i].y = y + r*sin(phi);
    }
    
    p->type = 1;
    
    UpdatePolylineInfo();
    
    SetAnyToggle("boxes", 1);
}

void PostPolyCircleDialog(Widget wid, char *cmd, XtPointer cd)
{
    Widget rc;
    Widget w = wid;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Create polyline circle",
             BUTT_APPLY, (XtCallbackProc)DoPolyCircle, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             4, NULL);
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    
    XtCreateManagedWidget("Center of circle (x and y):",
                          xmLabelWidgetClass,
                          rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                          rc, NULL, 0);
    sf->edit[1] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                          rc, NULL, 0);
    XtCreateManagedWidget("Circle radius:",
                          xmLabelWidgetClass,
                          rc, NULL, 0);
    sf->edit[2] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                          rc, NULL, 0);
    XtCreateManagedWidget("Number of points in polyline (>=3):",
                          xmLabelWidgetClass,
                          rc, NULL, 0);
    sf->edit[3] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                          rc, NULL, 0);
    
    ArrangeStdFormDialog(sf, rc);
    
    wprintf(sf->edit[0], "0.0");
    wprintf(sf->edit[1], "0.0");
    wprintf(sf->edit[2], "1.0");
    wprintf(sf->edit[3], "24");

    ManageDialogCenteredOnPointer(sf->form);
}    

void MenuDeletePolyLines(Widget wid, char *cmd, XtPointer cd)
{
    int n = 0;
    string buf;
    
    if (strcmp(cmd, "all") == 0) {   /* remove all polylines */
       n = count_polyline();
       destroy_polylist();
       /* we must initialize the list after it has been destroyed */
       init_polylist();
    } else {                         /* remove last polyline */
       if (polyline_delete(&polylist, polyline_iterator(NULL)) != NULL) n++;
    }
    if (n == 0)
        sprintf(buf, "No polylines to delete.");
    else if (n == 1)
        sprintf(buf, "Deleted one polyline.");
    else
        sprintf(buf, "Deleted %d polylines.", n);

    if (n > 0) SetAnyToggle("boxes", 1);
    
    UpdatePolylineInfo();
    
    PostMessageDialog(wid, buf);
}
