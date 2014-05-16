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
#include <errno.h>

#include "dfil.h"

/* open a dfil, given dfil number, block length and user */
FILE *OpenDfil(int dfil, int blocklen, char *user, const char *dfilname)
{
    FILE *df;

    df = fopen(dfilname, "r");
    return (df);
}

/* read the block index of dfil */
int ReadDfilIndex(FILE *df, DFWORD index[])
{
    int nw, rw;

    rw = fseek(df, 0L, 0);
    if (rw == -1) {
        perror("Can't position to beginning of file");
        return -1;
    }
    if ((nw = fread((char *)index, sizeof(DFWORD), DFILBLOCK, df)) == 0) {
        perror("Can't read index");
        return -1;
    }
    return (nw);
}

/* write the block index of dfil */
void WriteDfilIndex(FILE *df, DFWORD index[])
{
    int nw, rw;

    rw = fseek(df, 0L, 0);
    if (rw == -1) {
        perror("Can't position to beginning of file");
        return;
    }
    nw = fwrite((char *)index, sizeof(DFWORD), DFILBLOCK, df);
    if (nw == 0) {
        perror("Can't write index");
        return;
    }
}

/* find block in dfil, 1 <= block <= DFILBLOCK */
int GetDfilBlock(FILE *df, int block, int blocklen, DFWORD itwh[])
{
    long where;
    int rw, b;
    DFWORD *wp;
    char errmsg[80];
   
    where = (long)((block-1)*blocklen+1)*DFILBLOCK*sizeof(DFWORD);
    rw = fseek(df, where, 0);
    if (rw == -1) {
        sprintf(errmsg, "Can't position to block %ld.", where);
        perror(errmsg);
        return 1;
    }
    for (b=0, wp=itwh; b<blocklen; b++, wp+=DFILBLOCK) {
        if (fread((char *)wp, sizeof(DFWORD), DFILBLOCK, df) == 0) {
            sprintf(errmsg, "Can't read block %d (%ld).", b, where);
            perror(errmsg);
            return 1;
        }
    }
    return 0;
}

/* write block in dfil, 1 <= block <= DFILBLOCK */
void PutDfilBlock(FILE *df, int block, int blocklen, DFWORD itwh[])
{
    long where;
    int rw, b;
    DFWORD *wp;
    char errmsg[80];
   
    where = (long)((block-1)*blocklen+1)*DFILBLOCK*sizeof(DFWORD);
    rw = fseek(df, where, 0);
    if (rw == -1) {
        sprintf(errmsg, "can't position to block %ld", where);
        perror(errmsg);
        return;
    }
    for (b=0, wp=itwh; b<blocklen; b++, wp+=DFILBLOCK) {
        if (fwrite((char *)wp, sizeof(DFWORD), DFILBLOCK, df) == 0) {
            sprintf(errmsg, "can't write block %d (%ld)", b, where);
            perror(errmsg);
            return;
        }
    }
}

/* look up block number for given scan number */
int FindDfilScan(DFWORD scanno, DFWORD index[])
{
    int i;

    for (i=0; i<DFILBLOCK; i++) {
        if (scanno == index[i]) break;
    }
    return (i==DFILBLOCK ? 0 : i+1);
}
