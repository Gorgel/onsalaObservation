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
/* This is the local.h file */
#define DEF_EDITOR      "/usr/bin/vi"
#define DATA_DIR        "~/data"
#define GAUSS_DIR       "~/gauss"
#define TMP_DIR         "/tmp"

#ifdef __HPUX
#define UNIX_SORT       "/usr/bin/sort -n"
#define UNIX_MV         "/usr/bin/mv -f"
#define UNIX_RM         "/usr/bin/rm -f"
#define UNIX_CP         "/usr/bin/cp -f"
#define UNIX_GUNZIP     "gunzip"
#define UNIX_CAT        "/usr/bin/cat"
#define UNIX_LPR        "/usr/bin/lp"
#else
#define UNIX_SORT       "sort -n"
#define UNIX_MV         "mv -f"
#define UNIX_RM         "rm -f"
#define UNIX_CP         "cp -f"
#define UNIX_GUNZIP     "gunzip"
#define UNIX_CAT        "cat"
#define UNIX_LPR        "lpr"
#endif

#define DEF_FILE	XS_DIR "data/mace.fits"

char slaim_dir[]  = XS_DIR;
char lovas_dir[]  = XS_DIR;
char jpl_dir[]    = XS_DIR;
char ident_dir[]  = XS_DIR;

char slaim_file[] = "lovas.cat";
char stmp_file[]  = "lovas.tmp";
char jpl_file[]   = "jpl.cat";
char jtmp_file[]  = "jpl.tmp";
char ident_file[] = "ident.cat";
char itmp_file[]  = "ident.tmp";
char lovas_file[] = "lovas_obs.cat";
char ltmp_file[]  = "lovas_obs.tmp";
