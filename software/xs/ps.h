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

/*** External variables ***/
extern GLOBAL *gp;
extern USER   *pP;

void    PostErrorDialog(Widget, char *);
void    ManageDialogCenteredOnPointer(Widget);
Widget  CreateOptionMenu(Widget, MenuBarItem *);
void    SetDefaultOptionMenuItem(Widget, int);
void    QueryOptionMenuItem(Widget, int *);
void    XS_system(const char *, int);
char   *GetTmpFile(const char *);
void    wdscanf(Widget, double *);
void    wiscanf(Widget, int *);
void    wfscanf(Widget, float *);
void    wsscanf(Widget, char *);
void    wprintf(Widget, char *, ...);
void    draw_main();
void    set_PS_file();
void    StdApplyCancel(Widget, Widget, Widget, Widget, Widget, Widget);

/*** Local variables ***/

#define ASPECT_DEFAULT 0
#define ASPECT_SQUARE  1
#define ASPECT_A4_PORT 2
#define ASPECT_A4_LAND 3
#define ASPECT_LETTER_PORT 4
#define ASPECT_LETTER_LAND 5

typedef struct {
    char *s;
    char c;
} STRINT;

int    pgplot=0;
PSDATA ps;

void PostPgmWindow(Widget, char *, XtPointer);
void ShowPostScriptFile(Widget, char *, XtPointer);

#ifdef HAVE_LIBPGPLOT

static void cancel_PS_dialog(Widget, char *, XmAnyCallbackStruct *);
static void PSStyleSetup(Widget, char *, XtPointer);
static void PSAxisSetup(Widget, char *, XtPointer);
static void PGSetup(Widget, char *, XtPointer);
static void SetPSTypeCallback();

/*** File Menu Items ***/

MenuItem PSSetupMenuData[] = {
   {"PS setup...", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PGSetup, "", NULL},
   {"Save PS setup...", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, set_PS_file, "wsetup", NULL},
   {"Read PS setup...", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, set_PS_file, "rsetup", NULL},
 MENUSEPARATOR,
   {"Exit PS viewer", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, cancel_PS_dialog, NULL, NULL},
EOI};

MenuItem PSStyleMenuData[] = {
   {"Outer frame", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "oframe", NULL},
   {"Inner frame", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "iframe", NULL},
   {"Special inner frame", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "triframe", NULL},
   {"Contour wedge", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "wedge", NULL},
   {"Header", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "header", NULL},
   {"Markers", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "marker", NULL},
   {"Position markers", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "posmarker", NULL},
   {"Frame labels", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "label", NULL},
   {"Inner labels", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "ilabel", NULL},
   {"Graph line", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "line", NULL},
   {"Secondary graph line", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "secondary", NULL},
   {"Graph zero line", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "zero", NULL},
   {"Gaussians", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "gauss", NULL},
   {"Polynomials", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "poly", NULL},
   {"Baseline boxes", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "blbox", NULL},
   {"Moment boxes", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "mobox", NULL},
   {"Beam box", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "bebox", NULL},
   {"Beam", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSStyleSetup, "beam", NULL},
EOI};

MenuItem PSAxisMenuData[] = {
   {"Outer frame x-axis", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSAxisSetup, "xOuter", NULL},
   {"Outer frame y-axis", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSAxisSetup, "yOuter", NULL},
   {"Inner frame(s) x-axis", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSAxisSetup, "xInner", NULL},
   {"Inner frame(s) y-axis", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSAxisSetup, "yInner", NULL},
   {"Special inner x-axis", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSAxisSetup, "xTR", NULL},
   {"Special inner y-axis", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSAxisSetup, "yTR", NULL},
   {"Contour wedge axis", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PSAxisSetup, "wedge", NULL},
EOI};

MenuItem PSHelpMenuData[] = {
EOI};

/*** Menu-Bar Items ***/

MenuBarItem PSMenuBarData[] = {
  {"Setup",      ' ', True, PSSetupMenuData},
  {"Style",      ' ', True, PSStyleMenuData},
  {"Axes",       ' ', True, PSAxisMenuData},
  {"Help",       ' ', True, PSHelpMenuData},
EOI};

MenuItem PSMenuData[] = {
   {"PS landscape", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSTypeCallback, "ps", NULL},
   {"PS portrait", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSTypeCallback, "vps", NULL},
   {"Color PS landscape", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSTypeCallback, "cps", NULL},
   {"Color PS portrait", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSTypeCallback, "vcps", NULL},
   {"Encapsulated PS", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSTypeCallback, "eps", NULL},
   {"Color EPS", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSTypeCallback, "ceps", NULL},
   {"GIF landscape", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSTypeCallback, "gif", NULL},
   {"GIF portrait", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSTypeCallback, "vgif", NULL},
   {"PNG", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSTypeCallback, "png", NULL},
   {"PNG transp. bkgrnd", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSTypeCallback, "tpng", NULL},
EOI};
MenuBarItem PSOptionMenu = {
   "as", ' ', True, PSMenuData
};

static void SetPSLineStyleCallback();
static void SetPSColorStyleCallback();
static void SetPSFontStyleCallback();
static void SetPSFillAreaStyleCallback();

#define NaxisEdts 2
static char *pgAxisEdits[] = {
    "Tick increment (0.0=automatic):",
    "No of subticks (0=automatic):"
};

