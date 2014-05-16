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
#include <math.h>

#include <Xm/Xm.h>

#include "defines.h"
#include "global_structs.h"

#define INTP_BILINEAR    0
#define INTP_BICUBIC     1
#define INTP_BIQUADRATIC 2

#define BICUB_INIT       0
#define BICUB_END        1
#define BICUB_CALC       2

#define FIRST_QUADRANT   0
#define SECOND_QUADRANT  1
#define THIRD_QUADRANT   2
#define FOURTH_QUADRANT  3

/*** Local variables ***/
typedef struct _quad_struct {
    int i0, j0;
    int i1, j1;
} quadrant;

double *AllocDoubleVector(int n)
{
    double *v;
    
    v = (double *) calloc( n, sizeof(double));
    
    return v;
}

void FreeDoubleVector(double *v)
{
    if (v) free(v);
}

int *AllocIntVector(int n)
{
    int *v;
    
    v = (int *) calloc( n, sizeof(int));
    
    return v;
}

void FreeIntVector(int *v)
{
    if (v) free(v);
}

double **AllocDoubleArray(int nX, int nY)
{
   int n, m;
   double **a;
   
   a = (double **) malloc(nX * sizeof(double *));
   if (!a) return NULL;
   for (n=0; n<nX; n++) {
        a[n] = (double *) calloc( nY, sizeof(double));
        if (!a[n]) {
            for (m=n-1; m>=0; m--) if (a[m]) free((char *)a[m]);
            if (a) free((char *)a);
            return NULL;
        }
   }
   
   return a;
}

void CopyDoubleArray(double ***new, double **old, int nX, int nY)
{
    int n;
    *new = old;
    for (n=0; n<nX; n++)
        (*new)[n] = old[n];
}

void FreeDoubleArray(double **a, int nX, int nY)
{
   int n;
   
   for (n=nX-1; n>=0; n--) free((char *) a[n]);
   free((char *)a);
}

int **AllocIntArray(int nX, int nY)
{
   int n, m;
   int **a;
   
   a = (int **) malloc(nX * sizeof(int *));
   if (!a) return NULL;
   for (n=0; n<nX; n++) {
        a[n] = (int *) calloc( nY, sizeof(int));
        if (!a[n]) {
            for (m=n-1; m>=0; m--) if (a[m]) free((char *)a[m]);
            if (a) free((char *)a);
            return NULL;
        }
   }
   
   return a;
}

void FreeIntArray(int **a, int nX, int nY)
{
   int n;
   
   for (n=nX-1; n>=0; n--) free((char *) a[n]);
   free((char *)a);
}

scanPtr **AllocScanPtrArray(int nX, int nY)
{
   int n, m;
   scanPtr **a;
   
   a = (scanPtr **) malloc(nX * sizeof(scanPtr *));
   if (!a) return NULL;
   for (n=0; n<nX; n++) {
        a[n] = (scanPtr *) calloc( nY, sizeof(scanPtr));
        if (!a[n]) {
            for (m=n-1; m>=0; m--) if (a[m]) free((char *)a[m]);
            if (a) free((char *)a);
            return NULL;
        }
   }
   
   return a;
}

void FreeScanPtrArray(scanPtr **a, int nX, int nY)
{
   int n;
   
   for (n=nX-1; n>=0; n--) free((char *) a[n]);
   free((char *)a);
}

