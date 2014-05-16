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
#include <Xm/Xm.h>

#include "defines.h"
#include "global_structs.h"
#include "list.h"

#ifdef HAVE_LIBPGPLOT
#include "cpgplot.h"
#endif

/*** External variables ***/
extern int     pgplot;
extern VIEW   *vP;
extern PSDATA  ps;
extern GLOBAL *gp;
extern USER   *pP;

void SetPGStyle(PSSTY *);

list    scan_iterator(list, DataSetPtr);

/*** Local variables ***/
DRAW draw;

static int  find_letter(char, char *);
static int  inside_box(int *, int *, int *, int *);

int point_is_inside(int, int);
int xunit2x(double), yunit2y(double);

#define TICK_FACTOR     50
#define MIN_TICK_LENGTH 4
#define MIN_NO_OF_TICKS 4

#define AX_X_INVTICKS      find_letter('i', xcode)
#define AX_Y_INVTICKS      find_letter('i', ycode)
#define AX_X_SUBTICKS      find_letter('s', xcode)
#define AX_Y_SUBTICKS      find_letter('s', ycode)
#define AX_LEFTLABEL       find_letter('L', ycode)
#define AX_RIGHTLABEL      find_letter('R', ycode)
#define AX_LEFTTICKS       find_letter('l', ycode)
#define AX_RIGHTTICKS      find_letter('r', ycode)
#define AX_BOTTOMLABEL     find_letter('B', xcode)
#define AX_TOPLABEL        find_letter('T', xcode)
#define AX_BOTTOMTICKS     find_letter('b', xcode)
#define AX_TOPTICKS        find_letter('t', xcode)

#define ANYMAP(m)   ((m)==SHOW_POSPOS || (m)==SHOW_VELPOS || (m)==SHOW_POSVEL)
#define DSCALAB(m)  ((m)==SHOW_SCATTER && (vP->nScat <= 1 || !draw.multiple))

typedef struct {
    int x, y;
} point;

void init_draw_parameters()
{
    strcpy(vP->x_label, DEF_XLABEL);
    strcpy(vP->y_label, DEF_YLABEL);
    strcpy(vP->t_label, DEF_TLABEL);
    strcpy(vP->r_label, "");
    strcpy(vP->l_label, "");

    draw.clear    = 1;
    draw.update   = 1;

    draw.frame    = 1;
    draw.labels   = 1;
    draw.ticks    = 1;

    draw.wframe   = 1;
    draw.wlabels  = 1;
    draw.wticks   = 1;

    draw.data     = 1;
    draw.data_rms = 0;
    draw.data_sec = 0;
    
    draw.wedge    = 1;
    draw.wedgepos = POS_RIGHT;
    draw.beam     = MAP_BEAM_NO;

    draw.zline     = atoi(pP->zeroLine);
    draw.histo     = 0;
    draw.histo_sec = 0;

    draw.markers  = 0;
    draw.boxes    = 0;
    draw.poly     = 0;
    draw.gsum     = 0;
    draw.gind     = 0;
    
    draw.header   = 0;
    
    draw.multiple = 1;
    
    draw.projaxes = 1;
    draw.projnums = 1;
}

int SaveState(char *filename)
{
    int n, err=0;
    FILE *fp;
    
    fp = fopen(filename, "w");
    if (!fp) return 1;
    
    n = fwrite(vP, sizeof(*vP), 1, fp);
    if (n != 1) err = 1;
    if (err) {
        fclose(fp);
        return err;
    }
    n = fwrite(&draw, sizeof(draw), 1, fp);
    if (n != 1) err = 2;

    fclose(fp);
        
    return err;
}

int ReadState(char *filename)
{
    int n, err = 0;
    FILE *fp;
    VIEW vtmp = *vP;
    DRAW dtmp=draw;
    string buf;
    
    void send_line();
    
    fp = fopen(filename, "r");
    if (!fp) return 1;
    
    n = fread(vP, sizeof(*vP), 1, fp);
    if (n != 1) err = 1;
    if (err) {
        fclose(fp);
        *vP = vtmp;
        sprintf(buf, "Couldn't load view from '%s' (err=%d).\n", filename, err);
        send_line(buf);
        return err;
    }
    n = fread(&draw, sizeof(draw), 1, fp);
    if (n != 1) err = 2;

    fclose(fp);
    
    if (err) {
        draw = dtmp;
        sprintf(buf, "Couldn't load drawing parameters from '%s' (err=%d).\n",
                filename, err);
        send_line(buf);
    } else {
        vP->from = vtmp.from;
        vP->to = vtmp.to;
        vP->s = vtmp.s;
        vP->s2 = vtmp.s2;
        vP->m = vtmp.m;
        vP->p = vtmp.p;
        vP->nMaps = vtmp.nMaps;
        vP->M = vtmp.M;
        vP->nScat = vtmp.nScat;
        vP->P = vtmp.P;
    }
    
    return err;
}

void draw_all_labels()
{
#ifdef HAVE_LIBPGPLOT
    string x_tmp, y_tmp, t_tmp;
    static string w_tmp="";
#endif
    
    void DrawRelLabel();
    void draw_axis_label(GC, char, char *);
    char *GetXLabel(), *GetYLabel(), *GetWedgeLabel();
    char *GetTopLabel(), *GetLeftLabel(), *GetRightLabel();
    
    strcpy(vP->x_label, GetXLabel());
    strcpy(vP->y_label, GetYLabel());
    strcpy(vP->t_label, GetTopLabel());
    strcpy(vP->w_label, GetWedgeLabel());

#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        if (strcmp(ps.x_label, "default") == 0)
            strcpy(x_tmp, GetXLabel());
        else
            strcpy(x_tmp, ps.x_label);
        if (strcmp(ps.y_label, "default") == 0)
            strcpy(y_tmp, GetYLabel());
        else
            strcpy(y_tmp, ps.y_label);
        if (strcmp(ps.t_label, "default") == 0)
            strcpy(t_tmp, vP->t_label);
        else
            strcpy(t_tmp, ps.t_label);
        if (draw.header) strcpy(t_tmp, "");
        
        if (strcmp(ps.w_label, "default") == 0)
            strcpy(w_tmp, vP->w_label);
        else
            strcpy(w_tmp, ps.w_label);
        
        SetPGStyle(&ps.label);
        if (vP->mode == SHOW_WEDGE) {
            switch (draw.wedgepos) {
                case POS_RIGHT:
                    cpgmtxt("R", 3.0, 0.5, 0.5, w_tmp);
                    break;
                case POS_LEFT:
                    cpgmtxt("L", 3.0, 0.5, 0.5, w_tmp);
                    break;
                case POS_ABOVE:
                    cpgmtxt("T", 3.0, 0.5, 0.5, w_tmp);
                    break;
                case POS_BELOW:
                    cpgmtxt("B", 3.0, 0.5, 0.5, w_tmp);
                    break;
            }
        } else {
            cpglab(x_tmp, y_tmp, t_tmp);
        }
    }
