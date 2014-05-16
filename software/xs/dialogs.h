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
typedef struct {
    Widget form;
    Widget content;
    Widget sep, button_form;
    Widget apply, cancel, help;
    int n;
    Widget *edit;
    XtPointer any;
    XtPointer user;
    void (*clean_up)();
    void (*user_func)();
} StdForm;

StdForm *PostStdFormDialog(
             Widget, char *,
             char *,  XtCallbackProc, XtPointer,
             char *,  XtCallbackProc, XtPointer,
             char *,  XtCallbackProc, XtPointer,
             int,
             void (*f)()
             );

void ArrangeStdFormDialog(StdForm *, Widget);
