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
#include <ctype.h>
#include <math.h>

#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Frame.h>
#include <Xm/Separator.h>
#include <Xm/LabelG.h>
#include <Xm/CascadeB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Label.h>

#include <X11/cursorfont.h>

#include "defines.h"
#include "global_structs.h"
#include "menus.h"
#include "dialogs.h"

#ifdef HAVE_LIBPGPLOT
#include "XmPgplot.h"
#include "cpgplot.h"
#define CM2PIXEL (650.0/21.0)
#endif

#include "ps.h"

void init_PS_parameters()
{
    ps.Height = atoi(pP->psHeight);
    ps.Width  = atoi(pP->psWidth);
    ps.AspectRatio = ASPECT_DEFAULT;
    strcpy(ps.device, "xwin");
    strcpy(ps.send_to_lp, pP->printerCmd);
    strcpy(pgType, "ps");

    ps.cmAspectRatio = ASPECT_DEFAULT;
    ps.force_cm_size = 0;
    ps.cmHeight = atof(pP->cmHeight);
    ps.cmWidth  = atof(pP->cmWidth);
    ps.scale = 0.9;

    strcpy(ps.box.x.label, "bcnst");
    strcpy(ps.box.y.label, "bcnst");
    ps.box.x.inc = 0.0;
    ps.box.y.inc = 0.0;
    ps.box.x.ticks = 0;
    ps.box.y.ticks = 0;
    ps.box.style.lw = 2;
    ps.box.style.ls = 1;
    ps.box.style.fo = 2;
    ps.box.style.ch = 1.2;
    ps.box.style.ci = 1;
    ps.box.style.fs = 2;

    strcpy(ps.subbox.x.label, "bct");
    strcpy(ps.subbox.y.label, "bct");
    ps.subbox.x.inc = 0.0;
    ps.subbox.y.inc = 0.0;
    ps.subbox.x.ticks = 0;
    ps.subbox.y.ticks = 0;
    ps.subbox.style.lw = 2;
    ps.subbox.style.ls = 1;
    ps.subbox.style.fo = 2;
    ps.subbox.style.ch = 0.5;
    ps.subbox.style.ci = 1;
    ps.subbox.style.fs = 2;

    ps.TRsubbox = ps.subbox;
    strcpy(ps.TRsubbox.x.label, "bcmt");
    strcpy(ps.TRsubbox.y.label, "bcmt");
    
    strcpy(ps.wedge.x.label, "bc");
    strcpy(ps.wedge.y.label, "bcmistv");
    ps.wedge.x.inc = 0.0;
    ps.wedge.y.inc = 0.0;
    ps.wedge.x.ticks = 0;
    ps.wedge.y.ticks = 0;
    ps.wedge.style.lw = 2;
    ps.wedge.style.ls = 1;
    ps.wedge.style.fo = 2;
    ps.wedge.style.ch = 1.0;
    ps.wedge.style.ci = 1;
    ps.wedge.style.fs = 2;

    ps.marker.lw = 2;
    ps.marker.ls = 1;
    ps.marker.fo = 2;
    ps.marker.ch = 0.8;
    ps.marker.ci = 1;
    ps.marker.fs = 2;

    ps.posmarker.lw = 2;
    ps.posmarker.ls = 1;
    ps.posmarker.fo = 2;
    ps.posmarker.ch = 0.8;
    ps.posmarker.ci = 1;
    ps.posmarker.fs = 2;

    strcpy(ps.x_label, "default");
    strcpy(ps.y_label, "default");
    strcpy(ps.t_label, "default");
    strcpy(ps.w_label, "default");
    ps.label.lw = 2;
    ps.label.ls = 1;
    ps.label.fo = 2;
    ps.label.ch = 1.2;
    ps.label.ci = 1;
    ps.label.fs = 2;
    
    ps.header.lw = 1;
    ps.header.ls = 1;
    ps.header.fo = 1;
    ps.header.ch = 1.0;
    ps.header.ci = 1;
    ps.header.fs = 2;
    
    ps.ilabel.lw = 2;
    ps.ilabel.ls = 1;
    ps.ilabel.fo = 2;
    ps.ilabel.ch = 1.0;
    ps.ilabel.ci = 1;
    ps.ilabel.fs = 2;

    ps.line.lw = 2;
    ps.line.ls = 1;
    ps.line.fo = 2;
    ps.line.ch = 0.8;
    ps.line.ci = 1;
    ps.line.fs = 2;
    
    ps.secondary.lw = 2;
    ps.secondary.ls = 1;
    ps.secondary.fo = 2;
    ps.secondary.ch = 0.8;
    ps.secondary.ci = 7;
    ps.secondary.fs = 2;

    ps.poly.lw = 2;
    ps.poly.ls = 1;
    ps.poly.fo = 2;
    ps.poly.ch = 1.0;
    ps.poly.ci = 6;
    ps.poly.fs = 2;

    ps.blbox.lw = 2;
    ps.blbox.ls = 1;
    ps.blbox.fo = 2;
    ps.blbox.ch = 1.0;
    ps.blbox.ci = 3;
    ps.blbox.fs = 2;

    ps.mobox.lw = 2;
    ps.mobox.ls = 2;
    ps.mobox.fo = 2;
    ps.mobox.ch = 1.0;
    ps.mobox.ci = 2;
    ps.mobox.fs = 2;

    ps.gauss.lw = 2;
    ps.gauss.ls = 1;
    ps.gauss.fo = 2;
    ps.gauss.ch = 1.0;
    ps.gauss.ci = 2;
    ps.gauss.fs = 2;

    ps.zero.lw = 2;
    ps.zero.ls = 1;
    ps.zero.fo = 2;
    ps.zero.ch = 1.0;
    ps.zero.ci = 1;
    ps.zero.fs = 2;
    
    ps.beambox.lw = 2;
    ps.beambox.ls = 1;
    ps.beambox.fo = 2;
    ps.beambox.ch = 1.0;
    ps.beambox.ci = 1;
    ps.beambox.fs = 2;
    
    ps.beam.lw = 2;
    ps.beam.ls = 1;
    ps.beam.fo = 2;
    ps.beam.ch = 1.0;
    ps.beam.ci = 1;
    ps.beam.fs = 2;
    
    pgplot = 0;
#ifdef HAVE_LIBPGPLOT
    pgOpen = 0;
#endif
}