#endif

    if (vP->mode == SHOW_WEDGE) {
        switch (draw.wedgepos) {
            case POS_RIGHT:
            case POS_LEFT:
                draw_axis_label(gp->gcFrame[2], 't', vP->w_label);
                break;
            case POS_ABOVE:
                draw_axis_label(gp->gcFrame[2], 'T', vP->w_label);
                break;
            case POS_BELOW:
                draw_axis_label(gp->gcFrame[2], 'B', vP->w_label);
                break;
        }
        return;
    }
    draw_axis_label(gp->gcFrame[2], 'x', vP->x_label);
    draw_axis_label(gp->gcFrame[2], 'y', vP->y_label);
    
    if ((vP->tlab_type || vP->slab_type) && !draw.header)
        draw_axis_label(gp->gcFrame[3], 't', vP->t_label);
    
    if (vP->llab_type &&
        (DSCALAB(vP->mode) || vP->mode == SHOW_SPE)) {
        DrawRelLabel(gp->gcFrame[3], vP->lef_x, vP->lef_y, 0.0,
                     GetLeftLabel());
    }
    if (vP->rlab_type &&
        (DSCALAB(vP->mode) || vP->mode == SHOW_SPE)) {
        DrawRelLabel(gp->gcFrame[3], vP->rig_x, vP->rig_y, 0.0,
                     GetRightLabel());
    }
}

int CheckHeader()
{
    if (vP->mode != SHOW_SPE && vP->mode != SHOW_POSPOS) return 0;
    
    return draw.header;
}

void draw_main()
{
    int m = vP->mode, tmp_replace=0;
#ifdef HAVE_LIBPGPLOT
    PLFLT x1=0.0, x2=1.0, y1=0.0, y2=1.0, dx, dy;
#endif
    DRAW tmp;
    list curr = NULL;

    void draw_frame(), draw_ticks(), draw_gauss();
    void draw_gauss_sum(), draw_gauss_ind();
    void draw_spectrum(), draw_boxes(), draw_poly();
    void draw_markers(), draw_map(), SetScanView();
    void draw_secondary(scanPtr, GC);
    void draw_header(scanPtr, mapPtr, GC);
#ifdef USE_IMAGE_STORAGE   
    void SaveImage();
#endif

    if (!XtIsRealized(gp->graph)) return;
    
    if (!vP->from) return;

    if (draw.clear) {
        XClearWindow(XtDisplay(gp->graph), XtWindow(gp->graph));
#ifdef USE_PIXMAP_STORAGE
        XFillRectangle(XtDisplay(gp->graph), gp->pm, gp->gcClear,
                       0, 0, gp->p_w, gp->p_h);
#endif
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            if (pgplot == 2 || ps.force_cm_size)
                cpgpap((PLFLT)(ps.cmWidth/2.54),
                       (PLFLT)(ps.cmHeight/ps.cmWidth));
            else
                cpgvstd();
            /* if (draw.header) { */
            if (CheckHeader()) {
                cpgqvp(0, &x1, &x2, &y1, &y2);
                dy = (PLFLT)(1.0 - 0.85) * (y2-y1);
                cpgsvp(x1, x2, y1, y2 - dy);
            }
            cpgqvp(0, &x1, &x2, &y1, &y2);
            dx = (PLFLT)(1.0 - ps.scale) / 2.0 * (x2-x1);
            dy = (PLFLT)(1.0 - ps.scale) / 2.0 * (y2-y1);
            cpgsvp(x1 + dx, x2 - dx, y1 + dy, y2 - dy);
            cpgask(0);
            cpgpage();
        }
#endif
    }

    if ((m == SHOW_ALLSPE && vP->s && vP->from->sequence) ||
        (ANYMAP(m) && vP->nMaps > 1)) {
        tmp = draw;
        draw.labels = 0;
        draw.ticks = 0;
        draw.frame = 0;
        tmp_replace = 1;
        if (m == SHOW_ALLSPE) draw_frame(gp->gcFrame[5], 1, 0);
    } else if (m == SHOW_SCATTER && vP->nScat > 1 && draw.multiple){
        tmp = draw;
        draw.labels = 0;
        draw.ticks = 0;
        draw.frame = 0;
        tmp_replace = 1;
    } else {
        draw_frame(gp->gcFrame[5], 1, 0);
    }
    
    if (draw.ticks && (m == SHOW_ALLSPE || m == SHOW_SCATTER || m == SHOW_SPE)) {
        if (m == SHOW_ALLSPE)
            draw_ticks(gp->gcFrame[2], "Bb", "Ll");
        else
            draw_ticks(gp->gcFrame[2], "Bbts", "Llrs");
    }
    
    if (tmp_replace) draw = tmp;
    
    if (draw.labels && m != SHOW_POSPOS) draw_all_labels();

    if (m == SHOW_ALLSPE || m == SHOW_POSPOS || m == SHOW_VELPOS ||
        m == SHOW_SCATTER|| m == SHOW_POSVEL) {
        draw_map();
        if (draw.labels && m == SHOW_POSPOS && vP->nMaps <= 1)
            draw_all_labels();
    } else {
        SetScanView(NULL);

        if (draw.boxes)     draw_boxes(gp->gcBox, gp->gcMom);

        if (draw.data_sec == 2) {
            while ( (curr = scan_iterator(curr, vP->from)) ) {
                if ((scanPtr)DATA(curr) == vP->s) continue;
                draw_secondary((scanPtr)DATA(curr), gp->gcSec);
                break;
            }
        } else if (draw.data_sec == 3) {
            while ( (curr = scan_iterator(curr, vP->from)) ) {
                if ((scanPtr)DATA(curr) != vP->s)
                    draw_secondary((scanPtr)DATA(curr), gp->gcSec);
            }
        }
        
        draw_spectrum(gp->gcLine, gp->gcRms);
        
        if (draw.poly)      draw_poly(gp->gcPoly, vP->s);
        if (draw.gsum) {
            if (vP->s->gaussFit)
                            draw_gauss(gp->gcGaussI, vP->s->g);
            else
                            draw_gauss_sum(gp->gcGauss);
        }
        if (draw.gind)      draw_gauss_ind(gp->gcGaussI);
    }

    if (draw.markers) draw_markers(gp->gcFrame[1], gp->gcGauss, gp->gcTag);
    if (draw.header)  draw_header(vP->s, vP->m, gp->gcFrame[2]);

#ifdef USE_IMAGE_STORAGE   
    SaveImage();
#endif
}

void redraw_graph(Widget w, char *cmd, XtPointer call_data)
{
    void SetDefWindow(), obtain_map_info(), draw_main();

    if (strcmp(cmd, "update")==0) {
        if (vP->mode == SHOW_POSPOS)
            obtain_map_info(NULL, "load", NULL);
        SetDefWindow(SCALE_BOTH);
    }
    draw_main();
}

