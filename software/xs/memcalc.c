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

#ifdef NUMTHREADS
#include <pthread.h>
#endif

#include "defines.h"
#include "global_structs.h"
#include "mem.h"

/* #define LAPACK */

/*** External variables ***/
void send_line();
int MyLoop(int);
int QueryHaltMEM();

double  *AllocDoubleVector(int);
double **AllocDoubleArray(int, int);
void     FreeDoubleVector(double *);
void     FreeDoubleArray(double **, int, int);
#ifdef LAPACK
int     *AllocIntVector(int);
void     FreeIntVector(int *);
#endif

/*** Local variables ***/
static int    nMem, nObs, iType;
static int    *memSize, *obsSize;
static double **A, *B, *C, D, E;
static double *P, QB, QC, R;
static double **H, *rhs;
#ifdef LAPACK
static double *H1;
static int *IPIV;
#endif

static double *f, *x_f, *y_f, *f_old;
static double *d, *e, *x_d, *y_d;

static double c2_prev, c2_now;
static double s_df_old, s_df;

#ifdef NUMTHREADS
pthread_mutex_t P_lock;                       /* Lock for P[i]     */
volatile int n_threads = NUMTHREADS;          /* How many threads? */
#endif

static int ret_obs(int *size)
{
    if (!size) return 1;
    
    if (d)   FreeDoubleVector(d);
    if (e)   FreeDoubleVector(e);
    if (x_d) FreeDoubleVector(x_d);
    if (y_d) FreeDoubleVector(y_d);
    
    d   = NULL;
    e   = NULL;
    x_d = NULL;
    y_d = NULL;
    
    *size = 0;
    
    return 0;
}

static int ret_mem(int *size)
{    
    if (!size) return 1;

    if (A)     FreeDoubleArray(A,     *size, *size);
    if (H)     FreeDoubleArray(H,     *size + 2, *size + 2);
#ifdef LAPACK
    if (H1)    FreeDoubleVector(H1);
    if (IPIV)  FreeIntVector(IPIV);
#endif
    if (B)     FreeDoubleVector(B);
    if (C)     FreeDoubleVector(C);
    if (P)     FreeDoubleVector(P);
    if (rhs)   FreeDoubleVector(rhs);
    if (f)     FreeDoubleVector(f);
    if (f_old) FreeDoubleVector(f_old);
    if (x_f)   FreeDoubleVector(x_f);
    if (y_f)   FreeDoubleVector(y_f);
    
    A =     NULL;
    H =     NULL;
#ifdef LAPACK
    H1 =    NULL;
    IPIV =  NULL;
#endif
    B =     NULL;
    C =     NULL;
    P =     NULL;
    rhs =   NULL;
    f =     NULL;
    f_old = NULL;
    x_f =   NULL;
    y_f =   NULL;
    
    *size = 0;
    
    return 0;
}

static int CleanUpMEM(int n)
{
    ret_obs(obsSize);
    ret_mem(memSize);
    
    return n;
}

static int init_obs(MEMData *md)
{
    static int size=0;
    int new_size;
    int i, j, n;
    MAP *m = md->obs;
    
    n = 0;
    for (i=0; i<m->i_no; i++) {
        for (j=0; j<m->j_no; j++) {
            if (m->f[i][j] <= BLANK) continue;
            n++;
        }
    }
    nObs = n;
    
    new_size = nObs;
    
    if (new_size != size) {
        if (size)
            ret_obs(&size);
        d   = AllocDoubleVector(new_size);
        e   = AllocDoubleVector(new_size);
        x_d = AllocDoubleVector(new_size);
        y_d = AllocDoubleVector(new_size);
        size = new_size;
        obsSize = &size;
        if (!d || !e || !x_d || !y_d)
            return ret_obs(obsSize);
    }
    
    n = 0;
    for (i=0; i<m->i_no; i++) {
        for (j=0; j<m->j_no; j++) {
            if (m->f[i][j] <= BLANK) continue;
            d[n]   = m->d[i][j];
            e[n]   = m->e[i][j];
            x_d[n] = m->xleft  + (double)i * m->xspacing;
            y_d[n] = m->ylower + (double)j * m->yspacing;
            n++;
        }
    }
    
    return size;
}

