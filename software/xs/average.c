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
#include <math.h>
#include <Xm/Xm.h>

#include "defines.h"
#include "global_structs.h"

/*** External variables ***/
extern int nbox;
extern BLINE bl;
extern VIEW *vP;

void   PostErrorDialog(Widget, char *);
void   PostWarningDialog(Widget, char *);
void   PostMessageDialog(Widget, char *);
Widget PostWaitingDialog(Widget, char *, Widget *, int);
void   SetWaitingScale(Widget, int);
    
void   UpdateData(int, int);
void   send_line(char *);
void   SetWatchCursor(int);
void   update_bl_data(scanPtr);
void   SetStdView();
int    SetViewMode(int, scanPtr, MAP*, scatter *);
int    AllocSpectrum(int);
double SpecUnitRes(scanPtr, int);
    
int        count_scans(DataSetPtr);
scanPtr    copy_scan(DataSetPtr, scanPtr);
scanPtr    copy_scanheader(DataSetPtr, int, scanPtr);
list       scan_iterator(list, DataSetPtr);
list      *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);
void       DeleteLastDataSet();

/*** Local variables ***/
#define VELRES_EPS     1.0e-5

#define AVE_ERR_INTERNAL -1
#define AVE_ERR_NONE      0
#define AVE_ERR_DIFFCHAN  1
#define AVE_ERR_ZEROSIG   2
#define AVE_ERR_ZEROTSYS  3
#define AVE_ERR_MEMORY    4
#define AVE_ERR_INCOMRES  5
#define AVE_ERR_TOOFEW    6

static int add_two(scanPtr a, scanPtr s, int atype, int no)
{
    int n;
    string buf;
    double w1=1.0, w2=1.0, ws=2.0;
    
    if (!a || !s) return AVE_ERR_INTERNAL;
    
    if (a->nChan != s->nChan) {
        sprintf(buf, "Error: Different no of channels (%d and %d)!\n",
                a->nChan, s->nChan);
        send_line(buf);
        return AVE_ERR_DIFFCHAN;
    }
    
    switch (atype) {
        case AVETYPE_SAME:
            w1 = (double)a->added; 
            w2 = 1.0;
            break;
        case AVETYPE_NONE:
            w1 = w2 = 1.0;
            break;
        case AVETYPE_RMS:
            update_bl_data(a);
            if (bl.sigma != 0.0)
                w1 = 1.0/bl.sigma/bl.sigma;
            else {
                sprintf(buf, "Error: Zero sigma for a!\n");
                send_line(buf);
                return AVE_ERR_ZEROSIG;
            }
            update_bl_data(s);
            if (bl.sigma != 0.0)
                w2 = 1.0/bl.sigma/bl.sigma;
            else {
                sprintf(buf, "Error: Zero sigma for s!\n");
                send_line(buf);
                return AVE_ERR_ZEROSIG;
            }
            break;
        case AVETYPE_TSYS:
            if (a->tsys != 0.0) {
                w1 = a->int_time/a->tsys/a->tsys;
            } else {
                sprintf(buf, "Error: Zero Tsys for a!\n");
                send_line(buf);
                return AVE_ERR_ZEROTSYS;
            }
            if (s->tsys != 0.0) {
                w2 = s->int_time/s->tsys/s->tsys;
            } else {
                sprintf(buf, "Error: Zero Tsys for s!\n");
                send_line(buf);
                return AVE_ERR_ZEROTSYS;
            }
            break;
        case AVETYPE_TIME:
            w1 = a->int_time;
            w2 = s->int_time;
            break;
    }
    for (n=0; n<a->nChan; n++) {
        if (atype == AVETYPE_IRMS) {
            if (a->e[n] != 0.0 && s->e[n] != 0.0) {
                w1 = 1.0/a->e[n]/a->e[n];
                w2 = 1.0/s->e[n]/s->e[n];
            } else
                return AVE_ERR_ZEROSIG;
        }
        ws = w1 + w2;
        a->d[n] = (w1*a->d[n] + w2*s->d[n]);
        if (atype != AVETYPE_NONE) a->d[n] /= ws;
        if (atype == AVETYPE_IRMS)
            a->e[n] = 1.0/sqrt(ws);
    }

    a->int_time += s->int_time;
    a->added += 1;

    if (ws != 0.0)
        a->tsys = sqrt((a->tsys*a->tsys*w1 + s->tsys*s->tsys*w2)/ws);
            
    return AVE_ERR_NONE;
}

