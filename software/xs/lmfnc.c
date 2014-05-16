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

#include "lmfit.h"

#define SIMPSON_NSTEPS 300

typedef struct {
    double v, T0, v0, ve, vt, tau, R, B;
} ExpArg;

double simpson(double x1, double x2, ExpArg a, int n, double (* f)())
{
  int i, n_step = n;
  double x, x_step, s, weight;

  if (n_step < 4) n_step = 4;
  if ((n_step % 2) == 1) n_step++;
  
  x_step = (x2-x1)/(double)(n_step-1);

  s = f(x1, a) + f(x2, a);

  for (i=2; i<=n_step-1; i++) {
      x = x1 + (double)(i-1) * x_step;
      weight = (double)(2*(1 + (i % 2)));
      s += weight*f(x, a);
  }

  return (s*x_step/3.0);
}

static double I0_p_coeffs[7] = {
      1.0000000e0, 3.5156229e0, 3.0899424e0, 1.2067492e0,
      0.2659732e0, 0.0360768e0, 0.0045813e0
};
static double I0_r_coeffs[9] = {
     0.39894228e0, 0.01328592e0, 0.00225319e0,-0.00157565e0,
     0.00916281e0,-0.02057706e0, 0.02635537e0,-0.01647633e0,
     0.00392377e0
};

static double I1_p_coeffs[7] = {
      0.50000000e0, 0.87890594e0, 0.51498869e0, 0.15084934e0,
      0.02658733e0, 0.00301532e0, 0.00032411e0
};
static double I1_r_coeffs[9] = {
     0.39894228e0,-0.03988024e0,-0.00362018e0, 0.00163801e0,
    -0.01031555e0, 0.02282967e0,-0.02895312e0, 0.01787654e0,
    -0.00420059e0
};

static double expI0(double x)
{
  int i;
  double a, t, i0;

  a = fabs(x);
  
  if (a == 0.0) return 1.0;

  if (a <= 3.75) {
    t = a*a/3.75/3.75;
    i0 = I0_p_coeffs[6];
    for (i=5; i>=0; i--) {
        i0 = i0*t + I0_p_coeffs[i];
    }
    i0 *= exp(-a);
  } else {
    t = 3.75/a;
    i0 = I0_r_coeffs[8];
    for (i=7; i>=0; i--) {
        i0 = i0*t + I0_r_coeffs[i];
    }
    i0 /= sqrt(a);
  }

  return i0;
}

double Bessel_I0(double x)
{
  int i;
  double a, t, i0;

  a = fabs(x);
  
  if (a == 0.0) return 1.0;

  if (a <= 3.75) {
    t = a*a/3.75/3.75;
    i0 = I0_p_coeffs[6];
    for (i=5; i>=0; i--) {
        i0 = i0*t + I0_p_coeffs[i];
    }
  } else {
    t = 3.75/a;
    i0 = I0_r_coeffs[8];
    for (i=7; i>=0; i--) {
        i0 = i0*t + I0_r_coeffs[i];
    }
    i0 *= exp(a) / sqrt(a);
  }

  return i0;
}

double Bessel_I1(double x)
{
  int i;
  double a, t, i1;

  a = fabs(x);
  
  if (a == 0.0) return 0.0;

  if (a <= 3.75) {
    t = a*a/3.75/3.75;
    i1 = I1_p_coeffs[6];
    for (i=5; i>=0; i--) {
        i1 = i1*t + I1_p_coeffs[i];
    }
  } else {
    t = 3.75/a;
    i1 = I1_r_coeffs[8];
    for (i=7; i>=0; i--) {
        i1 = i1*t + I1_r_coeffs[i];
    }
    i1 *= exp(a) / sqrt(a);
  }
  
  if (x < 0.0) i1 = -i1;

  return i1;
}

static double K0_p_coeffs[] = {
     -0.57721566, 0.42278420, 0.23069756, 0.03488590,
      0.00262698, 0.00010750, 0.00000740
};
static double K0_r_coeffs[] = {
      1.25331414, -0.07832358, 0.02189568, -0.01062446,
      0.00587872, -0.00251540, 0.00053208
};

double Bessel_K0(double x)
{
  int i;
  double t, k0;

  if (x <= 0.0) return 0.0;
  
  if (x <= 2.0) {
    t = x*x/2.0/2.0;
    k0 = K0_p_coeffs[6];
    for (i=5; i>=0; i--) {
        k0 = t*k0 + K0_p_coeffs[i];
    }
    k0 -= log(x/2.0) * Bessel_I0(x);
  } else {
    t = 2.0/x;
    k0 = K0_r_coeffs[6];
    for (i=5; i>=0; i--) {
        k0 = t*k0 + K0_r_coeffs[i];
    }
    k0 /= (exp(x) * sqrt(x));
  }

  return k0;
}

static double ExpansionIntegrand0(double u, ExpArg a)
{
    double y, arg, su;
    
    if (a.vt == 0.0) return 0.0;
    su = sin(u);
    arg = (a.v - a.v0 - a.ve * su)/a.vt;
    
    y = exp(-arg * arg);
    
    return y;
}

static double ExpansionIntegrand1(double u, ExpArg a)
{
    double y, arg, su;
    
    if (a.vt == 0.0) return 0.0;
    su = sin(u);
    arg = (a.v - a.v0 - a.ve * su)/a.vt;
    
    y = exp(-arg * arg) * su;
    
    return y;
}