static quadrant *GetQuadrant(int qNumber, int **flag, int nX, int nY,
                             int i, int j)
{
    static quadrant q;
    
    if (qNumber == FIRST_QUADRANT) {
        if (i >= nX - 1 || j <= 0) return NULL;
        q.i0 = i;
        q.j0 = j - 1;
        q.i1 = i + 1;
        q.j1 = j;
        if (!flag[q.i0][q.j0]) return NULL;
        if (!flag[q.i1][q.j0]) return NULL;
        if (!flag[q.i1][q.j1]) return NULL;
    } else if (qNumber == SECOND_QUADRANT) {
        if (i <= 0 || j <= 0) return NULL;
        q.i0 = i - 1;
        q.j0 = j - 1;
        q.i1 = i;
        q.j1 = j;
        if (!flag[q.i0][q.j0]) return NULL;
        if (!flag[q.i0][q.j1]) return NULL;
        if (!flag[q.i1][q.j0]) return NULL;
    } else if (qNumber == THIRD_QUADRANT) {
        if (i <= 0 || j >= nY - 1) return NULL;
        q.i0 = i - 1;
        q.j0 = j;
        q.i1 = i;
        q.j1 = j + 1;
        if (!flag[q.i0][q.j0]) return NULL;
        if (!flag[q.i0][q.j1]) return NULL;
        if (!flag[q.i1][q.j1]) return NULL;
    } else if (qNumber == FOURTH_QUADRANT) {
        if (i >= nX - 1 || j >= nY - 1) return NULL;
        q.i0 = i;
        q.j0 = j;
        q.i1 = i + 1;
        q.j1 = j + 1;
        if (!flag[q.i0][q.j1]) return NULL;
        if (!flag[q.i1][q.j0]) return NULL;
        if (!flag[q.i1][q.j1]) return NULL;
    } else {
        return NULL;
    }
    
    return &q;
}

static double *PlaneValue(int qNumber, quadrant *q, double **a,
                          double xp, double yp)
{
    int i;
    double z[3], x[3], y[3], vx[2], vy[2], vz[2], v[3];
    static double zp;
    
    if (qNumber == FIRST_QUADRANT) {
        z[0] = a[q->i1][q->j0]; x[0] =  1.0; y[0] =  1.0;
        z[1] = a[q->i0][q->j0]; x[1] =  0.0; y[1] =  1.0;
        z[2] = a[q->i1][q->j1]; x[2] =  1.0; y[2] =  0.0;
    } else if (qNumber == SECOND_QUADRANT) {
        z[0] = a[q->i0][q->j0]; x[0] = -1.0; y[0] =  1.0;
        z[1] = a[q->i0][q->j1]; x[1] = -1.0; y[1] =  0.0;
        z[2] = a[q->i1][q->j0]; x[2] =  0.0; y[2] =  1.0;
    } else if (qNumber == THIRD_QUADRANT) {
        z[0] = a[q->i0][q->j0]; x[0] = -1.0; y[0] =  0.0;
        z[1] = a[q->i0][q->j1]; x[1] = -1.0; y[1] = -1.0;
        z[2] = a[q->i1][q->j1]; x[2] =  0.0; y[2] = -1.0;
    } else if (qNumber == FOURTH_QUADRANT) {
        z[0] = a[q->i0][q->j1]; x[0] =  0.0; y[0] = -1.0;
        z[1] = a[q->i1][q->j0]; x[1] =  1.0; y[1] =  0.0;
        z[2] = a[q->i1][q->j1]; x[2] =  1.0; y[2] = -1.0;
    }
    
    for (i=1; i<3; i++) {
        vx[i-1] = x[i] - x[0];
        vy[i-1] = y[i] - y[0];
        vz[i-1] = z[i] - z[0];
    }

    v[2] = vx[0]*vy[1] - vy[0]*vx[1];
    if (v[2] == 0.0) return NULL;

    v[0] = (vy[0]*vz[1] - vz[0]*vy[1])/v[2];
    v[1] = (vz[0]*vx[1] - vx[0]*vz[1])/v[2];

    zp  = z[0] + v[0]*(x[0] - xp) + v[1]*(y[0] - yp);
    
    return &zp;
}

static double *PlaneFit(double **a, int **flag, int nX, int nY,
                        int i, int j, int nc)
{
    int n, nq=0;
    double *v;
    string buf;
    static double d;
    quadrant *q;
    
    void send_line();
    
    d = 0.0;
    for (n=FIRST_QUADRANT; n<=FOURTH_QUADRANT; n++) {
        q = GetQuadrant(n, flag, nX, nY, i, j);
        if (!q) {
#ifdef DEEPDEBUG
            printf("Unusuable quadrant %1d\n", n);
#endif
            continue;
        }
        v = PlaneValue(n, q, a, 0.0, 0.0);
        if (v == NULL) {
            sprintf(buf, "PlaneFit: v == NULL!? n=%d\n", n);
            send_line(buf);
            return NULL;
        }
        d += *v;
        nq++;
#ifdef DEEPDEBUG
        printf("Found %1d[%1d] quadrant, v = %f, nc=%1d\n", n, nq, v, nc);
#endif
    }
    
    if (nq < nc) return NULL;
    
    d /= (double)nq;
    
    return &d;
}

