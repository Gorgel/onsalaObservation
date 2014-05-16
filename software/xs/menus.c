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
#include <stdlib.h>
#include <string.h>

#include <Xm/Separator.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/ArrowB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleBG.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/ArrowBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>

#include "defines.h"
#include "global_structs.h"
#include "menus.h"

extern VIEW   *vP;
extern GLOBAL *gp;

#define PERM_MENU_ITEM 0
#define TEMP_MENU_ITEM 1
#define MORE_MENU_ITEM 2

COLOR *GetColorInfo();

/*** Local defs ***/
typedef struct {
    Widget menu;
    list *(*lf)();
    void (*af)();
} listWidget;

static int MaxMenuItems;
static list togglelist;

static Widget BuildMenu(Widget, int, char *, char, Boolean, MenuItem *);

void init_togglelist()
{
    status init_list();

    init_list(&togglelist);
}

list *get_togglelist()
{
    return &togglelist;
}

static list toggle_iterator(list last)
{
    return (last == NULL) ? togglelist : NEXT(last);
}

static CallbackData *toggle_ret(CallbackData *pS)
{
    if (pS) {
        free(pS);
    }
    return NULL;
}

static CallbackData *new_toggle(list *pL, Widget w, CallbackData *cd)
{
    CallbackData *pS = NULL;

    status insert(list *, generic_ptr);
    
    pS = (CallbackData *)malloc(sizeof(CallbackData));
    if (!pS)
        return toggle_ret(pS);

    if (insert(pL, (generic_ptr)pS) == Error)
        return toggle_ret(pS);
        
    pS->widget = w;
    pS->data = cd->data;
    pS->toggle_var = cd->toggle_var;
    pS->value = cd->value;

    return pS;
}

static void ListMenuCB(Widget, listWidget *, XmAnyCallbackStruct *);

static int NoOfMenuItems(Widget menu)
{
    int n, m, nItems;
    WidgetList items;
    
    XtVaGetValues(menu, XmNchildren, &items, XmNnumChildren, &nItems, NULL);
    
    m = 0;
    for (n=0; n<nItems; n++) {
        if (!XtIsSubclass(items[n], xmSeparatorWidgetClass)) m++;
    }
    
    return m;
}