#define NaxisOpts 14
static char *pgAxisOptions[] = {
 "a : draw Axis (X axis is horizontal line Y=0, Y axis is vertical\n    \
line X=0)",
 "b : draw bottom (X) or left (Y) edge of frame.",
 "c : draw top (X) or right (Y) edge of frame.",
 "g : draw Grid of vertical (X) or horizontal (Y) lines.",
 "i : Invert the tick marks; ie draw them outside the viewport\n    \
instead of inside.",
 "l : label axis Logarithmically.",
 "n : write Numeric labels in the conventional location below\n    \
the viewport (X) or to the left of the viewport (Y).",
 "p : extend (\"Project\") major tick marks outside the box\n    \
(ignored if option I is specified).",
 "m : write numeric labels in the unconventional location above\n    \
the viewport (X) or to the right of the viewport (Y).",
 "t : draw major Tick marks at the major coordinate interval.",
 "s : draw minor tick marks (Subticks).",
 "v : orient numeric labels Vertically. This is only applicable to Y.\n    \
The default is to write Y-labels parallel to the axis.",
 "1 : force decimal labelling, instead of automatic choice.",
 "2 : force exponential labelling, instead of automatic."
};
static char pgAxisLetters[] = {
  'a', 'b', 'c', 'g', 'i', 'l', 'n', 'p', 'm', 't', 's', 'v', '1', '2'
};
#define NaxisAll 16

static char *pgStyleLabels[] = {
   "Line thickness (>=1):", "Font scaling factor (1=normal):"
};

MenuItem PSLineStyleData[] = {
   {"Full line", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSLineStyleCallback, "1", NULL},
   {"Dashed", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSLineStyleCallback, "2", NULL},
   {"Dot-dash-dot-dash", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSLineStyleCallback, "3", NULL},
   {"Dotted", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSLineStyleCallback, "4", NULL},
   {"Dash-dot-dot-dot", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSLineStyleCallback, "5", NULL},
EOI};

MenuItem PSColorStyleData[] = {
   {"Background", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSColorStyleCallback, "0", NULL},
   {"Foreground", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSColorStyleCallback, "1", NULL},
   {"Red", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSColorStyleCallback, "2", NULL},
   {"Green", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSColorStyleCallback, "3", NULL},
   {"Blue", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSColorStyleCallback, "4", NULL},
   {"Yellow", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSColorStyleCallback, "5", NULL},
   {"Cyan", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSColorStyleCallback, "6", NULL},
   {"Magenta", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSColorStyleCallback, "7", NULL},
   {"Coral", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSColorStyleCallback, "8", NULL},
EOI};

MenuItem PSFontStyleData[] = {
   {"Default", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSFontStyleCallback, "1", NULL},
   {"Roman", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSFontStyleCallback, "2", NULL},
   {"Italic", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSFontStyleCallback, "3", NULL},
   {"Script", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSFontStyleCallback, "4", NULL},
EOI};

MenuItem PSFillAreaStyleData[] = {
   {"Solid", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSFillAreaStyleCallback, "1", NULL},
   {"Outline", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSFillAreaStyleCallback, "2", NULL},
   {"Hatched", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSFillAreaStyleCallback, "3", NULL},
   {"Cross-hatched", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSFillAreaStyleCallback, "4", NULL},
EOI};

MenuBarItem PSLineStyleMenu = {
   "Line style", ' ', True, PSLineStyleData
};

MenuBarItem PSColorStyleMenu = {
   "Color", ' ', True, PSColorStyleData
};

MenuBarItem PSFontStyleMenu = {
   "Font style", ' ', True, PSFontStyleData
};

MenuBarItem PSFillAreaStyleMenu = {
   "Area fill style", ' ', True, PSFillAreaStyleData
};

static char *pgplot_labels[] = {
   "Send to printer:",
   "Relative size of plot:",
   "Height (in cm) of PS plot:",
   "Width (in cm) of PS plot:",
   "Height (in pixels) of PGPLOT window:",
   "Width (in pixels) of PGPLOT window:",
   "x-axis label:",
   "y-axis label:",
   "Top label:",
   "Wedge label:"
};

static void SetPSWidgetSizeCallback();
static MenuItem PSWidgetSizeData[] = {
   {"Default", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSWidgetSizeCallback, "0", NULL},
   {"Square", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSWidgetSizeCallback, "1", NULL},
   {"A4 portrait", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSWidgetSizeCallback, "2", NULL},
   {"A4 landscape", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSWidgetSizeCallback, "3", NULL},
   {"US Letter portrait", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSWidgetSizeCallback, "4", NULL},
   {"US Letter landscape", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSWidgetSizeCallback, "5", NULL},
EOI};
static MenuBarItem PSWidgetSizeMenu = {
   "Aspect ratio of PGPLOT window", ' ', True, PSWidgetSizeData
};

static void SetPSPlotSizeCallback();
static MenuItem PSPlotSizeData[] = {
   {"Default", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSPlotSizeCallback, "0", NULL},
   {"Square 15x15 cm", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSPlotSizeCallback, "1", NULL},
   {"A4 portrait", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSPlotSizeCallback, "2", NULL},
   {"A4 landscape", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSPlotSizeCallback, "3", NULL},
   {"US Letter portrait", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSPlotSizeCallback, "4", NULL},
   {"US Letter landscape", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, SetPSPlotSizeCallback, "5", NULL},
EOI};
static MenuBarItem PSPlotSizeMenu = {
   "Size of PostScript plot", ' ', True, PSPlotSizeData
};

static Widget pgWidget, pgTop, cm_wWidget, cm_hWidget, wWidget, hWidget;
static int    pgOpen, c_offset, g_offset, f_offset;

#endif /* HAVE_LIBPGPLOT */

static char   pgType[10];