void draw_spectrum(GC s_gc, GC r_gc)
{
    int xl, xm, xr;
    int c, c1, c2;
    int yl, yr, ylm, yrm;
    double *s;
#ifdef HAVE_LIBPGPLOT
    PLFLT fxl, fxm, fxr, fyl, fyr;
    PLFLT x[4], y[4];
#endif

    void draw_line();
    int x2chan(), chan2x(), dchan2x();
    double chan2xunit();
    double *chan2s(), *chan2r();

    if (draw.zline) {
        draw_line(s_gc, vP->min_x,             yunit2y(0.0),
                        vP->min_x + vP->box_w, yunit2y(0.0));
#ifdef HAVE_LIBPGPLOT
        if (pgplot) {
            SetPGStyle(&ps.zero);
            x[0] = (PLFLT)vP->xleft;
            x[1] = (PLFLT)vP->xright;
            y[0] = y[1] = 0.0;
            cpgline(2, x, y);
        }
#endif
    }

    if (!vP->s)
        return;

    c1 = x2chan(vP->min_x);
    c2 = x2chan(vP->min_x + vP->box_w);
    if (c1 > c2) {
        c = c1; c1 = c2; c2 = c;
    }
    c1--;
    c2++;

    if (c1 < 0) c1 = 0;
    if (c2 >= vP->s->nChan) c2 = vP->s->nChan - 1;

    for (c=c1; c<c2; c++) {
        xl = chan2x(c);
        xr = chan2x(c+1);
        if ((s = chan2s(c)) == NULL) continue;
        yl = yunit2y(*s);
        if ((s = chan2s(c+1)) == NULL) continue;
        yr = yunit2y(*s);

        if (draw.histo) {
            xm = dchan2x((double)c + 0.5);
            if (draw.data) {
                draw_line(s_gc, xl, yl, xm, yl);
                draw_line(s_gc, xm, yl, xm, yr);
                draw_line(s_gc, xm, yr, xr, yr);
            }
            if (draw.data_rms) {
                if ((s = chan2r(c)) == NULL) continue;
                yl = yunit2y(*s);
		ylm = yunit2y(-1.0*(*s));
                if ((s = chan2r(c+1)) == NULL) continue;
                yr = yunit2y(*s);
                yrm = yunit2y(-1.0*(*s));
                draw_line(r_gc, xl, yl, xm, yl);
                draw_line(r_gc, xm, yl, xm, yr);
                draw_line(r_gc, xm, yr, xr, yr);
                draw_line(r_gc, xl, ylm, xm, ylm);
                draw_line(r_gc, xm, ylm, xm, yrm);
                draw_line(r_gc, xm, yrm, xr, yrm);
            }
        } else {
            if (draw.data) draw_line(s_gc, xl, yl, xr, yr);
            if (draw.data_rms) {
                if ((s = chan2r(c)) == NULL) continue;
                yl = yunit2y(*s);
                ylm = yunit2y(-1.0*(*s));
                if ((s = chan2r(c+1)) == NULL) continue;
                yr = yunit2y(*s);
                yrm = yunit2y(-1.0*(*s));
                draw_line(r_gc, xl, yl, xr, yr);
                draw_line(r_gc, xl, ylm, xr, yrm);
            }
        }
    }

#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.line);
        cpgbbuf();
        for (c=0; c<vP->s->nChan-1; c++) {
            fxl = (PLFLT)chan2xunit(c);
            fxr = (PLFLT)chan2xunit(c+1);
            if ((s = chan2s(c)) == NULL) continue;
            fyl = (PLFLT)*s;
            if ((s = chan2s(c+1)) == NULL) continue;
            fyr = (PLFLT)*s;
            if (draw.histo) {
                fxm = (fxr + fxl)/2.;
                x[0] = fxl; y[0] = fyl;
                x[1] = fxm; y[1] = fyl;
                x[2] = fxm; y[2] = fyr;
                x[3] = fxr; y[3] = fyr;
                if (draw.data) {
                    cpgline(4, x, y);
                }
                if (draw.data_rms) {
                    if ((s = chan2r(c)) == NULL) continue;
                    fyl = (PLFLT)*s;
                    if ((s = chan2r(c+1)) == NULL) continue;
                    fyr = (PLFLT)*s;
                    y[0] = fyl;
                    y[1] = fyl;
                    y[2] = fyr;
                    y[3] = fyr;
                    cpgline(4, x, y);
                    y[0] = -fyl;
                    y[1] = -fyl;
                    y[2] = -fyr;
                    y[3] = -fyr;
                    cpgline(4, x, y);
                }
            } else {
                x[0] = fxl; y[0] = fyl;
                x[1] = fxr; y[1] = fyr;
                if (draw.data) {
                    cpgline(2, x, y);
                }
                if (draw.data_rms) {
                    if ((s = chan2r(c)) == NULL) continue;
                    fyl = (PLFLT)*s;
                    if ((s = chan2r(c+1)) == NULL) continue;
                    fyr = (PLFLT)*s;
                    y[0] = fyl; y[1] = fyr;
                    cpgline(2, x, y);
                    y[0] = -fyl; y[1] = -fyr;
                    cpgline(2, x, y);
                }
            }
        }
        cpgebuf();
    }
#endif
}

void draw_secondary(scanPtr sec, GC local_gc)
{
    int tmp_rms   = draw.data_rms;
    int tmp_data  = draw.data;
    int tmp_histo = draw.histo;
    scanPtr s = vP->s;
    PSSTY tmp_sty = ps.line;
    
    if (!sec) return;

    draw.data_rms  = 0;
    draw.data      = 1;
    draw.histo     = draw.histo_sec;
    vP->s = sec;
    ps.line = ps.secondary;
    
    draw_spectrum(local_gc, local_gc);

    vP->s = s;
    draw.data_rms = tmp_rms;
    draw.data     = tmp_data;
    draw.histo    = tmp_histo;
    ps.line       = tmp_sty;
}

void draw_line(GC gc, int x1, int y1, int x2, int y2)
{
    if (!inside_box(&x1, &y1, &x2, &y2)) return;

    XDrawLine(XtDisplay(gp->graph), XtWindow(gp->graph), gc, x1, y1, x2, y2);
#ifdef USE_PIXMAP_STORAGE
    XDrawLine(XtDisplay(gp->graph), gp->pm, gc, x1, y1, x2, y2);
#endif
}

void draw_xbox(GC gc, int x1, int y1, int x2, int y2)
{
    void draw_line();

    draw_line(gc, x1, y1, x1, y2);
    draw_line(gc, x1, y2, x2, y2);
    draw_line(gc, x2, y2, x2, y1);
    draw_line(gc, x2, y1, x1, y1);
}

void draw_xcircle(GC gc, int x, int y, int r)
{
    if (!point_is_inside(x,y) || r <= 0) return;

    XDrawArc(XtDisplay(gp->graph), XtWindow(gp->graph), gc,
             x-r/2, y-r/2, r, r, 0, 64*360);
#ifdef USE_PIXMAP_STORAGE
    XDrawArc(XtDisplay(gp->graph), gp->pm, gc,
             x-r/2, y-r/2, r, r, 0, 64*360);
#endif
}

void draw_filled_xcircle(GC gc, int x, int y, int r)
{
    if (!point_is_inside(x,y) || r <= 0) return;

    XFillArc(XtDisplay(gp->graph), XtWindow(gp->graph), gc,
             x-r/2, y-r/2, r, r, 0, 64*360);
#ifdef USE_PIXMAP_STORAGE
    XFillArc(XtDisplay(gp->graph), gp->pm, gc,
             x-r/2, y-r/2, r, r, 0, 64*360);
#endif
}

void draw_xellipse(GC gc, int x, int y, int dx, int dy)
{
    if (!point_is_inside(x,y) || dx <= 0 || dy <= 0) return;

    XDrawArc(XtDisplay(gp->graph), XtWindow(gp->graph), gc,
             x-dx/2, y-dy/2, dx, dy, 0, 64*360);
#ifdef USE_PIXMAP_STORAGE
    XDrawArc(XtDisplay(gp->graph), gp->pm, gc,
             x-dx/2, y-dy/2, dx, dy, 0, 64*360);
#endif
}

