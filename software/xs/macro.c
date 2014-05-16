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

#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/Frame.h>

#include "defines.h"
#include "global_structs.h"
#include "dialogs.h"

/*** External variables ***/
extern GLOBAL *gp;

void PostErrorDialog(Widget, char *);
void PostWarningDialog(Widget, char *);
void ManageDialogCenteredOnPointer(Widget);

/* average.c */
void AverageScans(Widget, char *, XtPointer);

/* baseline.c */
void do_baseline_fit(Widget, char *, XtPointer);
void SetRmsFromBoxes(Widget, char *, XtPointer);
void remove_poly(Widget, char *, XtPointer);
void add_poly(Widget, char *, XtPointer);

/* drawing.c */
void draw_main();

/* fold.c */
void DoFoldAllScans();

/*** Local variables ***/
typedef struct {
    void (*f)();
    int narg;
    char *arg;
} CmdFunction;

typedef struct {
    int    n;
    string a[5];
} CmdArg;

typedef struct {
    char *cmd;
    CmdFunction funcInfo;
    CmdArg arg;
} CmdItem;

#define ARGNULL  {0, {""}}
#define CMDNULL  {NULL, {NULL, 0, NULL}, ARGNULL}

/* Macros to implement

typedef struct {
    char *cmd;
    char *arg;
} CmdItem;

   SetFilter [source/date/time/molecule/backend/freq/
             int/tsys/nchan/scan/xoff/yoff] [on/off] a1,a2
             
   ReadFITS [seq/map/2d/3d] filepattern Filter [on/off]
   AppendFITS [seq/map] filepattern Filter [on/off]
   
   Boxes [mom/rms] a1,a2,... [vel/fre/cha]
   SetRMSfromBoxes
   
   FitBaseline [pol/che] order
   SubtractFit [baseline/gauss]
   AddFit [baseline/gauss]
   
   Fold throw offset
   
   AveragePos [rms/tsys]
   AverageAll [rms/tsys]
   
   DeleteDatasets [all/old/last]
   SelectDataset  [last/first/prompt]
   
   Show [spe/all/map/sca]
   
   PolyLine [open/closed] x1,y1 x2,y2 ...
 */

static CmdItem MacroCmds[] = {
    {"DoBaselineFit",         {do_baseline_fit, 3, "Macro"}, ARGNULL},
    {"SetRMSFromBoxes",       {SetRmsFromBoxes, 3, "Macro"}, ARGNULL},
    {"SubtractPolynomialFit", {remove_poly, 3, "Macro"}, ARGNULL},
    {"AddPolynomialFit",      {add_poly, 3, "Macro"}, ARGNULL},
    {"FoldAllScans",          {DoFoldAllScans, 3, NULL}, ARGNULL},
    {"AverageAllScansTsys",   {AverageScans, 3, "AMtsys"}, ARGNULL},
    {"AveragePosScansTsys",   {AverageScans, 3, "PMtsys"}, ARGNULL},
    {"AverageAllScansRMS",    {AverageScans, 3, "AMrms"}, ARGNULL},
    {"AveragePosScansRMS",    {AverageScans, 3, "PMrms"}, ARGNULL},
    {"Show",                  {draw_main, 0, ""}, ARGNULL},
    CMDNULL
};

static int nMacroCmds, MacroError, RunningMacro;
static string MacroWarning;
static XmString *xMacroCmdStrs;

static int *Macros, nMacros;

void init_macro()
{
    int n=0;
    
    Macros = NULL;
    nMacros = 0;
    MacroError = 0;
    strcpy(MacroWarning, "");
    RunningMacro = 0;
    
    while (MacroCmds[n].cmd) n++;

    nMacroCmds = n;
    
    xMacroCmdStrs = (XmString *)XtMalloc(n * sizeof(XmString));
    
    for (n=0; n<nMacroCmds; n++) {
        xMacroCmdStrs[n] = MKSTRING(MacroCmds[n].cmd);
    }
}

void SetMacroError(int err)
{
    if (!RunningMacro) return;
    
    MacroError = err;
}

void SetMacroWarning(char *warn)
{
    if (!RunningMacro) return;
    
    strcpy(MacroWarning, warn);
}

int QueryRunningMacro()
{
    return RunningMacro;
}

static int CheckMacroName(char *name)
{
    int n;
    
    if (!name) return -1;
    
    for (n=0; n<nMacroCmds; n++) {
        if (strcmp(name, MacroCmds[n].cmd)==0) return n;
    }
    
    return -1;
}

static int NewMacroCmd(char *cmd)
{
    int n;
    string buf;
    
    n = CheckMacroName(cmd);
    
    if (n == -1) {
        sprintf(buf, "Macro cmd '%s' does not exist.", cmd);
        PostErrorDialog(NULL, buf);
        return 1;
    }
    
    if (!Macros) {
        Macros = (int *)XtMalloc((nMacros+1) * sizeof(int));
    } else {
        Macros = (int *)XtRealloc((char *)Macros, (nMacros+1) * sizeof(int));
    }
    Macros[nMacros++] = n;
    
    return 0;
}

