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

#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>

#include "defines.h"
#include "local.h"
#include "fits.h"
#include "global_structs.h"
#include "menus.h"
#include "dialogs.h"

/*** External variables ***/
void    PostErrorDialog(Widget, char *);
void    PostWarningDialog(Widget, char *);
void    PostMessageDialog(Widget, char *);
void    ManageDialogCenteredOnPointer(Widget);
void    wprintf();
Widget  CreateOptionMenu(Widget, MenuBarItem *);
void    SetDefaultOptionMenuItemString(Widget, MenuItem *, char *);
void    SetSiteCoord(double, double);
void    SetMaxMenuItems(int);
void    SetTinyWindow(int);

extern GLOBAL *gp;

/*** Local variables ***/
USER *pP;
static USER prefs;
static double version=-1.0;
static Holography Holo;
static APEXMap Apex;

typedef struct {
  string  key;
  char   *var;
  int     found;
  char   *def;
} PKEY;

static PKEY pkey[] = {
  {"KW_VERSION",    prefs.version,           0, "0.0"},		/*  0 */
  {"KW_DATADIR",    prefs.dataDir,           0, NULL},		/*  1 */
  {"KW_GAUSSDIR",   prefs.gaussDir,          0, NULL},		/*  2 */
  {"KW_TMPDIR",     prefs.tmpDir,            0, NULL},		/*  3 */
  {"KW_FIRSTFILE",  prefs.firstFile,         0, NULL},		/*  4 */
  {"KW_EDITOR",     prefs.editor,            0, "vi"},		/*  5 */
  {"KW_RM_CMD",     prefs.unixRmCmd,         0, UNIX_RM},	/*  6 */
  {"KW_MV_CMD",     prefs.unixMvCmd,         0, UNIX_MV},	/*  7 */
  {"KW_SORT_CMD",   prefs.unixSortCmd,       0, UNIX_SORT},	/*  8 */
  {"KW_CAT_CMD",    prefs.unixCatCmd,        0, UNIX_CAT},	/*  9 */
  {"KW_PRINTER",    prefs.printerCmd,        0, UNIX_LPR},	/* 10 */
  {"KW_SLAIM_D",    prefs.slaimDir,          0, NULL},		/* 11 */
  {"KW_SLAIM_F",    prefs.slaimFile,         0, NULL},		/* 12 */
  {"KW_LOVAS_D",    prefs.lovasDir,          0, NULL},		/* 13 */
  {"KW_LOVAS_F",    prefs.lovasFile,         0, NULL},		/* 14 */
  {"KW_IDENT_D",    prefs.identDir,          0, NULL},		/* 15 */
  {"KW_IDENT_F",    prefs.identFile,         0, NULL},		/* 16 */
  {"KW_JPL_D",      prefs.jplDir,            0, NULL},		/* 17 */
  {"KW_JPL_F",      prefs.jplFile,           0, NULL},		/* 18 */
  {"KW_FITS_TYPE",  prefs.bitpix,            0, "16"},		/* 19 */
  {"KW_XS_HEIGHT",  prefs.xsHeight,          0, "600"},		/* 20 */
  {"KW_XS_WIDTH",   prefs.xsWidth,           0, "900"},		/* 21 */
  {"KW_PS_HEIGHT",  prefs.psHeight,          0, "400"},		/* 22 */
  {"KW_PS_WIDTH",   prefs.psWidth,           0, "600"},		/* 23 */
  {"KW_CM_HEIGHT",  prefs.cmHeight,          0, "12.9"},	/* 24 */
  {"KW_CM_WIDTH",   prefs.cmWidth,           0, "19.4"},	/* 25 */
  {"KW_UNIT",       prefs.unit,              0, DEF_XUNIT},	/* 26 */
  {"KW_POLORDER",   prefs.polOrder,          0, "0"},		/* 27 */
  {"KW_ZEROLINE",   prefs.zeroLine,          0, "0"},		/* 28 */
  {"KW_CNTMARKER",  prefs.contMarker,        0, "5"},		/* 29 */
  {"KW_CNTMRKSIZE", prefs.contMarkerSize,    0, "2"},		/* 30 */
  {"KW_SCAMARKER",  prefs.scatMarker,        0, "5"},		/* 31 */
  {"KW_SCAMRKSIZE", prefs.scatMarkerSize,    0, "2"},		/* 32 */
  {"KW_MOUSEBUTT",  prefs.mouseButtons,      0, "123"},		/* 33 */
  {"KW_MEGABYTE",   prefs.limitMegaByte,     0, "0.0"},         /* 34 */
  {"KW_SITELAT",    prefs.siteLatitude,      0, "57.39583333"}, /* 35 */
  {"KW_SITELON",    prefs.siteLongitude,     0, "11.92638889"}, /* 36 */
  {"KW_MENUITEMS",  prefs.maxMenuItems,      0, "25"},          /* 37 */
  {"KW_TINYWINDOW", prefs.tinyWindow,        0, "10"}           /* 38 */
};

#define nKeys  ((int)(sizeof(pkey)/sizeof(PKEY)))

typedef struct {
    char *desc;
    double *var;
} HOLO;

static HOLO HoloData[] = {
    {"Phase scaling",     &Holo.Phase_k},
    {"Phase offset",      &Holo.Phase_m},
    {"Amplitude offset",  &Holo.Ampl_m},
    {"Array dimension",   &Holo.nDim},
    {"Sampling [\"]",     &Holo.Sampling},
    {"Nyquist rate",      &Holo.NyRate},
    {"Transm. az.",       &Holo.Az},
    {"Transm. el.",       &Holo.El},
    {"Transm. dist.",     &Holo.TransDist},
    {"Freq. [MHz]",       &Holo.TransFreq},
    {"Prim. mirror d.",   &Holo.Dprim},
    {"Seco. mirror d.",   &Holo.Dseco},
    {"Focal length",      &Holo.Fprim},
    {"Cassegr. magn.",    &Holo.Fmag},
    {"Dist. to ref.",     &Holo.Zref},
    {"Dist. to sec. ref.",   &Holo.Zseco},
    {"Defocussing  ",     &Holo.Defocus},
    {"Quad. leg width",   &Holo.QuadWidth},
    {"Extra mask",        &Holo.Mask},
    {"Outer radius",      &Holo.Ro},
    {"Inner radius",      &Holo.Ri},
    {"Distance from legs", &Holo.Rq}
};

static string HoloFitStr[] = {
  "Constant       ",
  "Slope x        ",
  "Slope y        ",
  "Defocus        ",
  "Diffraction    ",
  "Astigmatism x*x",
  "Astigmatism y*y",
  "Astigmatism x*y",
  "Coma (x^2+y^2)x",
  "Coma (x^2+y^2)y"
};

