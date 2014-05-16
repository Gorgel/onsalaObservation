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

#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>

#include "defines.h"
#include "global_structs.h"
#include "dialogs.h"

static void destroy_stdform(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    if (sf) {
        /* printf("Freed stdform\n"); */
        if (sf->user) {
            if (sf->clean_up) sf->clean_up(sf->user);
            XtFree((char *)(sf->user));
        }
        if (sf->edit) XtFree((char *)(sf->edit));
        XtFree((char *)sf);
    }
}

static void unmap_stdform(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    if (sf) XtDestroyWidget(sf->form);
}

static void cancel_std_form_dialog(Widget w, StdForm *sf, XmAnyCallbackStruct *cb)
{
    if (sf) XtDestroyWidget(sf->form);
}

void cancel_dialog(Widget w, Widget dialog, XmAnyCallbackStruct *cb)
{
    XtDestroyWidget(dialog);
}

void help_dialog(Widget w, char *str, XmAnyCallbackStruct *cb)
{
    string title;

    void PostScrolledMessageDialog(Widget, char *, char *);
    
    sprintf(title, "%s help", PKGNAME);
    
    PostScrolledMessageDialog(w, str, title);
}

StdForm *PostStdFormDialog(
             Widget parent, char *title,
             char *apply,  XtCallbackProc a_cb, XtPointer a_data,
             char *cancel, XtCallbackProc c_cb, XtPointer c_data,
             char *help,   XtCallbackProc h_cb, XtPointer h_data,
             int numEdits,
             void (*f)()
             )
{
    int n;
    Arg wargs[10];
    string tbuf;
    StdForm *sf;
    
    sf = (StdForm *)XtMalloc(sizeof(StdForm));
    if (!sf) return NULL;
        
    sf->n = numEdits;
    sf->edit = NULL;
    sf->user = sf->any = NULL;
    sf->apply = sf->cancel = sf->help = sf->content = NULL;
    sf->clean_up = NULL;
    sf->user_func = NULL;
    if (f) sf->clean_up = f;

    if (sf->n > 0) {
        sf->edit = (Widget *)XtMalloc(numEdits * sizeof(Widget));
        if (!sf->edit) {
            XtFree((char *)sf);
            return NULL;
        }
    }
    
    n = 0;
    if (title) {
        XtSetArg(wargs[n], XmNtitle, title); n++;
    } else {
        sprintf(tbuf, "%s Dialog", PKGNAME);
        XtSetArg(wargs[n], XmNtitle, tbuf); n++;
    }
    XtSetArg(wargs[n], XmNautoUnmanage, False); n++;
    sf->form = XmCreateFormDialog(parent, "form", wargs, n);

    XtAddCallback(sf->form, XmNdestroyCallback,
                  (XtCallbackProc)destroy_stdform, sf);

    XtAddCallback(sf->form, XmNunmapCallback,
                  (XtCallbackProc)unmap_stdform, sf);
    
    sf->sep = XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
				                  sf->form, XmNseparatorType, XmSHADOW_ETCHED_IN,
				                  NULL);

    sf->button_form = XtVaCreateManagedWidget("buttonform", xmFormWidgetClass,
                                              sf->form, NULL);
    
    if (apply) {
        sf->apply  = XtVaCreateManagedWidget(apply, xmPushButtonWidgetClass,
                                             sf->button_form, NULL);
        if (a_cb) {
            XtAddCallback(sf->apply, XmNactivateCallback,
                          a_cb, a_data ? a_data : sf);
        }
    }
    if (cancel) {
        sf->cancel = XtVaCreateManagedWidget(cancel, xmPushButtonWidgetClass,
                                             sf->button_form, NULL);
        if (c_cb) {
            XtAddCallback(sf->cancel, XmNactivateCallback,
                          c_cb, c_data ? c_data : sf);
        } else {
            XtAddCallback(sf->cancel, XmNactivateCallback,
                          (XtCallbackProc)cancel_std_form_dialog, sf);
        }
    }
    if (help) {
        sf->help   = XtVaCreateManagedWidget(help, xmPushButtonWidgetClass,
                                             sf->button_form, NULL);
        if (h_cb) {
            XtAddCallback(sf->help, XmNactivateCallback,
                          h_cb, h_data);
        } else {
            XtAddCallback(sf->help, XmNactivateCallback,
                          (XtCallbackProc)help_dialog, h_data);
        }
    }
    return sf;
}