static double MEMgauss(double x, double y, double b)
{
    double arg, g;
    
    if (b == 0.0) return 0.0;
    
    arg = ALPHA * (x*x + y*y)/b/b;
    
    g = GAUSSNORM * exp(-arg) / b / b;
    
    return g;
}

static double Gbeam(double x1, double x2, double y1, double y2,
                    double beam, double mpix_area)
{
    return mpix_area * MEMgauss(x1-x2, y1-y2, beam);
}

static double Agauss(double xi, double xj, double yi, double yj, int nobs,
                     double *x, double *y, double *e,
                     double beam, double mpix_area)
{
    int k;
    double g, arg, mu, nu, fact, X, Y, b2;
    
    mu = (xi + xj)/2.0;
    nu = (yi + yj)/2.0;
    b2 = beam * beam;
    fact = 2.0 * ALPHA / b2;
    
    g = 0.0;
    for (k=0; k<nobs; k++) {
        X = x[k] - mu;
        Y = y[k] - nu;
        arg = fact * (X*X + Y*Y);
        g += exp(-arg)/e[k]/e[k];
    }
    
    arg = fact*((xi-xj)*(xi-xj) + (yi-yj)*(yi-yj))/4.0;
    g *= GAUSSNORM*GAUSSNORM * exp(-arg) / b2 / b2 * mpix_area * mpix_area;
    
    return g;
}

static int do_not_blank(double x, double y, double limit)
{
    int n;
    double d2, d2min=0.0;
    
    for (n=0; n<nObs; n++) {
        d2 = (x-x_d[n])*(x-x_d[n]) + (y-y_d[n])*(y-y_d[n]);
        if (n == 0) {
            d2min = d2;
        } else {
            if (d2 < d2min) {
                d2min = d2;
            }
        }
    }
    
    if (d2min <= limit*limit) return 1;
    
    return 0;
}

static double get_init_int(double x, double y, double beam)
{
    int n, nmin=-1;
    double Imem0=1.0e-6, d2, d2min=0.0;
    
    for (n=0; n<nObs; n++) {
        d2 = (x-x_d[n])*(x-x_d[n]) + (y-y_d[n])*(y-y_d[n]);
        if (n == 0) {
            d2min = d2;
            nmin = n;
        } else {
            if (d2 < d2min) {
                d2min = d2;
                nmin = n;
            }
        }
    }
    
    if (nmin >= 0 && nmin < nObs) {
        if (d[nmin] > 0.0) Imem0 = d[nmin]*exp(-ALPHA*d2min/beam/beam);
    }
    
    return Imem0;
}

#ifdef NUMTHREADS
void *bbc1(void *arg)
{
    int i, j;
    MEMData *md = (MEMData *)arg;
    MAP *m = md->mem;
    
    for (i=0; i<nMem; i++) {
        if (EXAC(iType)) {
            for (j=i; j<nMem; j++) {
                A[j][i] = Agauss(x_f[i], x_f[j], y_f[i], y_f[j],
                                 nObs, x_d, y_d, e, md->beam,
                                 fabs(m->xspacing * m->yspacing));
                if (i != j)
                    A[i][j] = A[j][i];
            }
        } else {
            A[i][0] = Agauss(x_f[i], x_f[i], y_f[i], y_f[i],
                             nObs, x_d, y_d, e, md->beam,
                             fabs(m->xspacing * m->yspacing));
        }
    }
    return(NULL);
}

void *bbc2(void *arg)
{
    int i, k;
    MEMData *md = (MEMData *)arg;
    MAP *m = md->mem;
    double bik;
    
    for (i=0; i<nMem; i++) {
        B[i] = 0.0;
        C[i] = 0.0;
        for (k=0; k<nObs; k++) {
            bik = Gbeam(x_d[k], x_f[i], y_d[k], y_f[i], md->beam,
                        fabs(m->xspacing * m->yspacing));
            B[i] += bik;
            C[i] += bik * d[k] / (e[k] * e[k]);
        }
    }
    return(NULL);
}
#endif