static double ExpansionIntegrand2(double u, ExpArg a)
{
    double y, arg, su;
    
    if (a.vt == 0.0) return 0.0;
    su = sin(u);
    arg = (a.v - a.v0 - a.ve * su)/a.vt;
    
    y = exp(-arg * arg) * su * su;
    
    return y;
}

static double ExpansionIntegral(ExpArg a, int n)
{
    double y=0.0;
    
    if (n == 0) {
        y = simpson(-M_PI_2, M_PI_2, a, SIMPSON_NSTEPS, ExpansionIntegrand0);
    } else if (n== 1) {
        y = simpson(-M_PI_2, M_PI_2, a, SIMPSON_NSTEPS, ExpansionIntegrand1);
    } else if (n == 2) {
        y = simpson(-M_PI_2, M_PI_2, a, SIMPSON_NSTEPS, ExpansionIntegrand2);
    }
    
    return y;
}

static double inner(double x, ExpArg a)
{
    double y;
    
    y = exp(x*(2.0 * SQALPHA * a.v - x)) * expI0(2.0 * SQALPHA * a.v * x);
    
    return y;
}

static double outer1(double t, ExpArg a)
{
    double y;
    
    double Bessel_K0();
    
    y = Bessel_K0(t) *
        simpson(0.0, SQALPHA * a.v0 * t, a, SIMPSON_NSTEPS, inner);
    
    return y;
}

static double outer2(double t, ExpArg a)
{
    double y, arg;
    
    double Bessel_K0();
    
    arg = a.v - a.v0 * t;
    y = Bessel_K0(t) * t * exp(-ALPHA * arg * arg) *
        expI0(2.0 * ALPHA * a.v * a.v0 * t);
    
    return y;
}

static double CometIntegral(ExpArg a, int n)
{
    double y=0.0;

    if (n == 0) {
        y = SQALPHA/M_PI * exp(-ALPHA * a.v * a.v) *
            simpson(0.0, 25.0, a, SIMPSON_NSTEPS, outer1);
    } else if (n == 1) {
        y = ALPHA/M_PI * simpson(0.0, 25.0, a, SIMPSON_NSTEPS, outer2);
    }
    
    return y;
}

static double dg0(double t, ExpArg a)
{
    double y, p, q;
    
    double Bessel_I0();
    
    p = ALPHA * a.R * a.R / a.B / a.B;
    q = a.v/a.R;
    y = exp(-p*(q*q+t*t)) * Bessel_I0(2.0*p*q*t) * t;
    
    return y;
}

static double dg1(double t, ExpArg a)
{
    double y, p, q;
    
    double Bessel_I0();
    
    p = ALPHA * a.R * a.R / a.B / a.B;
    q = a.v/a.R;
    y = exp(-p*(q*q+t*t)) * Bessel_I0(2.0*p*q*t) * t*t*t;
    
    return y;
}

static double dg2(double t, ExpArg a)
{
    double y, p, q;
    
    double Bessel_I0();
    
    p = ALPHA * a.R * a.R / a.B / a.B;
    q = a.v/a.R;
    y = exp(-p*(q*q+t*t)) * Bessel_I1(2.0*p*q*t) * t*t;
    
    return y;
}

static double DiscIntegral(ExpArg a, int n)
{
    double y=0.0;

    if (n == 0) {
      y = simpson(0.0, 1.0, a, SIMPSON_NSTEPS, dg0);
    } else if (n == 1) {
      y = simpson(0.0, 1.0, a, SIMPSON_NSTEPS, dg1);
    } else if (n == 2) {
      y = simpson(0.0, 1.0, a, SIMPSON_NSTEPS, dg2);
    }
    
    return 2.0 * ALPHA * a.R * a.R / a.B / a.B * y;
}

void lm_poly(int n, FitData *fd, double *y, FitParameters *fp)
{
    int i, m = fp->nPar;
    double v, x = fd->x1[n];

    *y = fp->p[0];
    fp->d[0] = 1.0;
    for (i = 1; i < m; i++) {
        v = pow(x, (double)i);
        *y += fp->p[i] * v;
        fp->d[i] = v;
    }
}

void lm_line2(int n, FitData *fd, double *y, FitParameters *fp)
{
    double x = fd->x1[n];
    
    *y      = fp->p[0] + fp->p[1]*x;
    fp->d[0] = 1.0;
    fp->d[1] = x;
}

void lm_invpoly(int n, FitData *fd, double *y, FitParameters *fp)
{
    int i, m = fp->nPar;
    double v, t, x = fd->x1[n];

    t = fp->p[0];
    fp->d[0] = 1.0;
    for (i = 1; i < m; i++) {
        v = pow(x, (double)i);
        t += fp->p[i]*v;
        fp->d[i] = v;
    }
    if (t != 0.0) {
        *y = 1.0/t;
        for (i = 0; i < m; i++) fp->d[i] /= -t*t;
    } else {
        *y = 0.0;
        for (i = 1; i < m; i++) fp->d[i] = 0.0;
    }
}

