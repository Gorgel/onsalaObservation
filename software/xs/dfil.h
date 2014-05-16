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
#define DFWORD    short int
#define DFILBLOCK 1024

FILE *OpenDfil(int dfil, int blocklen, char *user, const char *dfilname);
int ReadDfilIndex(FILE *df, DFWORD index[]);
void WriteDfilIndex(FILE *df, DFWORD index[]);
int GetDfilBlock(FILE *df, int block, int blocklen, DFWORD itwh[]);
void PutDfilBlock(FILE *df, int block, int blocklen, DFWORD itwh[]);
int FindDfilScan(DFWORD scanno, DFWORD index[]);
