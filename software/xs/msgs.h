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
#define MAXSIZE      300
#define MAXLINES    2000
#define MIN(a,b)     (((a) < (b)) ? (a) : (b))
#define ABS(a)       (((a) >= 0) ? (a) : -(a))
#define MAX(a,b)     ((a > b) ? a : b)
#define MARGIN         5

typedef struct {
  char         *chars[MAXLINES];   /* Lines of text         */
  int           length[MAXLINES];  /* Length of each line   */
  int           rbearing[MAXLINES];/* right bearing of line */
  int           descent;           /* descent below baseline*/
  XFontStruct  *font;              /* The font struct       */
  GC            gc;                /* A read/write GC       */
  Widget        scrollbar;
  Widget        canvas;
  Dimension     canvas_height;     /* canvas dimensions     */
  Dimension     canvas_width;
  int           fontheight;        /* descent + ascent      */
  int           nitems;            /* number of text lines  */
  int           top;               /* line at top of window */
} MSG, *MSG_ptr;