static double chpol(int n, double x)
{
    double val, x2;

    if (n < 0) return 0.0;

    x2 = x*x;
    if (n == 0) {
        val = 1.0;
    } else if (n == 1) {
        val = x;
    } else if (n == 2) {
        val = 2.0*x2 - 1.0;
    } else if (n == 3) {
        val = (4.0*x2 - 3.0)*x;
    } else if (n == 4) {
        val = 8.0*(x2 - 1.0)*x2 + 1.0;
    } else if (n == 5) {
        val = ((16.0*x2 - 20.0)*x2 + 5.0)*x;
    } else if (n == 6) {
        val = ((32.0*x2 - 48.0)*x2 + 18.0)*x2 - 1.0;
    } else if (n == 7) {
        val = (((64.0*x2 - 112.0)*x2 + 56.0)*x2 - 7.0)*x;
    } else if (n == 8) {
        val = (((128.0*x2 - 256.0)*x2 + 160.0)*x2 - 32.0)*x2 + 1.0;
    } else if (n == 9) {
        val = ((((256.0*x2 - 576.0)*x2 + 432.0)*x2 - 120.0)*x2 + 9.0)*x;
    } else if (n == 10) {
        val = ((((512.0*x2 - 1280.0)*x2 + 1120.0)*x2 - 400.0)
               *x2 + 50.0)*x2 - 1.0;
    } else {
        val = 0.0;
    }
    return  val;
}

double cheby_poly(int n, double x)
{
    int i;
    double fi=0.0, f1, f2;

    if (n < 0) return 0.0;
    if (n <= 10) return chpol(n, x);

    f2 = chpol(9, x);
    f1 = chpol(10, x);

    for (i=11; i<=n; i++) {
        fi = 2.0*x*f1 - f2;
        f1 = fi;
        f2 = f1;
    }
    
    return fi;
}

double sinc(double x)
{
    if (fabs(x) < 1.0e-3) {
        return (1.0 - x*x*(1.0 - x*x/20.0)/6.0);
    }
    return (sin(x)/x);
}

void lm_cheby(int n, FitData *fd, double *y, FitParameters *fp)
{
    int i, m = fp->nPar;
    double v, x = fd->x1[n];

    *y = fp->p[0];
    fp->d[0] = 1.0;
    for (i = 1; i < m; i++) {
        v = cheby_poly(i, x);
        *y += fp->p[i] * v;
        fp->d[i] = v;
    }
}

void lm_polysin(int n, FitData *fd, double *y, FitParameters *fp)
{
    int i, m = fp->nPar;
    double v, s, c, x = fd->x1[n];

    *y = fp->p[0];
    fp->d[0] = 1.0;
    for (i = 1; i < m-3; i++) {
        v = pow(x, (double)i);
        *y += fp->p[i] * v;
        fp->d[i] = v;
    }
    
    s = sin(2.0*M_PI*x*fp->p[m-2] + fp->p[m-1]);
    c = cos(2.0*M_PI*x*fp->p[m-2] + fp->p[m-1]);
    
    *y += fp->p[m-3] * s;
    
    fp->d[m-3] = s;
    fp->d[m-2] = fp->p[m-3]*c*2.0*M_PI*x;
    fp->d[m-1] = fp->p[m-3]*c;
}

void lm_polysinc(int n, FitData *fd, double *y, FitParameters *fp)
{
    int i, m = fp->nPar;
    double v, a, s, x = fd->x1[n];

    *y = fp->p[0];
    fp->d[0] = 1.0;
    for (i = 1; i < m-3; i++) {
        v = pow(x, (double)i);
        *y += fp->p[i] * v;
        fp->d[i] = v;
    }
    
    a = (x - fp->p[m-2])/fp->p[m-1];
    s = sinc(a);
    
    *y += fp->p[m-3] * s;
    
    fp->d[m-3] = s;
    if (a != 0.0) {
        fp->d[m-2] = fp->p[m-3]*(s - cos(a))/fp->p[m-1]/a;
    } else {
        fp->d[m-2] = 0.0;
    }
    fp->d[m-1] = fp->p[m-3]*(s - cos(a))/fp->p[m-1];
}

void lm_AbsGauss(int n, FitData *fd, double *y, FitParameters *fp)
{
    /* int   i;
    double fac, ex, arg; */
    int i, m = fp->nPar;
    double fac, ex, arg, x = fd->x1[n];
    
    *y = 0.0;
    for (i=0; i<=m-2; i += 3) {
        if (i == 0) {
            arg   = (x - fp->p[1])/fp->p[2];
        } else {
            arg   = (x-(fp->p[1] + fp->p[i+1]))/fp->p[i+2];
        }
        ex        = exp(-arg*arg);
        fac       = fp->p[i]*ex*2.0*arg;
        *y        += fp->p[i]*ex;
        fp->d[i]   = ex;
        fp->d[i+1] = fac/fp->p[i+2];
        if (i > 0) fp->d[1] += fp->d[i+1];
        fp->d[i+2] = fac*arg/fp->p[i+2];
    }
}

void lm_RelGauss(int n, FitData *fd, double *y, FitParameters *fp)
{
    int i, m = fp->nPar;
    double fac, ex, arg, x = fd->x1[n];
    
    *y = 0.0;
    for (i=0; i<=m-2; i += 3) {
        if (i == 0) {
            arg   = (x - fp->p[1])/fp->p[2];
        } else {
            arg   = (x - (fp->p[1] + fp->p[i+1]))/fp->p[2]/fp->p[i+2];
        }
        ex = exp(-arg*arg);
        if (i == 0) {
            fac    = fp->p[0]*ex*2.0*arg;
            *y    += fp->p[0]*ex;
        } else {
            fac    = fp->p[0]*fp->p[i]*ex*2.0*arg;
            *y    += fp->p[0]*fp->p[i]*ex;
        }
        
        if (i == 0) {
            fp->d[0] = ex;
        } else {
            fp->d[i] = fp->p[0]*ex;
            fp->d[0] += fp->p[i]*ex;
        }
        
	    if (i == 0) {
	        fp->d[1] = fac/fp->p[2];
	    } else {
            fp->d[i+1] = fac/fp->p[2]/fp->p[i+2];
            fp->d[1] += fp->d[i+1];
	    }

	    if (i == 0) {
	        fp->d[2] = fac*arg/fp->p[2];
	    } else {
            fp->d[i+2] = fac*arg/fp->p[i+2];
	        fp->d[2] += fac*arg/fp->p[2];
	    }
    }
}

