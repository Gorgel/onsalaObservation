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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

double  *AllocDoubleVector(int);
void     FreeDoubleVector(double *);
int     *AllocIntVector(int);
void     FreeIntVector(int *);

/*							simq.c
 *
 *	Solution of simultaneous linear equations AX = B
 *	by Gaussian elimination with partial pivoting
 *
 *
 *
 * SYNOPSIS:
 *
 * double A[n*n], B[n], X[n];
 * int n, flag;
 * int IPS[];
 * int simq();
 *
 * ercode = simq( A, B, X, n, flag, IPS );
 *
 *
 *
 * DESCRIPTION:
 *
 * B, X, IPS are vectors of length n.
 * A is an n x n matrix (i.e., a vector of length n*n),
 * stored row-wise: that is, A(i,j) = A[ij],
 * where ij = i*n + j, which is the transpose of the normal
 * column-wise storage.
 *
 * The contents of matrix A are destroyed.
 *
 * Set flag=0 to solve.
 * Set flag=-1 to do a new back substitution for different B vector
 * using the same A matrix previously reduced when flag=0.
 *
 * The routine returns nonzero on error; messages are printed.
 *
 *
 * ACCURACY:
 *
 * Depends on the conditioning (range of eigenvalues) of matrix A.
 *
 *
 * REFERENCE:
 *
 * Computer Solution of Linear Algebraic Systems,
 * by George E. Forsythe and Cleve B. Moler; Prentice-Hall, 1967.
 *
 */

/*							simq	2 */

static int simq(double A[], double B[], double X[], int n, int flag, int IPS[])
{
int i, j, ij, ip, ipj, ipk, ipn;
int idxpiv, iback;
int k, kp, kp1, kpk, kpn;
int nip, nkp, nm1;
double em, q, rownrm, big, size, pivot, sum;

nm1 = n-1;
if( flag < 0 )
	goto solve;

/*	Initialize IPS and X	*/

ij=0;
for( i=0; i<n; i++ )
	{
	IPS[i] = i;
	rownrm = 0.0;
	for( j=0; j<n; j++ )
		{
		q = fabs( A[ij] );
		if( rownrm < q )
			rownrm = q;
		++ij;
		}
	if( rownrm == 0.0 )
		{
/* //		puts("SIMQ ROWNRM=0"); */
		return(1);
		}
	X[i] = 1.0/rownrm;
	}

/*							simq	3 */
/*	Gaussian elimination with partial pivoting 	*/

for( k=0; k<nm1; k++ )
	{
	big= 0.0;
	idxpiv = 0;
	for( i=k; i<n; i++ )
		{
		ip = IPS[i];
		ipk = n*ip + k;
		size = fabs( A[ipk] ) * X[ip];
		if( size > big )
			{
			big = size;
			idxpiv = i;
			}
		}

	if( big == 0.0 )
		{
/* //		puts( "SIMQ BIG=0" ); */
		return(2);
		}
	if( idxpiv != k )
		{
		j = IPS[k];
		IPS[k] = IPS[idxpiv];
		IPS[idxpiv] = j;
		}
	kp = IPS[k];
	kpk = n*kp + k;
	pivot = A[kpk];
	kp1 = k+1;
	for( i=kp1; i<n; i++ )
		{
		ip = IPS[i];
		ipk = n*ip + k;
		em = -A[ipk]/pivot;
		A[ipk] = -em;
		nip = n*ip;
		nkp = n*kp;
		for( j=kp1; j<n; j++ )
			{
			ipj = nip + j;
			A[ipj] = A[ipj] + em * A[nkp + j];
			}
		}
	}
kpn = n * IPS[n-1] + n - 1;	/* last element of IPS[n] th row */
if( A[kpn] == 0.0 )
	{
/* //	puts( "SIMQ A[kpn]=0"); */
	return(3);
	}

/*							simq 4 */
/*	back substitution	*/

solve:
ip = IPS[0];
X[0] = B[ip];
for( i=1; i<n; i++ )
	{
	ip = IPS[i];
	ipj = n * ip;
	sum = 0.0;
	for( j=0; j<i; j++ )
		{
		sum += A[ipj] * X[j];
		++ipj;
		}
	X[i] = B[ip] - sum;
	}

ipn = n * IPS[n-1] + n - 1;
X[n-1] = X[n-1]/A[ipn];

for( iback=1; iback<n; iback++ )
	{
/* i goes (n-1),...,1	*/
	i = nm1 - iback;
	ip = IPS[i];
	nip = n*ip;
	sum = 0.0;
	for( j=i+1; j<n; j++ )
		sum += A[nip+j] * X[j];
	X[i] = (X[i] - sum)/A[nip+i];
	}
return(0);
}

/*
 * GaussJ: Solution of the linear system of equations
 *                     a x = b
 *         It provides a convenient interface to simq
 *
 *      Input:     a[][]         matrix of size nxn
 *                 n             size
 *                 b[]           rhs vector
 *                 inv           set to 1 if a[][] is
 *                               to be replaced with
 *                               its inverse
 *      Output:   b[]            contains the solution
 *                               vector x[]
 *                a[][]          if inv=1 it will
 *                               contain the inverse
 *                               otherwise the
 *                               original data
 */
int GaussJ(double **a, int n, double *b, int inv)
{
    int i, j, err;
    static int size = 0;
    static int *ips;
    static double *x, *A;
    
    if (n <= 0 || !a || !b) return -1;
    
    if (n != size) {
        if (size > 0) {
            FreeIntVector(ips);
            FreeDoubleVector(x);
            FreeDoubleVector(A);
        }
        size = 0;
        ips = AllocIntVector(n);
        if (!ips) return -2;
    
        x = AllocDoubleVector(n);
        if (!x) {
            FreeIntVector(ips);
            return -3;
        }
    
        A = AllocDoubleVector(n*n);
        if (!A) {
            FreeIntVector(ips);
            FreeDoubleVector(x);
            return -4;
        }
        size = n;
    }
    
    for (i=0; i<n; i++) {
        for (j=0; j<n; j++) {
            A[i*n + j] = a[i][j];
        }
    }
    
    err = simq(A, b, x, n, 0, ips);
    
    if (err) return err;
    
    if (inv) {
        for (i=0; i<n; i++) {
            for (j=0; j<n; j++) b[j] = 0.0;
            b[i] = 1.0;
            err = simq(A, b, x, n, -1, ips);
            for (j=0; j<n; j++) a[j][i] = x[j];
        }
    } else {
        for (i=0; i<n; i++) b[i] = x[i];
    }
    
    return 0;
}
