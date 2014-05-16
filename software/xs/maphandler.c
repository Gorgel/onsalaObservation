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

#define BLANK_INSIDE_POLYLINES    1
#define BLANK_OUTSIDE_POLYLINES   2
#define UNBLANK_INSIDE_POLYLINES  3
#define UNBLANK_OUTSIDE_POLYLINES 4

#define AVE_X_IN   1
#define AVE_X_OUT  2
#define AVE_Y_IN   3
#define AVE_Y_OUT  4

#define OP_ADD 0
#define OP_SUB 1
#define OP_MUL 2
#define OP_DIV 3
#define OP_AVE 4
#define OP_CHI 5
#define OP_CPLADD 6
#define OP_CPLSUB 7

#define NUMOPS 8

/* External declarations */
extern DRAW         draw;
extern GLOBAL      *gp;
extern VIEW        *vP;

void PostErrorDialog();
void PostMessageDialog();
void PostWarningDialog();
int  PostQuestionDialog();
void ManageDialogCenteredOnPointer(Widget);

void SetViewMode(int, scanPtr, MAP *, scatter *);

/* Local declarations */
typedef struct {
    MAP *m;
    Widget w;
} mapWidget;

typedef struct {
    MAP *m;
    Widget *w;
} mapWidgetPtr;

typedef struct {
    MAP **m;
    Widget w;
    StdForm *sf;
} mapDualWidget;

typedef struct {
    int n;
    StdForm *sf;
} intWidget;

typedef struct {
    mapDualWidget mwL, mwR;
    intWidget iw[NUMOPS];
    MAP *m1, *m2;
    int op;
} MapAriStruct;

static list maplist;

unsigned long int map_bytes = 0;

static char *op_string[] = {"+", "-", "*", "/", "ave", "chi", "cplx+", "cplx-"};
static int   op_types[]  = {OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_AVE, OP_CHI,
OP_CPLADD, OP_CPLSUB};

static char *smm_label[] = {
    "Store maps as:",
    "Start velocity:",
    "Velocity increment",
    "No. of intervals:"
};

static char *PrecessMap_Help = "\
                           Precess map help\n\
                           ----------------\n\
Here you can precess the coordinates of map. By entering the epoch as\n\
J2000.0 you will precess the coordinates from the current epoch (see the\n\
'Current data information' frame to the left) to Julian 2000.0 coordinates.\n\
To precess to Besselian 1950.0 use B1950.0.\n\
";

static char *CombineMaps_Help = "\
                           Combine maps help\n\
                           -----------------\n\
Here you can select two maps to be combined into a single map. If new center\n\
coordinates are specified these will be the center coordinates of the resulting\n\
map, otherwise the coordinates from the 'left' map will be used.\n\
";

#define nSMM 4

void init_maplist()
{
    status init_list();

    init_list(&maplist);
}

double GetMapMemory()
{
    return (double)map_bytes/1024.0/1024.0;
}

list *get_maplist()
{
    return &maplist;
}

static int count_maps()
{
    int n = 0;
    list curr = NULL;

    list list_iterator();
    bool empty_list();

    if (empty_list(maplist) == tRUE)
        return 0;

    while ( (curr = list_iterator(maplist, curr)) != NULL) n++;

    return n;
}

static MAP *del_map(MAP *pMap)
{
    void FreeDoubleArray(), FreeIntArray(), FreeScanPtrArray();

    if (pMap) {
        if (pMap->d)
            FreeDoubleArray(pMap->d, pMap->i_no, pMap->j_no);
        if (pMap->e)
            FreeDoubleArray(pMap->e, pMap->i_no, pMap->j_no);
        if (pMap->f)
            FreeIntArray(pMap->f, pMap->i_no, pMap->j_no);
        if (pMap->sp)
            FreeScanPtrArray(pMap->sp, pMap->i_no, pMap->j_no);
        free(pMap);
    }
    return NULL;
}

MAP *new_map(list *p_L, int nX, int nY)
{
    MAP *pMap;

    double  **AllocDoubleArray();
    int     **AllocIntArray();
    scanPtr **AllocScanPtrArray();

    status   insert(list *, generic_ptr);

    pMap = (MAP *)calloc(1, sizeof(MAP));
    if (pMap == NULL)
        return NULL;

    pMap->d = NULL;
    pMap->e = NULL;
    pMap->f = NULL;
    pMap->original = NULL;
    pMap->i_no = nX;
    pMap->j_no = nY;
    pMap->saved = 0;
    pMap->use_attached_cont = 0;
    pMap->posAngle = 0.0;
    pMap->epoch = ' ';
    pMap->equinox = 0.0;
    pMap->nfftx = pMap->nffty = 0;

    pMap->d = AllocDoubleArray(nX, nY);
    if (pMap->d == NULL)
        return del_map(pMap);

    pMap->e = AllocDoubleArray(nX, nY);
    if (pMap->e == NULL)
        return del_map(pMap);

    pMap->f = AllocIntArray(nX, nY);
    if (pMap->f == NULL)
        return del_map(pMap);
        
    pMap->sp = AllocScanPtrArray(nX, nY);
    if (pMap->sp == NULL)
        return del_map(pMap);

    if (insert(p_L, (generic_ptr)pMap) == Error)
        return del_map(pMap);

    map_bytes += sizeof(MAP);
    map_bytes += nX * nY *
                 (2 * sizeof(double) + sizeof(int) + sizeof(scanPtr));

    return pMap;
}

list *map_delete(list *pL, list node)
{
    unsigned long int n = 0;
    MAP *pMap;

    status delete_node();
    bool empty_list();

    if (empty_list(*pL) == tRUE || !node)
        return NULL;

    pMap = (MAP *)DATA(node);
    if (pMap) {
        n = sizeof(MAP);
        n += pMap->i_no * pMap->j_no *
             (2 * sizeof(double) + sizeof(int) + sizeof(scanPtr));
        pMap = del_map(pMap);
    }

    if (delete_node(pL, node) == Error)
        return NULL;

    if (n < map_bytes)
        map_bytes -= n;
    else
        map_bytes = 0;

    return pL;
}

list map_iterator(list last)
{
    return (last == NULL) ? maplist : NEXT(last);
}

void destroy_maplist()
{
    list curr = maplist;

    while (map_delete(&curr, curr) != NULL)
        ;
}

list *get_action_list(XmListCallbackStruct *cb, int *nL, list which)
{
    int n, i, nSel, nItem;
    char *txt;
    XmString xStr;
    string buf;
    list curr, *pL = NULL;

    list list_iterator();

    *nL = 0;

    if (cb->reason == XmCR_SINGLE_SELECT) {
        nItem = 1;
    } else {
        nItem = cb->selected_item_count;
    }

    if (nItem == 0)
        return NULL;

    pL = (list *)malloc(nItem * sizeof(list));

    if (pL == NULL)
        return NULL;

    for (n=0; n<nItem; n++) {
        if (cb->reason == XmCR_SINGLE_SELECT)
           xStr = XmStringCopy(cb->item);
        else
           xStr = XmStringCopy(cb->selected_items[n]);
        if (XmStringGetLtoR(xStr, XmSTRING_DEFAULT_CHARSET, &txt)) {
            sscanf(txt, "%d %s", &nSel, buf);
            XtFree(txt);

            i = 1;
            curr = NULL;
            while ((curr = list_iterator(which, curr)) != NULL) {
                if (i == nSel)
                    break;
                i++;
            }
            if (i != nSel) {
                free(pL);
                return NULL;
            }
            pL[(*nL)++] = curr;
        }
    }
    return pL;
}

MAP *copy_map(list *pL, MAP *old)
{
    int i, j, nX, nY;
    double **d, **e;
    int **f;
    scanPtr **s;
    MAP *new = NULL;
    
    if (!old)
        return new;
        
    nX = old->i_no;
    nY = old->j_no;
        
    new = new_map(pL, nX, nY);
    if (!new)
        return new;

    d = new->d;
    e = new->e;
    f = new->f;
    s = new->sp;

    *new = *old;

    new->sp = s;
    new->d = d;
    new->e = e;
    new->f = f;

    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            d[i][j] = old->d[i][j];
            e[i][j] = old->e[i][j];
            f[i][j] = old->f[i][j];
            s[i][j] = old->sp[i][j];
        }
    }
    
    new->original = old;
    
    return new;
}

void fill_map(MAP *m, MAP *old)
{
    int nX, nY;
    double **d, **e;
    int **f;
    scanPtr **s;

    d = m->d;
    e = m->e;
    f = m->f;
    s = m->sp;
    nX = m->i_no;
    nY = m->j_no;

    *m = *old;

    m->sp = s;
    m->d = d;
    m->e = e;
    m->f = f;
    m->i_no = nX;
    m->j_no = nY;
}

void ZeroScanInMapList(scanPtr s)
{
    int i, j;
    MAP *m;
    list curr = NULL;
    
    while ( (curr = map_iterator(curr)) != NULL) {
        m = (MAP *)DATA(curr);
        for (i=0; i<m->i_no; i++) {
            for (j=0; j<m->j_no; j++) {
        	if (m->sp[i][j] == s) {
                    m->sp[i][j] = NULL;
        	}
            }
        }
    }
}

void ZeroOriginalMapInMapList(MAP *mdel)
{
    MAP *m;
    list curr = NULL;
    
    while ( (curr = map_iterator(curr)) != NULL) {
        m = (MAP *)DATA(curr);
        if (m->original == mdel) m->original = NULL;
    }
}

static void get_map_name(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    string buf;
    MAP *pMap, *pCur = vP->m;

    void send_line(), wsscanf();

    if (!pCur) {
        PostErrorDialog(NULL, "Couldn't find any current map.");
        return;
    }
    
    if ((pMap = copy_map(&maplist, pCur)) == NULL) {
        PostErrorDialog(NULL, "Couldn't allocate the space for the new map.");
        return;
    }

    wsscanf(sf->edit[0], pMap->name);

    sprintf(buf, "Stored new map as %s.", pMap->name);
    send_line(buf);
}

void store_current_map(Widget w, char *cmd, XtPointer call_data)
{
    MAP *pOld = NULL;
    Widget rc, label;
    StdForm *sf;

    void wprintf();

    while (!XtIsWMShell(w))
        w = XtParent(w);

    if (vP->mode == SHOW_POSPOS || vP->mode == SHOW_VELPOS ||
        vP->mode == SHOW_POSVEL) pOld = vP->m;

    if (!pOld) {
        PostErrorDialog(w, "There is no current map to store!\n");
        return;
    }

    sf = PostStdFormDialog(w, "Store current map",
             BUTT_APPLY, (XtCallbackProc)get_map_name, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             1, NULL);

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation,       XmVERTICAL,
                                 NULL);
    label = XtCreateManagedWidget("Store map as:", xmLabelWidgetClass,
                                  rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                  rc, NULL, 0);
    
    ArrangeStdFormDialog(sf, rc);

    wprintf(sf->edit[0], "%s",  pOld->name);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void DoMultipleMaps(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int n, nv;
    double v1, v2, v0, dv;
    string buf, base_name, map_name;
    MAP *m;
    
    void wsscanf(), wdscanf(), wiscanf(), send_line();
    int SetSingleMomentBox();
    void set_show_mode();
    
    wsscanf(sf->edit[0], base_name);
    wdscanf(sf->edit[1], &v0);
    wdscanf(sf->edit[2], &dv);
    wiscanf(sf->edit[3], &nv);
    
    for (n=0; n<nv; n++) {
        v1 = v0 + (double)n * dv;
        v2 = v1 + dv;
        if (SetSingleMomentBox(UNIT_VEL, v1, v2)) {
            sprintf(buf, "Invalid interval [%f - %f].", v1, v2);
            PostErrorDialog(w, buf);
            continue;
        }
        set_show_mode(NULL, "contour", NULL);
        if (!vP->m) {
            sprintf(buf, "No current map!");
            PostErrorDialog(w, buf);
            continue;
        }
        strcpy(map_name, base_name);
        sprintf(map_name, "%s %6.1f-%6.1f", base_name, v1, v2);
        if ((m = copy_map(&maplist, vP->m)) == NULL) {
            sprintf(buf, "Out of memory, couldn't save map.");
            send_line(buf);
            continue;
        }
        strcpy(m->name, map_name);
        sprintf(buf, "Saved new map as '%s'.", m->name);
        send_line(buf);
    }
}

