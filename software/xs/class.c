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

#include "class.h"
#include "defines.h"
#include "global_structs.h"

/* #define DEBUG */
extern VIEW    *vP;

int nst=0, current=0;
CLASS *st = NULL;

void send_line(char *);

static string buf;

static void c4_to_int(char *c4, int *i)
{
    union {
        char c[4];
        int i;
    } b;
    
    if (!c4) return;
    if (!i) return;
    
    memcpy(b.c, c4, 4);
    *i = b.i;
}

static void c4_to_float(char *c4, float *f)
{
    union {
        char c[4];
        float f;
    } b;
    
    if (!c4) return;
    if (!f) return;
    
    memcpy(b.c, c4, 4);
    *f = b.f;
}

static void c8_to_double(char *c8, double *d)
{
    union {
        char c[8];
        double d;
    } b;
    
    if (!c8) return;
    if (!d) return;
    
    memcpy(b.c, c8, 8);
    *d = b.d;
}


static void cn_to_char(char *cn, char *c, int n)
{
    
    if (!cn) return;
    if (!c) return;
    
    memcpy(c, cn, n);
    c[n] = '\0';
}

static void fill_class_table(CLASS *o, char *i)
{
    int n;
   
    if (!o) return;
    if (!i) return;
    
    n = 0;
    c4_to_int(&i[n], &(o->xbloc));    n += 4;
    c4_to_int(&i[n], &(o->xnum));     n += 4;
    c4_to_int(&i[n], &(o->xver));     n += 4;
    cn_to_char(&i[n], o->xsourc, 12); n += 12;
    cn_to_char(&i[n], o->xline, 12);  n += 12;
    cn_to_char(&i[n], o->xtel, 12);   n += 12;
    c4_to_int(&i[n], &(o->xdobs));    n += 4;
    c4_to_int(&i[n], &(o->xdred));    n += 4;
    c4_to_float(&i[n], &(o->xoff1));  n += 4;
    c4_to_float(&i[n], &(o->xoff2));  n += 4;
    cn_to_char(&i[n], o->xtype, 4);   n += 4;
    c4_to_int(&i[n], &(o->xkind));    n += 4;
    c4_to_int(&i[n], &(o->xqual));    n += 4;
    c4_to_int(&i[n], &(o->xscan));    n += 4;
    c4_to_int(&i[n], &(o->xposa));    n += 4;
    cn_to_char(&i[n], o->xfront, 8);  n += 8;
    cn_to_char(&i[n], o->xback, 8);   n += 8;
    cn_to_char(&i[n], o->xproc, 8);   n += 8;
    cn_to_char(&i[n], o->xproj, 8);   n += 8;
    cn_to_char(&i[n], o->unused, 24); n += 24;
}

static void fill_class_obs(CLASS_SECTION *o, char *i)
{    
    int n, nmax, a;
    
    if (!o) return;
    if (!i) return;
        
    a = 0;
    memcpy(o->ident, &i[a], 4);     a += 4;
    c4_to_int(&i[a], &(o->nbl));    a += 4;
    c4_to_int(&i[a], &(o->bytes));  a += 4;
    c4_to_int(&i[a], &(o->adr));    a += 4;
    c4_to_int(&i[a], &(o->nhead));  a += 4;
    c4_to_int(&i[a], &(o->len));    a += 4;
    c4_to_int(&i[a], &(o->ientry)); a += 4;
    c4_to_int(&i[a], &(o->nsec));   a += 4;
    c4_to_int(&i[a], &(o->obsnum)); a += 4;
    
    nmax = o->nsec;
    if (nmax >= 4) nmax=4;
    
    for (n=0; n<nmax; n++) {
        c4_to_int(&i[a], &(o->sec_cod[n])); a += 4;
    }
    for (n=0; n<nmax; n++) {
        c4_to_int(&i[a], &(o->sec_len[n])); a += 4;
    }
    for (n=0; n<nmax; n++) {
        c4_to_int(&i[a], &(o->sec_adr[n])); a += 4;
    }
}