#define nHolo     ((int)(sizeof(HoloData)/sizeof(HOLO)))
#define nHoloFit  ((int)(sizeof(HoloFitStr)/sizeof(string)))

static HOLO APEXData[] = {
    {"xleft [\"]",                  &Apex.xleft},
    {"xright [\"]",                 &Apex.xright},
    {"xspacing [\"]",               &Apex.xspacing}, 
    {"ylower [\"]",                 &Apex.ylower},   
    {"yupper [\"]",                 &Apex.yupper},
    {"yspacing  [\"]",              &Apex.yspacing},
    {"Gaussian intp. width  [\"]",  &Apex.width}
};

#define nAPEX     ((int)(sizeof(APEXData)/sizeof(HOLO)))

typedef struct {
  char *desc;
  PKEY *key;
  PKEY *opt;
  Widget w;
  Widget wopt;
  int  browse;
  MenuBarItem *menu;
} PREF;

#define NULLPREF {NULL, NULL, NULL, NULL, NULL, 0}

static PREF DataPrefStruct[] = {
    {"Data directory", &pkey[1], NULL,
     NULL, NULL, BROWSE_DIR, NULL},
    {"Gauss directory", &pkey[2], NULL,
     NULL, NULL, BROWSE_DIR, NULL},
    {"Temporary directory", &pkey[3], NULL,
     NULL, NULL, BROWSE_DIR, NULL},
    {"File to read at startup", &pkey[4], NULL,
     NULL, NULL, BROWSE_FILE, NULL},
NULLPREF
};


static void SiteSetup(Widget, char *, XtPointer);
static MenuItem SiteData[] = {
   {"None", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "None", NULL},
   {"OSO", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "OSO", NULL},
   {"SEST", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "SEST", NULL},
   {"IRAM 30m", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "IRAM30m", NULL},
   {"IRAM PdB", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "IRAMPdB", NULL},
   {"JCMT", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "JCMT", NULL},
   {"CSO", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "CSO", NULL},
   {"NRAO 12m", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "NRAO12m", NULL},
   {"SMT/HHT", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "SMT", NULL},
   {"KOSMA", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "KOSMA", NULL},
   {"OVRO", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "OVRO", NULL},
   {"Eff 100m", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "Eff100m", NULL},
   {"APEX", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SiteSetup, "APEX", NULL},
EOI};
static MenuBarItem SiteMenu = {
   "Select site", ' ', True, SiteData
};

static PREF AstroPrefStruct[] = {
    {"Site long./lat. [deg]", &pkey[36], &pkey[35],
     NULL, NULL, 0, &SiteMenu},
NULLPREF
};

static PREF UnixPrefStruct[] = {
    {"Your favourite editor", &pkey[5], NULL,
     NULL, NULL, BROWSE_FILE, NULL},
    {"UNIX rm command", &pkey[6], NULL,
     NULL, NULL, BROWSE_FILE, NULL},
    {"UNIX mv command", &pkey[7], NULL,
     NULL, NULL, BROWSE_FILE, NULL},
    {"UNIX sort command", &pkey[8], NULL,
     NULL, NULL, BROWSE_FILE, NULL},
    {"UNIX cat command", &pkey[9], NULL,
     NULL, NULL, BROWSE_FILE, NULL},
    {"UNIX printer command (eg 'lpr -Pxxx')", &pkey[10], NULL,
     NULL, NULL, 0, NULL},
NULLPREF
};

static PREF FreqfilePrefStruct[] = {
    {"Directory of SLAIM", &pkey[11], NULL, NULL,
     NULL, BROWSE_DIR, NULL},
    {"SLAIM file name:", &pkey[12], NULL,
     NULL, NULL, 0, NULL},
    {"Directory of Lovas", &pkey[13], NULL,
     NULL, NULL, BROWSE_DIR},
    {"Lovas file name:", &pkey[14], NULL,
     NULL, NULL, 0, NULL},
    {"Directory of identification files", &pkey[15], NULL,
     NULL, NULL, BROWSE_DIR},
    {"Identification file name:", &pkey[16], NULL,
     NULL, NULL, 0, NULL},
    {"Directory location of JPL", &pkey[17], NULL,
     NULL, NULL, BROWSE_DIR},
    {"JPL file name:", &pkey[18], NULL,
     NULL, NULL, 0, NULL},
NULLPREF
};

static PREF WindowPrefStruct[] = {
    {"Pixel height/width for main window:", &pkey[20], &pkey[21],
     NULL, NULL, 0, NULL},
    {"Pixel height/width for PGPLOT window:", &pkey[22], &pkey[23],
     NULL, NULL, 0, NULL},
    {"Height/width in cm for PS plots:", &pkey[24], &pkey[25],
     NULL, NULL, 0, NULL},
NULLPREF
};

static char *editor_msg = "\
Don't know your editor, use 'setenv EDITOR xxx'\n\
or edit ~/.xsrc manually.";

static char *prefs_msg = "\
Ok. Preferences updated.\n\
Use menu option 'Save preferences' for a permanent change.\
";

static char *old_prefs = "\
Your ~/.xsrc was saved using an earlier version (%3.1f.%d) of " PKGNAME ".\n\
'Save preferences' file from menu 'Prefs'.\
";

