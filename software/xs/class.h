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
#define CLASS_BLK_SIZE 512

typedef struct {
  char fic[4];
  int nextbl, nie, nex, first;
} CLASS_INFO;

typedef struct {
  double ut, st;
  float az, el;
  float tau, tsys, time;
  int xunit;
} CLASS_GEN;

typedef struct {
  char source[12];
  float epoch;
  double lam, bet;
  float lamof, betof;
  int proj;
  double sl0p, sb0p, sk0p;
} CLASS_POS;

typedef struct {
  char line[13];
  double restf;
  int nchan;
  float rchan, fres, foff, vres, voff, bad;
  double image;
  int vtype;
  double doppler;
} CLASS_SPE;

typedef struct {
  float beeff, foeff, gaini, h2omm, pamb, tamb, tatms, tchop, tcold, taus, taui, tatmi, trec;
  int cmode;
  float atfac, alti, count[3], lcalof, bcalof;
  double geolong, geolat;
} CLASS_CAL;

typedef struct {
  double freq;
  float width;
  int npoin;
  float rpoin, tref, aref, apos, tres, ares, bad;
  int ctype;
  double cimag;
  float colla, colle;
} CLASS_CON;

typedef struct {
  int xbloc;
  int xnum;
  int xver;
  char xsourc[13];
  char xline[13];
  char xtel[13];
  int xdobs;
  int xdred;
  float xoff1;
  float xoff2;
  char xtype[5];
  int xkind;
  int xqual;
  int xscan;
  int xposa;
  char xfront[9];
  char xback[9];
  char xproc[9];
  char xproj[9];
  char unused[25];
  CLASS_GEN g;
  CLASS_POS p;
  CLASS_SPE s;
  CLASS_CAL c;
  CLASS_CON u;
  int ndata;
  float *data;
} CLASS;

typedef struct {
   char ident[4];
   int nbl;
   int bytes;
   int adr;
   int nhead;
   int len;
   int ientry;
   int nsec;
   int obsnum;
   int sec_cod[4];
   int sec_adr[4];
   int sec_len[4];
} CLASS_SECTION;