void draw_filled_xellipse(GC gc, int x, int y, int dx, int dy)
{
    if (!point_is_inside(x,y) || dx <= 0 || dy <= 0) return;

    XFillArc(XtDisplay(gp->graph), XtWindow(gp->graph), gc,
             x-dx/2, y-dy/2, dx, dy, 0, 64*360);
#ifdef USE_PIXMAP_STORAGE
    XFillArc(XtDisplay(gp->graph), gp->pm, gc,
             x-dx/2, y-dy/2, dx, dy, 0, 64*360);
#endif
}

void draw_filled_xbox(GC gc, int x1, int y1, int x2, int y2)
{
    int t, xmax, xmin, ymin, ymax;

    if (x1 > x2) { t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { t = y1; y1 = y2; y2 = t; }
    xmin = vP->min_x;
    xmax = vP->min_x + vP->box_w;
    ymin = vP->min_y - vP->box_h;
    ymax = vP->min_y;
    if (x1 < xmin) x1 = xmin;
    if (x2 > xmax) x2 = xmax;
    if (y1 < ymin) y1 = ymin;
    if (y2 > ymax) y2 = ymax;
    if (x1 > x2) return;
    if (y1 > y2) return;
    XFillRectangle(XtDisplay(gp->graph), XtWindow(gp->graph), gc,
                   x1, y1, x2-x1, y2-y1);
#ifdef USE_PIXMAP_STORAGE
    XFillRectangle(XtDisplay(gp->graph), gp->pm, gc, x1, y1, x2-x1, y2-y1);
#endif
}

void draw_filled_xpol(GC gc, int x[], int y[], int npoints)
{
    int n, m=0;
    XPoint *pts = NULL;
    XRectangle rect[1];
    
    for (n=0; n<npoints; n++) {
        if (!point_is_inside(x[n], y[n])) m++;
    }
    
    /* If any of the points in x[], y[] is outside the view (i.e. m > 0),
       we need to set the clip mask into gc, when plotting is done it
       must be removed. */
    if (m > 0) {
        rect[0].x = vP->min_x;
        rect[0].y = vP->min_y - vP->box_h;
        rect[0].width = vP->box_w;
        rect[0].height = vP->box_h;
        XSetClipRectangles(XtDisplay(gp->graph), gc, 0, 0,
                           rect, 1, Unsorted);

    }
    
    pts = (XPoint *)XtMalloc(npoints * sizeof(XPoint));
    
    if (!pts) return;    
    
    for (n=0; n<npoints; n++) {
        pts[n].x = (short) x[n];
        pts[n].y = (short) y[n];
    }
    
    XFillPolygon(XtDisplay(gp->graph), XtWindow(gp->graph), gc,
                 pts, npoints, Complex, CoordModeOrigin);
#ifdef USE_PIXMAP_STORAGE
    XFillPolygon(XtDisplay(gp->graph), gp->pm, gc,
                 pts, npoints, Complex, CoordModeOrigin);
#endif

    XtFree((char *)pts);
    
    /* Remove the clip mask if necessary */
    if (m > 0) {
        XSetClipMask(XtDisplay(gp->graph), gc, None);
    }
}

static int inside_x_range(int x)
{
    if (x >= vP->min_x && x <= vP->min_x + vP->box_w) return 1;
    return 0;
}

static int inside_y_range(int y)
{
    if (y >= vP->min_y - vP->box_h && y <= vP->min_y) return 1;
    return 0;
}

int point_is_inside(int x, int y)
{
    if (inside_x_range(x) && inside_y_range(y)) return 1;
    return 0;
}

static point *find_intercepts(point p1, point p2, int *npoints)
{
    int n = 0;
    int x, y;
    point tmp;
    static point p[4];
    
    if (p1.y != p2.y) {
        if (p1.y > p2.y) {
           tmp = p1;
           p1 = p2;
           p2 = tmp;
        }
        y = vP->min_y;
        x = p1.x + ((p2.x - p1.x)*(y - p1.y))/(p2.y - p1.y);
        if (point_is_inside(x, y) && y >= p1.y && y <= p2.y) {
            p[n].x = x;
            p[n].y = y;
            n++;
        }
        y = vP->min_y - vP->box_h;
        x = p1.x + ((p2.x - p1.x)*(y - p1.y))/(p2.y - p1.y);
        if (point_is_inside(x, y) && y >= p1.y && y <= p2.y) {
            p[n].x = x;
            p[n].y = y;
            n++;
        }
    }
    if (p1.x != p2.x) {
        if (p1.x > p2.x) {
           tmp = p1;
           p1 = p2;
           p2 = tmp;
        }
        x = vP->min_x;
        y = p1.y + ((p2.y - p1.y)*(x - p1.x))/(p2.x - p1.x);
        if (point_is_inside(x, y) && x >= p1.x && x <= p2.x) {
            p[n].x = x;
            p[n].y = y;
            n++;
        }
        x = vP->min_x + vP->box_w;
        y = p1.y + ((p2.y - p1.y)*(x - p1.x))/(p2.x - p1.x);
        if (point_is_inside(x, y) && x >= p1.x && x <= p2.x) {
            p[n].x = x;
            p[n].y = y;
            n++;
        }
    }
    if (n == 0) return NULL;
    
    *npoints = n;
    
    return p;
}

static int inside_box(int *x1, int *y1, int *x2, int *y2)
{
    int n;
    point p1, p2, *p;
    
    if (point_is_inside(*x1, *y1) && point_is_inside(*x2, *y2)) return 1;

    if ((*x1) == (*x2) && (*y1) == (*y2)) return 0;
    
    p1.x = *x1;
    p1.y = *y1;
    p2.x = *x2;
    p2.y = *y2;
    
    p = find_intercepts(p1, p2, &n);
    
    if (!p) return 0;
    
    if (n == 1) {
        if (point_is_inside(*x1, *y1)) {
            *x2 = p->x;
            *y2 = p->y;
        } else {
            *x1 = p->x;
            *y1 = p->y;
        }
    } else if (n == 2) {
        *x1 = p[0].x;
        *y1 = p[0].y;
        *x2 = p[1].x;
        *y2 = p[1].y;
    } else {
        return 0;
    }
    
    return 1;
}

void draw_frame(GC l_gc, int adjust, int special_sub)
{
    int drawframe = 0;
#ifdef HAVE_LIBPGPLOT
    double amount;
    PLFLT x1, x2, y1, y2;
    PSBOX *box;
    
    int CheckWedge(), GetWedgePos();
#endif

    if (vP->mode == SHOW_WEDGE) {
        drawframe = draw.wframe;
    } else {
        drawframe = draw.frame;
    }

    if (drawframe)
        draw_xbox(l_gc, vP->min_x, vP->min_y,
                  vP->min_x + vP->box_w, vP->min_y - vP->box_h);

#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        if (adjust) {
            if (CheckWedge()) {
                cpgqvp(0, &x1, &x2, &y1, &y2);
                switch (GetWedgePos()) {
                    case POS_RIGHT:
                        amount = (x2-x1)*8.0/60.0;
                        x2 -= amount;
                        /* x2 = x1 + (x2-x1)*52.0/60.0; */
                        strcpy(ps.wedge.x.label, "bc");
                        strcpy(ps.wedge.y.label, "bcmistv");
                        break;
                    case POS_LEFT:
                        amount = (x2-x1)*8.0/60.0;
                        x1 += amount;
                        strcpy(ps.wedge.x.label, "bc");
                        strcpy(ps.wedge.y.label, "bcnistv");
                        break;
                    case POS_ABOVE:
                        amount = (y2-y1)*8.0/60.0;
                        y2 -= amount;
                        strcpy(ps.wedge.x.label, "bcmist");
                        strcpy(ps.wedge.y.label, "bc");
                        break;
                    case POS_BELOW:
                        amount = (y2-y1)*8.0/60.0;
                        y1 += amount;
                        strcpy(ps.wedge.x.label, "bcnist");
                        strcpy(ps.wedge.y.label, "bc");
                        break;
                }
                cpgsvp(x1, x2, y1, y2);
            }
            x1 = (PLFLT)vP->xleft;
            x2 = (PLFLT)vP->xright;
            y1 = (PLFLT)vP->ylower;
            y2 = (PLFLT)vP->yupper;
            if (vP->mode == SHOW_POSPOS) {
                cpgwnad(x1, x2, y1, y2);
            } else {
                cpgswin(x1, x2, y1, y2);
            }
        }
        if (drawframe) {
            if (vP->mode == SHOW_SUBSPE) {
                if (special_sub) {
                    box = &ps.TRsubbox;
                } else {
                    box = &ps.subbox;
                }
            } else if (vP->mode == SHOW_WEDGE) {
                switch (GetWedgePos()) {
                    case POS_RIGHT:
                        strcpy(ps.wedge.x.label, "bc");
                        strcpy(ps.wedge.y.label, "bcmistv");
                        break;
                    case POS_LEFT:
                        strcpy(ps.wedge.x.label, "bc");
                        strcpy(ps.wedge.y.label, "bcnistv");
                        break;
                    case POS_ABOVE:
                        strcpy(ps.wedge.x.label, "bcmist");
                        strcpy(ps.wedge.y.label, "bc");
                        break;
                    case POS_BELOW:
                        strcpy(ps.wedge.x.label, "bcnist");
                        strcpy(ps.wedge.y.label, "bc");
                        break;
                }
                box = &ps.wedge;
            } else {
                if (special_sub) {
                    box = &ps.TRsubbox;
                } else {
                    box = &ps.box;
                }
            }
            SetPGStyle(&(box->style));
            cpgbox(box->x.label, box->x.inc, box->x.ticks,
                   box->y.label, box->y.inc, box->y.ticks);
        }
    }