static void FitsTypeSetup(Widget, char *, XtPointer);
static MenuItem FitsTypeData[] = {
   {"16 bit (short int)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, FitsTypeSetup, "16", NULL},
   {"32 bit (long int)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, FitsTypeSetup, "32", NULL},
   {"32 bit (float)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, FitsTypeSetup, "FLOAT", NULL},
   {"64 bit (double)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, FitsTypeSetup, "DOUBLE", NULL},
EOI};
static MenuBarItem FitsTypeMenu = {
   "Type of saved FITS files", ' ', True, FitsTypeData
};

static void UnitTypeSetup(Widget, char *, XtPointer);
static MenuItem UnitTypeData[] = {
   {"Velocity", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, UnitTypeSetup, "Velocity", NULL},
   {"Frequency (GHz)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, UnitTypeSetup, "Frequency", NULL},
   {"Frequency (MHz)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, UnitTypeSetup, "Freq.MHz", NULL},
   {"Freq. offset", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, UnitTypeSetup, "Freq.offset", NULL},
   {"Channel", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, UnitTypeSetup, "Channel", NULL},
EOI};
static MenuBarItem UnitTypeMenu = {
   "Type of x-scale at startup", ' ', True, UnitTypeData
};

static void PolOrderTypeSetup(Widget, char *, XtPointer);
static MenuItem PolOrderTypeData[] = {
   {" 0", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "0", NULL},
   {" 1", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "1", NULL},
   {" 2", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "2", NULL},
   {" 3", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "3", NULL},
   {" 4", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "4", NULL},
   {" 5", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "5", NULL},
   {" 6", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "6", NULL},
   {" 7", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "7", NULL},
   {" 8", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "8", NULL},
   {" 9", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "9", NULL},
   {"10", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "10", NULL},
   {"11", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "11", NULL},
   {"12", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "12", NULL},
   {"13", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "13", NULL},
   {"14", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PolOrderTypeSetup, "14", NULL},
EOI};
static MenuBarItem PolOrderTypeMenu = {
   "Polynomial order at startup", ' ', True, PolOrderTypeData
};

static void ZeroLineTypeSetup(Widget, char *, XtPointer);
static MenuItem ZeroLineTypeData[] = {
   {"No", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ZeroLineTypeSetup, "0", NULL},
   {"Yes", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ZeroLineTypeSetup, "1", NULL},
EOI};
static MenuBarItem ZeroLineTypeMenu = {
   "Draw zero line at startup?", ' ', True, ZeroLineTypeData
};

static void ContMarkerTypeSetup(Widget, char *, XtPointer);
static MenuItem ContMarkerTypeData[] = {
   {"None", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerTypeSetup, "0", NULL},
   {"Square", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerTypeSetup, "1", NULL},
   {"Circle", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerTypeSetup, "2", NULL},
   {"Value", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerTypeSetup, "3", NULL},
   {"Cross", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerTypeSetup, "4", NULL},
   {"Plus", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerTypeSetup, "5", NULL},
EOI};
static MenuBarItem ContMarkerTypeMenu = {
   "Type of map marker", ' ', True, ContMarkerTypeData
};

static void ContMarkerSizeSetup(Widget, char *, XtPointer);
static MenuItem ContMarkerSizeData[] = {
   {"0", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerSizeSetup, "0", NULL},
   {"1", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerSizeSetup, "1", NULL},
   {"2", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerSizeSetup, "2", NULL},
   {"3", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerSizeSetup, "3", NULL},
   {"4", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerSizeSetup, "4", NULL},
   {"5", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerSizeSetup, "5", NULL},
   {"6", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerSizeSetup, "6", NULL},
   {"7", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ContMarkerSizeSetup, "7", NULL},
EOI};
static MenuBarItem ContMarkerSizeMenu = {
   "Size of map marker", ' ', True, ContMarkerSizeData
};

static void ScatMarkerTypeSetup(Widget, char *, XtPointer);
static MenuItem ScatMarkerTypeData[] = {
   {"None", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerTypeSetup, "0", NULL},
   {"Square", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerTypeSetup, "1", NULL},
   {"Circle", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerTypeSetup, "2", NULL},
   {"Value", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerTypeSetup, "3", NULL},
   {"Cross", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerTypeSetup, "4", NULL},
   {"Plus", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerTypeSetup, "5", NULL},
EOI};
static MenuBarItem ScatMarkerTypeMenu = {
   "Type of scatter plot marker", ' ', True, ScatMarkerTypeData
};

static void ScatMarkerSizeSetup(Widget, char *, XtPointer);
static MenuItem ScatMarkerSizeData[] = {
   {"0", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerSizeSetup, "0", NULL},
   {"1", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerSizeSetup, "1", NULL},
   {"2", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerSizeSetup, "2", NULL},
   {"3", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerSizeSetup, "3", NULL},
   {"4", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerSizeSetup, "4", NULL},
   {"5", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerSizeSetup, "5", NULL},
   {"6", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerSizeSetup, "6", NULL},
   {"7", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, ScatMarkerSizeSetup, "7", NULL},
EOI};
static MenuBarItem ScatMarkerSizeMenu = {
   "Size of scatter plot marker", ' ', True, ScatMarkerSizeData
};

static void MouseButtonSetup(Widget, char *, XtPointer);
static MenuItem MouseButtonData[] = {
   {"Zooming-Boxes-Gaussians", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MouseButtonSetup, "123", NULL},
   {"Zooming-Gaussians-Boxes", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MouseButtonSetup, "132", NULL},
   {"Boxes-Gaussians-Zooming", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MouseButtonSetup, "312", NULL},
EOI};
static MenuBarItem MouseButtonMenu = {
   "Mouse buttons (lef-mid-rig)", ' ', True, MouseButtonData
};

static void MemoryWarningSetup(Widget, char *, XtPointer);
static MenuItem MemoryWarningData[] = {
   {"None", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MemoryWarningSetup, "0.0", NULL},
   {"4 Mb", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MemoryWarningSetup, "4.0", NULL},
   {"8 Mb", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MemoryWarningSetup, "8.0", NULL},
   {"16 Mb", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MemoryWarningSetup, "16.0", NULL},
   {"32 Mb", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MemoryWarningSetup, "32.0", NULL},
   {"64 Mb", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MemoryWarningSetup, "64.0", NULL},
   {"128 Mb", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MemoryWarningSetup, "128.0", NULL},
   {"256 Mb", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MemoryWarningSetup, "256.0", NULL},
   {"512 Mb", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MemoryWarningSetup, "512.0", NULL},
EOI};
static MenuBarItem MemoryWarningMenu = {
   "Memory warning limit", ' ', True, MemoryWarningData
};

static void MaxMenuItemsSetup(Widget, char *, XtPointer);
static MenuItem MaxMenuItemsData[] = {
   {"10", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MaxMenuItemsSetup, "10", NULL},
   {"15", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MaxMenuItemsSetup, "15", NULL},
   {"20", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MaxMenuItemsSetup, "20", NULL},
   {"25", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MaxMenuItemsSetup, "25", NULL},
   {"30", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MaxMenuItemsSetup, "30", NULL},
   {"35", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MaxMenuItemsSetup, "35", NULL},
   {"40", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MaxMenuItemsSetup, "40", NULL},
   {"50", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MaxMenuItemsSetup, "50", NULL},
   {"No limit", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, MaxMenuItemsSetup, "0", NULL},
EOI};
static MenuBarItem MaxMenuItemsMenu = {
   "Maximum no of menu items", ' ', True, MaxMenuItemsData
};

static void TinyWindowSetup(Widget, char *, XtPointer);
static MenuItem TinyWindowData[] = {
   {"No limit", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, TinyWindowSetup, "0", NULL},
   {"2", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, TinyWindowSetup, "2", NULL},
   {"5", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, TinyWindowSetup, "5", NULL},
   {"8", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, TinyWindowSetup, "8", NULL},
   {"10", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, TinyWindowSetup, "10", NULL},
   {"15", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, TinyWindowSetup, "15", NULL},
   {"20", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, TinyWindowSetup, "20", NULL},
   {"25", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, TinyWindowSetup, "25", NULL},
   {"30", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, TinyWindowSetup, "30", NULL},
   {"35", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, TinyWindowSetup, "35", NULL},
   {"40", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, TinyWindowSetup, "40", NULL},
EOI};
static MenuBarItem TinyWindowMenu = {
   "Draw tiny spectra? Pixel limit", ' ', True, TinyWindowData
};

static int check_prefs_file()
{
    string pref_file, buf;
    char *home=NULL, *xslib = NULL;
    FILE *fp;
    
    char *GetHome();
    char *XS_getenv();
    
    if (!(home = GetHome())) {
        fprintf(stderr,
                "Warning: Couldn't find the environment variable HOME, using ~.\n");
        strcpy(home, "~");
    }
    strcpy(pref_file, home);
    strcat(pref_file, "/.xsrc");
    
    strcpy(prefs.prefsFile, pref_file);
    
    fp = fopen(pref_file, "r");
    
    if (!fp) {
        xslib = XS_getenv("XS_LIB");
        if (!xslib) {
            sprintf(buf, "Cannot find %s or environment XS_LIB.\n", pref_file);
            fprintf(stderr, "%s", buf);
            return 2;
        }
        strcpy(pref_file, xslib);
        strcat(pref_file, "/.xsrc");
        sprintf(buf, "Trying file %s.\n", pref_file);
        fprintf(stderr, "%s", buf);
        fp = fopen(pref_file, "r");
        if (!fp) {
            sprintf(buf, "Cannot find ~/.xsrc or %s.\n", pref_file);
            fprintf(stderr, "%s", buf);
            return 2;
        }
        strcpy(prefs.prefsFile, pref_file);
    }
    
    fclose(fp);
    
    return 0;
}

static PKEY *FindPrefKey(const char *key)
{
    int n = 0;
    
    while (n < nKeys) {
        if (strcmp(pkey[n].key, key) == 0)
            return &pkey[n];
        n++;
    }
    
    return NULL;
}

static void read_prefs_file()
{
    int n;
    FILE *fp;
    char buf[512], *p;
    string s;
    PKEY *pKey;
    
    fp = fopen(prefs.prefsFile, "r");
    if (!fp) {
        return;
    }
    
    for (n=0; n<nKeys; n++)
        pkey[n].found = 0;
    
    while ((fgets(buf, 511, fp)) != NULL) {
        if (buf[0] != 'K' || buf[1] != 'W' || buf[2] != '_') continue;
        p = strtok(buf, " ");
        if (p) {
            strcpy(s, p);
            p = strtok(NULL, "\n");
            if (!p) continue;
        } else
            continue;
        pKey = FindPrefKey(s);
        if (pKey) {
            strcpy(pKey->var, p);
            pKey->found = 1;
        }
    }
    
    for (n=0; n<nKeys; n++) {
        if (!pkey[n].found) {
            if (!pkey[n].def)
                strcpy(pkey[n].var, "<empty>");
            else
                strcpy(pkey[n].var, pkey[n].def);
        }
    }
    
    fclose(fp);
}

static void MouseButtonSetup(Widget wid, char *cmd, XtPointer cd)
{    
    int l, m, r;
    string buf;

    void SetMouseButtons(int, int, int);
    
    if (wid) {
        if (strcmp(prefs.mouseButtons, cmd) == 0) return;

        prefs.dirty = 1;
        strcpy(prefs.mouseButtons, cmd);
    }
    
    if (sscanf(cmd, "%1d%1d%1d", &l, &m, &r) != 3) {
        sprintf(buf, "Error in the mouse button pref. format: %s\n", cmd);
        if (wid) PostErrorDialog(NULL, buf);
        return;
    }
    if (l < LEFT_BUTTON || l > RIGHT_BUTTON || m < LEFT_BUTTON ||
        m > RIGHT_BUTTON || r < LEFT_BUTTON || r > RIGHT_BUTTON) {
        sprintf(buf, "Error in the mouse button pref. format: %s\n", cmd);
        if (wid) PostErrorDialog(NULL, buf);
        return;
    }
    
    if (wid) {
        sprintf(buf, "Changed bindings for mouse buttons. (%s).", cmd);
        PostMessageDialog(NULL, buf);
    }
    
    SetMouseButtons(l, m, r);
}

static void MemoryWarningSetup(Widget wid, char *cmd, XtPointer cd)
{
    void SetMemoryWarningLimit(double);
    
    if (strcmp(prefs.limitMegaByte, cmd) == 0) return;
    
    prefs.dirty = 1;
    strcpy(prefs.limitMegaByte, cmd);
    
    SetMemoryWarningLimit(strtod(cmd, NULL));
}

void init_prefs()
{
    int n;
    Holography *h;
    APEXMap *am;
    
    char *ptr = NULL, *xslib = NULL;
    
    char *XS_getenv();
    char *GetHome();
    
    h = gp->hp = &Holo;
    am = gp->am = &Apex;

    pP = &prefs;
    prefs.dirty = 0;
    
    strcpy(prefs.limitMegaByte, "0.0");
    strcpy(prefs.tinyWindow, "5");
    if (check_prefs_file()) {
        sprintf(prefs.version, "%f", XS_FULLVER);
        fprintf(stderr, "Using default preferences...\n");
        ptr = XS_getenv("EDITOR");
        if (ptr) {
            strcpy(prefs.editor, ptr);
        } else {
            strcpy(prefs.editor, DEF_EDITOR);
        }
        
        strcpy(prefs.unixSortCmd, UNIX_SORT);
        strcpy(prefs.unixMvCmd,   UNIX_MV);
        strcpy(prefs.unixRmCmd,   UNIX_RM);
        strcpy(prefs.unixCatCmd,  UNIX_CAT);
        strcpy(prefs.printerCmd,  UNIX_LPR);
        
        ptr = GetHome();
        if (ptr) {
            strcpy(prefs.homeDir, ptr);
        } else {
            strcpy(prefs.homeDir, "./");
        }
        strcpy(prefs.dataDir,  DATA_DIR);
        strcpy(prefs.gaussDir, GAUSS_DIR);
        strcpy(prefs.tmpDir,   TMP_DIR);
        
        xslib = XS_getenv("XS_LIB");
        
        if (xslib) {
            strcpy(prefs.firstFile, xslib);
            strcat(prefs.firstFile, "/mace.fits");
            strcpy(prefs.slaimDir,  xslib);
            strcpy(prefs.lovasDir,  xslib);
            strcpy(prefs.jplDir,    xslib);
        } else {
            strcpy(prefs.firstFile, DEF_FILE);
            strcpy(prefs.slaimDir,  slaim_dir);
            strcpy(prefs.lovasDir,  lovas_dir);
            strcpy(prefs.jplDir,    jpl_dir);
        }
        
        strcpy(prefs.slaimFile, slaim_file);
        strcpy(prefs.lovasFile, lovas_file);
        strcpy(prefs.jplFile,   jpl_file);
        strcpy(prefs.identDir,  ident_dir);
        strcpy(prefs.identFile, ident_file);
        strcpy(prefs.bitpix,    "16");
        strcpy(prefs.xsWidth,   "900");
        strcpy(prefs.xsHeight,  "600");
        strcpy(prefs.psWidth,   "600");
        strcpy(prefs.psHeight,  "400");
        strcpy(prefs.cmWidth,   "19.4");
        strcpy(prefs.cmHeight,  "12.9");
        strcpy(prefs.unit,      DEF_XUNIT);
        strcpy(prefs.maxMenuItems, "0");
        MouseButtonSetup(NULL, "123", NULL);
    } else {
        read_prefs_file();
        if (pkey[0].found) sscanf(prefs.version, "%lf", &version);
        MouseButtonSetup(NULL, prefs.mouseButtons, NULL);
        SetSiteCoord(atof(prefs.siteLongitude), atof(prefs.siteLatitude));
    }
    SetMaxMenuItems(atoi(prefs.maxMenuItems));
    SetTinyWindow(atoi(prefs.tinyWindow));
    strcpy(prefs.xs_xpm, "xs.xpm");
    strcpy(prefs.gauss_xpm, "gauss.xpm");
    strcpy(prefs.msgs_xpm, "msgs.xpm");
    
    /* Holo defaults to APEX */
    h->Phase_k = 1.597e-3;
    h->Phase_m = -0.818;
    h->Ampl_m = 450.0;
    h->Zref = 2.18;
    h->Zseco = 6.98;
    h->nDim = 128.;
    h->Sampling = 42.0;
    h->NyRate = 0.7525;
    h->Az = 38.0; h->El = 12.0;
    h->TransFreq = 92400.0;
    h->TransDist = 1800.0;
    h->Fprim = 4.8;
    h->Fmag = 20.0;
    h->QuadWidth = 0.1;
    h->Dprim = 12.0;
    h->Dseco = 0.75;
    h->Defocus = 0.013;
    h->Mask = 0.0;
    h->Ro = 5.0;
    h->Ri = 1.0;
    h->Rq = 0.2;
    for (n=0; n<10; n++) {
      h->p[n]=h->q[n]=0.0;
      h->fit[n]=1;
      h->fit[4]=0;
    }
    /* APEX map defaults */
    am->xleft = -200.0;
    am->xright = 200.0;
    am->xspacing = 5.0;
    am->ylower = -200.0;
    am->yupper = 200.0;
    am->yspacing = 5.0;
    am->width = 3.0;
    am->Az = am->El = 0.0;
}

static double GetVersion(double v)
{
    return ((int)10.0*v)/10.0;
}

static int GetPatchLevel(double v)
{
    int p;
    double a;
    
    a = 10.0*v - (int)(10.0*v);
    p = 100.0*a;
    
    return p;
}

void check_prefs()
{
    string buf;
    
    if (version < XS_FULLVER) {
        sprintf(buf, old_prefs, GetVersion(version), GetPatchLevel(version));
        sprintf(prefs.version, "%f", XS_FULLVER);
        PostWarningDialog(NULL, buf);
    }
}

char *GetStringFromUnit(int unit)
{
    static string s;
    
    switch (unit) {
        case UNIT_FRE:
            strcpy(s, "Frequency");
            break;
        case UNIT_FMHZ:
            strcpy(s, "Freq.MHz");
            break;
        case UNIT_FOFF:
            strcpy(s, "Freq.offset");
            break;
        case UNIT_VEL:
            strcpy(s, "Velocity");
            break;
        case UNIT_CHA:
            strcpy(s, "Channel");
            break;
        default:
            strcpy(s, "<unknown>");
            break;
    }
    
    return s;
}

int GetUnitFromString(char *unit)
{
    if (strcmp(unit, "Frequency") == 0 || strcmp(unit, "frequency") == 0)
        return UNIT_FRE;
    else if (strcmp(unit, "Velocity") == 0 || strcmp(unit, "velocity") == 0)
        return UNIT_VEL;
    else if (strcmp(unit, "Channel") == 0 || strcmp(unit, "channel") == 0)
        return UNIT_CHA;
    else if (strcmp(unit, "Arcsec") == 0 || strcmp(unit, "arcsec") == 0)
        return UNIT_ASEC;
    else if (strcmp(unit, "Arcmin") == 0 || strcmp(unit, "arcmin") == 0)
        return UNIT_AMIN;
    else if (strcmp(unit, "Freq.offset") == 0 || strcmp(unit, "freq.offset") == 0)
        return UNIT_FOFF;
    else if (strcmp(unit, "Freq.MHz") == 0 || strcmp(unit, "freq.mhz") == 0)
        return UNIT_FMHZ;
    else
        return UNIT_FRE;
}

int GetStartingUnit()
{
    return GetUnitFromString(prefs.unit);
}

int GetTypeOfFITS()
{
    if (strcmp(prefs.bitpix, "16") == 0)
        return FITS_TYPE_16BIT;
    else if (strcmp(prefs.bitpix, "32") == 0)
        return FITS_TYPE_32BIT;
    else if (strcmp(prefs.bitpix, "FLOAT") == 0 ||
             strcmp(prefs.bitpix, "float") == 0)
        return FITS_TYPE_FLOAT;
    else if (strcmp(prefs.bitpix, "DOUBLE") == 0 ||
             strcmp(prefs.bitpix, "double") == 0)
        return FITS_TYPE_DOUBLE;
    else
        return FITS_TYPE_UNKNOWN;
}

static void SavePrefsFile(Widget w, char *cmd, XmAnyCallbackStruct *cd)
{
    int n;
    char *home = NULL;
    string buf;
    FILE *fp;
    
    char *GetHome();
    
    if (!(home = GetHome())) {
        strcpy(prefs.prefsFile, "./.xsrc");
    } else {
        strcpy(prefs.prefsFile, home);
        strcat(prefs.prefsFile, "/.xsrc");
    }
    
    fp = fopen(prefs.prefsFile, "w");
    if (!fp) {
        return;
    }
    
    fprintf(fp, "%s\n", "### This is the .xsrc init file.");
    
    for (n=0; n<nKeys; n++) {
        fprintf(fp, "%s %s\n", pkey[n].key, pkey[n].var);
    }
    
    fclose(fp);
    
    prefs.dirty = 0;
    sprintf(buf, "Preferences saved in '%s'.", prefs.prefsFile);
    PostMessageDialog(NULL, buf);
}

void MenuSavePrefsFile(Widget w, char *cmd, XtPointer cd)
{
    SavePrefsFile(w, cmd, NULL);
}

void ViewPrefsFile(Widget w, char *cmd, XtPointer cd)
{
    string buf;
    
    void XS_system();
    
    if (strlen(prefs.editor)) {
        sprintf(buf, "%s %s", prefs.editor, prefs.prefsFile);
        XS_system(buf, 0);
    } else {
        PostErrorDialog(w, editor_msg);
    }
}

char *GetTmpFile(const char *file)
{
    static string tmp_file;
    static int n = 0;
    
    int XS_getuid(), XS_getpid();
    
    sprintf(tmp_file, "%s/%s.%d.%d.%d",
            prefs.tmpDir, file, XS_getuid(), XS_getpid(), n);
    
    n++;
    
    return tmp_file;
}

int GzippedFile(char *file, char *tmp_file)
{
    string cmd;
    string gz_file;
    
    void XS_system(const char *, int);
    
    if (!tmp_file) {
        sprintf(cmd, "%s %s", UNIX_RM, file);
        XS_system(cmd, 1);
        return 0;
    }
    
    if (strstr(file, ".gz")) {
        strcpy(tmp_file, GetTmpFile("xs"));
        strcpy(gz_file, tmp_file);
        strcat(gz_file, ".gz");
        sprintf(cmd, "%s %s %s", UNIX_CP, file, gz_file);
        XS_system(cmd, 1);
        sprintf(cmd, "%s %s", UNIX_GUNZIP, gz_file);
        XS_system(cmd, 1);
        return 1;
    }
    
    return 0;
}

int DirtyPrefs()
{
    return prefs.dirty;
}

static void UpdatePrefs(Widget w, PREF *first, XmAnyCallbackStruct *cd)
{
    PREF *p = first;
    
    void wsscanf();
    
    while (p->desc != NULL) {
        wsscanf(p->w, p->key->var);
        if (p->opt && p->wopt) wsscanf(p->wopt, p->opt->var);
        p++;
    }
    
    SetSiteCoord(atof(prefs.siteLongitude), atof(prefs.siteLatitude));
    prefs.dirty = 1;
    PostMessageDialog(NULL, prefs_msg);
}

static void DoBrowse(Widget w, PREF *p, XmPushButtonCallbackStruct *cd)
{
    char *txt;
    string buf;
    
    void BrowseFile(Widget, int, char **);
    
    if (!p) return;
    
    BrowseFile(w, p->browse, &txt);
    if (!txt) return;
    
    strcpy(buf, txt);
    XtFree(txt);

    if (p->browse == BROWSE_DIR) buf[strlen(buf)-1] = '\0';    
    wprintf(p->w, "%s", buf);
}

static void FitsTypeSetup(Widget wid, char *cmd, XtPointer cd)
{    
    if (strcmp(prefs.bitpix, cmd) == 0) return;
    
    prefs.dirty = 1;
    strcpy(prefs.bitpix, cmd);
}

static void UnitTypeSetup(Widget wid, char *cmd, XtPointer cd)
{    
    if (strcmp(prefs.unit, cmd) == 0) return;
    
    prefs.dirty = 1;
    strcpy(prefs.unit, cmd);
}

static void PolOrderTypeSetup(Widget wid, char *cmd, XtPointer cd)
{    
    if (strcmp(prefs.polOrder, cmd) == 0) return;
    
    prefs.dirty = 1;
    strcpy(prefs.polOrder, cmd);
}

static void ZeroLineTypeSetup(Widget wid, char *cmd, XtPointer cd)
{    
    if (strcmp(prefs.zeroLine, cmd) == 0) return;
    
    prefs.dirty = 1;
    strcpy(prefs.zeroLine, cmd);
}

static void ContMarkerTypeSetup(Widget wid, char *cmd, XtPointer cd)
{    
    if (strcmp(prefs.contMarker, cmd) == 0) return;
    
    prefs.dirty = 1;
    strcpy(prefs.contMarker, cmd);
}

static void ContMarkerSizeSetup(Widget wid, char *cmd, XtPointer cd)
{    
    if (strcmp(prefs.contMarkerSize, cmd) == 0) return;
    
    prefs.dirty = 1;
    strcpy(prefs.contMarkerSize, cmd);
}

static void ScatMarkerTypeSetup(Widget wid, char *cmd, XtPointer cd)
{    
    if (strcmp(prefs.scatMarker, cmd) == 0) return;
    
    prefs.dirty = 1;
    strcpy(prefs.scatMarker, cmd);
}

static void ScatMarkerSizeSetup(Widget wid, char *cmd, XtPointer cd)
{    
    if (strcmp(prefs.scatMarkerSize, cmd) == 0) return;
    
    prefs.dirty = 1;
    strcpy(prefs.scatMarkerSize, cmd);
}

static void MaxMenuItemsSetup(Widget wid, char *cmd, XtPointer cd)
{    
    if (strcmp(prefs.maxMenuItems, cmd) == 0) return;
    
    prefs.dirty = 1;
    strcpy(prefs.maxMenuItems, cmd);
    SetMaxMenuItems(atoi(prefs.maxMenuItems));
}

static void TinyWindowSetup(Widget wid, char *cmd, XtPointer cd)
{    
    if (strcmp(prefs.tinyWindow, cmd) == 0) return;
    
    prefs.dirty = 1;
    strcpy(prefs.tinyWindow, cmd);
    SetTinyWindow(atoi(prefs.tinyWindow));
}

static void SiteSetup(Widget wid, char *cmd, XtPointer cd)
{
    static PREF *p = NULL;
    
    if (!cmd) { /* Initialize */
        p = (PREF *)cd;
        return;
    }
    
    if (!p) return;
    if (!p->w || !p->wopt) return;
    
    if (strcmp(cmd, "OSO")==0) {
        wprintf(p->w,     "11.92635330");
        wprintf(p->wopt,  "57.39583461");
    } else if (strcmp(cmd, "SEST")==0) {
        wprintf(p->w,    "-70.73444444");
        wprintf(p->wopt, "-29.25944444");
    } else if (strcmp(cmd, "APEX")==0) {
        wprintf(p->w,    "-67.75916667");
        wprintf(p->wopt, "-23.00577778");
    } else if (strcmp(cmd, "IRAM30m")==0) {
        wprintf(p->w,     "-3.39269444");
        wprintf(p->wopt,  "37.06619444");
    } else if (strcmp(cmd, "IRAMPdB")==0) {
        wprintf(p->w,      "5.90791667");
        wprintf(p->wopt,  "44.63388889");
    } else if (strcmp(cmd, "JCMT")==0) {
        wprintf(p->w,   "-155.47972222");
        wprintf(p->wopt,  "19.82583333");
    } else if (strcmp(cmd, "CSO")==0) {
        wprintf(p->w,   "-155.47972222");
        wprintf(p->wopt,  "19.82583333");
    } else if (strcmp(cmd, "Eff100m")==0) {
        wprintf(p->w,      "6.88444444");
        wprintf(p->wopt,  "50.525");
    } else if (strcmp(cmd, "KOSMA")==0) {
        wprintf(p->w,      "7.784");
        wprintf(p->wopt,  "45.984");
    } else if (strcmp(cmd, "OVRO")==0) {
        wprintf(p->w,   "-118.26555556");
        wprintf(p->wopt,  "37.23388889");
    } else if (strcmp(cmd, "SMT")==0) {
        wprintf(p->w,   "-109.89055556");
        wprintf(p->wopt,  "32.70138889");
    } else if (strcmp(cmd, "NRAO12m")==0) {
        wprintf(p->w,   "-111.61485417");
        wprintf(p->wopt,  "31.95333333");
    }
}

static void SetHoloFit(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n;
    
    void wdscanf(Widget, double *);
    
    for (n=0; n<nHoloFit; n++) {
      wdscanf(sf->edit[n], &(gp->hp->p[n])); 
    }
}

static void SetHoloData(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n;
    
    void wdscanf(Widget, double *);
    
    for (n=0; n<nHolo; n++) {
      wdscanf(sf->edit[n], HoloData[n].var); 
    }
}

static void fitbutt_changed(Widget w, char *ptr,
                                XmToggleButtonCallbackStruct *cb)
{
    int *fit;
    
    fit = (int *)ptr;
    
    if (cb->set) {
        *fit = 1;
    } else {
        *fit = 0;
    }
}

void EditHoloFit(Widget parent, char *cmd, XtPointer cd)
{
    int n, m;
    Arg wargs[10];
    Widget w = parent, rc;
    Widget rcH, fitbutt;
    StdForm *sf;
    Holography *h = gp->hp;

    while (!XtIsWMShell(w))
        w = XtParent(w);
    
    sf = PostStdFormDialog(w, "Holographic fitting",
             BUTT_APPLY, (XtCallbackProc)SetHoloFit, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
	     2*nHoloFit, NULL);
    /* sf->user = (XtPointer)h; */
    
    rc  = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                           XmNorientation, XmVERTICAL,
                           NULL);
   

    for (n=0; n<nHoloFit; n++) {
      rcH = XtVaCreateManagedWidget("rowcol", xmRowColumnWidgetClass, rc,
                              XmNorientation, XmHORIZONTAL,
                              NULL);
      XtCreateManagedWidget(HoloFitStr[n], xmLabelWidgetClass,
                            rcH, NULL, 0);
      m = 0;
      XtSetArg(wargs[m], XmNset, h->fit[n] ? True : False); m++;
      fitbutt = XtCreateManagedWidget("", xmToggleButtonWidgetClass,
                                      rcH, wargs, m);
      XtAddCallback(fitbutt, XmNvalueChangedCallback,
                    (XtCallbackProc)fitbutt_changed, &(h->fit[n]));
		    
      sf->edit[n] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                                rcH, NULL, 0);
      sf->edit[n+nHoloFit] = XtCreateManagedWidget("edit",
                                                xmTextWidgetClass,
                                                rcH, NULL, 0);
    }
    		  
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(rc);
    
    for (n=0; n<nHoloFit; n++) {
       wprintf(sf->edit[n], "%lf", h->p[n]);
       wprintf(sf->edit[n+nHoloFit], "%lf", h->q[n]);
    }
        
    ManageDialogCenteredOnPointer(sf->form);
}

void EditHoloData(Widget parent, char *cmd, XtPointer cd)
{
    int n;
    Widget w = parent, rc;
    StdForm *sf;
    /* Holography *h = gp->hp; */

    while (!XtIsWMShell(w))
        w = XtParent(w);
    
    sf = PostStdFormDialog(w, "Holographic setup data",
             BUTT_APPLY, (XtCallbackProc)SetHoloData, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
	     nHolo, NULL);
    /* sf->user = (XtPointer)h; */
    
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmHORIZONTAL,
                          XmNnumColumns, nHolo/2,
                          XmNadjustLast, False,
                          XmNpacking, XmPACK_COLUMN,
                          NULL);
    for (n=0; n<nHolo; n++) {
      XtCreateManagedWidget(HoloData[n].desc, xmLabelWidgetClass,
                            rc, NULL, 0);
      sf->edit[n] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                           rc, NULL, 0);
    }
    		  
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(rc);
    
    for (n=0; n<nHolo; n++) {
       wprintf(sf->edit[n], "%lf", *(HoloData[n].var));
    }
        
    ManageDialogCenteredOnPointer(sf->form);
}

static void SetAPEXData(Widget w, StdForm *sf, XmListCallbackStruct *cb)
{
    int n;
    
    void wdscanf(Widget, double *);
    
    for (n=0; n<nAPEX; n++) {
      wdscanf(sf->edit[n], APEXData[n].var); 
    }
}

void EditAPEXMap(Widget parent, char *cmd, XtPointer cd)
{
    int n;
    Widget w = parent, rc;
    StdForm *sf;
    /* APEXMap *am = gp->am; */

    while (!XtIsWMShell(w))
        w = XtParent(w);
    
    sf = PostStdFormDialog(w, "APEX map regrid setup",
             BUTT_APPLY, (XtCallbackProc)SetAPEXData, NULL,
             BUTT_CANCEL, NULL, NULL,
             NULL, NULL, NULL,
	     nHolo, NULL);
    /* sf->user = (XtPointer)am; */
    
    rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                          XmNorientation, XmHORIZONTAL,
                          XmNnumColumns, nAPEX,
                          XmNadjustLast, False,
                          XmNpacking, XmPACK_COLUMN,
                          NULL);
    for (n=0; n<nAPEX; n++) {
      XtCreateManagedWidget(APEXData[n].desc, xmLabelWidgetClass,
                            rc, NULL, 0);
      sf->edit[n] = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                           rc, NULL, 0);
    }
    		  
    ArrangeStdFormDialog(sf, rc);

    XtManageChild(rc);
    
    for (n=0; n<nAPEX; n++) {
       wprintf(sf->edit[n], "%lf", *(APEXData[n].var));
    }
        
    ManageDialogCenteredOnPointer(sf->form);
}