int FillHolesInArray(double **a, int **flag, int nX, int nY, int nCorners)
{
    int i, j;
    double *d;
    int **flagTmp;
    
    flagTmp = AllocIntArray(nX, nY);
    if (!flagTmp) {
        return 1;
    }
    
    if (nCorners <= 2) {
        for (i=0; i<nX; i++) {
            for (j=1; j<nY-1; j++) {
                if (flag[i][j]) continue;
                if (flag[i][j-1] && flag[i][j+1]) {
                    flag[i][j] = 1;
                    a[i][j] = (a[i][j-1] + a[i][j+1])/2.0;
                }
            }
        }

        for (j=0; j<nY; j++) {
            for (i=1; i<nX-1; i++) {
                if (flag[i][j]) continue;
                if (flag[i-1][j] && flag[i+1][j]) {
                    flag[i][j] = 1;
                    a[i][j] = (a[i-1][j] + a[i+1][j])/2.0;
                }
            }
        }
    }
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            if (flag[i][j]) {
                flagTmp[i][j] = 1;
                continue;
            }
            d = PlaneFit(a, flag, nX, nY, i, j, nCorners);
            if (d) {
                a[i][j] = *d;
                flagTmp[i][j] = 1;
            } else {
                a[i][j] = UNDEF;
                flagTmp[i][j] = 0;
            }
        }
    }
    
    for (i=0; i<nX; i++) {
        for (j=0; j<nY; j++) {
            flag[i][j] = flagTmp[i][j];
        }
    }
    
    FreeIntArray(flagTmp, nX, nY);
    
    return 0;
}

static double BiLinear(double **a, int x1, int y1, int x2, int y2,
                       int i, int j)
{
    double x, y, v;
    double A, B, C, D;
    
    x = (double)(i-x1-x1)/2.0;
    y = (double)(j-y1-y1)/2.0;
    
    A = a[x1][y2] + a[x2][y1] - a[x2][y2];
    B = a[x2][y1] - A;
    C = a[x1][y2] - A;
    D = a[x1][y1] - A;
    
    v = A + B*x + C*y + D*(x-1.0)*(y-1.0);
    
    return v;
}

static double BiQuadratic(double **a, int x1, int y1, int x2, int y2,
                          int i, int j)
{
    double v=0.0, x, y;
    
    x = (double)(i-x1-x1)/2.0;
    y = (double)(j-y1-y1)/2.0;
    
    return v;
}

static int CheckIndices(int **flag, int nX, int nY,
                        int x1, int y1, int x2, int y2,
                        int x_new, int y_new)
{
    int i, j;
    
    if (x2 < x1 || y2 < y1) return 0;
    if (x1 < 0 || y1 < 0) return 0;
    if (x2 > nX-1 || y2 > nY-1) return 0;

    if (x_new >= 0 && y_new >= 0) {
        i = x_new - x1 - x1;
        j = y_new - y1 - y1;
        if (i > 2 || i < 0) return 0;
        if (j > 2 || j < 0) return 0;
    }
    
    for (i=x1; i<=x2; i++) {
        for (j=y1; j<=y2; j++) {
            if (!flag[i][j]) return 0;
        }
    }
    return 1;
}