static int init_mem(MEMData *md)
{
    static int size=0;
    int new_size;
    int i, j, n, k;
    double x, y;
    string buf;
    MAP *m = md->mem;
#ifdef NUMTHREADS
    pthread_t thread[2];
    void * retval;
#else
    double bik;
#endif
        
    if (md->aIter > 0 && md->eIter > 0) {
        iType = MEM_TYPE_BOTH;
    } else if (md->eIter > 0) {
        iType = MEM_TYPE_EXAC;
    } else if (md->aIter > 0) {
        iType = MEM_TYPE_APPR;
    } else {
        fprintf(stderr, "%s: Invalid MEM_TYPE=%d.", PKGNAME, iType);
        return 0;
    }
    
    if (md->doBlank) {
        n = 0;
        for (i=0; i<m->i_no; i++) {
            for (j=0; j<m->j_no; j++) {
                x = m->xleft  + (double)i * m->xspacing;
                y = m->ylower + (double)j * m->yspacing;
                if (do_not_blank(x, y, md->blankLim)) {
                    m->f[i][j] = 1;
                    n++;
                } else {
                    m->f[i][j] = BLANK;
                }
            }
        }
        new_size = n;
    } else {
        for (i=0; i<m->i_no; i++) {
            for (j=0; j<m->j_no; j++) {
                m->f[i][j] = 1;
            }
        }
        new_size = m->ndata;
    }
    
    if (new_size != size) {
        if (size)
            ret_mem(&size);
        A     = AllocDoubleArray(new_size, new_size);
        H     = AllocDoubleArray(new_size+2, new_size+2);
#ifdef LAPACK
        H1    = AllocDoubleVector((new_size+2)*(new_size+2));
        IPIV  = AllocIntVector(new_size+2);
#endif
        B     = AllocDoubleVector(new_size);
        C     = AllocDoubleVector(new_size);
        P     = AllocDoubleVector(new_size);
        rhs   = AllocDoubleVector(new_size+2);
        f     = AllocDoubleVector(new_size+2);
        f_old = AllocDoubleVector(new_size+2);
        x_f   = AllocDoubleVector(new_size);
        y_f   = AllocDoubleVector(new_size);
        
        size = new_size;
        memSize = &size;

#ifdef LAPACK
        if (!A || !H || !H1 || !IPIV || !B || !C || !P || !rhs || !f ||
            !f_old || !x_f || !y_f) return ret_mem(memSize);
#else        
        if (!A || !H || !B || !C || !P || !rhs || !f || !f_old || !x_f || !y_f)
            return ret_mem(memSize);
#endif
    }
    
    n = 0;
    for (i=0; i<m->i_no; i++) {
        for (j=0; j<m->j_no; j++) {
            if (m->f[i][j] <= BLANK) continue;
            if (m->memed) {
                f[n]     = m->d[i][j];
                f_old[n] = m->e[i][j];
            }
            x_f[n] = m->xleft  + (double)i * m->xspacing;
            y_f[n] = m->ylower + (double)j * m->yspacing;
            n++;
        }
    }
    
    nMem = n;
    
    sprintf(buf, "MEM: No. of positions: nObs=%d nMem=%d.", nObs, nMem);
    send_line(buf);
#ifdef NUMTHREADS
    sprintf(buf, "MEM: Using %d number of threads.", n_threads);
    send_line(buf);
#endif

    send_line("MEM: Calculating beam-beam correlation array... ");
#ifdef NUMTHREADS
    if (pthread_create(&thread[0], NULL, bbc1, md)) {
        fprintf(stderr, "init_mem: cannot make thread bbc1\n");
        exit(1);
    }
    if (pthread_create(&thread[1], NULL, bbc2, md)) {
        fprintf(stderr, "init_mem: cannot make thread bbc2\n");
        exit(1);
    }
    for (i=0; i < 2; i++) {
        if (pthread_join(thread[i], &retval)) {
            fprintf(stderr, "init_mem: thread %d join failed\n", i);
            exit(1);
        }
    }
#else
    for (i=0; i<nMem; i++) {
        B[i] = 0.0;
        C[i] = 0.0;
        if (EXAC(iType)) {
            for (j=i; j<nMem; j++) {
                A[j][i] = Agauss(x_f[i], x_f[j], y_f[i], y_f[j],
                                 nObs, x_d, y_d, e, md->beam,
                                 fabs(m->xspacing * m->yspacing));
                if (i != j)
                    A[i][j] = A[j][i];
            }
        } else {
            A[i][0] = Agauss(x_f[i], x_f[i], y_f[i], y_f[i],
                             nObs, x_d, y_d, e, md->beam,
                             fabs(m->xspacing * m->yspacing));
        }
        for (k=0; k<nObs; k++) {
            bik = Gbeam(x_d[k], x_f[i], y_d[k], y_f[i], md->beam,
                        fabs(m->xspacing * m->yspacing));
            B[i] += bik;
            C[i] += bik * d[k] / (e[k] * e[k]);
        }
        while (MyLoop(1));
    }
#endif /* NUMTHREADS */
    send_line("Done.");
    
    D = E = 0.0;
    for (k=0; k<nObs; k++) {
        D += (d[k] * d[k]) / (e[k] * e[k]);
        E += d[k];
    }
    
    if (m->memed) {
        f[nMem]   = m->lam1;
        f[nMem+1] = m->lam2;
    } else {
        /* AreaCorr = fabs( (m->xspacing * m->yspacing) /
                         (md->obs->xspacing * md->obs->yspacing) ); */
        for (i=0; i<nMem; i++) {
            /* f[i] = E / (double)nMem / AreaCorr; */
            f[i] = get_init_int(x_f[i], y_f[i], md->beam);
            f_old[i] = f[i];
        }
        f[nMem]   = -1.0;
        f[nMem+1] = 1.0;
    }
    
    while (MyLoop(1));
    
    return size;
}