void EditPrefsFile(Widget parent, char *cmd, XtPointer cd)
{
    int nP = 0;
    Widget rc, butt, menu[12], w=parent, opt=NULL;
    PREF *p = NULL, *first = NULL;
    StdForm *sf;

    while (!XtIsWMShell(w))
        w = XtParent(w);

    if (strcmp(cmd, "UnixPrefs") == 0) {
        first = p = UnixPrefStruct;
    } else if (strcmp(cmd, "DataPrefs") == 0) {
        first = p = DataPrefStruct;
    } else if (strcmp(cmd, "FreqfilePrefs") == 0) {
        first = p = FreqfilePrefStruct;
    } else if (strcmp(cmd, "WindowPrefs") == 0) {
        first = p = WindowPrefStruct;
    } else if (strcmp(cmd, "AstroPrefs") == 0) {
        first = p = AstroPrefStruct;
    }
    
    if (first) {
        sf = PostStdFormDialog(w, "Edit current preferences",
                 BUTT_APPLY, (XtCallbackProc)UpdatePrefs, first,
                 BUTT_CANCEL, NULL, NULL,
                 NULL, NULL, NULL, 0, NULL);
        while (p->desc) {
            p++;
            nP++;
        }
        rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                              XmNorientation, XmHORIZONTAL,
                              XmNnumColumns, nP,
                              XmNadjustLast, False,
                              XmNpacking, XmPACK_COLUMN,
                              NULL);

        p = first;
        while (p->desc) {
            XtCreateManagedWidget(p->desc, xmLabelWidgetClass,
                                  rc, NULL, 0);
            p->w = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                         rc, NULL, 0);
            if (p->browse) {
                 butt = XtCreateManagedWidget(p->browse == BROWSE_FILE ?
                                              "Browse file..." :
                                              "Browse directory...",
                                              xmPushButtonWidgetClass,
                                              rc, NULL, 0);
                 XtAddCallback(butt, XmNactivateCallback,
                              (XtCallbackProc)DoBrowse, p);
            } else if (p->opt) {
                p->wopt = XtCreateManagedWidget("edit", xmTextWidgetClass,
                                                rc, NULL, 0);
            } else if (!(p->menu)) {
                XtCreateManagedWidget("", xmLabelWidgetClass,
                                      rc, NULL, 0);
            }
            if (p->menu) {
                opt = CreateOptionMenu(rc, p->menu);
                SetDefaultOptionMenuItemString(opt, SiteData, "None");
                SiteSetup(NULL, NULL, (XtPointer)p);
            }
            p++;
        }
    } else {
        sf = PostStdFormDialog(w, "Edit current preferences",
                 "Save prefs.", (XtCallbackProc)SavePrefsFile, NULL,
                 BUTT_CANCEL, NULL, NULL,
                 NULL, NULL, NULL, 0, NULL);
        rc = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, sf->form,
                              XmNorientation, XmHORIZONTAL,
                              XmNnumColumns, 6,
                              XmNadjustLast, False,
                              XmNpacking, XmPACK_COLUMN,
                              NULL);

        menu[0] = CreateOptionMenu(rc, &FitsTypeMenu);
        SetDefaultOptionMenuItemString(menu[0], FitsTypeData,
                                       prefs.bitpix);

        menu[1] = CreateOptionMenu(rc, &UnitTypeMenu);
        SetDefaultOptionMenuItemString(menu[1], UnitTypeData,
                                       prefs.unit);

        menu[2] = CreateOptionMenu(rc, &PolOrderTypeMenu);
        SetDefaultOptionMenuItemString(menu[2], PolOrderTypeData,
                                       prefs.polOrder);

        menu[3] = CreateOptionMenu(rc, &ZeroLineTypeMenu);
        SetDefaultOptionMenuItemString(menu[3], ZeroLineTypeData,
                                       prefs.zeroLine);

        menu[4] = CreateOptionMenu(rc, &ContMarkerTypeMenu);
        SetDefaultOptionMenuItemString(menu[4], ContMarkerTypeData,
                                       prefs.contMarker);

        menu[5] = CreateOptionMenu(rc, &ContMarkerSizeMenu);
        SetDefaultOptionMenuItemString(menu[5], ContMarkerSizeData,
                                       prefs.contMarkerSize);

        menu[6] = CreateOptionMenu(rc, &ScatMarkerTypeMenu);
        SetDefaultOptionMenuItemString(menu[6], ScatMarkerTypeData,
                                       prefs.scatMarker);

        menu[7] = CreateOptionMenu(rc, &ScatMarkerSizeMenu);
        SetDefaultOptionMenuItemString(menu[7], ScatMarkerSizeData,
                                       prefs.scatMarkerSize);
        menu[8] = CreateOptionMenu(rc, &MouseButtonMenu);
        SetDefaultOptionMenuItemString(menu[8], MouseButtonData,
                                       prefs.mouseButtons);
        menu[9] = CreateOptionMenu(rc, &MemoryWarningMenu);
        SetDefaultOptionMenuItemString(menu[9], MemoryWarningData,
                                       prefs.limitMegaByte); 
        menu[10]= CreateOptionMenu(rc, &MaxMenuItemsMenu);
        SetDefaultOptionMenuItemString(menu[10], MaxMenuItemsData,
                                       prefs.maxMenuItems);
        menu[11]= CreateOptionMenu(rc, &TinyWindowMenu);
        SetDefaultOptionMenuItemString(menu[11], TinyWindowData,
                                       prefs.tinyWindow);
    }

    ArrangeStdFormDialog(sf, rc);

    XtManageChild(rc);

    if (first) {
        p = first;
        while (p->desc) {
            wprintf(p->w, "%s", p->key->var);
            if (p->opt && p->wopt) wprintf(p->wopt, "%s", p->opt->var);
            p++;
        }
        if (opt) XtManageChild(opt);
    } else {
        XtManageChildren(menu, sizeof(menu)/sizeof(Widget));
    }
        
    ManageDialogCenteredOnPointer(sf->form);
}