static void ClearMacroCmds()
{
    XtFree((char *)Macros);
    Macros = NULL;
    nMacros = 0;
}

static void CallCmdFunction(CmdFunction *p)
{
    if (!p) return;
    
    switch (p->narg) {
        case 0:
            p->f();
            break;
        case 3:
            p->f(NULL, p->arg, NULL);
            break;
        default:
            return;
    }
}

static void RunMacro(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int n, m;
    string tmp;
    
    int MyLoop();
    void send_line();
    
    RunningMacro = 1;
    
    for (n=0; n<nMacros; n++) {
        m = Macros[n];
        sprintf(tmp, "Doing cmd: %s", MacroCmds[m].cmd);
        send_line(tmp);
        while (MyLoop(1));
        CallCmdFunction(&(MacroCmds[m].funcInfo));
        if (MacroError) {
            sprintf(tmp, "Error %d during macro cmd '%s'. Aborting...",
                    MacroError, MacroCmds[n].cmd);
            PostErrorDialog(w, tmp);
            MacroError = 0;
            break;
        }
        if (strlen(MacroWarning)) {
            sprintf(tmp, "Warning %s during macro cmd '%s'.",
                    MacroWarning, MacroCmds[n].cmd);
            PostWarningDialog(w, tmp);
            strcpy(MacroWarning, "");
        }
    }
    
    RunningMacro = 0;
}

static void UpdateMacroWindow(Widget w)
{
   int n, m;
   char *txt;
   string row;
   
   void wprintf();
   
   if (nMacros <= 0) {
       wprintf(w, "");
       return;
   }
   
   txt = (char *)XtMalloc(nMacros * 100 * sizeof(char));
   
   for (n=0; n<nMacros; n++) {
       m = Macros[n];
       sprintf(row, "%s\n", MacroCmds[m].cmd);
       if (n == 0) {
           strcpy(txt, row);
       } else {
           strcat(txt, row);
       }
   }
   wprintf(w, "%s", txt);
   
   XtFree(txt);
}

static void SelectMacroCmdCallback(Widget w, Widget text,
                                   XmListCallbackStruct *cb)
{
    char *txt;
    
    if (XmStringGetLtoR(cb->item, XmSTRING_DEFAULT_CHARSET, &txt)) {
        if (!NewMacroCmd(txt)) {
            UpdateMacroWindow(text);
        }
        XtFree(txt);
    }
}

static void ClearMacro(Widget w, Widget text, XmAnyCallbackStruct *cb)
{
    ClearMacroCmds();
    UpdateMacroWindow(text);
}

void ExecuteMacro(Widget wid, char *cmd, XtPointer cd)
{
    RunMacro(wid, NULL, NULL);
}

void PostMacroEditDialog(Widget wid, char *cmd, XtPointer cd)
{
    int n;
    Arg wargs[10];
    Widget w = wid;
    Widget fr, rc, rc1, rc2, list, text;
    Widget clear;
    StdForm *sf;
    
    while (!XtIsWMShell(w))
        w = XtParent(w);
    
    sf = PostStdFormDialog(w, "Macro Editor",
             BUTT_APPLY, (XtCallbackProc)RunMacro, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 0, NULL);

    fr = XtVaCreateWidget("frame", xmFrameWidgetClass,
				          sf->form, XmNshadowType, XmSHADOW_OUT, NULL);
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, fr,
                          XmNorientation, XmHORIZONTAL,
                          NULL);
    rc1 = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, rc,
                          XmNorientation, XmVERTICAL,
                          NULL);
    rc2 = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, rc,
                          XmNorientation, XmVERTICAL,
                          NULL);
    XtCreateManagedWidget("Available macro commands:", xmLabelWidgetClass,
                          rc1, NULL, 0);
    XtCreateManagedWidget("Macro:", xmLabelWidgetClass,
                          rc2, NULL, 0);
    n = 0;
    XtSetArg(wargs[n], XmNitemCount, nMacroCmds); n++;
    XtSetArg(wargs[n], XmNitems, xMacroCmdStrs); n++;
    XtSetArg(wargs[n], XmNvisibleItemCount, 15); n++;
    XtSetArg(wargs[n], XmNfontList, gp->flist12); n++;
    XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
    list = XmCreateScrolledList(rc1, "list", wargs, n);
    
    text = XtVaCreateManagedWidget("text", xmTextWidgetClass, rc2,
                                   XmNeditMode, XmMULTI_LINE_EDIT,
                                   XmNcolumns, 25,
                                   XmNrows, 15,
                                   XmNfontList, gp->flist12,
                                   NULL);
    clear = XtCreateManagedWidget("Clear", xmPushButtonWidgetClass,
                                  rc2, NULL, 0);

    XtAddCallback(list, XmNsingleSelectionCallback,
                  (XtCallbackProc)SelectMacroCmdCallback, text);
    XtAddCallback(clear, XmNactivateCallback,
                  (XtCallbackProc)ClearMacro, text);
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(list);
    XtManageChild(rc1);                
    XtManageChild(rc2);                
    XtManageChild(rc);                
    XtManageChild(fr);
    
    UpdateMacroWindow(text);     
    
    ManageDialogCenteredOnPointer(sf->form);
}
