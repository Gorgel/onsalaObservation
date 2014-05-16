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
#define AUTOMODE_SINGLE     0
#define AUTOMODE_MAP_FIRST  1
#define AUTOMODE_MAP_APPEND 2
#define AUTOMODE_SEQ_FIRST  3
#define AUTOMODE_SEQ_APPEND 4
#define AUTOMODE_END        10

#define ATOM_INFO      "FITS_data_information"
#define ATOM_INFO_TYPE "FITS_data_information_type"

#define ATOM_DATA      "FITS_data"

#define ATOM_SEND      "FITS_data_sender"

#define ATOM_MESG      "FITS_message"

typedef struct {
    int   type;          /* Type of FITS scan, see AUTOMODE_* above */
    int   reclen;        /* FITS record length, usually 2880        */
    int   nrec;          /* Number of records stored in FITS_DATA   */
    char *data;          /* A pointer to the data                   */
} auto_fits;

Atom FITS_SEND;
Atom FITS_MESG;
Atom FITS_INFO, FITS_INFO_TYPE;
Atom FITS_DATA;
