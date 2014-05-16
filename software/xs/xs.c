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
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include <Xm/DrawingA.h>
#include <Xm/Label.h>
#if XmVersion >= 2000
#include <Xm/PanedW.h>
#endif
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>
#include <Xm/Frame.h>
#include <Xm/ToggleB.h>
#include <Xm/ArrowB.h>
#include <Xm/ArrowBG.h>
#include <Xm/ToggleBG.h>
#include <Xm/CascadeBG.h>
#include <Xm/Form.h>
#include <Xm/FileSB.h>
#include <Xm/ScrolledW.h>
#if XmVersion <= 1100
#include <X11/StringDefs.h>
#endif
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include "defines.h"
#include "global_structs.h"
#include "global_vars.h"
#include "menu_defs.h"
#include "button_defs.h"
#include "dialogs.h"
#include "autoread.h"
#include "xs.h"

#ifdef STATIC
/*** Warning, this is a kludge ot be able to compile static
     binaries                                                ***/
void _IO_stdin_() { }
#endif

void SetMouseButtons(int left, int middle, int right)
{
    gp->LeftButton   = left; 
    gp->MiddleButton = middle; 
    gp->RightButton  = right; 
}

void SetMemoryWarningLimit(double limit)
{
    if (limit < 0.0) return;
    
    gp->MemoryWarningLimit = limit; 
}

static void process_arguments(int argc, char **argv)
{
    int opt;
    string buf;
    
    void SetFITSReadDir(char *, int);
    
    getcwd(buf, sizeof(string));
    
    while ((opt = getopt(argc, argv, "vph?sf:1:a:2:c:3:")) != EOF) {
        switch (opt) {
            case 'p':   /* use private colormap */
                gp->privateColors = 1;
                break;
            case 's':   /* use also as a server */
                gp->server = 1;
                break;
            case 'f':   /* read fits 1-dim file, FITS vector */
            case '1':
                strcpy(opt_fname, optarg);
                SetFITSReadDir(buf, 1);
                opt_fitstype = 1;
                    break;
            case 'a':
            case '2':   /* read 2-dim FITS file, FITS array */
                strcpy(opt_fname, optarg);
                SetFITSReadDir(buf, 2);
                opt_fitstype = 2;
                    break;
            case 'c':
            case '3':   /* read 3-dim FITS file, FITS cube */
                strcpy(opt_fname, optarg);
                SetFITSReadDir(buf, 3);
                opt_fitstype = 3;
                break;
            case 'h':
            case '?':
                fprintf(stdout, opt_help, PRGNAME);
                exit(0);
                break;
            case 'v':
                fprintf(stdout, opt_version,
		        PRGNAME, XS_VERSION, XS_PATCH, XS_VERDATE);
                exit(0);
                break;
            default:
                fprintf(stderr, "Unknown command line option -%c\n", (char)opt);
                break;
        }
    }
}

int main(int argc, char **argv)
{
    string     buf;
    char      *file_type;
    int        err=0;
#ifdef FANCY_STARTUP
    DRAW       tmp;
#endif

    gp = &globalPars;
    
    init_prefs();   /* This init call must be the first! */

    init_view();    /* This init call must be the second! */
    
    init_gauss_parameters();
    init_baseline_parameters();
    init_file_parameters();
    init_smooth_parameters();
    init_redres_parameters();
    init_clip_parameters();
    init_scale_parameters();
    init_mark_parameters();
    init_maplist();
    init_scatterlist();
    init_togglelist();
    init_fieldlist();
    init_scanlist();
    init_datasets();
    init_polylist();
    init_draw_parameters();
    init_convolve();
    init_memdata();
    init_map_parameters();
    init_scatter_data();
    init_testmap();
    init_macro();

#if XtSpecificationRelease <= 5                                  
    gp->top = XtVaAppInitialize(&(gp->app_cntxt), PKGNAME, NULL, 0,
                                &argc, argv, NULL, NULL);
#else
    gp->top = XtVaOpenApplication(&(gp->app_cntxt), PKGNAME, NULL, 0,
                                  &argc, argv, NULL,
                                  applicationShellWidgetClass,
                                  NULL);
#endif
    XtSetLanguageProc(gp->app_cntxt, NULL, NULL);
  
    gp->privateColors = 0;
    gp->server = 0;
    strcpy(opt_fname, pP->firstFile);
    opt_fitstype = 1;
    
    process_arguments(argc, argv);
    
    if (gp->server)
        sprintf(buf, "%s running as FITS server", PKGNAME);
    else
        strcpy(buf, PKGNAME);
    
    XtVaSetValues(gp->top,
                  XmNtitle, buf,
                  XmNiconName, PKGNAME,
                  NULL);
  
    CreateWindows();     /* Draws all the windows */
    
    check_prefs();       /* Check the version of the prefs file */

    file_type = GetFileType(opt_fname);

    if (!file_type) {
        sprintf(buf, "Argument file '%s' is of unknown type.", opt_fname);
        PostErrorDialog(gp->top, buf);
        err = 1;
    }

  /* Read initial data file, either from command argument or default file */
    if (!err) {
        switch (opt_fitstype) {
            case 1:
                err = read_file(file_type, opt_fname, vP->to);
                if (!err) {
                    strcpy(vP->to->name, vP->s->name);
                }
                InitView(SHOW_SPE);
                break;
            case 2:
                InitView(SHOW_POSPOS);
                if (strcmp(file_type, "fits") == 0) {
                    err = LoadFITS(opt_fname, "array");
                }
                break;
            case 3:
                if (strcmp(file_type, "fits") == 0) {
                    err = LoadFITS(opt_fname, "cube");
                    if (!err) {
                        obtain_map_info(NULL, "no_update_map_data", NULL);
                    }
                }
                InitView(SHOW_SPE);
                break;
        }
    } else {
        InitView(SHOW_SPE);
    }
  
#ifdef FANCY_STARTUP 
    XRaiseWindow(XtDisplay(gp->top), XtWindow(gp->top));
    tmp = draw;
    draw.ticks = 0;
    draw.labels = 0;
    draw.frame = 0;
    draw.data = 0;
#endif
    
    UpdateData(SCALE_BOTH, REDRAW);

#ifdef FANCY_STARTUP 
    if (!err) draw.data = 1;
    DoZoom("b", pow(1.04, 100.0));
    for (n=0; n<100; n++)
    DoZoom("b", 1./1.04);
    draw = tmp;
#endif
  
  /* Initialize all PGPLOT parameters (to their default values) */
    init_PS_parameters();

  /* Initialize the tracker strings */
    draw_tracker_strings(0, 0, 0);
    UpdateHeaderInfo();
    UpdatePolylineInfo();

  /* Post a warning msg if the colour allocation wasn't successful */    
    ColorAllocationWarning();

  /* Enter the event loop */
    while (1) {
        MyLoop(0);
    }
}

static void handle_graph_exposures(Widget w, char *cmd,
                                   XmDrawingAreaCallbackStruct *cb)
{
    XEvent *ev = cb->event;
    XExposeEvent *e;
    Region r;
    
#ifdef USE_IMAGE_STORAGE
    void RedrawImage();
#endif
    
    if (!ev) return;
    
    e = &(ev->xexpose);
    
    r = XCreateRegion();
    XtAddExposureToRegion(ev, r);
    if (!XEmptyRegion(r)) {
#ifdef USE_IMAGE_STORAGE
        RedrawImage();
#else
#ifdef USE_PIXMAP_STORAGE
        XCopyArea(XtDisplay(w), gp->pm, XtWindow(w), gp->gcLine,
                  e->x, e->y, e->width, e->height, e->x, e->y);
#else
        draw_main();
#endif
#endif
    }
    XDestroyRegion(r);
}

static void handle_configure(Widget w, char *cmd, XEvent *ev)
{
    int clear;
    Dimension widget_width, widget_height;
    XConfigureEvent *e;
#ifdef USE_PIXMAP_STORAGE
    Display *dpy = XtDisplay(gp->graph);
#endif
    
    if (!ev) return;
    
    if (ev->type != ConfigureNotify) return;
    
    e = &(ev->xconfigure);
            
    XtVaGetValues(gp->graph, XtNwidth,  &widget_width,
                  XtNheight, &widget_height, NULL);
    if (widget_width != vP->main_w ||
        widget_height != vP->main_h) {
        ResizeView(widget_width, widget_height);
#ifdef USE_PIXMAP_STORAGE
        if (gp->pm) XFreePixmap(dpy, gp->pm);
        gp->pm = XCreatePixmap(dpy, DefaultRootWindow(dpy),
                               widget_width, widget_height,
                               DefaultDepthOfScreen(XtScreen(gp->graph)));
        gp->p_w = widget_width;
        gp->p_h = widget_height;
#endif
        clear = draw.clear;
        draw.clear = 1;
        draw_main();
        draw.clear = clear;
    }
}

static Atom register_atom(Display *dpy, char *atom_name)
{
    Atom tmp;
    
    tmp = XInternAtom(dpy, atom_name, True);
    
    return tmp;
}

int SaveAutoFITS(char *file, auto_fits *af)
{
    int len;
    FILE *fp;
    
    fp = fopen(file, "w");
    if (!fp) return 1;
    
    len = fwrite(af->data, sizeof(char), af->reclen * af->nrec, fp);
    fclose(fp);
    
    if (len != af->reclen * af->nrec) return 1;
    
    return 0;
}