static Widget FillMenuItem(Widget menu, Widget cascade, int menu_type,
                           int item_type, MenuItem *item)
{
    static int    i=0;
    int           nItems, force_submenu = 0;
    string        s;
    Widget        widget;
    listWidget   *lw;
    XmString      str;
    CallbackData *cbd;

    if (item && item->listfunc) {
        if (menu_type == XmMENU_PULLDOWN && cascade) {
            lw = (listWidget *) XtMalloc(sizeof(listWidget));
            lw->menu = menu;
            lw->lf   = item->listfunc;
            lw->af   = item->callback;
            XtAddCallback(cascade, XmNcascadingCallback,
                          (XtCallbackProc)ListMenuCB, lw);
            return cascade;
        }
        return NULL;
    }
    
    if (MaxMenuItems > 0) {
        nItems = NoOfMenuItems(menu);
        if ((nItems + 1) % MaxMenuItems == 0 &&
            item_type != MORE_MENU_ITEM) force_submenu = 1;
        /* force_submenu = 0; */
    }
    
    if (force_submenu) {
        widget = BuildMenu(menu, XmMENU_PULLDOWN, "...more",
                           ' ', item->tear_off, item);
    } else if (item->subitems) {
        if (menu_type == XmMENU_OPTION) {
            XtWarning("You can't have submenus in option menus.");
            return NULL;
        } else {
            widget = BuildMenu(menu, XmMENU_PULLDOWN, item->label,
                               item->mnemonic, item->tear_off, item->subitems);
        }
    } else {
        /* printf("Menu item: '%s'   menuId=%d\n", item->label, (int)menu); */
        /* if (item_type == TEMP_MENU_ITEM) { */
            sprintf(s, "%s_%d", item->label, i);
            i++;
        /* } else {
            sprintf(s, "%s", item->label);
        } */
        str = MKSTRING(item->label);
        /* printf("WidgetLabel='%s'\n", s); */
        widget = XtVaCreateManagedWidget(s, *item->class, menu, 
                                         XmNlabelString, str,
                                         NULL);
        /* printf("New widgetId=%d\n", (int)widget); */
        XmStringFree(str);
        if (item_type == TEMP_MENU_ITEM) {
            /* printf("Temp. menu item: '%s'\n", item->label); */
            if (item->callback_data == (XtPointer)vP->from ||
                item->callback_data == (XtPointer)vP->m ||
                item->callback_data == (XtPointer)vP->p) {
                COLOR *c = GetColorInfo();
                XtVaSetValues(widget,
                              XmNforeground, c->cols[0],
                              NULL);
            }
        }
    }
    
    /* printf("item_type = %d\n", item_type); */
    XtVaSetValues(widget, XmNuserData, item_type, NULL);

    if (item->mnemonic != ' ')
        XtVaSetValues(widget, XmNmnemonic, item->mnemonic, NULL);

    if (item->accelerator) {
        str = MKSTRING(item->accel_text);
        XtVaSetValues(widget, XmNaccelerator, item->accelerator,
                      XmNacceleratorText, str, NULL);
        XmStringFree(str);
    }
    /* printf("callbacks...\n"); */
    if (item->callback && !force_submenu) {
        if (*item->class == xmToggleButtonWidgetClass ||
            *item->class == xmToggleButtonGadgetClass) {
            if (item_type != TEMP_MENU_ITEM) {
                cbd = new_toggle(&togglelist,
                                 widget, (CallbackData *)item->callback_data);
                if (cbd->toggle_var)
                    XtVaSetValues(widget, XmNset,
                                  *(cbd->toggle_var) ? True : False, NULL);
                XtAddCallback(widget, XmNvalueChangedCallback,
                              item->callback, (XtPointer)cbd);
            } else {
                XtAddCallback(widget, XmNvalueChangedCallback,
                              item->callback, item->callback_data);
            }
        } else {
            XtAddCallback(widget, XmNactivateCallback,
                          item->callback, item->callback_data);
        }
    }
    return (force_submenu == 1) ? cascade : widget;
}

static Widget BuildMenu(Widget parent, int menu_type, char *menu_title,
                        char menu_mnemonic, Boolean tear_off, MenuItem *items)
{
    int i, itype=PERM_MENU_ITEM;
    Widget menu = NULL, cascade = NULL;
    XmString str;

    if (menu_type == XmMENU_PULLDOWN || menu_type == XmMENU_OPTION)
        menu = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);
    else if (menu_type == XmMENU_POPUP)
        menu = XmCreatePopupMenu(parent, "_popup", NULL, 0);
    else if (menu_type == -101) {
        menu = parent;
        itype = TEMP_MENU_ITEM;
    } else {
        XtWarning("Invalid menu type passed to BuildMenu().");
        return NULL;
    }

#ifndef VUEWM
#if XmVersion >= 1002
    if (tear_off) {
        XtVaSetValues(menu, XmNtearOffModel, XmTEAR_OFF_ENABLED, NULL);
    }
