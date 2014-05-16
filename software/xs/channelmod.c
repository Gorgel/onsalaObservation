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
#include <Xm/Xm.h>

#include "defines.h"
#include "global_structs.h"

/*** Global variables ***/
extern MOD    mods[MAXMOD];
extern int    nmod, rbox_sel, mod_sel;
extern VIEW  *vP; 

void PostErrorDialog(Widget, char *);
void PostWarningDialog(Widget, char *);
void PostMessageDialog(Widget, char *);

/*** Local variables ***/

void new_mod(int chan, double new_value)
{
    string buf;
    
    void UpdateData();

    if (!vP->s) {
        PostErrorDialog(NULL, "No data at all to modify.");
        return;
    }

    if (nmod < MAXMOD) {
        if (vP->s && chan >= 0 && chan < vP->s->nChan) {
            mods[nmod].chan = chan;
            mods[nmod].old  = vP->s->d[chan];
            mods[nmod].new  = new_value;
            nmod++;
            vP->s->d[chan] = new_value;
            vP->s->saved = 0;
            UpdateData(SCALE_NONE, REDRAW);
        } else {
            sprintf(buf, "Selected channel %d is outside of spectrum.", chan);
            PostErrorDialog(NULL, buf);
        }
    } else {
        sprintf(buf, "Too many channel modifications: %d > %d.",
                nmod+1, MAXMOD);
        PostErrorDialog(NULL, buf);
    }
}

void mod_reset(Widget w, char *client_data, XtPointer call_data)
{
    int n;
    
    void draw_main();

    if (!vP->s) {
        PostErrorDialog(NULL, "No data at all to modify.");
        return;
    }

    if (strncmp(client_data, "all", 3) == 0) {
        for (n=nmod-1; n>=0; n--) {
            if (mods[n].chan >= 0 && mods[n].chan < vP->s->nChan)
                vP->s->d[mods[n].chan] = mods[n].old;
        }
        vP->s->saved = 0;
        nmod = 0;
    } else {     
        nmod--;
        if (nmod < 0) {
            nmod = 0;
        } else {
            if (mods[nmod].chan >= 0 && mods[nmod].chan < vP->s->nChan)
                vP->s->d[mods[nmod].chan] = mods[nmod].old;
            vP->s->saved = 0;
        }
    }
    draw_main();
}

void channel_mod(Widget w, char *client_data, XtPointer call_data)
{
    if (mod_sel == 0) {
        rbox_sel = 0;
        mod_sel = 1;
    }
}