static void handle_property_change(Widget w, char *cmd, XEvent *ev)
{
    Atom type;
    int format, read_data = 0, err=0;
    unsigned long nitems, left;
    unsigned char *retdata;
    static auto_fits *af = NULL;
    Display *dpy = XtDisplay(gp->top);
    Window root = DefaultRootWindow(dpy);
    static DataSetPtr d = NULL;
    string tmpfile, buf;
    static string sender;
    DataSetPtr tmp_d;
    string mode;

    list      *get_listlist();
    DataSetPtr new_dataset(list *, char *, DataSetPtr);
    void       DeleteLastDataSet();
    char      *GetTmpFile(const char *);
    void       XS_system(), send_line();

    /* Is the event of the correct type? */
    if (ev->type != PropertyNotify) return;
    
    /* Is the property set in the root window? */
    if (ev->xproperty.window != root) return;
    
    if (ev->xproperty.atom == None) return;
    
    FITS_SEND = register_atom(dpy, ATOM_SEND);
    FITS_MESG = register_atom(dpy, ATOM_MESG);
    FITS_DATA = register_atom(dpy, ATOM_DATA);
    
    FITS_INFO      = register_atom(dpy, ATOM_INFO);
    FITS_INFO_TYPE = register_atom(dpy, ATOM_INFO_TYPE);
    
    /* Is the property atom the one containing the info? */
    if (ev->xproperty.atom == FITS_SEND) {
        if (XGetWindowProperty(dpy, root, FITS_SEND,
                               0, sizeof(string),
                               False, XA_STRING,
                               &type, &format, &nitems, &left,
                               &retdata) == Success &&
            type == XA_STRING && retdata) {
            strcpy(sender, (char *)retdata);
            /* sprintf(buf, "Sender info received from %s.", sender);
            send_line(buf); */
            XFree((void *)retdata);
        }
        return;
    }
    if (ev->xproperty.atom == FITS_MESG) {
        if (XGetWindowProperty(dpy, root, FITS_MESG,
                               0, sizeof(string),
                               False, XA_STRING,
                               &type, &format, &nitems, &left,
                               &retdata) == Success &&
            type == XA_STRING && retdata) {
            sprintf(buf, "[%s] %s", sender, (char *)retdata);
            send_line(buf);
            XFree((void *)retdata);
        }
        return;
    }
    if (ev->xproperty.atom == FITS_INFO) {
        if (XGetWindowProperty(dpy, root, FITS_INFO,
                               0, sizeof(auto_fits),
                               False, FITS_INFO_TYPE,
                               &type, &format, &nitems, &left,
                               &retdata) == Success &&
            type == FITS_INFO_TYPE && retdata) {
            af = (auto_fits *)retdata;
            af->data = NULL;
            /* sprintf(buf, "Server info received (%dx%d) from %s.",
                    af->nrec, af->reclen, sender);
            send_line(buf); */
        }
        return;
    }
         
    if (ev->xproperty.atom != FITS_DATA) return;
    
    /* Have we had the info yet? */
    if (!af) return;
    
    /* Does the property contain the correct structure? */
    if (XGetWindowProperty(dpy, root, FITS_DATA,
                           0, af->reclen * af->nrec,
                           False, XA_STRING,
                           &type, &format, &nitems, &left,
                           &retdata) == Success &&
        type == XA_STRING && retdata) {
        af->data = (char *)retdata;
        /* Now we can extract the data using mode af->mode */
        switch (af->type) {
            case AUTOMODE_SINGLE:
                strcpy(mode, "fits");
                break;
            case AUTOMODE_MAP_FIRST:
                strcpy(mode, "fits");
                break;
            case AUTOMODE_SEQ_FIRST:
                strcpy(mode, "seqfits");
                break;
            case AUTOMODE_MAP_APPEND:
                strcpy(mode, "fits");
                break;
            case AUTOMODE_SEQ_APPEND:
                strcpy(mode, "seqfits");
                break;
            case AUTOMODE_END:
                d = NULL;
                return;
            default:
                return;
        }
        sprintf(buf, "Server data received: %dx%d bytes in mode %s.\n",
                af->nrec, af->reclen, mode);
        send_line(buf);
       
        /* The easiest thing at the moment is to save the data into a tmp file
           and then read it the normal way */
        strcpy(tmpfile, GetTmpFile(".fits"));
        err = SaveAutoFITS(tmpfile, af);
        XFree((void *)retdata);
        if (err) {
            PostErrorDialog(w, "Couldn't save the server FITS data properly.");
            af = NULL;
            return;
        }
        switch (af->type) {
            case AUTOMODE_SINGLE:
            case AUTOMODE_MAP_FIRST:
            case AUTOMODE_SEQ_FIRST:
                d = new_dataset(get_listlist(), "FITS", NULL);
                if (d && !read_file(mode, tmpfile, d)) {
                    vP->from = vP->to = d;
                    strcpy(d->name, ((scanPtr)DATA(d->scanlist))->name);
                    read_data = 1;
                } else {
                    if (d) DeleteLastDataSet();
                }
                break;
            case AUTOMODE_MAP_APPEND:
            case AUTOMODE_SEQ_APPEND:
                tmp_d = view.to;
                view.to = d;
                if (d && !read_file(mode, tmpfile, NULL)) {
                    strcpy(d->name, ((scanPtr)DATA(d->scanlist))->name);
                    read_data = 1;
                }
                view.to = tmp_d;
                break;
        }
        sprintf(buf, "%s %s", pP->unixRmCmd, tmpfile);
        XS_system(buf, 0);
        if (read_data == 1) {
            sprintf(buf, "Data scan %s from %s properly received.",
                    view.s->name, sender);
            send_line(buf);
            UpdateData(SCALE_BOTH, REDRAW);
        }
        if (af) XFree((void *)af);
        af = NULL;
    }
}

static void handle_leave(Widget w, char *cmd, XEvent *ev)
{
    draw_tracker_strings(0, 0, 0);
}

static void handle_kb_input(Widget w, char *cmd, XEvent *ev)
{
    int m = vP->mode;
    double rel_scroll = 0.1;
    XKeyEvent *e;
    char key_str[10];
    list curr = NULL;
    KeySym ksym;
    Point p;
    PolyLine *vpl;
    
    void delete_spectrum(double, double, int);
    void edit_map_gauss(Point *);
    void TogglePolyLines(Point *, int);
    void EditPolyLines(Widget, Point *);
    void DeletePolyLines(Widget, Point *);
    void Post2DimFitDialog(Widget, Gauss2D *, XtPointer);
    void DoScroll(char *, double);
    PolyLine *GetFirstPolyLine(Point *);
    void setup_previous_scans(DataSetPtr);
    void ToggleSpeUnit();
    list previous_scan(list, DataSetPtr);
    
    if (!ev) return;
    
    e = &(ev->xkey);
    
    strcpy(key_str, "");
    XLookupString(e, key_str, sizeof(key_str), &ksym, NULL);
    p.x = x2xunit(e->x);
    p.y = y2yunit(e->y);
    /* printf("key '%c'  ksym: %d  state: %d\n",
           key_str[0], (int)ksym, e->state); */
    switch (key_str[0]) {
        case 'r':            /* Redraw plot */
            redraw_graph(gp->top, "redraw", NULL);
            break;
        case 'u':            /* Redraw plot */
            redraw_graph(gp->top, "update", NULL);
            break;
        case 'x':            /* change x scale */
            AdjustScale(gp->top, "x", NULL);
            break;
        case 'X':            /* change x scale unit */
            if (m == SHOW_SPE || m == SHOW_ALLSPE || m == SHOW_SUBSPE) {
                ToggleSpeUnit();
            }
            break;
        case 'y':            /* change temperature scale */
            AdjustScale(gp->top, "y", NULL);
            break;
        case '>':            /* Zoom in x2.0 */
            DoZoom("b", 0.5);
            break;
        case '<':            /* Zoom out x0.5 */
            DoZoom("b", 2.0);
            break;
        case 'g':            /* Store last gaussian */
            new_gaussian(NULL, "draw", NULL);
            break;
        case 'h':           /* toggle header */
            toggle_any(NULL, "header", NULL);
            break;
        case 'm':            /* Store last moment box */
            new_box(gp->top, "mom", NULL);
            break;
        case 'b':            /* Store last baseline box */
            new_box(gp->top, "box", NULL);
            break;
        case 'c':            /* Activate channel modification */
            if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL)
                TogglePolyLines(&p, 1); /* Close */
            else
                channel_mod(gp->top, NULL, NULL);
            break;
        case 'o':
            if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL)
                TogglePolyLines(&p, 0); /* Open */
            break;
        case 'f':            /* fit gaussians using M-L method */
            if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL)
                Post2DimFitDialog(gp->graph, NULL, NULL);
            else
                do_fit(gp->top, NULL, NULL);
            break;
        case 'v':            /* fit polynomial using M-L method */
            if (m == SHOW_POSPOS) {
                vpl = GetFirstPolyLine(&p);
                if (!vpl) {
                    PostWarningDialog(w, "Couldn't find a polyline.");
                    break;
                }
                PostVelPosDialog(gp->graph, 0.0, 0.0, 0.0, 0.0, vpl);
            } else
                do_baseline_fit(gp->top, NULL, NULL);
            break;
        case '2':           /* toggle secondary show mode */
            toggle_any(NULL, "sec", NULL);
            break;
        case 'd':           /* delete spectrum in SHOW_ALLSPE */
            if (m == SHOW_ALLSPE)
                delete_spectrum(p.x, p.y, 1);
            else if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL)
                DeletePolyLines(NULL, &p);
            break;
        case 'D':           /* delete spectrum in SHOW_ALLSPE */
            if (m == SHOW_ALLSPE) delete_spectrum(p.x, p.y, 0);
            break;
        case 'n':           /* select next spectrum */
            if ((m == SHOW_SPE || m == SHOW_ALLSPE) &&
                count_scans(vP->from) > 1) {
                while ((curr = scan_iterator(curr, vP->from)) != NULL) {
                    if (vP->s == (scanPtr)DATA(curr)){
                         curr = scan_iterator(curr, vP->from);
                         break;
                    }
                }
                if (curr == NULL) curr = scan_iterator(curr, vP->from);
                vP->s = (scanPtr) DATA(curr);
                UpdateData(SCALE_BOTH, REDRAW);
            }
            break;
        case 'p':           /* select previous spectrum */
            if ((m == SHOW_SPE || m == SHOW_ALLSPE) &&
                count_scans(vP->from) > 1) {
                setup_previous_scans(vP->from);
                while ((curr = scan_iterator(curr, vP->from)) != NULL) {
                    if (vP->s == (scanPtr)DATA(curr)){
                         curr = previous_scan(curr, vP->from);
                         break;
                    }
                }
                if (curr == NULL) curr = vP->from->scanlist;
                vP->s = (scanPtr) DATA(curr);
                UpdateData(SCALE_BOTH, REDRAW);
            }
            break;
        case 'e':           /* edit gaussian par. */
            if (m == SHOW_ALLSPE)
                edit_map_gauss(&p);
            else if (m == SHOW_SPE)
                edit_map_gauss(NULL);
            else if (m == SHOW_POSPOS || m == SHOW_VELPOS || m == SHOW_POSVEL)
                EditPolyLines(NULL, &p);
            break;
        case 'A':
            box_reset(NULL, "all", NULL);
            break;
        case 'H':           /* toggle histogram/line mode */
            toggle_any(NULL, "histo", NULL);
            break;
        case 's':
	    if (m == SHOW_SPE || m == SHOW_ALLSPE) {
	                    /* subtract polynomial fit */
                remove_poly(NULL, NULL, NULL);
	    } else {        /* toggle select scatter points */
                toggle_any(NULL, "tagscatterpts", NULL);
	    }
            break;
        case 'S':           /* toggle gauss sum */
            toggle_any(NULL, "gsum", NULL);
            break;
        case 'I':           /* plot individual gaussians */
            toggle_any(NULL, "gind", NULL);
            break;
        case 'Z':           /* toggle zero-line */
            toggle_any(NULL, "zline", NULL);
            break;
        case 'B':           /* toggle boxes */
            toggle_any(NULL, "boxes", NULL);
            break;
        case 'P':           /* toggle polynomial fitting */
            toggle_any(NULL, "poly", NULL);
            break;
        case 'M':           /* toggle markers */
            toggle_any(NULL, "markers", NULL);
            break;
        case 'C':           /* Swap fg and bg */
            SwapForegroundAndBackground();
            break;
        default:
            switch (e->state) {
                case 1:                     /* Shift modifier     */
                case 2:                     /* Caps lock modifier */
                    rel_scroll = 0.01;
                    break;
                case 4:                     /* Control modifier   */
                    rel_scroll = 0.025;
                    break;
                case 8:                     /* Alt left modifier  */
                case 32:                    /* Alt right modifier */
                    rel_scroll = 1.0;
                    break;
                case 0:                     /* No modifier        */
                default:
                    rel_scroll = 0.1;
                    break;
            }
            switch (ksym) {
                case XK_Left:
                    DoScroll("l", rel_scroll);
                    break;
                case XK_Up:
                    DoScroll("u", rel_scroll);
                    break;
                case XK_Right:
                    DoScroll("r", rel_scroll);
                    break;
                case XK_Down:
                    DoScroll("d", rel_scroll);
                    break;
                case XK_Delete:
                    if (m == SHOW_ALLSPE) delete_spectrum(p.x, p.y, 1);
                    break;
                case XK_BackSpace:
                    if (m == SHOW_ALLSPE) delete_spectrum(p.x, p.y, 0);
                    break;
                case XK_F1:
                    set_show_mode(NULL, "single", NULL);
                    break;
                case XK_F2:
                    set_show_mode(NULL, "map", NULL);
                    break;
                case XK_F3:
                    set_show_mode(NULL, "contour", NULL);
                    break;
                case XK_F4:
                    set_show_mode(NULL, "velpos", NULL);
                    break;
                case XK_F5:
                    set_show_mode(NULL, "scatter", NULL);
                    break;
            }/* End ksym switch */
            break;
    }/* End key_str switch */
}