void StoreMultipleMaps(Widget w, char *cmd, XtPointer call_data)
{
    int n;
    Widget rc;
    StdForm *sf;

    void wprintf();

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Store multiple maps",
             BUTT_APPLY, (XtCallbackProc)DoMultipleMaps, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             nSMM, NULL);

    rc = XtVaCreateManagedWidget("rc", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 XmNnumColumns, nSMM,
                                 XmNadjustLast, False,
                                 XmNpacking, XmPACK_COLUMN,
                                 NULL);

    for (n=0; n<nSMM; n++) {
        XtCreateManagedWidget(smm_label[n], xmLabelWidgetClass,
                              rc, NULL, 0);
        sf->edit[n] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                            rc, NULL, 0);
    }
    
    ArrangeStdFormDialog(sf, rc);
    
    if (vP->m) {
        wprintf(sf->edit[0], "%s", vP->m->name);
        wprintf(sf->edit[1], "%f", vP->m->v);
        wprintf(sf->edit[2], "%f", vP->m->dv);
        wprintf(sf->edit[3], "%d", 1);
    } else if (vP->s) {
        wprintf(sf->edit[0], "%s", vP->s->name);
        wprintf(sf->edit[1], "%f", vP->s->vel0);
        wprintf(sf->edit[2], "%f", vP->s->velres);
        wprintf(sf->edit[3], "%d", vP->s->nChan);
    } else {
        wprintf(sf->edit[0], "%s", "<change me>");
        wprintf(sf->edit[1], "%f", 0.0);
        wprintf(sf->edit[2], "%f", 0.0);
        wprintf(sf->edit[3], "%d", 1);
    }
    
    ManageDialogCenteredOnPointer(sf->form);
}

static char *MapListing(MAP *m, int i)
{
    string s="";
    static string txt;

    if (!m)
        return NULL;

    if (m->swapped)
        strcpy(s, "Swapped");

    if (m->type == MAP_POSPOS) {
        sprintf(txt, "%4d %s PosxPos=%dx%d %s", i+1, s, m->i_no, m->j_no,
                m->name);
    } else if (m->type == MAP_VELPOS) {
        sprintf(txt, "%4d %s VelxPos=%dx%d %s", i+1, s, m->i_no, m->j_no,
                m->name);
    } else if (m->type == MAP_POSVEL) {
        sprintf(txt, "%4d %s PosxVel=%dx%d %s", i+1, s, m->i_no, m->j_no,
                m->name);
    } else {
        sprintf(txt, "%4d %s Unknown=%dx%d %s", i+1, s, m->i_no, m->j_no,
                m->name);
    }

    return txt;
}

static XmString *GetMapListStrings(int *nmaps)
{
    int n, nm;
    XmString *xmstr;
    list curr = NULL;
    string buf;
        
    nm = count_maps();
    if (nmaps) *nmaps = nm;
    
    if (nm == 0) return NULL;
    
    xmstr = (XmString *) XtMalloc(nm * sizeof(XmString));
    if (!xmstr) return NULL;
    
    n = 0;
    while ( (curr = map_iterator(curr)) != NULL) {
        strcpy(buf, MapListing((MAP *)DATA(curr), n));
        xmstr[n] = MKSTRING(buf);
        n++;
    }
    
    return xmstr;
}

static void CleanupMapListStrings(XmString *xmstr, int nmaps)
{
    int n;
    
    if (xmstr) {
        n = nmaps;
        while (n > 0) XmStringFree(xmstr[--n]);
        
        XtFree((char *)xmstr);
    }
}

static void UpdateMapListDialog(Widget listD)
{
    int nmaps;
    XmString *xmstr;
    
    if (!listD) return;
    
    xmstr = GetMapListStrings(&nmaps);
    
    XmListDeselectAllItems(listD);
    XtVaSetValues(listD,
                  XmNitemCount, nmaps,
                  XmNitems, xmstr,
                  XmNvisibleItemCount, (nmaps > 10 ? 10 : nmaps),
                  NULL);
    
    CleanupMapListStrings(xmstr, nmaps);
}

static void MapDelete(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, i, nL;
    string buf;
    list *pL;

    void send_line();
    list *map_delete();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    i = 0;
    for (n=0; n<nL; n++) {
        if (vP->m && vP->m == (MAP *)DATA(pL[n])) {
            sprintf(buf, 
                "Warning: Removed map '%s' which is used in current display.", 
                    vP->m->name);
            PostWarningDialog(NULL, buf);
            vP->m = NULL;
        }
        ZeroOriginalMapInMapList((MAP *)DATA(pL[n]));
        if (!map_delete(&maplist, pL[n])) {
            sprintf(buf, "Warning! Couldn't remove map %d.", i+1);
            PostErrorDialog(NULL, buf);
        } else {
            i++;
        }
    }

    free(pL);

    UpdateMapListDialog(sf->edit[1]);

    if (i == nL)
        sprintf(buf, "Deleted all %d selected maps.", nL);
    else
        sprintf(buf, "Deleted only %d out of %d selected maps.", i, nL);
    send_line(buf);
}

static void MapCopy(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, i, nL;
    string buf;
    list *pL;
    MAP *m, *old;

    void send_line();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    i = 0;
    for (n=0; n<nL; n++) {
        old = (MAP *)DATA(pL[n]);
        m = copy_map(&maplist, old);
        if (!m) continue;
        sprintf(m->name, "%s %s", "Copy of", old->name);
        i++;
    }

    free(pL);

    UpdateMapListDialog(sf->edit[1]);

    if (i == nL)
        sprintf(buf, "Copying all %d selected maps.", nL);
    else
        sprintf(buf, "Copying only %d out of %d selected maps.", i, nL);

    send_line(buf);
}

void blank_map_pixel(double x, double y)
{
    int i, j;
    MAP *m = vP->m;
    
    if (!m) return;
    
    i = NINT((x - m->xleft)/m->xspacing);
    if (i < 0 || i >= m->i_no) return;
    
    j = NINT((y - m->ylower)/m->yspacing);
    if (j < 0 || j >= m->j_no) return;
    
    m->f[i][j] = BLANK_TMP;
}

static void blank_map(MAP *m, int action)
{
    int i, j, nX = m->i_no, nY=m->j_no, inside = 0;
    Point p;
    
    int InsidePolyLines(Point *);

    for (i=0; i<nX; i++) {
        p.x = m->xleft + (double)i * m->xspacing;
        for (j=0; j<nY; j++) {
            p.y = m->ylower + (double)j * m->yspacing;
            if (action > UNBLANK) inside = InsidePolyLines(&p);
            if (m->f[i][j] == BLANK) continue;
            switch (action) {
                case UNBLANK:
                    if (m->f[i][j] == BLANK_TMP) m->f[i][j] = UNBLANK;
                    break;
                case BLANK_TMP:
                    if (fabs(m->d[i][j]) < 3.0 * (m->e[i][j]))
                        m->f[i][j] = BLANK_TMP;
                    break;
                case UNBLANK_INSIDE_POLYLINES:
                    if (!inside) break;
                    if (m->f[i][j] == BLANK_TMP) m->f[i][j] = UNBLANK;
                    break;
                case BLANK_INSIDE_POLYLINES:
                    if (inside) m->f[i][j] = BLANK_TMP;
                    break;
                case UNBLANK_OUTSIDE_POLYLINES:
                    if (inside) break;
                    if (m->f[i][j] == BLANK_TMP) m->f[i][j] = UNBLANK;
                    break;
                case BLANK_OUTSIDE_POLYLINES:
                    if (!inside) m->f[i][j] = BLANK_TMP;
                    break;
                default:
                    return;
            }
        }
    }
}

static MAP *ave_map(MAP *o, int action)
{
    int i, j, nX = o->i_no, nY=o->j_no, nAvg, inside = 0;
    double s1, s2;
    Point p;
    MAP *m = NULL;
    
    int InsidePolyLines(Point *);
    
    m = copy_map(get_maplist(), o);
    if (!m) return NULL;

    if (action == AVE_X_IN || action == AVE_X_OUT) {
	for (j=0; j<nY; j++) {
            p.y = m->ylower + (double)j * m->yspacing;
	    s1 = 0.0; s2= 0.0; nAvg = 0;
            for (i=0; i<nX; i++) {
                p.x = m->xleft + (double)i * m->xspacing;
		if (o->f[i][j] <= BLANK) continue;
        	inside = InsidePolyLines(&p);
		if (action == AVE_X_IN && inside) {
		    s1 += o->d[i][j];
		    s2 += o->d[i][j]*o->d[i][j];
		    nAvg++;
		} else if (action == AVE_X_OUT && !inside) {
		    s1 += o->d[i][j];
		    s2 += o->d[i][j]*o->d[i][j];
		    nAvg++;
		}
            }
	    if (nAvg > 0) {
	        for (i=0; i<nX; i++) {
		    m->d[i][j] = s1/(double)nAvg;
		    if (nAvg > 1)
		        m->e[i][j] = sqrt(
			        (s2 - s1*s1/(double)nAvg)/(double)(nAvg-1));
		    else
		        m->e[i][j] = m->e[i][j];
		    m->f[i][j] = UNBLANK;
		}
	    } else {
	        for (i=0; i<nX; i++) {
		    m->d[i][j] = o->d[i][j];
		    m->e[i][j] = o->e[i][j];
		    m->f[i][j] = BLANK;
		}
	    }
	}
    } else {
	for (i=0; i<nX; i++) {
            p.x = m->xleft + (double)i * m->xspacing;
	    s1 = 0.0; s2= 0.0; nAvg = 0;
            for (j=0; j<nY; j++) {
        	p.y = m->ylower + (double)j * m->yspacing;
		if (o->f[i][j] <= BLANK) continue;
        	inside = InsidePolyLines(&p);
		if (action == AVE_Y_IN && inside) {
		    s1 += o->d[i][j];
		    s2 += o->d[i][j]*o->d[i][j];
		    nAvg++;
		} else if (action == AVE_Y_OUT && !inside) {
		    s1 += o->d[i][j];
		    s2 += o->d[i][j]*o->d[i][j];
		    nAvg++;
		}
            }
	    if (nAvg > 0) {
	        for (j=0; j<nY; j++) {
		    m->d[i][j] = s1/(double)nAvg;
		    if (nAvg > 1)
		        m->e[i][j] = sqrt(
			        (s2 - s1*s1/(double)nAvg)/(double)(nAvg-1));
		    else
		        m->e[i][j] = m->e[i][j];
		    m->f[i][j] = UNBLANK;
		}
	    } else {
	        for (j=0; j<nY; j++) {
		    m->d[i][j] = 0.0;
		    m->e[i][j] = 0.0;
		    m->f[i][j] = BLANK;
		}
	    }
	}
    }
    
    sprintf(m->name, "Avg %s", o->name);
    
    return m;
}

