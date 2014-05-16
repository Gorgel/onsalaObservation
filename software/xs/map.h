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
#define PLEPS       1.0e-4
#define PLABS(x, y) ((x)-(y) < 0.0 ? (y)-(x) : (x)-(y))
#define PLEQU(x, y) (PLABS((x), (y)) < PLEPS ? 1 : 0)

#define ANYMAP(m)   ((m)==SHOW_POSPOS || (m)==SHOW_VELPOS || (m)==SHOW_POSVEL)

#define ZEROSPACING  0.25
#define HIGHSPACING  1.e15
#define SHADE_SWAP   (0.66667)
#define MAXPAIRS     10000
#define MAXARRAYSIZE 256

#define CONT_LINEAR        0
#define CONT_LOGARITHMIC   1
#define CONT_EXPONENTIAL   2
#define CONT_SQUAREROOT    3
#define CONT_QUADRATIC     4
#define CONT_VARYING       5

#define CONT_MINMAX        0
#define CONT_NULLMAX       1

#define CONT_LABLIMIT      0

/* #define DEBUG */

/*** External variables ***/
extern char v2_form[];

extern VIEW   *vP;
extern DRAW    draw;
extern int     nreg, pgplot;
extern BOX     regs[MAXBOX];
extern PSDATA  ps;
extern GLOBAL *gp;
extern scatter scat;
extern USER   *pP;

void PostErrorDialog(Widget, char *);
void PostWarningDialog(Widget, char *);
void PostMessageDialog(Widget, char *);
int  PostQuestionDialog(Widget, char *);
void ManageDialogCenteredOnPointer(Widget);
void SetWatchCursor(int);

Widget CreateOptionMenu(Widget, MenuBarItem *);
void SetDefaultOptionMenuItem(Widget, int);
void SetDefaultOptionMenuItemString(Widget, MenuItem *, char *);
void SetPGStyle(PSSTY *);
void draw_main();
void redraw_graph(Widget, char *, XtPointer);
void UpdateData(int, int);
void wprintf();
int  InsidePolyLines(Point *);
void SetViewMode(int, scanPtr, MAP *, scatter *);
void uv2xy(Point, double, Point *);
void uvunit2xy(Point, double, int *, int *);
void pt2xy(double, double, int, Point *);
void ptunit2xy(double, double, int, int *, int *);
void pt4xy(double, double, double, double, int, Point *);
void ptunit4xy(double, double, double, double, int, int *, int *);

list    scan_iterator(list, DataSetPtr);
int     count_scans(DataSetPtr);
int     DeleteScan(DataSetPtr, scanPtr);
scanPtr copy_scan(DataSetPtr, scanPtr);

/*** Local variables ***/
double xmap(scanPtr), ymap(scanPtr), zmap(scanPtr), emap(scanPtr);
void   set_contour_levels(double, double, int, int, double);

static contour cont, tmp_cont;

typedef struct {
  double x[2];
  double y[2];
} PAIR;

typedef struct {
  int type;
  int x[2];
  int y[2];
} XPAIR;

#ifdef HAVE_LIBPGPLOT
typedef struct {
  int type;
  PLFLT x[2];
  PLFLT y[2];
} PLPAIR;
#endif

#define PAIR_UNDEF  -1
#define PAIR_HEAD    0
#define PAIR_TAIL    1

static MAP Map;
static int zType, cType, fType, rType, pType, forcePosAngle, tinyWindow;
static unsigned long int map_bytes = 0;
static double posAngle;
static double xStep, yStep, zeroSpacing, xCentre, yCentre;

static int     **gm    = NULL;
static double  **ddata = NULL;
static double  **edata = NULL;
static scanPtr **sdata = NULL;

static Widget slider[5];

static char *radio_cont_mode[] = {
   "No contours", "Show contours", "Contours and labels"
};
static char *radio_grey_mode[] = {
    "No shading", "Grey scale", "False colours"
};
static char *slider_labels[] = {
   "First contour", "Last contour", "No of contours", "Type of spacing"
};

static int slider_min[] = { -200, -199,           1, -50};
static int slider_100[] = { 1000, 1000, MAXCONTOURS,   0};
static int slider_max[] = { 1299, 1300, MAXCONTOURS, +50};
static int slider_dec[] = {    3,    3,           0,   0};

static void draw_contours(GC, GC, MAP *);
static void draw_mapbeam(GC, MAP *);
static void draw_projaxes(GC);
static void draw_projnums(GC);

int yunit2y(double), xunit2x(double);

