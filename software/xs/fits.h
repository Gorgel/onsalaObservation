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
#define LOGRECLEN     80
#define PHYSRECLEN  2880
#define KEYWORDLEN     8
#define MAXVARLEN     80
#define UPTOCOMMENT   31
#define COMMENTLEN    47

#define NVARTYPE 0
#define BOOLTYPE 1
#define CHARTYPE 2
#define LONGTYPE 3
#define REALTYPE 5
#define CPLXTYPE 6

#define MAXNAXIS 5

#define FITS_TYPE_UNKNOWN  0
#define FITS_TYPE_8BIT     8
#define FITS_TYPE_16BIT   16
#define FITS_TYPE_32BIT   32
#define FITS_TYPE_FLOAT  -32
#define FITS_TYPE_64BIT   64
#define FITS_TYPE_DOUBLE -64

/* #define STRTOD(s) atof(s) */
#define STRTOD(s) strtod(s, NULL)

struct fitskey {
    int hit;
    char kw[KEYWORDLEN+1];
    int vartyp;
    union {
	    char str[17];
	    long l;
	    double d;
	    double c[2];
    } val;
    char c[COMMENTLEN+1];
};

typedef struct fitskey fkey;

enum kw_known {
    KW_SIMPLE = 0, 
    KW_BITPIX,
    KW_NAXIS, 
    KW_NAXIS1,
    KW_NAXIS2,
    KW_NAXIS3,
    KW_NAXIS4,
    KW_NAXIS5,
    KW_CTYPE1,
    KW_CRPIX1,
    KW_CRVAL1,
    KW_CDELT1,
    KW_CROTA1,
    KW_CTYPE2,
    KW_CRPIX2,
    KW_CRVAL2,
    KW_CDELT2,
    KW_CROTA2,
    KW_CTYPE3,
    KW_CRPIX3,
    KW_CRVAL3,
    KW_CDELT3,
    KW_CROTA3,
    KW_CTYPE4,
    KW_CRPIX4,
    KW_CRVAL4,
    KW_CDELT4,
    KW_CROTA4,
    KW_CTYPE5,
    KW_CRPIX5,
    KW_CRVAL5,
    KW_CDELT5,
    KW_CROTA5,
    KW_BSCALE,
    KW_BZERO,
    KW_BUNIT,
    KW_BLANK,
    KW_DATAMAX,
    KW_DATAMIN,
    KW_ORIGIN,
    KW_OBSERVER,
    KW_OBJECT,
    KW_POLARIZA,
    KW_INSTRUME,
    KW_TELESCOP,
    KW_LINE,
    KW_SCAN,
    KW_UTC,
    KW_UT,
    KW_LST,
    KW_TIMESYS,
    KW_DATE,
    KW_DATE_OBS,
    KW_JDATE,
    KW_TSYS,
    KW_TLOAD,
    KW_TCAL,
    KW_TREC,
    KW_TAU,
    KW_DBLOAD,
    KW_EPOCH,
    KW_EQUINOX,
    KW_RA,
    KW_DEC,
    KW_AZIMUTH,
    KW_ELEVATIO,
    KW_AZOFF,
    KW_ELOFF,
    KW_AZPOINT,
    KW_ELPOINT,
    KW_AZCORR,
    KW_ELCORR,
    KW_COLLIMAT,
    KW_BMAJ,
    KW_BMIN,
    KW_BPA,
    KW_RESTFREQ,
    KW_OBSFREQ,
    KW_IMAGFREQ,
    KW_VLSR,
    KW_VELO_LSR,
    KW_VHEL,
    KW_VELO_HEL,
    KW_VELO_GEO,
    KW_DELTAV,
    KW_OBSTIME,
    KW_TOUTSIDE,
    KW_PRESSURE,
    KW_HUMIDITY,
    KW_COMMENT,
    KW_HISTORY,
    KW_WHITE,
    KW_EXTEND,
    KW_XTENSION,
    KW_PCOUNT,
    KW_GCOUNT,
    KW_TFIELDS,
    KW_EXTNAME,
    KW_EXTVER,
    KW_NMATRIX,
    KW_MAXIS,
    KW_MAPTILT,
    KW_BLOCKED,
    KW_TTYPE,
    KW_TFORM,
    KW_TUNIT,
    KW_IRAF_MAX,
    KW_IRAF_MIN,
    KW_IRAF_BPX,
    KW_BEAMEFF,
    KW_CD11,
    KW_CD12,
    KW_CD21,
    KW_CD22,
    KW_CD33,
    KW_CUNIT3,
    KW_OBSMODE,
    KW_OBSTYPE,
    KW_ORBIT,
    KW_END
};

