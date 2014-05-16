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
#include <math.h>
#include <stdlib.h>

#include "lmfit.h"

/* #define DEBUG */

#ifdef DEBUG
#include <stdio.h>
#endif

double  *AllocDoubleVector(int);
void     FreeDoubleVector(double *);
double **AllocDoubleArray(int, int);
void     FreeDoubleArray(double **, int, int);
int     *AllocIntVector(int);
void     FreeIntVector(int *);
    
int      GaussJ(double **, int, double *, int);

static int FitterReturn(FitParameters *fp, LevMar *lm, int retval)
{
    if (fp) {
        if (fp->pp) free(fp->pp);
        if (fp->pold) FreeDoubleVector(fp->pold);
        if (fp->pq) free(fp->pq);
        if (fp->d) FreeDoubleVector(fp->d);
        fp->pp = fp->pq = NULL;
        fp->d = fp->pold = NULL;
    }
    if (lm) {
        if (lm->C)   FreeDoubleArray(lm->C, fp->nFit, fp->nFit);
        if (lm->H)   FreeDoubleArray(lm->H, fp->nFit, fp->nFit);
        if (lm->rhs) FreeDoubleVector(lm->rhs);
        if (lm->dp)  FreeDoubleVector(lm->dp);
    }
    
    return retval;
}

static int InitFitParameters(double p[], int fit[], double q[], int nPar,
                             FitParameters *fp)
{
    int n, nFit = 0;
    
    for (n=0; n<nPar; n++) {
        if (fit[n]) nFit++;
    }
    
    if (nFit <= 0) return -1;
    
    fp->p = p;
    fp->q = q;
    fp->f = fit;
    
    fp->nFit = 0;
    
    fp->pp = (double **)malloc(nFit * sizeof(double *));
    if (!fp->pp) return -2;
    
    fp->pold = AllocDoubleVector(nPar);
    if (!fp->pold) {
        free(fp->pp);
        return -2;
    }
    
    fp->pq = (double **)malloc(nFit * sizeof(double *));
    if (!fp->pq) {
        free(fp->pp);
        free(fp->pold);
        return -2;
    }
    
    fp->pd = (double **)malloc(nFit * sizeof(double *));
    if (!fp->pd) {
        free(fp->pq);
        free(fp->pp);
        free(fp->pold);
        return -2;
    }
    
    fp->d = AllocDoubleVector(nPar);
    if (!fp->d) {
        free(fp->pp);
        free(fp->pold);
        free(fp->pq);
        free(fp->pd);
        return -2;
    }
    
    fp->nFit = nFit;
    fp->nPar = nPar;

#ifdef DEBUG
    printf("nPar=%d  nFit=%d\n", nPar, fp->nFit);
#endif
    
    nFit = 0;
    for (n=0; n<nPar; n++) {
        if (fit[n]) {
            fp->pp[nFit] = &p[n];
            fp->pq[nFit] = &q[n];
            fp->pd[nFit] = &(fp->d[n]);
#ifdef DEBUG
            printf("n=%d  %f\n", nFit, *(fp->pp[nFit]));
#endif
            nFit++;
        }
    }
    
    
    return 0;
}

static int InitData1(double x[], double y[], double e[], int nData,
                    void (*f)(), FitData *fd)
{
    fd->x1 = x;
    fd->x2 = NULL;
    fd->y  = y;
    fd->e  = e;
    fd->nData = nData;
    fd->f = f;
    
    return 0;
}

static int InitData2(double x1[], double x2[], double y[], double e[],
                     int nData,
                     void (*f)(), FitData *fd)
{
    fd->x1 = x1;
    fd->x2 = x2;
    fd->y  = y;
    fd->e  = e;
    fd->nData = nData;
    fd->f = f;
    
    return 0;
}