static void save_mem_map(MEMData *md)
{
    int n=0, i, j;
    MAP *m = md->mem;
    
    for (i=0; i<m->i_no; i++) {
        for (j=0; j<m->j_no; j++) {
            if (m->f[i][j] <= BLANK) continue;
            m->d[i][j] = f[n];
            m->e[i][j] = f_old[n];
            n++;
        }
    }
    m->memed    = 1;
    m->original = md->obs;
    m->lam1 = f[nMem];
    m->lam2 = f[nMem+1];
}

#ifdef NUMTHREADS
static void *pqr(void *arg)
{
    int i, j;
    MEMthread *mt = (MEMthread *)arg;
    double beam = mt->md->beam;
    double area = fabs(mt->md->mem->xspacing * mt->md->mem->yspacing);
    
    for (i=mt->iproc; i<nMem; i+=n_threads) {
        P[i] = -C[i];
        if (EXAC(iType)) {
            for (j=0; j<nMem; j++)
                P[i] += A[i][j] * f[j];
        } else {
            for (j=0; j<nMem; j++)
                P[i] += Agauss(x_f[i], x_f[j], y_f[i], y_f[j], nObs,
                               x_d, y_d, e, beam, area) * f[j];
        }
    }
    
    return(NULL);
}
#endif

static void calc_PQR(MEMData *md, int iter)
{
    int i, j;
    string buf;
#ifdef NUMTHREADS
    pthread_t thread[MAXTHREADS];
    void *retval;
    MEMthread memthr[MAXTHREADS];
#else
    double beam = md->beam;
    double area = fabs(md->mem->xspacing * md->mem->yspacing);
#endif
    
#ifdef NUMTHREADS
    for (i=0; i<n_threads; i++) {
        memthr[i].md = md;
        memthr[i].iproc = i;
        if (pthread_create(&thread[i], NULL, pqr, (void *)&memthr[i])) {
            fprintf(stderr, "calc_PQR: cannot make thread %d.\n", i);
            exit(1);
        }
    }
    for (i=0; i<n_threads; i++) {
        if (pthread_join(thread[i], &retval)) {
            fprintf(stderr, "calc_PQR: thread %d join failed.\n", i);
            exit(1);
        }
    }
#else
    for (i=0; i<nMem; i++) {
        P[i] = -C[i];
        if (EXAC(iType)) {
            for (j=0; j<nMem; j++)
                P[i] += A[i][j] * f[j];
        } else {
            for (j=0; j<nMem; j++)
                P[i] += Agauss(x_f[i], x_f[j], y_f[i], y_f[j], nObs,
                               x_d, y_d, e, beam, area) * f[j];
        }
    }
#endif
    
    QB = 0.0;
    QC = 0.0;
    R  = 0.0;
    for (j=0; j<nMem; j++) {
        QB += B[j] * f[j];
        QC += C[j] * f[j];
        R  += P[j] * f[j];
    }
    R -= QC;
    c2_prev = c2_now;
    c2_now = (R + D)/(double)nObs;
    if (iter == 0) {
        sprintf(buf, "I(%3d): Chi^2=%10.5f  lam=%10.3f,%8.3f  s_df=%10.3e\n",
                iter, c2_now, f[nMem], f[nMem+1], s_df);
    } else if (iter < 0) {
        sprintf(buf, "A(%3d): Chi^2=%10.5f  lam=%10.3f,%8.3f  s_df=%10.3e\n",
                -iter, c2_now, f[nMem], f[nMem+1], s_df);
    } else {
        sprintf(buf, "E(%3d): Chi^2=%10.5f  lam=%10.3f,%8.3f  s_df=%10.3e\n",
                iter, c2_now, f[nMem], f[nMem+1], s_df);
    }
    send_line(buf);
    while (MyLoop(1));
}

