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
#define PKGNAME       "XS"
#define PRGNAME       "xs"
#define XS_VERSION    1.2
#define XS_PATCH      18
#define XS_FULLVER    (XS_VERSION + XS_PATCH/1000.0)
/* #define XS_BETA        'b' */
#define XS_VERDATE    "Nov 22, 2012"

#define USE_PIXMAP_STORAGE

#define MAXMARK       500
#define MAXGAUSS       50
#define MAXORDER       14
#define MAXBOX         30
#define MAXMOD         30
#define MAXSTRINGSIZE 150
#define MAXBUFSIZE    500
#define MAXCONTOURS    30

#define WAITFILES      50
#define WAITSPECTRA   100

#define MILLISEC      200

#define SPEEDOFLIGHT  299792.458
#define INVCMTOKELVIN 1.43875

#ifndef M_LN2
#define M_LN2         0.69314718055994530942
#endif
#ifndef M_PI
#define M_PI          3.14159265358979323846
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI    1.12837916709551257390
#endif

#ifdef HAS_NOT_RINT
#define NINT(x)       (((x) >= 0.0) ? (int)((x) + 0.5) : -(int)(-(x) + 0.5))
#else
#define NINT(x)       ((int)rint(x))
#endif

#define ALPHA         (4.0 * M_LN2)      /* 4*log(2)       */
#define SQALPHA       sqrt(ALPHA)        /* sqrt(4*log(2)) */

#ifndef PI
#define PI            M_PI
#endif

#define GAUSSNORM     (ALPHA / PI)

#define RADTODEG      (180.0/PI)
#define RADTOMIN      (60.0*180.0/PI)
#define RADTOSEC      (3600.0*180.0/PI)
#define RADTOHR       (12.0/PI)

#define TRUE          1
#define FALSE         0
#define EPSILON       1.e-5
#define UNDEF         (-32768)

#define UNBLANK       0
#define BLANK         -1
#define BLANK_TMP     -2

#define EMPTY         0
#define FILLED        1

#define SEQ_NSPEC     5

#define DEF_XUNIT     "Frequency"
#define DEF_XLABEL    "Frequency [GHz]"
#define DEF_YLABEL    "TA* [K]"
#define DEF_TLABEL    "<none>"

#define SHOW_SPE      0
#define SHOW_ALLSPE   1
#define SHOW_SUBSPE   2
#define SHOW_POSPOS   3
#define SHOW_VELPOS   4
#define SHOW_POSVEL   5
#define SHOW_SCATTER  6
#define SHOW_SUBMAP   7
#define SHOW_WEDGE    8

#define SCALE_NONE        0
#define SCALE_ONLY_X      1
#define SCALE_ONLY_Y      2
#define SCALE_BOTH        3

#define NO_REDRAW     0
#define REDRAW        1

#define UNIT_FRE      0
#define UNIT_VEL      1
#define UNIT_CHA      2
#define UNIT_FOFF     3
#define UNIT_ASEC     4
#define UNIT_AMIN     5
#define UNIT_FMHZ     6

#define FITS_VECTOR   0
#define FITS_ARRAY    1
#define FITS_CUBE     2

#define MAP_POSPOS    0
#define MAP_VELPOS    1
#define MAP_POSVEL    2

#define COORD_TYPE_UNKNOWN -1
#define COORD_TYPE_EQU      0
#define COORD_TYPE_HOR      1
#define COORD_TYPE_GAL      2
#define COORD_TYPE_SCAN     3

#define BOX_RMS       0
#define BOX_MOM       1

#define DOT_NOMARK        0
#define DOT_SQUAREMARK    1
#define DOT_CIRCLEMARK    2
#define DOT_VALUEMARK     3
#define DOT_CROSSMARK     4
#define DOT_PLUSMARK      5

#define SHADE_NONE  0
#define SHADE_GREY  1
#define SHADE_FALSE 2

#define TLAB_NONE          0
#define TLAB_SOURCE        1
#define TLAB_MOLECULE      2
#define TLAB_DATE          3
#define TLAB_TIME          4
#define TLAB_POSITION      5
#define TLAB_INTINT        6
#define TLAB_SCANNO        7
#define TLAB_RESTFQ        8
#define TLAB_GAUSS         9