#endif
#endif

    if (menu_type == XmMENU_PULLDOWN) {
        str = MKSTRING(menu_title);
        if (strcmp(menu_title, "...more")==0) {
            COLOR *c = GetColorInfo();
            /* printf("Forced cascade.\n"); */
            cascade = XtVaCreateManagedWidget(menu_title,
                                          xmCascadeButtonWidgetClass,
                                          parent,
                                          XmNsubMenuId, menu,
                                          XmNlabelString, str,
                                          XmNbackground, c->cols[1],
                                          XmNmnemonic, menu_mnemonic,
                                          XmNuserData, MORE_MENU_ITEM,
                                          NULL);
        } else if (menu_mnemonic != ' ')
            cascade = XtVaCreateManagedWidget(menu_title,
                                          xmCascadeButtonGadgetClass,
                                          parent,
                                          XmNsubMenuId, menu,
                                          XmNlabelString, str,
                                          XmNmnemonic, menu_mnemonic,
                                          NULL);
        else
            cascade = XtVaCreateManagedWidget(menu_title,
                                          xmCascadeButtonGadgetClass,
                                          parent,
                                          XmNsubMenuId, menu,
                                          XmNlabelString, str,
                                          NULL);
        XmStringFree(str);
    } else if (menu_type == XmMENU_OPTION) {
        Arg args[5];
        int n = 0;
        str = MKSTRING(menu_title);
        XtSetArg(args[n], XmNsubMenuId, menu); n++;
        XtSetArg(args[n], XmNlabelString, str); n++;
        cascade = XmCreateOptionMenu(parent, menu_title, args, n);
        XmStringFree(str);
    }

    for (i=0; items[i].label != NULL; i++) {
         /* printf("BM %2d  '%s'\n", i, items[i].label); */
         if (cascade == FillMenuItem(menu, cascade,
                                     menu_type, itype, &items[i])) {
             /* printf("Item loop break detected.\n"); */
             break;
         }
    }

    return (menu_type == XmMENU_POPUP) ? menu : cascade;
}

Widget CreateMenuBar(Widget MainWindow, MenuBarItem *items)
{
    int i = 0;
    Widget MenuBar, widget=NULL;

    MenuBar = XmCreateMenuBar(MainWindow, "MenuBar", NULL, 0);

    while (items[i].label != NULL) {
        widget = BuildMenu(MenuBar, XmMENU_PULLDOWN,
                           items[i].label, items[i].mnemonic,
                           items[i].tear_off, items[i].items);
        i++;
    }

    if (widget)
        XtVaSetValues(MenuBar, XmNmenuHelpWidget, widget, NULL);

    XtManageChild(MenuBar);

    return MenuBar;
}

Widget CreateOptionMenu(Widget parent, MenuBarItem *item)
{
    Widget Menu;

    Menu = BuildMenu(parent, XmMENU_OPTION,
                     item->label, item->mnemonic,
                     item->tear_off, item->items);

    return Menu;
}

void SetDefaultOptionMenuItem(Widget menu, int item_no)
{
    int nChildren;
    Widget subMenu=NULL;
    WidgetList children;
    
    if (!menu || item_no < 0) return;
    
    XtVaGetValues(menu, XmNsubMenuId, &subMenu, NULL);
    
    if (!subMenu) return;
    
    XtVaGetValues(subMenu, XmNchildren, &children,
                  XmNnumChildren, &nChildren, NULL);
    
    if (item_no >= nChildren) return;
    
    XtVaSetValues(menu, XmNmenuHistory, children[item_no], NULL);
}

void SetDefaultOptionMenuItemString(Widget menu, MenuItem *items, char *item)
{
    int i;
    
    if (!items || !item) return;
    
    for (i=0; items[i].label != NULL; i++) {
        if (!items[i].callback_data) continue;
        if (strcmp((char *)items[i].callback_data, item) == 0) {
            SetDefaultOptionMenuItem(menu, i);
            return;
        }
    }
}

void SetDefaultOptionMenuItemNumString(Widget menu, MenuItem *items,
                                       int item_num)
{
    int i;
    
    if (!items) return;
    
    for (i=0; items[i].label != NULL; i++) {
        if (!items[i].callback_data) continue;
        if (atoi(items[i].callback_data) == item_num) {
            SetDefaultOptionMenuItem(menu, i);
            return;
        }
    }
}