void MapDraw(Widget w, MAP *m, XmListCallbackStruct *cb)
{
    void set_map_minmax();
    void SetStdView(), draw_main(), SetDefWindow();

    if (!m) return;
    
    set_map_minmax(m);

    if (m->type == MAP_VELPOS) {
        SetViewMode(SHOW_VELPOS, vP->s, m, vP->p);
    } else if (m->type == MAP_POSVEL) {
        SetViewMode(SHOW_POSVEL, vP->s, m, vP->p);
    } else {
        SetViewMode(SHOW_POSPOS, vP->s, m, vP->p);
    }
    strcpy(vP->t_label, m->name);
    strcpy(vP->x_label, "x-axis");
    strcpy(vP->y_label, "y-axis");
    vP->xunit = m->unit;
    SetStdView();
    SetDefWindow(SCALE_BOTH);
    
    vP->nMaps = 0;
    vP->M = NULL;
    
    draw_main();
}

static void MapBlank(Widget w, char *str, XmListCallbackStruct *cb)
{
    int n, nL, action=0;
    string buf;
    list *pL;

    void send_line();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;
        
    sprintf(buf, "%s the %d selected maps?", str, nL);
    if (nL <= 0 || !PostQuestionDialog(w, buf)) {
        free(pL);
        return;
    }

    if (strcmp(str, "Blank") == 0) {
        action = BLANK_TMP;
    } else if (strcmp(str, "Unblank")==0) {
        action = UNBLANK;
    } else if (strcmp(str, "UnblankInside")==0) {
        action = UNBLANK_INSIDE_POLYLINES;
    } else if (strcmp(str, "UnblankOutside")==0) {
        action = UNBLANK_OUTSIDE_POLYLINES;
    } else if (strcmp(str, "BlankInside")==0) {
        action = BLANK_INSIDE_POLYLINES;
    } else if (strcmp(str, "BlankOutside")==0) {
        action = BLANK_OUTSIDE_POLYLINES;
    } else {
        PostErrorDialog(w, "No such (un)blanking action.");
        free(pL);
        return;
    }

    for (n=0; n<nL; n++)
        blank_map((MAP *)DATA(pL[n]), action);

    if (action == BLANK_TMP)
        sprintf(buf, "Blanked |data| < 3sigma in %d selected maps.", nL);
    else if (action == UNBLANK)
        sprintf(buf, "Unblanked |data| < 3sigma in %d selected maps.", nL);
    else if (action == UNBLANK_INSIDE_POLYLINES)
        sprintf(buf, "Unblanked data inside polylines in %d selected maps.",
                nL);
    else if (action == UNBLANK_OUTSIDE_POLYLINES)
        sprintf(buf, "Unblanked data outside polylines in %d selected maps.",
                nL);
    else if (action == BLANK_INSIDE_POLYLINES)
        sprintf(buf, "Blanked data inside polylines in %d selected maps.", nL);
    else if (action == BLANK_OUTSIDE_POLYLINES)
        sprintf(buf, "Blanked data outside polylines in %d selected maps.", nL);

    send_line(buf);
    
    if (nL == 1) MapDraw(w, (MAP *)DATA(pL[0]), cb);

    free(pL);
}

static void MapAve(Widget w, char *str, XmListCallbackStruct *cb)
{
    int n, nL, action=0;
    string buf;
    list *pL;
    MAP *m=NULL;

    void send_line();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;
        
    sprintf(buf, "%s the %d selected maps?", str, nL);
    if (nL <= 0 || !PostQuestionDialog(w, buf)) {
        free(pL);
        return;
    }

    if (strcmp(str, "AveXinside") == 0) {
        action = AVE_X_IN;
    } else if (strcmp(str, "AveYinside")==0) {
        action = AVE_Y_IN;
    } else if (strcmp(str, "AveXoutside")==0) {
        action = AVE_X_OUT;
    } else if (strcmp(str, "AveYoutside")==0) {
        action = AVE_Y_OUT;
    } else {
        PostErrorDialog(w, "No such map averaging action.");
        free(pL);
        return;
    }

    for (n=0; n<nL; n++)
        m = ave_map((MAP *)DATA(pL[n]), action);

    if (action == AVE_X_IN)
        sprintf(buf, "x-average inside polylines %d selected maps.", nL);
    else if (action == AVE_Y_IN)
        sprintf(buf, "y-average inside polylines %d selected maps.", nL);
    else if (action == AVE_X_OUT)
        sprintf(buf, "x-average outside polylines %d selected maps.", nL);
    else if (action == AVE_Y_OUT)
        sprintf(buf, "y-average outside polylines %d selected maps.", nL);

    send_line(buf);
    
    if (nL == 1) MapDraw(w, m, cb);

    free(pL);
}

static void MapSwap(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    string buf;
    list *pL;
    MAP *m;
    double **tmp;

    void send_line();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    for (n=0; n<nL; n++) {
        m = (MAP *)DATA(pL[n]);

        tmp = m->e;
        m->e = m->d;
        m->d = tmp;

        if (m->swapped)
            m->swapped = 0;
        else
            m->swapped = 1;
    }

    free(pL);

    UpdateMapListDialog(sf->edit[1]);

    sprintf(buf, "Swapping %d selected map(s).", nL);
    send_line(buf);
}

static double holofunc(double p[], double x, double y,
                       double Fprim, double Fm)
{
    double z, r2, A, P, d;
    
    r2 = x*x + y*y;
    
    A = r2/4.0/Fprim/Fprim;
    P = r2/4.0/Fm/Fm;
    d = 4.0 * M_PI * (A/(1.0+A) + P/(1.0+P));
    
    z = p[0] + p[1]*x + p[2]*y;
    
    z += p[3]*d + p[5]*x*x + p[6]*y*y + p[7]*x*y;
    
    z += p[8]*r2*x + p[9]*r2*y;
    
    return z;
}

static MAP *HoloFit(MAP *o)
{
    int i, j, npix, n, fit[10];
    double Fprim, Fmag, Fm, alambda, r2;
    double x, y, s1, s2;
    double mean=0.0, sigma=0.0;
    double p[10], q[10];
    MAP *m = NULL;
    Holography *h = gp->hp;

    int StdMapFitter();
    extern void lm_Holo();
    
    m = copy_map(get_maplist(), o);
    if (!m) return NULL;
        
    if (!m->swapped) {
      for (i=0; i<m->i_no; i++) {
	for (j=0; j<m->j_no; j++) {
    	    m->d[i][j] = o->e[i][j];
    	    m->e[i][j] = o->d[i][j];
	}
      }
    }
    
    m->swapped = 1;
    
    Fprim = h->Fprim;
    Fmag = h->Fmag;
    Fm = Fprim*Fmag;
    
    alambda = SPEEDOFLIGHT*1.0e3/m->fMHz/1.0e6;
    
    for (n=0; n<10; n++) {
        p[n] = h->p[n];
	q[n] = h->q[n];
	fit[n] = h->fit[n];
    }
    
    p[3] /= alambda;
    p[5] /= (alambda * h->TransDist / M_PI);
    p[6] /= (alambda * h->TransDist / M_PI);
    p[7] /= (alambda * h->TransDist / M_PI);
    
    s1 = s2 = 0.0; npix=0;
    for (i=0; i<m->i_no; i++) {
      for (j=0; j<m->j_no; j++) {
	if (m->f[i][j] <= BLANK) continue;
	s1 += m->d[i][j];
	s2 += m->d[i][j] * m->d[i][j];
	npix++;
      }
    }
    
    if (npix > 1) {
      mean = s1/(double)npix;
      sigma = sqrt((s2 - s1*s1/(double)npix)/(double)(npix-1));
      printf("\nBefore fit: mean=%f rad  sigma=%f rad\n", mean, sigma);
    }
    
    StdMapFitter(m, 5.0/RADTODEG, NULL, p, fit, q, 10, lm_Holo);
   
    for (i=0; i<m->i_no; i++) {
      x = m->xleft + (double)i * m->xspacing;
      for (j=0; j<m->j_no; j++) {
        y = m->ylower + (double)j * m->yspacing;
	if (m->f[i][j] <= BLANK) continue;
	r2 = x*x + y*y;
	m->d[i][j] -= holofunc(p, x, y, Fprim, Fm);
	/* m->e[i][j] = m->d[i][j]*alambda/4.0/M_PI*1.0e6*
	             sqrt(1.0 + r2/4.0/Fprim/Fprim); */
      }
    }
    
    s1 = s2 = 0.0; npix=0;
    for (i=0; i<m->i_no; i++) {
      x = m->xleft + (double)i * m->xspacing;
      for (j=0; j<m->j_no; j++) {
    	y = m->ylower + (double)j * m->yspacing;
        r2 = x*x + y*y;
    	if (NINT(h->Mask)) {
          if (r2 > h->Ro * h->Ro) m->f[i][j] = BLANK_TMP;
          if (r2 < h->Ri * h->Ri) m->f[i][j] = BLANK_TMP;
          if (fabs(x) < h->Rq || fabs(y) < h->Rq) m->f[i][j] = BLANK_TMP;
    	}
        if (m->f[i][j] <= BLANK) continue;
        s1 += m->d[i][j];
        s2 += m->d[i][j] * m->d[i][j];
        npix++;
      }
    }
    
    p[3] *= alambda;
    q[3] *= alambda;
    p[5] *= alambda * h->TransDist / M_PI;
    q[5] *= alambda * h->TransDist / M_PI;
    p[6] *= alambda * h->TransDist / M_PI;
    q[6] *= alambda * h->TransDist / M_PI;
    p[7] *= alambda * h->TransDist / M_PI;
    q[7] *= alambda * h->TransDist / M_PI;
    
    for (n=0; n<10; n++) {
        h->p[n] = p[n];
	h->q[n] = q[n];
    }
        
    printf("Constant: %f (%f)\n", p[0], q[0]);
    printf("x: %f (%f) \n", p[1], q[1]);
    printf("y: %f (%f)\n", p[2], q[2]);
    printf("focus: %f (%f)\n", p[3], q[3]);
    /* printf("diffr: %f (%f)\n", p[4], q[4]); */
    printf("x*x: %f (%f)\n", p[5], q[5]);
    printf("y*y: %f (%f)\n", p[6], q[6]);
    printf("x*y: %f (%f)\n", p[7], q[7]);
    printf("r2*x: %f (%f)\n", p[8], q[8]);
    printf("r2*y: %f (%f)\n", p[9], q[9]);
    
    if (npix > 1) {
      mean = s1/(double)npix;
      sigma = sqrt((s2 - s1*s1/(double)npix)/(double)(npix-1));
      printf("After fit: mean=%f rad sigma=%f rad\n", mean, sigma);
      sprintf(m->molecule, "\\gs=%.1f \\gmm", sigma*alambda/4.0/M_PI*1.0e6);
    }
    h->sigma = sigma*alambda/4.0/M_PI*1.0e6;
    
    return m;
}

