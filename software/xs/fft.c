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
#include "fftn.h"

/*** External variables ***/
extern VIEW *vP;

void UpdateData(int, int);
void PostWarningDialog(Widget, char *);
void PostErrorDialog(Widget, char *);
void send_line(char *);

list       scan_iterator(list, DataSetPtr);
scanPtr    copy_scanheader(DataSetPtr, int, scanPtr);
list      *get_listlist(), *get_scatterlist(), *get_maplist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);
void       DeleteLastDataSet();
scatter   *new_scatter(list *, int);
void       fill_scatter(scatter *, scatter *);
MAP       *new_map(list *, int, int);
void       fill_map(MAP *, MAP *);

/*** Local variables ***/

#define FFT_INVERSE 0
#define FFT_FOR_CAR 1
#define FFT_FOR_POL 2
#define FFT_CHACOR  3

static scanPtr fft_scan(char *code, DataSetPtr dsp, scanPtr s)
{
    int m, i, inverse = 0, polar = 0, mlim, err = 0;
    int dims[1];
    string buf;
    scanPtr new = NULL;
    static int size = 0;
    static double *d, *e;
    
    if (code[0] == 'i') inverse = 1;
    if (code[0] == 'p') polar = 1;

    if (inverse && s->fft == FFT_INVERSE) {
        sprintf(buf, "iFFT: This is not forward FFT data!\n");
        send_line(buf);
        return NULL;
    }

    if (inverse)
        mlim = s->nfft;
    else
        mlim = s->nChan;

    m = 1;                            /* m of the form 2^n */
    while (m < mlim) m *= 2;

    if (m != size) {                        /* Need to allocate memory? */
        if (!size) {
            d = (double *)malloc(m * sizeof(double));
            e = (double *)malloc(m * sizeof(double));
        } else {
            d = (double *)realloc(d, m * sizeof(double));
            e = (double *)realloc(e, m * sizeof(double));
        }
        if (!d || !e) {
            sprintf(buf, "FFT: Couldn't allocate data vector: size=%d\n", m);
            send_line(buf);
            size = 0;
            return NULL;
        }
        size = m;
    }
#ifdef DEBUG
    if (inverse)
        printf("Inverse: m=%4d(%4d) nChan=%d nfft=%d\n",
               m, size, s->nChan, s->nfft);
    else
        printf("Forward: m=%4d(%4d) nChan=%d\n", m, size, s->nChan);
#endif
    dims[0] = m;
    if (inverse) {                   /* Inverse FFT */
        new = copy_scanheader(dsp, s->nfft, s);
        if (!new) return NULL;
        for (i=0; i<s->nChan; i++) {
            if (s->fft == FFT_FOR_CAR) {
                d[i] = s->d[i];
                e[i] = s->e[i];
            } else if (s->fft == FFT_FOR_POL) {
                d[i] = s->d[i]*cos(s->e[i]);
                e[i] = s->d[i]*sin(s->e[i]);
            }
            if (i == 0) continue;
            /* Set up the mirror values for the Real FFT case */
            d[m - i]= d[i];
            e[m - i]= -e[i];
        }
        
        err = fftn(1, dims, d, e, -1, (double)m);
        if (err) return NULL;
        
        new->fft = FFT_INVERSE;
        new->nfft = 0;
        for (i=0; i<new->nChan; i++) {
            new->d[i] = d[i];
            new->e[i] = 1.0;
        }
    } else {                                /* Forward FFT */
        new = copy_scanheader(dsp, m/2+1, s);
        if (!new) return NULL;
        for (i=0; i<s->nChan; i++) {    /* Fill d with the data    */
             d[i] = s->d[i];
             e[i] = 0.0;
        }
        for (i=s->nChan; i<m; i++) {    /* Pad the rest with zeros */
             d[i] = e[i] = 0.0;
        }
        err = fftn(1, dims, d, e, 1, 0.0);
        if (err) return NULL;
        for (i=0; i<new->nChan; i++) {
            if (polar) {
                new->d[i] = sqrt(d[i]*d[i] + e[i]*e[i]);
                new->e[i] = atan2(e[i], d[i]);
            } else{
                new->d[i] = d[i];     /* Real part      */
                new->e[i] = e[i];     /* Imaginary part */
            }
        }
        new->nfft = s->nChan;
        if (!polar) {
            new->fft = FFT_FOR_CAR;
        } else {
            new->fft = FFT_FOR_POL;
        }
    }
    
    fft_free();
    
    return new;
}