static int check_block(CLASS *c, int nst, int block_no)
{
    int n;
    
    for (n=1; n<nst; n++) {
        if (c[n].xbloc == block_no) return n;
    }
    
    return 0;
}

static void fill_class_header(int code, int first, int len, char *s, int size, CLASS *c)
{
    int n;
    
    if (!s) return;
    if (!c) return;
    
    n = (first-1)*4;
    if (code == -2) { /* General */
	c8_to_double(&s[n], &(c->g.ut));  n += 8;
	c8_to_double(&s[n], &(c->g.st));  n += 8;
	c4_to_float(&s[n], &(c->g.az));   n += 4;
	c4_to_float(&s[n], &(c->g.el));   n += 4;
	c4_to_float(&s[n], &(c->g.tau));  n += 4;
	c4_to_float(&s[n], &(c->g.tsys)); n += 4;
	c4_to_float(&s[n], &(c->g.time)); n += 4;
	
	if (len > 9) {
	  c4_to_int(&s[n], &(c->g.xunit)); n += 4;
	} else
	  c->g.xunit = 0;
	
#ifdef DEBUG
	printf("    -2  UT=%f %f   Az,El=%f,%f  time=%f\n", c->g.ut, c->g.st, c->g.az, c->g.el, c->g.time);
#endif
    } else if (code == -3) { /* Position */
	cn_to_char(&s[n], c->p.source, 12); n += 12;
	c4_to_float(&s[n], &(c->p.epoch));  n += 4;
 	c8_to_double(&s[n], &(c->p.lam));   n += 8;
	c8_to_double(&s[n], &(c->p.bet));   n += 8;
	c4_to_float(&s[n], &(c->p.lamof));  n += 4;
	c4_to_float(&s[n], &(c->p.betof));  n += 4;
	c4_to_int(&s[n], &(c->p.proj));     n += 4;
 	c8_to_double(&s[n], &(c->p.sl0p));  n += 8;
 	c8_to_double(&s[n], &(c->p.sb0p));  n += 8;
 	c8_to_double(&s[n], &(c->p.sk0p));  n += 8;
	
#ifdef DEBUG
	printf("    -3  '%s' %f Coord:%f,%f (%f,%f) %4d %f,%f,%f\n", c->p.source,
	       c->p.epoch, c->p.lam, c->p.bet, c->p.lamof, c->p.betof, c->p.proj,
	       c->p.sl0p, c->p.sb0p, c->p.sk0p);
#endif
    } else if (code == -4) { /* Spectroscopic */
	cn_to_char(&s[n], c->s.line, 12);     n += 12;
  	c8_to_double(&s[n], &(c->s.restf));   n += 8;
	c4_to_int(&s[n], &(c->s.nchan));      n += 4;
	c4_to_float(&s[n], &(c->s.rchan));    n += 4;
	c4_to_float(&s[n], &(c->s.fres));     n += 4;
	c4_to_float(&s[n], &(c->s.foff));     n += 4;
	c4_to_float(&s[n], &(c->s.vres));     n += 4;
	c4_to_float(&s[n], &(c->s.voff));     n += 4;
	c4_to_float(&s[n], &(c->s.bad));      n += 4;
  	c8_to_double(&s[n], &(c->s.image));   n += 8;
	c4_to_int(&s[n], &(c->s.vtype));      n += 4;
  	c8_to_double(&s[n], &(c->s.doppler)); n += 8;
	
#ifdef DEBUG
	printf("    -4  '%s' %f i%f (%f %f) n=%d r=%f v=%f %f %f\n", c->s.line,
	       c->s.restf, c->s.image, c->s.fres, c->s.foff, c->s.nchan, c->s.rchan, c->s.vres, c->s.voff, c->s.doppler);
#endif
    } else if (code == -10) { /* Continuum drifts */
  	c8_to_double(&s[n], &(c->u.freq));  n += 8;
	c4_to_float(&s[n], &(c->u.width));  n += 4;
	c4_to_int(&s[n], &(c->u.npoin));    n += 4;
	c4_to_float(&s[n], &(c->u.rpoin));  n += 4;
	c4_to_float(&s[n], &(c->u.tref));   n += 4;
	c4_to_float(&s[n], &(c->u.aref));   n += 4;
	c4_to_float(&s[n], &(c->u.apos));   n += 4;
	c4_to_float(&s[n], &(c->u.tres));   n += 4;
	c4_to_float(&s[n], &(c->u.ares));   n += 4;
	c4_to_float(&s[n], &(c->u.bad));    n += 4;
	c4_to_int(&s[n], &(c->u.ctype));    n += 4;
  	c8_to_double(&s[n], &(c->u.cimag)); n += 8;
	c4_to_float(&s[n], &(c->u.colla));  n += 4;
	c4_to_float(&s[n], &(c->u.colle));  n += 4;
	
#ifdef DEBUG
	printf("   -10  f=%f %f  n=%d r=%f\n", c->u.freq, c->u.width, c->u.npoin, c->u.rpoin);
	printf("   -10  t=%f %f  a=%f %f\n", c->u.tref, c->u.tres, c->u.aref, c->u.ares);
#endif
    } else if (code == -14) { /* Calibration */
	c4_to_float(&s[n], &(c->c.beeff));    n += 4;
	c4_to_float(&s[n], &(c->c.foeff));    n += 4;
	c4_to_float(&s[n], &(c->c.gaini));    n += 4;
	c4_to_float(&s[n], &(c->c.h2omm));    n += 4;
	c4_to_float(&s[n], &(c->c.pamb));     n += 4;
	c4_to_float(&s[n], &(c->c.tamb));     n += 4;
	c4_to_float(&s[n], &(c->c.tatms));    n += 4;
	c4_to_float(&s[n], &(c->c.tchop));    n += 4;
	c4_to_float(&s[n], &(c->c.tcold));    n += 4;
	c4_to_float(&s[n], &(c->c.taus));     n += 4;
	c4_to_float(&s[n], &(c->c.taui));     n += 4;
	c4_to_float(&s[n], &(c->c.tatmi));    n += 4;
	c4_to_float(&s[n], &(c->c.trec));     n += 4;
	c4_to_int(&s[n], &(c->c.cmode));      n += 4;
	c4_to_float(&s[n], &(c->c.atfac));    n += 4;
	c4_to_float(&s[n], &(c->c.alti));     n += 4;
	c4_to_float(&s[n], &(c->c.count[0])); n += 4;
	c4_to_float(&s[n], &(c->c.count[1])); n += 4;
	c4_to_float(&s[n], &(c->c.count[2])); n += 4;
	c4_to_float(&s[n], &(c->c.lcalof));   n += 4;
	c4_to_float(&s[n], &(c->c.bcalof));   n += 4;
  	c8_to_double(&s[n], &(c->c.geolong)); n += 8;
  	c8_to_double(&s[n], &(c->c.geolat));  n += 8;
        
#ifdef DEBUG
	printf("   -14 %d %f %f %f trec=%f  (%f,%f) Site:(%f,%f, %f)\n", n/4, c->c.h2omm, c->c.tamb, c->c.pamb, c->c.trec,
	       c->c.lcalof, c->c.bcalof, c->c.geolong, c->c.geolat, c->c.alti);
#endif
    } else {
        sprintf(buf, "Cannot handle CLASS section code %d yet.Sorry.", code);
	send_line(buf);
    }
}