int SavePSstruct(char *filename)
{
    int err=0;
#ifndef HAVE_LIBPGPLOT
    string buf;
    
    sprintf(buf, "%s has not been linked with the PGPLOT package.", PKGNAME);
    PostErrorDialog(NULL, buf);
#else
    int n;
    FILE *fp;
    
    fp = fopen(filename, "w");
    if (!fp) return 1;
    
    n = fwrite(&ps, sizeof(ps), 1, fp);
    if (n != 1) err = 1;

    fclose(fp);
#endif
        
    return err;
}

#ifdef HAVE_LIBPGPLOT
static void copy_PS_style(PSSTY *new, PSSTY *old)
{
    new->e = old->e;
}

static void copy_PS_box(PSBOX *new, PSBOX *old)
{
    new->e      = old->e;
    new->x.e    = old->x.e;
    new->y.e    = old->y.e;
    
    copy_PS_style(&(new->style), &(new->style));
}

/* We here need a nice way of checking the available pgplot drivers */
static int check_pgplot_driver(char *sel)
{
    int n, ndev, ntype, ndesc, inter;
    char type[9], desc[65];
    
    cpgqndt(&ndev);
    
    for (n=0; n<ndev; n++) {
        ntype = sizeof(type);
        ndesc = sizeof(desc);
        cpgqdt(n+1, type, &ntype, desc, &ndesc, &inter);
        if (!strcmp(type, "/GIF") && !strcmp(sel, "gif")) return 0;
        if (!strcmp(type, "/VGIF") && !strcmp(sel, "vgif")) return 0;
        if (!strcmp(type, "/PS") && !strcmp(sel, "ps")) return 0;
        if (!strcmp(type, "/VPS") && !strcmp(sel, "vps")) return 0;
        if (!strcmp(type, "/VPS") && !strcmp(sel, "eps")) return 0;
        if (!strcmp(type, "/CPS") && !strcmp(sel, "cps")) return 0;
        if (!strcmp(type, "/VCPS") && !strcmp(sel, "vcps")) return 0;
        if (!strcmp(type, "/VCPS") && !strcmp(sel, "ceps")) return 0;
        if (!strcmp(type, "/PNG") && !strcmp(sel, "png")) return 0;
        if (!strcmp(type, "/TPNG") && !strcmp(sel, "tpng")) return 0;
    }
    
    return 1;
}
#endif

int ReadPSstruct(char *filename)
{
    int err=0;
#ifndef HAVE_LIBPGPLOT
    string buf;
    
    sprintf(buf, "%s has not been linked with the PGPLOT package.", PKGNAME);
    PostErrorDialog(NULL, buf);
#else
    int n;
    FILE *fp;
    PSDATA tmp = ps;
    
    fp = fopen(filename, "r");
    if (!fp) return 1;
    
    n = fread(&ps, sizeof(ps), 1, fp);
    if (n != 1) err = 1;
    
    copy_PS_box(&(ps.box), &(tmp.box));
    copy_PS_box(&(ps.subbox), &(tmp.subbox));
    copy_PS_box(&(ps.TRsubbox), &(tmp.TRsubbox));
    copy_PS_box(&(ps.wedge), &(tmp.wedge));
    
    copy_PS_style(&(ps.marker), &(tmp.marker));
    copy_PS_style(&(ps.posmarker), &(tmp.posmarker));
    copy_PS_style(&(ps.label), &(tmp.label));
    copy_PS_style(&(ps.ilabel), &(tmp.ilabel));
    copy_PS_style(&(ps.line), &(tmp.line));
    copy_PS_style(&(ps.secondary), &(tmp.secondary));
    copy_PS_style(&(ps.zero), &(tmp.zero));
    copy_PS_style(&(ps.gauss), &(tmp.gauss));
    copy_PS_style(&(ps.poly), &(tmp.poly));
    copy_PS_style(&(ps.blbox), &(tmp.blbox));
    copy_PS_style(&(ps.mobox), &(tmp.mobox));
    copy_PS_style(&(ps.beambox), &(tmp.beambox));
    copy_PS_style(&(ps.beam), &(tmp.beam));
    copy_PS_style(&(ps.header), &(tmp.header));

    fclose(fp);
    
    if (!err) ShowPostScriptFile(NULL, "xwin", NULL);
#endif
    
    return err;
}