scatter *fft_scatter(char *code, scatter *p)
{
    int m, i, inverse = 0, polar = 0, mlim, err = 0;
    int dims[1];
    string buf;
    scatter *new = NULL;
    static int size = 0;
    static double *d, *e;
    
    if (code[0] == 'i') inverse = 1;
    if (code[0] == 'p') polar = 1;

    if (inverse && p->fft == FFT_INVERSE) {
        sprintf(buf, "iFFT: This is not forward FFT data!\n");
        send_line(buf);
        return NULL;
    }


    if (inverse)
        mlim = p->nfft;
    else
        mlim = p->nData;

    m = 1;                            /* m of the form 2^n */
    while (m < mlim) m *= 2;

    if (m != size) {                        /* Need to allocate memory? */
        if (!size) {
            d = (double *)malloc(m * sizeof(double));
            e = (double *)malloc(m * sizeof(double));
        } else {
            d = (double *)realloc(d, m * sizeof(double));
            e = (double *)realloc(e, m * sizeof(double));
        }
        if (!d || !e) {
            sprintf(buf, "FFT: Couldn't allocate data vector: size=%d\n", m);
            send_line(buf);
            size = 0;
            return NULL;
        }
        size = m;
    }
#ifdef DEBUG
    if (inverse)
        printf("Inverse: m=%4d(%4d) nData=%d\n", m, size, p->nData);
    else
        printf("Forward: m=%4d(%4d) nData=%d\n", m, size, p->nData);
#endif
    dims[0] = m;
    if (inverse) {                   /* Inverse FFT */
        new = new_scatter(get_scatterlist(), p->nfft);
        if (!new) return NULL;
        fill_scatter(new, p); 
        for (i=0; i<p->nData; i++) {
            if (p->fft == FFT_FOR_CAR) {
                d[i] = p->y[i];
                e[i] = p->ey[i];
            } else if (p->fft == FFT_FOR_POL) {
                d[i] = p->y[i]*cos(p->ey[i]);
                e[i] = p->y[i]*sin(p->ey[i]);
            }
            if (i == 0) continue;
            /* Set up the mirror values for the Real FFT case */
            d[m - i] = d[i];
            e[m - i] = -e[i];
        }
        
        err = fftn(1, dims, d, e, -1, (double)m);
        if (err) return NULL;
        
        new->fft = FFT_INVERSE;
        new->nfft = 0;
        
        for (i=0; i<new->nData; i++) {
            new->y[i] = d[i];
            new->ey[i] = 1.0;
            new->x[i] = (double)i;
            new->ex[i] = 0.0;
        }
    } else {                                /* Forward FFT */
        new = new_scatter(get_scatterlist(), m/2+1);
        if (!new) return NULL;
        fill_scatter(new, p); 

        for (i=0; i<p->nData; i++) {   /* Fill d with the data    */
             d[i] = p->y[i];
             e[i] = 0.0;
        }
        for (i=p->nData; i<m; i++) {   /* Pad the rest with zeros */
             d[i] = e[i] = 0.0;
        }
        
        err = fftn(1, dims, d, e, 1, 0.0);
        if (err) return NULL;
        
        for (i=0; i<new->nData; i++) {
            if (polar) {
                new->y[i]  = sqrt(d[i]*d[i] + e[i]*e[i]);
                new->ey[i] = atan2(e[i], d[i]);
            } else {
                new->y[i]  = d[i];     /* Real part      */
                new->ey[i] = e[i];     /* Imaginary part */
            }
            new->x[i] = (double)i;
            new->ex[i] = 0.0;
        }
        new->nfft = p->nData;
        if (!polar) {
            new->fft = FFT_FOR_CAR;
        } else {
            new->fft = FFT_FOR_POL;
        }
    }
    
    fft_free();
    
    return new;
}