static void addup_scan(scanPtr a, scanPtr s, double w)
{
    int n;
    
    if (!s) {
        for (n=0; n<a->nChan; n++) {
            a->d[n] *= w;
        }
    } else {
        for (n=0; n<a->nChan; n++) {
            if (n < s->nChan) a->d[n] += w * s->d[n];
        }
    }
}

static void rms_ave(scanPtr a, scanPtr s, int action)
{
    double w;
    static double wsum=0.0;
    
    if (action == 0) {        /* initialize */
        w = 1.0/a->mom.sigma/a->mom.sigma;
        wsum = w;
        addup_scan(a, NULL, w);
        a->tsys = a->tsys * a->tsys * w;
    } else if (action == 1) { /* add contribution */
        w = 1.0/s->mom.sigma/s->mom.sigma;
        addup_scan(a, s, w);
        wsum += w;
        a->int_time += s->int_time;
        a->tsys += s->tsys * s->tsys * w;
    } else if (action == 2) { /* end, normalize */
        addup_scan(a, NULL, 1.0/wsum);
        a->tsys = sqrt(a->tsys/wsum);
    }
}

static int AddScans(int atype, int show)
{
    int n, err=AVE_ERR_NONE, mode = vP->mode, diff_pos=0, diff_chan=0;
    int diff_fres=0, zero_rms = 0;
    Point a_pos;
    string buf;
    list curr = NULL;
    scanPtr ave = NULL, s;
    DataSetPtr dsp;
    Widget wait=NULL, scale;
    
    a_pos.x = a_pos.y = 0.0;
    
    dsp = new_dataset(get_listlist(), "Averaged scan", NULL);
    if (!dsp) {
        return AVE_ERR_MEMORY;
    }
    
    n = count_scans(vP->from);
    if (n > WAITSPECTRA)
        wait = PostWaitingDialog(NULL, "Averaging spectra...",
                                 &scale, n);
    
    n = 0;
    while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
        if (wait) SetWaitingScale(scale, n+1);
        s = (scanPtr)DATA(curr);
        if (atype == AVETYPE_ADDRMS) {
            update_bl_data(s);
            if (s->mom.sigma == 0.0) {
                zero_rms = 1;
                continue;
            }
        }
        if (!ave) {
            ave = copy_scan(dsp, s);
            if (!ave) {
                if (wait) XtDestroyWidget(wait);
                return AVE_ERR_MEMORY;
            }
            if (vP->from->sequence) {
                ave->xoffset = s->tx;
                ave->yoffset = s->ty;
            }
            a_pos.x = ave->xoffset;
            a_pos.y = ave->yoffset;
            if (atype == AVETYPE_ADDRMS) {
                rms_ave(ave, NULL, 0);
            }
        } else {
            if (atype == AVETYPE_ADDRMS) {
                rms_ave(ave, s, 1);
            } else {
                err = add_two(ave, s, atype, n);
                if (err != AVE_ERR_NONE) {
                    if (wait) XtDestroyWidget(wait);
                    return err;
                }
            }
            if (!diff_pos) {
                if (fabs(a_pos.x - s->tx) > 0.01 ||
                    fabs(a_pos.y - s->ty) > 0.01) {
                    diff_pos = 1;
                }
            }
            if (!diff_chan) {
               if (ave->nChan != s->nChan) {
                   diff_chan = 1;
               }
            }
            if (!diff_fres) {
                if (fabs(ave->freqres - s->freqres)/ave->freqres > 1.0e-5) {
                    diff_fres = 1;
                }
            }
        }
        n++;
    }
    
    if (atype == AVETYPE_ADDRMS && n > 0) {
        rms_ave(ave, NULL, 2);
    }
    
    if (wait) XtDestroyWidget(wait);
    
    if (n > 0) {
        if (vP->m) vP->m->saved = 1;

        if (atype == AVETYPE_RMS)
            sprintf(dsp->name, "%s acc. rms. ave. (%d)", ave->name, n);
        else if (atype == AVETYPE_ADDRMS)
            sprintf(dsp->name, "%s add. rms. ave. (%d)", ave->name, n);
        else if (atype == AVETYPE_TSYS)
            sprintf(dsp->name, "%s Tsys ave. (%d)", ave->name, n);
        else if (atype == AVETYPE_TIME)
            sprintf(dsp->name, "%s int. time ave. (%d)", ave->name, n);
        else if (atype == AVETYPE_SAME)
            sprintf(dsp->name, "%s eq. weight ave. (%d)", ave->name, n);
        else
            sprintf(dsp->name, "%s average (%d)", ave->name, n);

        vP->from = vP->to = dsp;
        SetViewMode(SHOW_SPE, ave, vP->m, vP->p);

        if (mode != SHOW_SPE) {
            SetStdView();
            if (show)
                UpdateData(SCALE_BOTH, REDRAW);
            else
                UpdateData(SCALE_BOTH, NO_REDRAW);
        } else {
            if (show)
                UpdateData(SCALE_ONLY_Y, REDRAW);
            else
                UpdateData(SCALE_ONLY_Y, NO_REDRAW);
        }
    } else {
        DeleteLastDataSet();
    }
    
    if (diff_pos) {
        sprintf(buf, "The average\n'%s'\n\
consists of scans in different positions.", dsp->name);
        PostWarningDialog(NULL, buf);
    }
    if (diff_chan) {
        sprintf(buf, "The average\n'%s'\n\
consists of scans with different no of channels.", dsp->name);
        PostWarningDialog(NULL, buf);
    }
    if (diff_fres) {
        sprintf(buf, "The average\n'%s'\n\
consists of scans with different frequency resolutions.", dsp->name);
        PostWarningDialog(NULL, buf);
    }
    if (zero_rms) {
        if (n > 0) {
            sprintf(buf,
                "At least some data (skipped in average) had zero rms!");
        } else {
            sprintf(buf,
                "All data had zero rms! Forgot the baseline boxes?");
        }
        PostWarningDialog(NULL, buf);
    }
    
    return AVE_ERR_NONE;
}

