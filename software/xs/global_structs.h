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
/* Structures that appear in the XS package */
#ifndef LIST_H
#include "list.h"
#endif

typedef char string[MAXSTRINGSIZE];
typedef float PLFLT;

typedef struct {
   int begin;                   /* First channel no in box             */
   int end;                     /* Last channel no in box              */
} BOX;

typedef struct {
    int use;
    double x1, x2;
    double y1, y2;
} Frame;

typedef struct {
    string name;
    list scanlist;
    int sequence;
    int gridded;
    double dx, dy;
    double posAngle;
    int r;
    BOX *rms;
    int m;
    BOX *mom;
} DataSet;

typedef DataSet *DataSetPtr; 

typedef struct {
    int Year;
    int Month;
    int Day;
    int Hour;
    int Min;
    int Sec;
} DATE;

typedef struct {
   int     nchan;               /* No of data channels in boxes        */
   double  sigma;               /* Standard deviation within all boxes */
   double  mean;                /* Mean within all boxes               */
   double  iint;                /* Integrated intensity with boxes     */
   double  iunc;                /* its uncertainty                     */
   double  v;                   /* Mean velocity                       */
   double  dv;                  /* Entire velocity interval            */
   double  vcent;               /* Centroid velocity, Sum[I*v]/Sum[I]  */
   double  v2mom;               /* Centroid velocity, Sum[I*v^2]/Sum[I]*/
   double  ucent;               /* Uncertainty in vcent                */
   double  u2mom;               /* Uncertainty in v2mom                */
   double  TMax;                /* Max. intensity in mom. boxes        */
   double  TMin;                /* Min. intensity in mom. boxes        */
   double  xTMax;               /* x val. at max. int. in mom. boxes   */
   double  xTMin;               /* x val. at min. int. in mom. boxes   */
} MOMENT;

typedef struct {
   double cen;                  /* Center channel of Gaussian comp.    */
   double amp;                  /* Amplitude of Gaussian comp.         */
   double wid;                  /* Width in channels of Gaussian       */
   double ucen;                 /* Unc. in center channel of comp.     */
   double uamp;                 /* Unc. in amplitude of comp.          */
   double uwid;                 /* Unc. in width in channels of comp.  */
} Gauss;

typedef struct {
   double x;                    /* Center in x-dir. of Gaussian comp.  */
   double y;                    /* Center in y-dir. of Gaussian comp.  */
   double A;                    /* Amplitude of Gaussian comp.         */
   double maj;                  /* Major axis FWHM width of Gaussian   */
   double min;                  /* Minor axis FWHM width of Gaussian   */
   double PA;                   /* Position angle (from y) of maj axis */
   double ux;                   /* Unc. in x                           */
   double uy;                   /* Unc. in y                           */
   double uA;                   /* Unc. in amplitude                   */
   double umaj;                 /* Unc. in maj                         */
   double umin;                 /* Unc. in min                         */
   double uPA;                  /* Unc. in PA                          */
} Gauss2D;

typedef struct {
    double maj;
    double min;
    double PA;
} Beam;

typedef struct {
   int    quick;
   int    relative;
   int    markers;
   int    dot_size;
   double zmin, zmax;
   int    nc;
   int    spacing;
   int    minmax;
   double pexp;
   double c[MAXCONTOURS];
   int    ndigits;
   int    intpType;
   int    intpOrder;
   int    nCorners;
   int    grey, grey_inverted;
   int    grey_res;
   int    blank;
} contour;

typedef struct {
    double x;
    double y;
} Point;

typedef struct {
    int n;             /* No of points                 */
    int type;          /* Type, open (0) or closed (1) */
    Point *p;          /* Vector of n (x,y) values     */
} PolyLine;

