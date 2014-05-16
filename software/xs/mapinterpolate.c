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
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/Frame.h>

#include "list.h"
#include "defines.h"
#include "global_structs.h"
#include "dialogs.h"

/*** External variables ***/
void PostErrorDialog(Widget, char *);
void PostWarningDialog(Widget, char *);
void ManageDialogCenteredOnPointer(Widget);
void PostInterpolationDialog(Widget, char *, XtPointer);

/*** Local variables ***/

static void MapInterpolateGet(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n, nL;
    list *pL;
    list *mL;
    MAP *m = NULL;

    list *get_action_list();
    list *get_maplist();
    void wprintf();

    mL = get_maplist();

    if ((pL = get_action_list(cb, &nL, *mL)) == NULL)
        return;

    for (n=0; n<nL; n++)
        m = (MAP *)DATA(pL[n]);

    sf->any = m;

    wprintf(sf->edit[0], "Interpolated %s", m->name);

    free(pL);
}

static void DoInterpolateMap(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int i, j, nX, nY;
    int err, type, order, corners;
    double xspa, yspa;
    string buf;
    MAP *m, *old = (MAP *)sf->any;
    int **L;
    double **A;
    
    int FillHolesInArray(), InterpolateArray();
    void FreeIntArray(), FreeDoubleArray();
    int **AllocIntArray();
    double **AllocDoubleArray();
    void send_line(), wsscanf(), SetWatchCursor();
    void MapDraw(), SetIntpOrder();
    int GetIntpType(), GetIntpOrder(), GetIntpCorners();
    MAP *new_map();
    list *get_maplist();

    if (!old) {
        PostErrorDialog(w, "No map selected to be interpolated.");
        return;
    }
    
    type    = GetIntpType();
    order   = GetIntpOrder();
    corners = GetIntpCorners();
    
    if (order <= 0) {
        PostWarningDialog(w, "Selected interpolation order is zero.");
        return;
    }
    
    nX   = old->i_no;
    nY   = old->j_no;
    xspa = old->xspacing;
    yspa = old->yspacing;
    
    A = AllocDoubleArray(nX, nY);
    if (!A) {
        PostErrorDialog(NULL, "InterpolateMap: Out of memory.");
        return;
    }
    L = AllocIntArray(nX, nY);
    if (!L) {
        FreeDoubleArray(A, nX, nY);
        PostErrorDialog(NULL, "InterpolateMap: Out of memory.");
        return;
    }
    
    SetWatchCursor(True);
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            A[i][j] = old->d[i][j];
            if (old->f[i][j] <= BLANK)
                L[i][j] = EMPTY;
            else
                L[i][j] = FILLED;
        }
    }

    if (corners > 0)
        err = FillHolesInArray(A, L, nX, nY, corners);

    if ((i = order)) {
        while (i) {
            err = InterpolateArray(&A, &L, &nX, &nY, type);
            if (err != 0) {
                sprintf(buf, "Couldn't interpolate array. err=%d.", err);
                send_line(buf);
                if (A) FreeDoubleArray(A, nX, nY);
                if (L) FreeIntArray(L, nX, nY);
                SetWatchCursor(False);
                PostErrorDialog(NULL, buf);
                return;
            }
            xspa /= 2.0;
            yspa /= 2.0;
            i--;
            if (corners > 0)
                err = FillHolesInArray(A, L, nX, nY, corners);
        }
    }

    m = new_map(get_maplist(), nX, nY);
    
    m->type     = old->type;
    m->swapped  = old->swapped;
    m->memed    = old->memed;
    m->original = old->original;
    m->x0       = old->x0;
    m->y0       = old->y0;
    m->xleft    = old->xleft;
    m->xright   = old->xright;
    m->ylower   = old->ylower;
    m->yupper   = old->yupper;
    m->date     = old->date;
    strcpy(m->molecule, old->molecule);
    m->interpolated = old->interpolated + 1;
    m->equinox  = old->equinox;
    m->epoch    = old->epoch;
    
    m->ndata    = nX * nY;
    m->i_min    = NINT(m->xleft/xspa);
    m->i_max    = NINT(m->xright/xspa);
    m->j_min    = NINT(m->ylower/yspa);
    m->j_max    = NINT(m->yupper/yspa);
    m->xspacing = xspa;
    m->yspacing = yspa;
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (L[i][j] == FILLED) {
                m->f[i][j] = UNBLANK;
                m->d[i][j] = A[i][j];
                m->e[i][j] = UNDEF;
            } else {
                m->f[i][j] = BLANK;
                m->d[i][j] = UNDEF;
                m->e[i][j] = UNDEF;
            }
        }
    }
    
    FreeDoubleArray(A, nX, nY);
    FreeIntArray(L, nX, nY);

    SetWatchCursor(False);

    wsscanf(sf->edit[0], m->name);
    sprintf(buf, "Interpolated map stored as '%s': %dx%d\n",  m->name, nX, nY);
    send_line(buf);
    
    XtDestroyWidget(sf->form);
    
    SetIntpOrder(0);
    MapDraw(NULL, m, NULL);
}

void CreateMapInterpolateDialog(Widget parent, Arg *args, int nArgs)
{
    Widget rc, fr, rc2, maps, intpB;
    StdForm *sf;

    void wprintf();
   
    sf = PostStdFormDialog(parent, "Map interpolation",
             BUTT_APPLY, (XtCallbackProc)DoInterpolateMap, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             1, NULL);
    sf->any = NULL;
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);

    maps  = XmCreateScrolledList(rc,  "list",  args,  nArgs);

    fr = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rc,
                                 XmNshadowType, XmSHADOW_OUT, NULL);
    rc2 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                  XmNorientation, XmVERTICAL,
                                  NULL);
    XtCreateManagedWidget("Name of new map:", xmLabelWidgetClass,
                          rc2, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc2, NULL, 0);

    intpB = XtCreateManagedWidget("Interpolation...", xmPushButtonWidgetClass,
                                  rc, NULL, 0);
    XtAddCallback(intpB, XmNactivateCallback,
                  (XtCallbackProc)PostInterpolationDialog, "interpolate");

    XtAddCallback(maps, XmNsingleSelectionCallback,
                  (XtCallbackProc)MapInterpolateGet, sf);

    ArrangeStdFormDialog(sf, rc);

    XtManageChild(maps);

    wprintf(sf->edit[0], "<none>");
    
    ManageDialogCenteredOnPointer(sf->form);
}
