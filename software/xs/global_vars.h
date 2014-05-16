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
const int nFonts = 4;

char  *fontname08[] = {
 "-adobe-courier-medium-r-normal--8-80-75-75-m-50-iso8859-1",
 "-adobe-helvetica-medium-r-normal--8-80-75-75-p-46-iso8859-1",
 "-adobe-times-medium-r-normal--8-80-75-75-p-44-iso8859-1",
 "5x8"
};
char  *fontname10[] = {
 "-adobe-courier-medium-r-normal--10-100-75-75-m-60-iso8859-1",
 "-adobe-helvetica-medium-r-normal--10-100-75-75-p-57-iso8859-1",
 "-adobe-times-medium-r-normal--10-100-75-75-p-54-iso8859-1",
 "6x10"
};
char  *fontname12[] = {
 "-adobe-courier-medium-r-normal--12-120-75-75-m-70-iso8859-1",
 "-adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1",
 "-adobe-times-medium-r-normal--12-120-75-75-p-64-iso8859-1",
 "6x12"
};
char  *fontname14[] = {
 "-adobe-courier-medium-r-normal--14-100-100-100-m-90-iso8859-1",
 "-adobe-helvetica-medium-r-normal--14-100-100-100-p-78-iso8859-1",
 "-adobe-times-medium-r-normal--14-100-100-100-p-74-iso8859-1",
 "7x14"
};
char  *fontname18[] = {
 "-adobe-courier-medium-r-normal--18-180-75-75-m-110-iso8859-1",
 "-adobe-helvetica-medium-r-normal--18-180-75-75-p-98-iso8859-1",
 "-adobe-times-medium-r-normal--18-180-75-75-p-94-iso8859-1",
 "r16",
};
char  *fontname24[] = {
 "-adobe-courier-medium-r-normal--24-240-75-75-m-150-iso8859-1",
 "-adobe-helvetica-medium-r-normal--24-240-75-75-p-130-iso8859-1",
 "-adobe-times-medium-r-normal--24-240-75-75-p-124-iso8859-1",
 "r24"
};

/* char   f_form[]   = "Frequency  %9.4f";
char   v_form[]   = "Velocity   %9.4f";
char   c_form[]   = "Channel    %9d";
char   t_form[]   = "Intensity  %9.4f";
char   s_form[]   = "Spectrum   %9.4f"; */
char   f_form[]   = "Frequency";
char   f_form2[]  = "Freq.";
char   fo_form[]  = "Freq. offset";
char   v_form[]   = "Velocity";
char   c_form[]   = "Channel %d";
char   t_form[]   = "Intensity";
char   s_form[]   = "Spectrum";
/* char   RA_form[]  = "RA offset  %8.1f";
char   Dec_form[] = "Dec offset %8.1f"; */
char   RA_form[]  = "X offset";
char   Dec_form[] = "Y offset";
/* char   p_form[]   = "Position   %8.1f"; */
char   p_form[]   = "Position";
/* char   z_form[]   = "Value %8.3f"; */
char   z_form[]   = "Value";
char   v2_form[]  = "Vel. [%7.2f,%7.2f]";
char   p1_form[]  = "Pos1 [%7.2f,%7.2f]";
char   p2_form[]  = "Pos2 [%7.2f,%7.2f]";

char   x1_label[] = "Frequency  [GHz]";
char   x2_label[] = "Velocity  [km/s]";
char   x3_label[] = "Channel";
char   x4_label[] = "Freq. offset [MHz]";
char   x5_label[] = "Frequency  [MHz]";
char   RA_label[] = "RA offset  [\"]";
char   Dec_label[] = "Dec offset  [\"]";
char   Pos_label[] = "Position [\"]";

char  *unit_labels[] = {"GHz", "km/s", "channels", "\"", "'", "MHz", "MHz"};

int    gau_h = 80;
int    msg_h = 80;

GLOBAL *gp;

int    mod_sel=0;
int    mark_sel=0, box_sel=0, rbox_sel=0, rgauss_sel=0;
int    gauss_show;
Gauss  gau;
int    nmod;
MOD    mods[MAXMOD];
BOX    box;

extern VIEW  view, *vP;
extern DRAW  draw;