void lm_Gauss3(int n, FitData *fd, double *y, FitParameters *fp)
{
    double ex, arg, x = fd->x1[n];
  
    arg     = ALPHA * (x - fp->p[2]) * (x - fp->p[2]) / fp->p[1] / fp->p[1];
    ex      = exp(-arg);
    *y      = fp->p[0]*ex;
    fp->d[0] = ex;
    fp->d[1] = 2.0 * (*y) * arg / fp->p[1];
    fp->d[2] = (*y) * ALPHA * (x - fp->p[2]) / fp->p[1] / fp->p[1];
}

void lm_Gauss4(int n, FitData *fd, double *y, FitParameters *fp)
{
    double ex, arg, x = fd->x1[n];
  
    arg     = ALPHA * (x - fp->p[2]) * (x - fp->p[2]) / fp->p[1] / fp->p[1];
    ex      = exp(-arg);
    *y      = fp->p[3] + fp->p[0]*ex;
    fp->d[0] = ex;
    fp->d[1] = 2.0 * fp->p[0] * ex * arg / fp->p[1];
    fp->d[2] = fp->p[0] * ex * ALPHA * (x - fp->p[2]) / fp->p[1] / fp->p[1];
    fp->d[3] = 1.0;
}

void lm_Lorentz4(int n, FitData *fd, double *y, FitParameters *fp)
{
    double ex=0.0, arg=0.0, x = fd->x1[n];
  
    if (fp->p[1] != 0.0) {
        arg     = 2.0 * (x - fp->p[2]) / fp->p[1] ;
        ex      = 1.0/(1.0 + arg*arg);
        *y      = fp->p[3] + fp->p[0]*ex;
        fp->d[0] = ex;
        fp->d[1] = 2.0 * fp->p[0] * ex * ex * arg * arg / fp->p[1];
        fp->d[2] = 4.0 * fp->p[0] * ex * ex * arg / fp->p[1];
        fp->d[3] = 1.0;
    } else {
        *y      = fp->p[3];
        fp->d[0] = 0.0;
        fp->d[1] = 0.0;
        fp->d[2] = 0.0;
        fp->d[3] = 1.0;
    }
}

void lm_exp3(int n, FitData *fd, double *y, FitParameters *fp)
{
    double ex, arg, x = fd->x1[n];
  
    arg     = fp->p[1] * (x - fp->p[2]);
    ex      = exp(arg);
    *y      = fp->p[0]*ex;
    fp->d[0] = ex;
    fp->d[1] = (x - fp->p[2]) * (*y);
    fp->d[2] = (-fp->p[1]) * (*y);
}

void lm_erfc3(int n, FitData *fd, double *y, FitParameters *fp)
{
    double er, arg, x = fd->x1[n];
  
    arg     = SQALPHA * (x - fp->p[1])/fp->p[2];
    er      = erfc(arg);
    *y      = fp->p[0]*er;
    fp->d[0] = er;
    fp->d[1] = M_2_SQRTPI * SQALPHA * fp->p[0]/fp->p[2] * exp(-arg*arg);
    fp->d[2] = fp->d[1] * arg;
}

void lm_epro1(int n, FitData *fd, double *y, FitParameters *fp)
{
    double ex = 0.0, arg = 0.0, a1 = 0.0, x = fd->x1[n];
  
    if (fp->p[2] != 0.0) arg = (x - fp->p[1])/fp->p[2];
    
    if (fabs(arg) < 1.0) {
        a1 = 1.0 - arg*arg;
        if (fp->p[3] != 0.0)
            ex = pow(a1, fp->p[3]/2.0);
        else
            ex = 1.0;
    }
    *y = fp->p[0] * ex;
    
    fp->d[0] = ex;
    if (ex == 0.0 || fp->p[2] == 0.0) {
        fp->d[1] = 0.0;
        fp->d[2] = 0.0;
        fp->d[3] = 0.0;
    } else {
        fp->d[1] = (*y) * fp->p[3] * arg / fp->p[2] / a1;
        fp->d[2] = fp->d[1] * arg;
        fp->d[3] = (*y) * log(a1)/2.0;
    }
}

void lm_epro2(int n, FitData *fd, double *y, FitParameters *fp)
{
    double dv, arg = 0.0, I0, I1, I2, x = fd->x1[n];
    ExpArg A;
    
    A.v = x;
    A.T0 = fp->p[0];
    A.v0 = fp->p[1];
    A.ve = fp->p[2];
    A.vt = fp->p[3];
    
    I0 = ExpansionIntegral(A, 0);
    I1 = ExpansionIntegral(A, 1);
    I2 = ExpansionIntegral(A, 2);
      
    *y = A.T0 * I0;
    if (A.vt != 0.0) arg=2.0*A.T0/A.vt/A.vt;
    dv = A.v - A.v0;
    
    fp->d[0] = I0;
    fp->d[1] = arg*(dv*I0 - A.ve*I1);
    fp->d[2] = arg*(dv*I1 - A.ve*I2);
    if (A.vt != 0.0) {
        fp->d[3] = arg*(dv*dv*I0 - 2.0*dv*A.ve*I1 + A.ve*A.ve*I2)/A.vt;
    } else {
        fp->d[3] = 0.0;
    }
}