static void dblclck_handling(int x, int y, double w_x, double w_y)
{
    /* printf("Doubleclick: (%d,%d) or (%d,%d)\n", x, y, vP->min_x, vP->min_y); */
    
    if (abs(x - vP->min_x) < 3 && abs(y - vP->min_y) < 3) {
        /* printf("X and Y axes.\n"); */
    } else if (abs(x - vP->min_x) < 3 &&
               y > vP->min_y - vP->box_h -3 && y < vP->min_y + 3) {
        /* printf("Y axis.\n"); */
    } else if (abs(y - vP->min_y) < 3 &&
               x > vP->min_x - 3 && x < vP->min_x + vP->box_w + 3) {
        /* printf("X axis.\n"); */
    }
}

static void handle_rubber_band(Widget w, char *cmd, XEvent *ev)
{
    int       *iPtr = NULL, dbleclick=0;
    double    *zval = NULL;
    unsigned long delta_t;
    Point      p, p2;
    Gauss2D    g2;
    Display   *dpy = XtDisplay(gp->graph);
    Window     win = XtWindow(gp->graph);
    static int start_x, start_y, last_x, last_y, tagactive=0, njoin=0;
    static int prev_x, prev_y, scatter_tagging=0;
    static int zoom_sel = 0, line_show = 0, swap_sel=0, swap_drawn=0;
    static int dx, dy;
    static Time start_t = 0, last_t = 0;
    static XRectangle r, r_s;
    static double x0, y0;
    static XImage *xi = NULL;
    static MARK *m1, *m2;
    static MAP *M1, *M2;
    static scatter *P1, *P2;
    static PolyLine *pL=NULL;
    
    int GetSpectrumXExtent();
    MAP *GetMapXExtent();
    scatter *GetScatterXExtent();
    void swap_seq_spectra(), swap_maps(), swap_scatters();
    int *QueryAnyToggle();
    MARK *GetMarker();
    void JoinMarkers();
    void UnsetAnyToggle();
    PolyLine *InitPoint(Point);
    PolyLine *AddPoint(Point, PolyLine *);
    void SetAnyToggle(char *, int);
    void Post2DimFitDialog(Widget, Gauss2D *, XtPointer);
    void blank_map_pixel(double, double);

    switch (ev->type) {
        case MotionNotify:      /* The pointer has moved, update its
                                   coordinates */
            draw_tracker_strings(1, ev->xmotion.x, ev->xmotion.y);
            if (zoom_sel == 1 || box_sel == 1 || tagactive == 1 ||
                scatter_tagging == 1) {
                XDrawRectangle(dpy, win, gp->gcErase,
                               start_x, start_y,
                               last_x - start_x, last_y - start_y);
                last_x = ev->xmotion.x;
                last_y = ev->xmotion.y;
                check_points(&start_x, &last_x);
                check_points(&start_y, &last_y);
                XDrawRectangle(dpy, win, gp->gcErase,
                               start_x, start_y,
                               last_x - start_x, last_y - start_y);
            }
            if (gauss_show == 1) {
                draw_gauss(gp->gcErase, gau);
                last_x = ev->xmotion.x;
                last_y = ev->xmotion.y;
                gau.amp = y2yunit(last_y);
                gau.wid = 2.0*fabs(x2dchan(last_x) - gau.cen);
                draw_gauss(gp->gcErase, gau);
            }
            if (swap_sel) {
                if (swap_drawn && xi) {
                    XPutImage(dpy, win, gp->gcErase, xi, 0, 0,
                              r.x, r.y, r.width, r.height);
                }
                last_x = ev->xmotion.x;
                last_y = ev->xmotion.y;
                r.x = r_s.x + last_x - start_x;
                r.y = r_s.y + last_y - start_y;
                swap_drawn = 1;
                if (xi) XPutImage(dpy, win, gp->gcErase, xi, 0, 0,
                                  r.x, r.y, r.width, r.height);
            }
            if (line_show == 1) {
                dx = abs(start_x-last_x); dy = abs(start_y-last_y);
                XDrawArc(dpy, win, gp->gcErase,
                         start_x-dx, start_y-dy, 2*dx, 2*dy, 0, 64*360);
                last_x = ev->xmotion.x;
                last_y = ev->xmotion.y;
                dx = abs(start_x-last_x); dy = abs(start_y-last_y);
                XDrawArc(dpy, win, gp->gcErase,
                         start_x-dx, start_y-dy, 2*dx, 2*dy, 0, 64*360);
            }
            if (pL) {
                XDrawLine(dpy, win, gp->gcErase,
                          start_x, start_y, last_x, last_y);
                last_x = ev->xmotion.x;
                last_y = ev->xmotion.y;
                XDrawLine(dpy, win, gp->gcErase,
                          start_x, start_y, last_x, last_y);
            }
            break;
        case ButtonPress:      /* Button 1 is for zooming (and markers) */
            start_x  = ev->xbutton.x;
            start_y  = ev->xbutton.y;
            last_x   = start_x;
            last_y   = start_y;
            if (ev->xbutton.button == gp->LeftButton) {
                last_t   = start_t;
                start_t  = ev->xbutton.time;
                delta_t = (unsigned long)(start_t - last_t);
                if (delta_t < MILLISEC) dbleclick = 1;
                else dbleclick = 0;
                if (mark_sel) {
                    mark_handling(gp->top, mark_sel,
                                  x2xunit(start_x), y2yunit(start_y),
                                  x2xunit(last_x),  y2yunit(last_y));
                    mark_sel = 0;
                } else if ((iPtr = QueryAnyToggle("tagmarkers")) && *iPtr) {
                    XDrawRectangle(dpy, win, gp->gcErase,
                                   start_x, start_y,
                                   last_x-start_x, last_y-start_y);
                    tagactive = 1;
                } else if ((iPtr = QueryAnyToggle("tagscatterpts")) && *iPtr) {
                    XDrawRectangle(dpy, win, gp->gcErase,
                                   start_x, start_y,
                                   last_x-start_x, last_y-start_y);
                    scatter_tagging = 1;
                } else if ((iPtr = QueryAnyToggle("joinmarkers")) && *iPtr) {
                    UnsetAnyToggle("removejoint", 0);
                    if (njoin == 0) {
                        m1 = GetMarker(x2xunit(start_x), y2yunit(start_y));
                        njoin++;
                    } else if (njoin == 1) {
                        if (m1) {
                            m2 = GetMarker(x2xunit(start_x), y2yunit(start_y));
                            JoinMarkers(m1, m2);
                        }
                        njoin = 0;
                        UnsetAnyToggle("joinmarkers", 1);
                    }
                } else if ((iPtr = QueryAnyToggle("removejoint")) && *iPtr) {
                    UnsetAnyToggle("joinmarkers", 0);
                    m1 = GetMarker(x2xunit(start_x), y2yunit(start_y));
                    if (m1 && (m2 = m1->mark)) {
                        m1->mark = NULL;
                        UnsetAnyToggle("removejoint", 1);
                    } else
                        UnsetAnyToggle("removejoint", 0);
                } else if (pL) {
                    pL = NULL;
                    SetAnyToggle("boxes", 1);
                } else if (dbleclick) {
                    dblclck_handling(start_x, start_y, x2xunit(start_x),
                                     y2yunit(start_y));
                } else {
                    zoom_sel = 1;
                    XDrawRectangle(dpy, win, gp->gcErase,
                                   start_x, start_y,
                                   last_x-start_x, last_y-start_y);
                }
		if (pL) pL = NULL;
            } else if (ev->xbutton.button == gp->MiddleButton &&
                       vP->mode == SHOW_SPE) {
                if (rbox_sel == 1 || rbox_sel == 2) {
                    remove_box(rbox_sel, x2chan(start_x), 1);
                    rbox_sel = 0;
                } else if (rbox_sel == 3 || rbox_sel == 4) {
                    EditBox(gp->graph, rbox_sel, x2chan(start_x));
                    rbox_sel = 0;
                } else if (mod_sel == 1) {
                    new_mod(x2chan(start_x), y2yunit(start_y));
                    mod_sel = 0;
                } else {
                    box_sel = 1;
                    XDrawRectangle(dpy, win, gp->gcErase,
                                   start_x, start_y,
                                   last_x-start_x, last_y-start_y);
                }
            } else if (ev->xbutton.button == gp->MiddleButton &&
                       vP->mode == SHOW_ALLSPE) {
                swap_drawn = 0;
                x0 = x2xunit(start_x);
                y0 = y2yunit(start_y);
                if (xi) XDestroyImage(xi);
                xi = NULL;
                if (GetSpectrumXExtent(x0, y0, &r)) {
                    swap_drawn = 1;
                    r_s = r;
                    xi = XGetImage(dpy, win, r.x, r.y, r.width, r.height,
                                   AllPlanes, ZPixmap);
                    XPutImage(dpy, win, gp->gcErase, xi, 0, 0,
                              r.x, r.y, r.width, r.height);
                }
                swap_sel = 1;
            } else if (ev->xbutton.button == gp->MiddleButton &&
                       (vP->mode == SHOW_POSPOS || vP->mode == SHOW_VELPOS ||
		        vP->mode == SHOW_POSVEL) && vP->nMaps > 1) {
                swap_drawn = 0;
                M1 = M2 = NULL;
                if (xi) XDestroyImage(xi);
                xi = NULL;
                if ((M1=GetMapXExtent(start_x, start_y, &r))) {
                    swap_drawn = 1;
                    r_s = r;
                    xi = XGetImage(dpy, win, r.x, r.y, r.width, r.height,
                                   AllPlanes, ZPixmap);
                    XPutImage(dpy, win, gp->gcErase, xi, 0, 0,
                              r.x, r.y, r.width, r.height);
                }
                swap_sel = 1;
            } else if (ev->xbutton.button == gp->MiddleButton &&
                       vP->mode == SHOW_SCATTER && vP->nScat > 1) {
                swap_drawn = 0;
                P1 = P2 = NULL;
                if (xi) XDestroyImage(xi);
                xi = NULL;
                if ((P1=GetScatterXExtent(start_x, start_y, &r))) {
                    swap_drawn = 1;
                    r_s = r;
                    xi = XGetImage(dpy, win, r.x, r.y, r.width, r.height,
                                   AllPlanes, ZPixmap);
                    XPutImage(dpy, win, gp->gcErase, xi, 0, 0,
                              r.x, r.y, r.width, r.height);
                }
                swap_sel = 1;
            } else if (ev->xbutton.button == gp->MiddleButton &&
                       (vP->mode == SHOW_POSPOS || vP->mode == SHOW_VELPOS ||
		        vP->mode == SHOW_POSVEL)) {
                x0 = x2xunit(start_x);
                y0 = y2yunit(start_y);
		if (mod_sel == 1) {
		    blank_map_pixel(x0, y0);
                    mod_sel = 0;
		} else {
                    p.x = x0;
                    p.y = y0;
                    if (!pL) { /* New polyline */
                	pL = InitPoint(p);
                    } else {
                	XDrawLine(dpy, win, gp->gcErase,
                        	  start_x, start_y, prev_x, prev_y);
                	pL = AddPoint(p, pL);
                	if (pL) XDrawLine(dpy, win, gp->gcMom,
                                	  start_x, start_y, prev_x, prev_y);
                    }
                    prev_x = start_x;
                    prev_y = start_y;
		}
            } else if (ev->xbutton.button == gp->RightButton) {
                if (vP->mode == SHOW_SPE) {
                    if (rgauss_sel == 1) {
                        remove_gauss(x2chan(start_x), y2yunit(start_y));
                        rgauss_sel = 0;
                    } else {
                        gau.cen    = x2dchan(start_x);
                        gau.amp    = y2yunit(start_y);
                        gau.wid    = 0.0;
                        gauss_show = 1;
                        draw_gauss(gp->gcErase, gau);
                    }
                } else if (vP->mode == SHOW_ALLSPE) {
                    swap_map_spectra(x2xunit(start_x), y2yunit(start_y));
                } else if (vP->mode == SHOW_POSPOS) {
                    line_show = 1;
                    dx = abs(start_x-last_x); dy = abs(start_y-last_y);
                    XDrawArc(dpy, win, gp->gcErase,
                             start_x-dx, start_y-dy, 2*dx, 2*dy, 0, 64*360);
                }
            }
            break;
        case ButtonRelease:
            if (ev->xbutton.button == gp->LeftButton) {
                delta_t = (unsigned long)(ev->xbutton.time - start_t);
                if (zoom_sel == 1 || tagactive == 1 || scatter_tagging == 1)
                    XDrawRectangle(dpy, win, gp->gcErase,
                                   start_x, start_y,
                                   last_x-start_x, last_y-start_y);
                if (zoom_sel == 1) {
                    zoom_sel = 0;
                    if (start_x == last_x || start_y == last_y ||
                        delta_t < MILLISEC) break;
                    SetWindowXCoord(start_x, start_y, last_x, last_y);
                    draw_main();
                } else if (tagactive == 1) {
                    mark_handling(gp->top, 4,
                                  x2xunit(start_x), y2yunit(start_y),
                                  x2xunit(last_x),  y2yunit(last_y));
                    tagactive = 0;
                } else if (scatter_tagging == 1) {
                    p.x  = x2xunit(start_x); p.y  = y2yunit(start_y);
                    p2.x = x2xunit(last_x);  p2.y = y2yunit(last_y);
                    scatter_pnts_handling(gp->top, "tag_inside", p, p2);
                    scatter_tagging = 0;
                }
            } else if (ev->xbutton.button == gp->MiddleButton &&
                       box_sel == 1 && vP->mode == SHOW_SPE) {
                XDrawRectangle(dpy, win, gp->gcErase,
                           start_x, start_y, last_x-start_x, last_y-start_y);
                box_sel = 0;
                box.begin = x2chan(start_x);
                box.end   = x2chan(last_x);
                draw_box(gp->gcBox, box);
            } else if (ev->xbutton.button == gp->MiddleButton &&
                       swap_sel == 1 && vP->mode == SHOW_ALLSPE) {
                if (swap_drawn && xi)
                    XPutImage(dpy, win, gp->gcErase, xi, 0, 0,
                              r.x, r.y, r.width, r.height);
                if (GetSpectrumXExtent(x2xunit(r.x+r.width/2),
                                       y2yunit(r.y+r.height/2),
                                       &r))
                    swap_seq_spectra(x0, y0,
                                     x2xunit(r.x+r.width/2),
                                     y2yunit(r.y+r.height/2));
                swap_sel = 0;
                swap_drawn = 0;
            } else if (ev->xbutton.button == gp->MiddleButton && swap_sel == 1 &&
                       (vP->mode == SHOW_POSPOS || vP->mode == SHOW_VELPOS ||
		        vP->mode == SHOW_VELPOS) &&
                       vP->nMaps > 1) {
                if (swap_drawn && xi)
                    XPutImage(dpy, win, gp->gcErase, xi, 0, 0,
                              r.x, r.y, r.width, r.height);
                if ((M2 = GetMapXExtent(r.x+r.width/2, r.y+r.height/2, NULL)))
                    swap_maps(M1, M2);
                swap_sel = 0;
                swap_drawn = 0;
            } else if (ev->xbutton.button == gp->MiddleButton &&
                       vP->mode == SHOW_SCATTER && vP->nScat > 1) {
                if (swap_drawn && xi)
                    XPutImage(dpy, win, gp->gcErase, xi, 0, 0,
                              r.x, r.y, r.width, r.height);
                if ((P2 = GetScatterXExtent(r.x+r.width/2, r.y+r.height/2, NULL)))
                    swap_scatters(P1, P2);
                swap_sel = 0;
                swap_drawn = 0;
            } else if (ev->xbutton.button == gp->RightButton &&
                       gauss_show == 1 && vP->mode == SHOW_SPE) {
                gauss_show = 0;
                draw_gauss(gp->gcGauss, gau);
            } else if (ev->xbutton.button == gp->RightButton &&
                       vP->mode == SHOW_POSPOS) {
                line_show = 0;
                dx = abs(start_x-last_x); dy = abs(start_y-last_y);
                XDrawArc(dpy, win, gp->gcErase,
                         start_x-dx, start_y-dy, 2*dx, 2*dy, 0, 64*360);
                g2.x = x2xunit(start_x); g2.y = y2yunit(start_y);
                p.x = g2.x; p.y = g2.y;
                zval = GetMapValue(vP->m, &p);
                if (zval)
                    g2.A = *zval;
                else
                    g2.A = 1.0;
                g2.maj = fabs(x2xunit(start_x-dx) - x2xunit(start_x+dx));
                g2.min = fabs(y2yunit(start_y-dy) - y2yunit(start_y+dy));
                g2.PA = 0.0;
                Post2DimFitDialog(gp->graph, &g2, NULL);
            }
            break;
    }
}