typedef struct {
   string name;
   string molecule;
   int    scan_no, subscan, saved, scanType;
   double freq0, freqn, freqres;
   double vel0, velres;
   double vlsr;
   double s_max, s_min;
   double r_max, r_min;
   Frame  frame;
   double tsys, tau, int_time;
   double coeffs[MAXORDER+1];
   double scoeffs[3];
   int    coordType;
   char   epoch;
   double equinox;
   double x0, y0;
   double az, el;
   double beameff, firstIF, lofreq;
   double tair, pair, rair;
   DATE   date, LST;
   double tx, ty;
   double xoffset, yoffset;
   double aoffset, eoffset;
   int    i, j;
   int    doublet;
   int    fft, nfft;
   int    added;
   MOMENT mom;
   int    gaussFit;
   Gauss  g;
   Beam   b;
   int    polarization;
   int    nChan;
   double *d;
   double *e;
} scan;

typedef scan *scanPtr;

struct xs_map {
   string     name;
   string     molecule;
   DATE       date;
   int        type, coordType, sequence, saved;
   int        ndata;
   char       epoch;
   double     equinox;
   double     x0, y0;
   double     xspacing, yspacing;
   double     xleft, xright;
   double     yupper, ylower;
   double     posAngle;
   Frame      frame;
   int        zType, unit;
   double     gx_min, gx_max;
   double     gt_min, gt_max;
   double     z_min, z_max;
   double     d_min, d_max;
   int        i_no,  j_no;
   int        i_min, i_max;
   int        j_min, j_max;
   double     v, dv;
   double     fMHz;
   int        use_attached_cont;
   contour    c;
   Gauss2D    g2;
   Beam       b;
   int        fft, nfftx, nffty;
   int        swapped, memed, interpolated;
   XRectangle r;
   struct xs_map *original;
   double     lam1, lam2;
   double   **d;
   double   **e;
   int      **f;
   scanPtr  **sp;
};

typedef struct xs_map MAP;
typedef MAP *mapPtr;

typedef struct {
    int type;
    double xle, xri;
    int xunit;
    double ylo, yup;
    int yunit;
} RECT;

typedef struct {
   int     norder;              /* Polynomial order                    */
   int     pol_type;            /* Polynomial type, 0=ordinary         */
                                /*                  1=Chebyshev        */
   double  sigma;               /* Standard deviation within all boxes */
   double  mean;                /* Mean within all boxes               */
   double  iint;                /* Integrated intensity with boxes     */
   double  iunc;                /* its uncertainty                     */
} BLINE;

typedef struct {
   int    chan;                 /* Modified channel number             */
   double new;                  /* New channel value                   */
   double old;                  /* Old channel value                   */
} MOD;

typedef struct {
   double first, last, step;
   int nsteps, digits;
   double subfirst, sublast, substep;
   int nsubsteps;
} TICKS;

typedef struct {
   string  mol;
   double  fre_cal;
   char    fre_unc[9];
   double  fre_obs;
   double  e_lower;
   char    tra[26];
   char    tra_hf[21];
   double  rel_hf;
   double  line_str;
   double  log_aul;
   char    refs[8];
   int     joined;
} SLAIM;

typedef struct {
   int lw, ls, ci, fo, fs;
   double ch;
   Widget *e;
} PSSTY;

typedef struct {
    char  label[14];
    PLFLT inc;
    int   ticks;
    Widget *e;
} PSAXIS;

typedef struct {
   PSAXIS  x, y;
   PSSTY   style;
   Widget *e;
} PSBOX;

typedef struct {
   int     AspectRatio;
   int     Width, Height;
   char    device[80];
   char    send_to_lp[80];
   double  scale;
   int     force_cm_size;
   int     cmAspectRatio;
   double  cmWidth, cmHeight;
   char    x_label[80], y_label[80], t_label[80], w_label[80];
   PSBOX   box;
   PSBOX   subbox;
   PSBOX   TRsubbox;
   PSBOX   wedge;
   PSSTY   marker;
   PSSTY   posmarker;
   PSSTY   label;
   PSSTY   ilabel;
   PSSTY   line;
   PSSTY   secondary;
   PSSTY   zero;
   PSSTY   gauss;
   PSSTY   poly;
   PSSTY   blbox;
   PSSTY   mobox;
   PSSTY   beambox;
   PSSTY   beam;
   PSSTY   header;
} PSDATA;