#endif
}

static int find_letter(char c, char *str)
{
    if (str == NULL) return 0;

    while (*str != '\0') {
        if (*str == c) return 1;
        str++;
    }
    return 0;
}

static int get_ticks(TICKS *ticks, double v_min, double v_max, TICKS *t)
{
    double range = v_max - v_min, try_step=0.0, try_sub=0.0, val;
    int lrange, i, j, n, *s, *ss, ns;
    static int step[3]       = {5, 2, 1};
    static int substep[3]    = {5, 4, 5};
    static int mapstep[3]    = {5, 2, 1};
    static int mapsubstep[3] = {5, 4, 5};
    int digits;

    if (range == 0.0) return -1;

    if (range < 0.0) {
        range = -range;
        val = v_min;
        v_min = v_max;
        v_max = val;
    }
    
    if (t) {
        ticks->first  = t->step * ceil(v_min/t->step);
        ticks->last   = t->step * floor(v_max/t->step);
        ticks->step   = t->step;
        ticks->nsteps = 1 + NINT((ticks->last - ticks->first)/t->step);
        ticks->digits = 1;
        ticks->substep = t->substep;
    } else {
        if (vP->mode == SHOW_POSPOS || vP->mode == SHOW_ALLSPE) {
            ns = sizeof(mapstep)/sizeof(int);
            s  = mapstep;
            ss = mapsubstep;
        } else {
            ns = sizeof(mapstep)/sizeof(int);
            s  = step;
            ss = substep;
        }
        lrange = (int)ceil(log10(range));
        n = j = 0;
        while (n < MIN_NO_OF_TICKS) {
            j++;
            for (i=0; i<ns; i++) {
                try_step = (double)s[i] * pow(10.0, (double)(lrange-j));
                try_sub  = try_step/(double)ss[i];
                n = 1 + NINT(floor(v_max/try_step) - ceil(v_min/try_step));
                if (n >= MIN_NO_OF_TICKS) break;
            }
            if (j > 10) return -1;
        }
        ticks->first  = try_step * ceil(v_min/try_step);
        ticks->last   = try_step * floor(v_max/try_step);
        ticks->step   = try_step;
        ticks->nsteps = n;
        ticks->digits = 1;
        ticks->substep = try_sub;
    }

    val = ticks->first;
    while (val - ticks->substep > v_min)
        val -= ticks->substep;
    ticks->subfirst = val;

    val = ticks->last;
    while (val + ticks->substep < v_max)
        val += ticks->substep;
    ticks->sublast = val;

    ticks->nsubsteps = 1 + NINT((ticks->sublast -
                                    ticks->subfirst)/ticks->substep);

    for (n=0; n<ticks->nsteps; n++) {
        val = fabs(ticks->first + (double)n*ticks->step);
        digits = 1;
        if (val != 0.0) digits = NINT(log10(val/ticks->step)) + 1;
        if (digits > ticks->digits)
            ticks->digits = digits;
    }

#ifdef DEEPDEBUG
    printf("      v_min = %f   v_max = %f\n", v_min, v_max);
    printf("Main: first, last, step = %f, %f, %f\n",
           ticks->first, ticks->last, ticks->step);
    printf("Sub:  first, last, step = %f, %f, %f\n",
           ticks->subfirst, ticks->sublast, ticks->substep);
#endif
    return 0;
}