static double d1S(double x, double F)
{
    if (x <= 0.0 || F <= 0.0) return 0.0;
    
    return (log(F/x) - 1.0);
}

static double d2S(double x, double F)
{
    if (x == 0.0) return 0.0;
    
    return (-1.0/x);
}

#ifdef NUMTHREADS
static void *se(void *arg)
{
    int i, j;
    double fact, lam1;
    MEMthread *mt = (MEMthread *)arg;
    
    lam1 = f[nMem];
    fact = 2.0 * lam1 / (double)nObs;
    
    for (i=mt->iproc; i<nMem; i+=n_threads) {
        for (j=0; j<=i; j++) {
             H[i][j] = fact * A[i][j];
             if (i == j)
                 H[i][i] += d2S(f[i], 1.0);
             else
                 H[j][i] = H[i][j];
        }
        H[i][nMem] = 2.0 * P[i] / (double)nObs;
        H[nMem][i] = H[i][nMem];
        H[i][nMem + 1] = B[i];
        H[nMem + 1][i] = B[i];
    }
    
    return(NULL);
}
#endif

static void calc_SE(MEMData *md)
{
    int i;
    double lam1, lam2, fact;
#ifdef NUMTHREADS
    pthread_t thread[MAXTHREADS];
    MEMthread memthr[MAXTHREADS];
    void *retval;
#else
    int j;
#endif

    lam1 = f[nMem];
    lam2 = f[nMem + 1];
    
    fact = 2.0 * lam1 / (double)nObs;
    
#ifdef NUMTHREADS
    for (i=0; i<n_threads; i++) {
        memthr[i].md = md;
        memthr[i].iproc = i;
        if (pthread_create(&thread[i], NULL, se, (void *)&memthr[i])) {
            fprintf(stderr, "calc_PQR: cannot make thread %d.\n", i);
            exit(1);
        }
    }
    for (i=0; i<n_threads; i++) {
        if (pthread_join(thread[i], &retval)) {
            fprintf(stderr, "calc_PQR: thread %d join failed.\n", i);
            exit(1);
        }
    }
    /* for (i=0; i<nMem; i++) {
        for (j=0; j<i; j++) {
            H[j][i] = H[i][j];
        }
        H[nMem][i] = H[i][nMem];
        H[nMem + 1][i] = H[i][nMem + 1];
    } */
#else    
    for (i=0; i<nMem; i++) {
        for (j=0; j<=i; j++) {
             H[i][j] = fact * A[i][j];
             if (i == j)
                 H[i][i] += d2S(f[i], 1.0);
             else
                 H[j][i] = H[i][j];
        }
        H[i][nMem] = 2.0 * P[i] / (double)nObs;
        H[nMem][i] = H[i][nMem];
        H[i][nMem + 1] = B[i];
        H[nMem + 1][i] = B[i];
    }
#endif
    H[nMem][nMem] = 0.0;
    H[nMem][nMem+1] = 0.0;
    H[nMem+1][nMem] = 0.0;
    H[nMem+1][nMem+1] = 0.0;
    
#ifdef LAPACK
    for (i=0; i<nMem+2; i++) {
        for (j=0; j<nMem+2; j++) {
            H1[i*(nMem+2) + j] = H[i][j];
        }
    }
#endif
    
    for (i=0; i<nMem; i++)
        rhs[i] = -d1S(f[i], 1.0) - fact*P[i] - lam2*B[i];

    rhs[nMem] = md->chi2 - (R + D)/(double)nObs;
    rhs[nMem + 1] = E - QB;
}