static int AddPosScans(int atype, int show)
{
    int n, m, k, no, err=AVE_ERR_NONE, mode = vP->mode;
    string buf;
    scanPtr s, a=NULL;
    DataSetPtr dsp;
    list curr=NULL, next;
    Widget wait=NULL, scale;
    
    void obtain_map_info(Widget, char *, XtPointer);
    
    dsp = new_dataset(get_listlist(), "Averaged scans", vP->from);
    if (!dsp) {
        return AVE_ERR_MEMORY;
    }
    n = count_scans(vP->from);
    
    SetWatchCursor(True);
    
    if (n > 50)
        wait = PostWaitingDialog(NULL, "Averaging spectra...",
                                 &scale, n);
    
    n = k = m = 0;
    while ( (curr = scan_iterator(curr, vP->from)) != NULL) { /* old dataset */
        if (wait) SetWaitingScale(scale, n+1);
        s = (scanPtr)DATA(curr);
	s->added = 1;
        next = NULL;
        no = 1;
        while ( (next = scan_iterator(next, dsp) ) != NULL) { /* new dataset */
            a = (scanPtr)DATA(next);
            if (s->i != a->i || s->j != a->j) continue;
            err = add_two(a, s, atype, no);
            if (err != AVE_ERR_NONE) {
                SetWatchCursor(False);
                if (wait) XtDestroyWidget(wait);
                return err;
            }
            k++;
            no++;
        }
        if (no == 1) {
            a = copy_scan(dsp, s);
            if (!a) {
                if (wait) XtDestroyWidget(wait);
                return AVE_ERR_MEMORY;
            }
            m++;
        }
        n++;
    }
    
    if (wait) XtDestroyWidget(wait);
    
    vP->from = vP->to = dsp;
    
    sprintf(buf, "Added together %d spectra.", k);
    send_line(buf);
    
    sprintf(buf, "No. of spectra is now %d (%d).", m, n);
    send_line(buf);
    
    sprintf(dsp->name, "%s averaged (%d scans)", a->name, m);
    
    obtain_map_info(NULL, "no_update_map_data", NULL);
    
    if (mode == SHOW_SPE) {
        SetViewMode(SHOW_SPE, a, vP->m, vP->p);
        UpdateData(SCALE_ONLY_Y, REDRAW);
    } else if (mode == SHOW_ALLSPE) {
        SetViewMode(SHOW_ALLSPE, a, vP->m, vP->p);
        UpdateData(SCALE_BOTH, REDRAW);
    } else {
        SetViewMode(SHOW_ALLSPE, a, vP->m, vP->p);
        SetStdView();
        UpdateData(SCALE_BOTH, REDRAW);
    }
        
    SetWatchCursor(False);
    
    return AVE_ERR_NONE;
}