#ifdef HAVE_LIBPGPLOT
static void cancel_PS_dialog(Widget w, char *cmd, XmAnyCallbackStruct *cb)
{
    if (pgOpen) cpgclos();
    pgWidget = NULL;
    pgplot = 0;
    pgOpen = 0;
    if (pgTop) {
        XtDestroyWidget(pgTop);
        pgTop = NULL;
    }
}

static void SetScreenPGColors()
{
    cpgscr(0, 0.0, 0.0, 0.0);
    cpgscr(1, 1.0, 1.0, 1.0);
}

static void SetFilePGColors()
{
    cpgscr(1, 0.0, 0.0, 0.0);
    cpgscr(0, 1.0, 1.0, 1.0);
}

static void SetupPGColors()
{
    int i, n;
    double r, g, b;
    COLOR *c;

    COLOR *GetColorInfo();
    
    c = GetColorInfo();
    
    SetScreenPGColors();
    
    c_offset = i = 2;
    for (n=0; n<c->ncols; n++) {
        r = (double)c->c_r[n]/65535.0;
        g = (double)c->c_g[n]/65535.0;
        b = (double)c->c_b[n]/65535.0;
        cpgscr(i, (PLFLT)r, (PLFLT)g, (PLFLT)b);
        i++;
    }
    
    g_offset = i;
    for (n=0; n<c->ngreys; n++) {
        r = (double)c->g_r[n]/65535.0;
        g = (double)c->g_g[n]/65535.0;
        b = (double)c->g_b[n]/65535.0;
        cpgscr(i, (PLFLT)r, (PLFLT)g, (PLFLT)b);
        i++;
    }
    
    f_offset = i;
    for (n=0; n<c->nfalse; n++) {
        r = (double)c->f_r[n]/65535.0;
        g = (double)c->f_g[n]/65535.0;
        b = (double)c->f_b[n]/65535.0;
        cpgscr(i, (PLFLT)r, (PLFLT)g, (PLFLT)b);
        i++;
    }
}

static void EndPG(char *cmd)
{
    if (!cmd && pgWidget) return;
    
    cpgclos();
    pgOpen = 0;
    
    if (pgWidget) {
        (void)cpgopen(xmp_device_name(pgWidget));
        pgOpen = 1;
    }
}

static int BeginPG(char *cmd)
{
    if (!cmd) {
        if (pgWidget) {
            return 0;
        }
        PostPgmWindow(NULL, NULL, NULL);
        if (cpgopen(xmp_device_name(pgWidget)) <= 0) return 1;
        pgOpen = 1;
        SetupPGColors();
        return 0;
    } else {
        if (pgWidget) {
            cpgclos();
            pgOpen = 0;
        }
        if (cpgopen(cmd) <= 0) return 1;
        pgOpen = 1;
        SetupPGColors();
        SetFilePGColors();
        return 0;
    }
    
    return 1;
}
#endif

int GetPGColorOffset(int type)
{
#ifdef HAVE_LIBPGPLOT
    if (type == SHADE_FALSE)
        return f_offset;
    else if (type == SHADE_GREY)
        return g_offset;
    else
        return 0;
#else
    return 0;
#endif
}

void SetPGStyle(PSSTY *s)
{
    if (!s) return;

#ifdef HAVE_LIBPGPLOT
    cpgslw(s->lw);
    cpgsls(s->ls);
    cpgscf(s->fo);
    cpgsch((PLFLT)s->ch);
    cpgsci(s->ci);
    cpgsfs(s->fs);
#endif
}

void write_PS_file(char *file, char *cmd)
{
#ifndef HAVE_LIBPGPLOT
    string buf;
    
    sprintf(buf, "%s has not been linked with the PGPLOT package.", PKGNAME);
    PostErrorDialog(NULL, buf);
#else
    string tmp_device, buf;

    if (check_pgplot_driver(cmd)) {
        sprintf(buf,
"The PGPLOT package has no installed driver to support %s files.\n\
Check the drivers.list file and recompile PGPLOT.", cmd);
        PostErrorDialog(NULL, buf);
        return;
    }
    
    strcpy(tmp_device, ps.device);
    
    if (strcmp(cmd, "eps")==0) {
        pgplot = 2;
/* 2 indicates an EPS file, but we have to use vps since PGPLOT doesn't
   provide an EPS format, and the PGPLOT landscape modes (ps and cps)
   includes a rotation of 90 deg in the page setup of the plot, which is
   irrelevant for an EPS file */
        sprintf(ps.device, "\"%s\"/vps", file);
    } else if (strcmp(cmd, "ceps") == 0) {
        pgplot = 2;
        sprintf(ps.device, "\"%s\"/vcps", file);
    } else {
        pgplot = 1;
        sprintf(ps.device, "\"%s\"/%s", file, cmd);
    }

    if (BeginPG(ps.device)) {
        sprintf(buf, "Couldn't open '%s' for writing.", ps.device);
        PostErrorDialog(NULL, buf);
    } else {
        draw_main();
        EndPG(ps.device);
    }
    
    if (!pgWidget) pgplot = 0;
    strcpy(ps.device, tmp_device);
#endif
}