static void unwrap_phase_map(MAP *m)
{
    int i, j, k, l, I, J;
    double phi;
    
    /* Make phase continous within +/- pi */
    
    for (i=m->i_no/4; i<m->i_no-1; i++) {
     for (j=m->j_no/4; j<m->j_no-1; j++) {
       if (m->f[i][j] <= BLANK) continue;
       phi = m->e[i][j];
       for (k=-1; k<=1; k++) {
         for (l=-1; l<=1; l++) {
	   I = i+k; J=j+l;
	   if (m->f[I][J] <= BLANK) continue;
	   if (m->e[I][J] - phi >  M_PI) m->e[I][J] -= 2.0*M_PI;
	   if (m->e[I][J] - phi < -M_PI) m->e[I][J] += 2.0*M_PI;
         }
       }
     }
     for (j=m->j_no/4-1; j>0; j--) {
       if (m->f[i][j] <= BLANK) continue;
       phi = m->e[i][j];
       for (k=-1; k<=1; k++) {
         for (l=-1; l<=1; l++) {
	   I = i+k; J=j+l;
	   if (m->f[I][J] <= BLANK) continue;
	   if (m->e[I][J] - phi >  M_PI) m->e[I][J] -= 2.0*M_PI;
	   if (m->e[I][J] - phi < -M_PI) m->e[I][J] += 2.0*M_PI;
          }
       }
     }
    }
    for (i=m->i_no/4-1; i>0; i--) {
     for (j=m->j_no/4; j<m->j_no-1; j++) {
       if (m->f[i][j] <= BLANK) continue;
       phi = m->e[i][j];
       for (k=-1; k<=1; k++) {
         for (l=-1; l<=1; l++) {
	   I = i+k; J=j+l;
	   if (m->f[I][J] <= BLANK) continue;
	   if (m->e[I][J] - phi >  M_PI) m->e[I][J] -= 2.0*M_PI;
	   if (m->e[I][J] - phi < -M_PI) m->e[I][J] += 2.0*M_PI;
         }
       }
     }
     for (j=m->j_no/4-1; j>0; j--) {
       if (m->f[i][j] <= BLANK) continue;
       phi = m->e[i][j];
       for (k=-1; k<=1; k++) {
         for (l=-1; l<=1; l++) {
	   I = i+k; J=j+l;
	   if (m->f[I][J] <= BLANK) continue;
	   if (m->e[I][J] - phi >  M_PI) m->e[I][J] -= 2.0*M_PI;
	   if (m->e[I][J] - phi < -M_PI) m->e[I][J] += 2.0*M_PI;
          }
       }
     }
    }
}

static MAP *NearFieldDefocus(MAP *o)
{
    int i, j;
    double a, p, span, legw;
    double Fprim, Fmag, Fm, alambda, r2;
    double resol, wk, phi, x, y;
    MAP *m = NULL;
    Holography *h = gp->hp;
    
    m = copy_map(get_maplist(), o);
    if (!m) return NULL;
    
    for (i=0; i<m->i_no; i++) {
      for (j=0; j<m->j_no-i; j++) {
    	  a = m->d[i][j];
    	  p = m->e[i][j];
    	  m->d[i][j] = m->d[m->i_no - 1 - i][m->j_no - 1 - j];
    	  m->e[i][j] = m->e[m->i_no - 1 - i][m->j_no - 1 - j];
    	  m->d[m->i_no - 1 - i][m->j_no - 1 - j] = a;
    	  m->e[m->i_no - 1 - i][m->j_no - 1 - j] = p;
      }
    }
    for (i=0; i<m->i_no/2; i++) {
      j = m->j_no - 1 - i;
      a = m->d[i][j];
      p = m->e[i][j];
      m->d[i][j] = m->d[m->i_no - 1 - i][m->j_no - 1 - j];
      m->e[i][j] = m->e[m->i_no - 1 - i][m->j_no - 1 - j];
      m->d[m->i_no - 1 - i][m->j_no - 1 - j] = a;
      m->e[m->i_no - 1 - i][m->j_no - 1 - j] = p;
    }
    
    Fprim = h->Fprim;
    Fmag = h->Fmag;
    Fm = Fprim*Fmag;
    legw = h->QuadWidth;
    
    span = h->Dprim / h->NyRate;
    
    m->xspacing = span/(double)(m->i_no-1);
    m->xleft  = -span/2.0;
    m->xright = span/2.0;
    m->yspacing = span/(double)(m->j_no-1);
    m->ylower = -span/2.0;
    m->yupper = span/2.0;
   
    for (i=0; i<m->i_no; i++) {
      x = m->xleft + (double)i * m->xspacing;
      for (j=0; j<m->j_no; j++) {
        y = m->ylower + (double)j * m->yspacing;
	m->e[i][j] *= -1.0;
	r2 = x*x + y*y;
	if (r2  > h->Dprim * h->Dprim/4.0) m->f[i][j] = BLANK;
	if (r2 <= h->Dseco * h->Dseco/4.0) m->f[i][j] = BLANK;
      }
    }
    
    alambda = SPEEDOFLIGHT*1.0e3/m->fMHz/1.0e6;
    resol = h->Dprim/h->NyRate/(double)(m->i_no-1);
    wk = 2.0*M_PI/alambda/h->TransDist;

    for (i=0; i<m->i_no; i++) {
      x = m->xleft + (double)i * m->xspacing;
      for (j=0; j<m->j_no; j++) {
        y = m->ylower + (double)j * m->yspacing;
	r2 = x*x + y*y;
	
	/* Nearfield correction to phase */
	phi = wk*r2/2.0;
	m->e[i][j] += phi;
	
	/* Defocussing correction to phase */
	a = r2/4.0/Fprim/Fprim;
	p = r2/4.0/Fm/Fm;
	phi = 4.0*M_PI/alambda*h->Defocus*(a/(1.0 + a) + p/(1.0 + p));
	m->e[i][j] -= phi;
	
      }
    }

    unwrap_phase_map(m);
    
    for (i=0; i<m->i_no; i++) {
      x = m->xleft + (double)i * m->xspacing;
      for (j=0; j<m->j_no; j++) {
        y = m->ylower + (double)j * m->yspacing;
	if (fabs(x) <= legw || fabs(y) <= legw) m->f[i][j] = BLANK;
      }
    }
   
    return m;
}

static void MapHoloFit(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL, i=0;
    string buf;
    list *pL;
    MAP *m, *o;

    void send_line();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    for (n=0; n<nL; n++) {
        o = (MAP *)DATA(pL[n]);
	m = HoloFit(o);
        if (m) {
            i++;
            sprintf(m->name, "Fitted %s", o->name);
        } else
            break;
    }

    free(pL);

    UpdateMapListDialog(sf->edit[1]);

    sprintf(buf, "Fitted %d selected map(s).", i);
    send_line(buf);
}

static void MapHoloCorrect(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL, i=0;
    string buf;
    list *pL;
    MAP *m, *o;

    void send_line();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    for (n=0; n<nL; n++) {
        o = (MAP *)DATA(pL[n]);
	m = NearFieldDefocus(o);
        if (m) {
            i++;
            sprintf(m->name, "Holo %s", o->name);
        } else
            break;
    }

    free(pL);

    UpdateMapListDialog(sf->edit[1]);

    sprintf(buf, "Holo corrected %d selected map(s).", i);
    send_line(buf);
}

static void MapFFTShift(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL, i, j;
    string buf;
    double a, p;
    list *pL;
    MAP *m;

    void send_line();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    for (n=0; n<nL; n++) {
        m = (MAP *)DATA(pL[n]);
	for (j=0; j<m->j_no; j++) {
	  for (i=0; i<m->i_no/2; i++) {
	    a = m->d[i][j];
	    p = m->e[i][j];
	    m->d[i][j] = m->d[i+m->i_no/2][j];
	    m->e[i][j] = m->e[i+m->i_no/2][j];
	    m->d[i+m->i_no/2][j] = a;
            m->e[i+m->i_no/2][j] = p;
	  }
	}
	for (i=0; i<m->i_no; i++) {
	  for (j=0; j<m->j_no/2; j++) {
	    a = m->d[i][j];
	    p = m->e[i][j];
	    m->d[i][j] = m->d[i][j+m->j_no/2];
	    m->e[i][j] = m->e[i][j+m->j_no/2];
	    m->d[i][j+m->j_no/2] = a;
            m->e[i][j+m->i_no/2] = p;
	  }
	}
    }

    free(pL);

    UpdateMapListDialog(sf->edit[1]);

    sprintf(buf, "FFT shifting %d selected map(s).", nL);
    send_line(buf);
}

static MAP *DoMapFFT(char how, MAP *old)
{
    MAP *new;
    
    MAP *fft_map(char *, MAP *);
    
    if (how == 'f' || how == 'F') {
        new = fft_map("p", old);
    } else {
        new = fft_map("i", old);
    }
    
    return new;
}

static void MapFFT(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, i=0, nL;
    string buf;
    list *pL;
    MAP *o, *m;

    void send_line();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    for (n=0; n<nL; n++) {
        o = (MAP *)DATA(pL[n]);
        m = DoMapFFT('f', o);
        if (m) {
            i++;
            sprintf(m->name, "Forward FFT of %s", o->name);
        } else
            break;
    }

    free(pL);

    UpdateMapListDialog(sf->edit[1]);
    
    if (i > 0) {
        sprintf(buf, "Calculated forward FFT of %d map(s).", nL);
        send_line(buf);
    }
}

static void MapiFFT(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, i=0, nL;
    string buf;
    list *pL;
    MAP *o, *m;

    void send_line();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    for (n=0; n<nL; n++) {
        o = (MAP *)DATA(pL[n]);
        m = DoMapFFT('i', o);
        if (m) {
            i++;
            sprintf(m->name, "Inverse FFT of %s", o->name);
        } else
            break;
    }

    free(pL);

    UpdateMapListDialog(sf->edit[1]);
    
    if (i > 0) {
        sprintf(buf, "Calculated inverse FFT of %d map(s).", nL);
        send_line(buf);
    }
}

static void replace_data(MAP *m)
{
    int i, j, nX=m->i_no, nY=m->j_no;

    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (m->f[i][j] == BLANK) continue;
            if (m->e[i][j] != 0.0)
                m->d[i][j] /= m->e[i][j];
            else {
                m->f[i][j] = BLANK;
                m->d[i][j] = UNDEF;
            }
        }
    }
}

static void MapReplace(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    string buf;
    list *pL;

    void send_line();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    for (n=0; n<nL; n++)
        replace_data((MAP *)DATA(pL[n]));

    free(pL);

    XtDestroyWidget(sf->form);

    sprintf(buf, "Replaced data with S/N in %d selected map(s).", nL);
    send_line(buf);
}

static void get_new_name(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    string buf;
    MAP *m = (MAP *)sf->any;

    void send_line(), wsscanf();

    wsscanf(sf->edit[0], m->name);

    sprintf(buf, "Map renamed as %s.\n", m->name);
    send_line(buf);
}