static int GetAveType(char *cmd)
{
    int atype;
    
    if (strncmp(cmd, "accrms", 6) == 0) {
        atype = AVETYPE_RMS;
        if (!nbox) atype = AVETYPE_IRMS;
    } else if (strncmp(cmd, "tsys", 4) == 0) {
        atype = AVETYPE_TSYS;
    } else if (strncmp(cmd, "time", 4) == 0) {
        atype = AVETYPE_TIME;
    } else if (strncmp(cmd, "same", 4) == 0) {
        atype = AVETYPE_SAME;
    } else if (strncmp(cmd, "addrms", 6) == 0) {
        atype = AVETYPE_ADDRMS;
    } else {
        atype = AVETYPE_NONE;
    }
    
    return atype;
}

#define DSWAP(x, y) {double tmp=(x); (x)=(y); (y)=tmp;}

static double check_resolution(double eps)
{
    list curr = NULL;
    scanPtr s;
    double res, r;
    
    if (count_scans(vP->from) < 2) return 0.0;
    
    res = fabs(SpecUnitRes(vP->s, vP->xunit));
    if (res == 0.0) return 0.0;
    
    while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
        s = (scanPtr)DATA(curr);
        r = fabs(SpecUnitRes(s, vP->xunit));
        if (fabs(r/res - 1.0) >= eps) {
            res = 0.0;
            break;
        }
    }
    
    return res;
}

static scanPtr find_extent(double res, DataSetPtr dsp)
{
    int first=1, n;
    double x1, x2, x_min=0.0, x_max=0.0, xres;
    double f1, f2, f_min=0.0, f_max=0.0, fres;
    list curr = NULL;
    string buf;
    scanPtr s = NULL, new = NULL;
    
    void send_line();
    double SpecUnitConv();
    
    if (vP->xunit == UNIT_CHA || vP->xunit == UNIT_FOFF ||
        count_scans(vP->from) < 2) return new;

    while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
        s = vP->s = (scanPtr)DATA(curr);
        if (first) {
            x1 = SpecUnitConv(UNIT_VEL, UNIT_CHA, 0.0);
            x2 = SpecUnitConv(UNIT_VEL, UNIT_CHA, (double)(s->nChan-1));
            if (x1 > x2) DSWAP(x1, x2)
            x_min = x1; x_max = x2;

            f1 = SpecUnitConv(UNIT_FRE, UNIT_CHA, 0.0);
            f2 = SpecUnitConv(UNIT_FRE, UNIT_CHA, (double)(s->nChan-1));
            if (f1 > f2) DSWAP(f1, f2)
            f_min = f1; f_max = f2;
            first = 0;
            continue;
        }
        x1 = SpecUnitConv(UNIT_VEL, UNIT_CHA, 0.0);
        x2 = SpecUnitConv(UNIT_VEL, UNIT_CHA, (double)(s->nChan-1));
        if (x1 > x2) DSWAP(x1, x2)
        if (x1 < x_min) { x_min = x1; }
        if (x2 > x_max) { x_max = x2; }
        
        f1 = SpecUnitConv(UNIT_FRE, UNIT_CHA, 0.0);
        f2 = SpecUnitConv(UNIT_FRE, UNIT_CHA, (double)(s->nChan-1));
        if (f1 > f2) DSWAP(f1, f2)
        if (f1 < f_min) { f_min = f1; }
        if (f2 > f_max) { f_max = f2; }
    }
    
    if (vP->xunit == UNIT_VEL) {
        n = (int)((x_max - x_min)/res) + 1;
        xres = res;
        fres = -(f_min+f_max)*res/2.0/SPEEDOFLIGHT;
    } else {
        if (vP->xunit == UNIT_FMHZ) res /= 1000.0;
        n = (int)((f_max - f_min)/res) + 1;
        fres = res;
        xres = -2.0*SPEEDOFLIGHT*res/(f_min+f_max);
    }

    x_max = x_min + (double)(n - 1)*fabs(xres);
    f_max = f_min + (double)(n - 1)*fabs(fres);
    
    new = copy_scanheader(dsp, n, s);
    if (!new) {
        sprintf(buf,
	  "Error in join: Resulting no. of channels %d is too large!\n", n);
        send_line(buf);
        return NULL;
    } else {
        sprintf(buf, "New spectrum created with %d no. of channels.\n", n);
        send_line(buf);
    }
    
    if (vP->xunit == UNIT_FRE || vP->xunit == UNIT_FMHZ) {
        new->freq0   = f_min;
        new->freqn   = f_max;
        new->freqres = fres;
        new->velres  = xres;
        new->vel0    = x_max;
    } else if (vP->xunit == UNIT_VEL) {
        new->vel0    = x_min;
        new->velres  = xres;
        new->freqres = fres;
        new->freq0   = f_max;
        new->freqn   = f_min;
    }

    for (n=0; n<new->nChan; n++) {
        new->d[n] = 0.0;
        new->e[n] = 0.0;
    }
    
    return new;
}

