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
#include <time.h>
#include <math.h>
#ifdef POSIX
#define _INCLUDE_POSIX_SOURCE
#endif
#include <unistd.h>

#include <Xm/Xm.h>

#include "defines.h"
#include "global_structs.h"

static int MonthDays[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304,
                            334};

void XS_system(const char *cmd, int notify)
{
    int error;
    string buf;
    
    void send_line();
    
    if (notify) {
        sprintf(buf, "Doing system cmd: %s", cmd);
        send_line(buf);
    }
    
    error = system(cmd);
}

char *XS_getlogin()
{
    static string user;
    char *e = getlogin();
    
    char *XS_getenv();
    
    if (!e) {
        e = XS_getenv("USER");
        if (!e) {
            fprintf(stderr, "Error: Couldn't determine user.\n");
            exit(0);
        }
    }
    
    strcpy(user, e);
    
    return user;
}

char *XS_getenv(const char *env)
{
    static string envstring;
    char *e;
    
    if (!env) return NULL;
    
    e = getenv(env);
    
    if (!e) return NULL;
    
    strcpy(envstring, e);
    
    return envstring;
}

char *GetHome()
{
    static string home;
    char *e;
    
    e = XS_getenv("HOME");
    
    if (!e) return NULL;
    
    strcpy(home, e);
    
    return home;
}

int XS_getuid()
{
    uid_t u;
    
    u = getuid();
    
    return (int)u;
}

int XS_getpid()
{
    pid_t p;
    
    p = getpid();
    
    return (int)p;
}

int IsLeapYear(int year)
{
    if (year % 4 != 0) return 0;
    
    if (year % 400 == 0) return 1;
    
    if (year % 100 == 0) return 0;
    
    return 1;
}

static long int DaysSince1900(DATE *d)
{
    int n;
    long int days=0;
    
    for (n=1900; n<d->Year; n++) {
        days += 365;
        if (IsLeapYear(n)) days++;
    }
    
    days += MonthDays[d->Month-1];
    
    if (d->Month >= 3 && IsLeapYear(d->Year)) days++;
    
    days += d->Day;
    
    return days;
}

int DayOfYear(DATE *d)
{
    int days=0;
        
    days += MonthDays[d->Month-1];
    
    if (d->Month >= 3 && IsLeapYear(d->Year)) days++;
    
    days += d->Day;
    
    return days;
}

double JulianDay(DATE *d)
{
    double jd = 0.0;
    
    if (d) {
        jd = 2415019.5;
        jd += (double)DaysSince1900(d);
	    jd += (double)d->Hour/24.0;
	    jd += (double)d->Min/24.0/60.0;
	    jd += d->Sec/24.0/3600.0;
    }
    
    return jd; 
}

double ModifiedJulianDay(DATE *d)
{
    double mjd = 0.0;
    
    if (d) {
	    mjd = 15019.0;
	    mjd += (double)DaysSince1900(d);
	    mjd += (double)d->Hour/24.0;
	    mjd += (double)d->Min/24.0/60.0;
	    mjd += d->Sec/24.0/3600.0;
    }
    
    return mjd; 
}

void ParseMJD(int MJD, short int *y, short int *m, short int *d)
{
    int n, d1900, days, pdays, doy, yr, mo=0, da=0;
    
    d1900 = MJD - 15019;
    
    n = 1900;
    
    days = 0;
    do {
        doy = d1900 - days;
        days += 365;
        if (IsLeapYear(n)) days++;
        n++;
    } while (d1900 > days);
    
    yr = n-1;
    
    if (y) *y = yr;
    
    pdays = 0;
    for (n=0; n<12; n++) {
        days = MonthDays[n];
        if (n >= 2 && IsLeapYear(yr)) days++;
        if (days > doy) {
            mo = n;
            da = doy - pdays;
            break;
        }
        pdays = days;
    }
    
    if (m) *m = mo;
    if (d) *d = da;
}

char *GetDateStr(DATE *d)
{
    static string s;
    
    sprintf(s, "%4d/%02d/%02d %02d:%02d:%02d",
            d->Year, d->Month, d->Day, d->Hour, d->Min, d->Sec);
    
    return s;
}

DATE *XS_localtime()
{
    struct tm *now;
    time_t clock;
    static DATE d;

    time(&clock);
    now = localtime(&clock);
    
    d.Year  = now->tm_year + 1900;
    d.Month = now->tm_mon + 1;
    d.Day   = now->tm_mday;
    
    d.Hour  = now->tm_hour;
    d.Min   = now->tm_min;
    d.Sec   = now->tm_sec;
    
    return &d;
}

void XS_RndInit(unsigned int seed)
{
    srand(seed);
}

void XS_RndInitClock()
{
    DATE *date = XS_localtime();
    
    XS_RndInit(date->Sec + date->Min*60 + date->Hour*3600);
}

double XS_UniformRnd(double min, double range)
{
    return (min + range * (double)rand() / (double)RAND_MAX);
}

double XS_NormalRnd(double mean, double sigma)
{
    double r1 = XS_UniformRnd(0.0, 1.0);
    double r2 = XS_UniformRnd(0.0, 1.0);
    
    if (r1 == 0.0) r1 = 1.0e-10;
    
    return (mean + sigma * sqrt(-2.0*log(r1)) * sin(2.0*PI*r2));
}