static void GetDerivatives(double **a, double **aX, double **aY, double **aXY,
                           int **flag, int nX, int nY)
{
    int i, j, n;
    double d;
    
    for (j=0; j<nY; j++) {
        for (i=0; i<nX; i++) {
            d = 0.0;
            n = 0;
            if (CheckIndices(flag, nX, nY, i-2, j, i+2, j, -1, -1)) {
                d += (a[i-2][j] - 8.0*a[i-1][j] + 8.0*a[i+1][j] -
                      a[i+2][j])/12.0;
                n++;
            } else if (CheckIndices(flag, nX, nY, i-1, j, i+1, j, -1, -1)) {
                d += (a[i+1][j] - a[i-1][j])/2.0;
                n++;
            } else {
                if (CheckIndices(flag, nX, nY, i-1, j, i, j, -1, -1)) {
                    d += (a[i][j] - a[i-1][j]);
                    n++;
                }
                if (CheckIndices(flag, nX, nY, i, j, i+1, j, -1, -1)) {
                    d += (a[i+1][j] - a[i][j]);
                    n++;
                }
            }
            if (n)
                aX[i][j] = d/(double)n;
            else
                aX[i][j] = 0.0;
            
            d = 0.0;
            n = 0;
            if (CheckIndices(flag, nX, nY, i, j-2, i, j+2, -1, -1)) {
                d += (a[i][j-2] - 8.0*a[i][j-1] + 8.0*a[i][j+1] -
                      a[i][j+2])/12.0;
                n++;
            } else if (CheckIndices(flag, nX, nY, i, j-1, i, j+1, -1, -1)) {
                d += (a[i][j+1] - a[i][j-1])/2.0;
                n++;
            } else {
                if (CheckIndices(flag, nX, nY, i, j-1, i, j, -1, -1)) {
                    d += (a[i][j] - a[i][j-1]);
                    n++;
                }
                if (CheckIndices(flag, nX, nY, i, j, i, j+1, -1, -1)) {
                    d += (a[i][j+1] - a[i][j]);
                    n++;
                }
            }
            if (n)
                aY[i][j] = d/(double)n;
            else
                aY[i][j] = 0.0;
            
            d = 0.0;
            n = 0;
            if (CheckIndices(flag, nX, nY, i-1, j-1, i+1, j+1, -1, -1)) {
                d += (a[i+1][j] + a[i-1][j] + a[i][j+1] + a[i][j-1] -
                      2.0*a[i][j] - a[i+1][j+1] - a[i-1][j-1])/(-2.0);
                n++;
            } else {
                if (CheckIndices(flag, nX, nY, i-1, j-1, i, j, -1, -1)) {
                    d += (a[i][j] + a[i-1][j-1] -
                          a[i][j-1] - a[i-1][j]);
                    n++;
                }
                if (CheckIndices(flag, nX, nY, i, j, i+1, j+1, -1, -1)) {
                    d += (a[i][j] + a[i+1][j+1] -
                          a[i][j+1] - a[i+1][j]);
                    n++;
                }
                if (CheckIndices(flag, nX, nY, i-1, j, i, j+1, -1, -1)) {
                    d += (a[i-1][j] + a[i][j+1] -
                          a[i-1][j+1] - a[i][j]);
                    n++;
                }
                if (CheckIndices(flag, nX, nY, i, j-1, i+1, j, -1, -1)) {
                    d += (a[i][j-1] + a[i+1][j] -
                          a[i][j] - a[i+1][j-1]);
                    n++;
                }
            }
            if (n)
                aXY[i][j] = d/(double)n;
            else
                aXY[i][j] = 0.0;
        }
    }
}

static int wArr[16][16] = {
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
  {-3,0,0,3,0,0,0,0,-2,0,0,-1,0,0,0,0},
  {2,0,0,-2,0,0,0,0,1,0,0,1,0,0,0,0},
  {0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0},
  {0,0,0,0,-3,0,0,3,0,0,0,0,-2,0,0,-1},
  {0,0,0,0,2,0,0,-2,0,0,0,0,1,0,0,1},
  {-3,3,0,0,-2,-1,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,-3,3,0,0,-2,-1,0,0},
  {9,-9,9,-9,6,3,-3,-6,6,-6,-3,3,4,2,1,2},
  {-6,6,-6,6,-4,-2,2,4,-3,3,3,-3,-2,-1,-1,-2},
  {2,-2,0,0,1,1,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,2,-2,0,0,1,1,0,0},
  {-6,6,-6,6,-3,-3,3,3,-4,4,2,-2,-2,-2,-1,-1},
  {4,-4,4,-4,2,2,-2,-2,2,-2,-2,2,1,1,1,1}
};

