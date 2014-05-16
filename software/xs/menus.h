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
#ifndef MENUS_H
#define MENUS_H

#ifndef LIST_H
#include "list.h"
#endif

typedef struct _callback_struct {
    Widget             widget;          /* ToggleButton widget             */
    XtPointer          data;            /* Data for the callback           */
    int               *toggle_var;      /* Toggle variable                 */
    int               *value;           /* Toggle on value, if != 1        */
} CallbackData;

typedef struct _menu_item {
   char               *label;           /* the label for the item          */
   WidgetClass        *class;
   char                mnemonic;
   char               *accelerator;
   char               *accel_text;
   Boolean             tear_off;
   list             *(*listfunc)();     /* A list ptr for on-the-fly items */
   void              (*callback)();     /* Callback to be invoked          */
   XtPointer           callback_data;   /* Data for the callback           */
   struct _menu_item  *subitems;        /* data for submenu of this button */
} MenuItem;

#define MENUSEPARATOR {"separator", &xmSeparatorWidgetClass,\
                       ' ', NULL, NULL, False, NULL, NULL, NULL, NULL}
#define NULLMENU      {NULL, NULL, ' ', NULL, NULL, False, NULL, NULL, NULL, NULL}

typedef struct _menu_bar_item {
    char              *label;
    char               mnemonic;
    Boolean            tear_off;
    MenuItem          *items;
} MenuBarItem;

typedef struct _button_info {
    int top, bottom, left, right, dir;
} ButtonInfo;

typedef struct _button_item {
    char              *label;           /* the label for the item          */
    WidgetClass       *class;           /* Button class, Push, toggle, ... */
    ButtonInfo        *info;            /* contains info 'bout button      */
    void             (*callback)();     /* Callback to be invoked          */
    XtPointer          callback_data;   /* Data for the callback           */
} ButtonItem;

typedef struct _button_struct {
   char              *name;            /* Name of the button              */
   void             (*callback)();     /* Callback to be invoked          */
   XtPointer          callback_data;   /* Data for the callback           */
} button_struct;

#define EOI {NULL}

#define APEX

#endif
