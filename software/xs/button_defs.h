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
void freq_scroll();

#ifndef MENUS_H
#include "menus.h"
#endif

ButtonItem LeftButtons[] = {
  {"Remove all Gaussians",        &xmPushButtonGadgetClass, NULL,
   gauss_reset, "all"},
  {"Remove latest Gaussian",      &xmPushButtonGadgetClass, NULL, 
   gauss_reset, "latest"},
  {"Remove Gaussian with cursor", &xmPushButtonGadgetClass, NULL,
   gauss_reset, "cursor"},
EOI};

ButtonInfo binfo1 = {1, 5, 0, 2, XmARROW_LEFT};
ButtonInfo binfo2 = {1, 5, 2, 3, XmARROW_LEFT};
ButtonInfo binfo3 = {1, 5, 3, 4, XmARROW_RIGHT};
ButtonInfo binfo4 = {1, 5, 4, 6, XmARROW_RIGHT};

ButtonItem ScrollButtons[] = {
  {"arrow1", &xmArrowButtonGadgetClass, &binfo1, freq_scroll, "<<"},
  {"arrow2", &xmArrowButtonGadgetClass, &binfo2, freq_scroll, "<"},
  {"arrow3", &xmArrowButtonGadgetClass, &binfo3, freq_scroll, ">"},
  {"arrow4", &xmArrowButtonGadgetClass, &binfo4, freq_scroll, ">>"},
EOI};