void lm_epro3(int n, FitData *fd, double *y, FitParameters *fp)
{
    double dv, arg = 0.0, I0, I1, I2, ex = 0.0, x = fd->x1[n];
    ExpArg A;
    
    A.v = x;
    A.T0  = fp->p[0];
    A.v0  = fp->p[1];
    A.ve  = fp->p[2];
    A.vt  = fp->p[3];
    A.tau = fp->p[4];
    
    I0 = ExpansionIntegral(A, 0);
    I1 = ExpansionIntegral(A, 1);
    I2 = ExpansionIntegral(A, 2);
      
    ex = exp(-A.tau * I0);
    
    *y = A.T0 * (1.0 - ex);
    if (A.vt != 0.0) arg=2.0*A.T0*A.tau/A.vt/A.vt;
    dv = A.v - A.v0;
    
    fp->d[0] = 1.0 - ex;
    fp->d[1] = ex*arg*(dv*I0 - A.ve*I1);
    fp->d[2] = ex*arg*(dv*I1 - A.ve*I2);
    if (A.vt != 0.0) {
        fp->d[3] = ex*arg*(dv*dv*I0 - 2.0*dv*A.ve*I1 + A.ve*A.ve*I2)/A.vt;
    } else {
        fp->d[3] = 0.0;
    }
    fp->d[4] = A.T0 * ex * I0;
}

void lm_comet(int n, FitData *fd, double *y, FitParameters *fp)
{
    double I0, I1, x = fd->x1[n];
    ExpArg A;
    
    A.v = x;
    A.T0 = fp->p[0];
    A.v0 = fp->p[1];
    
    I0 = CometIntegral(A, 0);
    I1 = CometIntegral(A, 1);
      
    *y = A.T0 * I0;
    
    fp->d[0] = I0;
    fp->d[1] = A.T0 * I1;
}

void lm_discgauss(int n, FitData *fd, double *y, FitParameters *fp)
{
    double I01, I03, I12, p, q, r, s, x = fd->x1[n];
    ExpArg A;
    
    A.v  = x;
    A.T0 = fp->p[0];
    A.R  = fp->p[1];
    A.B  = fp->p[2];
    
    I01 = DiscIntegral(A, 0);
    I03 = DiscIntegral(A, 1);
    I12 = DiscIntegral(A, 2);
      
    *y = A.T0 * I01;
    
    r = s = 0.0;
    if (A.B != 0.0) {
      r = A.R/A.B;
      s = ALPHA/A.B/A.B;
    }
    p = s*A.R;
    q = s*x;
    
    fp->d[0] = I01;
    fp->d[1] = fp->d[2] = 0.0;
    if (A.R != 0.0)
      fp->d[1] = 2.0*A.T0*(I01/A.R - p*I03 + q*I12);
    if (A.B != 0.0)
      fp->d[2] = 2.0*A.T0*(I01*(s - 1.0)/A.B + p*r*I03 - p*x*I12);
}

void lm_Fourier(int n, FitData *fd, double *y, FitParameters *fp)
{
    double arg=0.0, x = fd->x1[n];
  
    *y = fp->p[1]/2.0;
    fp->d[1] = 0.5;
    
    if (fp->p[0] != 0.0) arg = 2.0*M_PI*x/fp->p[0];
    
    fp->d[2] = cos(arg);     fp->d[3] = sin(arg);
    fp->d[4] = cos(2.0*arg); fp->d[5] = sin(2.0*arg);
    fp->d[6] = cos(3.0*arg); fp->d[7] = sin(3.0*arg);
    fp->d[8] = cos(4.0*arg); fp->d[9] = sin(4.0*arg);
    
    *y += fp->p[2]*fp->d[2] + fp->p[3]*fp->d[3];
    *y += fp->p[4]*fp->d[4] + fp->p[5]*fp->d[5];
    *y += fp->p[6]*fp->d[6] + fp->p[7]*fp->d[7];
    *y += fp->p[8]*fp->d[8] + fp->p[9]*fp->d[9];
    
    if (fp->p[0] != 0.0) {
        fp->d[0]  = 1.0*(fp->p[2]*fp->d[3] - fp->p[3]*fp->d[2]);
        fp->d[0] += 2.0*(fp->p[4]*fp->d[5] - fp->p[5]*fp->d[4]);
        fp->d[0] += 3.0*(fp->p[6]*fp->d[7] - fp->p[7]*fp->d[6]);
        fp->d[0] += 4.0*(fp->p[8]*fp->d[9] - fp->p[9]*fp->d[8]);
        fp->d[0] *= arg/fp->p[0];
    } else {
        fp->d[0] = 0.0;
    }
}

/* These constants are valid for HC17O+(J=1-0) (or any I=5/2) */
static double Q1_cnst=-0.05;  /* F=7/2-5/2 */
static double Q3_cnst= 0.16;  /* F=5/2-5/2 */
static double Q5_cnst=-0.14;  /* F=3/2-5/2 */
/* a[0] = Amplitude of strongest 5/5 component
   a[1] = Relative amplitude of 3/5 component
   a[2] = Relative amplitude of 1/5 component
   a[3] = Quadrupole group centre frequency              
   a[4] = eQq, quadrupole coupling constant  
   a[5] = Line width                          */