MAP *fft_map(char *code, MAP *m)
{
    int k, i, j, inverse = 0, polar = 0, mlimx, mlimy, err = 0;
    int nX, nY, dims[2];
    string buf;
    MAP *new = NULL;
    static int size = 0;
    static double *d, *e;
    
    void MapDraw(Widget, MAP *, XmListCallbackStruct *);

    if (code[0] == 'i') inverse = 1;
    if (code[0] == 'p') polar = 1;

    if (inverse && m->fft == FFT_INVERSE) {
        sprintf(buf, "iFFT: This is not forward FFT data!\n");
        send_line(buf);
        return NULL;
    }


    if (inverse) {
        mlimx = m->nfftx;
        mlimy = m->nffty;
    } else {
        mlimx = m->i_no;
        mlimy = m->j_no;
    }

    k = 1;                            /* nX of the form 2^n */
    while (k < mlimx) k *= 2;
    nX = k;

    k = 1;                            /* nY of the form 2^n */
    while (k < mlimy) k *= 2;
    nY = k;

    if (nX*nY != size) {              /* Need to allocate memory? */
        if (!size) {
            d = (double *)malloc(nX*nY * sizeof(double));
            e = (double *)malloc(nX*nY * sizeof(double));
        } else {
            d = (double *)realloc(d, nX*nY * sizeof(double));
            e = (double *)realloc(e, nX*nY * sizeof(double));
        }
        if (!d || !e) {
            sprintf(buf, "FFT: Couldn't allocate data vector: size=%d\n",
                    nX*nY);
            send_line(buf);
            size = 0;
            return NULL;
        }
        size = nX*nY;
    }
#ifdef DEBUG
    if (inverse)
        printf("Inverse: nX,nY=%4d,%4d(%4d) i_no,j_no=%d,%d\n",
               nX, nY, size, m->i_no, m->j_no);
    else
        printf("Forward: nX,nY=%4d,%4d(%4d) i_no,j_no=%d,%d\n",
               nX, nY, size, m->i_no, m->j_no);
#endif
    dims[0] = nX;
    dims[1] = nY;
    if (inverse) {                   /* Inverse FFT */
        new = new_map(get_maplist(), m->nfftx, m->nffty);
        if (!new) return NULL;
        fill_map(new, m);
        for (j=0; j<m->j_no; j++) {
            for (i=0; i<m->i_no; i++) {
                k = j * m->i_no + i;
                if (m->fft == FFT_FOR_POL || m->fft == -2) {
                    d[k] = m->d[i][j] * cos(m->e[i][j]);
                    e[k] = m->d[i][j] * sin(m->e[i][j]);
                } else {
                    d[k] = m->d[i][j];
                    e[k] = m->e[i][j];
                }
            }
        }
        
        err = fftn(2, dims, d, e, -1, (double)(m->i_no * m->j_no));
        if (err) return NULL;
        
        new->fft = FFT_INVERSE;
        new->nfftx = new->nffty = 0;
        for (j=0; j<new->j_no; j++) {
            for (i=0; i<new->i_no; i++) {
                k = j * m->i_no + i;
		if (m->fft == -2) {
                  new->d[i][j] = sqrt(d[k]*d[k] + e[k]*e[k]);
                  new->e[i][j] = atan2(e[k], d[k]);
		} else {
                  new->d[i][j] = d[k];
                  new->e[i][j] = 1.0;
		}
            }
        }
    } else {                                /* Forward FFT */
        new = new_map(get_maplist(), nX, nY);
        if (!new) return NULL;
        fill_map(new, m);

        for (j=0; j<nY; j++) {
            for (i=0; i<nX; i++) {   /* Fill d with the data    */
                k = j*nX + i;
		if (m->fft == -2) {
                  if (i < m->i_no && j < m->j_no) {
                      d[k] = m->d[i][j] * cos(m->e[i][j]);
                      e[k] = m->d[i][j] * sin(m->e[i][j]);
                  } else {
                      d[k] = e[k] = 0.0;
                  }
		} else if (m->fft == -1) {
                  if (i < m->i_no && j < m->j_no) {
                      d[k] = m->d[i][j];
                      e[k] = m->e[i][j];
                  } else {
                      d[k] = e[k] = 0.0;
                  }
		} else {
                  if (i < m->i_no && j < m->j_no) {
                      d[k] = m->d[i][j];
                  } else {
                      d[k] = 0.0;
                  }
                  e[k] = 0.0;
		}
            }
        }
        
        err = fftn(2, dims, d, e, 1, 0.0);
        if (err) return NULL;
        
        for (j=0; j<nY; j++) {
          for (i=0; i<nX; i++) {
            k = j * nX + i;
            if (polar) {
                new->d[i][j] = sqrt(d[k]*d[k] + e[k]*e[k]);
                new->e[i][j] = atan2(e[k], d[k]);
            } else {
                new->d[i][j] = d[k];     /* Real part      */
                new->e[i][j] = e[k];     /* Imaginary part */
            }
          }
        }
        new->nfftx = m->i_no;
        new->nffty = m->j_no;
        if (!polar) {
            new->fft = FFT_FOR_CAR;
        } else {
            new->fft = FFT_FOR_POL;
        }
    }
    
    fft_free();
    
    new->original = m;
    
    MapDraw(NULL, new, NULL);
    
    return new;
}

