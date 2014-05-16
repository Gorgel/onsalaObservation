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
/* #define FANCY_STARTUP */

#if XtSpecificationRelease <= 4
#define XtSetLanguageProc(x,y,z) (void)((x),(y),(z))
#endif

/*** External variables and functions ***/
void    init_prefs();
void    check_prefs();
void    init_gauss_parameters();
void    init_baseline_parameters();
void    init_file_parameters();
void    init_smooth_parameters();
void    init_redres_parameters();
void    init_clip_parameters();
void    init_scale_parameters();
void    init_mark_parameters();
void    init_maplist();
void    init_scatterlist();
void    init_togglelist();
void    init_fieldlist();
void    init_scanlist();
void    init_datasets();
void    init_polylist();
void    init_draw_parameters();
void    init_convolve();
void    init_memdata();
void    init_map_parameters();
void    init_scatter_data();
void    init_testmap();
void    init_macro();
void    init_PS_parameters();
void    init_view();
void    InitView(int);
void    UpdateData(int, int);
void    DoZoom(char *, double);
void    ResizeView(int, int);
void    draw_main();
void    draw_gauss(GC, Gauss);
void    draw_box(GC, BOX);
void    mark_handling(Widget, int, double, double, double, double);
void    scatter_pnts_handling(Widget, char *, Point, Point);
void    remove_box(int, int, int);
void    EditBox(Widget, int, int);
void    new_mod(int, double);
void    remove_gauss(int, double);
void    swap_map_spectra(double, double);
void    SetWindowXCoord(int, int, int, int);
void    SetWindow(double, double, double, double);
void    PostVelPosDialog(Widget, double, double, double, double, PolyLine *);
void    wprintf(Widget, char *, ...);
void    CreateColors(Display *, int);
void    CreateGCs(Display *, int);
void    SwapForegroundAndBackground();
void    CreateButtons(Widget, ButtonItem *);
void    ChangeSpecUnit(int);
void    change_unit_in_marks(int);
void    ChangeUnitInGaussar(int);
void    wdscanf(Widget, double *);
void    wsscanf(Widget, char *);
int     read_file(char *, char *, DataSetPtr);
int     LoadFITS(char *, char *);
int     x2chan(int);
int     xunit2chan(double);
int     GetBox(int, int, int *, int *);
int     GetVelPos(double *, double *, double *, double *);
int     SetViewMode(int, scan *, MAP *, scatter *);
char   *GetFileType(const char *);
char   *GetRAStr(double);
char   *GetDECStr(double);
char   *GetLongStr(double);
char   *GetLatStr(double);
char   *GetCoordType(int);
char   *GetEpochStr(char, double);
double  y2yunit(int);
double  x2dchan(int);
double  x2xunit(int);
double  chan2xunit(int);
double  SpecUnitConv(int, int, double);
double *chan2s(int);
double *pos_to_z(double, double);
double *xy_to_z(int, int);
double *GetMapValue(MAP *, Point *);
Widget  CreateMenuBar(Widget, MenuBarItem *);
Widget  make_msg_viewer(Widget);
Widget  get_mombox_slider();
VIEW   *GetScanView();
void    PostErrorDialog(Widget, char *);
void    PostWarningDialog(Widget, char *);
void    PostMessageDialog(Widget, char *);
int     PostQuestionDialog(Widget, char *);
void    ManageDialogCenteredOnPointer(Widget);
Widget  CreateOptionMenu(Widget, MenuBarItem *);
void    SetDefaultOptionMenuItem(Widget, int);
void    SetDefaultOptionMenuItemNumString(Widget, MenuItem *, int);
void    ColorAllocationWarning(void);
void    UpdatePolylineInfo(void);

list    scan_iterator(list, DataSetPtr);
int     count_scans(DataSetPtr);
    
int    *GetMapDotSize(), *GetMapDotType();
int    *GetScatterDotSize(), *GetScatterDotType();

extern USER   *pP;

extern char *optarg;
extern int optind, opterr, optopt;

/*** Local variables and functions ***/
int MyLoop(int);
static void draw_tracker_strings(int, int, int);
static void CreateWindows();
static void CreateFonts();
static void CreateHeaderInfo(Widget);
static void check_points(int *, int *);
void UpdateHeaderInfo();
void SetWatchCursor(int);
void get_scales(Widget, char *, XtPointer);
void freq_scroll(Widget, char *, XmArrowButtonCallbackStruct *);
void get_vel_info(Widget, char *, XtPointer);
int  AllocSpectrum(int);
void do_quit(Widget, char *, XtPointer);
GC   GetGC(unsigned long, XGCValues *);

const int nHeaders = 14;
static char *HeaderDesc[] = {
  "Data set:",
  "#Separator",
  "Source:",
  "Date:",
  "RA: ",
  "Dec:",
  "Offset:",
  "No. of spectra:",
  "Molecule:",
  "Vel.:",
  "Freq.:",
  "Res.:",
  "#Separator",
  "Tot. memory:"
};

typedef struct {
    int n;
    char *title;
    Widget *w;
} TrackerBlock;

static TrackerBlock TBlock[] = {
    {4, " Cursor info ", NULL},
    {3, " Gaussian info ", NULL},
    {5, " Baseline region ", NULL},
    {3, " Moment region ", NULL},
    {4, " Polyline info ", NULL},
    {0, NULL, NULL}
};