static void CalCurMat(FitData *fd, FitParameters *fp,
                      double **C, double *rhs, double *chi2)
{
    int i, j, n;
    double v, w, dy, e2;
    
#ifdef DEBUG
    printf("Zeroing C[][]...\n");
#endif
    for (i=0; i<fp->nFit; i++) {
        for (j=0; j<=i; j++) C[i][j] = 0.0;
        rhs[i] = 0.0;
    }
    
    *chi2 = 0.0;
    
#ifdef DEBUG
    printf("Calc chi^2...  nData=%d\n", fd->nData);
#endif
    for (n=0; n<fd->nData; n++) {
        fd->f(n, fd, &v, fp);
        dy = fd->y[n] - v;
        e2 = 1.0/fd->e[n]/fd->e[n];
        for (i=0; i<fp->nFit; i++) {
            w = (*(fp->pd[i])) * e2;
            for (j=0; j<=i; j++) C[i][j] += w * (*(fp->pd[j]));
            rhs[i] += w * dy;
        }
        *chi2 += dy*dy*e2;
    }
#ifdef DEBUG
    printf("chi^2 = %f\n", *chi2/(double)fd->nData);
#endif
    for (i=0; i<fp->nFit; i++) {
        for (j=0; j<i; j++)  C[j][i] = C[i][j];
    }
}

static int InitLevMar(FitData *fd, FitParameters *fp, LevMar *lm)
{
    if (!lm) return -1;
    
    lm->C = lm->H = NULL;
    lm->rhs = lm->dp = NULL;
    
    lm->C = AllocDoubleArray(fp->nFit, fp->nFit);
    if (!lm->C) return -2;
    lm->H = AllocDoubleArray(fp->nFit, fp->nFit);
    if (!lm->H) return -2;
    
    lm->rhs = AllocDoubleVector(fp->nFit);
    if (!lm->rhs) return -2;
    lm->dp = AllocDoubleVector(fp->nFit);
    if (!lm->dp) return -2;
    
    lm->lambda = 0.001;
    
#ifdef DEBUG
    printf("CalCurMat...\n");
#endif
    CalCurMat(fd, fp, lm->H, lm->rhs, &(lm->chi2));
#ifdef DEBUG
    printf("...done.\n");
#endif
    
    lm->old_chi2 = lm->chi2;
    
    return 0;
}

static int LoopLevMar(FitData *fd, FitParameters *fp, LevMar *lm)
{
    int i, j, err;
    
    for (i=0; i<fp->nFit; i++) {
        for (j=0; j<i; j++) lm->C[i][j] = lm->C[j][i] = lm->H[i][j];
        lm->C[i][i] = lm->H[i][i] * (1.0 + lm->lambda);
    }
    
    err = GaussJ(lm->C, fp->nFit, lm->rhs, 0);
    
    if (err) return err;
    
    for (i=0; i<fp->nFit; i++) lm->dp[i] = lm->rhs[i];
    for (i=0; i<fp->nPar; i++) fp->pold[i] = fp->p[i];
    
    /* try the new set of parameters */
    for (i=0; i<fp->nFit; i++) *(fp->pp[i]) += lm->dp[i];
    
    CalCurMat(fd, fp, lm->C, lm->rhs, &(lm->chi2));
    
    if (lm->chi2 < lm->old_chi2) {
        lm->lambda *= 0.1;
        lm->old_chi2 = lm->chi2;
        for (i=0; i<fp->nFit; i++) {
            lm->H[i][i] = lm->C[i][i];
            for (j=0; j<i; j++) lm->H[i][j] = lm->H[j][i] = lm->C[i][j];
        }
    } else {
        lm->lambda *= 10.0;
        lm->chi2 = lm->old_chi2;
        for (i=0; i<fp->nPar; i++) fp->p[i] = fp->pold[i];
    }
    
    return 0;
}

static int ExitLevMar(FitData *fd, FitParameters *fp, LevMar *lm, double *chi2)
{
    int i, err;
    
    err = GaussJ(lm->H, fp->nFit, lm->rhs, 1);
    if (err) return err;
    
    if (chi2) *chi2 = lm->chi2;
    
    for (i=0; i<fp->nPar; i++) fp->q[i] = 0.0;
    for (i=0; i<fp->nFit; i++) *(fp->pq[i]) = sqrt(lm->H[i][i]);
    
    return 0;
}