static void fill_class_data(CLASS_SECTION *cs, char *s, int size, CLASS *c)
{
    int i, n, ndata;
    char *p;
    
    n = 4*(cs->nhead - 1);
    
    if (c->xkind == 1)
      ndata = c->u.npoin;
    else
      ndata = c->s.nchan;
    
    c->ndata = ndata;
    
    c->data = (float *)malloc(ndata * sizeof(float));
    
    if (!c->data) return;
      
    p = &s[n];
    
    for (i=0; i<ndata; i++) {
	c4_to_float(p, &(c->data[i]));
        p += 4;
    }
    
#ifdef DEBUG
    if (ndata > 30)
        printf("    -D %d %d  %f %f %f %f ... %f %f\n", ndata, n,
           c->data[0], c->data[10], c->data[20], c->data[30],
	   c->data[ndata-2], c->data[ndata-1]);
#endif
}

FILE *get_classfile_info(char *file, CLASS_INFO *i)
{
    int len;
    char block[CLASS_BLK_SIZE];
    FILE *fp;
    
    fp = fopen(file, "r");                  /* open the file for reading  */
    if (!fp) {                              /* test for failure           */
        sprintf(buf, "Cannot open file '%s'.\n", file);
	send_line(buf);
        return NULL;
    }
    
    len = fread(block, sizeof(char), CLASS_BLK_SIZE, fp);
    
    if (len != CLASS_BLK_SIZE) {
        sprintf(buf, "Cannot read %d byte block from file '%s'.\n", CLASS_BLK_SIZE, file);
	send_line(buf);
        fclose(fp);
	return NULL;
    }
    
    if (i) {
	memcpy(i->fic, &block[0], 4);
	i->fic[3] = '\0';
	c4_to_int(&block[4], &(i->nextbl));
	c4_to_int(&block[8], &(i->nie));
	c4_to_int(&block[12], &(i->nex));
	c4_to_int(&block[16], &(i->first));
#ifdef DEBUG
	printf("fic '%s'  (Type of data '1A' -> IEEE)\n", i->fic);
	printf("nextbl '%d' (Next free block)\n", i->nextbl);
	printf("nie '%d' (No of index entries)\n", i->nie);
	printf("nex '%d' (No if index extensions)\n", i->nex);
	printf("first '%d' (First block of 1st index extension)\n", i->first);
#endif
    }
    
    if (strncmp(i->fic, "1A", 2) != 0) {
        sprintf(buf, "Cannot read '%2s' type data from file '%s'.", i->fic, file);
	send_line(buf);
        fclose(fp);
	return NULL;
    }
    if (i->nex != 1) {
        sprintf(buf, "Cannot read %d index extensions from file '%s'.", i->nex, file);
	send_line(buf);
        fclose(fp);
	return NULL;
    }
    nst = i->first;
    st = (CLASS *)malloc(i->first * sizeof(CLASS));
    if (!st) {
        nst = 0;
        sprintf(buf, "Out of memory: Cannot allocate CLASS file structure.");
	send_line(buf);
        fclose(fp);
	return NULL;
    }
    
    return fp;
}