void ShowPostScriptFile(Widget w, char *cmd, XtPointer cd)
{
#ifndef HAVE_LIBPGPLOT
    string buf;
    
    sprintf(buf, "%s has not been linked with the PGPLOT package.", PKGNAME);
    PostErrorDialog(NULL, buf);
#else
    string tmp_device;

    strcpy(tmp_device, ps.device);

    pgplot = 1;
    strcpy(ps.device, cmd);

    BeginPG(NULL);
    draw_main();
    EndPG(NULL);

    if (!pgWidget) pgplot = 0;
    strcpy(ps.device, tmp_device);
#endif
}

void PrintPostScriptFile(Widget w, char *cmd, XtPointer cd)
{
#ifndef HAVE_LIBPGPLOT
    string buf;
    
    sprintf(buf, "%s has not been linked with the PGPLOT package.", PKGNAME);
    PostErrorDialog(NULL, buf);
#else
    string tmp_device, file, buf;

    if (strcmp(cmd, "eps")==0 || strcmp(cmd, "ceps")==0) {
        PostErrorDialog(w,
"An encapsulated PS (*.eps) file cannot be printed.\n\
It can only be SAVEd to disk.");
        return;
    }
    if (strcmp(cmd, "gif")==0 || strcmp(cmd, "vgif")==0) {
        PostErrorDialog(w,
"A GIF (*.gif) file cannot be printed.\n\
It can only be SAVEd to disk.");
        return;
    }
    if (strcmp(cmd, "png")==0 || strcmp(cmd, "tpng")==0) {
        PostErrorDialog(w,
"A PNG (*.png) file cannot be printed.\n\
It can only be SAVEd to disk.");
        return;
    }
    
    strcpy(tmp_device, ps.device);

    pgplot = 1;
    strcpy(file, GetTmpFile("XSpecPSfile"));
    sprintf(ps.device, "%s/%s", file, cmd);


    if (BeginPG(ps.device)) {
        sprintf(buf, "Couldn't open '%s' for writing.", ps.device);
        PostErrorDialog(NULL, buf);
    } else {
        draw_main();
        EndPG(ps.device);
    
        sprintf(buf, "%s %s", ps.send_to_lp, file);
        XS_system(buf, 1);
        sprintf(buf, "%s %s", pP->unixRmCmd, file);
        XS_system(buf, 1);
    }

    if (!pgWidget) pgplot = 0;
    strcpy(ps.device, tmp_device);
#endif
}

#ifdef HAVE_LIBPGPLOT
static int GetPSTypeEntry()
{
    if (strcmp(pgType, "ps")==0) {
        return 0;
    } else if (strcmp(pgType, "vps")==0) {
        return 1;
    } else if (strcmp(pgType, "cps")==0) {
        return 2;
    } else if (strcmp(pgType, "vcps")==0) {
        return 3;
    } else if (strcmp(pgType, "eps")==0) {
        return 4;
    } else if (strcmp(pgType, "ceps")==0) {
        return 5;
    } else if (strcmp(pgType, "gif")==0) {
        return 6;
    } else if (strcmp(pgType, "vgif")==0) {
        return 7;
    } else if (strcmp(pgType, "png")==0) {
        return 8;
    } else if (strcmp(pgType, "tpng")==0) {
        return 9;
    } else {
        return 0;
    }
}
#endif