int MyLoop(int check_pending)
{
    XEvent ev;
    
    if (check_pending && !XtAppPending(gp->app_cntxt)) return 0;
    
    XtAppNextEvent(gp->app_cntxt, &ev);
    
    if (ev.type == PropertyNotify && gp->server) {
        if (ev.xproperty.window == DefaultRootWindow(XtDisplay(gp->top))) {
            handle_property_change(gp->top, NULL, &ev);
        }
    }
    
    XtDispatchEvent(&ev);
        
    return 1;
}

static void draw_tracker_strings(int flag, int x, int y)
{
    int begin, end, chan;
    double xval, xval2, yval, *z, v1, v2, tmp;
    double x1, y1, x2, y2;
    Point p;
    
    char *GetXTrackerFormat(double, char *);
    char *GetDualXTrackerFormat(double, double, char *);
    char *GetYTrackerFormat(double, char *);
    char *GetZTrackerFormat(double, char *);

    if (flag == 0 || (!vP->s && !vP->m && !vP->p)) {
        wprintf(gp->TCursor[0], "");
        wprintf(gp->TCursor[1], "");
        wprintf(gp->TCursor[2], "");
        wprintf(gp->TCursor[3], "");
        return;
    }

    xval = x2xunit(x);
    yval = y2yunit(y);
    p.x = xval;
    p.y = yval;

    if (vP->mode == SHOW_SPE) {
        chan = x2chan(x);
        wprintf(gp->TCursor[0], c_form, chan);
        if (vP->xunit == UNIT_FRE) {
	    if (vP->s) {
	        xval2=2.*(vP->s->lofreq) - xval;
	    } else {
	        xval2=0.;
	    }
            wprintf(gp->TCursor[1], GetDualXTrackerFormat(xval, xval2, f_form2),
	            xval, xval2);
        } else if (vP->xunit == UNIT_FMHZ) {
	    if (vP->s) {
	        xval2=2000.*(vP->s->lofreq) - xval;
	    } else {
	        xval2=0.;
	    }
            wprintf(gp->TCursor[1], GetDualXTrackerFormat(xval, xval2, f_form2),
	            xval, xval2);
        } else if (vP->xunit == UNIT_FOFF) {
            wprintf(gp->TCursor[1], GetXTrackerFormat(xval, fo_form), xval);
        } else if (vP->xunit == UNIT_VEL) {
            wprintf(gp->TCursor[1], GetXTrackerFormat(xval, v_form), xval);
        } else {
            wprintf(gp->TCursor[1], "");
	}
        wprintf(gp->TCursor[2], GetYTrackerFormat(yval, t_form), yval);
        if ((z = chan2s(chan)))
            wprintf(gp->TCursor[3], GetYTrackerFormat(*z, s_form), *z);
        else
            wprintf(gp->TCursor[3], "");
    } else if (vP->mode == SHOW_POSPOS || vP->mode == SHOW_ALLSPE) {
        wprintf(gp->TCursor[0], GetXTrackerFormat(xval, RA_form), xval);
        wprintf(gp->TCursor[1], GetYTrackerFormat(yval, Dec_form), yval);
        if (vP->mode == SHOW_POSPOS && (z = GetMapValue(vP->m, &p)))
            wprintf(gp->TCursor[2], GetZTrackerFormat(*z, z_form), *z);
        else if (vP->mode == SHOW_ALLSPE && (z = xy_to_z(x, y)))
            wprintf(gp->TCursor[2], GetZTrackerFormat(*z, z_form), *z);
        else
            wprintf(gp->TCursor[2], "");
        if (!GetBox(BOX_MOM, 0, &begin, &end)) {
            v1 = SpecUnitConv(UNIT_VEL, UNIT_CHA, (double)begin);
            v2 = SpecUnitConv(UNIT_VEL, UNIT_CHA, (double)end);
            if (v1 > v2) {tmp = v1; v1 = v2; v2 = tmp;}
            wprintf(gp->TCursor[3], v2_form, v1, v2);
        } else
            wprintf(gp->TCursor[3], "");
    } else if (vP->mode == SHOW_VELPOS) {
        if (vP->xunit == UNIT_CHA)
            wprintf(gp->TCursor[0], c_form, NINT(xval));
        else if (vP->xunit == UNIT_FRE)
            wprintf(gp->TCursor[0], GetXTrackerFormat(xval, f_form), xval);
        else if (vP->xunit == UNIT_FMHZ)
            wprintf(gp->TCursor[0], GetXTrackerFormat(xval, f_form), xval);
        else if (vP->xunit == UNIT_FOFF)
            wprintf(gp->TCursor[0], GetXTrackerFormat(xval, fo_form), xval);
        else if (vP->xunit == UNIT_VEL)
            wprintf(gp->TCursor[0], GetXTrackerFormat(xval, v_form), xval);
        else
            wprintf(gp->TCursor[0], "");
        wprintf(gp->TCursor[1], GetXTrackerFormat(yval, p_form), yval);
        GetVelPos(&x1, &y1, &x2, &y2);
        wprintf(gp->TCursor[2], p1_form, x1, y1);
        wprintf(gp->TCursor[3], p2_form, x2, y2);
    } else if (vP->mode == SHOW_POSVEL) {
        wprintf(gp->TCursor[0], GetXTrackerFormat(xval, p_form), xval);
        if (vP->yunit == UNIT_CHA)
            wprintf(gp->TCursor[1], c_form, NINT(yval));
        else if (vP->yunit == UNIT_FRE)
            wprintf(gp->TCursor[1], GetYTrackerFormat(yval, f_form), yval);
        else if (vP->yunit == UNIT_FMHZ)
            wprintf(gp->TCursor[1], GetYTrackerFormat(yval, f_form), yval);
        else if (vP->yunit == UNIT_FOFF)
            wprintf(gp->TCursor[1], GetYTrackerFormat(yval, fo_form), yval);
        else if (vP->yunit == UNIT_VEL)
            wprintf(gp->TCursor[1], GetYTrackerFormat(yval, v_form), yval);
        else
            wprintf(gp->TCursor[1], "");
        GetVelPos(&x1, &y1, &x2, &y2);
        wprintf(gp->TCursor[2], p1_form, x1, y1);
        wprintf(gp->TCursor[3], p2_form, x2, y2);
    } else if (vP->mode == SHOW_SCATTER) {
        wprintf(gp->TCursor[0], GetXTrackerFormat(xval, p_form), xval);
        wprintf(gp->TCursor[1], GetYTrackerFormat(yval, z_form), yval);
    } else {
        draw_tracker_strings(0, 0, 0);
    }
}