struct xs_mark {
    int    mode;
    double x;
    double y;
    int    type, pgtype;
    int    dir;
    double xlength, ylength;
    double align;
    double angle;
    int    tagged;
    struct xs_mark *mark;
    string label;
};

typedef struct xs_mark MARK;

typedef struct {
    string     name;
    string     molecule;
    DATE       date;
    MAP       *m1;
    MAP       *m2;
    scan      *s;
    PolyLine  *p;
    char       epoch;
    double     equinox;
    double     x0, y0;
    double     xmin, xmax;
    double     ymin, ymax;
    Frame      frame;
    int        swapped;
    int        single;
    XRectangle r;
    int        xtype, ytype;
    int        fft, nfft;
    DataSetPtr dsp;
    int        nData;
    double    *x;
    double    *y;
    double    *ex;
    double    *ey;
    scanPtr   *sp;
    int       *t;
} scatter;

typedef scatter *scatterPtr;

typedef struct {
    int    mode;
    int    min_x, min_y;
    int    box_w, box_h;
    int    main_w, main_h;
    double scale_x, scale_y;
    int    xunit, yunit;
    int    fixed_x, fixed_y, fixed_z;
    int    autoscale_x, autoscale_y;
    int    use_attached_frame;
    int    Nx, Ny;
    int    subXmarg, subYmarg;
    double subXmagn, subYmagn;
    int    tlab_type, slab_type, llab_type, rlab_type;
    int    join_markers, remove_joint, tag_markers;
    int    tag_scatters, special_view_type;
    double xleft, xright;
    double ylower, yupper;
    double xspacing, yspacing;
    double xrange, yrange;
    double xref, yref;
    string t_label, y_label, x_label, w_label;
    double rig_x, rig_y, lef_x, lef_y;
    string r_label, l_label;
    DataSetPtr from;
    DataSetPtr to;
    scanPtr s, s2;
    MAP     *m;
    scatter *p;
    int     nMaps;
    MAP     **M;
    int     nScat;
    scatter **P;
} VIEW;

typedef struct {
   string sname;
   string molecule;
   int    n, nb, nm, nr, sno, subno, coordType, scanType;
   DATE   date, LST;
   double f0, fn, fres;
   double v0, vres, vlsr;
   double *d;
   double *e;
   double xoff, yoff;
   double aoff, eoff;
   double posang;
   double tsys, tau, int_time;
   double TAir, PAir, RAir;
   BOX    boxar[MAXBOX];
   BOX    regs[MAXBOX];
   MOD    mods[MAXMOD];
   BLINE  bl;
   double coeffs[MAXORDER+1];
   double scoeffs[3];
   int    nmark;
   MARK   marks[MAXMARK];
   int    nX, nY;
   double refX, refY;
   char   epoch;
   double equinox;
   double x0, y0;
   double az, el;
   double xspa, yspa;
   double beameff, firstIF, lofreq, skyfreq;
   int    pol;
   Beam   b;
   double  **data;
   int     **flag;
   scanPtr **sp;
} FDATA;

typedef struct {
  int clear, update;
  int frame, labels, ticks;
  int wframe, wlabels, wticks;
  int data, data_rms, data_sec, wedge, wedgepos, beam, projaxes, projnums;
  int zline, histo, histo_sec, header;
  int markers, boxes, poly, gsum, gind;
  int xebars, yebars;
  int iterate_color;
  int multiple;
} DRAW;

typedef struct {
  int dirty;
  string version;
  string homeDir;
  string dataDir;
  string gaussDir;
  string tmpDir;
  string firstFile;
  string prefsFile;
  string editor;
  string unixSortCmd;
  string unixMvCmd;
  string unixRmCmd;
  string unixCatCmd;
  string printerCmd;
  string slaimDir, slaimFile;
  string lovasDir, lovasFile;
  string jplDir,   jplFile;
  string identDir, identFile;
  string bitpix;
  string unit;
  string xsWidth, xsHeight;
  string psWidth, psHeight;
  string cmWidth, cmHeight;
  string xs_xpm, gauss_xpm, msgs_xpm;
  string zeroLine, polOrder;
  string contMarker, contMarkerSize;
  string scatMarker, scatMarkerSize;
  string mouseButtons;
  string maxMenuItems;
  string limitMegaByte;
  string siteLatitude, siteLongitude;
  string tinyWindow;
} USER;