#define XTYPE_SCA_NO       0
#define XTYPE_SCA_SCAN     1
#define XTYPE_SCA_RA       2
#define XTYPE_SCA_DEC      3
#define XTYPE_SCA_DIST     4
#define XTYPE_SCA_POSANG   5
#define XTYPE_SCA_AZ       6
#define XTYPE_SCA_EL       7
#define XTYPE_SCA_AZOFF    8
#define XTYPE_SCA_ELOFF    9
#define XTYPE_SCA_EQDIST   10
#define XTYPE_SCA_INT      11
#define XTYPE_SCA_MEAN     12
#define XTYPE_SCA_SIGMA    13
#define XTYPE_SCA_TSYS     14
#define XTYPE_SCA_DATE     15
#define XTYPE_SCA_JD       16
#define XTYPE_SCA_MJD      17
#define XTYPE_SCA_GAMP     18
#define XTYPE_SCA_GCEN     19
#define XTYPE_SCA_GWID     20
#define XTYPE_SCA_POL0     21
#define XTYPE_SCA_POL1     22
#define XTYPE_SCA_VCENT    23
#define XTYPE_SCA_V2MOM    24
#define XTYPE_SCA_VELO     25
#define XTYPE_SCA_FREQ     26
#define XTYPE_SCA_CHAN     27
#define XTYPE_SCA_TEMP     28
#define XTYPE_SCA_FOFF     29
#define XTYPE_SCA_TMAX     30
#define XTYPE_SCA_TMIN     31
#define XTYPE_SCA_PDIST    32
#define XTYPE_SCA_CORR     33
#define XTYPE_SCA_POLA_EL  34
#define XTYPE_SCA_COSPA    35
#define XTYPE_SCA_OSOAZ    36
#define XTYPE_SCA_BEFF     37
#define XTYPE_SCA_AIRMASS  38
#define XTYPE_SCA_TAU      39
#define XTYPE_SCA_UT       40
#define XTYPE_SCA_FMHZ     41
#define XTYPE_SCA_RECT     42
#define XTYPE_SCA_DECL     43
#define XTYPE_SCA_VRES     44
#define XTYPE_SCA_FRES     45
#define XTYPE_SCA_TIME     46
#define XTYPE_SCA_TSQRT    47
#define XTYPE_SCA_POLA_AZ  48
#define XTYPE_SCA_TAIR     49
#define XTYPE_SCA_PAIR     50
#define XTYPE_SCA_RAIR     51
#define XTYPE_SCA_EXPTAU   52

#define YTYPE_SCA_NO       0
#define YTYPE_SCA_SCAN     1
#define YTYPE_SCA_RA       2
#define YTYPE_SCA_DEC      3
#define YTYPE_SCA_DIST     4
#define YTYPE_SCA_POSANG   5
#define YTYPE_SCA_AZ       6
#define YTYPE_SCA_EL       7
#define YTYPE_SCA_AZOFF    8
#define YTYPE_SCA_ELOFF    9
#define YTYPE_SCA_EQDIST   10
#define YTYPE_SCA_INT      11
#define YTYPE_SCA_MEAN     12
#define YTYPE_SCA_SIGMA    13
#define YTYPE_SCA_TSYS     14
#define YTYPE_SCA_DATE     15
#define YTYPE_SCA_JD       16
#define YTYPE_SCA_MJD      17
#define YTYPE_SCA_GAMP     18
#define YTYPE_SCA_GCEN     19
#define YTYPE_SCA_GWID     20
#define YTYPE_SCA_POL0     21
#define YTYPE_SCA_POL1     22
#define YTYPE_SCA_VCENT    23
#define YTYPE_SCA_V2MOM    24
#define YTYPE_SCA_VELO     25
#define YTYPE_SCA_FREQ     26
#define YTYPE_SCA_CHAN     27
#define YTYPE_SCA_TEMP     28
#define YTYPE_SCA_FOFF     29
#define YTYPE_SCA_TMAX     30
#define YTYPE_SCA_TMIN     31
#define YTYPE_SCA_PDIST    32
#define YTYPE_SCA_CORR     33
#define YTYPE_SCA_POLA_EL  34
#define YTYPE_SCA_COSPA    35
#define YTYPE_SCA_OSOAZ    36
#define YTYPE_SCA_BEFF     37
#define YTYPE_SCA_AIRMASS  38
#define YTYPE_SCA_TAU      39
#define YTYPE_SCA_UT       40
#define YTYPE_SCA_FMHZ     41
#define YTYPE_SCA_RECT     42
#define YTYPE_SCA_DECL     43
#define YTYPE_SCA_VRES     44
#define YTYPE_SCA_FRES     45
#define YTYPE_SCA_TIME     46
#define YTYPE_SCA_TSQRT    47
#define YTYPE_SCA_POLA_AZ  48
#define YTYPE_SCA_TAIR     49
#define YTYPE_SCA_PAIR     50
#define YTYPE_SCA_RAIR     51
#define YTYPE_SCA_EXPTAU   52