void QueryOptionMenuItem(Widget menu, int *index)
{
    int n, nChildren;
    Widget subMenu=NULL, current;
    WidgetList children;
    
    *index = -1;
    if (!menu) return;
    
    XtVaGetValues(menu, XmNsubMenuId, &subMenu,
                  XmNmenuHistory, &current, NULL);
    
    if (!subMenu || !current) return;
    
    XtVaGetValues(subMenu, XmNchildren, &children,
                  XmNnumChildren, &nChildren, NULL);
    
    for (n=0; n<nChildren; n++) {
        if (current == children[n]) {
            *index = n;
            return;
        }
    }
}

int QueryOptionMenuItemNumber(Widget menu, Widget qMenu)
{
    int n, nFound=-1, nChildren;
    Widget subMenu=NULL;
    WidgetList children;
    
    XtVaGetValues(menu, XmNsubMenuId, &subMenu, NULL);
    
    if (!subMenu) return nFound;
    
    XtVaGetValues(subMenu, XmNchildren, &children,
                  XmNnumChildren, &nChildren, NULL);
                  
    for (n=0; n<nChildren; n++) {
        if (qMenu == children[n]) {
            nFound = n;
            break;
        }
    }
     
    return nFound;
}

void ChangeCallbackDataInMenuItems(MenuItem *items, char *cbd)
{
    int i;
    
    if (!items || !cbd) return;
    
    for (i=0; items[i].label != NULL; i++) {
        items[i].callback_data = cbd;
    }
}

/* static int NoOfTempChildren(Widget menu)
{
    int n, nItems, userData, nTemp=0;
    WidgetList items;
    
    XtVaGetValues(menu, XmNchildren, &items, XmNnumChildren, &nItems, NULL);
    
    for (n=0; n<nItems; n++) {
        XtVaGetValues(items[n], XmNuserData, &userData, NULL);
        if (userData == TEMP_MENU_ITEM) nTemp++;
    }
    
    return nTemp;
}


static void PurgeFirstTempItem(Widget menu)
{
    int n, nItems, userData;
    WidgetList items;
    
    XtVaGetValues(menu, XmNchildren, &items, XmNnumChildren, &nItems, NULL);
    
    printf("PFT: nItems=%d\n", nItems);
    for (n=nItems-1; n>=0; n--) {
        XtVaGetValues(items[n], XmNuserData, &userData, NULL);
        if (userData == TEMP_MENU_ITEM) {
            XtUnmanageChild(items[n]);
            XtDestroyWidget(items[n]);
            printf("Purged %d.\n", n);
            return;
        }
    }
} */