void draw_ticks(GC gc, char *xcode, char *ycode)
{
    int    i, x1, x2, y1, y2, statusX, statusY;
    int    xsubtick_length, ysubtick_length;
    int    xtick_length = vP->box_h/TICK_FACTOR;
    int    ytick_length = vP->box_w/TICK_FACTOR;
    double val;
    TICKS  *t, yticks, xticks;
    int      nseg;
    XSegment *seg;

    char  *get_numeric_label();
    void   draw_numeric_label(GC, char, int, int, char *);

    if (xtick_length < MIN_TICK_LENGTH)
        xtick_length = MIN_TICK_LENGTH;
    if (ytick_length < MIN_TICK_LENGTH)
        ytick_length = MIN_TICK_LENGTH;

    if (xtick_length < ytick_length)
        xtick_length = ytick_length;
    else
        ytick_length = xtick_length;

    if (vP->mode == SHOW_POSPOS || vP->mode == SHOW_ALLSPE) {
        if (vP->box_h > vP->box_w) {
            statusY = get_ticks(&yticks, vP->ylower, vP->yupper, NULL);
            statusX = get_ticks(&xticks, vP->xleft,  vP->xright, &yticks);
        } else {
            statusX = get_ticks(&xticks, vP->xleft,  vP->xright, NULL);
            statusY = get_ticks(&yticks, vP->ylower, vP->yupper, &xticks);
        }
    } else {
        statusY = get_ticks(&yticks, vP->ylower, vP->yupper, NULL);
        statusX = get_ticks(&xticks, vP->xleft,  vP->xright, NULL);
    }

    if (statusY == 0) {
        t = &yticks;
        nseg = 2*(t->nsteps + t->nsubsteps);
        seg = (XSegment *)malloc(nseg * sizeof(XSegment));
        nseg = 0;
        for (i=0; i<t->nsteps; i++) {
            x1 = vP->min_x;
            if (AX_Y_INVTICKS)
                x2 = x1 - ytick_length;
            else
                x2 = x1 + ytick_length;
            val = t->first + (double)i * t->step;
            y1 = yunit2y(val);
            y2 = y1;
            if (AX_LEFTLABEL)
                draw_numeric_label(gc, 'y', (AX_Y_INVTICKS == 1) ? x2 : x1, y1,
                                   get_numeric_label(val, t->step, t->digits));
            if (AX_LEFTTICKS) {
                seg[nseg].x1 = x1; seg[nseg].x2 = x2;
                seg[nseg].y1 = y1; seg[nseg].y2 = y2;
                nseg++;
            }
            x1 += vP->box_w;
            if (AX_Y_INVTICKS)
                x2 = x1 + ytick_length;
            else
                x2 = x1 - ytick_length;
            if (AX_RIGHTLABEL)
                draw_numeric_label(gc, 'R', (AX_Y_INVTICKS == 1) ? x2 : x1, y1,
                                   get_numeric_label(val, t->step, t->digits));
            if (AX_RIGHTTICKS) {
                seg[nseg].x1 = x1; seg[nseg].x2 = x2;
                seg[nseg].y1 = y1; seg[nseg].y2 = y2;
                nseg++;
            }
        }
        if (AX_Y_SUBTICKS) {
            ysubtick_length = ytick_length/2;
            for (i=0; i<t->nsubsteps; i++) {
                x1 = vP->min_x;
                if (AX_Y_INVTICKS)
                    x2 = x1 - ysubtick_length;
                else
                    x2 = x1 + ysubtick_length;
                val = t->subfirst + (double)i * t->substep;
                y1 = yunit2y(val);
                y2 = y1;
                if (AX_LEFTTICKS) {
                    seg[nseg].x1 = x1; seg[nseg].x2 = x2;
                    seg[nseg].y1 = y1; seg[nseg].y2 = y2;
                    nseg++;
                }
                x1 += vP->box_w;
                if (AX_Y_INVTICKS)
                    x2 = x1 + ysubtick_length;
                else
                    x2 = x1 - ysubtick_length;
                if (AX_RIGHTTICKS) {
                    seg[nseg].x1 = x1; seg[nseg].x2 = x2;
                    seg[nseg].y1 = y1; seg[nseg].y2 = y2;
                    nseg++;
                }
            }
        }
        XDrawSegments(XtDisplay(gp->graph), XtWindow(gp->graph), gc,
                      seg, nseg);
#ifdef USE_PIXMAP_STORAGE
        XDrawSegments(XtDisplay(gp->graph), gp->pm, gc, seg, nseg);
#endif
        free(seg);
    }

    if (statusX == 0) {
        t = &xticks;
        nseg = 2*(t->nsteps + t->nsubsteps);
        seg = (XSegment *)malloc(nseg * sizeof(XSegment));
        nseg = 0;
        for (i=0; i<t->nsteps; i++) {
            val = t->first + (double)i * t->step;
            x1 = xunit2x(val);
            x2 = x1;
            y1 = vP->min_y;
            if (AX_X_INVTICKS)
                y2 = y1 + xtick_length;
            else
                y2 = y1 - xtick_length;
            if (AX_BOTTOMLABEL)
                draw_numeric_label(gc, 'x', x1, (AX_X_INVTICKS == 1) ? y2 : y1,
                                   get_numeric_label(val, t->step, t->digits));
            if (AX_BOTTOMTICKS) {
                seg[nseg].x1 = x1; seg[nseg].x2 = x2;
                seg[nseg].y1 = y1; seg[nseg].y2 = y2;
                nseg++;
            }
            y1 -= vP->box_h;
            if (AX_X_INVTICKS)
                y2 = y1 - xtick_length;
            else
                y2 = y1 + xtick_length;
            if (AX_TOPLABEL)
                draw_numeric_label(gc, 'T', x1, (AX_X_INVTICKS == 1) ? y2 : y1,
                                   get_numeric_label(val, t->step, t->digits));
            if (AX_TOPTICKS) {
                seg[nseg].x1 = x1; seg[nseg].x2 = x2;
                seg[nseg].y1 = y1; seg[nseg].y2 = y2;
                nseg++;
            }
        }
        if (AX_X_SUBTICKS) {
            xsubtick_length = xtick_length/2;
            for (i=0; i<t->nsubsteps; i++) {
                val = t->subfirst + (double)i * t->substep;
                x1 = xunit2x(val);
                x2 = x1;
                y1 = vP->min_y;
                if (AX_X_INVTICKS)
                    y2 = y1 + xsubtick_length;
                else
                    y2 = y1 - xsubtick_length;
                if (AX_BOTTOMTICKS) {
                    seg[nseg].x1 = x1; seg[nseg].x2 = x2;
                    seg[nseg].y1 = y1; seg[nseg].y2 = y2;
                    nseg++;
                }
                y1 -= vP->box_h;
                if (AX_X_INVTICKS)
                    y2 = y1 - xsubtick_length;
                else
                    y2 = y1 + xsubtick_length;
                if (AX_TOPTICKS) {
                    seg[nseg].x1 = x1; seg[nseg].x2 = x2;
                    seg[nseg].y1 = y1; seg[nseg].y2 = y2;
                    nseg++;
                }
            }
        }
        XDrawSegments(XtDisplay(gp->graph), XtWindow(gp->graph), gc,
                      seg, nseg);
#ifdef USE_PIXMAP_STORAGE
        XDrawSegments(XtDisplay(gp->graph), gp->pm, gc, seg, nseg);
#endif
        free(seg);
    }
}

char *get_numeric_label(double val, double step, int digits)
{
    static char str[80];
    char exp_str[80];
    int d_places = 0, numbers;

    if (step < 1.0) d_places = 1 + (int) log10(1.0/step);
    numbers = digits - d_places;

    if (d_places == 0 && fabs(val) >= 1.0)
        numbers = 1 + (int) log10(fabs(val));

    if (fabs(val) < EPSILON*step) {
        sprintf(exp_str, "%%%d.%df", 1+numbers+d_places, d_places);
        val = 0.0;
    } else if (fabs(val) < 0.001 || fabs(val) >= 1000000.0) {
        if (val < 0) {
            sprintf(exp_str, "%%%d.%de", 7+digits-1, digits-1);
        } else {
            sprintf(exp_str, "%%%d.%de", 6+digits-1, digits-1);
        }
    } else {
        if (val < 0) {
            sprintf(exp_str, "%%%d.%df", 2+numbers+d_places, d_places);
        } else {
            sprintf(exp_str, "%%%d.%df", 1+numbers+d_places, d_places);
        }
    }
    sprintf(str, exp_str, val);
    return str;
}

