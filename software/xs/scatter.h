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
#define SCATTER_FIT_ITER   7

#define SCATTER_NO_FIT       0
#define SCATTER_LINE_FIT     1
#define SCATTER_POLY_FIT     2
#define SCATTER_GAUSS_FIT    3
#define SCATTER_EXP_FIT      4
#define SCATTER_ERFC_FIT     5
#define SCATTER_EXPPROF1_FIT 6
#define SCATTER_EXPPROF2_FIT 7
#define SCATTER_EXPPROF3_FIT 8
#define SCATTER_COMET_FIT    9
#define SCATTER_QUAD_FIT    10
#define SCATTER_LORENTZ_FIT 11
#define SCATTER_FOURIER_FIT 12
#define SCATTER_INVPOLY_FIT 13
#define SCATTER_DISGAU_FIT  14
#define SCATTER_POI_FIT     15

/* Global declarations */
extern int    pgplot;

extern PSDATA ps;
extern DRAW   draw;
extern VIEW   *vP;
extern GLOBAL *gp;
extern USER   *pP;

void PostErrorDialog(Widget, char *);
void PostWarningDialog(Widget, char *);
void PostMessageDialog(Widget, char *);
void ManageDialogCenteredOnPointer(Widget);

Widget CreateOptionMenu(Widget, MenuBarItem *);
void SetDefaultOptionMenuItem(Widget, int);
void SetDefaultOptionMenuItemString(Widget, MenuItem *, char *);
void SetPGStyle(PSSTY *);
void StdApplyCancel(Widget, Widget, Widget, Widget, Widget, Widget);
void SetViewMode(int, scanPtr, MAP *, scatter *);
void UpdateData(int, int);

int     count_scans(DataSetPtr);
list    scan_iterator(list, DataSetPtr);

/* Local declarations */
scatter scat;

#define nSODs 9

static char *scatterOptDescs[] = {
    "Reference x-coordinate:",
    "Reference y-coordinate:",
    "Inner radius:",
    "Outer radius:",
    "First PA [deg]:",
    "Second PA [deg]:",
    "Force Y error (> 0):",
    "Polyline nPos:",
    "Polyline width:"
};

static char *scatterLineDescs[] = {
    "Intercept:",
    "Slope:"
};

static char *scatterPolyDescs[] = {
    "a[0]:",
    "a[1]:",
    "a[2]:",
    "a[3]:",
    "a[4]:",
    "a[5]:",
    "a[6]:",
    "a[7]:",
    "a[8]:",
    "a[9]:"
};

static char *scatterInvPolyDescs[] = {
    "a[0]:",
    "a[1]:",
    "a[2]:",
    "a[3]:",
    "a[4]:",
    "a[5]:",
    "a[6]:",
    "a[7]:",
    "a[8]:",
    "a[9]:"
};

static char *scatterGaussDescs[] = {
    "Gaussian amplitude:",
    "Gaussian width:",
    "Gaussian centre:",
    "Gaussian offset:"
};

static char *scatterLorentzDescs[] = {
    "Lorentz amplitude:",
    "Lorentz width:",
    "Lorentz centre:",
    "Lorentz offset:"
};

static char *scatterExpDescs[] = {
    "Exponential ampl.:",
    "Exponential decay:",
    "Exponential centre:"
};

static char *scatterErfcDescs[] = {
    "Erfc amplitude:",
    "Erfc centre:",
    "Erfc width:"
};

static char *scatterExpProf1Descs[] = {
    "Amplitude:",
    "Centre velocity:",
    "Expansion velocity:",
    "Gamma:"
};

static char *scatterExpProf2Descs[] = {
    "Amplitude:",
    "Centre velocity:",
    "Expansion velocity:",
    "Turbulent velocity:"
};

static char *scatterExpProf3Descs[] = {
    "Amplitude:",
    "Centre velocity:",
    "Expansion velocity:",
    "Turbulent velocity:",
    "Optical depth:"
};

static char *scatterCometDescs[] = {
    "Amplitude:",
    "Gamma/Beam:"
};

static char *scatterDisGauDescs[] = {
    "Amplitude:",
    "Radius:",
    "Beam:"
};