static void list_xdata(int nbl, CLASS *c)
{
    double rta(float);
    
#ifdef DEBUG
    printf("%5d 1 %7d %4d %d %12s %12s %12s %d (%+9.2e,%+9.2e) '%s' %d %d %d\n",
           nbl+1,
	   c->xbloc, c->xnum, c->xver, c->xsourc, c->xline, c->xtel,
	   c->xdobs, c->xoff1, c->xoff2,
	   c->xtype, c->xkind, c->xqual, c->xscan);
#endif
    sprintf(buf, "%5d 1 %7d %4d %d %12s %12s %12s %d (%+7.1f\",%+7.1f\") %d",
           nbl+1,
	   c->xbloc, c->xnum, c->xver, c->xsourc, c->xline, c->xtel,
	   c->xdobs, rta(c->xoff1), rta(c->xoff2), c->xscan);
    send_line(buf);
}

int get_classfile_listing(FILE *fp, CLASS_INFO *i)
{
    int n, len;
    char block[CLASS_BLK_SIZE];
    char index1[129], index2[129], index3[129], index4[129];
    CLASS obs;
    
    if (!fp) return -1;
    
    /* move one blk forward */
    fseek(fp, CLASS_BLK_SIZE*sizeof(char), SEEK_CUR);
    
    for (n=2; n<i->first; n++) {
      len = fread(block, sizeof(char), CLASS_BLK_SIZE, fp);
      if (len != CLASS_BLK_SIZE) {
        sprintf(buf, "Cannot read %d byte block from CLASS file.", CLASS_BLK_SIZE);
	send_line(buf);
        fclose(fp);
	return -2;
      }
      memcpy(index1, &block[0], 128);
      memcpy(index2, &block[128], 128);
      memcpy(index3, &block[128+128], 128);
      memcpy(index4, &block[128+128+128], 128);
      
      index1[128] = index2[128] = index3[128] = index4[128] = '\0';
      
      fill_class_table(&obs, index1);
      if (obs.xnum > 0 && obs.xnum < i->first) {
          st[obs.xnum] = obs;
          list_xdata(n, &obs);
      } else
          break;
      fill_class_table(&obs, index2);
      if (obs.xnum > 0 && obs.xnum < i->first) {
          st[obs.xnum] = obs;
          list_xdata(n, &obs);
      } else
          break;
      fill_class_table(&obs, index3);
      if (obs.xnum > 0 && obs.xnum < i->first) {
          st[obs.xnum] = obs;
          list_xdata(n, &obs);
      } else
          break;
      fill_class_table(&obs, index4);
      if (obs.xnum > 0 && obs.xnum < i->first) {
          st[obs.xnum] = obs;
          list_xdata(n, &obs);
      } else
          break;
    }
    
    return n;
}

