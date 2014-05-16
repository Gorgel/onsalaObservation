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
#ifndef M_LN2
#define M_LN2         0.69314718055994530942
#endif
#ifndef M_PI
#define M_PI          3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2        1.57079632679489661923	/* pi/2 */
#endif
#ifndef M_2_PI
#define M_2_PI        0.63661977236758134308	/* 2/pi */
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI    1.12837916709551257390
#endif

#define ALPHA         (4.0 * M_LN2)             /* 4 ln(2)    */
#define SQALPHA       1.6651092223154           /* sqrt(4 ln(2)) */

typedef struct {
    int nData;
    double *x1;
    double *x2;
    double *y;
    double *e;
    void (*f)();
} FitData;

typedef struct {
    int nPar;
    double *p, *pold;
    double *q;
    int    *f;
    double *d;
    int nFit;
    double **pp;
    double **pq;
    double **pd;
} FitParameters;

typedef struct {
    double lambda;
    double old_chi2, chi2;
    double **C, **H;
    double *rhs, *dp;
} LevMar;