static char *scatterQuadDescs[] = {
    "Amplitude 5/5:",
    "Rel ampl. 3/5:",
    "Rel ampl. 1/5:",
    "Group centre:",
    "Quadr. coupl. cnst:",
    "Line width:"
};

static char *scatterFourierDescs[] = {
    "Period, T:",
    "a[0]:",
    "a[1]:",
    "b[1]:",
    "a[2]:",
    "b[2]:",
    "a[3]:",
    "b[3]:",
    "a[4]:",
    "b[4]:"
};

static char *scatterPointingDescs[] = {
    "IA:",
    "IE:",
    "NPAE:",
    "CA:",
    "AN:",
    "AW:",
    "HECE:",
    "HASA:",
    "HACA:",
    "HESE:",
    "HESA:",
    "HASA2:",
    "HACA2:",
    "HESA2:",
    "HECA2:",
    "HECA3:",
    "HACA3:",
    "HESA3:"
};

static void FitScatterCallback(Widget, char *, XmAnyCallbackStruct *);

MenuItem FitMenuData[] = {
  {"No fit", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "0", NULL},
  {"Line", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "1", NULL},
  {"Polynomial", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "2", NULL},
  {"Gaussian", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "3", NULL},
  {"Exponential", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "4", NULL},
  {"Erfc", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "5", NULL},
  {"(1-x^2)^(g/2)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "6", NULL},
  {"Exp. profile", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "7", NULL},
  {"1-exp(...)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "8", NULL},
  {"Comet func", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "9", NULL},
  {"Quadrupole prof.", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "10", NULL},
  {"Lorentz", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "11", NULL},
  {"Fourier sum", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "12", NULL},
  {"Inverse poly.", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "13", NULL},
  {"Gaussian conv. disc", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "14", NULL},
  {"APEX pointing fit", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitScatterCallback, "15", NULL},
EOI};

MenuBarItem FitOptionMenu = {
   "Fitting function", ' ', True, FitMenuData
};

static void StatScatterCallback(Widget, char *, XmAnyCallbackStruct *);

MenuItem StatMenuData[] = {
  {"No", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, StatScatterCallback, "0", NULL},
  {"For all points", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, StatScatterCallback, "1", NULL},
  {"Only for visible points", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, StatScatterCallback, "2", NULL},
EOI};

MenuBarItem StatOptionMenu = {
   "Show statistics?", ' ', True, StatMenuData
};

static void JoinScatterCallback(Widget, char *, XmAnyCallbackStruct *);

MenuItem JoinMenuData[] = {
  {"No", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, JoinScatterCallback, "0", NULL},
  {"Yes", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, JoinScatterCallback, "1", NULL},
EOI};

MenuBarItem JoinOptionMenu = {
   "Join points?", ' ', True, JoinMenuData
};

static void LabelScatterCallback(Widget, char *, XmAnyCallbackStruct *);

MenuItem LabelMenuData[] = {
  {"No", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, LabelScatterCallback, "0", NULL},
  {"Yes", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, LabelScatterCallback, "1", NULL},
EOI};

MenuBarItem LabelOptionMenu = {
   "Show fitting label?", ' ', True, LabelMenuData
};

static void XTypeOfScatterCallback(Widget, char *, XmAnyCallbackStruct *);
static void YTypeOfScatterCallback(Widget, char *, XmAnyCallbackStruct *);

MenuItem XTypeMenuData[] = {
  {"Running number", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback,  "0", NULL},
  {"Scan number", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback,  "1", NULL},
  {"RA", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback,  "42", NULL},
  {"Dec", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback,  "43", NULL},
  {"RA offset", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback,  "2", NULL},
  {"Dec offset", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback,  "3", NULL},
  {"Distance from (0,0)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback,  "4", NULL},
  {"Position angle", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback,  "5", NULL},
  {"Azimuth", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback,  "6", NULL},
  {"OSO azimuth", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "36", NULL},
  {"Elevation", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback,  "7", NULL},
  {"Azimuth offset", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback,  "8", NULL},
  {"Elevation offset", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback,  "9", NULL},
  {"Equ. distance", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "10", NULL},
  {"Integrated intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "11", NULL},
  {"Mean intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "12", NULL},
  {"Integr. time", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "46", NULL},
  {"Sigma", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "13", NULL},
  {"System temperature", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "14", NULL},
  {"Tsys/sqrt(Bt)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "47", NULL},
  {"Date (YYMMDD.HHMMSS)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "15", NULL},
  {"UT (hr)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "40", NULL},
  {"Julian date", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "16", NULL},
  {"Modified Julian date", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "17", NULL},
  {"Gaussian amplitude", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "18", NULL},
  {"Gaussian centre", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "19", NULL},
  {"Gaussian width", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "20", NULL},
  {"Polynomial constant", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "21", NULL},
  {"Polynomial slope", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "22", NULL},
  {"Centroid velocity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "23", NULL},
  {"2nd moment velocity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "24", NULL},
  {"Center velocity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "25", NULL},
  {"Center frequency", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "26", NULL},
  {"Velocity res.", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "44", NULL},
  {"Frequency res.", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "45", NULL},
  {"Max. intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "30", NULL},
  {"Min. intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "31", NULL},
  {"Pol. angle (from El)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "34", NULL},
  {"Pol. angle (from Az)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "48", NULL},
  {"Cos(Pol. angle)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "35", NULL},
  {"Main beam eff.", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "37", NULL},
  {"Air mass", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "38", NULL},
  {"Atm. tau", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "39", NULL},
  {"Atm. temperature", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "49", NULL},
  {"Atm. pressure", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "50", NULL},
  {"Atm. rel. humidity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "51", NULL},
  {"Atm. Exp(tau)-1", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, XTypeOfScatterCallback, "52", NULL},
EOI};

MenuItem YTypeMenuData[] = {
  {"Running number", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback,  "0", NULL},
  {"Scan number", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback,  "1", NULL},
  {"RA", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback,  "42", NULL},
  {"Dec", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback,  "43", NULL},
  {"RA offset", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback,  "2", NULL},
  {"Dec offset", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback,  "3", NULL},
  {"Distance from (0,0)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback,  "4", NULL},
  {"Position angle", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback,  "5", NULL},
  {"Azimuth", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback,  "6", NULL},
  {"OSO azimuth", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "36", NULL},
  {"Elevation", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback,  "7", NULL},
  {"Azimuth offset", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback,  "8", NULL},
  {"Elevation offset", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback,  "9", NULL},
  {"Equ. distance", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "10", NULL},
  {"Integrated intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "11", NULL},
  {"Mean intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "12", NULL},
  {"Integr. time", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "46", NULL},
  {"Sigma", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "13", NULL},
  {"System temperature", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "14", NULL},
  {"Tsys/sqrt(Bt)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "47", NULL},
  {"Date (YYMMDD.HHMMSS)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "15", NULL},
  {"UT (hr)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "40", NULL},
  {"Julian date", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "16", NULL},
  {"Modified Julian date", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "17", NULL},
  {"Gaussian amplitude", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "18", NULL},
  {"Gaussian centre", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "19", NULL},
  {"Gaussian width", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "20", NULL},
  {"Polynomial constant", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "21", NULL},
  {"Polynomial slope", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "22", NULL},
  {"Centroid velocity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "23", NULL},
  {"2nd moment velocity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "24", NULL},
  {"Center velocity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "25", NULL},
  {"Center frequency", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "26", NULL},
  {"Velocity res.", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "44", NULL},
  {"Frequency res.", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "45", NULL},
  {"Max. intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "30", NULL},
  {"Min. intensity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "31", NULL},
  {"Pol. angle (from El)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "34", NULL},
  {"Pol. angle (from Az)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "48", NULL},
  {"Cos(Pol. angle)", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "35", NULL},
  {"Main beam eff.", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "37", NULL},
  {"Air mass", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "38", NULL},
  {"Atm. tau", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "39", NULL},
  {"Atm. temperature", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "49", NULL},
  {"Atm. pressure", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "50", NULL},
  {"Atm. rel. humidity", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "51", NULL},
  {"Atm. Exp(tau)-1", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, YTypeOfScatterCallback, "52", NULL},
EOI};

MenuBarItem XTypeOptionMenu = {
   "X axis parameter", ' ', True, XTypeMenuData
};
MenuBarItem YTypeOptionMenu = {
   "Y axis parameter", ' ', True, YTypeMenuData
};

typedef struct {
    double xRef, yRef;
    double PA1, PA2;
    double r1, r2;
    int xtype, ytype;
    double yerror;
    int updateError, stat;
    Widget e[nSODs];
    int fit;
    int join, label;
    int nPos;
    double Width;
    double xmean, ymean;
    double xsigma, ysigma;
    scatter *p;
} ScatterOpt;

#define MAX_PAR 18

typedef struct {
    double p, q;
    int    fit;
    Widget f, e, error;
} ScatterPar;

static ScatterOpt sopt;
static ScatterPar spar[MAX_PAR];

static int dotSize, dotType;

static char *plotopt_help = "\
                        Scatter plot option help\n\
                        ------------------------\n\
Reference x/y-coordinates\n\
    When maps are made into scatter plots, and one of the axes describes\n\
    distance (radius) or angle it is here possible to use these x and y\n\
    coordinates to supply a reference point other than (0,0).\n\
Inner/outer radius\n\
    Can be used to truncate data at an inner and an outer radius. If the\n\
    outer radius is less than the inner radius, the data will not be truncated.\n\
First/second PA\n\
    Can be used to truncate the data with respect to position angle. Unit\n\
    should be degrees.\n\
Force Y-error (> 0)\n\
    Can be used to explicitly set the 1sigma y-error of the data. Should in\n\
    principle only be used if the scatter data lack y-errors. If zero or less\n\
    it is ignored.\n\
Polyline nPos\n\
    Specifies the number of points when a cut (along a polyline) in an image is\n\
    done.\n\
Polyline width\n\
    Specifies how to smooth when interpolating the data along a polyline in a\n\
    map. Unless extra smoothing is required the value supplied should be\n\
    similar to the map spacing.\n\
Join points?\n\
    If selected will join the points of the scatter plot with a line. It is\n\
    only useful if the scatter plot data are sorted.\n\
Show fitting label?\n\
    If set to yes (default) the function fitted will be displayed in the plot.\n\
Show statistcis?\n\
    If set, the statistics of all points (or, alternatively, all visible points)\n\
    will be shown. The statistics will be represented as a dot (of the current\n\
    type) for the mean of x and y scatter values. Also errorbars will be shown\n\
    which indicate the 1-sigma scatter for the x and y-values, respectively.\n\
    A label with the values will also be shown.\n\
Fitting function\n\
    No fit:           Don't fit any function.\n\
    Line:             Straight line fit, y=kx+m.\n\
    Polynomial:       Fit a polynomial up to the 9th order.\n\
    Gaussian:         Fit a Gaussian + constant offset.\n\
    Exponential:      Fit an exponential.\n\
    Erfc:             Fit a complementary error function to the data, useful\n\
                      for scans across the moon limb to obtain beam widths.\n\
    (1-x^2)^(g/2):    Function suitable for CSEs.\n\
    Exp. profile:     Function for CSEs but is time consuming since it involves\n\
                      the numerical solving of an integral.\n\
    1-exp(...):       Same as above but includes also a parameter for optical\n\
                      depth.\n\
    Comet func:       Special for comet profiles.\n\
    Quadrupole prof.: Fit a quadrupole HFS group, only for J=1-0, I=5/2.\n\
    Lorentz:          Fit a Lorentz profile + constant offset.\n\
    Fourier sum:      Fit the first few terms of a Fourier sum. The sum is\n\
                      written as\n\
                          f(x) = a0/2 + a1 cos(A)  + b1 sin(A) +\n\
                                        a2 cos(2A) + b2 sin(2A) +\n\
                                        a3 cos(3A) + b3 sin(3A) +\n\
                                        a4 cos(4A) + b4 sin(4A)\n\
                      where A = 2 Pi x / T and T is the period.\n\
    Inverse poly.:    Fit an inverse polynomial up to the 9th order.\n\
Set fitting parameters...\n\
    Will start a dialog in which starting values of the fitting parameters.\n\
    Here one can also specify which parameters to fit.\n\
Update y-errors using fit\n\
    If selected the y-error will be updated so that the Chi^2 value is 1.\n\
";