void lm_Quadrupole(int n, FitData *fd, double *y, FitParameters *fp)
{
    double e1=0.0, e3=0.0, e5=0.0, a1=0.0, a3=0.0, a5=0.0;
    double c = 0.0, x = fd->x1[n];

    if (fp->p[5] != 0.0) {
        a1 = (x - (fp->p[3] + fp->p[4]*Q1_cnst))/fp->p[5];  
        a3 = (x - (fp->p[3] + fp->p[4]*Q3_cnst))/fp->p[5];  
        a5 = (x - (fp->p[3] + fp->p[4]*Q5_cnst))/fp->p[5];
        e1 = exp(-ALPHA * a1*a1);
        e3 = exp(-ALPHA * a3*a3);
        e5 = exp(-ALPHA * a5*a5);
        c = 2.0*ALPHA*fp->p[0]/fp->p[5];
    }
    
    *y = fp->p[0]*(e1 + fp->p[1]*e3 + fp->p[2]*e5);
    fp->d[0] = (e1 + fp->p[1]*e3 + fp->p[2]*e5);
    fp->d[1] = fp->p[0]*e3;
    fp->d[2] = fp->p[0]*e5;
    fp->d[3] = c*(e1*a1 + fp->p[1]*e3*a3 + fp->p[2]*e5*a5);
    fp->d[4] = c*(Q1_cnst*e1*a1 + Q3_cnst*fp->p[1]*e3*a3 +
                  Q5_cnst*fp->p[2]*e5*a5);
    fp->d[5] = c*(e1*a1*a1 + fp->p[1]*e3*a3*a3 + fp->p[2]*e5*a5*a5);
}

double QuadrupoleValue(double x, double *a, int nPar)
{
    double e1=0.0, e3=0.0, e5=0.0, a1=0.0, a3=0.0, a5=0.0;
    double y=0.0;
    
    if (a[5] != 0.0) {
        a1 = (x - (a[3] + a[4]*Q1_cnst))/a[5];  
        a3 = (x - (a[3] + a[4]*Q3_cnst))/a[5];  
        a5 = (x - (a[3] + a[4]*Q5_cnst))/a[5];
        e1 = exp(-ALPHA * a1*a1);
        e3 = exp(-ALPHA * a3*a3);
        e5 = exp(-ALPHA * a5*a5);
        
        y = a[0]*(e1 + a[1]*e3 + a[2]*e5);
    }
    
    return y;
}

double LorentzValue(double x, double *a, int nPar)
{
    double y, arg=0.0;
    
    if (a[1] != 0.0) arg = 2.0*(x-a[2])/a[1];
    
    y = a[3] + a[0]/(1.0 + arg*arg);
    
    return y;
}

double CometValue(double p, double *a, int nPar)
{
    ExpArg A;
    
    if (nPar != 2) return 0;
    
    A.v = p;
    A.T0 = a[0];
    A.v0 = a[1];

    return A.T0 * CometIntegral(A, 0);
}

double ExpProfileValue(double v, double *a, int nPar)
{
    ExpArg A;
    
    if (nPar != 4) return 0;
    
    A.v = v;
    A.T0 = a[0];
    A.v0 = a[1];
    A.ve = a[2];
    A.vt = a[3];
    
    return A.T0 * ExpansionIntegral(A, 0);
}

double ExpProfileValue3(double v, double *a, int nPar)
{
    ExpArg A;
    
    if (nPar != 5) return 0;
    
    A.v = v;
    A.T0  = a[0];
    A.v0  = a[1];
    A.ve  = a[2];
    A.vt  = a[3];
    A.tau = a[4];
    
    return A.T0 * (1.0 - exp(-A.tau * ExpansionIntegral(A, 0)));
}

double DiscGaussValue(double x, double *a, int nPar)
{
    ExpArg A;
    
    if (nPar != 3) return 0;
    
    A.v = x;
    A.T0 = a[0];
    A.R  = a[1];
    A.B  = a[2];
    
    return A.T0 * DiscIntegral(A, 0);
}

double FourierValue(double x, double *a, int nPar)
{
    double y, A;
    
    if (nPar != 10) return 0;
    
    y = a[1]/2.0;
    if (a[0] == 0.0) return y;
    
    A = 2.0*M_PI*x/a[0];
    
    y += a[2]*cos(A)     + a[3]*sin(A);
    y += a[4]*cos(2.0*A) + a[5]*sin(2.0*A);
    y += a[6]*cos(3.0*A) + a[7]*sin(3.0*A);
    y += a[8]*cos(4.0*A) + a[9]*sin(4.0*A);
    
    return y;
}

#define RADTODEG (180.0/M_PI)

double AzPoi2DValue(double x, double y, double *a, int nPar)
{
    double dA, cE, sE, cA, sA, s2A, c2A, c3A;
    
    if (nPar != 18) return 0.0;
    
    cE = cos(y/RADTODEG);
    sE = sin(y/RADTODEG);
    cA = cos(x/RADTODEG);
    sA = sin(x/RADTODEG);
    c2A = cos(2.0*x/RADTODEG);
    s2A = sin(2.0*x/RADTODEG);
    c3A = cos(3.0*x/RADTODEG);
    
    dA = -a[0] * cE;          /* IA	   */
    dA += -a[2] * sE;         /* NPAE	   */
    dA += -a[3];              /* CA	   */
    dA += -a[4] * sA * sE;    /* AN	   */
    dA += -a[5] * cA * sE;    /* AW	   */
    dA += -a[7] * sA * cE;     /* HASA	   */
    dA += a[8] * cA * cE;     /* HACA	   */
    dA += a[11] * s2A * cE;   /* HASA2     */
    dA += -a[12] * c2A * cE;   /* HACA2     */
    dA += a[16] * c3A * cE;   /* HACA3     */
    
    return dA;
}