static void join_average(scanPtr s, scanPtr a)
{
    int c, n;
    double x, ws, wa;
    
    double SpecUnitConv();
    
    for (c=0; c<s->nChan; c++) {
        vP->s = s;
        x = SpecUnitConv(vP->xunit, UNIT_CHA, (double)c);
        vP->s = a;
        n = NINT(SpecUnitConv(UNIT_CHA, vP->xunit, x));
        if (n >= 0 && n < a->nChan) {
            wa = a->e[n];
            if (wa != 0.0) wa = 1.0/wa/wa;
            ws = s->e[c];
            if (ws != 0.0) ws = 1.0/ws/ws;
            if (wa + ws > 0.0) {
                a->d[n] = (wa * a->d[n] + ws * s->d[c]) / (wa + ws);
                a->e[n] = 1.0/sqrt(wa + ws);
            }
        }
    }
}

static int JoinScans(int atype, int show)
{
    int n = 0, mode = vP->mode;
    double res;
    list curr = NULL;
    scanPtr ave;
    DataSetPtr dsp;
    
    dsp = new_dataset(get_listlist(), "Joined scan", NULL);
    if (!dsp) {
        return AVE_ERR_MEMORY;
    }

    res = check_resolution(VELRES_EPS);
    if (res <= 0.0) return AVE_ERR_INCOMRES;
    
    ave = find_extent(res, dsp);
    if (!ave) return AVE_ERR_MEMORY;
    
    while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
        join_average((scanPtr)DATA(curr), ave);
        n++;
    }
    
    vP->from = vP->to = dsp;
    sprintf(dsp->name, "%s composite of %d scans", ave->name, n);
    SetViewMode(SHOW_SPE, ave, vP->m, vP->p);
    
    if (mode != SHOW_SPE) {
        SetStdView();
        if (show)
            UpdateData(SCALE_BOTH, REDRAW);
        else
            UpdateData(SCALE_BOTH, NO_REDRAW);
    } else {
        if (show)
            UpdateData(SCALE_ONLY_Y, REDRAW);
        else
            UpdateData(SCALE_ONLY_Y, NO_REDRAW);
    }
    
    return AVE_ERR_NONE;
}

static int subtract_scan(scanPtr s, scanPtr p)
{
    int n;
    
    for (n=0; n<s->nChan; n++) {
        if (n < p->nChan) {
            s->d[n] -= p->d[n];
            s->e[n] = sqrt(s->e[n] * s->e[n] + p->e[n] * p->e[n]);
        } else {
            s->d[n] = 0.0;
            s->e[n] = 0.0;
        }
    }
    
    return AVE_ERR_NONE;
}