void PostPgmWindow(Widget wid, char *cmd, XtPointer cd)
{
#ifdef HAVE_LIBPGPLOT
    int n;
    Arg wargs[10];
    Widget frame, cancel, show, print, save;
    Widget menuBar, menuF;
    Widget w = wid;
    COLOR *c;

    COLOR *GetColorInfo();
    Widget CreateMenuBar(Widget, MenuBarItem *);
#else
    Widget w = wid;
    string buf;
#endif
    
    if (!w) w = gp->top;
    
    while (!XtIsWMShell(w))
        w = XtParent(w);

#ifndef HAVE_LIBPGPLOT
    sprintf(buf, "%s has not been linked with the PGPLOT package.", PKGNAME);
    PostErrorDialog(w, buf);
    return;
#else
    c = GetColorInfo();

    n = 0;
    XtSetArg(wargs[n], XmNautoUnmanage, False); n++;
    XtSetArg(wargs[n], XmNtitle, "PostScript viewer"); n++;
    pgTop = XmCreateFormDialog(w, "form", wargs, n);
        
    menuBar = CreateMenuBar(pgTop, PSMenuBarData);
    
    frame = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, pgTop,
				                    XmNshadowType, XmSHADOW_IN,
				                    NULL);

    pgWidget = XtVaCreateManagedWidget("pgplot", xmPgplotWidgetClass, frame,
					XmNheight, ps.Height, XmNwidth, ps.Width,
                    /* XmNcolormap, c->cmap, */
					XmpNmaxColors, (2+c->ncols+c->ngreys+c->nfalse),
					NULL);
    
    cancel = XtCreateManagedWidget(BUTT_CANCEL, xmPushButtonWidgetClass,
                                   pgTop, NULL, 0);
    XtAddCallback(cancel, XmNactivateCallback,
                  (XtCallbackProc)cancel_PS_dialog, NULL);
        
    show = XtCreateManagedWidget("show", xmPushButtonWidgetClass,
                                 pgTop, NULL, 0);
    XtAddCallback(show, XmNactivateCallback,
                  (XtCallbackProc)ShowPostScriptFile, "xwin");
    
    save = XtCreateManagedWidget("save", xmPushButtonWidgetClass,
                                 pgTop, NULL, 0);
    XtAddCallback(save, XmNactivateCallback,
                  (XtCallbackProc)set_PS_file, pgType);
    
    print = XtCreateManagedWidget("print", xmPushButtonWidgetClass,
                                 pgTop, NULL, 0);
    XtAddCallback(print, XmNactivateCallback,
                  (XtCallbackProc)PrintPostScriptFile, pgType);
    
    menuF = CreateOptionMenu(pgTop, &PSOptionMenu);
    SetDefaultOptionMenuItem(menuF, GetPSTypeEntry());

    XtAddCallback(pgTop, XmNunmapCallback,
                  (XtCallbackProc)cancel_PS_dialog, NULL);

    XtVaSetValues(pgTop, XmNdefaultButton, cancel, NULL);
    
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNtopOffset,           5); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,          5); n++;
    XtSetArg(wargs[n], XmNrightAttachment,     XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,         5); n++;
    XtSetValues(menuBar, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,           menuBar); n++;
    XtSetArg(wargs[n], XmNtopOffset,           5); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,          5); n++;
    XtSetArg(wargs[n], XmNrightAttachment,     XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset,         5); n++;
    XtSetValues(frame, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,           frame); n++;
    XtSetArg(wargs[n], XmNtopOffset,           5); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,          5); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,        5); n++;
    XtSetValues(cancel, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,           frame); n++;
    XtSetArg(wargs[n], XmNtopOffset,           5); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,          cancel); n++;
    XtSetArg(wargs[n], XmNleftOffset,          20); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,        5); n++;
    XtSetValues(show, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,           frame); n++;
    XtSetArg(wargs[n], XmNtopOffset,           5); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,          show); n++;
    XtSetArg(wargs[n], XmNleftOffset,          20); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,        5); n++;
    XtSetValues(save, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,           frame); n++;
    XtSetArg(wargs[n], XmNtopOffset,           5); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,          save); n++;
    XtSetArg(wargs[n], XmNleftOffset,          5); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,        5); n++;
    XtSetValues(print, wargs, n);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget,           frame); n++;
    XtSetArg(wargs[n], XmNtopOffset,           5); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget,          print); n++;
    XtSetArg(wargs[n], XmNleftOffset,          5); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,        5); n++;
    XtSetValues(menuF, wargs, n);
    
    XtManageChild(menuF);
    ManageDialogCenteredOnPointer(pgTop);
#endif
}

#ifdef HAVE_LIBPGPLOT
static void UpdatePGParameters(Widget wid, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int n = 0, h = ps.Height, w = ps.Width;

    wsscanf(sf->edit[n++],  ps.send_to_lp);
    wdscanf(sf->edit[n++], &ps.scale);
    wdscanf(sf->edit[n++], &ps.cmHeight);
    wdscanf(sf->edit[n++], &ps.cmWidth);
    wiscanf(sf->edit[n++], &ps.Height);
    wiscanf(sf->edit[n++], &ps.Width);
    wsscanf(sf->edit[n++],  ps.x_label);
    wsscanf(sf->edit[n++],  ps.y_label);
    wsscanf(sf->edit[n++],  ps.t_label);
    wsscanf(sf->edit[n++],  ps.w_label);
    
    if (w != ps.Width || h != ps.Height) {
        if (pgWidget) XtVaSetValues(pgWidget,
                                    XmNheight, ps.Height,
                                    XmNwidth, ps.Width, NULL);
    }
    ShowPostScriptFile(NULL, "xwin", NULL);
}

static void SetPSWidgetSizeCallback(Widget wid, char *s,
                                    XmAnyCallbackStruct *cb)
{
    int a = atoi(s);
    int h = ps.Height;
    int w = ps.Width;;
    
    switch (a) {
        case ASPECT_DEFAULT:
            ps.Height = atoi(pP->psHeight);
            ps.Width  = atoi(pP->psWidth);
            break;
        case ASPECT_SQUARE:
            ps.Height = ps.Width = (h+w)/2;
            break;
        case ASPECT_A4_PORT:
            ps.Width = w;
            ps.Height = (int)((double)w*297.0/210.0 + 0.5);
            break;
        case ASPECT_A4_LAND:
            ps.Height = h;
            ps.Width = (int)((double)h*297.0/210.0 + 0.5);
            break;
        case ASPECT_LETTER_PORT:
            ps.Width = w;
            ps.Height = (int)((double)w*278.0/215.0 + 0.5);
            break;
        case ASPECT_LETTER_LAND:
            ps.Height = h;
            ps.Width = (int)((double)h*278.0/215.0 + 0.5);
            break;
        default:
            return;
    }
    ps.AspectRatio = a;
    if (hWidget) wprintf(hWidget, "%d", ps.Height);
    if (wWidget) wprintf(wWidget, "%d", ps.Width);
    if (pgWidget) XtVaSetValues(pgWidget,
                                XmNheight, ps.Height,
                                XmNwidth, ps.Width, NULL);
    ShowPostScriptFile(NULL, "xwin", NULL);
}