static double BiCubic(double **a, int **flag, int nX, int nY,
                      int x1, int y1, int x2, int y2,
                      int i, int j, int sflag)
{
    int n, m, k;
    static int status = BICUB_END;
    double v = 0.0, x, y;
    double A[4], AX[4], AY[4], AXY[4], B[16], D[16], C[4][4];
    static double **aX, **aY, **aXY;
    
    if (sflag == BICUB_INIT && status == BICUB_END) {
        aX  = AllocDoubleArray(nX, nY);
        if (!aX)
            return UNDEF;
        aY  = AllocDoubleArray(nX, nY);
        if (!aY) {
            FreeDoubleArray(aX, nX, nY);
            return UNDEF;
        }
        aXY = AllocDoubleArray(nX, nY);
        if (!aXY) {
            FreeDoubleArray(aX, nX, nY);
            FreeDoubleArray(aY, nX, nY);
            return UNDEF;
        }
        GetDerivatives(a, aX, aY, aXY, flag, nX, nY);
        status = BICUB_INIT;
    } else if (sflag == BICUB_END && status != BICUB_END) {
        if (aX)  FreeDoubleArray(aX,  nX, nY);
        if (aY)  FreeDoubleArray(aY,  nX, nY);
        if (aXY) FreeDoubleArray(aXY, nX, nY);
        status = BICUB_END;
    } else if (sflag == BICUB_CALC && status != BICUB_END) {
        x = (double)(i-x1-x1)/2.0;
        y = (double)(j-y1-y1)/2.0;
        
        A[0] = a[x1][y1];
        A[1] = a[x2][y1];
        A[2] = a[x2][y2];
        A[3] = a[x1][y2];
        
        AX[0] = aX[x1][y1];
        AX[1] = aX[x2][y1];
        AX[2] = aX[x2][y2];
        AX[3] = aX[x1][y2];
        
        AY[0] = aY[x1][y1];
        AY[1] = aY[x2][y1];
        AY[2] = aY[x2][y2];
        AY[3] = aY[x1][y2];
        
        AXY[0] = aXY[x1][y1];
        AXY[1] = aXY[x2][y1];
        AXY[2] = aXY[x2][y2];
        AXY[3] = aXY[x1][y2];
        
        for (n=0; n<4; n++) {
             B[n]    = A[n];
             B[n+4]  = AX[n];
             B[n+8]  = AY[n];
             B[n+12] = AXY[n];
        }
        
        for (n=0; n<16; n++) {
            v = 0.0;
            for (m=0; m<16; m++)
                v += (double)wArr[n][m] * B[m];
            D[n] = v;
        }
        k = 0;
        for (n=0; n<4; n++) {
            for (m=0; m<4; m++) {
                C[n][m] = D[k++];
            }
        }
        
        v = 0.0;
        for (n=3; n>=0; n--) {
            v = x*v + ((C[n][3]*y + C[n][2])*y + C[n][1])*y + C[n][0];
        }
        
        status = BICUB_CALC;
    } else {
        return UNDEF;
    }
    return v;
}

static int arrILin[4] = {-1,  0, 0, -1};
static int arrJLin[4] = {-1, -1, 0,  0};

static int arrIQua[9] = {-1,  0, -1, 0, -2, -1,  0, -2, -2};
static int arrJQua[9] = {-1, -1,  0, 0, -2, -2, -2, -1,  0};

static double *GetNewArrayElement(double **a, int **flag, int nX, int nY,
                                  int intpType, int i_new, int j_new)
{
    int x, y, i, j, n;
    static double d;
    
    i = (i_new + 1)/2;
    j = (j_new + 1)/2;
    
    if (intpType == INTP_BILINEAR) {
        for (n=0; n<4; n++) {
            x = i + arrILin[n];
            y = j + arrJLin[n];
            if (CheckIndices(flag, nX, nY, x, y, x+1, y+1, i_new, j_new)) {
                d = BiLinear(a, x, y, x+1, y+1, i_new, j_new);
                return &d;
            }
        }
    } else if (intpType == INTP_BIQUADRATIC) {
        for (n=0; n<9; n++) {
            x = i + arrIQua[n];
            y = j + arrJQua[n];
            if (CheckIndices(flag, nX, nY, x, y, x+1, y+1, i_new, j_new)) {
                d = BiQuadratic(a, x, y, x+1, y+1, i_new, j_new);
                return &d;
            }
        }
    } else if (intpType == INTP_BICUBIC) {
        for (n=0; n<4; n++) {
            x = i + arrILin[n];
            y = j + arrJLin[n];
            if (CheckIndices(flag, nX, nY, x, y, x+1, y+1, i_new, j_new)) {
                d = BiCubic(a, flag, nX, nY, x, y, x+1, y+1, 
                            i_new, j_new, BICUB_CALC);
                if (d == UNDEF) return NULL;
                return &d;
            }
        }
    }
    return NULL;
}