static void PostRenameDialog(Widget wid, MAP *m)
{
    Widget w = wid, rc;
    StdForm *sf;
    string title;

    void wprintf();

    while (!XtIsWMShell(w))
        w = XtParent(w);

    if (!m) {
        PostErrorDialog(w, "There is no such map to rename.");
        return;
    }
    sprintf(title, "Rename map %s", m->name);
    sf = PostStdFormDialog(w, title,
             BUTT_APPLY, (XtCallbackProc)get_new_name, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             1, NULL);
             
    sf->any = (XtPointer)m;

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    XtCreateManagedWidget("Name map as:", xmLabelWidgetClass,
                          rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);
    
    ArrangeStdFormDialog(sf, rc);

    wprintf(sf->edit[0], "%s",  m->name);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void MapRename(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    list *pL;

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    for (n=0; n<nL; n++)
        PostRenameDialog(gp->top, (MAP *)DATA(pL[n]));

    free(pL);

    XtDestroyWidget(sf->form);
}

static void Map2Cube(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, k, nL, nX=0, nY=0, mixed_maps=0;
    string buf, cubename;
    list *pL;
    MAP *m, **maps, *m1, *m2;

    void send_line(), wsscanf();
    int MakeFITSCubeOfMaps();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;
    
    wsscanf(sf->edit[0], cubename);

    /* Allocate memory for the array of map pointers */
    maps = (MAP **)malloc(nL * sizeof(MAP *));
    
    for (n=0; n<nL; n++) {
        m = (MAP *)DATA(pL[n]);
        if (n == 0) {
            nX = m->i_no;
            nY = m->j_no;
        } else {
            if (m->i_no != nX || m->j_no != nY) { /* More checks? */
                mixed_maps = 1;
                break;
            }
        }
        maps[n] = m;
    }
    
    if (mixed_maps) {
        free(maps);
        free(pL);
        sprintf(buf, "The selected %d maps are of different sizes!", nL);
        PostErrorDialog(w, buf);
        return;
    }
    
    if (!strlen(cubename)) {
        m = maps[0];
        strcpy(cubename, m->name);
    }
    
    /* Sort the maps with respect to increasing v */
    for (n=0; n<nL-1; n++) {
        for (k=n+1; k<nL; k++) {
            m1 = maps[n];
            m2 = maps[k];
            if (m1->v > m2->v) {
                maps[n] = m2;
                maps[k] = m1;
            }
        }
    }
    
    for (n=0; n<nL; n++) {
        m = maps[n];
        sprintf(buf, "Map %d: v = %f\n", n+1, m->v);
        send_line(buf); 
    }
    
    if (!strstr(cubename, ".fits")) strcat(cubename, ".fits");
    
    n = MakeFITSCubeOfMaps(cubename, maps, nL);

    free(maps);
    free(pL);
    
    if (n == 0) {
        sprintf(buf, "Selected %d maps saved as FITS cube %s.",
                nL, cubename);
        send_line(buf);
    } else {
        PostErrorDialog(w, "An error occurred. FITS cube not saved.");
    }
}

static void ChangeMap(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int i, j, old_i, old_j, old_nX, old_nY;
    int i_min, i_max, j_min, j_max, nX, nY;
    double dx, dy;
    string buf;
    int **f;
    double **d, **e;
    scanPtr **s;
    MAP *m, *o;

    void send_line(), wiscanf(), wdscanf();

    o = (MAP *)sf->any;
    if (!o) {
        PostErrorDialog(w, "Couldn't find the old map!");
        return;
    }
    
    wiscanf(sf->edit[0], &i_min);
    wiscanf(sf->edit[1], &i_max);
    wiscanf(sf->edit[2], &j_min);
    wiscanf(sf->edit[3], &j_max);
    wdscanf(sf->edit[4], &dx);
    wdscanf(sf->edit[5], &dy);
    
    nX = i_max - i_min + 1;
    nY = j_max - j_min + 1;
    old_nX = o->i_no;
    old_nY = o->j_no;
    
    m = new_map(&maplist, nX, nY);

    if (!m) {
        PostErrorDialog(w, "Couldn't allocate the space for the new map!");
        return;
    }

    d = m->d;
    e = m->e;
    f = m->f;
    s = m->sp;

    *m = *o;

    m->sp = s;
    m->d = d;
    m->e = e;
    m->f = f;
    
    m->i_no = nX;
    m->j_no = nY;
    m->i_min = i_min;
    m->j_min = j_min;
    m->i_max = i_max;
    m->j_max = j_max;
    m->xleft  = o->xleft  + (double)(i_min - o->i_min)*(m->xspacing);
    m->xright = o->xright + (double)(i_max - o->i_max)*(m->xspacing);
    m->ylower = o->ylower + (double)(j_min - o->j_min)*(m->yspacing);
    m->yupper = o->yupper + (double)(j_max - o->j_max)*(m->yspacing);
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            old_i = i + i_min - o->i_min;
            old_j = j + j_min - o->j_min;
            if (old_i < 0 || old_i >= old_nX || old_j < 0 || old_j >= old_nY) {
                m->f[i][j] = BLANK;
                m->d[i][j] = 0.0;
                m->e[i][j] = 0.0;
                m->sp[i][j] = NULL;
            } else {
                m->f[i][j] = o->f[old_i][old_j];
                m->d[i][j] = o->d[old_i][old_j];
                m->e[i][j] = o->e[old_i][old_j];
                m->sp[i][j] = o->sp[old_i][old_j];
            }
        }
    }
    if (dy != 0.0) {
        m->ylower -= dy;
        m->yupper -= dy;
        m->j_min = NINT(m->ylower/m->yspacing);
        m->j_max = m->j_min + nY - 1;
        m->y0 = o->y0 + dy/3600.0/RADTODEG;
    }
    if (dx != 0.0) {
        m->xleft  -= dx;
        m->xright -= dx;
        m->i_min = NINT(m->xleft/m->xspacing);
        m->i_max = m->i_min + nX - 1;
        m->x0 = o->x0 + dx/cos(o->y0)/3600.0/RADTODEG;
    }
    
    m->original = o;
    
    strcpy(m->name, "Modified ");
    strcat(m->name, o->name);

    sprintf(buf, "New map %d x %d created as %s.\n", nX, nY, m->name);
    send_line(buf);
}

static void PostChangeDialog(Widget wid, MAP *m)
{
    Widget w = wid, fr, rc, rc2;
    string title;
    StdForm *sf;

    void wprintf();
    Widget ThreeHorEdit(Widget, Widget *, Widget *, Widget *);

    while (!XtIsWMShell(w))
        w = XtParent(w);

    if (!m) {
        PostErrorDialog(w, "There is no such map to change!");
        return;
    }

    sprintf(title, "Change %s", m->name);
    sf = PostStdFormDialog(w, title,
             BUTT_APPLY, (XtCallbackProc)ChangeMap, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             6, NULL);
    
    sf->any = m;
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    XtCreateManagedWidget(m->name, xmLabelWidgetClass,
                          rc, NULL, 0);
    fr = XtVaCreateWidget("frame", xmFrameWidgetClass, rc,
				          XmNshadowType, XmSHADOW_OUT, NULL);

    rc2 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                  XmNorientation, XmVERTICAL,
                                  NULL);
    XtCreateManagedWidget("X pixel range (left and right):", xmLabelWidgetClass,
                                 rc2, NULL, 0);
    ThreeHorEdit(rc2, &(sf->edit[0]), &(sf->edit[1]), NULL);

    XtCreateManagedWidget("Y pixel range (lower and upper):", xmLabelWidgetClass,
                                 rc2, NULL, 0);
    ThreeHorEdit(rc2, &(sf->edit[2]), &(sf->edit[3]), NULL);

    XtCreateManagedWidget("Put new centre at offsets", xmLabelWidgetClass,
                                 rc2, NULL, 0);
    ThreeHorEdit(rc2, &(sf->edit[4]), &(sf->edit[5]), NULL);
    
    ArrangeStdFormDialog(sf, rc);
    
    XtManageChild(fr);

    wprintf(sf->edit[0], "%d",  m->i_min);
    wprintf(sf->edit[1], "%d",  m->i_max);
    wprintf(sf->edit[2], "%d",  m->j_min);
    wprintf(sf->edit[3], "%d",  m->j_max);
    wprintf(sf->edit[4], "%f", 0.0);
    wprintf(sf->edit[5], "%f", 0.0);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void MapChange(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    list *pL;

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    for (n=0; n<nL; n++)
        PostChangeDialog(gp->top, (MAP *)DATA(pL[n]));

    free(pL);

    XtDestroyWidget(sf->form);
}

static void precess_map(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    char c;
    string epo, buf;
    double e;
    MAP *m = NULL;
    
    void send_line(), wsscanf();
    void PrecessMap(MAP *, char *);
    
    m = (MAP *)sf->any;
    if (!m) {
        PostErrorDialog(w, "Couldn't find the map!");
        return;
    }
    
    wsscanf(sf->edit[0], epo);
    
    if (sscanf(epo, "%1c%lf", &c, &e) != 2) {
        sprintf(buf, "'%s' does not conform to JXXXX.x or BXXXX.x", epo);
        PostErrorDialog(w, buf);
        return;
    }
    
    if (c != 'J' && c != 'j' && c != 'B' && c != 'b') {
        sprintf(buf, "'%s' does not conform to the Julian or Besselian epochs",
                epo);
        PostErrorDialog(w, buf);
        return;
    }
    
    PrecessMap(m, epo);
    
    sprintf(buf, "Map %s precessed to %s.\n", m->name, epo);
    send_line(buf);
}

static void PostPrecessDialog(Widget wid, MAP *m)
{
    Widget w = wid, fr, rc, rc2;
    string title;
    StdForm *sf;

    void wprintf();
    Widget ThreeHorEdit(Widget, Widget *, Widget *, Widget *);

    while (!XtIsWMShell(w))
        w = XtParent(w);

    if (!m) {
        PostErrorDialog(w, "There is no such map to precess!");
        return;
    }

    sprintf(title, "Precess %s (%1c%6.1f)", m->name, m->epoch, m->equinox);
    sf = PostStdFormDialog(w, title,
             BUTT_APPLY, (XtCallbackProc)precess_map, NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, (XtPointer)PrecessMap_Help,
             1, NULL);
    
    sf->any = m;
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    XtCreateManagedWidget(title, xmLabelWidgetClass,
                          rc, NULL, 0);
    fr = XtVaCreateWidget("frame", xmFrameWidgetClass, rc,
				          XmNshadowType, XmSHADOW_OUT, NULL);

    rc2 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                  XmNorientation, XmVERTICAL,
                                  NULL);
    XtCreateManagedWidget("Epoch (JXXXX.X/BYYYY.Y):", xmLabelWidgetClass,
                                 rc2, NULL, 0);
    ThreeHorEdit(rc2, &(sf->edit[0]), NULL, NULL);
    
    ArrangeStdFormDialog(sf, rc);
    
    XtManageChild(fr);

    wprintf(sf->edit[0], "%1c%6.1f",  m->epoch, m->equinox);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void MapPrecess(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    list *pL;

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    for (n=0; n<nL; n++)
        PostPrecessDialog(gp->top, (MAP *)DATA(pL[n]));

    free(pL);

    XtDestroyWidget(sf->form);
}

void DualMapDraw(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int show_type;
    MapAriStruct *mas = (MapAriStruct *)sf->user;
    static MAP *m[2];
    
    void set_map_minmax();
    void SetStdView(), draw_main(), SetDefWindow();

    if (!mas) return;
    if (!mas->m1) {
        PostErrorDialog(w, "Couldn't find first map.");
        return;
    }
    if (!mas->m2) {
        PostErrorDialog(w, "Couldn't find second map.");
        return;
    }
    m[0] = mas->m1;
    m[1] = mas->m2;
    
    if (mas->m1->type != mas->m2->type) {
        PostErrorDialog(w, "The two maps are of different type.");
        return;
    }

    set_map_minmax(mas->m1);

    if (mas->m1->type == MAP_VELPOS) {
        show_type = SHOW_VELPOS;
    } else if (mas->m1->type == MAP_POSVEL) {
        show_type = SHOW_POSVEL;
    } else {
        show_type = SHOW_POSPOS;
    }
    SetViewMode(show_type, vP->s, mas->m1, vP->p);
    strcpy(vP->t_label, mas->m1->name);
    strcpy(vP->x_label, "x-axis");
    strcpy(vP->y_label, "y-axis");
    vP->xunit = mas->m1->unit;
    SetStdView();
    SetDefWindow(SCALE_BOTH);
    
    vP->nMaps = -2;
    vP->M = m;
    
    draw_main();
}

static void MapShow(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int nL, i, j;
    list *pL;
    static MAP **maps = NULL;
    MAP *m1, *m2;
    
    void draw_main();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    if (nL == 1) {
        MapDraw(w, (MAP *)DATA(pL[0]), cb);
        if (maps) free(maps);
        maps = NULL;
        free(pL);
        return;
    }
    
    /* Allocate memory for the array of map pointers */
    if (maps)
        maps = (MAP **)realloc(maps, nL * sizeof(MAP *));
    else
        maps = (MAP **)malloc(nL * sizeof(MAP *));
    
    for (i=0; i<nL; i++) {
        maps[i] = (MAP *)DATA(pL[i]);
    }
    
    /* Sort the maps with respect to increasing v */
    for (i=0; i<nL-1; i++) {
        for (j=i+1; j<nL; j++) {
            m1 = maps[i];
            m2 = maps[j];
            if (m1->v > m2->v) {
                maps[i] = m2;
                maps[j] = m1;
            }
        }
    }
    
    vP->nMaps = nL;
    vP->M = maps;

    free(pL);
    
    draw_main();
}