static void CreateHeaderInfo(Widget parent)
{
    int n;
    Widget fr, rc;
    
    fr = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, parent,
                                 XmNshadowType, XmSHADOW_ETCHED_IN, NULL);
#if XmVersion <= 1100
    rc = XtVaCreateManagedWidget("form", xmRowColumnWidgetClass, fr,
                                 XmNmarginHeight, 0, NULL);
    XtVaCreateManagedWidget(" Current data information ",
                            xmLabelWidgetClass, rc,
                            /* XmNrecomputeSize, False, */ NULL);
#else
    XtVaCreateManagedWidget(" Current data information ",
                            xmLabelWidgetClass, fr,
                            /* XmNrecomputeSize, False, */
#if XmVersion >= 2000                                  
                            XmNframeChildType, XmFRAME_TITLE_CHILD,
#else
                            XmNchildType, XmFRAME_TITLE_CHILD,
#endif
                            NULL);
    rc = XtVaCreateManagedWidget("form", xmRowColumnWidgetClass, fr,
                                 XmNmarginHeight, 0, NULL);
#endif /* XmVersion <= 1100 */
    gp->hw = (Widget *) XtMalloc(nHeaders * sizeof(Widget));
    for (n=0; n<nHeaders; n++) {
        if (strcmp(HeaderDesc[n], "#Separator") == 0)
            gp->hw[n] = XtVaCreateManagedWidget("sep",
                                  xmSeparatorWidgetClass, rc,
                                  /* XmNrecomputeSize, False, */
                                  XmNseparatorType, XmSINGLE_DASHED_LINE,                                                XmNfontList, gp->flist10,
                                  NULL);
        else
            gp->hw[n] = XtVaCreateManagedWidget(HeaderDesc[n],
                                  xmLabelWidgetClass, rc,
                                  /* XmNrecomputeSize, False, */
                                  XmNfontList, gp->flist10,
                                  NULL);
    }
}

static void CreateTrackers(Widget parent)
{
    int n, i;
    Widget fr, rc;
    
    i = 0;
    while (TBlock[i].n) {
        fr = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, parent,
                                     XmNshadowType, XmSHADOW_ETCHED_IN, NULL);
#if XmVersion <= 1100
        rc = XtVaCreateManagedWidget("form", xmRowColumnWidgetClass, fr,
                                     XmNmarginHeight, 0, NULL);
        if (TBlock[i].title)
            XtVaCreateManagedWidget(TBlock[i].title,
                                  xmLabelWidgetClass, rc,
                                  XmNrecomputeSize, False, NULL);
#else
        if (TBlock[i].title) {
            XtVaCreateManagedWidget(TBlock[i].title,
                                  xmLabelWidgetClass, fr,
                                  XmNrecomputeSize, False,
#if XmVersion >= 2000                                  
                                  XmNframeChildType, XmFRAME_TITLE_CHILD,
#else
                                  XmNchildType, XmFRAME_TITLE_CHILD,
#endif
                                  NULL);
        }
        rc = XtVaCreateManagedWidget("form", xmRowColumnWidgetClass, fr,
                                     XmNmarginHeight, 0, NULL);
#endif
        TBlock[i].w = (Widget *) XtMalloc(TBlock[i].n * sizeof(Widget));
        for (n=0; n<TBlock[i].n; n++) {
            TBlock[i].w[n] = XtVaCreateManagedWidget(" ",
                                           xmLabelWidgetClass, rc,
                                           XmNrecomputeSize, False,
                                           XmNfontList, gp->flist10,
                                           NULL);
        }
        i++;
    }
    gp->TCursor   = TBlock[0].w;
    gp->TGauss    = TBlock[1].w;
    gp->TBaseline = TBlock[2].w;
    gp->TMoment   = TBlock[3].w;
    gp->TPolyline = TBlock[4].w;
}

void UpdateHeaderInfo()
{
    int n, vm = vP->mode, funit=0;
    int showFandV = 0, showRes=0;
    DATE d;
    char *s = NULL, *m = NULL, *pol=NULL, *ra = NULL, *dec = NULL, *epo = NULL;
    char *lon = NULL, *lat = NULL;
    double v=0.0, f=0.0, dv=0.0, df=0.0, xo=0.0, yo=0.0;
    double MegaByte;
    COLOR *c;
    static Pixel background=0;

    COLOR *GetColorInfo();
    double xmap(scanPtr), ymap(scanPtr);
    double GetScanMemory(), GetMapMemory(), GetScatterMemory();
    double GetCurrentMapMemory(), GetVelPosMapMemory();
    char *GetPolStr(int);
   
    d.Sec = d.Min = d.Hour = d.Day = d.Month = d.Year = 0;
    if (vP->m && (vm == SHOW_POSPOS || vm == SHOW_VELPOS ||
                   vm == SHOW_POSVEL || vm == SHOW_SUBMAP)) {
        s   = vP->m->name;
        m   = vP->m->molecule;
        if (vP->m->coordType == COORD_TYPE_GAL) {
            lon = GetLongStr(vP->m->x0);
            lat = GetLatStr(vP->m->y0);
        } else {
            ra  = GetRAStr(vP->m->x0);
            dec = GetDECStr(vP->m->y0);
        }
        epo = GetEpochStr(vP->m->epoch, vP->m->equinox);
        f   = vP->m->fMHz;
        v   = vP->m->v;
        d   = vP->m->date;
        showFandV = 1;
        showRes = 0;
    } else if (vP->p && vm == SHOW_SCATTER) {
        s   = vP->p->name;
        m   = vP->p->molecule;
        /* if (vP->p->coordType == COORD_TYPE_GAL) {
            lon = GetLongStr(vP->p->x0);
            lat = GetLatStr(vP->p->y0);
        } else { */
            ra  = GetRAStr(vP->p->x0);
            dec = GetDECStr(vP->p->y0);
        /* } */
        epo = GetEpochStr(vP->p->epoch, vP->p->equinox);
        d   = vP->p->date;
        showFandV = 0;
        showRes = 0;
    } else if (vP->s && vP->from) {
        s   = vP->s->name;
        m   = vP->s->molecule;
        if (vP->s->coordType == COORD_TYPE_GAL) {
            lon = GetLongStr(vP->s->x0);
            lat = GetLatStr(vP->s->y0);
        } else {
            ra = GetRAStr(vP->s->x0);
            dec = GetDECStr(vP->s->y0);
        }
	pol = GetPolStr(vP->s->polarization);
        xo  = vP->from->sequence ? vP->s->tx : xmap(vP->s);
        yo  = vP->from->sequence ? vP->s->ty : ymap(vP->s);
        epo = GetEpochStr(vP->s->epoch, vP->s->equinox);
        d   = vP->s->date;
        n   = vP->s->nChan/2;
        f   = 1000.0*(vP->s->freq0 + (double)n * vP->s->freqres);
        v   = vP->s->vlsr;
        df  = 1.0e6*fabs(vP->s->freqres);
        dv  = fabs(vP->s->velres);
        showFandV = 1;
        showRes = 1;
        if (df >= 1000.0) {
            funit = 1;
            df /= 1000.0;
        }
    }
    
    n = 0;
    if (gp->hw[n] && vP->from)
        wprintf(gp->hw[n], "%s %s", HeaderDesc[n], vP->from->name);
    
    n += 2;   /* Separator */

    if (gp->hw[n] && s)
        wprintf(gp->hw[n], "%s %s", HeaderDesc[n], s);
    n++;
    if (gp->hw[n] && s)
        wprintf(gp->hw[n], "%s %4d%02d%02d %dh%02dm%02ds", HeaderDesc[n],
                d.Year, d.Month, d.Day, d.Hour, d.Min, d.Sec);
    n++;
    if (gp->hw[n]) {
        if (ra)
            wprintf(gp->hw[n], "%s %s (%s)", HeaderDesc[n], ra, epo);
        else if (lon)
            wprintf(gp->hw[n], "Gal. long.: %s", lon);
    }
    n++;
    if (gp->hw[n]) {
        if (dec)
            wprintf(gp->hw[n], "%s %s (%s)", HeaderDesc[n], dec, epo);
        else if (lat)
            wprintf(gp->hw[n], "Gal. lat.: %s",lat);
    }
    n++;
    if (gp->hw[n])
        wprintf(gp->hw[n], "%s (%+6.1f\",%+6.1f\")", HeaderDesc[n], xo, yo);
    n++;
    if (gp->hw[n])
        wprintf(gp->hw[n], "%s %d", HeaderDesc[n], count_scans(vP->from));
    n++;
    if (gp->hw[n] && m)
        wprintf(gp->hw[n], "%s %s", HeaderDesc[n], m);
    n++;
    if (showFandV) {
        if (gp->hw[n])
            wprintf(gp->hw[n], "%s %7.2f km/s", HeaderDesc[n], v);
        n++;
        if (gp->hw[n]) {
	    if (pol)
                wprintf(gp->hw[n], "%s %9.1f MHz %s", HeaderDesc[n], f, pol);
	    else
                wprintf(gp->hw[n], "%s %9.1f MHz", HeaderDesc[n], f);
	}
    } else {
        if (gp->hw[n])
            wprintf(gp->hw[n], "%s", HeaderDesc[n]);
        n++;
        if (gp->hw[n])
            wprintf(gp->hw[n], "%s", HeaderDesc[n]);
    }
    n++;
    if (showRes) {
        if (gp->hw[n]) {
            if (funit) {
                if (dv >= 10.0)
                    wprintf(gp->hw[n], "%s %5.2f km/s %5.3f MHz",
                            HeaderDesc[n], dv, df);
                else if (dv < 0.1)
                    wprintf(gp->hw[n], "%s %5.2f m/s %5.3f MHz",
                            HeaderDesc[n], 1000.0*dv, df);
                else
                    wprintf(gp->hw[n], "%s %5.3f km/s %5.3f MHz",
                            HeaderDesc[n], dv, df);
            } else {
                if (dv >= 10.0)
                    wprintf(gp->hw[n], "%s %5.2f km/s %5.1f kHz",
                            HeaderDesc[n], dv, df);
                else if (dv < 0.1)
                    wprintf(gp->hw[n], "%s %5.2f m/s %5.1f kHz",
                            HeaderDesc[n], 1000.0*dv, df);
                else
                    wprintf(gp->hw[n], "%s %5.3f km/s %5.1f kHz",
                            HeaderDesc[n], dv, df);
            }
        }
    } else {
        if (gp->hw[n])
            wprintf(gp->hw[n], "%s", HeaderDesc[n]);
    }
    n += 2;   /* Separator */
    if (gp->hw[n]) {
        MegaByte = GetScanMemory() + GetMapMemory() + GetScatterMemory() +
                   GetCurrentMapMemory() + GetVelPosMapMemory();
        if (gp->MemoryWarningLimit > 0.0 && MegaByte > gp->MemoryWarningLimit) {
            if (!background) {
                XtVaGetValues(gp->hw[n], XmNbackground, &background, NULL);
            }
            c = GetColorInfo();
            XtVaSetValues(gp->hw[n], XmNbackground, c->cols[0], NULL);
        } else if (background) {
            XtVaSetValues(gp->hw[n], XmNbackground, background, NULL);
        }
        wprintf(gp->hw[n], "%s %.3f Mb", HeaderDesc[n], MegaByte);
    }
}