/* Option menu setup for the kind of z-value in maps */
static void MagnXCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem MagnXData[] = {
  {"0.5", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnXCallback, "0.5", NULL},
  {"1.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnXCallback, "1.0", NULL},
  {"2.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnXCallback, "2.0", NULL},
  {"5.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnXCallback, "5.0", NULL},
  {"10.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnXCallback, "10.0", NULL},
  {"20.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnXCallback, "20.0", NULL},
  {"50.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnXCallback, "50.0", NULL},
EOI};
static MenuBarItem MagnXMenu = {
   "Select X magnification", ' ', True, MagnXData
};

static void MagnYCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem MagnYData[] = {
  {"0.5", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnYCallback, "0.5", NULL},
  {"1.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnYCallback, "1.0", NULL},
  {"2.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnYCallback, "2.0", NULL},
  {"5.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnYCallback, "5.0", NULL},
  {"10.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnYCallback, "10.0", NULL},
  {"20.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnYCallback, "20.0", NULL},
  {"50.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MagnYCallback, "50.0", NULL},
EOI};
static MenuBarItem MagnYMenu = {
   "Select Y magnification", ' ', True, MagnYData
};

static void set_view_type(Widget, char *, XmAnyCallbackStruct *);
static MenuItem SpecialViewTypeData[] = {
  {"None", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_view_type, "0", NULL},
  {"Top right", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_view_type, "1", NULL},
  {"Top left", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_view_type, "2", NULL},
  {"Bottom left", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_view_type, "3", NULL},
  {"Bottom right", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_view_type, "4", NULL},
EOI};
static MenuBarItem SpecialViewTypeMenu = {
   "Panel for special view", ' ', True, SpecialViewTypeData
};

static void WedgeCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem WedgeData[] = {
  {"Right", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, WedgeCallback, "0", NULL},
  {"Left", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, WedgeCallback, "1", NULL},
  {"Above", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, WedgeCallback, "2", NULL},
  {"Below", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, WedgeCallback, "3", NULL},
EOI};
static MenuBarItem WedgeMenu = {
   "Wedge position:", ' ', True, WedgeData
};

static void BeamCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem BeamData[] = {
  {"No", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, BeamCallback, "0", NULL},
  {"Lower left", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, BeamCallback, "1", NULL},
  {"Lower right", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, BeamCallback, "2", NULL},
  {"Upper left", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, BeamCallback, "3", NULL},
  {"Upper right", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, BeamCallback, "4", NULL},
EOI};
static MenuBarItem BeamMenu = {
   "Show beam", ' ', True, BeamData
};

static void ProjCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem ProjData[] = {
  {"None", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ProjCallback, "0", NULL},
  {"Sin(Theta)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ProjCallback, "1", NULL},
  {"Cos(Theta)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ProjCallback, "2", NULL},
  {"Mollweide", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ProjCallback, "3", NULL},
  {"Hammer-Aitoff", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ProjCallback, "4", NULL},
  {"Parabolic", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ProjCallback, "5", NULL},
  {"Orthographic, front/back", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ProjCallback, "6", NULL},
  {"Orthographic, north/south", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ProjCallback, "7", NULL},
  {"Orthographic, left/right", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ProjCallback, "8", NULL},
EOI};
static MenuBarItem ProjMenu = {
   "Projection", ' ', True, ProjData
};

static void ZTypeCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem ZTypeData[] = {
  {"Integrated intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback,  "0", NULL},
  {"Mean intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback,  "1", NULL},
  {"Gaussian amplitude", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback,  "2", NULL},
  {"Gaussian center", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback,  "3", NULL},
  {"Gaussian width", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback,  "4", NULL},
  {"Polynomial constant", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback,  "5", NULL},
  {"Polynomial slope", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback,  "6", NULL},
  {"Centroid velocity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback,  "7", NULL},
  {"2nd vel. moment", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback,  "8", NULL},
  {"Max. intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback,  "9", NULL},
  {"Min. intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback, "10", NULL},
  {"x unit at max. intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback, "11", NULL},
  {"x unit at min. intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback, "12", NULL},
  {"Intensity RMS", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ZTypeCallback, "13", NULL},
EOI};
static MenuBarItem ZTypeMenu = {
   "Type of z-value", ' ', True, ZTypeData
};

static void CTypeCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem CTypeData[] = {
  {"RA & Dec offsets", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, CTypeCallback, "0", NULL},
  {"Az & El offsets", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, CTypeCallback, "1", NULL},
  {"Gal. long. & Gal. Lat. offsets", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, CTypeCallback, "2", NULL},
EOI};
static MenuBarItem CTypeMenu = {
   "Type of map offsets", ' ', True, CTypeData
};

static void FTypeCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem FTypeData[] = {
  {"Reference (OSO)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FTypeCallback, "0", NULL},
  {"Absolute (SEST)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FTypeCallback, "1", NULL},
EOI};
static MenuBarItem FTypeMenu = {
   "Type of FITS CRVAL1/2", ' ', True, FTypeData
};

/* Option menu setup for the type of contour level spacing */
static void ContSpacingCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem ContSpacingData[] = {
  {"Linear", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ContSpacingCallback, "0", NULL},
  {"Logarithmic", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ContSpacingCallback, "1", NULL},
  {"Exponential", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ContSpacingCallback, "2", NULL},
  {"Squareroot", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ContSpacingCallback, "3", NULL},
  {"Quadratic", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ContSpacingCallback, "4", NULL},
  {"Varying", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ContSpacingCallback, "5", NULL},
EOI};
static MenuBarItem ContSpacingMenu = {
   "Type of contour level spacing", ' ', True, ContSpacingData
};

/* Option menu setup for the type of contour level spacing */
static void ContMinMaxCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem ContMinMaxData[] = {
  {"[min,max]", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ContMinMaxCallback, "0", NULL},
  {"[0,max]", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ContMinMaxCallback, "1", NULL},
EOI};
static MenuBarItem ContMinMaxMenu = {
   "Type of contour level range", ' ', True, ContMinMaxData
};

/* Option menu setup for the interpolation order */
static void IntpOrderCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem IntpOrderData[] = {
  {"No interpolation", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, IntpOrderCallback, "0", NULL},
  {"First order", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, IntpOrderCallback, "1", NULL},
  {"Second order", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, IntpOrderCallback, "2", NULL},
  {"Third order", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, IntpOrderCallback, "3", NULL},
EOI};
static MenuBarItem IntpOrderMenu = {
   "Map interpolation", ' ', True, IntpOrderData
};

/* Option menu setup for the interpolation type */
static void IntpTypeCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem IntpTypeData[] = {
  {"Bilinear", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, IntpTypeCallback, "0", NULL},
  {"Bicubic", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, IntpTypeCallback, "1", NULL},
EOI};
static MenuBarItem IntpTypeMenu = {
   "Type of interpolation", ' ', True, IntpTypeData
};

/* Option menu setup for the fill hole options */
static void IntpFillCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem IntpFillData[] = {
  {"No filling", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, IntpFillCallback, "0", NULL},
  {"One coner", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, IntpFillCallback, "1", NULL},
  {"Two corners", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, IntpFillCallback, "2", NULL},
  {"Three corners", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, IntpFillCallback, "3", NULL},
  {"Four corners", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, IntpFillCallback, "4", NULL},
EOI};
static MenuBarItem IntpFillMenu = {
   "Fill holes in map", ' ', True, IntpFillData
};

static void NDigitsCallback(Widget, char *, XmAnyCallbackStruct *);
static MenuItem NDigitsData[] = {
  {"1", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NDigitsCallback, "1", NULL},
  {"2", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NDigitsCallback, "2", NULL},
  {"3", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NDigitsCallback, "3", NULL},
  {"4", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NDigitsCallback, "4", NULL},
  {"5", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NDigitsCallback, "5", NULL},
  {"6", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NDigitsCallback, "6", NULL},
  {"7", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NDigitsCallback, "7", NULL},
EOI};
static MenuBarItem NDigitsMenu = {
   "No of digits in contour label", ' ', True, NDigitsData
};

static char *TypeOfMap_Help = "\
                           Type of map help\n\
                           ----------------\n\
In this dialog there are a number of options dealing with how maps are\n\
interpreted when reading map spectra. The options are:\n\
Type of z-value:        Here you may specify what z-value to be used when\n\
                        displaying maps based on spectra. Options:\n\
    Integrated intensity       Integrated intensity within moment box(es).\n\
    Mean intensity             Mean intensity within moment boxes.\n\
    Gaussian amplitude         The amplitudes of Gaussians fitted to map\n\
                               spectra. See under menu\n\
                                    'Gauss'->'Fit Gaussians to map spectra'\n\
    Gaussian center            Same as above but use the center of the fitted\n\
                               Gaussians.\n\
    Gaussian width             Same as above but use the FWHM width of the\n\
                               fitted Gaussians.\n\
    Polynomial constant        Use the zeroth order in the polynomial fitting\n\
                               of baselines. Useful when making maps of the\n\
                               continuum level.\n\
    Polynomial slope           Same as above, but use instead the first order\n\
                               (slope) of the polynomial fitting.\n\
    Centroid velocity          Use the centroid velocity as calculated within\n\
                               the moment boxes.\n\
    2nd vel. moment            Use the 2nd order moment (within the moment\n\
                               boxes).\n\
    Max. intensity             Use the maximum intensity within the moment\n\
                               boxes.\n\
    Min. intensity             Same as above, but use minimum intensity instead.\n\
    Intensity RMS              Use the RMS value (from baseline boxes).\n\n\
Type of map offsets:    Here you may override the type of offsets (as specified\n\
                        in the, e.g. FITS files). Useful when the data header\n\
                        lacks proper description of the type of offsets.\n\
                        Options:\n\
    RA & Dec offsets\n\
    Az & El offsets\n\
    Gal. long. & Gal. lat. offsets\n\n\
Type of FITS CRVAL1/2:  Some older (?) SEST data have absolute coordinates\n\
                        in the FITS header keywords CRVAL1 and CRVAL2. The\n\
                        normal way is to specify reference coordinates with\n\
                        these coordinates when a map is made. Options:\n\
    Reference (OSO)            default\n\
    Absolute (SEST)            Take into account that absolute coordinates\n\
                               are used in the CRVAL1/2 keywords when reading\n\
                               map spectra.\n\n\
Centre grid on offsets:\n\
                        Enter offsets to centre grid on.\n\
Force map spacing:\n\
                        Here you can explicitly specify a map spacing. If not\n\
                        zero the read scans will be gridded with these\n\
                        spacings regardless of the observed spacings.\n\
Regrid current data using the values above:\n\
                        Regrid the current scans again using the selected\n\
                        offsets and spacings. Normally the scans is only\n\
                        gridded once.\n\
Set offsets in scans to the current grid values:\n\
                        Used to set the offsets in each current scan according\n\
                        to the current gridding. Not that these scans cannot\n\
                        be regridded on a finer grid later on. A new data set\n\
                        is created.\n\n\
Minimum map spacing:    If the distance between two map positions is smaller\n\
                        than this value they are binned into the same\n\
                        position. Unit is arcsec.\n\n\
Map position angle:     Here you can specify a map position angle in degrees.\n\
                        When 'Force this PA' is selected the data offsets of\n\
                        the read scans will be derotated (with this PA) before\n\
                        the gridding.\n\
";

static char *ContSel_Help = "\
                        Contour selection help\n\
                        ----------------------\n\
From this dialog you can control the options of contour, grey scale, and false\n\
colour maps. The first section deals with contour levels. The options in the\n\
first section are:\n\
Type of contour level spacing:\n\
    Here you select the type of contour spacing. The options are:\n\
        Linear spacing\n\
        Logarithmic\n\
        Exponential\n\
        Square root type\n\
        Quadratic\n\
        Varying                 (see third section below)\n\
Type of contour level range:\n\
    Here you select the kind of range for the contour levels. The options are:\n\
        [min,max]             First contour is the minimum value in the map\n\
                              and the last contour is the maximum value in the\n\
                              map.\n\
        [0,max]               First contour is 0, and the last contour is\n\
                              determined by the maximum value in the map.\n\
Relative levels:\n\
    Here you specify if you want your contour levels to be spaced relative the\n\
    maximum and minimum values of the map.\n\
No of digits in contour label:\n\
    Here you can select the number of digits to be shown in the contour labels.\n\
    The default value is 3.\n\
Radio buttons:\n\
    No contours               Turn of contour levels.\n\
    Show contours             Display the values of the contour levels next to\n\
                              the contours. Somewhat slower method is used to\n\
                              draw the contour levels.\n\
    Contours and labels       Just draw the contour levels.\n\
No of digits in contour label\n\
    Here you may select the number of digits in the labels that are shown next\n\
    to the contour lines (if Slow mode, show values are selected, that is).\n\n\
------------------------------------------------------------------------------\n\
The second section contains various options:\n\
Radio buttons:\n\
    No shading:               Don't display grey scale or false color map.\n\
    Grey scale:               Display grey scale map.\n\
    False colours:            Display false color map. The false colour scale\n\
                              is fixed (rainbow like, similar to AIPS).\n\
Toggle buttons:\n\
    Invert:                   Invert grey scale or false color map.\n\
    Show wedge:               Draw a wedge scale indicating colour vs z-value.\n\
    Rotate with PA:           If a map has a position angle stored, rotate the\n\
                              map with this PA.\n\
    Blank pixels\n\
    inside polylines:         Pixels inside closed polylines will not be drawn\n\
                              if this button is checked. Contours are still\n\
                              unaffected.\n\
Interpolation...:\n\
    Will start up a separate dialog for how interpolating/filling holes in the\n\
    should be done. See separate help in that dialog.\n\
Wedge position:\n\
    Select where the wedge is to be displayed. The options are:\n\
        Right      (default)\n\
        Left\n\
        Above\n\
        Below\n\
Show beam:\n\
    Display the beam in a map (if it has a map stored into it, FITS keywords\n\
    BMAJ, BMIN, BPA). You can manually attach a beam to map under menu 'Maps'\n\
    and 'Manipulate maps'->'Edit beam in map'. The options are:\n\
        No\n\
        Lower left\n\
        Lower right\n\
        Upper left\n\
        Upper right\n\
Projection:\n\
    You can display two-dimensional data with different types of map\n\
    projections. The x- and y-values (Phi and Theta) are assumed to be in\n\
    degrees. The options are:\n\
        None\n\
        Sin(Theta)                   x' = x Sin(y), y' = y\n\
        Cos(Theta)                   x' = x Cos(y), y' = y\n\
        Mollweide                    A standard map projection\n\
        Hammer-Aitoff                as above\n\
        Parabolic                    as above\n\
        Orthographic, front/back     This projection is used when viewing the\n\
                                     surface distribution of a spherical body\n\
                                     from an infinite distance. Here two views\n\
                                     will be displayed; the front and back\n\
                                     views.\n\
        Orthographic, north/south    Same as above but two polar views.\n\
        Orthographic, left/right     Same as above but two side views.\n\
    It should be noted that the drawing of contour levels doesn't work for the\n\
    orthographic projections since they use two views. The drawing of projected\n\
    axes and labels can be controlled from menu 'Graph'->'Graph options'->\n\
    'Toggle proj. axes' and 'Toggle proj. numbers'.\n\n\
------------------------------------------------------------------------------\n\
Third section:\n\
Contour sliders:\n\
    Will display five sliders that can be used to interactively change the\n\
    appearance of the contour levels as well as move the moment box. The\n\
    sliders are:\n\
        First contour:         Change the the first contour. The scale is\n\
                               relative in the sense that 0 stands for a zero\n\
                               z-value, and 1 is the maximum value of the map.\n\
        Last contour:          Same as above but for the last contour.\n\
        No of contours:        Here you can specify the number of contours to\n\
                               draw.\n\
        Type of spacing:       If the 'Type of contour level spacing' is\n\
                               'Varying' this slider controls the type of\n\
                               spacing. The scale (from -50 to +50) is such\n\
                               that 0 corresponds to linear spacing, a positive\n\
                               value is exponential-type spacing and a negative\n\
                               value results in logarithmic-type spacing.\n\n\
Note that you can also set the contour levels manually under menu 'Graph'->\n\
'Set contour levels'.\n\
";

static char *IntpOrder_Help = "\
                       Interpolation options help\n\
                       --------------------------\n\
Map interpolation\n\
    Here the depth of the interpolation can be specified. The options are:\n\
        No interpolation\n\
        First order        New spacing=old/2\n\
        Second order       New spacing=old/4\n\
        Third order        New spacing=old/8\n\n\
Type of interpolation\n\
    Here you specify the method of the two-dimensional interpolation. The\n\
    options are:\n\
        Bilinear: A two-dimensional linear interpolation\n\
        Bicubic:  Will use the derivative when interpolating. Note that this\n\
                  method may result in extrapolated values of the interpolated\n\
                  pixels. It results often in very smooth contours.\n\n\
Fill holes in map\n\
    Here you specify how and whether holes in the map should be interpolated.\n\
    The options are:\n\
        No filling\n\
        One corner:    Interpolate empty pixels if they are surrounded by at\n\
                       least one corner. A corner is defined as\n\
                            x   o              o   x\n\
                                        or                     etc.\n\
                            x   x              x   x\n\
                       where the empty pixel at 'o' will be interpolated using\n\
                       the corner values at the three x's.\n\
        Two corners:   Same as above but with\n\
                            x   x              x   x   x\n\
\n\
                            x   o       or     x   o   x       etc.\n\
\n\
                            x   x\n\
        Three corners: Same as above but with\n\
                            x   x   x         x   x   x\n\
\n\
                            x   o   x   or    x   o   x        etc.\n\
\n\
                            x   x                 x   x\n\
        Four corners:  Same as above but with\n\
                            x   x   x\n\
\n\
                            x   o   x\n\
\n\
                            x   x   x\n\
";