int get_classfile_data(FILE *fp, int curr_nbl, CLASS_INFO *i)
{
    int k, n, len, nbl;
    char block[CLASS_BLK_SIZE], *sbl = NULL;
    
    CLASS_SECTION cobs;
    
    n = curr_nbl + 1;
    
    while (n < i->nextbl) {
      len = fread(block, sizeof(char), CLASS_BLK_SIZE, fp);
      nbl = check_block(st, i->first, n+1);
      if (nbl) {
	 fill_class_obs(&cobs, block);
#ifdef DEBUG
 	 printf("%7d %12s %12s %d %d %d %d %d %d %d %d %d (%d %d %d %d | %d %d %d %d | %d %d %d %d)\n",
	 	n,
	 	st[nbl].xsourc, st[nbl].xline, st[nbl].xkind, 
	 	cobs.nbl, cobs.bytes, cobs.adr, cobs.nhead, cobs.len, cobs.ientry, cobs.nsec, cobs.obsnum,
	   	cobs.sec_cod[0], cobs.sec_cod[1], cobs.sec_cod[2], cobs.sec_cod[3],
	   	cobs.sec_adr[0], cobs.sec_adr[1], cobs.sec_adr[2], cobs.sec_adr[3],
	   	cobs.sec_len[0], cobs.sec_len[1], cobs.sec_len[2], cobs.sec_len[3]);
#endif
	 sbl = (char *)malloc(cobs.nbl * sizeof(block));
	 if (!sbl) {
	     sprintf(buf, "CLASS: Not enough memory to allocate %d blocks.", cobs.nbl);
	     send_line(buf);
	     fclose(fp);
	     return 1;
	 }
	 /* copy the already read block into the sbl */
	 memcpy(sbl, block, 512);
	 if (cobs.nbl > 1) { /* and the remaining, if any */
	   len = fread(&sbl[512], sizeof(char), (cobs.nbl - 1)*512, fp);
	   if (len != (cobs.nbl - 1)*512) {
	       sprintf(buf, "CLASS: Read only %d (of %d) bytes.", len, (cobs.nbl - 1)*512);
	       send_line(buf);
	       free(sbl);
	       fclose(fp);
	       return 1;
	   }
	   n += cobs.nbl - 1;
	 }
	   /* Scan all nsec headers */
	 for (k=0; k<cobs.nsec; k++) {
	     fill_class_header(cobs.sec_cod[k], cobs.sec_adr[k], cobs.sec_len[k],
	                       sbl, cobs.nbl*512, &st[cobs.obsnum]);
	 }
	   /* Scan data */
	 fill_class_data(&cobs, sbl, cobs.nbl*512, &st[cobs.obsnum]);
#ifdef DEBUG
	 printf("Fill class data done for block %d.\n", n);
#endif
	 free(sbl);
      }
      n++;
    }
#ifdef DEBUG
    printf("Return get_classfile_data() at n = %d\n", n);
#endif
    
    return 0;
}