/* The standard Levenberg-Marquardt fitter routine for one-dimensional data.
 * Arguments:
 *      x[]     data vector containing the nData x-values
 *      y[]     data vector containing the nData y-values
 *      e[]     data vector containing the nData 1-sigma errors of the y-values
 *      nData   the number of data points in the vectors x[], y[], and e[]
 *
 *      p[]     parameter vector of length nPar, should contain an initial
 *              guess when called, and will hold the fitted parameter values
 *              upon return
 *      fit[]   integer vector of same size as p[] indicating, by a non-zero
 *              value, which parameters to be fit
 *      q[]     vector that holds the 1-sigma uncertainties of the nPar
 *              parameters p[]
 *      nPar    the number of parameters in the vector p[]
 *
 *      nIter   the maximum number of iterations to use in the fitting
 *              procedure
 *      *chi2   returns the chi^2 value of the fit
 *      (*f)()  a function pointer to the fitting function
 */
int Fitter1D(double x[], double y[], double e[], int nData,
             double p[], int fit[], double q[], int nPar,
             int nIter, double *chi2, void (*f)())
{
    int n, err=0;
    FitParameters fp;
    FitData fd;
    LevMar lm;
    
#ifdef DEBUG
    printf("InitFitPar.\n");
#endif
    err = InitFitParameters(p, fit, q, nPar, &fp);
    if (err) return err;
    
#ifdef DEBUG
    printf("InitData1\n");
#endif
    err = InitData1(x, y, e, nData, f, &fd);
    if (err) return err;
    
#ifdef DEBUG
    printf("InitLevMar\n");
#endif
    err = InitLevMar(&fd, &fp, &lm);
    if (err) return FitterReturn(&fp, &lm, err);
    
#ifdef DEBUG
    printf("Loop:\n");
#endif
    for (n=0; n<nIter; n++) {
#ifdef DEBUG
        printf("n=%d\n", n);
#endif
        err = LoopLevMar(&fd, &fp, &lm);
        if (err) return FitterReturn(&fp, &lm, err);
    }
#ifdef DEBUG
    printf("ExitLevMar\n");
#endif
    err = ExitLevMar(&fd, &fp, &lm, chi2);
    if (err) return FitterReturn(&fp, &lm, err);
    
    return FitterReturn(&fp, &lm, 0);
}

/* Same as Fitter1D() but for two variables, x1[] and x2[] */
int Fitter2D(double x1[], double x2[], double y[], double e[], int nData,
             double p[], int fit[], double q[], int nPar,
             int nIter, double *chi2, void (*f)())
{
    int n, err=0;
    FitParameters fp;
    FitData fd;
    LevMar lm;
    
#ifdef DEBUG
    printf("InitFitPar.\n");
#endif
    err = InitFitParameters(p, fit, q, nPar, &fp);
    if (err) return err;
    
#ifdef DEBUG
    printf("InitData2\n");
#endif
    err = InitData2(x1, x2, y, e, nData, f, &fd);
    if (err) return err;
    
#ifdef DEBUG
    printf("InitLevMar\n");
#endif
    err = InitLevMar(&fd, &fp, &lm);
    if (err) return FitterReturn(&fp, &lm, err);
    
#ifdef DEBUG
    printf("Loop:\n");
#endif
    for (n=0; n<nIter; n++) {
#ifdef DEBUG
        printf("n=%d\n", n);
#endif
        err = LoopLevMar(&fd, &fp, &lm);
        if (err) return FitterReturn(&fp, &lm, err);
    }
#ifdef DEBUG
    printf("ExitLevMar\n");
#endif
    err = ExitLevMar(&fd, &fp, &lm, chi2);
    if (err) return FitterReturn(&fp, &lm, err);
    
    return FitterReturn(&fp, &lm, 0);
}