static int SubtractScans()
{
    int n, no, err=AVE_ERR_NONE, mode = vP->mode;
    string buf;
    scanPtr new=NULL, s=NULL;
    DataSetPtr dsp;
    list curr=NULL;
    Widget wait=NULL, scale;
    
    void obtain_map_info(Widget, char *, XtPointer);
    
    no = n = count_scans(vP->from);
    if (n < 2) return AVE_ERR_TOOFEW;
    
    dsp = new_dataset(get_listlist(), "Seq. subtr. scans", vP->from);
    if (!dsp) {
        return AVE_ERR_MEMORY;
    }
    
    SetWatchCursor(True);
    
    if (n > 50)
        wait = PostWaitingDialog(NULL, "Subtracting spectra...",
                                 &scale, no);
    
    n = 0;
    while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
        if (wait) SetWaitingScale(scale, n+1);
        if (!s) {
            s = (scanPtr)DATA(curr);
        } else {
            new = copy_scan(dsp, s);
            s = (scanPtr)DATA(curr);
            if (!new) break;
            err = subtract_scan(new, s);
        }
        n++;
    }
    
    if (wait) XtDestroyWidget(wait);
    
    sprintf(dsp->name, "%s subtracted (%d scans)", vP->from->name, n-1);
    
    vP->from = vP->to = dsp;
    
    sprintf(buf, "No. of spectra is now %d.", n-1);
    send_line(buf);
    
    obtain_map_info(NULL, "no_update_map_data", NULL);
    
    if (mode == SHOW_SPE) {
        SetViewMode(SHOW_SPE, new, vP->m, vP->p);
        UpdateData(SCALE_ONLY_Y, REDRAW);
    } else if (mode == SHOW_ALLSPE) {
        SetViewMode(SHOW_ALLSPE, new, vP->m, vP->p);
        UpdateData(SCALE_BOTH, REDRAW);
    } else {
        SetViewMode(SHOW_ALLSPE, new, vP->m, vP->p);
        SetStdView();
        UpdateData(SCALE_BOTH, REDRAW);
    }
        
    SetWatchCursor(False);
    
    return AVE_ERR_NONE;
}

void AverageScans(Widget w, char *cmd, XtPointer cd)
{
    int atype, err = 0;
    
    atype = GetAveType(&cmd[2]);
    
    if (cmd[0] == 'A') {
        if (cmd[1] == 'M')
            err = AddScans(atype, 0);
        else
            err = AddScans(atype, 1);
    } else if (cmd[0] == 'P') {
        if (cmd[1] == 'M')
            err = AddPosScans(atype, 0);
        else
            err = AddPosScans(atype, 1);
    } else if (cmd[0] == 'J') {
        err = JoinScans(atype, 1);
    } else if (cmd[0] == 'S') {
        err = SubtractScans();
    }
    
    if (err == AVE_ERR_NONE) return;
    
    /* Handle errors below */
    
    DeleteLastDataSet();
    
    switch (err) {
        case AVE_ERR_INTERNAL:
            PostErrorDialog(w,
                   "Internal averaging error. Data may have been corrupted.");
            break;
        case AVE_ERR_DIFFCHAN:
            PostErrorDialog(w,
                   "The spectra have different no of channels.");
            break;
        case AVE_ERR_ZEROSIG:
            PostErrorDialog(w,
                   "Spectra lack sigma estimate.");
            break;
        case AVE_ERR_ZEROTSYS:
            PostErrorDialog(w,
                   "Spectra lack Tsys estimate.");
            break;
        case AVE_ERR_MEMORY:
            PostErrorDialog(w,
                   "No memory left to save resulting average(s).");
            break;
        case AVE_ERR_INCOMRES:
            PostErrorDialog(w,
                   "Incompatible resolutions in spectra.");
            break;
        case AVE_ERR_TOOFEW:
            PostErrorDialog(w,
                   "Too few spectra to perform operation.");
            break;
    }
}