void ArrangeStdFormDialog(StdForm *sf, Widget content)
{
    if (!sf) return;
    
    sf->content = content;
    
    if (sf->apply) {
        XtVaSetValues(sf->form, XmNdefaultButton, sf->apply, NULL);
    } else if (sf->cancel) {
        XtVaSetValues(sf->form, XmNdefaultButton, sf->cancel, NULL);
    }
    
    if (sf->content) {
        XtVaSetValues(sf->content,
            XmNtopAttachment, XmATTACH_FORM,
            XmNtopOffset, 10,
            XmNleftAttachment, XmATTACH_FORM,
            XmNleftOffset, 10,
            XmNrightAttachment, XmATTACH_FORM,
            XmNrightOffset, 10,
            NULL);
   }
   if (sf->sep) {
        XtVaSetValues(sf->sep,
            XmNtopAttachment, XmATTACH_WIDGET,
            XmNtopWidget, sf->content,
            XmNtopOffset, 10,
            XmNleftAttachment, XmATTACH_FORM,
            XmNrightAttachment, XmATTACH_FORM,
            NULL);
   }
   if (sf->button_form) {
        XtVaSetValues(sf->button_form,
            XmNtopAttachment, XmATTACH_WIDGET,
            XmNtopWidget, sf->sep,
            XmNleftAttachment, XmATTACH_FORM,
            XmNrightAttachment, XmATTACH_FORM,
            XmNbottomAttachment, XmATTACH_FORM,
            NULL);
        if (sf->apply) {
            XtVaSetValues(sf->apply,
                XmNtopAttachment, XmATTACH_FORM,
                XmNtopOffset, 10,
                XmNleftAttachment, XmATTACH_FORM,
                XmNleftOffset, 10,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNbottomOffset, 10,
                NULL);
        }
        if (sf->cancel) {
            XtVaSetValues(sf->cancel,
                XmNleftAttachment, sf->apply ? XmATTACH_WIDGET : XmATTACH_FORM,
                XmNleftOffset, 10,
                XmNtopAttachment, XmATTACH_FORM,
                XmNtopOffset, 10,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNbottomOffset, 10,
                NULL);
           if (sf->apply) {
                XtVaSetValues(sf->cancel, XmNleftWidget, sf->apply, NULL);
           }
        }
        if (sf->help) {
            XtVaSetValues(sf->help,
                XmNrightAttachment, XmATTACH_FORM,
                XmNrightOffset, 10,
                XmNtopAttachment, XmATTACH_FORM,
                XmNtopOffset, 10,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNbottomOffset, 10,
                NULL);
        }
   }
}

void StdApplyCancel(Widget above, Widget t, Widget s, Widget a, Widget c,
                    Widget h)
{
    int n;
    Arg wargs[10];
    
    if (t) {
        n = 0;
        if (above) {
            XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
            XtSetArg(wargs[n], XmNtopWidget,           above); n++;
        } else {
            XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_FORM); n++;
        }
        XtSetArg(wargs[n], XmNtopOffset,           10); n++;
        XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_FORM); n++;
        XtSetArg(wargs[n], XmNleftOffset,          10); n++;
        XtSetArg(wargs[n], XmNrightAttachment,     XmATTACH_FORM); n++;
        XtSetArg(wargs[n], XmNrightOffset,         10); n++;
        XtSetValues(t, wargs, n);
    }
    
    n = 0;
    if (t) {
        XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
        XtSetArg(wargs[n], XmNtopWidget,           t); n++;
    } else {
        XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_FORM); n++;
    }
    if (s) {
        XtSetArg(wargs[n], XmNtopOffset,           10); n++;
        XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_FORM); n++;
        XtSetArg(wargs[n], XmNleftOffset,          1); n++;
        XtSetArg(wargs[n], XmNrightAttachment,     XmATTACH_FORM); n++;
        XtSetArg(wargs[n], XmNrightOffset,         1); n++;
        XtSetValues(s, wargs, n);
    }
    
    n = 0;
    if (s) {
        XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
        XtSetArg(wargs[n], XmNtopWidget,           s); n++;
    } else if (t) {
        XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
        XtSetArg(wargs[n], XmNtopWidget,           t); n++;
    } else {
        XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_FORM); n++;
    }
    XtSetArg(wargs[n], XmNtopOffset,           10); n++;
    XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset,          20); n++;
    XtSetArg(wargs[n], XmNbottomAttachment,    XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset,        10); n++;
    if (a) {
        XtSetValues(a, wargs, n);
    } else if (c) {
        XtSetValues(c, wargs, n);
        if (!h) return;
    }
    
    if (c && a) {
        n = 0;
        if (s) {
            XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
            XtSetArg(wargs[n], XmNtopWidget,           s); n++;
        } else if (t) {
            XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
            XtSetArg(wargs[n], XmNtopWidget,           t); n++;
        } else {
            XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_FORM); n++;
        }
        XtSetArg(wargs[n], XmNtopOffset,           10); n++;
        XtSetArg(wargs[n], XmNleftAttachment,      XmATTACH_WIDGET); n++;
        XtSetArg(wargs[n], XmNleftWidget,          a); n++;
        XtSetArg(wargs[n], XmNleftOffset,          10); n++;
        XtSetArg(wargs[n], XmNbottomAttachment,    XmATTACH_FORM); n++;
        XtSetArg(wargs[n], XmNbottomOffset,        10); n++;
        XtSetValues(c, wargs, n);
    }
    
    if (h) {
        n = 0;
        if (s) {
            XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
            XtSetArg(wargs[n], XmNtopWidget,           s); n++;
        } else if (t) {
            XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_WIDGET); n++;
            XtSetArg(wargs[n], XmNtopWidget,           t); n++;
        } else {
            XtSetArg(wargs[n], XmNtopAttachment,       XmATTACH_FORM); n++;
        }
        XtSetArg(wargs[n], XmNtopOffset,           10); n++;
        XtSetArg(wargs[n], XmNbottomAttachment,    XmATTACH_FORM); n++;
        XtSetArg(wargs[n], XmNbottomOffset,        10); n++;
        if (c) {
            XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_WIDGET); n++;
            XtSetArg(wargs[n], XmNleftWidget,        c); n++;
            XtSetArg(wargs[n], XmNleftOffset,        10); n++;
        } else if (a) {
            XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_WIDGET); n++;
            XtSetArg(wargs[n], XmNleftWidget,        a); n++;
            XtSetArg(wargs[n], XmNleftOffset,        10); n++;
        } else {
            XtSetArg(wargs[n], XmNleftAttachment,    XmATTACH_FORM); n++;
            XtSetArg(wargs[n], XmNleftOffset,        20); n++;
        }
        XtSetValues(h, wargs, n);
    }
}