static int solve_quick(MEMData *md, double gain)
{
    int i;    
    double lam1, lam2, fact, a, r, e11, e22, e12, L1, L2;
    double det, dl1, dl2;
    
    lam1 = f[nMem];
    lam2 = f[nMem + 1];
    
    fact = 2.0 * lam1 / (double)nObs;
    
    e11 = e22 = e12 = 0.0;
    L1 = md->chi2 - (R + D)/(double)nObs;
    L2 = E - QB;

    for (i=0; i<nMem; i++) {
        if (EXAC(iType)) {
            a = H[i][i] = d2S(f[i], 1.0) + fact * A[i][i];
        } else {
            a = H[i][0] = d2S(f[i], 1.0) + fact * A[i][0];
        }
        if (a == 0.0) return (i+1);
        rhs[i] = d1S(f[i], 1.0) + fact*P[i] + lam2*B[i];
        r = rhs[i] / a;
        
        e11 += P[i] * P[i] / a;
        e22 += B[i] * B[i] / a;
        e12 += B[i] * P[i] / a;
        
        L1 += 2.0 * P[i] * r / (double)nObs;
        L2 += B[i] * r;
    }
    
    e11 *= -4.0/(double)(nObs*nObs);
    e22 *= -1.0;
    e12 *= -2.0/(double)nObs;
    
    det = e12*e12 - e11*e22;
    if (det == 0.0) return -1;
    
    dl1 = (e12*L2 - e22*L1)/det * gain;
    dl2 = (e12*L1 - e11*L2)/det * gain;
    
    s_df_old = s_df;
    s_df = 0.0;
    for (i=0; i<nMem; i++) {
        f_old[i] = f[i];
        if (EXAC(iType)) {
            r = -(rhs[i]*gain + 2.0*P[i]*dl1/(double)nObs + B[i]*dl2)/H[i][i];
        } else {
            r = -(rhs[i]*gain + 2.0*P[i]*dl1/(double)nObs + B[i]*dl2)/H[i][0];
        }
        if (f[i] != 0.0) s_df += fabs(r/f[i]);
        if (f[i] + r > 0.0) {
            f[i] += r;
        } else {
            f[i] /= 10.0;
        }
    }
    
    if (lam1 != 0.0) s_df += fabs(dl1/lam1);
    if (lam2 != 0.0) s_df += fabs(dl2/lam2);
    s_df /= (double)(nMem + 2);
    
    f_old[nMem]   = f[nMem];
    f_old[nMem+1] = f[nMem+1];
    
    f[nMem]   += dl1;
    f[nMem+1] += dl2;
    
    return 0;
}