int get_class(char *file, FDATA *fd)
{
    int i, j, nchan=0;
    CLASS *c;
    double sp, cp, sqrBt, tmp;
    
    void strip_trailing_spaces(char *);
    double rta(float);
    void MJDNtoDate(DATE *, int);
    int CheckDataSize(int);

#ifdef DEBUG    
    printf("current scan=%d (%d)\n", current, nst);
#endif
    if (current <= 0 || current >= nst) return -1;
    
    if (!st) return -2;
    
    c = &st[current];
    
    if (!c) return -3;
    
    strncpy(fd->sname, c->xsourc, 12);
    fd->sname[12] = '\0';
    strip_trailing_spaces(fd->sname);
    strncpy(fd->molecule, c->xline, 12);
    fd->molecule[12] = '\0';
    strip_trailing_spaces(fd->molecule);
    
    fd->n = (int)c->ndata;
    fd->sno = (int)c->xscan;
    fd->subno = 0;
    fd->coordType = 0;
    /* SetCoordType(s->CSystem); */
    /*
    if ((pa = CheckPosAngle()))
        fd->posang = (*pa)*PI/180.0;
    else
        fd->posang = (double)c->xposa;
    */
    fd->posang = 0.0;
    cp = cos(fd->posang);
    sp = sin(fd->posang);
    fd->xoff = rta(c->xoff1)*cp - rta(c->xoff2)*sp;
    fd->yoff = rta(c->xoff2)*cp + rta(c->xoff1)*sp;
    fd->equinox = c->p.epoch;
    if (fd->equinox <= 1950.1) {
        fd->epoch = 'B';
    } else {
        fd->epoch = 'J';
    }
    fd->x0 = c->p.lam;
    fd->y0 = c->p.bet;
/*    if (CheckCRVALType()) {
        fd->y0 -= s->BMapOff;
        fd->x0 -= s->LMapOff/cos(fd->y0);
    }
    fd->scanType = s->ObsMode;
 */
    fd->tsys = (double)c->g.tsys;
    fd->tau  = (double)c->g.tau;
    fd->int_time = (double)c->g.time;
    fd->vlsr = c->s.voff;
    MJDNtoDate(&(fd->date), c->xdobs + 60549);
    fd->date.Hour  = (int)(c->g.ut * RADTOHR);
    fd->date.Min   = (int)(60.0*(c->g.ut * RADTOHR - fd->date.Hour));
    fd->date.Sec   = (int)(60.0*(60.0*(c->g.ut * RADTOHR- fd->date.Hour) -
                           fd->date.Min));
    fd->LST = fd->date;
    fd->LST.Hour   = (int)(c->g.st * RADTOHR);
    fd->LST.Min    = (int)(60.0*(c->g.st * RADTOHR - fd->LST.Hour));
    fd->LST.Sec    = (int)(60.0*(60.0*(c->g.st * RADTOHR- fd->LST.Hour) -
                           fd->LST.Min));
    fd->az = c->g.az * RADTODEG;
    fd->el = c->g.el * RADTODEG;
    fd->aoff = fd->eoff = 0.0;
/*
    fd->aoff = rta(s->AzMapOff)*cp - rta(s->ElMapOff)*sp;
    fd->eoff = rta(s->ElMapOff)*cp + rta(s->AzMapOff)*sp;
  */
/*
#ifdef SISYFOS
    if (sscanf(s->Program, "COR%d", &arr_no) == 1) {
        if (arr_no >=1 && arr_no <= 4) {
            fd->aoff += (SisyfosAz[arr_no] - SisyfosAz[0]);
            fd->eoff += (SisyfosEl[arr_no] - SisyfosEl[0]);
            NorthAz = s->Azimuth + PI;
            GetEquOffsets(&(fd->LST), NorthAz, s->Elevation,
                          fd->aoff, fd->eoff,
                          s->Longitude, s->Latitude,
                          &RAOffset, &DecOffset);
            fd->xoff += RAOffset;
            fd->yoff += DecOffset;
        }
    }
#endif
#ifdef ONTHEFLY
    NorthAz = s->Azimuth + PI;
    GetEquOffsets(&(fd->LST), NorthAz, s->Elevation,
                  fd->aoff, fd->eoff,
                  s->Longitude, s->Latitude,
                  &RAOffset, &DecOffset);
    fd->xoff += RAOffset;
    fd->yoff += DecOffset;
#endif
*/
    fd->b.maj = 0;
    fd->b.min = 0;
    fd->b.PA  = 0;
    fd->beameff = c->c.beeff;
    fd->pol     = 0;
    fd->TAir = c->c.tamb; 
    fd->PAir = c->c.pamb; 
    fd->RAir = c->c.h2omm; 
    
    fd->firstIF = 6000.0/1000.0;
    fd->skyfreq = 0.0/1000.0;
    
    if (c->xkind == 0)
       nchan = c->s.nchan;
    else
       nchan = c->u.npoin;
       
    if (CheckDataSize(nchan) < nchan) {
        sprintf(buf, "Out of memory: Can't allocate enough memory for NChan=%d.", nchan);
        send_line(buf);
	return -4;
    }
    
    if (c->xkind == 0) { /* Spectral line scan */
      fd->lofreq = (c->s.restf + c->s.image)/2/1000.0;
      fd->f0 = (c->s.restf + (0.0 - c->s.rchan)*c->s.fres)/1000.0;
      fd->fn = (c->s.restf + (c->s.nchan - 1.0 - c->s.rchan)*c->s.fres)/1000.0;
      fd->fres = c->s.fres;
      fd->v0 = c->s.voff + (0.0 - c->s.rchan)*c->s.vres;
      fd->vres = c->s.vres;
      if (c->s.fres < 0.0) { /* always store in fd with positive df */
        fd->fres = -fd->fres;
        tmp = fd->f0; fd->f0 = fd->fn; fd->fn = tmp;
	fd->vres = -fd->vres;
	fd->v0 = c->s.voff + (c->s.nchan - 1.0 - c->s.rchan)*c->s.vres;
      }
      sqrBt = sqrt(fabs(c->s.fres) * 1.0e6 * fd->int_time);
      for (i=0; i<c->s.nchan; i++) {
          j = i;
	  if (c->s.fres < 0.0) j = c->s.nchan - 1 - i;
          fd->d[i] = c->data[j];
	  if (sqrBt > 0.0)
	    fd->e[i] = 2.0*fd->tsys/sqrBt;
	  else
	    fd->e[i] = 0.0;
      }
      fd->firstIF = (c->s.restf - c->s.image)/2./1000.0;
      fd->skyfreq = c->s.restf*(1.0 - c->s.doppler/SPEEDOFLIGHT)/1000.0;
    } else { /* continuum scan */
      fd->lofreq = (c->u.freq + c->u.cimag)/2/1000.0;
      fd->f0 = (c->u.tref + (0.0 - c->u.rpoin)*c->u.tres)/1000.0;
      fd->fn = (c->u.tref + (c->u.npoin - 1 - c->u.rpoin)*c->u.tres)/1000.0;
      fd->fres = c->u.tres;
      fd->v0 = rta(c->u.aref + (0.0 - c->u.rpoin)*c->u.ares);
      fd->vres = rta(c->u.ares);
      sqrBt = sqrt(fabs(c->u.width) * 1.0e6 * fd->int_time);
      for (i=0; i<c->u.npoin; i++) {
          fd->d[i] = (double)c->data[i];
	  if (sqrBt > 0.0)
	    fd->e[i] = 2.0*fd->tsys/sqrBt;
	  else
	    fd->e[i] = 0.0;
      }
    }
/*    if (s->FreqRes < 0.0) {
        fd->f0 = Frequency(x->NChannel-1, s, x)/1000.;
        fd->fn = Frequency(0, s, x)/1000.;
        if (odin) {
            fd->v0 = VelOdin(x->NChannel-1, s, x);
        } else {
            fd->v0 = Velocity(x->NChannel-1, s, x);
        }
        fd->fres = -s->FreqRes;
        fd->vres = -s->VelRes;
        sqrBt = sqrt(fabs(fd->fres) * fd->int_time)*1000.0;
        for (i=x->NChannel-1; i>=0; i--) {
            fd->d[x->NChannel-1-i] = (double)(s->c[i]);
            if (sqrBt > 0.0 && fd->tsys > 0.0)
                fd->e[x->NChannel-1-i] = 2.0*fd->tsys/sqrBt;
            else
                fd->e[x->NChannel-1-i] = 1.0;
        }
    } else {
        fd->fn = Frequency(x->NChannel-1, s, x)/1000.;
        fd->f0 = Frequency(0, s, x)/1000.;
        if (odin) {
            fd->v0 = VelOdin(0, s, x);
        } else {
            fd->v0 = Velocity(0, s, x);
        }
        fd->fres = s->FreqRes;
        fd->vres = s->VelRes;
        sqrBt = sqrt(fabs(fd->fres) * fd->int_time)*1000.0;
        for (i=0; i<x->NChannel; i++) {
          fd->d[i] = (double)(s->c[i]);
          if (sqrBt > 0.0 && fd->tsys > 0.0)
              fd->e[i] = 2.0*fd->tsys/sqrBt;
          else
              fd->e[i] = 1.0;
        }
    }
    */
    return 0;
}