static void CreateWindows()
{
    Window         window;
    int            xoff, yoff, scr_nr;
    unsigned int   xwidth, yheight, border_width, depth;
    Status         stgeom;
    Widget         menubar, form, tr_scroll, pane=NULL;
    COLOR         *c;
    static Pixmap  pm = 0;
    
    COLOR *GetColorInfo();

    scr_nr = DefaultScreen(XtDisplay(gp->top));
  
    CreateColors(XtDisplay(gp->top), scr_nr);

    form        = XtCreateWidget("topform", xmFormWidgetClass,
                                 gp->top, NULL, 0);
    gp->form = form;
    
    menubar     = CreateMenuBar(form, MenuBarData);

#if XmVersion >= 2000
    pane = XtVaCreateManagedWidget("pane", xmPanedWindowWidgetClass, form,
                                   XmNorientation, XmHORIZONTAL,
                                   XmNtraversalOn, False,
                                   NULL);
#endif
  
    tr_scroll = XtVaCreateManagedWidget("scroller", xmScrolledWindowWidgetClass,
                                        (pane) ? pane : form,
                                        XmNscrollingPolicy, XmAUTOMATIC,
                                        XmNwidth, (pane)? 207 : 160,
                                        XmNtraversalOn, False,
                                        NULL);
    
    gp->cmd   = XtVaCreateManagedWidget("command", xmRowColumnWidgetClass,
                                        tr_scroll,
                                        NULL);
                                      
    c = GetColorInfo();
    
    if (pm == 0)
        pm = XmGetPixmap(XtScreen(gp->top), pP->xs_xpm, c->black, c->white);
    XtVaSetValues(gp->top, XmNiconPixmap, pm, NULL);
    
    gp->graph = XtVaCreateManagedWidget("graph", xmDrawingAreaWidgetClass,
                                        (pane) ? pane : form,
                                        XmNbackground, c->white,
                                        NULL);

    XtVaSetValues(form,
                  XmNwidth,  (Dimension)atoi(pP->xsWidth),
                  XmNheight, (Dimension)atoi(pP->xsHeight),
                  NULL);
    XtVaSetValues(menubar,
                  XmNtopAttachment,     XmATTACH_FORM,
                  XmNleftAttachment,    XmATTACH_FORM,
                  XmNrightAttachment,   XmATTACH_FORM,
                  NULL);    

    if (pane) {
        XtVaSetValues(pane,
                      XmNtopAttachment,     XmATTACH_WIDGET,
                      XmNtopWidget,         menubar,
                      XmNrightAttachment,   XmATTACH_FORM,
                      XmNbottomAttachment,  XmATTACH_FORM,
                      XmNleftAttachment,    XmATTACH_FORM,
                      NULL);
    } else {
        XtVaSetValues(tr_scroll,
                      XmNtopAttachment,     XmATTACH_WIDGET,
                      XmNtopWidget,         menubar,
                      XmNbottomAttachment,  XmATTACH_FORM,
                      XmNleftAttachment,    XmATTACH_FORM,
                      NULL);
        XtVaSetValues(gp->graph,
                      XmNtopAttachment,     XmATTACH_WIDGET,
                      XmNtopWidget,         menubar,
                      XmNleftAttachment,    XmATTACH_WIDGET,
                      XmNleftWidget,        tr_scroll,
                      XmNbottomAttachment,  XmATTACH_FORM,
                      XmNrightAttachment,   XmATTACH_FORM,
                      NULL);
    }

    XtManageChild(form);

    CreateGCs(XtDisplay(gp->graph), scr_nr);

    CreateFonts();
   
    CreateHeaderInfo(gp->cmd);

#ifdef GAUSSBUTTONS
    XtCreateManagedWidget("sep", xmSeparatorWidgetClass, gp->cmd, NULL, 0);
    CreateButtons(gp->cmd, LeftButtons);
#endif
 
    CreateTrackers(gp->cmd);

    XtAddCallback(gp->graph, XmNexposeCallback, 
                  (XtCallbackProc)handle_graph_exposures, NULL);
    
    XSelectInput(XtDisplay(gp->top), DefaultRootWindow(XtDisplay(gp->top)),
                 PropertyChangeMask);
    /* XtAddEventHandler(gp->top, PropertyChangeMask, False,
                      (XtEventHandler)handle_property_change, NULL); */
    
    XtAddEventHandler(gp->graph, KeyPressMask, False,
                      (XtEventHandler)handle_kb_input, NULL);
    XtAddEventHandler(gp->graph, StructureNotifyMask, False,
                      (XtEventHandler)handle_configure, NULL);
    XtAddEventHandler(gp->graph, LeaveWindowMask, False,
                      (XtEventHandler)handle_leave, NULL);

    XtAddEventHandler(gp->graph,
                      PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
                      False,
                      (XtEventHandler)handle_rubber_band, NULL);

    XtRealizeWidget(gp->top);

    gp->gaussTop = NULL;

    stgeom  = XGetGeometry(XtDisplay(gp->graph), XtWindow(gp->graph),
                           &window,
                           &xoff, &yoff, &xwidth,
                           &yheight, &border_width, &depth);
    vP->main_w  = xwidth;
    vP->main_h  = yheight;

    XDefineCursor(XtDisplay(gp->graph), XtWindow(gp->graph),
                  XCreateFontCursor(XtDisplay(gp->graph), XC_crosshair));

    /* Init the pixmap storage */
#ifdef USE_PIXMAP_STORAGE
    gp->pm = XCreatePixmap(XtDisplay(gp->graph),
                           DefaultRootWindow(XtDisplay(gp->graph)),
                           (Dimension)xwidth, (Dimension)yheight,
                           DefaultDepthOfScreen(XtScreen(gp->graph)));
    gp->p_w = (Dimension)xwidth;
    gp->p_h = (Dimension)yheight;
    XFillRectangle(XtDisplay(gp->graph), gp->pm, gp->gcClear,
                   0, 0, gp->p_w, gp->p_h);
#endif

    SetWatchCursor(False);
  
    gp->msgTop = make_msg_viewer(form);
}

static void CreateFonts()
{
    int n;
    Display *dpy = XtDisplay(gp->graph);
    
    for (n=0; n<nFonts; n++) {
        if (!gp->font08)
            if ((gp->font08 = XLoadQueryFont(dpy, fontname08[n])))
                XSetFont(dpy, gp->gcFrame[0], gp->font08->fid);
        if (!gp->font10)
            if ((gp->font10 = XLoadQueryFont(dpy, fontname10[n]))) {
                XSetFont(dpy, gp->gcFrame[1], gp->font10->fid);
                XSetFont(dpy, gp->gcGauss, gp->font10->fid);
                XSetFont(dpy, gp->gcTag, gp->font10->fid);
                XSetFont(dpy, gp->gcLine, gp->font10->fid);
                XSetFont(dpy, gp->gcSec, gp->font10->fid);
            }
        if (!gp->font12)
            if ((gp->font12 = XLoadQueryFont(dpy, fontname12[n])))
                XSetFont(dpy, gp->gcFrame[2], gp->font12->fid);
        if (!gp->font14)
            if ((gp->font14 = XLoadQueryFont(dpy, fontname14[n])))
                XSetFont(dpy, gp->gcFrame[3], gp->font14->fid);
        if (!gp->font18)
            if ((gp->font18 = XLoadQueryFont(dpy, fontname18[n])))
                XSetFont(dpy, gp->gcFrame[4], gp->font18->fid);
        if (!gp->font24)
            if ((gp->font24 = XLoadQueryFont(dpy, fontname24[n])))
                XSetFont(dpy, gp->gcFrame[5], gp->font24->fid);
    }
    
    if (!gp->font08)
        PostWarningDialog(gp->top, "Couldn't load any 08 font.");
    if (!gp->font10)
        PostWarningDialog(gp->top, "Couldn't load any 10 font.");
    if (!gp->font12)
        PostWarningDialog(gp->top, "Couldn't load any 12 font.");
    if (!gp->font14)
        PostWarningDialog(gp->top, "Couldn't load any 14 font.");
    if (!gp->font18)
        PostWarningDialog(gp->top, "Couldn't load any 18 font.");
    if (!gp->font24)
        PostWarningDialog(gp->top, "Couldn't load any 24 font.");
        
    if (gp->font10) gp->flist10 = MKFLIST(gp->font10);
    if (gp->font12) gp->flist12 = MKFLIST(gp->font12);
}

void SetWatchCursor(int on)
{
    static Cursor watch=0, cross_h=0;
    Display *dpy = XtDisplay(gp->graph);
    Window   win = XtWindow(gp->graph);

    if (!watch)
        watch   = XCreateFontCursor(dpy, XC_watch);
    if (!cross_h)
        cross_h = XCreateFontCursor(dpy, XC_crosshair);

    XDefineCursor(dpy, win, on ? watch : cross_h);
    XFlush(dpy);
}

static void check_points(int *x1, int *x2)
{
    int tmp;
    
    if (*x2 < *x1) {
        tmp = *x1;
        *x1 = *x2;
        *x2 = tmp;
    }
}