static int solve_SE()
{
    int ierr=0, i;
#ifdef LAPACK
    int N = nMem+2, NRHS=1, LDA=nMem+2, LDB=nMem+2, INFO;

    int dgesv_(int *n, int *nrhs, double *a, int *lda,
               int *ipiv, double *b, int *ldb, int *info);
#else
    int GaussJ(double **, int, double *, int);
#endif

#ifdef LAPACK
    ierr = dgesv_(&N, &NRHS, H1, &LDA, IPIV, rhs, &LDB, &INFO);

    if (INFO != 0) {
        fprintf(stderr, "%s: Internal error %d in dgesv_()\n", PKGNAME, INFO);
        return INFO;
    }
#else
    ierr = GaussJ(H, nMem+2, rhs, 0);
    
    if (ierr != 0) {
        fprintf(stderr, "%s: Internal error %d in GaussJ()\n", PKGNAME, ierr);
        return ierr;
    }
#endif
    
    s_df_old = s_df;
    s_df = 0.0;
    for (i=0; i<nMem+2; i++) {
        f_old[i] = f[i];
        if (f[i] != 0.0) s_df += fabs(rhs[i]/f[i]);
        if (i < nMem && f[i] + rhs[i] <= 0.0) {
            f[i] /= 10.0;
        } else {
            f[i] += rhs[i];
        }
    }
    s_df /= (double)(nMem + 2);
    
    return ierr;
}

static void set_back_to_old()
{
    int i;
    
    for (i=0; i<nMem+2; i++) f[i] = f_old[i];
}

int MakeMEM(MEMData *md)
{
    int i, nc, ierr;
    double gain;
    string buf;
    
    void MapDraw();
    
    if (!md) return 1;
    if (!md->obs) return 2;
    if (!md->mem) return 3;
    
    if (init_obs(md) <= 0) return CleanUpMEM(4);
    if (init_mem(md) <= 0) return CleanUpMEM(5);
    
    calc_PQR(md, 0);
    
    nc = 0;
    gain = md->aGain;
    for (i=0; i<md->aIter; i++) {
        ierr = solve_quick(md, gain);
        if (ierr != 0) return CleanUpMEM(6);
        
        calc_PQR(md, -i-1);
        
        if (c2_now > c2_prev)
            nc++;
        else
            nc = 0;
            
        if (nc >= 1) {
            set_back_to_old();
            s_df = s_df_old;
            send_line("Increasing Chi^2 value! Aborting approx. iterations.");
            calc_PQR(md, -i-1);
            while (MyLoop(1));
            break;
        }
        
        if (s_df > s_df_old && i > 0) {
            gain *= md->aGain;
            set_back_to_old();
            s_df = s_df_old;
            sprintf(buf, "Lowering gain to %12.5e.", gain);
            send_line(buf);
            calc_PQR(md, -i-1);
        }
        
        save_mem_map(md);
        sprintf(buf, "Approx. %d, Chi^2=%f", i+1, c2_now);
        strcpy(md->mem->name, buf);
        if (s_df < md->aLimit) {
            sprintf(buf,
                    "Approx. accuracy limit achieved: s_df=%10.3e < %10.3e.",
                    s_df, md->aLimit);
            send_line(buf);
            MapDraw(NULL, md->mem, NULL);
            while (MyLoop(1));
            break;
        }
        while (MyLoop(1));
        if (QueryHaltMEM()) return CleanUpMEM(0);
    }
    
    
    for (i=0; i<md->eIter; i++) {
        calc_SE(md);
        ierr = solve_SE();
        if (ierr != 0) return CleanUpMEM(7);
        
        calc_PQR(md, i+1);
        
        save_mem_map(md);
        sprintf(buf, "Iter=%d, Chi^2=%f", i+1, c2_now);
        strcpy(md->mem->name, buf);
        MapDraw(NULL, md->mem, NULL);
        
        if (s_df < md->eLimit) {
            sprintf(buf,
                    "Exact accuracy limit achieved: s_df=%10.3e < %10.3e.",
                    s_df, md->eLimit);
            send_line(buf);
            break;
        }
        while (MyLoop(1));
        if (QueryHaltMEM()) return CleanUpMEM(0);
    }

    return CleanUpMEM(0);
}