char *GetNumericLabel(double val, int digits, int decimals, int type)
{
    static char str[80];
    char format[80];
    
    switch (type) {
        case 0: /* Integer */
            if (digits > 0)
                sprintf(format, "%%%dd", digits);
            else
                strcpy(format, "%d");
            break;
        case 1: /* Float */
            if (digits > 0)
                sprintf(format, "%%%d.%df", digits+2, decimals);
            else
                sprintf(format, "%%.%df", decimals);
            break;
        case 2: /* Exponential */
            if (digits > 0)
                sprintf(format, "%%%d.%de", digits+6, digits-1);
            else
                sprintf(format, "%%.%de", decimals);
            break;
        default: /* On-the-fly */
            if (digits > 0 && decimals > -1)
                sprintf(format, "%%%d.%dg", digits, decimals);
            else if (decimals > -1)
                sprintf(format, "%%.%dg", decimals);
            else
                sprintf(format, "%%%dg", decimals);
            break;
    }
    sprintf(str, format, val);
    
    return str;
}

XID GetFIDFromGC(GC gc)
{
    XGCValues    xgc;
    Display     *dpy = XtDisplay(gp->graph);

    XGetGCValues(dpy, gc, GCFont, &xgc);

    return (XID)xgc.font;
}


void draw_string(GC gc, int x, int y, char *str)
{
    if (!str) return;

    XDrawString(XtDisplay(gp->graph), XtWindow(gp->graph), gc,
                         x, y, str, strlen(str));
#ifdef USE_PIXMAP_STORAGE
    XDrawString(XtDisplay(gp->graph), gp->pm, gc, x, y, str, strlen(str));
#endif
}

void DrawAbsLabel(GC gc, double xw, double yw, double align, char *str)
{
    int          x, y;
    int          dir, ascent, descent;
    XCharStruct  str_info;

    XID GetFIDFromGC();

    XQueryTextExtents(XtDisplay(gp->graph), GetFIDFromGC(gc), str, strlen(str),
                      &dir, &ascent, &descent, &str_info);

    x = xunit2x(xw) - NINT(align * (double)str_info.width);
    y = yunit2y(yw) + descent;
    draw_string(gc, x, y, str);

#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.marker);
        cpgptxt((PLFLT)xw, (PLFLT)yw, (PLFLT)0.0, (PLFLT)align, str);
    }
#endif
}

void DrawRelLabel(GC gc, double xr, double yr, double align, char *str)
{
    int         x, y;
    int         dir, ascent, descent;
    double      xw, yw;
    XCharStruct str_info;

    XID GetFIDFromGC();

    XQueryTextExtents(XtDisplay(gp->graph), GetFIDFromGC(gc), str, strlen(str),
                      &dir, &ascent, &descent, &str_info);

    xw = vP->xleft  + xr * vP->xrange;
    yw = vP->ylower + yr * vP->yrange;
    x = xunit2x(xw) - NINT(align * (double)str_info.width);
    y = yunit2y(yw) + descent;
    draw_string(gc, x, y, str);

#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        SetPGStyle(&ps.ilabel);
        cpgptxt((PLFLT)xw, (PLFLT)yw, (PLFLT)0.0, (PLFLT)align, str);
    }
#endif
}

void draw_header(scanPtr s, mapPtr m, GC gc)
{
    string tmp, itime, sno;
    int refChan, is_seq = 0;
    double y=1.30, f;
#ifdef HAVE_LIBPGPLOT
    PSSTY pstmp = ps.ilabel;
#endif
    
    double xmap(scanPtr), ymap(scanPtr);
    char   *GetRAStr(double), *GetDECStr(double);
    char   *GetLongStr(double), *GetLatStr(double);
    char   *GetEpochStr(char, double);
    
    if (!s && vP->mode == SHOW_SPE) return;
    if (!m && vP->mode == SHOW_POSPOS) return;
    
    if (vP->mode != SHOW_SPE && vP->mode != SHOW_POSPOS) return;
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        pstmp = ps.ilabel;
        ps.ilabel = ps.header;
    }
#endif

    if (vP->mode == SHOW_SPE) {
      y = 1.30;
      is_seq = (vP->from->sequence);

      if (s->coordType == COORD_TYPE_GAL) {
          sprintf(tmp, "Source:%s Gal. coord.: %s %s Offset:(%+.1f\",%+.1f\")",
                  s->name, GetLongStr(s->x0), GetLatStr(s->y0),
                  is_seq ? s->tx : xmap(s), is_seq ? s->ty : ymap(s));
      } else {
          sprintf(tmp, "Source:%s %s %s (%s) Offset:(%+.1f\",%+.1f\")",
                  s->name, GetRAStr(s->x0), GetDECStr(s->y0),
                  GetEpochStr(s->epoch, s->equinox),
                  is_seq ? s->tx : xmap(s), is_seq ? s->ty : ymap(s));
      }
      DrawRelLabel(gc, 0.0, y, 0.0, tmp); y -= 0.05;
      sprintf(tmp,
            "Date:%4d%02d%02d  UT:%dh%02dm%02ds  Az,El:%.1f,%.1f  Molecule:%s",
                  s->date.Year, s->date.Month, s->date.Day,
                  s->date.Hour, s->date.Min, s->date.Sec,
                  s->az, s->el,
                  s->molecule);

      DrawRelLabel(gc, 0.0, y, 0.0, tmp); y -= 0.05;

      refChan = s->nChan/2;
      f = 1000.0*(s->freq0 + (double)(refChan) * s->freqres);
      sprintf(tmp, "NCh=%d  RefCh=%d  Fref(im)=%.2f(%.1f) MHz  Vref=%.2f km/s",
              s->nChan, refChan, f, 2000.0*s->lofreq - f,
              s->vel0 + (double)(refChan) * s->velres);
      DrawRelLabel(gc, 0.0, y, 0.0, tmp); y -= 0.05;

      sprintf(tmp, "BW=%.0f MHz  Freq.res.=%.1f kHz  Vel.res.=%.5f km/s",
              1000.0*(double)(s->nChan)*fabs(s->freqres),
              1.0e6*(s->freqres), s->velres);
      DrawRelLabel(gc, 0.0, y, 0.0, tmp); y -= 0.05;

      if (s->subscan > 0) {
          sprintf(sno, "Scan. no:%d.%0d", s->scan_no, s->subscan);
      } else {
          sprintf(sno, "Scan. no:%d", s->scan_no);
      }
      if (s->int_time >= 10000.0) {
          sprintf(itime, "Int. time=%.2f h", s->int_time/3600.0);
      } else if (s->int_time < 100.0) {
          sprintf(itime, "Int. time=%.1f s", s->int_time);
      } else {
          sprintf(itime, "Int. time=%.0f s", s->int_time);
      }
      sprintf(tmp, "Tsys=%.1f K  %s  %s  Vlsr=%.2f km/s",
              s->tsys, itime, sno, s->vlsr);
      DrawRelLabel(gc, 0.0, y, 0.0, tmp); y -= 0.05;

      if (s->mom.nchan > 0) {
          sprintf(tmp, "Integr.int.=%f(%f) K km/s  Sigma=%g K",
                  s->mom.iint, s->mom.iunc, s->mom.sigma);
          DrawRelLabel(gc, 0.0, y, 0.0, tmp);
      }
    } else if (vP->mode == SHOW_POSPOS) {
      y = 1.25;
      if (m->coordType == COORD_TYPE_GAL) {
          sprintf(tmp, "Source:%s Gal. coord.: %s %s)",
                  m->name, GetLongStr(m->x0), GetLatStr(m->y0));
      } else {
          sprintf(tmp, "Source:%s %s %s (%s)",
                  m->name, GetRAStr(m->x0), GetDECStr(m->y0),
                  GetEpochStr(m->epoch, m->equinox));
      }
      DrawRelLabel(gc, 0.0, y, 0.0, tmp); y -= 0.05;
      sprintf(tmp,
            "Date:%4d%02d%02d  UT:%dh%02dm%02ds Molecule:%s",
                  m->date.Year, m->date.Month, m->date.Day,
                  m->date.Hour, m->date.Min, m->date.Sec,
                  m->molecule);
      DrawRelLabel(gc, 0.0, y, 0.0, tmp); y -= 0.05;
      sprintf(tmp, "Long. offsets: %.1f %.1f %.1f (%d)",
                  m->xleft, m->xright, m->xspacing, m->i_no);
      DrawRelLabel(gc, 0.0, y, 0.0, tmp); y -= 0.05;
      sprintf(tmp, "Lat. offsets: %.1f %.1f %.1f (%d)  PA=%.1f",
                  m->ylower, m->yupper, m->yspacing, m->j_no,
		  m->posAngle);
      DrawRelLabel(gc, 0.0, y, 0.0, tmp); y -= 0.05;
    }
    