double ElPoi2DValue(double x, double y, double *a, int nPar)
{
    double dE, cE, sE, cA, sA, s2A, c2A, s3A, c3A;
    
    if (nPar != 18) return 0.0;
    
    cE = cos(y/RADTODEG);
    sE = sin(y/RADTODEG);
    cA = cos(x/RADTODEG);
    sA = sin(x/RADTODEG);
    c2A = cos(2.0*x/RADTODEG);
    s2A = sin(2.0*x/RADTODEG);
    c3A = cos(3.0*x/RADTODEG);
    s3A = sin(3.0*x/RADTODEG);
    
    dE = a[1];              /* IE        */
    dE += -a[4] * cA;       /* AN        */
    dE += a[5] * sA;        /* AW        */
    dE += a[6] * cE;        /* HECE      */
    dE += a[9] * sE;        /* HESE      */
    dE += a[10] * sA;       /* HESA      */
    dE += -a[13] * s2A;      /* HESA2     */
    dE += a[14] * c2A;      /* HECA2     */
    dE += -a[15] * c3A;      /* HECA3     */
    dE += a[17] * s3A;      /* HESA3     */
    
    return dE;
}

void lm_Holo(int n, FitData *fd, double *z, FitParameters *fp)
{
    double r2, d, A, P;
    double x = fd->x1[n], y = fd->x2[n];
    double Fprim, Fmag, Fm;

    Fprim = 4.8;
    Fmag = 20.0;
    Fm = Fprim*Fmag;
    
    r2 = x*x + y*y;
    
    *z = fp->p[0] + fp->p[1]*x + fp->p[2]*y;
    
    fp->d[0] = 1.0;
    fp->d[1] = x;
    fp->d[2] = y;
    
    A = r2/4.0/Fprim/Fprim;
    P = r2/4.0/Fm/Fm;
    d = 4.0 * M_PI * (A/(1.0+A) + P/(1.0+P));
    *z += fp->p[3] * d;
    fp->d[3] = d;
    
    fp->d[4] = 0.0;
    
    /* full astigmatism */
    *z += fp->p[5]*x*x + fp->p[6]*y*y + fp->p[7]*x*y;
    fp->d[5] = x*x;
    fp->d[6] = y*y;
    fp->d[7] = x*y;
    
    /* coma */
    *z += fp->p[8]*r2*x + fp->p[9]*r2*y;
    fp->d[8] = r2*x;
    fp->d[9] = r2*y;
}

void lm_Gauss2D(int n, FitData *fd, double *y, FitParameters *fp)
{
    double x0, y0, u, v, ua, va, wu, wv, cp, sp, ex=0.0;
    double x1 = fd->x1[n], x2 = fd->x2[n];
    
    cp = cos(fp->p[5]); sp = sin(fp->p[5]);
    
    x0 = x1 - fp->p[1]; y0 = x2 - fp->p[2];
    
    u = (x0*cp - y0*sp);
    v = (y0*cp + x0*sp);
    
    if (fp->p[3] == 0.0 || fp->p[4] == 0.0) {
        *y = 0.0;
        fp->d[0] = fp->d[1] = fp->d[2] = fp->d[3] = fp->d[4] = fp->d[5] = 0.0;
    } else {
        wu = 1.0/fp->p[4]/fp->p[4];
        wv = 1.0/fp->p[3]/fp->p[3];

        ua = ALPHA * u * u * wu;
        va = ALPHA * v * v * wv;

        ex = exp(-ua-va);

        *y = fp->p[0] * ex;

        fp->d[0] = ex;
        fp->d[1] = 2.0 * (*y) * ALPHA * ( u*cp*wu + v*sp*wv);
        fp->d[2] = 2.0 * (*y) * ALPHA * (-u*sp*wu + v*cp*wv);
        fp->d[3] = 2.0 * (*y) * va / fp->p[3];
        fp->d[4] = 2.0 * (*y) * ua / fp->p[4];
        fp->d[5] = 2.0 * (*y) * ALPHA * u * v * (wu - wv);
    }
}

void lm_Gauss2DCirc(int n, FitData *fd, double *y, FitParameters *fp)
{
    double arg, ax=0.0, ay=0.0, ex = 0.0;
    double x1 = fd->x1[n], x2 = fd->x2[n];
   
    if (fp->p[3] != 0.0) {
        ax = (x1 - fp->p[1])/fp->p[3];
        ay = (x2 - fp->p[2])/fp->p[3];
    }
    
    arg = ALPHA * (ax*ax + ay*ay);
    
    ex = exp(-arg);
    
    *y = fp->p[0] * ex;
    
    fp->d[0] = ex;
    if (fp->p[3] != 0.0) {
        fp->d[1] = 2.0 * (*y) * ALPHA * ax / fp->p[3];
        fp->d[2] = 2.0 * (*y) * ALPHA * ay / fp->p[3];
        fp->d[3] = 2.0 * (*y) * arg / fp->p[3];
    } else {
        fp->d[1] = 0.0;
        fp->d[2] = 0.0;
        fp->d[3] = 0.0;
    }
}