static void SetPSPlotSizeCallback(Widget wid, char *s,
                                  XmAnyCallbackStruct *cb)
{
    int a = atoi(s);
    
    switch (a) {
        case ASPECT_DEFAULT:
            ps.cmHeight = atof(pP->cmHeight);
            ps.cmWidth  = atof(pP->cmWidth);
            break;
        case ASPECT_SQUARE:
            ps.cmHeight = ps.cmWidth = 15.0;
            break;
        case ASPECT_A4_PORT:
            ps.cmHeight = 27.0;
            ps.cmWidth = 19.0;
            break;
        case ASPECT_A4_LAND:
            ps.cmHeight = 19.0;
            ps.cmWidth = 27.0;
            break;
        case ASPECT_LETTER_PORT:
            ps.cmHeight = 26.0;
            ps.cmWidth = 19.5;
            break;
        case ASPECT_LETTER_LAND:
            ps.cmHeight = 19.5;
            ps.cmWidth = 26.0;
            break;
        default:
            return;
    }
    ps.AspectRatio = ps.cmAspectRatio = a;
    ps.Height = (int)(ps.cmHeight * CM2PIXEL + 0.5);
    ps.Width  = (int)(ps.cmWidth * CM2PIXEL + 0.5);
    if (cm_hWidget) wprintf(cm_hWidget, "%f", ps.cmHeight);
    if (cm_wWidget) wprintf(cm_wWidget, "%f", ps.cmWidth);
    if (hWidget) wprintf(hWidget, "%d", ps.Height);
    if (wWidget) wprintf(wWidget, "%d", ps.Width);
    if (pgWidget) XtVaSetValues(pgWidget,
                                XmNheight, ps.Height,
                                XmNwidth, ps.Width, NULL);
    ShowPostScriptFile(NULL, "xwin", NULL);
}

static void SetPSTypeCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
    strcpy(pgType, s);
}

static void SetPSLineStyleCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
}

static void SetPSColorStyleCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
}

static void SetPSFontStyleCallback(Widget w, char *s, XmAnyCallbackStruct *cb)
{
}

static void SetPSFillAreaStyleCallback(Widget w, char *s,
                                       XmAnyCallbackStruct *cb)
{
}

static void SetPGAxisToggles(Widget *e, char *s)
{
    int i;
    
    for (i=0; i<NaxisOpts; i++) {
        if (strchr(s, (int)pgAxisLetters[i])) {
            XtVaSetValues(e[i], XmNset, True, NULL);
        } else {
            XtVaSetValues(e[i], XmNset, False, NULL);
        }
    }
}

static void AddLetterToString(char *s, char c)
{
    if (!s) return;
    
    if (strchr(s, (int)c)) return; /* Character is already in the string */

    strncat(s, &c, 1);
}

static void RemoveLetterFromString(char *s, char c)
{
    char *p, *next;
    
    if (!s) return;
    
    if (!(p=strchr(s, (int)c))) return; /* Character is not in string */
    
    while (*p != '\0') {
        next = p+1;
        *p = *next;
        p++;
    }
}

static void AxisOptionCallback(Widget w, STRINT *p,
                               XmToggleButtonCallbackStruct *cb)
{    
    if (cb->set) {
        AddLetterToString(p->s, p->c);
    } else {
        RemoveLetterFromString(p->s, p->c);
    }
}

static void pgUpdateStyleParameters(Widget w, StdForm *sf,
                                    XmAnyCallbackStruct *cb)
{
    int i;
    PSSTY *b = (PSSTY *)sf->any;
    
    wiscanf(sf->edit[0], &b->lw);
    wdscanf(sf->edit[1], &b->ch);
    
    QueryOptionMenuItem(sf->edit[2], &i);
    if (i >= 0) b->ls = i+1;
    
    QueryOptionMenuItem(sf->edit[3], &i);
    if (i >= 0) b->ci = i;
    
    QueryOptionMenuItem(sf->edit[4], &i);
    if (i >= 0) b->fo = i+1;
    
    QueryOptionMenuItem(sf->edit[5], &i);
    if (i >= 0) b->fs = i+1;
    
    ShowPostScriptFile(NULL, "xwin", NULL);
}

static void pgUpdateAxisParameters(Widget w, StdForm *sf,
                                    XmAnyCallbackStruct *cb)
{
    PSAXIS *a = (PSAXIS *)sf->any;
    
    wfscanf(sf->edit[0], &a->inc);
    wiscanf(sf->edit[1], &a->ticks);
    
    ShowPostScriptFile(w, "xwin", NULL);
}