static void LoadClassScans(char *file)
{
    int ierr, n, nread=0;
    DataSetPtr d;
    
    list   *get_listlist();
    DataSetPtr new_dataset(list *, char *, DataSetPtr);
    void DeleteLastDataSet();
    int count_scans(DataSetPtr);
    void obtain_map_info(Widget, char *, XtPointer);
    void UpdateData(int, int);
    int read_file(char *, char *, DataSetPtr);
    char *StripSuffix(const char *);
    char *StripPath(const char *);
    
    if (file)
       strcpy(buf, StripSuffix(StripPath(file)));
    else
       strcpy(buf, "CLASS file");
       
    d = new_dataset(get_listlist(), buf, NULL);
    
    if (!d) {
        sprintf(buf, "Couldn't allocate dataset for %s.", file);
	send_line(buf);
        return;
    }
	
#ifdef DEBUG    
    printf("Trying to load %d CLASS scans from %s.\n", nst-1, file);
#endif
    for (n=1; n<nst; n++) {
        current = n;
	if (n==1)
	  ierr = read_file("seqclass", file, d);
	else
	  ierr = read_file("seqclass", file, NULL);
	  
	if (ierr == 0) nread++;
    }
    sprintf(buf, "Read %d scans (of %d) from CLASS file %s.",
            nread, nst-1, StripPath(file));
    send_line(buf);
    if (count_scans(d) > 0) {
        vP->from = d;
        if (count_scans(d) > 1) {
            obtain_map_info(NULL, "map", NULL);
        }
	UpdateData(SCALE_BOTH, REDRAW);
    } else {
        vP->to = vP->from;
	DeleteLastDataSet();
    }
#ifdef DEBUG
    printf("Loading done.\n");
#endif

}

int PostClassScanListDialog(char *file, Widget w, fsel_struct *fs)
{
    int ierr, n;
    CLASS_INFO info;
    FILE *fp = NULL;

#ifdef DEBUG    
    printf("File: '%s'\n", file);
#endif
    
    fp = get_classfile_info(file, &info);
    
    if (!fp) {
        return 0;
    }
    
    n = get_classfile_listing(fp, &info);
    
    if (n <= 0) {
        if (st) free(st);
	st = NULL;
	nst = 0;
        return 0;
    }
    
    ierr = get_classfile_data(fp, n, &info);
    
    if (!ierr) {
        LoadClassScans(file);
    }
    
    if (st) {
        for (n=0; n<nst; n++)
	  if (st[n].data) free(st[n].data);
        free(st);
	st = NULL;
	nst = 0;
    }
    
    return 0;
}
