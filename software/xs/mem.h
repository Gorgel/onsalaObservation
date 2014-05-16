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
#define MEM_DEFAULT_BEAM   30.0
#define MEM_DEFAULT_CHI2    1.0
#define MEM_DEFAULT_AITER 100
#define MEM_DEFAULT_EITER  20
#define MEM_DEFAULT_AGAIN   0.2
#define MEM_DEFAULT_ALIM    1.0e-3
#define MEM_DEFAULT_ELIM    2.0e-6

#define MAXTHREADS     32

#define MEM_TYPE_BOTH  0
#define MEM_TYPE_APPR  1
#define MEM_TYPE_EXAC  2
#define EXAC(t)        ((t) == MEM_TYPE_BOTH || (t) == MEM_TYPE_EXAC)
#define APPR(t)        ((t) == MEM_TYPE_BOTH || (t) == MEM_TYPE_APPR)

typedef struct {
    int     aIter, eIter;
    double  beam;
    int     doBlank;
    double  blankLim;
    double  chi2;
    double  aGain;
    double  aLimit;
    double  eLimit;
    MAP    *obs;
    MAP    *mem;
} MEMData;

#ifdef NUMTHREADS
typedef struct {
    MEMData *md;
    int iproc;
} MEMthread;
#endif