typedef struct {
  unsigned long   black, white;
  int             swapped;
  Colormap        cmap;
  int             ncols;
  unsigned long  *cols;
  unsigned short *c_r, *c_g, *c_b;
  int             ngreys;
  unsigned long  *greys;
  unsigned short *g_r, *g_g, *g_b;
  int             nfalse;
  unsigned long  *false;
  unsigned short *f_r, *f_g, *f_b;
} COLOR;

typedef struct {
  double Phase_k, Phase_m; /* phase calibration factors counts to degrees   */
  double Ampl_m;           /* Amplitude offset [counts]                     */
  double nDim;             /* array dimension				    */
  double Sampling;         /* sampling distance in arcsec		    */
  double Az, El;           /* Azimuth, elevation of holo transmitter [deg]  */
  double TransFreq;        /* Transmitter frequency  [MHz]		    */
  double TransDist;        /* Trasnmitter distance [m]  		    */
  double Dprim, Dseco;     /* Diameters of primary and secondary [m]	    */
  double Zref, Zseco;      /* Distances to ref. plane and ref. receiver [m] */
  double NyRate;           /* Nyqvist sampling rate                         */
  double Fprim;            /* Focal length of primary [m]                   */
  double Fmag;             /* Cassegrain magnification                      */
  double Defocus;          /* Defocus distance due to nearby transm. [m]    */
  double QuadWidth;        /* The width of the quadrupod legs [m]           */
  double Mask;             /* Use mask if != 0                              */
  double Ro, Ri, Rq;       /* Mask data outside Ro, inside Ri, and within 
                              Rq from the legs                              */
  double p[10], q[10];
  int    fit[10];
  double sigma;
} Holography;

typedef struct {
  double xleft, xright, xspacing;
  double ylower, yupper, yspacing;
  double width;
  double Az, El;
  string poimodel;
} APEXMap;

typedef struct {
  XtAppContext   app_cntxt;
  Widget         top, form, panel, cmd;
  Widget         graph;
  Widget         gaussTop, msgTop;
  Widget        *TCursor, *TGauss, *TBaseline, *TMoment, *TPolyline;
  unsigned int   LeftButton;
  unsigned int   MiddleButton;
  unsigned int   RightButton;
  int            server;
  XImage        *xi;
  Dimension      width, height;
  Pixmap         pm;
  Dimension      p_w, p_h;
  Widget        *hw;
  int            privateColors;
  double         MemoryWarningLimit;
  GC             gcLine, gcText, gcErase, gcGauss, gcGaussI;
  GC             gcStd, gcRms;
  GC             gcBox, gcMom, gcPoly, gcSec, gcGrey, gcTag;
  GC             gcFrame[6];
  GC             gcClear;
  XFontStruct   *font08, *font10, *font12, *font14, *font18, *font24;
  XmFontList     flist10, flist12;
  Holography    *hp;
  APEXMap       *am;
} GLOBAL;

typedef struct {
    Widget fs, form, dialog, label;
    string fmt, file_type;
    int busy, interupt;
    int app_mode, seq_mode;
    int sort1_type, sort2_type, sort_order;
    int (*f)();
    int size;
    XmString *files;
    XmString *descs;
    int      *order;
} XSTR;

typedef struct {
    string s;
    XSTR  *x;
    Widget w;
} fsel_struct;

typedef struct {
  double x1, x2, y1, y2;
  double xleft, xright;
  int xunit;
  int npos;
  int flag;
  int type;
  int posaxis;
  int posaxisdir;
  double width;
  PolyLine *pl;
} VELPOS;