struct formkey {
  int cnt;        /* T number            */
  int format;     /* character I J D E A */
  int bytes;      /* size in bytes of each entry */
  int n;          /* No of entries       */
  int nmap;       /* index to mapping struct */
  char type[17]; 
  char unit[17]; 
};

typedef struct formkey field;

#ifdef FITSKNOWN
/*                                              1         2         3         4        */
/*                                    012345678901234567890123456789012345678901234567 */
struct fitskey known[] = {
    { 0, "SIMPLE  ", BOOLTYPE, {""}, "Basic FITS format"},
    { 0, "BITPIX  ", LONGTYPE, {""}, "Number of bits used for pixel value"},
    { 0, "NAXIS   ", LONGTYPE, {""}, "Number of axes"},
    { 0, "NAXIS1  ", LONGTYPE, {""}, "Number of data points on first axis"},
    { 0, "NAXIS2  ", LONGTYPE, {""}, "Number of data points on second axis"},
    { 0, "NAXIS3  ", LONGTYPE, {""}, "Number of data points on third axis"},
    { 0, "NAXIS4  ", LONGTYPE, {""}, "Number of data points on fourth axis"},
    { 0, "NAXIS5  ", LONGTYPE, {""}, "Number of data points on fifth axis"},
    { 0, "CTYPE1  ", CHARTYPE, {""}, ""},
    { 0, "CRPIX1  ", REALTYPE, {""}, ""},
    { 0, "CRVAL1  ", REALTYPE, {""}, ""},
    { 0, "CDELT1  ", REALTYPE, {""}, ""},
    { 0, "CROTA1  ", REALTYPE, {""}, ""},
    { 0, "CTYPE2  ", CHARTYPE, {""}, ""},
    { 0, "CRPIX2  ", REALTYPE, {""}, ""},
    { 0, "CRVAL2  ", REALTYPE, {""}, ""},
    { 0, "CDELT2  ", REALTYPE, {""}, ""},
    { 0, "CROTA2  ", REALTYPE, {""}, ""},
    { 0, "CTYPE3  ", CHARTYPE, {""}, ""},
    { 0, "CRPIX3  ", REALTYPE, {""}, ""},
    { 0, "CRVAL3  ", REALTYPE, {""}, ""},
    { 0, "CDELT3  ", REALTYPE, {""}, ""},
    { 0, "CROTA3  ", REALTYPE, {""}, ""},
    { 0, "CTYPE4  ", CHARTYPE, {""}, ""},
    { 0, "CRPIX4  ", REALTYPE, {""}, ""},
    { 0, "CRVAL4  ", REALTYPE, {""}, ""},
    { 0, "CDELT4  ", REALTYPE, {""}, ""},
    { 0, "CROTA4  ", REALTYPE, {""}, ""},
    { 0, "CTYPE5  ", CHARTYPE, {""}, ""},
    { 0, "CRPIX5  ", REALTYPE, {""}, ""},
    { 0, "CRVAL5  ", REALTYPE, {""}, ""},
    { 0, "CDELT5  ", REALTYPE, {""}, ""},
    { 0, "CROTA5  ", REALTYPE, {""}, ""},
    { 0, "BSCALE  ", REALTYPE, {""}, "real = value * BSCALE + BZERO"},
    { 0, "BZERO   ", REALTYPE, {""}, "real = value * BSCALE + BZERO"},
    { 0, "BUNIT   ", CHARTYPE, {""}, "Intensity unit"},
    { 0, "BLANK   ", LONGTYPE, {""}, "Value indicating no data point"},
    { 0, "DATAMAX ", REALTYPE, {""}, "Maximum data value"},
    { 0, "DATAMIN ", REALTYPE, {""}, "Minimum data value"},
    { 0, "ORIGIN  ", CHARTYPE, {""}, "Origin of data"},
    { 0, "OBSERVER", CHARTYPE, {""}, "Observer"},
    { 0, "OBJECT  ", CHARTYPE, {""}, "Object (or source name)"},
    { 0, "POLARIZA", CHARTYPE, {""}, "LCP/RCP/LCP+RCP"},
    { 0, "INSTRUME", CHARTYPE, {""}, "Type of instrument"},
    { 0, "TELESCOP", CHARTYPE, {""}, "Data are taken with this telescope"},
    { 0, "LINE    ", CHARTYPE, {""}, "Molecular line designation"},
    { 0, "SCAN-NUM", LONGTYPE, {""}, "Scan number"},
    { 0, "UTC     ", CHARTYPE, {""}, "Universal time coordinated (at start of obs.)"},
    { 0, "UT      ", CHARTYPE, {""}, "Universal time (at start of obs.)"},
    { 0, "LST     ", CHARTYPE, {""}, "Local sidereal time (at start of obs.)"},
    { 0, "TIMESYS ", CHARTYPE, {""}, "Time system (new keyword)"},
    { 0, "DATE    ", CHARTYPE, {""}, "Date of file (?)"},
    { 0, "DATE-OBS", CHARTYPE, {""}, "Date when observation started"},
    { 0, "JDATE   ", REALTYPE, {""}, "Julian date (at start of obs.)"},
    { 0, "TSYS    ", REALTYPE, {""}, "System temperature (Kelvin)"},
    { 0, "TLOAD   ", REALTYPE, {""}, "Load temperature (K)"},
    { 0, "TCAL    ", REALTYPE, {""}, "Calibration temp. (K)"},
    { 0, "TREC    ", REALTYPE, {""}, "Receiver temperature (K)"},
    { 0, "TAU-ATM ", REALTYPE, {""}, "Zenith atmospheric opacity (neper)"},
    { 0, "DBLOAD  ", REALTYPE, {""}, "Load attenuation (dB)"},
    { 0, "EPOCH   ", REALTYPE, {""}, "Epoch of observation"},
    { 0, "EQUINOX ", REALTYPE, {""}, "Equinox of coordinates"},
    { 0, "RA      ", REALTYPE, {""}, "Apparent RA"},
    { 0, "DEC     ", REALTYPE, {""}, "Apparent decl."},
    { 0, "AZIMUTH ", REALTYPE, {""}, "Azimuth"},
    { 0, "ELEVATIO", REALTYPE, {""}, "Elevation"},
    { 0, "AZOFF   ", REALTYPE, {""}, ""},
    { 0, "ELOFF   ", REALTYPE, {""}, ""},
    { 0, "AZPOINT ", REALTYPE, {""}, ""},
    { 0, "ELPOINT ", REALTYPE, {""}, ""},
    { 0, "AZCORR  ", REALTYPE, {""}, ""},
    { 0, "ELCORR  ", REALTYPE, {""}, ""},
    { 0, "COLLIMAT", REALTYPE, {""}, ""},
    { 0, "BMAJ    ", REALTYPE, {""}, "Major axis beam"},
    { 0, "BMIN    ", REALTYPE, {""}, "Minor axis beam"},
    { 0, "BPA     ", REALTYPE, {""}, "Position angle of elliptic beam"},
    { 0, "RESTFREQ", REALTYPE, {""}, "Rest frequency (Hz)"},
    { 0, "OBSFREQ ", REALTYPE, {""}, "Sky frequency (Hz)"},
    { 0, "IMAGFREQ", REALTYPE, {""}, "Image frequency (Hz)"},
    { 0, "VLSR    ", REALTYPE, {""}, "Velocity"},
    { 0, "VELO-LSR", REALTYPE, {""}, "Velocity - Local Standard of Rest"},
    { 0, "VHEL    ", REALTYPE, {""}, "Velocity - Heliocentric"},
    { 0, "VELO-HEL", REALTYPE, {""}, "Velocity - Heliocentric"},
    { 0, "VELO-GEO", REALTYPE, {""}, "Velocity - Geocentric"},
    { 0, "DELTAV  ", REALTYPE, {""}, "Velocity resolution"},
    { 0, "OBSTIME ", REALTYPE, {""}, "Integration time"},
    { 0, "TAMB    ", REALTYPE, {""}, "Ambient temperature"},
    { 0, "PRESSURE", REALTYPE, {""}, "Ambient pressure"},
    { 0, "HUMIDITY", REALTYPE, {""}, "Ambient humidity"},
    { 0, "COMMENT ", NVARTYPE, {""}, ""},
    { 0, "HISTORY ", NVARTYPE, {""}, ""},
    { 0, "        ", NVARTYPE, {""}, ""},
    { 0, "EXTEND  ", BOOLTYPE, {""}, ""},
    { 0, "XTENSION", CHARTYPE, {""}, ""},
    { 0, "PCOUNT  ", LONGTYPE, {""}, ""},
    { 0, "GCOUNT  ", LONGTYPE, {""}, ""},
    { 0, "TFIELDS ", LONGTYPE, {""}, ""},
    { 0, "EXTNAME ", CHARTYPE, {""}, ""},
    { 0, "EXTVER  ", LONGTYPE, {""}, ""},
    { 0, "NMATRIX ", LONGTYPE, {""}, ""},
    { 0, "MAXIS   ", LONGTYPE, {""}, ""},
    { 0, "MAPTILT ", REALTYPE, {""}, ""},
    { 0, "BLOCKED ", BOOLTYPE, {""}, ""},
    { 0, "TTYPE   ", CHARTYPE, {""}, ""},
    { 0, "TFORM   ", CHARTYPE, {""}, ""},
    { 0, "TUNIT   ", CHARTYPE, {""}, ""},
    { 0, "IRAF-MAX", REALTYPE, {""}, ""},
    { 0, "IRAF-MIN", REALTYPE, {""}, ""},
    { 0, "IRAF-BPX", LONGTYPE, {""}, ""},
    { 0, "BEAMEFF ", REALTYPE, {""}, "Antenna main beam efficiency"},
    { 0, "CD1_1   ", REALTYPE, {""}, ""},
    { 0, "CD1_2   ", REALTYPE, {""}, ""},
    { 0, "CD2_1   ", REALTYPE, {""}, ""},
    { 0, "CD2_2   ", REALTYPE, {""}, ""},
    { 0, "CD3_3   ", REALTYPE, {""}, ""},
    { 0, "CUNIT3  ", CHARTYPE, {""}, ""},
    { 0, "OBSMODE ", CHARTYPE, {""}, ""},
    { 0, "OBSTYPE ", CHARTYPE, {""}, ""},
    { 0, "ORBIT   ", REALTYPE, {""}, "Orbit number"},
    { 0, "END     ", NVARTYPE, {""}, ""}
};
#else
extern struct fitskey *known;
#endif

double swap2bytes(short int);
double swap4bytes(long int);
double swapf(float);
double swapd(double);

/* 
   Macros to get swapped values. They turn into NOPs if swapping is not
   needed. Some FITS data, notably SEST On-the-fly mapping binary tables
   are not swapped to FITS standard. For those we define the "anti"-functions
 */

#ifdef BYTESWAP

#define GET1(x) (x)
#define GET2(x) (swap2bytes(x))
#define GET4(x) (swap4bytes(x))
#define GETF(x) (swapf(x))
#define GETD(x) (swapd(x))

#define AGET1(x) (x)
#define AGET2(x) (x)
#define AGET4(x) (x)
#define AGETF(x) (x)
#define AGETD(x) (x)

#else

#define GET1(x) (x)
#define GET2(x) (x)
#define GET4(x) (x)
#define GETF(x) (x)
#define GETD(x) (x)

#define AGET1(x) (x)
#define AGET2(x) (swap2bytes(x))
#define AGET4(x) (swap4bytes(x))
#define AGETF(x) (swapf(x))
#define AGETD(x) (swapd(x))

#endif