static void ListMenuCB(Widget w, listWidget *lw, XmAnyCallbackStruct *cb)
{
    int n, nItems, userData;
    WidgetList items;
    /* Widget menuW; */
    list curr = NULL;
    MenuItem *m = NULL, mnull = NULLMENU;

    list list_iterator();
    int count_list();

    /* printf("Entering ListMenuCallback.\n"); */
    
    XtVaGetValues(lw->menu, XmNchildren, &items, XmNnumChildren, &nItems, NULL);
    for (n=nItems-1; n>=0; n--) {
        /* printf("n=%d of %d (-)  %10d  %10d  %10d\n",
                n, nItems, (int)items[n], (int)w, (int)lw->menu); */
        XtVaGetValues(items[n], XmNuserData, &userData, NULL);
        /* printf("n=%d of %d (%d)  %10d\n",
                n, nItems, userData, (int)items[n]); */
        if (userData == TEMP_MENU_ITEM) {
            /* printf("..... --- deleting item %d.\n", n); */
            XtUnmanageChild(items[n]);
            XtDestroyWidget(items[n]);
        }
    }
    /* while ((n=NoOfTempChildren(lw->menu))) {
        printf("Purging one item, %d left...", n);
        PurgeFirstTempItem(lw->menu);
        printf("...done.\n");
        if (n==1) break;
    } */
    
    n = count_list(*lw->lf());
    
    if (!n) return;
    
    m = (MenuItem *)XtMalloc( (n+1)*sizeof(MenuItem));

    n = 0;
    while ((curr = list_iterator(*lw->lf(), curr)) != NULL) {
        m[n] = mnull;
        m[n].label = (char *)DATA(curr);
        /* if ((XtPointer) DATA(curr) == (XtPointer)vP->from ||
            (XtPointer) DATA(curr) == (XtPointer)vP->m ||
            (XtPointer) DATA(curr) == (XtPointer)vP->p) { */
            m[n].class = &xmPushButtonWidgetClass;
        /* } else {
            m[n].class = &xmPushButtonGadgetClass;
        } */
/*
        m.mnemonic = ' ';
        m.accelerator = NULL;
        m.accel_text = NULL;
        m.tear_off = False;
        m.listfunc = NULL;
 */
        m[n].callback = lw->af;
        m[n].callback_data = (XtPointer) DATA(curr);
 /*
        m.subitems = NULL;
  */
 /*
        menuW = FillMenuItem(lw->menu, NULL,
                             XmMENU_PULLDOWN, TEMP_MENU_ITEM, &m);
        if ((XtPointer) DATA(curr) == (XtPointer)vP->from ||
            (XtPointer) DATA(curr) == (XtPointer)vP->m ||
            (XtPointer) DATA(curr) == (XtPointer)vP->p) {
            COLOR *c = GetColorInfo();
            XtVaSetValues(menuW,
                          XmNforeground, c->cols[0],
                          NULL);
        }
  */
        n++;
    }
    m[n] = mnull;
    
    /* printf("BM from ListCB with %d temp. menu items.\n", n-1); */
    BuildMenu(lw->menu, -101, "", ' ', False, m);
    
    XtFree((char *)m);
}

void CreateButtons(Widget parent, ButtonItem *items)
{
    int    i = 0;
    Widget button;

    while (items[i].label) {
        if (items[i].callback) {
            if (*items[i].class == xmArrowButtonWidgetClass ||
                *items[i].class == xmArrowButtonGadgetClass) {
                button = XtVaCreateManagedWidget(
                                   items[i].label, *items[i].class, parent,
                                   XmNtopAttachment, XmATTACH_POSITION,
                                   XmNtopPosition, items[i].info->top,
                                   XmNbottomAttachment, XmATTACH_POSITION,
                                   XmNbottomPosition, items[i].info->bottom,
                                   XmNleftAttachment, XmATTACH_POSITION,
                                   XmNleftPosition, items[i].info->left,
                                   XmNrightAttachment, XmATTACH_POSITION,
                                   XmNrightPosition, items[i].info->right,
                                   XmNarrowDirection, items[i].info->dir,
                                   XmNtraversalOn, False,
                                   NULL);
                XtAddCallback(button, XmNarmCallback,
                              items[i].callback, items[i].callback_data);
                XtAddCallback(button, XmNdisarmCallback,
                              items[i].callback, items[i].callback_data);
            } else {
                button = XtVaCreateManagedWidget(items[i].label,
                                                *items[i].class,
                                                 parent, NULL);
                XtAddCallback(button, XmNactivateCallback,
                              items[i].callback, items[i].callback_data);
            }
        } else {
            XtVaCreateManagedWidget(items[i].label, *items[i].class,
                                    parent, NULL);
        }
        i++;
    }
}

/* These three routines are currently not used
static int count_toggles()
{
    int n = 0;
    list curr = NULL;

    bool empty_list();

    if (empty_list(&togglelist) == tRUE)
        return 0;

    while ((curr = toggle_iterator(curr)) != NULL)
        n++;

    return n;
}

static CallbackData *delete_toggle(list node)
{
    CallbackData *pS = NULL;

    status delete_node();
    bool empty_list();

    if (empty_list(&togglelist) == tRUE || !node)
        return pS;

    pS = toggle_ret((CallbackData *)DATA(node));

    if (delete_node(&togglelist, node) == Error)
        return pS;

    return pS;
}

void destroy_togglelist()
{
    list curr = NULL;

    bool empty_list();
    list list_iterator();

    if (empty_list(togglelist) == tRUE)
        return;

    while ((curr = toggle_iterator(curr)) != NULL)
        delete_toggle(curr);
}
 */