static void set_top_label(Widget, char *, XmAnyCallbackStruct *);
static void set_sectop_label(Widget, char *, XmAnyCallbackStruct *);
static void set_lef_label(Widget, char *, XmAnyCallbackStruct *);
static void set_rig_label(Widget, char *, XmAnyCallbackStruct *);
static MenuItem TopLabelData[] = {
  {"None", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_top_label, "0", NULL},
  {"Source name", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_top_label, "1", NULL},
  {"Molecule name", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_top_label, "2", NULL},
  {"Date string", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_top_label, "3", NULL},
  {"Time string", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_top_label, "4", NULL},
  {"Position string", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_top_label, "5", NULL},
  {"Integrated intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_top_label, "6", NULL},
  {"Scan number", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_top_label, "7", NULL},
  {"Ref. frequency", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_top_label, "8", NULL},
  {"Fitted gaussian", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_top_label, "9", NULL},
EOI};
static MenuItem SecTopLabelData[] = {
  {"None", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sectop_label, "0", NULL},
  {"Source name", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sectop_label, "1", NULL},
  {"Molecule name", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sectop_label, "2", NULL},
  {"Date string", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sectop_label, "3", NULL},
  {"Time string", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sectop_label, "4", NULL},
  {"Position string", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sectop_label, "5", NULL},
  {"Integrated intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sectop_label, "6", NULL},
  {"Scan number", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sectop_label, "7", NULL},
  {"Ref. frequency", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sectop_label, "8", NULL},
  {"Fitted gaussian", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_sectop_label, "9", NULL},
EOI};
static MenuItem LeftLabelData[] = {
  {"None", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_lef_label, "0", NULL},
  {"Source name", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_lef_label, "1", NULL},
  {"Molecule name", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_lef_label, "2", NULL},
  {"Date string", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_lef_label, "3", NULL},
  {"Time string", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_lef_label, "4", NULL},
  {"Position string", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_lef_label, "5", NULL},
  {"Integrated intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_lef_label, "6", NULL},
  {"Scan number", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_lef_label, "7", NULL},
  {"Ref. frequency", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_lef_label, "8", NULL},
  {"Fitted gausssian", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_lef_label, "9", NULL},
EOI};
static MenuItem RightLabelData[] = {
  {"None", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_rig_label, "0", NULL},
  {"Source name", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_rig_label, "1", NULL},
  {"Molecule name", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_rig_label, "2", NULL},
  {"Date string", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_rig_label, "3", NULL},
  {"Time string", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_rig_label, "4", NULL},
  {"Position string", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_rig_label, "5", NULL},
  {"Integrated intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_rig_label, "6", NULL},
  {"Scan number", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_rig_label, "7", NULL},
  {"Ref. frequency", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_rig_label, "8", NULL},
  {"Fitted Gaussian", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_rig_label, "9", NULL},
EOI};
static MenuBarItem TopLabelMenu = {
   "Type of top label", ' ', True, TopLabelData
};
static MenuBarItem SecTopLabelMenu = {
   "Type of secondary top label", ' ', True, SecTopLabelData
};
static MenuBarItem LeftLabelMenu = {
   "Type of left inside-label", ' ', True, LeftLabelData
};
static MenuBarItem RightLabelMenu = {
   "Type of right inside-label", ' ', True, RightLabelData
};

#define nModHeaders 6
static char *ModHeaderDesc[] = {
  "Source:",
  "Molecule:",
  "Coordinates:",
  "",
  "",
  "Epoch:"
};

static void DotTypeSetup(Widget, char *, XtPointer);
static MenuItem DotTypeData[] = {
   {"None", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotTypeSetup, "0", NULL},
   {"Square", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotTypeSetup, "1", NULL},
   {"Circle", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotTypeSetup, "2", NULL},
   {"Value", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotTypeSetup, "3", NULL},
   {"Cross", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotTypeSetup, "4", NULL},
   {"Plus", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotTypeSetup, "5", NULL},
EOI};
static MenuBarItem DotTypeMenu = {
   "Type of dot marker", ' ', True, DotTypeData
};

static void DotSizeSetup(Widget, char *, XtPointer);
static MenuItem DotSizeData[] = {
   {"0", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotSizeSetup, "0", NULL},
   {"1", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotSizeSetup, "1", NULL},
   {"2", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotSizeSetup, "2", NULL},
   {"3", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotSizeSetup, "3", NULL},
   {"4", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotSizeSetup, "4", NULL},
   {"5", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotSizeSetup, "5", NULL},
   {"6", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotSizeSetup, "6", NULL},
   {"7", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DotSizeSetup, "7", NULL},
EOI};
static MenuBarItem DotSizeMenu = {
   "Size of dot marker", ' ', True, DotSizeData
};

static void SpeUnitSetup(Widget, char *, XtPointer);
static MenuItem SpeUnitData[] = {
   {"Frequency (GHz)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SpeUnitSetup, "0", NULL},
   {"Frequency (MHz)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SpeUnitSetup, "6", NULL},
   {"Velocity", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SpeUnitSetup, "1", NULL},
   {"Channel", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SpeUnitSetup, "2", NULL},
   {"Freq. offset", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SpeUnitSetup, "3", NULL},
EOI};
static MenuBarItem SpeUnitMenu = {
   "Type of spectral x-axis", ' ', True, SpeUnitData
};

static char opt_help[] = "Usage: \
%s [-p] [-f file] [-a file] [-c file] [-h] [-s]\n\
where\n\
    -p            Use private color map\n\
    -f file       Load with file\n\
    -a file       Load with FITS array file\n\
    -c file       Load with FITS cube file\n\
    -h            Print this info\n\
    -v            Print version info\n\
    -s            Allow for server use (experimental)\n";

static char opt_version[] = "%s Version: %3.1f  Patch level: %d   Date: %s\n";

static Widget scale_edit[5];

static XtIntervalId arrow_timer_id;

static GLOBAL globalPars;

static string opt_fname;
static int opt_fitstype;