int InterpolateArray(double ***a, int ***flag, int *nX, int *nY, int intpType)
{
    int i, j, nXNew, nYNew;
    double *v, d;
    string buf;
    int **flagTmp;
    double **aTmp;
    
    void send_line();
    
    aTmp = AllocDoubleArray(*nX, *nY);
    if (!aTmp) {
        sprintf(buf, "Couldn't allocate aTmp[%d][%d].\n", *nX, *nY);
        send_line(buf);
        return 1;
    }
    
    flagTmp = AllocIntArray(*nX, *nY);
    if (!flagTmp) {
        FreeDoubleArray(aTmp, *nX, *nY);
        return 2;
    }
    
    for (i=0; i<*nX; i++) {
        for (j=0; j<*nY; j++) {
            aTmp[i][j] = (*a)[i][j];
            flagTmp[i][j] = (*flag)[i][j];
        }
    }
    
    FreeDoubleArray(*a, *nX, *nY);
    FreeIntArray(*flag, *nX, *nY);
    
    nXNew = 2*(*nX) - 1;
    nYNew = 2*(*nY) - 1;

    *a = AllocDoubleArray(nXNew, nYNew);
    if (!(*a)) {
        FreeDoubleArray(aTmp, *nX, *nY);
        FreeIntArray(flagTmp, *nX, *nY);
        return 3;
    }
    *flag = AllocIntArray(nXNew, nYNew);
    if (!(*flag)) {
        FreeDoubleArray(aTmp, *nX, *nY);
        FreeIntArray(flagTmp, *nX, *nY);
        FreeDoubleArray(*a, nXNew, nYNew);
        return 4;
    }
    
    if (intpType == INTP_BICUBIC) {
       d = BiCubic(aTmp, flagTmp, *nX, *nY, 0, 0, 0, 0, 0, 0, BICUB_INIT);
       if (d == UNDEF) {
           FreeDoubleArray(aTmp, *nX, *nY);
           FreeIntArray(flagTmp, *nX, *nY);
           FreeDoubleArray(*a, nXNew, nYNew);
           FreeIntArray(*flag, nXNew, nYNew);
           return 5;
       }
    }
    
    for (i=0; i<nXNew; i++) {
        for (j=0; j<nYNew; j++) {
            if (i % 2 == 0 && j % 2 == 0) {
#ifdef DEEPDEBUG
  printf("Old: i,j=%2d,%2d (%2d,%2d) %f\n", i/2, j/2,
         *nX, *nY, aTmp[i/2][j/2]);
#endif
                (*flag)[i][j] = flagTmp[i/2][j/2];
                (*a)[i][j] = aTmp[i/2][j/2];
            } else {
                v = GetNewArrayElement(aTmp, flagTmp, *nX, *nY, intpType,
                                       i, j);
                if (v) {
                    (*a)[i][j] = *v;
                    (*flag)[i][j] = 1;
#ifdef DEEPDEBUG
  printf("New: i,j=%2d,%2d (%2d,%2d) %f\n", i, j, nXNew, nYNew, *v);
#endif
                } else {
                    (*a)[i][j] = UNDEF;
                    (*flag)[i][j] = 0;
#ifdef DEEPDEBUG
  printf("New: i,j=%2d,%2d (%2d,%2d) BLANK\n", i, j, nXNew, nYNew);
#endif
                }
            }
        }
    }
    
    if (intpType == INTP_BICUBIC) {
       d = BiCubic(aTmp, flagTmp, *nX, *nY, 0, 0, 0, 0, 0, 0, BICUB_END);
       if (d == UNDEF) {
           FreeDoubleArray(aTmp, *nX, *nY);
           FreeIntArray(flagTmp, *nX, *nY);
           FreeDoubleArray(*a, nXNew, nYNew);
           FreeIntArray(*flag, nXNew, nYNew);
           return 6;
       }
    }
    
    FreeDoubleArray(aTmp, *nX, *nY);
    FreeIntArray(flagTmp, *nX, *nY);
    
    *nX = nXNew;
    *nY = nYNew;
    
    return 0;
}