static void PSStyleSetup(Widget wid, char *cmd, XtPointer cd)
{
    int i, j;
    Widget w = wid, frame, rc, rc1, menu, label;
    string title;
    StdForm *sf;
    PSSTY *b;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    if (strcmp(cmd, "oframe") == 0) {
        strcpy(title, "Outer frame style setup");
        b = &ps.box.style;
    } else if (strcmp(cmd, "iframe") == 0) {
        strcpy(title, "Inner frame style setup");
        b = &ps.subbox.style;
    } else if (strcmp(cmd, "triframe") == 0) {
        strcpy(title, "Special frame style setup");
        b = &ps.TRsubbox.style;
    } else if (strcmp(cmd, "wedge") == 0) {
        strcpy(title, "Contour wedge frame style setup");
        b = &ps.wedge.style;
    } else if (strcmp(cmd, "header") == 0) {
        strcpy(title, "Header style setup");
        b = &ps.header;
    } else if (strcmp(cmd, "marker") == 0) {
        strcpy(title, "Marker style setup");
        b = &ps.marker;
    } else if (strcmp(cmd, "posmarker") == 0) {
        strcpy(title, "Position marker style setup");
        b = &ps.posmarker;
    } else if (strcmp(cmd, "label") == 0) {
        strcpy(title, "Outer label style setup");
        b = &ps.label;
    } else if (strcmp(cmd, "ilabel") == 0) {
        strcpy(title, "Inner label style setup");
        b = &ps.ilabel;
    } else if (strcmp(cmd, "line") == 0) {
        strcpy(title, "Data line style setup");
        b = &ps.line;
    } else if (strcmp(cmd, "secondary") == 0) {
        strcpy(title, "Secondary data line style setup");
        b = &ps.secondary;
    } else if (strcmp(cmd, "zero") == 0) {
        strcpy(title, "Zeroline style setup");
        b = &ps.zero;
    } else if (strcmp(cmd, "gauss") == 0) {
        strcpy(title, "Gaussian style setup");
        b = &ps.gauss;
    } else if (strcmp(cmd, "poly") == 0) {
        strcpy(title, "Polynomial style setup");
        b = &ps.poly;
    } else if (strcmp(cmd, "blbox") == 0) {
        strcpy(title, "Baseline box style setup");
        b = &ps.blbox;
    } else if (strcmp(cmd, "mobox") == 0) {
        strcpy(title, "Moment box style setup");
        b = &ps.mobox;
    } else if (strcmp(cmd, "bebox") == 0) {
        strcpy(title, "Beam box style setup");
        b = &ps.beambox;
    } else if (strcmp(cmd, "beam") == 0) {
        strcpy(title, "Beam style setup");
        b = &ps.beam;
    } else {
        XtWarning("Invalid option to PSStyleSetup.");
        return;
    }
    
    sf = PostStdFormDialog(w, title,
             BUTT_APPLY, (XtCallbackProc)pgUpdateStyleParameters, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             6, NULL);
    sf->any = (XtPointer)b;
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    
    frame = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rc,
				                    XmNshadowType, XmSHADOW_OUT,
				                    NULL);
    rc1 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, frame,
                                  XmNorientation, XmHORIZONTAL,
                                  XmNnumColumns, 2,
                                  XmNadjustLast, False,
                                  XmNpacking, XmPACK_COLUMN,
                                  NULL);
    j = 0;
    for (i=0; i<2; i++) {
        label = XtCreateManagedWidget(pgStyleLabels[j],
                                      xmLabelWidgetClass,
                                      rc1, NULL, 0);
        sf->edit[j++]  = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                               rc1, NULL, 0);
    }
    
    rc1 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                  XmNorientation,       XmVERTICAL,
                                  NULL);
    sf->edit[j++] = menu = CreateOptionMenu(rc1, &PSLineStyleMenu);
    SetDefaultOptionMenuItem(menu, b->ls - 1);
    sf->edit[j++] = menu = CreateOptionMenu(rc1, &PSColorStyleMenu);
    SetDefaultOptionMenuItem(menu, b->ci);
    sf->edit[j++] = menu = CreateOptionMenu(rc1, &PSFontStyleMenu);
    SetDefaultOptionMenuItem(menu, b->fo - 1);
    sf->edit[j++] = menu = CreateOptionMenu(rc1, &PSFillAreaStyleMenu);
    SetDefaultOptionMenuItem(menu, b->fs - 1);
    
    ArrangeStdFormDialog(sf, rc);

    wprintf(sf->edit[0], "%d",  b->lw);
    wprintf(sf->edit[1], "%f",  b->ch);
    XtManageChild(sf->edit[2]);
    XtManageChild(sf->edit[3]);
    XtManageChild(sf->edit[4]);
    XtManageChild(sf->edit[5]);

    ManageDialogCenteredOnPointer(sf->form);
}