void swap_spectrum_and_rms(Widget w, char *cmd, XtPointer cd)
{
    int i;
    double tmp;
    list curr = NULL;
    scanPtr s;
    
    while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
        s = (scanPtr)DATA(curr);
        for (i=0; i<s->nChan; i++) {
           tmp = s->d[i];
           s->d[i] = s->e[i];
           s->e[i] = tmp;
        }
    }
    
    UpdateData(SCALE_ONLY_Y, REDRAW);
}

void do_fft(Widget w, char *cmd, XtPointer cd)
{
    list curr = NULL;
    string buf;
    scanPtr new = NULL;
    DataSetPtr dsp;
    
    if (cmd[0] == 'i') {
        sprintf(buf, "Inv. FFT of %s", vP->from->name);
    } else {
        sprintf(buf, "FFT of %s", vP->from->name);
    }
    
    dsp = new_dataset(get_listlist(), buf, vP->from);
    if (!dsp) {
        PostErrorDialog(w, "Out of memory when allocating new dataset.");
        return;
    }
    
    if (vP->mode == SHOW_SPE) {
        new = fft_scan(cmd, dsp, vP->s);
        if (!new) {
            PostWarningDialog(w,
                "An error occurred during FFT.\nSee the message log.");
            DeleteLastDataSet();
            return;
        }
    } else {
        while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
            new = fft_scan(cmd, dsp, (scanPtr)DATA(curr));
            if (!new) break;
        }
    }
    
    if (!new)
        PostWarningDialog(w,
            "An error might have occurred during FFT.\nSee the message log.");
    else {
        vP->to = vP->from = dsp;
        vP->s = new;
    }
    
    UpdateData(SCALE_BOTH, REDRAW);
}

static scanPtr cor_scan(DataSetPtr dsp, scanPtr s)
{
    int i, n;
    double sx, sx2, sy, sy2, sxy;
    scanPtr new = NULL;
    
    new = copy_scanheader(dsp, s->nChan-5, s);
    if (!new) return NULL;

    for (n=0; n<s->nChan-5; n++) {
        i = 0;
        sx = sx2 = sy = sy2 = sxy = 0.0;
        while (i+n < s->nChan) {
            sx  += s->d[i];
            sx2 += s->d[i] * s->d[i];
            sy  += s->d[i+n];
            sy2 += s->d[i+n] * s->d[i+n];
            sxy += s->d[i] * s->d[i+n];
            i++;
        }
        new->d[n] = ((double)i * sxy - sx*sy)/
                    sqrt(((double)i*sx2 - sx*sx)*((double)i*sy2 - sy*sy));
        new->e[n] = 0.0;
    }
    
    return new;
}