#ifdef HAVE_LIBPGPLOT
    if (pgplot) {
        ps.ilabel = pstmp;
    }
#endif
}

void draw_numeric_label(GC gc, char type, int x, int y, char *str)
{
    int          dir, ascent, descent;
    int          X, Y;
    XCharStruct  str_info;

    XID GetFIDFromGC();

    XQueryTextExtents(XtDisplay(gp->graph), GetFIDFromGC(gc), str, strlen(str),
                      &dir, &ascent, &descent, &str_info);

    switch (type) {
        case 'x':
        case 'B':
            X = x - str_info.width/2;
            Y = y + ascent + descent;
            break;
        case 'T':
            X = x - str_info.width/2;
            Y = y - descent - 3;
            break;
        case 'y':
        case 'L':
            X = x - str_info.width - 3;
            Y = y + descent;
            break;
        case 'R':
            X = x + 3;
            Y = y + descent;
            break;
        case 'o':
            X = x - str_info.width/2;
            Y = y + descent;
            break;
        default:
            return;
    }
    draw_string(gc, X, Y, str);
}

void draw_axis_label(GC gc, char type, char *str)
{
    int          x, y;
    int          dir, ascent, descent;
    XCharStruct  str_info;

    XID GetFIDFromGC();

    XQueryTextExtents(XtDisplay(gp->graph), GetFIDFromGC(gc), str, strlen(str),
                      &dir, &ascent, &descent, &str_info);

    switch (type) {
        case 'x':
            x = (vP->min_x + vP->box_w) - str_info.width/2;
            y = (vP->min_y + 2*(ascent + descent));
            break;
        case 'y':
            x = vP->min_x - str_info.width/2;
            y = vP->min_y - vP->box_h - descent - ascent;
            break;
        case 'Y':
            x = vP->min_x + vP->box_w - str_info.width/2;
            y = vP->min_y - vP->box_h - descent - ascent;
            break;
        case 't':
            x = (2*vP->min_x + vP->box_w - str_info.width)/2;
            y = vP->min_y - vP->box_h - 1*(ascent + descent);
            break;
        case 'T':
            x = (2*vP->min_x + vP->box_w - str_info.width)/2;
            y = vP->min_y - vP->box_h - 2*(ascent + descent);
            break;
        case 'B':
            x = (2*vP->min_x + vP->box_w - str_info.width)/2;
            y = (2*vP->min_y + 5*(ascent + descent))/2;
            break;
        default:
            return;
    }
    draw_string(gc, x, y, str);
}

void draw_contour_dot(GC gc, char c, int size, int x, int y)
{
    void draw_filled_xbox(), draw_filled_xcircle(), draw_line();

    if (c == 's')
        draw_filled_xbox(gc, x - size, y + size, x + size, y - size);
    else if (c == 'c')
        draw_filled_xcircle(gc, x, y, size);
    else if (c == 'x') {
        draw_line(gc, x-size, y-size, x+size, y+size);
        draw_line(gc, x-size, y+size, x+size, y-size);
    } else if (c == '+') {
        draw_line(gc, x-size, y, x+size, y);
        draw_line(gc, x, y-size, x, y+size);
    }
}

void DrawContourDot(GC gc, char c, double size, double x, double y)
{
    int p_size;
#ifdef HAVE_LIBPGPLOT
    PLFLT fx[1], fy[1];
#endif
    
    if (size >= 1.0)
        p_size = NINT(size);
    else
        p_size = NINT(size * (double)(vP->box_w));
    
    draw_contour_dot(gc, c, p_size, xunit2x(x), yunit2y(y));
    
    if (!pgplot) return;
        
#ifdef HAVE_LIBPGPLOT
    SetPGStyle(&ps.marker);
    
    fx[0] = (PLFLT)x;
    fy[0] = (PLFLT)y;
    if (c == 's') {
        cpgpt(1, fx, fy, 851);
    } else if (c == 'c') {
        cpgpt(1, fx, fy, 903);
    } else if (c == 'x') {
        cpgpt(1, fx, fy, 846);
    } else if (c == '+') {
        cpgpt(1, fx, fy, 845);
    }
#endif
}

GC IterateColor()
{
    static int n = 0;
    COLOR *c;

    GC SetColor();
    COLOR *GetColorInfo();

    c = GetColorInfo();
    if (n >= c->ncols) n = 0;

    return SetColor(n++);
}

GC SetColor(int n)
{
    XGCValues gcv;
    GC gc;
    COLOR *c;
    
    GC GetGC();
    COLOR *GetColorInfo();

    c = GetColorInfo();
    if (n < 0 || n >= c->ncols) return NULL;

    gcv.foreground = c->cols[n];
    gc = GetGC(GCForeground, &gcv);

    return gc;
}

#ifdef USE_IMAGE_STORAGE   
void SaveImage()
{
    int n;
    Arg wargs[5];
    
    n = 0;
    XtSetArg(wargs[n], XtNwidth,  &(gp->width));  n++;
    XtSetArg(wargs[n], XtNheight, &(gp->height)); n++;
    XtGetValues(gp->graph, wargs, n);
    
    if (gp->xi) XDestroyImage(gp->xi);
    
    gp->xi = XGetImage(XtDisplay(gp->graph), XtWindow(gp->graph),
                       0, 0, gp->width, gp->height,
                       AllPlanes, ZPixmap);
}

void RedrawImage()
{
    int n;
    Dimension w, h;
    Arg wargs[5];
    
    if (!(gp->xi)) return;
    
    n = 0;
    XtSetArg(wargs[n], XtNwidth,  &w);  n++;
    XtSetArg(wargs[n], XtNheight, &h); n++;
    XtGetValues(gp->graph, wargs, n);
    
    if (w != gp->width || h != gp->height) return;
    
    XPutImage(XtDisplay(gp->graph), XtWindow(gp->graph),
              gp->gcStd, gp->xi,
              0, 0, 0, 0, gp->width, gp->height);
}
#endif