static char *get_maparith_name(MapAriStruct *mas)
{
    string op;
    static string buf;

    strcpy(op, op_string[mas->op]);

    if (mas->m1 && mas->m2) {
        sprintf(buf, "[%s] %s [%s]", mas->m1->name, op, mas->m2->name);
    } else if (mas->m1) {
        sprintf(buf, "[%s] %s <none>", mas->m1->name, op);
    } else if (mas->m2) {
        sprintf(buf, "<none> %s [%s]", op, mas->m2->name);
    } else {
        sprintf(buf, "<none> %s <none>", op);
    }

    return buf;
}

static void set_type_of_op(Widget w, intWidget *iw, XmAnyCallbackStruct *cb)
{
    MapAriStruct *mas = (MapAriStruct *)(iw->sf->user);

    void wprintf();

    mas->op = iw->n;
    wprintf(iw->sf->edit[0], "%s", get_maparith_name(mas));
}

static void MapGet(Widget w, mapDualWidget *mdw, XmListCallbackStruct *cb)
{
    int   n, nL;
    list *pL;
    MAP  *pm = NULL;
    MapAriStruct *mas = (mdw->sf) ? (MapAriStruct *)(mdw->sf->user) : NULL;

    void  wprintf();

    if ((pL = get_action_list(cb, &nL, maplist)) == NULL)
        return;

    for (n=0; n<nL; n++)
        pm = (MAP *)DATA(pL[n]);

    *(mdw->m) = pm;

    if (mdw->w)
        wprintf(mdw->w, "%s", pm->name);

    free(pL);
    if (mdw->sf)
        wprintf(mdw->sf->edit[0], "%s", get_maparith_name(mas));
}

static void ApplyOpOnMaps(MAP *m, MapAriStruct *mas)
{
    int i, j, nX=m->i_no, nY=m->j_no, unwrap=0;
    double *d, d1, d2;
    double *e, e1, e2;
    double x1, x2, y1, y2, x, y;
    int *f, f1, f2;

    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            d  = &(m->d[i][j]);
            e  = &(m->e[i][j]);
            f  = &(m->f[i][j]);
            m->sp[i][j] = NULL;
            d1 = mas->m1->d[i][j];
            d2 = mas->m2->d[i][j];
            e1 = mas->m1->e[i][j];
            e2 = mas->m2->e[i][j];
            f1 = mas->m1->f[i][j];
            f2 = mas->m2->f[i][j];
            if (f1 <= BLANK && f2 <= BLANK) {
                *f = BLANK;
                *d = UNDEF;
                *e = UNDEF;
                continue;
            }
            switch (mas->op) {
                case OP_AVE:
                    if (f1 <= BLANK) {
                        *d = d2;
                        *e = e2;
                    } else if (f2 <= BLANK) {
                        *d = d1;
                        *e = e1;
                    } else {
                        *d = (d1 + d2)/2.0;
                        if (e1 != 0.0 && e2 != 0.0)
                            *e = 1.0/sqrt(1.0/e1/e1 + 1.0/e2/e2);
                        else
                            *e = 0.0;
                    }
                    *f = 1;
                    continue;
            }
            if (f1 <= BLANK || f2 <= BLANK) {
                *f = BLANK;
                *d = UNDEF;
                *e = UNDEF;
                continue;
            }
            switch (mas->op) {
                case OP_ADD:
                    *d = d1 + d2;
                    *e = sqrt(e1*e1 + e2*e2);
                    break;
                case OP_SUB:
                    *d = d1 - d2;
                    *e = sqrt(e1*e1 + e2*e2);
                    break;
                case OP_MUL:
                    *d = d1 * d2;
                    *e = sqrt(d2*d2*e1*e1 + d1*d1*e2*e2);
                    break;
                case OP_DIV:
                    if (d2 != 0.0) {
                        *d = d1 / d2;
                        *e = sqrt(d2*d2*e1*e1 + d1*d1*e2*e2)/d2/d2;
                    } else {
                        *f = BLANK;
                        *d = UNDEF;
                        *e = UNDEF;
                    }
                    break;
                case OP_CHI:
                    if (e1 != 0.0 && nX*nY > 1) {
                        *d = (d1 - d2)*(d1 - d2)/e1/e1;
                        *e = *d/(double)(nX*nY - 1);
                    } else if (e2 != 0.0 && nX*nY > 1) {
                        *d = (d1 - d2)*(d1 - d2)/e2/e2;
                        *e = *d/(double)(nX*nY - 1);
                    } else {
                        *f = BLANK;
                        *d = UNDEF;
                        *e = UNDEF;
                    }
                    break;
                case OP_CPLADD: /* complex addition */
		    if (mas->m1->swapped) {
		      x1 = e1*cos(d1); y1 = e1*sin(d1);
                    } else {
		      x1 = d1*cos(e1); y1 = d1*sin(e1);
		    }
		    if (mas->m2->swapped) {
		      x2 = e2*cos(d2); y2 = e2*sin(d2);
		    } else {
		      x2 = d2*cos(e2); y2 = d2*sin(e2);
		    }
		    x = x1 + x2; y = y1 + y2;
		    *d = sqrt(x*x + y*y);
		    *e = atan2(y, x);
		    unwrap = 1;
                    break;
                case OP_CPLSUB: /* complex subtraction */
		    if (mas->m1->swapped) {
		      x1 = e1*cos(d1); y1 = e1*sin(d1);
                    } else {
		      x1 = d1*cos(e1); y1 = d1*sin(e1);
		    }
		    if (mas->m2->swapped) {
		      x2 = e2*cos(d2); y2 = e2*sin(d2);
		    } else {
		      x2 = d2*cos(e2); y2 = d2*sin(e2);
		    }
		    x = x1 - x2; y = y1 - y2;
		    *d = sqrt(x*x + y*y);
		    *e = atan2(y, x);
		    unwrap = 1;
                    break;
            }
            *f = 1;
        }
    }
    mas->m1 = NULL;
    mas->m2 = NULL;
    if (unwrap) {
      m->swapped = 0;
      unwrap_phase_map(m);
    }
}

static char wrongDimForm[] = {
    "The maps '%s' and '%s' don't have the same %c-dimension: %d and %d."
};

void DoMapCalc(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int nX, nY;
    double **d, **e;
    int **f;
    scanPtr **s;
    string buf;
    MAP *m;
    MapAriStruct *mas = (MapAriStruct *)(sf->user);

    void send_line(), wsscanf();

    if (!mas->m1) {
        PostErrorDialog(w, "Cannot find a first map.");
        return;
    }
    if (!mas->m2) {
        PostErrorDialog(w, "Cannot find a second map.");
        return;
    }
    nX = mas->m1->i_no;
    nY = mas->m1->j_no;
    if (nX != mas->m2->i_no) {
        sprintf(buf, wrongDimForm, mas->m1->name, mas->m2->name, 'x',
                nX, mas->m2->i_no);
        PostErrorDialog(w, buf);
        return;
    }
    if (nY != mas->m2->j_no) {
        sprintf(buf, wrongDimForm, mas->m1->name, mas->m2->name, 'y',
                nY, mas->m2->j_no);
        PostErrorDialog(w, buf);
        return;
    }

    m = new_map(&maplist, nX, nY);

    if (!m) {
        PostErrorDialog(w, "Couldn't allocate the space for the new map!");
        return;
    }

    d = m->d;
    e = m->e;
    f = m->f;
    s = m->sp;

    *m = *(mas->m1);

    m->sp = s;
    m->d = d;
    m->e = e;
    m->f = f;
    
    m->original = mas->m1;

    ApplyOpOnMaps(m, mas);

    wsscanf(sf->edit[0], m->name);

    XtDestroyWidget(sf->form);

    sprintf(buf, "New map stored as '%s'\n", m->name);
    send_line(buf);
}