static void PSAxisSetup(Widget wid, char *cmd, XtPointer cd)
{
    int i, j;
    Widget w = wid, frame, rc, rc1, rc2, toggle, label;
    string title;
    XmString xstr;
    STRINT *p;
    PSAXIS *b;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);
        
    p = (STRINT *)XtMalloc(NaxisOpts * sizeof(STRINT));

    if (strcmp(cmd, "xOuter") == 0) {
        strcpy(title, "Outer frame x-axis setup");
        b = &ps.box.x;
    } else if (strcmp(cmd, "yOuter") == 0) {
        strcpy(title, "Outer frame y-axis setup");
        b = &ps.box.y;
    } else if (strcmp(cmd, "xInner") == 0) {
        strcpy(title, "Inner frame x-axis setup");
        b = &ps.subbox.x;
    } else if (strcmp(cmd, "yInner") == 0) {
        strcpy(title, "Inner frame y-axis setup");
        b = &ps.subbox.y;
    } else if (strcmp(cmd, "xTR") == 0) {
        strcpy(title, "Special inner frame x-axis setup");
        b = &ps.TRsubbox.x;
    } else if (strcmp(cmd, "yTR") == 0) {
        strcpy(title, "Special inner frame y-axis setup");
        b = &ps.TRsubbox.y;
    } else if (strcmp(cmd, "wedge") == 0) {
        strcpy(title, "Contour wedge axis setup");
        b = &ps.wedge.y;
    } else {
        XtWarning("Invalid option to PSAxisSetup.");
        return;
    }
    
    sf = PostStdFormDialog(w, title,
             BUTT_APPLY, (XtCallbackProc)pgUpdateAxisParameters, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             NaxisAll, NULL);
    sf->user = (XtPointer)p;
    sf->any = (XtPointer)b;
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    
    frame = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rc,
				                    XmNshadowType, XmSHADOW_OUT,
				                    NULL);
    
    rc1 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, frame,
                                  XmNorientation, XmHORIZONTAL,
                                  XmNnumColumns, 2,
                                  XmNadjustLast, False,
                                  XmNpacking, XmPACK_COLUMN,
                                  NULL);
    j = 0;
    for (i=0; i<NaxisEdts; i++) {
        label = XtCreateManagedWidget(pgAxisEdits[j], xmLabelWidgetClass,
                                      rc1, NULL, 0);
        sf->edit[j++] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                              rc1, NULL, 0);
    }
    
    rc2 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                  XmNorientation, XmVERTICAL,
                                  XmNnumColumns, 1,
                                  XmNadjustLast, False,
                                  XmNpacking, XmPACK_TIGHT,
                                  NULL);
                               
    for (i=0; i<NaxisOpts; i++) {
        sf->edit[j++] = toggle = XtVaCreateManagedWidget("label",
                                       xmToggleButtonWidgetClass,
                                       rc2,
                                       XmNfontList, gp->flist10,
                                       XmNlabelString,
                                       (xstr = MKSTRING(pgAxisOptions[i])),
                                       NULL);
        if (xstr) XmStringFree(xstr);
        p->s = b->label;
        p->c = pgAxisLetters[i];
        XtAddCallback(toggle, XmNvalueChangedCallback,
                      (XtCallbackProc)AxisOptionCallback, p);
        p++;
    }
    
    ArrangeStdFormDialog(sf, frame);

    wprintf(sf->edit[0], "%f",  b->inc);
    wprintf(sf->edit[1], "%d",  b->ticks);
    SetPGAxisToggles(&(sf->edit[2]), b->label);

    ManageDialogCenteredOnPointer(sf->form);
}

static void ForceSizeCallback(Widget w, char *c,
                              XmToggleButtonCallbackStruct *cb)
{    
    if (cb->set) {
        ps.force_cm_size = 1;
    } else {
        ps.force_cm_size = 0;
    }
}

static void PGSetup(Widget wid, char *cmd, XtPointer cd)
{
    Widget frame, rc, rc1, rc2, label, toggle;
    Widget menu1, menu2;
    Widget w = wid;
    XmString xstr;
    int i, n, ne=10;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "PGPLOT setup",
             BUTT_APPLY, (XtCallbackProc)UpdatePGParameters, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
             ne, NULL);
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);
    
    frame = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rc,
				                    XmNshadowType, XmSHADOW_OUT,
				                    NULL);

    rc1 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, frame,
                                  XmNorientation, XmHORIZONTAL,
                                  XmNnumColumns, ne,
                                  XmNadjustLast, False,
                                  XmNpacking, XmPACK_COLUMN,
                                  NULL);
    for (i=0; i<ne; i++) {
        label  = XtCreateManagedWidget(pgplot_labels[i],
                                       xmLabelWidgetClass,
                                       rc1, NULL, 0);
        sf->edit[i] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                            rc1, NULL, 0);
    }
    cm_hWidget = sf->edit[2];
    cm_wWidget = sf->edit[3];
    hWidget = sf->edit[4];
    wWidget = sf->edit[5];
    
    rc2 = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                                  XmNorientation, XmVERTICAL,
                                  NULL);
    
    menu1 = CreateOptionMenu(rc2, &PSPlotSizeMenu);
    SetDefaultOptionMenuItem(menu1, ps.cmAspectRatio);
    
    toggle = XtVaCreateManagedWidget("toggle",
                       xmToggleButtonWidgetClass,
                       rc2,
                       XmNset,
                       ps.force_cm_size ? True : False,
                       XmNlabelString,
                       (xstr = MKSTRING("Always use the size in cm above")),
                       NULL);
    XtAddCallback(toggle, XmNvalueChangedCallback,
                  (XtCallbackProc)ForceSizeCallback, NULL);
    if (xstr) XmStringFree(xstr);
    
    menu2 = CreateOptionMenu(rc2, &PSWidgetSizeMenu);
    SetDefaultOptionMenuItem(menu2, ps.AspectRatio);
    
    ArrangeStdFormDialog(sf, rc);
    
    XtManageChild(menu1);
    XtManageChild(menu2);

    n = 0;
    wprintf(sf->edit[n++], "%s",  ps.send_to_lp);
    wprintf(sf->edit[n++], "%f",  ps.scale);
    wprintf(sf->edit[n++], "%f",  ps.cmHeight);
    wprintf(sf->edit[n++], "%f",  ps.cmWidth);
    wprintf(sf->edit[n++], "%d",  ps.Height);
    wprintf(sf->edit[n++], "%d",  ps.Width);
    wprintf(sf->edit[n++], "%s",  ps.x_label);
    wprintf(sf->edit[n++], "%s",  ps.y_label);
    wprintf(sf->edit[n++], "%s",  ps.t_label);
    wprintf(sf->edit[n++], "%s",  ps.w_label);

    ManageDialogCenteredOnPointer(sf->form);
}
#endif