void toggle_any(Widget widget, XtPointer cmd, XtPointer call_data)
{
    CallbackData *cbd = NULL;
    int *t = NULL;
    list curr = NULL;
    Widget w = NULL;

    void draw_main(), SetStdView(), redraw_graph();
    
    if (widget) {     /* Function was called via a menu selection     */
        cbd = (CallbackData *)cmd;
        if (cbd)
            t = cbd->toggle_var;
    } else {          /* Function was not called via a menu selection */
        while ((curr = toggle_iterator(curr)) != NULL) {
            cbd = (CallbackData *)DATA(curr);
            if (!cbd || !cbd->data) continue;
            if (strcmp((char *)cbd->data, (char *)cmd) != 0) continue;
            t = cbd->toggle_var;
            if (!t) continue;
            w = cbd->widget;
            break;
        }
    }
    
    if (!t) return;

    if (*t) { /* Change the value of the toggle variable */
        *t = 0;
    } else {
        if (cbd->value)
            *t = *(cbd->value);
        else
            *t = 1;
    }
    
    /* If w exists we need to change the status of the widget */
    if (w) XtVaSetValues(w, XmNset, (*t) ? True : False, NULL);
    
    if (strcmp(cbd->data, "header") == 0) {
        SetStdView();
        redraw_graph(gp->top, "update", NULL);
    } else {
        draw_main();
    }
}

int *QueryAnyToggle(char *cmd)
{
    CallbackData *cbd = NULL;
    int *t = NULL;
    list curr = NULL;
    
    while ((curr = toggle_iterator(curr)) != NULL) {
        cbd = (CallbackData *)DATA(curr);
        if (!cbd || !cbd->data) continue;
        if (strcmp((char *)cbd->data, (char *)cmd) != 0) continue;
        t = cbd->toggle_var;
        break;
    }
    return t;
}

void SetAnyToggle(char *cmd, int draw)
{
    CallbackData *cbd = NULL;
    int *t = NULL;
    list curr = NULL;
    Widget w = NULL;

    void draw_main();
    
    while ((curr = toggle_iterator(curr)) != NULL) {
        cbd = (CallbackData *)DATA(curr);
        if (!cbd || !cbd->data) continue;
        if (strcmp((char *)cbd->data, (char *)cmd) != 0) continue;
        t = cbd->toggle_var;
        if (!t) continue;
        w = cbd->widget;
        break;
    }
    if (!t) return;

    if (*t == 0) {
        if (cbd->value)
            *t = *(cbd->value);
        else
            *t = 1;
        /* If w exists we also need to change the status of the widget */
        if (w) XtVaSetValues(w, XmNset, True, NULL);
    }
    
    if (draw) draw_main();
}

void UnsetAnyToggle(char *cmd, int draw)
{
    CallbackData *cbd = NULL;
    int *t = NULL;
    list curr = NULL;
    Widget w = NULL;

    void draw_main();
    
    while ((curr = toggle_iterator(curr)) != NULL) {
        cbd = (CallbackData *)DATA(curr);
        if (!cbd || !cbd->data) continue;
        if (strcmp((char *)cbd->data, (char *)cmd) != 0) continue;
        t = cbd->toggle_var;
        if (!t) continue;
        w = cbd->widget;
        break;
    }
    if (!t) return;

    if (*t) {
        *t = 0;
        /* If w exists we also need to change the status of the widget */
        if (w) XtVaSetValues(w, XmNset, False, NULL);
    }
    
    if (draw) draw_main();
}

void SetMaxMenuItems(int n)
{
    if (n < 0) return;
    
    MaxMenuItems = n;
}