#define ZTYPE_MOMENT       0
#define ZTYPE_MEAN         1
#define ZTYPE_GAMP         2
#define ZTYPE_GCEN         3
#define ZTYPE_GWID         4
#define ZTYPE_POL0         5
#define ZTYPE_POL1         6
#define ZTYPE_VCENT        7
#define ZTYPE_V2MOM        8
#define ZTYPE_TMAX         9
#define ZTYPE_TMIN        10
#define ZTYPE_XTMAX       11
#define ZTYPE_XTMIN       12
#define ZTYPE_TRMS        13

#define VIEW_NONE         0
#define VIEW_TOP_RIGHT    1
#define VIEW_TOP_LEFT     2
#define VIEW_BOTTOM_LEFT  3
#define VIEW_BOTTOM_RIGHT 4

#define MARK_TYPE_ARROW  0
#define MARK_TYPE_LINE   1
#define MARK_TYPE_SQUARE 2
#define MARK_TYPE_CIRCLE 3
#define MARK_TYPE_NONE   4
#define MARK_TYPE_PGPLOT 5
#define MARK_TYPE_ALINE  6

#define MARK_DIR_DOWN  0
#define MARK_DIR_UP    1
#define MARK_DIR_LEFT  2
#define MARK_DIR_RIGHT 3

#define MAP_BEAM_NO    0
#define MAP_BEAM_LL    1
#define MAP_BEAM_LR    2
#define MAP_BEAM_UL    3
#define MAP_BEAM_UR    4

#define POS_RIGHT      0
#define POS_LEFT       1
#define POS_ABOVE      2
#define POS_BELOW      3

#define POL_UNKNOWN    0
#define POL_LCP        1
#define POL_RCP        2
#define POL_BOTH       3

#define AVETYPE_NONE   0
#define AVETYPE_RMS    1
#define AVETYPE_TSYS   2
#define AVETYPE_TIME   3
#define AVETYPE_SAME   4
#define AVETYPE_IRMS   5
#define AVETYPE_ADDRMS 6

#define SORT_TYPE_NONE   -1
#define SORT_TYPE_SCAN    0
#define SORT_TYPE_NCHA    1
#define SORT_TYPE_FREQ   10
#define SORT_TYPE_VLSR   11
#define SORT_TYPE_TIME   12
#define SORT_TYPE_DATE   13
#define SORT_TYPE_RA     14
#define SORT_TYPE_DEC    15
#define SORT_TYPE_RAOFF  16
#define SORT_TYPE_DEOFF  17
#define SORT_TYPE_DIST   18
#define SORT_TYPE_NAME   30
#define SORT_TYPE_MOLE   31

#define SORT_INCREASING 0
#define SORT_DECREASING 1

#if XmVersion >= 2000
#define MKSTRING(s)  XmStringGenerate((s), NULL, XmCHARSET_TEXT, NULL)
#define MKFLIST(f)   XmFontListAppendEntry(NULL, XmFontListEntryCreate(\
                         XmFONTLIST_DEFAULT_TAG, XmFONT_IS_FONT, (f)))
#else
#define MKSTRING(s)  XmStringCreateLtoR((s), XmSTRING_DEFAULT_CHARSET)
#define MKFLIST(f)   XmFontListCreate((f), XmSTRING_DEFAULT_CHARSET)
#endif

#define LEFT_BUTTON    1
#define MIDDLE_BUTTON  2
#define RIGHT_BUTTON   3


#define BUTT_CANCEL		"dismiss"
#define BUTT_APPLY      "apply"
#define BUTT_HELP       "help"

#define BROWSE_NONE    0
#define BROWSE_FILE    1
#define BROWSE_DIR     2

#ifndef NO_PGPLOT
#define HAVE_LIBPGPLOT
#endif
