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
#include <string.h>
#include <stdarg.h>

#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>

#define BUFSIZE 1000
void PostErrorDialog(Widget, char *);


static char buf[BUFSIZE];

void wprintf(Widget w, char *format, ...)
{
    va_list   args;
    char      str[BUFSIZE];
    XmString  xmstr;
  
    if (!w) {
        sprintf(buf,
"Internal error: wprintf(w, %s, ...) requires an existing Widget", format);
        PostErrorDialog(NULL, buf);
        return;
    }
        
    if (!XtIsSubclass(w, xmLabelWidgetClass) &&
        !XtIsSubclass(w, xmTextWidgetClass) &&
        !XtIsSubclass(w, xmToggleButtonWidgetClass)) {
        sprintf(buf,
"Internal error: wprintf(w, %s, ...) requires a Label or Text widget", format);
        PostErrorDialog(NULL, buf);
        return;
    }
    
    va_start(args, format);
  
    vsprintf(str, format, args);

    if (XtIsSubclass(w, xmLabelWidgetClass) ||
        XtIsSubclass(w, xmToggleButtonWidgetClass)) {
        xmstr =  XmStringCreateLtoR(str, XmSTRING_DEFAULT_CHARSET);
        XtVaSetValues(w, XmNlabelString, xmstr, NULL);
        XmStringFree(xmstr);
    } else {
        XmTextSetString(w, str);
    }

    va_end(args);
}

void wdscanf(Widget w, double *d)
{
    if (!w) {
        sprintf(buf,
"Internal error: wdscanf(w, double *d) requires an existing Widget");
        PostErrorDialog(NULL, buf);
        return;
    }
        
    if (!XtIsSubclass(w, xmTextWidgetClass)) {
        sprintf(buf,
"Internal error: wdscanf(w, double *d) requires a Text Widget");
        PostErrorDialog(NULL, buf);
        return;
    }
    
    if (!d) {
        sprintf(buf,
"Internal error: wdscanf(w, double *d) detected d=NULL");
        PostErrorDialog(w, buf);
        return;
    }

    sscanf(XmTextGetString(w), "%lf", d);
}

void wiscanf(Widget w, int *i)
{
    if (!w) {
        sprintf(buf,
"Internal error: wiscanf(w, int *i) requires an existing Widget");
        PostErrorDialog(NULL, buf);
        return;
    }
        
    if (!XtIsSubclass(w, xmTextWidgetClass)) {
        sprintf(buf,
"Internal error: wiscanf(w, int *i) requires a Text Widget");
        PostErrorDialog(NULL, buf);
        return;
    }
    
    if (!i) {
        sprintf(buf,
"Internal error: wiscanf(w, int *i) detected i=NULL");
        PostErrorDialog(w, buf);
        return;
    }

    sscanf(XmTextGetString(w), "%d", i);
}

void wsscanf(Widget w, char *s)
{
    if (!w) {
        sprintf(buf,
"Internal error: wsscanf(w, char *s) requires an existing Widget");
        PostErrorDialog(NULL, buf);
        return;
    }
        
    if (!XtIsSubclass(w, xmTextWidgetClass)) {
        sprintf(buf,
"Internal error: wsscanf(w, char *s) requires a Text Widget");
        PostErrorDialog(NULL, buf);
        return;
    }
    
    if (!s) {
        sprintf(buf,
"Internal error: wsscanf(w, char *s) detected s=NULL");
        PostErrorDialog(w, buf);
        return;
    }

    strcpy(s, XmTextGetString(w));
}

void  wfscanf(Widget w, float *f)
{
    if (!w) {
        sprintf(buf,
"Internal error: wfscanf(w, float *f) requires an existing Widget");
        PostErrorDialog(NULL, buf);
        return;
    }
        
    if (!XtIsSubclass(w, xmTextWidgetClass)) {
        sprintf(buf,
"Internal error: wfscanf(w, float *f) requires a Text Widget");
        PostErrorDialog(NULL, buf);
        return;
    }
    
    if (!f) {
        sprintf(buf,
"Internal error: wfscanf(w, float *f) detected f=NULL");
        PostErrorDialog(w, buf);
        return;
    }

    sscanf(XmTextGetString(w), "%f", f);
}
