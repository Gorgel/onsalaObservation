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
#ifndef DRP
#define DRP 1
#define DRP 1

#define DRPVERSION 0xa001

#define VELOCITY   0
#define FREQUENCY  1
#define CHANNELS   2

/* #define MAXCHANNELS 4096 */                  /* maximum number of channels */
#define SCANLEN sizeof(struct scan)       /* scan length in bytes       */
#define MAXDATA MAXCHANNELS*sizeof(float) /* maximium length of data    */
#ifdef __HPUX
#define HEADER (448)                      /* length of header in bytes (HP) */
#else
#define HEADER ((int)(SCANLEN-sizeof(float *)))  /* length of header in bytes  */
#endif

#ifndef MAXNAMLEN
#define MAXNAMLEN 80
#endif

#define C 299792458.0               /* guess what?                       */

struct scan { short                 /*                      byte address */
                     Slength,       /* length of scan in words         0 */
                     NChannel,      /* number of channels              2 */
                     Ctrl,          /* control word                    4 */
                     ScanNo,        /* scan number                     6 */
                     SubScan,       /* subscan number                  8 */
                     Year,          /* year of observation            10 */
                     Month,         /* month of observation           12 */
                     Day,           /* day of observation             14 */
                     UTHour,        /* hour of UTC of observation     16 */
                     UTMin,         /* minute of UTC of observation   18 */
                     UTSec,         /* second of UTC of observation   20 */
                     STHour,        /* hour of LST of observation     22 */
                     STMin,         /* minute of LST of observation   24 */
                     STSec,         /* second of LST of observation   26 */
                     CSystem,       /* coordinate system used 1..6    28 */
                     ObsMode,       /* observing mode                 30 */
                     Backend,       /* backend used                   32 */
                     Frontend;      /* frontend used                  34 */
              short
                     MapX,          /* map grid coordinate in x       36 */
                     MapY;          /* map grid coordinate in y       38 */
              long
                     JulDate;       /* Julian date of observation     40 */
              char
                     Name[12],      /* source name                    44 */
                     Project[4],    /* project identification         56 */
                     Observer[16],  /* name of observer               60 */
                     Program[16],   /* name of program                76 */
                     Molecule[18];  /* name of molecule               92 */
#ifdef AMIGA
              short  junk1;
#endif
              float  
                     AirTemp,       /* ambient air temperature in K    112 */
                     Pressure,      /* ambient air pressure in Torr    116 */
                     Humidity,      /* relative air humidity in %      120 */
                     Equinox,       /* equinox of coordinate system    124 */
                     EquiNow,       /* current equinox                 128 */
                     Longitude,     /* descriptive longitude in radian 132 */
                     Latitude,      /* descriptive latitude in radian  136 */
                     Long2000,      /* longitude w.r.t. equinox 2000.0 140 */
                     Lat2000,       /* latitude w.r.t. equinox 2000.0  144 */
                     LMapOff,       /* descriptive offset in long.     148 */
                     BMapOff,       /* descriptive offset in lat.      152 */
                     Azimuth,       /* azimuth in radian               156 */
                     Elevation,     /* elevation in radian             160 */
                     AzOffset,      /* offset in azimuth               164 */
                     ElOffset,      /* offset in azimuth               168 */
                     AzMapOff,      /* map offset in azimuth           172 */
                     ElMapOff,      /* map offset in azimuth           176 */
                     AzPointg,      /* pointing correction in az.      180 */
                     ElPointg,      /* pointing correction in el.      184 */
                     AzErrAve,      /* average tracking error in az.   188 */
                     ElErrAve,      /* average tracking error in el.   192 */
                     AzErrRms,      /* rms tracking error in az.       196 */
                     ElErrRms,      /* rms tracking error in el.       200 */
                     GalLong,       /* galactic longitude              204 */
                     GalLat,        /* galactic latitude               208 */
                     VHel,          /* observer's velocity w.r.t. sun  212 */
                     VLsr,          /* observer's velocity w.r.t. LSR  216 */
                     Axial,         /* subreflector axial              220 */
                     Shift,         /* subreflector shift              224 */
                     VTilt,         /* subreflector vertical tilt      228 */
                     HTilt,         /* subreflector horizontal tilt    232 */
                     Tcal,          /* calbration temperature in K     236 */
                     Tsys,          /* system temperature in K         240 */
                     Trec,          /* receiver temperature            244 */
                     Tau,           /* atmospheric attenuation         248 */
                     dBl,           /* load/sky ratio in dB            252 */
                     IntTime,       /* integration time in seconds     256 */
                     RefCorr,       /* refraction correction           260 */
                     ParAngle,      /* parallactic angle               264 */
                     PosAngle,      /* position angle of map           268 */
                     StepX,         /* map grid spacing in x           272 */
                     StepY;         /* map grid spacing in y           276 */
              double
                     Bandwidth,     /* bandwidth in MHz                280 */
                     RestFreq,      /* rest frequency in MHz           288 */
                     SkyFreq,       /* sky frequency in MHz            296 */ 
                     FirstIF,       /* 1.IF in MHz                     304 */ 
                     FreqThrow,     /* frequency throw in MHz          312 */
                     FreqRes,       /* frequency resolution in MHz     320 */
                     VSource,       /* source velocity in km/s         328 */
                     VelRes;        /* velocity resolution in km/s     336 */
              float  work[10];      /* internal work area              344 */
              short  flag[31];      /* internal flags                  384 */
#ifdef AMIGA
              short  junk2;
#endif
              /* float  Channels[MAXCHANNELS]; */             /*       448 */
              float  *c;                                      /*       448 */
};

typedef struct scan SCAN;

/* This is an extension of the DRP scan variables, i.e. not saved, only for
 * internal use.
 */
struct xscan {
              SCAN   *s;            /* Normal scan pointer                 */
	      long
	             NChannel;      /* To be used for large no of channels */
              double
                     LOFreq;        /* LO frequency in MHz                 */
};

typedef struct xscan XSCAN;

void GetScan(SCAN *);               /* get scan from CWORK            */
void PutScan(SCAN *);               /* put scan to CWORK              */
void GetAve(SCAN *);                /* get scan from CHOLD            */
void PutAve(SCAN *);                /* put scan to CWORK              */
void GetTemp(SCAN *);               /* get scan from CTEMP            */
void PutTemp(SCAN *);               /* put scan to CTEMP              */

int CenterCh(SCAN *, XSCAN *);               /* calculate center channel       */
double Velocity(int , SCAN *, XSCAN *);      /* velocity at given channel      */
double Frequency(int , SCAN *, XSCAN *);     /* frequency at given channel     */
int FChannel(double , SCAN *);      /* channel number for velocity    */
int VChannel(double , SCAN *);      /* channel number for frequency   */

void DRPerror(char *, ...);
void DRPwarning(char *, ...);
void DRPinfo(char *, ...);

#endif