static void get_scale_str(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    VIEW *sv;

    if (vP->mode == SHOW_POSPOS || vP->mode == SHOW_ALLSPE) {
        if (sf->edit[5]) {
            wdscanf(sf->edit[5], &vP->xleft);
            wdscanf(sf->edit[6], &vP->xright);
            wdscanf(sf->edit[7], &vP->ylower);
            wdscanf(sf->edit[8], &vP->yupper);
            if (vP->mode == SHOW_ALLSPE) {
                if (!vP->fixed_x) {
                    vP->xleft  += vP->xspacing/2.0;
                    vP->xright -= vP->xspacing/2.0;
                }
                if (!vP->fixed_y) {
                    vP->ylower += vP->yspacing/2.0;
                    vP->yupper -= vP->yspacing/2.0;
                }
            }
        }
        sv = GetScanView();
        if (sv) {
            wdscanf(sf->edit[0], &(sv->xleft));
            wdscanf(sf->edit[1], &(sv->xright));
            wdscanf(sf->edit[2], &(sv->ylower));
            wdscanf(sf->edit[3], &(sv->yupper));
        }
    } else {
        wdscanf(sf->edit[0], &vP->xleft);
        wdscanf(sf->edit[1], &vP->xright);
        wdscanf(sf->edit[2], &vP->ylower);
        wdscanf(sf->edit[3], &vP->yupper);
    }
    wdscanf(sf->edit[4], &vP->xref);
    SetWindow(vP->xleft, vP->xright, vP->ylower, vP->yupper);
    draw_main();
}

static void SpeUnitSetup(Widget w, char *str, XtPointer cd)
{
    int n = atoi(str), old_unit;
    VIEW *sv;

    old_unit = vP->xunit;
    if (n != old_unit || n == UNIT_FOFF) {
        if (n == UNIT_FOFF && cd) wdscanf(scale_edit[4], &vP->xref);
        ChangeSpecUnit(n);
        if (cd) {
            if (vP->mode == SHOW_POSPOS || vP->mode == SHOW_ALLSPE) {
                sv = GetScanView();
                if (sv) {
                    wprintf(scale_edit[0], "%f", sv->xleft);
                    wprintf(scale_edit[1], "%f", sv->xright);
                }
            } else {
                wprintf(scale_edit[0], "%f", vP->xleft);
                wprintf(scale_edit[1], "%f", vP->xright);
            }
        }
        change_unit_in_marks(vP->xunit);
        ChangeUnitInGaussar(vP->xunit);
        draw_main();
    }
}

void ToggleSpeUnit()
{
    int n = vP->xunit;
    
    if (n == UNIT_FRE) {
        SpeUnitSetup(NULL, "1", NULL);
    } else if (n == UNIT_VEL) {
        SpeUnitSetup(NULL, "2", NULL);
    } else if (n == UNIT_CHA) {
        SpeUnitSetup(NULL, "0", NULL);
    }
}

void get_scales(Widget wid, char *cmd, XtPointer call_data)
{
    Widget fr, vert, rc, rc2, wx_label, wy_label, wR_label, wD_label;
    int i, n, ncols = 2;
    Arg wargs[10];
    VIEW *sv;
    Widget w = wid, menu;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Change Scales",
             BUTT_APPLY, (XtCallbackProc)get_scale_str, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 9, NULL);
    
    vert = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                   XmNorientation, XmVERTICAL,
                                   NULL);

    fr    = XtVaCreateWidget("frame", xmFrameWidgetClass,
                             vert, XmNshadowType, XmSHADOW_OUT, NULL);

    if (vP->mode == SHOW_ALLSPE || vP->mode == SHOW_POSPOS)
        ncols += 2;

    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                 XmNorientation, XmHORIZONTAL,
                                 XmNnumColumns, ncols,
                                 XmNadjustLast, False,
                                 XmNpacking, XmPACK_COLUMN,
                                 NULL);
    
    wx_label = XtCreateManagedWidget("X-range:", xmLabelWidgetClass,
                                     rc, NULL, 0);
    for (i=0; i<2; i++) {
        sf->edit[i] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                            rc, NULL, 0);
    }

    wy_label = XtCreateManagedWidget("Y-range:", xmLabelWidgetClass,
                                    rc, NULL, 0);
    for (i=2; i<4; i++) {
        sf->edit[i] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                              rc, NULL, 0);
    }

    if (vP->mode == SHOW_ALLSPE || vP->mode == SHOW_POSPOS) {
        wR_label = XtCreateManagedWidget("RA-range:", xmLabelWidgetClass,
                                          rc, NULL, 0);
        for (i=5; i<7; i++) {
            sf->edit[i] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                                rc, NULL, 0);
        }
        wD_label = XtCreateManagedWidget("Dec-range:", xmLabelWidgetClass,
                                          rc, NULL, 0);
        for (i=7; i<9; i++) {
            sf->edit[i] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                                rc, NULL, 0);
        }
    } else {
        for (i=5; i<9; i++) sf->edit[i] = NULL;
    }
    
    menu = CreateOptionMenu(vert, &SpeUnitMenu);
    /* SetDefaultOptionMenuItem(menu, vP->xunit < 4 ? vP->xunit : vP->xunit-2); */
    SetDefaultOptionMenuItemNumString(menu, SpeUnitData, vP->xunit);
        
    n = 0;
    XtSetArg(wargs[n], XmNorientation,       XmHORIZONTAL); n++;
    XtSetArg(wargs[n], XmNnumColumns,        1); n++;
    XtSetArg(wargs[n], XmNadjustLast,        FALSE); n++;
    XtSetArg(wargs[n], XmNpacking,           XmPACK_COLUMN); n++;
    rc2  = XtCreateManagedWidget("rowcol", xmRowColumnWidgetClass,
                                 vert, wargs, n);
    XtCreateManagedWidget("Freq. offset ref.:", xmLabelWidgetClass,
                          rc2, NULL, 0);
    sf->edit[4] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                        rc2, NULL, 0);
    XtCreateManagedWidget("GHz", xmLabelWidgetClass,
                          rc2, NULL, 0);

    ArrangeStdFormDialog(sf, vert);

    XtManageChild(menu);
    XtManageChild(fr);

    if (sf->edit[5]) {
        wprintf(sf->edit[5], "%f", vP->xleft);
        wprintf(sf->edit[6], "%f", vP->xright);
        wprintf(sf->edit[7], "%f", vP->ylower);
        wprintf(sf->edit[8], "%f", vP->yupper);
        sv = GetScanView();
        if (sv) {
            wprintf(sf->edit[0], "%f", sv->xleft);
            wprintf(sf->edit[1], "%f", sv->xright);
            wprintf(sf->edit[2], "%f", sv->ylower);
            wprintf(sf->edit[3], "%f", sv->yupper);
        }
    } else {
        wprintf(sf->edit[0], "%f", vP->xleft);
        wprintf(sf->edit[1], "%f", vP->xright);
        wprintf(sf->edit[2], "%f", vP->ylower);
        wprintf(sf->edit[3], "%f", vP->yupper);
    }
    wprintf(sf->edit[4], "%f", vP->xref);
    
    scale_edit[0] = sf->edit[0];
    scale_edit[1] = sf->edit[1];
    scale_edit[4] = sf->edit[4];

    ManageDialogCenteredOnPointer(sf->form);
}

static void do_scroll(XtPointer sdata, XtIntervalId id)
{
    int shift = (int)sdata;
    int s_ch, e_ch;

    if (vP->mode == SHOW_SPE) {
        s_ch = xunit2chan(vP->xleft)  + shift;
        e_ch = xunit2chan(vP->xright) + shift;
        SetWindow(chan2xunit(s_ch), chan2xunit(e_ch),
                  vP->ylower, vP->yupper);
        draw_main();
    }
    arrow_timer_id = XtAppAddTimeOut(gp->app_cntxt, id==1? 400 : 50,
                                     (XtTimerCallbackProc)do_scroll,
                                     (XtPointer)shift);
}

void freq_scroll(Widget w, char *str, XmArrowButtonCallbackStruct *cbs)
{
    int large=0, small=0, shift=0, vis_ch;
    int begin, end;

    if (cbs->reason == XmCR_DISARM) {
        XtRemoveTimeOut(arrow_timer_id);
        return;
    }

    if (vP->mode == SHOW_SPE) {
        vis_ch = xunit2chan(vP->xright) - xunit2chan(vP->xleft) + 1;
        small = vis_ch/100;
        if (small < 1) small = 1;
        large = 10*small;
    } else if ((vP->mode == SHOW_POSPOS || vP->mode == SHOW_ALLSPE ||
                vP->mode == SHOW_SCATTER) &&
               !GetBox(BOX_MOM, 0, &begin, &end)) {
        small = 1;
        large = end - begin + 1;
    }

    if (strncmp(str, "<", strlen(str)) == 0) {
        shift = -small;
    } else if (strncmp(str, "<<", strlen(str)) == 0) {
        shift = -large;
    } else if (strncmp(str, ">", strlen(str)) == 0) {
        shift = small;
    } else if (strncmp(str, ">>", strlen(str)) == 0) {
        shift = large;
    }
    do_scroll((XtPointer)shift, 1);
}

static void set_top_label(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int i = atoi(str);

    if (i != vP->tlab_type) {
        vP->tlab_type = i;
        draw_main();
    }
}

static void set_sectop_label(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int i = atoi(str);

    if (i != vP->slab_type) {
        vP->slab_type = i;
        draw_main();
    }
}

static void set_lef_label(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int i = atoi(str);

    if (i != vP->llab_type) {
        vP->llab_type = i;
        draw_main();
    }
}

static void set_rig_label(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    int i = atoi(str);

    if (i != vP->rlab_type) {
        vP->rlab_type = i;
        draw_main();
    }
}

static void SetLabelPositions(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int new_pos = 0;
    double lx, ly, rx, ry;
    
    if (!sf) return;
    
    wdscanf(sf->edit[0], &lx);
    wdscanf(sf->edit[1], &ly);
    wdscanf(sf->edit[2], &rx);
    wdscanf(sf->edit[3], &ry);
        
    if (lx != vP->lef_x || ly != vP->lef_y) new_pos = 1;
    vP->lef_x = lx;
    vP->lef_y = ly;
    
    if (rx != vP->lef_x || ry != vP->lef_y) new_pos = 1;
    vP->rig_x = rx;
    vP->rig_y = ry;
    
    if (new_pos)
        redraw_graph(gp->top, "update", NULL);
}