static void create_maparith_dialog(Widget parent,
                                   Arg *argsLeft,  int nArgsLeft,
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
    MapAriStruct *mas;

    void wprintf();

    mas = (MapAriStruct *)XtMalloc(sizeof(MapAriStruct));
    if (!mas) {
        PostErrorDialog(parent, "Out of memory in map arithmetic.");
        return;
    }
    mas->op = OP_ADD;
    mas->m1 = mas->m2 = NULL;

    sprintf(newName, "%s %s %s", lStr, op_string[mas->op], rStr);

    sf = PostStdFormDialog(parent, "Map arithmetic",
             BUTT_APPLY, (XtCallbackProc)DoMapCalc, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             1, NULL);
    sf->user = (XtPointer)mas;
    
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
        if (mas->op == i)
            XtSetArg(wargs[0], XmNset, True);
	else
            XtSetArg(wargs[0], XmNset, False);
        op[i] = XtCreateWidget(op_string[i], xmToggleButtonWidgetClass,
                               radioBox, wargs, 1);
        mas->iw[i].n = op_types[i];
        mas->iw[i].sf = sf;
        XtAddCallback(op[i], XmNarmCallback,
                      (XtCallbackProc)set_type_of_op, &(mas->iw[i]));
    }

    rcRight = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rcH,
                                      XmNorientation, XmVERTICAL,
                                      NULL);

    rList = XmCreateScrolledList(rcRight, "rightlist", argsRight, nArgsRight);
    rLab = XtCreateManagedWidget(rStr, xmLabelWidgetClass, rcRight, NULL, 0);

    XtVaCreateManagedWidget("Name of new map:", xmLabelWidgetClass, rc,
                            NULL);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc, NULL, 0);

    mas->mwL.w  = lLab;
    mas->mwL.sf = sf;
    mas->mwL.m  = &(mas->m1);
    XtAddCallback(lList, XmNsingleSelectionCallback,
                  (XtCallbackProc)MapGet, &(mas->mwL));
    mas->mwR.w  = rLab;
    mas->mwR.sf = sf;
    mas->mwR.m  = &(mas->m2);
    XtAddCallback(rList, XmNsingleSelectionCallback,
                  (XtCallbackProc)MapGet, &(mas->mwR));
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(lList);
    XtManageChild(rList);
    XtManageChildren(op, NUMOPS);
    XtManageChild(radioBox);

    wprintf(sf->edit[0], "%s", newName);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void create_mapslist_dialog(Widget parent, char *cmd,
                                   Arg *args, int nargs)
{
    int has_edit = 0;
    string str;
    Widget rc, listDialog;
    StdForm *sf;
    
    void wprintf();
    char *GetWriteFITSDir();

    if (strcmp(cmd, "Cube") == 0) has_edit = 1;

    sf = PostStdFormDialog(parent, "Maps",
             NULL, NULL, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             2, NULL);

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    if (has_edit) {
        XtCreateManagedWidget("Enter file name:", xmLabelWidgetClass,
                                       rc, NULL, 0);
        sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                       rc, NULL, 0);
    }
    XtVaCreateManagedWidget("Select map(s) from list:", xmLabelWidgetClass,
                            rc, NULL);

    sf->edit[1] = listDialog = XmCreateScrolledList(rc, "list", args, nargs);

    if (strcmp(cmd, "Delete") == 0) {
        sprintf(str, "Delete map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapDelete, sf);
    } else if (strcmp(cmd, "Rename") == 0) {
        sprintf(str, "Rename map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapRename, sf);
    } else if (strcmp(cmd, "Copy") == 0) {
        sprintf(str, "Copy map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapCopy, sf);
    } else if (strcmp(cmd, "Change") == 0) {
        sprintf(str, "Change map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapChange, sf);
    } else if (strcmp(cmd, "Precess") == 0) {
        sprintf(str, "Precess map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapPrecess, sf);
    } else if (strcmp(cmd, "AveXinside") == 0) {
        sprintf(str, "X average inside closed polylines map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapAve, cmd);
    } else if (strcmp(cmd, "AveXoutside") == 0) {
        sprintf(str, "X average inside closed polylines map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapAve, cmd);
    } else if (strcmp(cmd, "AveYinside") == 0) {
        sprintf(str, "X average inside closed polylines map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapAve, cmd);
    } else if (strcmp(cmd, "AveYoutside") == 0) {
        sprintf(str, "X average inside closed polylines map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapAve, cmd);
    } else if (strcmp(cmd, "FFT") == 0) {
        sprintf(str, "Forward FFT of map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapFFT, sf);
    } else if (strcmp(cmd, "iFFT") == 0) {
        sprintf(str, "Inverse FFT of map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapiFFT, sf);
    } else if (strcmp(cmd, "Blank") == 0) {
        sprintf(str, "Blank map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapBlank, cmd);
    } else if (strcmp(cmd, "Unblank") == 0) {
        sprintf(str, "Unblank map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapBlank, cmd);
    } else if (strcmp(cmd, "BlankInside") == 0) {
        sprintf(str, "Blank map(s) inside closed polylines");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapBlank, cmd);
    } else if (strcmp(cmd, "UnblankInside") == 0) {
        sprintf(str, "Unblank map(s) inside closed polylines");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapBlank, cmd);
    } else if (strcmp(cmd, "BlankOutside") == 0) {
        sprintf(str, "Blank map(s) outside closed polylines");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapBlank, cmd);
    } else if (strcmp(cmd, "UnblankOutside") == 0) {
        sprintf(str, "Unblank map(s) outside closed polylines");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapBlank, cmd);
    } else if (strcmp(cmd, "Swap") == 0) {
        sprintf(str, "Swap data and errors in map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapSwap, sf);
    } else if (strcmp(cmd, "Shift") == 0) {
        sprintf(str, "FFT shift data in map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapFFTShift, sf);
    } else if (strcmp(cmd, "NearF") == 0) {
        sprintf(str, "Holo correct data in map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapHoloCorrect, sf);
    } else if (strcmp(cmd, "HoloF") == 0) {
        sprintf(str, "Holo fit phase data in map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapHoloFit, sf);
    } else if (strcmp(cmd, "S/N") == 0) {
        sprintf(str, "Replace data with S/N ratios in map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapReplace, sf);
    } else if (strcmp(cmd, "Cube") == 0) {
        sprintf(str, "Make maps into a FITS cube");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)Map2Cube, sf);
    } else {
        sprintf(str, "Show map(s)");
        XtAddCallback(listDialog, XmNextendedSelectionCallback,
                      (XtCallbackProc)MapShow, sf);
    }
    XtCreateManagedWidget(str, xmLabelWidgetClass, rc, NULL, 0);

    XtManageChild(listDialog);
    
    ArrangeStdFormDialog(sf, rc);
    
    if (has_edit)
        wprintf(sf->edit[0], "%s/cube.fits", GetWriteFITSDir());
        
    ManageDialogCenteredOnPointer(sf->form);
}

static void create_dualmapshow_dialog(Widget parent,
                                      Arg *argsLeft,  int nArgsLeft,
                                      Arg *argsRight, int nArgsRight)
{
    Widget rc, rcLeft, rcRight;
    Widget lList, rList;
    Widget lLab, rLab;
    string rStr="<none>", lStr="<none>";
    MapAriStruct *mas;
    StdForm *sf;

    mas = (MapAriStruct *)XtMalloc(sizeof(MapAriStruct));
    if (!mas) {
        PostErrorDialog(parent, "Out of memory in map arithmetic.");
        return;
    }
    mas->op = OP_ADD;
    mas->m1 = mas->m2 = NULL;

    sf = PostStdFormDialog(parent, "Show two maps overlayed",
             BUTT_APPLY, (XtCallbackProc)DualMapDraw, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 0, NULL);
    sf->user = (XtPointer)mas;
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmHORIZONTAL,
                                 NULL);

    rcLeft = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                     XmNorientation, XmVERTICAL,
                                     NULL);

    XtCreateManagedWidget("First map:", xmLabelWidgetClass, rcLeft,
                          NULL, 0);
    lList = XmCreateScrolledList(rcLeft, "leftlist", argsLeft, nArgsLeft);
    lLab = XtCreateManagedWidget(lStr, xmLabelWidgetClass, rcLeft, NULL, 0);


    rcRight = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                      XmNorientation, XmVERTICAL,
                                      NULL);

    XtCreateManagedWidget("Second map (only contours):", xmLabelWidgetClass,
                          rcRight, NULL, 0);
    rList = XmCreateScrolledList(rcRight, "rightlist", argsRight, nArgsRight);
    rLab = XtCreateManagedWidget(rStr, xmLabelWidgetClass, rcRight, NULL, 0);

    mas->mwL.w  = lLab;
    mas->mwL.sf = NULL;
    mas->mwL.m  = &(mas->m1);
    XtAddCallback(lList, XmNsingleSelectionCallback,
                  (XtCallbackProc)MapGet, &(mas->mwL));
    mas->mwR.w  = rLab;
    mas->mwR.sf = NULL;
    mas->mwR.m  = &(mas->m2);
    XtAddCallback(rList, XmNsingleSelectionCallback,
                  (XtCallbackProc)MapGet, &(mas->mwR));
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(lList);
    XtManageChild(rList);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void MergeMaps(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int i, j;
    string buf;
    MAP *m1, *m2, *new;
    MapAriStruct *mas = (MapAriStruct *)sf->user;
    
    void send_line();

    if (!mas) return;
    
    m1 = mas->m1; m2 = mas->m2;
    if (!m1 || !m2) return;
    
    if (m1->type != m2->type) {
        PostErrorDialog(w, "The two maps are of different type.");
        return;
    }
    if (m1->i_no != m2->i_no || m1->j_no != m2->j_no) {
        sprintf(buf, "The two maps have different sizes %dx%d != %dx%d.",
                m1->i_no, m1->j_no, m2->i_no, m2->j_no);
        PostErrorDialog(w, buf);
        return;
    }

    new = copy_map(&maplist, m1);
    if (!new) {
        PostErrorDialog(w, "Couldn't allocate the space for the new map!");
        return;
    }
    
    for (i=0; i<new->i_no; i++) {
        for (j=0; j<new->j_no; j++) {
            if (m2->f[i][j] <= BLANK) {
                new->f[i][j] = BLANK;
            } else {
                new->e[i][j] = m2->d[i][j];
            }
        }
    }
    
    new->original = m1;
    
    sprintf(buf, "Merged data from %s and errors from %s\n.",
            m1->name, m2->name);
    send_line(buf);
    
    sprintf(new->name, "[%s & %s]", m1->name, m2->name);

    MapDraw(NULL, new, NULL);
}

static void create_mergemaps_dialog(Widget parent,
                                    Arg *argsLeft,  int nArgsLeft,
                                    Arg *argsRight, int nArgsRight)
{
    Widget rc, rcLeft, rcRight;
    Widget lList, rList;
    Widget lLab, rLab;
    string rStr="<none>", lStr="<none>";
    MapAriStruct *mas;
    StdForm *sf;

    mas = (MapAriStruct *)XtMalloc(sizeof(MapAriStruct));
    if (!mas) {
        PostErrorDialog(parent, "Out of memory in map merging.");
        return;
    }
    mas->op = OP_ADD;
    mas->m1 = mas->m2 = NULL;

    sf = PostStdFormDialog(parent, "Merge data and error maps",
             BUTT_APPLY, (XtCallbackProc)MergeMaps, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 0, NULL);
    sf->user = (XtPointer)mas;
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmHORIZONTAL,
                                 NULL);

    rcLeft = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                     XmNorientation, XmVERTICAL,
                                     NULL);

    XtCreateManagedWidget("Data map:", xmLabelWidgetClass, rcLeft,
                          NULL, 0);
    lList = XmCreateScrolledList(rcLeft, "leftlist", argsLeft, nArgsLeft);
    lLab = XtCreateManagedWidget(lStr, xmLabelWidgetClass, rcLeft, NULL, 0);


    rcRight = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                      XmNorientation, XmVERTICAL,
                                      NULL);

    XtCreateManagedWidget("Error map:", xmLabelWidgetClass,
                          rcRight, NULL, 0);
    rList = XmCreateScrolledList(rcRight, "rightlist", argsRight, nArgsRight);
    rLab = XtCreateManagedWidget(rStr, xmLabelWidgetClass, rcRight, NULL, 0);

    mas->mwL.w  = lLab;
    mas->mwL.sf = NULL;
    mas->mwL.m  = &(mas->m1);
    XtAddCallback(lList, XmNsingleSelectionCallback,
                  (XtCallbackProc)MapGet, &(mas->mwL));
    mas->mwR.w  = rLab;
    mas->mwR.sf = NULL;
    mas->mwR.m  = &(mas->m2);
    XtAddCallback(rList, XmNsingleSelectionCallback,
                  (XtCallbackProc)MapGet, &(mas->mwR));
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(lList);
    XtManageChild(rList);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void CombineMaps(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int i, j, nX, nY, i1, i2, j1, j2, move = 0;
    double *x0, *y0, x, y, dx, dy;
    double xle=0.0, xri=0.0, xsp, ylo=0.0, yup=0.0, ysp;
    string buf;
    MAP *m1, *m2, *m;
    MapAriStruct *mas = (MapAriStruct *)sf->user;
    
    void send_line(), wsscanf();
    double *RAStr2Rad(char *), *DECStr2Rad(char *);

    if (!mas) return;
    
    m1 = mas->m1; m2 = mas->m2;
    if (!m1 || !m2) return;
    
    if (m1->type != m2->type) {
        PostErrorDialog(w, "The two maps are of different type.");
        return;
    }
    if (fabs(m1->xspacing) != fabs(m2->xspacing) ||
        fabs(m1->yspacing) != fabs(m2->yspacing)) {
        PostErrorDialog(w, "The two maps are of different spacings.");
        return;
    }
    
    wsscanf(sf->edit[0], buf);
    x0 = RAStr2Rad(buf);
    wsscanf(sf->edit[1], buf);
    y0 = DECStr2Rad(buf);
    if (x0 && y0) {
        move = 1;
        dx = (m1->x0 - *x0)*RADTODEG*3600.0*cos(*y0);
        dy = (m1->y0 - *y0)*RADTODEG*3600.0;
    } else {
        dx = 0.0;
        dy = 0.0;
    }
    
    xsp = m1->xspacing;
    ysp = m1->yspacing;

    for (i=0; i<m1->i_no; i++) {
        for (j=0; j<m1->j_no; j++) {
            x = m1->xleft  + (double)i * xsp + dx;
            y = m1->ylower + (double)j * ysp + dy;
            if (i == 0 && j == 0) {
                xle = xri = x;
                ylo = yup = y;
                continue;
            }
            if (x > xle) xle = x;
            if (x < xri) xri = x;
            if (y > yup) yup = y;
            if (y < ylo) ylo = y;
        }
    }

    if (move) {
        dx = (m2->x0 - *x0)*RADTODEG*3600.0*cos(*y0);
        dy = (m2->y0 - *y0)*RADTODEG*3600.0;
    } else {
        dx = (m2->x0 - m1->x0)*RADTODEG*3600.0*cos(m1->y0);
        dy = (m2->y0 - m1->y0)*RADTODEG*3600.0;
    }

    for (i=0; i<m2->i_no; i++) {
        for (j=0; j<m2->j_no; j++) {
            x = (m2->xleft  + (double)i * m2->xspacing + dx)/xsp;
            x = xsp * (double)NINT(x);
            y = (m2->ylower + (double)j * m2->yspacing + dy)/ysp;
            y = ysp * (double)NINT(y);
            if (x > xle) xle = x;
            if (x < xri) xri = x;
            if (y > yup) yup = y;
            if (y < ylo) ylo = y;
        }
    }
    /* printf("xle=%f   xri=%f   xsp=%f\n", xle, xri, xsp);
    printf("ylo=%f   yup=%f   ysp=%f\n", ylo, yup, ysp); */
    nX = NINT((xri-xle)/xsp) + 1;
    nY = NINT((yup-ylo)/ysp) + 1;
    
    /* printf("nX=%d  nY=%d\n", nX, nY); */
  
    m = new_map(&maplist, nX, nY);
    if (!m) {
        sprintf(buf, "Couldn't allocate the space (%dx%d) for the new map!",
                nX, nY);
        PostErrorDialog(w, buf);
        return;
    }
    m->xleft = xle;  m->xright = xri;
    m->ylower = ylo; m->yupper = yup;
    m->xspacing = xsp; m->yspacing = ysp;
    m->i_min = NINT(m->xleft /m->xspacing);
    m->i_max = NINT(m->xright/m->xspacing);
    m->j_min = NINT(m->ylower/m->yspacing);
    m->j_max = NINT(m->yupper/m->yspacing);
    m->swapped = 0;
    m->saved = 0;
    m->original = m1;
    if (m1->b.maj > 0.0) {
        m->b = m1->b;
    } else if (m2->b.maj > 0.0) {
        m->b = m2->b;
    } else {
        m->b.maj = m->b.min = m->b.PA = 0.0;
    }
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            x = m->xleft  + (double)i * xsp;
            y = m->ylower + (double)j * ysp;
            if (move) {
                i1 = NINT((x - m1->xleft -
                                (m1->x0 - *x0)*RADTODEG*3600.0*cos(*y0))/xsp);
                j1 = NINT((y - m1->ylower -
                                (m1->y0 - *y0)*RADTODEG*3600.0)/ysp);
            } else {
                i1 = NINT((x - m1->xleft)/xsp);
                j1 = NINT((y - m1->ylower)/ysp);
            }
            i2 = NINT((x - m2->xleft - dx)/m2->xspacing);
            j2 = NINT((y - m2->ylower - dy)/m2->yspacing);
            if (i1 >= 0 && i1 < m1->i_no && j1 >= 0 && j1 < m1->j_no &&
                i2 >= 0 && i2 < m2->i_no && j2 >= 0 && j2 < m2->j_no) {
               if (m1->f[i1][j1] <= BLANK && m2->f[i2][j2] <= BLANK) {
                   m->d[i][j] = m1->d[i1][j1];
                   m->e[i][j] = m1->e[i1][j1];
                   m->f[i][j] = m1->f[i1][j1];
               } else if (m1->f[i1][j1] <= BLANK) {
                   m->d[i][j] = m2->d[i2][j2];
                   m->e[i][j] = m2->e[i2][j2];
                   m->f[i][j] = m2->f[i2][j2];
               } else {
                   m->d[i][j] = m1->d[i1][j1];
                   m->e[i][j] = m1->e[i1][j1];
                   m->f[i][j] = m1->f[i1][j1];
               }
            } else if (i1 >= 0 && i1 < m1->i_no && j1 >= 0 && j1 < m1->j_no) {
               m->d[i][j] = m1->d[i1][j1];
               m->e[i][j] = m1->e[i1][j1];
               m->f[i][j] = m1->f[i1][j1];
            } else if (i2 >= 0 && i2 < m2->i_no && j2 >= 0 && j2 < m2->j_no) {
               m->d[i][j] = m2->d[i2][j2];
               m->e[i][j] = m2->e[i2][j2];
               m->f[i][j] = m2->f[i2][j2];
            } else {
               m->d[i][j] = 0.0;
               m->e[i][j] = 0.0;
               m->f[i][j] = BLANK;
            }
        }
    }
    
    m->type = m1->type;
    m->date = m1->date;
    m->coordType = m1->coordType;
    m->epoch = m1->epoch;
    m->equinox = m1->equinox;
    m->x0 = m1->x0;
    m->y0 = m1->y0;

#ifdef DEBUG
    printf("Map size: %dx%d\n", nX, nY);
    printf("RA range: %.2f to %.2f\n", m->xleft, m->xright);
    printf("Dec. range: %.2f to %.2f\n", m->ylower, m->yupper);
    printf("Spacing: %.2f and %.2f\n", m->xspacing, m->yspacing);
    printf("i_min=%d i_max=%d\n", m->i_min, m->i_max);
    printf("j_min=%d j_max=%d\n", m->j_min, m->j_max);
#endif
    
    sprintf(buf, "Combined data from %s and %s\n.",
            m1->name, m2->name);
    send_line(buf);
    
    sprintf(m->name, "[%s & %s]", m1->name, m2->name);

    MapDraw(NULL, m, NULL);
}

static void create_combinemaps_dialog(Widget parent,
                                    Arg *argsLeft,  int nArgsLeft,
                                    Arg *argsRight, int nArgsRight)
{
    Widget rcv, rc, rcLeft, rcRight;
    Widget lList, rList;
    Widget lLab, rLab;
    string rStr="<none>", lStr="<none>";
    MapAriStruct *mas;
    StdForm *sf;

    void wprintf();
    Widget ThreeHorEdit(Widget, Widget *, Widget *, Widget *);

    mas = (MapAriStruct *)XtMalloc(sizeof(MapAriStruct));
    if (!mas) {
        PostErrorDialog(parent, "Out of memory in combine maps.");
        return;
    }
    mas->op = OP_ADD;
    mas->m1 = mas->m2 = NULL;

    sf = PostStdFormDialog(parent, "Combine two maps",
             BUTT_APPLY, (XtCallbackProc)CombineMaps, NULL,
             BUTT_CANCEL, NULL, NULL,
             BUTT_HELP, NULL, (XtPointer)CombineMaps_Help,
             2, NULL);
    sf->user = (XtPointer)mas;

    rcv = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                  XmNorientation, XmVERTICAL,
                                  NULL);
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rcv,
                                 XmNorientation, XmHORIZONTAL,
                                 NULL);

    rcLeft = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                     XmNorientation, XmVERTICAL,
                                     NULL);

    XtCreateManagedWidget("First map:", xmLabelWidgetClass, rcLeft,
                          NULL, 0);
    lList = XmCreateScrolledList(rcLeft, "leftlist", argsLeft, nArgsLeft);
    lLab = XtCreateManagedWidget(lStr, xmLabelWidgetClass, rcLeft, NULL, 0);


    rcRight = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                      XmNorientation, XmVERTICAL,
                                      NULL);

    XtCreateManagedWidget("Second map:", xmLabelWidgetClass,
                          rcRight, NULL, 0);
    rList = XmCreateScrolledList(rcRight, "rightlist", argsRight, nArgsRight);
    rLab = XtCreateManagedWidget(rStr, xmLabelWidgetClass, rcRight, NULL, 0);

    mas->mwL.w  = lLab;
    mas->mwL.sf = NULL;
    mas->mwL.m  = &(mas->m1);
    XtAddCallback(lList, XmNsingleSelectionCallback,
                  (XtCallbackProc)MapGet, &(mas->mwL));
    mas->mwR.w  = rLab;
    mas->mwR.sf = NULL;
    mas->mwR.m  = &(mas->m2);
    XtAddCallback(rList, XmNsingleSelectionCallback,
                  (XtCallbackProc)MapGet, &(mas->mwR));
    
    XtCreateManagedWidget("New center coordinates (AAhBBmCC.Cs AAdBB'CC.C\"):",
                          xmLabelWidgetClass, rcv, NULL, 0);

    ThreeHorEdit(rcv, &(sf->edit[0]), &(sf->edit[1]), NULL);
    
    ArrangeStdFormDialog(sf, rcv);

    XtManageChild(lList);
    XtManageChild(rList);
    
    ManageDialogCenteredOnPointer(sf->form);
    
    wprintf(sf->edit[0], "");
    wprintf(sf->edit[1], "");
}

void manipulate_maps(Widget wid, char *cmd, XtPointer call_data)
{
    Widget w = wid;
    Arg wargs[10], wargs2[10];
    XmString *xmstr;
    int n, nmaps;

    void CreateConvolveDialog();
    void CreateMapInterpolateDialog();
    void CreateMapBeamEditDialog();
    void CreateMapScaleDialog();
    void CreateMapCorrelationDialog();
    void CreateMEMDialog();

    while (!XtIsWMShell(w))
        w = XtParent(w);
    
    xmstr = GetMapListStrings(&nmaps);
    if (!xmstr) {
        PostErrorDialog(w, "There are no stored maps!");
        return;
    }

    n = 0;
    if (nmaps > 0) {
        XtSetArg(wargs2[n], XmNitemCount, nmaps);
        XtSetArg(wargs[n], XmNitemCount, nmaps); n++;
        XtSetArg(wargs2[n], XmNitems, xmstr);
        XtSetArg(wargs[n], XmNitems, xmstr); n++;
        if (nmaps <= 10) {
            XtSetArg(wargs2[n], XmNvisibleItemCount, nmaps);
            XtSetArg(wargs[n], XmNvisibleItemCount, nmaps); n++;
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
        create_maparith_dialog(w, wargs, n, wargs2, n);
    } else if (strcmp(cmd, "Overlay") == 0) {
        XtSetArg(wargs2[n], XmNselectionPolicy, XmSINGLE_SELECT);
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        create_dualmapshow_dialog(w, wargs, n, wargs2, n);
    } else if (strcmp(cmd, "Merge") == 0) {
        XtSetArg(wargs2[n], XmNselectionPolicy, XmSINGLE_SELECT);
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        create_mergemaps_dialog(w, wargs, n, wargs2, n);
    } else if (strcmp(cmd, "Combine") == 0) {
        XtSetArg(wargs2[n], XmNselectionPolicy, XmSINGLE_SELECT);
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        create_combinemaps_dialog(w, wargs, n, wargs2, n);
    } else if (strcmp(cmd, "Interpolate") == 0) {
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        CreateMapInterpolateDialog(w, wargs, n);
    } else if (strcmp(cmd, "Edit beam") == 0) {
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        CreateMapBeamEditDialog(w, "edit", wargs, n);
    } else if (strcmp(cmd, "Scale") == 0) {
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        CreateMapScaleDialog(w, "edit", wargs, n);
    } else if (strcmp(cmd, "Regrid") == 0) {
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        CreateConvolveDialog(w, "regrid", wargs, n);
    } else if (strcmp(cmd, "ConvolveTant") == 0) {
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        CreateConvolveDialog(w, "Tant", wargs, n);
    } else if (strcmp(cmd, "ConvolveJansky") == 0) {
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        CreateConvolveDialog(w, "Jansky", wargs, n);
    } else if (strcmp(cmd, "Correlate") == 0) {
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        CreateMapCorrelationDialog(w, "edit", wargs, n);
    } else if (strcmp(cmd, "MEM") == 0) {
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        CreateMEMDialog(w, wargs, n);
    } else {
        XtSetArg(wargs[n], XmNselectionPolicy, XmEXTENDED_SELECT); n++;
        create_mapslist_dialog(w, cmd, wargs, n);
    }
    
    CleanupMapListStrings(xmstr, nmaps);
}

int DirtyMaps()
{
    MAP *m;
    list curr = NULL;

    bool empty_list();
    MAP *GetPosPosMap();
    MAP *GetVelPosMap();
    
    m = GetPosPosMap();
    if (!m->saved) return 1;
    m = GetVelPosMap();
    if (!m->saved) return 1;

    if (empty_list(maplist) == tRUE)
        return 0;

    while ( (curr = map_iterator(curr)) != NULL) {
        m = (MAP *)curr;
        if (!m->saved) return 1;
    }
    
    return 0;
}