static scanPtr AddCor(int n, int m, int ns, DataSetPtr dsp, scanPtr s1, scanPtr s2)
{
    static int n_last=0, k=0, nsum=0;
    int i, j, n_new=0;
    static scanPtr new = NULL;
    static double **sum;
    
    void FreeDoubleArray(double **, int, int);
    double **AllocDoubleArray(int, int);
    
    if (!dsp) {
        FreeDoubleArray(sum, 5, nsum);
        sum = NULL;
        nsum = n_last = k = 0;
        return NULL;
    }
    if (n == n_last + 1) {
        n_new = 1;
        n_last++;
    }
    if (n_new) {
        /* printf("New=%d\n", n); */
        new = copy_scanheader(dsp, s1->nChan, s1);
        if (!new) return new;
        sum = AllocDoubleArray(5, s1->nChan);
        nsum = s1->nChan;
        for (j=0; j<5; j++) {
            for (i=0; i<nsum; i++) sum[j][i] = 0.0;
        }
        k = 0;
        sprintf(new->molecule, "N=%3d", m);
    }
    k++;
    /* printf("%2d %2d %4d-%4d (%3d, %2d)\n",
           n, n+m, s1->scan_no, s2->scan_no, ns, k); */
    for (i=0; i<nsum; i++) {
        if (i >= s2->nChan || i>= s1->nChan) break;
        sum[0][i] += s1->d[i];
        sum[1][i] += s1->d[i] * s1->d[i];
        sum[2][i] += s2->d[i];
        sum[3][i] += s2->d[i] * s2->d[i];
        sum[4][i] += s1->d[i] * s2->d[i];
        if (k < 2) continue;
        new->d[i] = ((double)k * sum[4][i] - sum[0][i]*sum[2][i])/
                    sqrt(((double)k*sum[1][i] - sum[0][i]*sum[0][i])*
                    ((double)k*sum[3][i] - sum[2][i]*sum[2][i]));
        new->e[i] = 0.0;
    }
    return new;
}

void do_cor(Widget w, char *cmd, XtPointer cd)
{
    int seq = 0, nseq=0, n, m;
    list curr = NULL, next;
    string buf;
    scanPtr new = NULL;
    DataSetPtr dsp;
    
    int count_scans(DataSetPtr);
    
    if (strcmp(cmd, "seq")==0) {
        seq=1;
        sprintf(buf, "Sequence cor. of %s", vP->from->name);
    } else {
        sprintf(buf, "Channel cor. of %s", vP->from->name);
    }
    
    if (seq) {
        nseq = count_scans(vP->from);
        if (nseq < 5) {
            PostErrorDialog(w,
    "Cannot correlate a sequence of scans when\nno of scans is less than 5.");
            return;
        }
    }

    dsp = new_dataset(get_listlist(), buf, vP->from);
    if (!dsp) {
        PostErrorDialog(w, "Out of memory when allocating new dataset.");
        return;
    }
    
    if (seq) {
        for (n=1;n<=nseq/3;n++) {
            curr = NULL;
            while ( (curr = scan_iterator(curr, vP->from)) ) {
                m = 1;
                next = curr;
                while ((next = scan_iterator(next, vP->from)) && m < n) {
                    m++;
                }
                if (!next) continue;
                new = AddCor(n, m, nseq/3, dsp, (scanPtr)DATA(curr), (scanPtr)DATA(next));
                if (!new) break;
            }
            if (!new) break;
        }
        AddCor(0, 0, 0, NULL, NULL, NULL);
    } else if (vP->mode == SHOW_SPE) {
        new = cor_scan(dsp, vP->s);
        if (!new) {
            PostWarningDialog(w,
                "An error occurred during correlation.\nSee the message log.");
            DeleteLastDataSet();
            return;
        }
    } else {
        while ( (curr = scan_iterator(curr, vP->from)) != NULL) {
            new = cor_scan(dsp, (scanPtr)DATA(curr));
            if (!new) break;
        }
    }
    
    if (!new)
        PostWarningDialog(w,
 "An error might have occurred during the correlation.\nSee the message log.");
    else {
        vP->to = vP->from = dsp;
        vP->s = new;
    }
    
    UpdateData(SCALE_BOTH, REDRAW);
}