void lm_Gauss2DRing(int n, FitData *fd, double *y, FitParameters *fp)
{
    double arg, ax, ay, r, ar=0.0, ex = 0.0;
    double x1 = fd->x1[n], x2 = fd->x2[n];
   
    ax = (x1 - fp->p[1]);
    ay = (x2 - fp->p[2]);
    r = sqrt(ax*ax + ay*ay);
    
    if (fp->p[3] != 0.0) {
        ar = (r - fp->p[4])/fp->p[3];
    }
    
    arg = ALPHA * ar * ar;
    
    ex = exp(-arg);
    
    *y = fp->p[0] * ex;
    
    fp->d[0] = ex;
    if (fp->p[3] != 0.0) {
        if (r != 0.0) {
            fp->d[1] = 2.0 * (*y) * ALPHA * ar * ax / fp->p[3] / r;
            fp->d[2] = 2.0 * (*y) * ALPHA * ar * ay / fp->p[3] / r;
        } else {
            fp->d[1] = 2.0 * (*y) * ALPHA * ar / fp->p[3];
            fp->d[2] = 2.0 * (*y) * ALPHA * ar / fp->p[3];
        }
        fp->d[3] = 2.0 * (*y) * arg / fp->p[3];
        fp->d[4] = 2.0 * (*y) * ALPHA * ar / fp->p[3];
    } else {
        fp->d[1] = 0.0;
        fp->d[2] = 0.0;
        fp->d[3] = 0.0;
        fp->d[4] = 0.0;
    }
}

void lm_Plane2D(int n, FitData *fd, double *y, FitParameters *fp)
{
    double x1 = fd->x1[n], x2 = fd->x2[n];
    
    *y = fp->p[0] + fp->p[1] * x1 + fp->p[2] * x2;
    
    fp->d[0] = 1.0;
    fp->d[1] = x1;
    fp->d[2] = x2;
}

void lm_Quad2D(int n, FitData *fd, double *y, FitParameters *fp)
{
    double x1 = fd->x1[n], x2 = fd->x2[n];
    
    *y = fp->p[0] + fp->p[1] * x1 + fp->p[2] * x2;
    *y += fp->p[3] *x1*x1 + fp->p[4] *x2*x2 + fp->p[5] *x1*x2;
    
    fp->d[0] = 1.0;
    fp->d[1] = x1;
    fp->d[2] = x2;
    fp->d[3] = x1*x1;
    fp->d[4] = x2*x2;
    fp->d[5] = x1*x2;
}

void lm_apexpoiA(int n, FitData *fd, double *y, FitParameters *fp)
{
    double A = fd->x1[n]/RADTODEG, E = fd->x2[n]/RADTODEG;
    double dA, cE, sE, cA, sA, s2A, c2A, c3A;
    
    cE = cos(E);
    sE = sin(E);
    cA = cos(A);
    sA = sin(A);
    c2A = cos(2.0*A);
    s2A = sin(2.0*A);
    c3A = cos(3.0*A);
    
    dA = -fp->p[0] * cE;          /* IA        */
    dA += -fp->p[2] * sE;         /* NPAE      */
    dA += -fp->p[3];              /* CA        */
    dA += -fp->p[4] * sA * sE;    /* AN        */
    dA += -fp->p[5] * cA * sE;    /* AW        */
    dA += -fp->p[7] * sA * cE;     /* HASA      */
    dA += fp->p[8] * cA * cE;     /* HACA      */
    dA += fp->p[11] * s2A * cE;   /* HASA2     */
    dA += -fp->p[12] * c2A * cE;   /* HACA2     */
    dA += fp->p[16] * c3A * cE;   /* HACA3     */
    
    *y = dA;
    
    fp->d[0] = -cE;
    fp->d[2] = -sE;
    fp->d[3] = -1.0;
    fp->d[4] = -sA * sE;
    fp->d[5] = -cA * sE;
    fp->d[7] = -sA * cE;
    fp->d[8] = cA * cE;
    fp->d[11]= s2A * cE;
    fp->d[12]= -c2A * cE;
    fp->d[16]= c3A * cE;
}

void lm_apexpoiE(int n, FitData *fd, double *y, FitParameters *fp)
{
    double A = fd->x1[n]/RADTODEG, E = fd->x2[n]/RADTODEG;
    double dE, cE, sE, cA, sA, c2A, s2A, c3A, s3A;
    
    cE = cos(E);
    sE = sin(E);
    cA = cos(A);
    sA = sin(A);
    c2A = cos(2.0*A);
    s2A = sin(2.0*A);
    c3A = cos(3.0*A);
    s3A = sin(3.0*A);
    
    dE = fp->p[1];              /* IE        */
    dE += -fp->p[4] * cA;       /* AN        */
    dE += fp->p[5] * sA;        /* AW        */
    dE += fp->p[6] * cE;        /* HECE      */
    dE += fp->p[9] * sE;        /* HESE      */
    dE += fp->p[10] * sA;       /* HESA      */
    dE += -fp->p[13] * s2A;      /* HESA2     */
    dE += fp->p[14] * c2A;      /* HECA2     */
    dE += -fp->p[15] * c3A;      /* HECA3     */
    dE += fp->p[17] * s3A;      /* HESA3     */
    
    *y = dE;
    
    fp->d[1] = 1.0;
    fp->d[4] = -cA;
    fp->d[5] = sA;
    fp->d[6] = cE;
    fp->d[9] = sE;
    fp->d[10]= sA;
    fp->d[13]= -s2A;
    fp->d[14]= c2A;
    fp->d[15]= -c3A;
    fp->d[17]= s3A;
}