void PostLabelDialog(Widget wid, char *cmd, XtPointer call_data)
{
    Widget rc, tMenu, sMenu, rMenu, lMenu;
    Widget w = wid;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Type of top label",
             BUTT_APPLY, (XtCallbackProc)SetLabelPositions, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 4, NULL);
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 NULL);

    tMenu = CreateOptionMenu(rc, &TopLabelMenu);
    SetDefaultOptionMenuItem(tMenu, vP->tlab_type);

    sMenu = CreateOptionMenu(rc, &SecTopLabelMenu);
    SetDefaultOptionMenuItem(sMenu, vP->slab_type);
    
    XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
                            rc, XmNseparatorType, XmSHADOW_ETCHED_IN, NULL);
    
    lMenu = CreateOptionMenu(rc, &LeftLabelMenu);
    SetDefaultOptionMenuItem(lMenu, vP->llab_type);
    XtCreateManagedWidget("Relative x-position for left label:",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[0] = XtCreateManagedWidget("xpos", xmTextWidgetClass, rc, NULL, 0);
    XtCreateManagedWidget("Relative y-position for left label:",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[1] = XtCreateManagedWidget("ypos", xmTextWidgetClass, rc, NULL, 0);
    
    XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
                            rc, XmNseparatorType, XmSHADOW_ETCHED_IN, NULL);
    
    rMenu = CreateOptionMenu(rc, &RightLabelMenu);
    SetDefaultOptionMenuItem(rMenu, vP->rlab_type);
    XtCreateManagedWidget("Relative x-position for right label:",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[2] = XtCreateManagedWidget("xpos", xmTextWidgetClass, rc, NULL, 0);
    XtCreateManagedWidget("Relative y-position for right label:",
                          xmLabelWidgetClass, rc, NULL, 0);
    sf->edit[3] = XtCreateManagedWidget("ypos", xmTextWidgetClass, rc, NULL, 0);
    
    ArrangeStdFormDialog(sf, rc);
    
    XtManageChild(tMenu);
    XtManageChild(sMenu);
    XtManageChild(lMenu);
    XtManageChild(rMenu);
    
    wprintf(sf->edit[0], "%f", vP->lef_x);
    wprintf(sf->edit[1], "%f", vP->lef_y);
    wprintf(sf->edit[2], "%f", vP->rig_x);
    wprintf(sf->edit[3], "%f", vP->rig_y);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void get_mod_headers(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    int i, vm = vP->mode;
    double *px, *py, eq;
    char ep;
    string  s[nModHeaders], c;
    list curr = NULL;
    scanPtr sptr;
    
    double *RAStr2Rad();
    double *DECStr2Rad();
    double *DegStr2Rad();

    for (i=0; i<nModHeaders; i++)
        wsscanf(sf->edit[i], s[i]);
        
    strcpy(c, s[2]);
    sscanf(s[5], "%1c%lf", &ep, &eq);
    
    if (c[0] == 'E' || c[0] == 'U' || c[0] == 'H') {    
        px = RAStr2Rad(s[3]);
        if (!px) {
            PostErrorDialog(w, "RA string doesn't conform to AAhBBmCCs.");
            return;
        }
        py = DECStr2Rad(s[4]);
        if (!py) {
            PostErrorDialog(w, "Dec string doesn't conform to AAdBB'CC\".");
            return;
        }
    } else {
        px = DegStr2Rad(s[3]);
        if (!px) {
            PostErrorDialog(w, "Long. string doesn't conform to AA.A");
            return;
        }
        py = DegStr2Rad(s[4]);
        if (!py) {
            PostErrorDialog(w, "Lat. string doesn't conform to AA.A");
            return;
        }
    }
    
    if (ep != 'j' && ep != 'J' && ep != 'b' && ep != 'B') {
        ep = ' ';
        eq = 0.0;
    }
    
    if (vP->m && (vm == SHOW_POSPOS || vm == SHOW_VELPOS ||
                   vm == SHOW_POSVEL || vm == SHOW_SUBMAP)) {
        strcpy(vP->m->name, s[0]);
        strcpy(vP->m->molecule, s[1]);
        vP->m->x0 = *px;
        vP->m->y0 = *py;
        vP->m->epoch = ep;
        vP->m->equinox = eq;
    } else if (vP->p && vm == SHOW_SCATTER) {
        strcpy(vP->p->name, s[0]);
        strcpy(vP->p->molecule, s[1]);
        vP->p->x0 = *px;
        vP->p->y0 = *py;
        vP->p->epoch = ep;
        vP->p->equinox = eq;
    } else if (vP->s) {
        if (vm == SHOW_SPE) {
            strcpy(vP->s->name, s[0]);
            strcpy(vP->s->molecule, s[1]);
            vP->s->x0 = *px;
            vP->s->y0 = *py;
            vP->s->epoch = ep;
            vP->s->equinox = eq;
        } else {
            while ( (curr = scan_iterator(curr, vP->from)) != NULL ) {
                sptr = (scanPtr)DATA(curr);
                strcpy(sptr->name, s[0]);
                strcpy(sptr->molecule, s[1]);
                sptr->x0 = *px;
                sptr->y0 = *py;
                sptr->epoch = ep;
                sptr->equinox = eq;
            }
        }
     }
    
    UpdateHeaderInfo();
    draw_main();
}

void PostModifyHeaderDialog(Widget wid, char *cmd, XtPointer cd)
{
    int i, cType = COORD_TYPE_EQU;
    Widget rc, fr;
    Widget w = wid;
    char *s[nModHeaders];
    StdForm *sf;
    
    for (i=0; i<nModHeaders; i++) s[i] = NULL;
    
    if (vP->s) {
        s[0] = vP->s->name;
        s[1] = vP->s->molecule;
        if (vP->s->coordType == COORD_TYPE_GAL) {
            s[3] = GetLongStr(vP->s->x0);
            s[4] = GetLatStr(vP->s->y0);
            cType = COORD_TYPE_GAL;
        } else {
            s[3] = GetRAStr(vP->s->x0);
            s[4] = GetDECStr(vP->s->y0);
        }
        s[2] = GetCoordType(cType);
        s[5] = GetEpochStr(vP->s->epoch, vP->s->equinox);
    } else if (vP->m) {
        s[0] = vP->m->name;
        s[1] = vP->m->molecule;
        if (vP->m->coordType == COORD_TYPE_GAL) {
            s[3] = GetLongStr(vP->m->x0);
            s[4] = GetLatStr(vP->m->y0);
            cType = COORD_TYPE_GAL;
        } else {
            s[3] = GetRAStr(vP->m->x0);
            s[4] = GetDECStr(vP->m->y0);
        }
        s[2] = GetCoordType(cType);
        s[5] = GetEpochStr(vP->m->epoch, vP->m->equinox);
    } else if (vP->p) {
        s[0] = vP->p->name;
        s[1] = vP->p->molecule;
        s[2] = GetCoordType(cType);
        s[3] = GetRAStr(vP->p->x0);
        s[4] = GetDECStr(vP->p->y0);
        s[5] = GetEpochStr(vP->p->epoch, vP->p->equinox);
    }

    while (!XtIsWMShell(w))
        w = XtParent(w);

    sf = PostStdFormDialog(w, "Modify header",
             BUTT_APPLY, (XtCallbackProc)get_mod_headers, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, nModHeaders, NULL);
    
    fr = XtVaCreateWidget("frame", xmFrameWidgetClass, sf->form,
                          XmNshadowType, XmSHADOW_OUT, NULL);
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, fr,
                                 XmNorientation, XmHORIZONTAL,
                                 XmNnumColumns, nModHeaders,
                                 XmNadjustLast, False,
                                 XmNpacking, XmPACK_COLUMN,
                                 NULL);
    for (i=0; i<nModHeaders; i++) {
        XtVaCreateManagedWidget(ModHeaderDesc[i], xmLabelWidgetClass,
                                rc, NULL);
        if (i == 2) {
            sf->edit[i] = XtVaCreateManagedWidget("edit", xmTextWidgetClass,
                                                  rc,
                                                  XmNeditable, False,
                                                  NULL);
        } else {
            sf->edit[i] = XtVaCreateManagedWidget("edit", xmTextWidgetClass,
                                                  rc, NULL);
        }
    }
    
    ArrangeStdFormDialog(sf, fr);
    
    for (i=0; i<nModHeaders; i++)
        wprintf(sf->edit[i], "%s", s[i]);
    
    XtManageChild(fr);
    
    ManageDialogCenteredOnPointer(sf->form);
}

static void DotTypeSetup(Widget w, char *cmd, XtPointer cd)
{
    int map=-1, n = 0;
    int *dtype;
    Widget wid = w;
    
    while (!XtIsWMShell(wid)) {
        if (n == 3) {
            XtVaGetValues(wid, XmNuserData, &map, NULL);
        break;
    }
        wid = XtParent(wid);
    n++;
    }
    
    if (map == -1) {
        return;
    } else if (map) {
        dtype = GetMapDotType();
    } else {
        dtype = GetScatterDotType();
    }
    
    if (*dtype != atoi(cmd)) {
        *dtype = atoi(cmd);
    draw_main();
    }
}

static void DotSizeSetup(Widget w, char *cmd, XtPointer cd)
{    
    int map=-1, n = 0;
    int *dsize;
    Widget wid = w;
    
    while (!XtIsWMShell(wid)) {
        if (n == 3) {
            XtVaGetValues(wid, XmNuserData, &map, NULL);
        break;
    }
        wid = XtParent(wid);
    n++;
    }
    
    if (map == -1) {
        return;
    } else if (map == 1) {
        dsize = GetMapDotSize();
    } else {
        dsize = GetScatterDotSize();
    }
    
    if (*dsize != atoi(cmd)) {
        *dsize = atoi(cmd);
    draw_main();
    }
}

void PostDotDialog(Widget wid, char *cmd, XtPointer cd)
{
    int map = 0;
    int *dtype, *dsize;
    string title;
    Widget rc, menuType, menuSize, w = wid;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    if (strcmp(cmd, "mapdot") == 0) {
        map = 1;
        dsize = GetMapDotSize();
        dtype = GetMapDotType();
        sprintf(title, "Dot markers in maps");
    } else {
        dsize = GetScatterDotSize();
        dtype = GetScatterDotType();
        sprintf(title, "Dot markers in scatter plots");
    }
    sf = PostStdFormDialog(w, title,
             NULL, NULL, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL, 0, NULL);
    
    rc = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                                 XmNorientation, XmVERTICAL,
                                 XmNuserData, map,
                                 NULL);

    menuType = CreateOptionMenu(rc, &DotTypeMenu);
    SetDefaultOptionMenuItem(menuType, *dtype);

    menuSize = CreateOptionMenu(rc, &DotSizeMenu);
    SetDefaultOptionMenuItem(menuSize, *dsize);
    
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(menuType);
    XtManageChild(menuSize);
    
    ManageDialogCenteredOnPointer(sf->form);
}

int DirtySpectra()
{
    list curr=NULL;
    scanPtr s;
    
    if (!vP->from) return 0;
    
    if (!count_scans(vP->from)) return 0;
    
    while ((curr = scan_iterator(curr, vP->from))) {
        s = (scanPtr)DATA(curr);
        if (!s->saved) return 1;
    }
    
    return 0;
}

void do_quit(Widget wid, char *client_data, XtPointer call_data)
{
    Widget w = wid;
    
    int DirtyMaps();
    int DirtyPrefs();

    while (!XtIsWMShell(w))
        w = XtParent(w);

    if (DirtySpectra() && !PostQuestionDialog(w,
"Spectra have not been saved.\n\
Are you sure you want to quit?")) return;

    if (DirtyMaps() && !PostQuestionDialog(w,
"Maps have not been saved.\n\
Are you sure you want to quit?")) return;

    if (DirtyPrefs() && !PostQuestionDialog(w,
"The new preferences have not been saved.\n\
Are you sure you want to quit?")) return;
    
    XtCloseDisplay(XtDisplay(w));

    exit(0);
}

GC GetGC(unsigned long valuemask, XGCValues *values)
{
    return XCreateGC(XtDisplay(gp->graph),
                     DefaultRootWindow(XtDisplay(gp->top)),
                     valuemask, values);
}
