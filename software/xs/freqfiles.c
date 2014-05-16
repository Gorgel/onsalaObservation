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
#include <stdlib.h>
#include <math.h>
#include <Xm/Xm.h>

#include "defines.h"
#include "global_structs.h"

/*** External variables ***/
extern VIEW  *vP;
extern MARK   marks[MAXMARK];
extern int    nmark, mark_xunit;
extern USER  *pP;

void PostErrorDialog(Widget, char *);

/*** Local variables ***/
/* static int sl_first[] =  { 1, 15, 25, 35, 45, 55,  81, 102, 107, 115, 122};
static int sl_last[]  =  {13, 24, 32, 43, 53, 79, 100, 106, 113, 120, 128}; */
static int sl_first[] =  { 0, 13, 25, 33, 51, 60,  88, 102, 109, 115, 123};
static int sl_last[]  =  {12, 23, 30, 43, 58, 86, 107, 106, 113, 121, 128};
static int jp_first[] =  { 0, 16, 28, 36, 44, 46, 56, 59, 66, 70};
static int jp_last[]  =  {15, 27, 35, 43, 45, 55, 58, 65, 69, 91};
static int id_first[] =  { 0, 64};
static int id_last[]  =  {9, 75};
/* static int lo_first[] =  {2, 18};
static int lo_last[]  =  {11, 27}; */
static int lo_first[] =  {2, 18};
static int lo_last[]  =  {11, 29};

static void make_freq_file(char *, int, double, double);
static void obtain_slaim_data(char *, SLAIM *);
static void obtain_jpl_data(char *, SLAIM *);
static void obtain_ident_data(char *, SLAIM *);
static void obtain_sgrb2_data(char *, SLAIM *);
static void obtain_lovas_data(char *, SLAIM *);

void open_freq_file(Widget w, char *str, XtPointer call_data)
{
    double fl, fu, tmp;

    void SetWatchCursor(), SetAnyToggle();
    double SpecUnitConv();

    fl = SpecUnitConv(UNIT_FRE, vP->xunit, vP->xleft);
    fu = SpecUnitConv(UNIT_FRE, vP->xunit, vP->xright);

    if (fl > fu) {tmp = fl; fl = fu; fu = tmp;}

    SetWatchCursor(True);
    
    if (strncmp(str, "SLAIM", 5)==0) {
        make_freq_file("SLAIM", 0, fl, fu);
    } else if (strncmp(str, "JPL", 3)==0) {
        make_freq_file("JPL", 0, fl, fu);
    } else if (strncmp(str, "LOVAS", 5)==0) {
        make_freq_file("LOVAS", 0, fl, fu);
    } else if (strncmp(str, "iSLAIM", 6)==0) {
        make_freq_file("SLAIM", -1, fl, fu);
    } else if (strncmp(str, "iJPL", 4)==0) {
        make_freq_file("JPL", -1, fl, fu);
    } else if (strncmp(str, "iLOVAS", 6)==0) {
        make_freq_file("LOVAS", -1, fl, fu);
    } else if (strncmp(str, "mSLAIM", 6)==0) {
        make_freq_file("SLAIM", 1, fl, fu);
        SetAnyToggle("markers", 1);
    } else if (strncmp(str, "mJPL", 4)==0) {
        make_freq_file("JPL", 1, fl, fu);
        SetAnyToggle("markers", 1);
    } else if (strncmp(str, "mIDENT", 6)==0) {
        make_freq_file("IDENT", 1, fl, fu);
        SetAnyToggle("markers", 1);
    } else if (strncmp(str, "sgrb2", 5)==0) {
        make_freq_file(str, 1, fl, fu);
        SetAnyToggle("markers", 1);
    } else if (strncmp(str, "orion", 5)==0) {
        make_freq_file(str, 1, fl, fu);
        SetAnyToggle("markers", 1);
    } else if (strncmp(str, "mLOVAS", 6)==0) {
        make_freq_file("LOVAS", 1, fl, fu);
        SetAnyToggle("markers", 1);
    } else if (strncmp(str, "imSLAIM", 7)==0) {
        make_freq_file("SLAIM", 2, fl, fu);
        SetAnyToggle("markers", 1);
    } else if (strncmp(str, "imJPL", 5)==0) {
        make_freq_file("JPL", 2, fl, fu);
        SetAnyToggle("markers", 1);
    } else if (strncmp(str, "imLOVAS", 7)==0) {
        make_freq_file("LOVAS", 2, fl, fu);
        SetAnyToggle("markers", 1);
    }
    
    SetWatchCursor(False);
}

static void make_freq_file(char *str, int make_marks, double f1, double f2)
{
    int i = 0, njoin, image = 0;
    FILE *fpr, *fpw, *fopen();
    string f_file, t_file, foo;
    char buf[MAXBUFSIZE];
    double offset, fl, fu, f0, f2LO=0.0;
    SLAIM data;
    
    void XS_system(), send_line();
    double SpecUnitConv();
    char *GetTmpFile();
    
    fl = f1; fu = f2;
    
    if ((make_marks == 2 || make_marks == -1) && vP->s) {
        f2LO = 2.0 * vP->s->lofreq;
        fl = f2LO - f2;
        fu = f2LO - f1;
	image = 1;
    }
    f0 = (fl + fu)/2.0;

    if (strncmp(str, "SLAIM", 5)==0) {
        if (f0 > 200.0 && f0 < 300.0) {
            sprintf(f_file, "%s/%s2", pP->slaimDir, pP->slaimFile);
        } else if (f0 >= 300.0) {
            sprintf(f_file, "%s/%s3", pP->slaimDir, pP->slaimFile);
        } else {
            sprintf(f_file, "%s/%s1", pP->slaimDir, pP->slaimFile);
        }
        strcpy(t_file, GetTmpFile("slaim"));
        offset = 0.1 + image*0.1;
    } else if (strncmp(str, "JPL", 3)==0) {
        if (f0 > 15.0 && f0 < 70.0) {
            sprintf(f_file, "%s/jpl.015-070", pP->jplDir);
        } else if (f0 >=  70.0 && f0 < 120.0) {
            sprintf(f_file, "%s/jpl.070-120", pP->jplDir);
        } else if (f0 >= 120.0 && f0 < 200.0) {
            sprintf(f_file, "%s/jpl.120-200", pP->jplDir);
        } else if (f0 >= 200.0 && f0 < 300.0) {
            sprintf(f_file, "%s/jpl.200-300", pP->jplDir);
        } else if (f0 >= 300.0 && f0 < 400.0) {
            sprintf(f_file, "%s/jpl.300-400", pP->jplDir);
        } else if (f0 >= 400.0 && f0 < 500.0) {
            sprintf(f_file, "%s/jpl.400-500", pP->jplDir);
        } else if (f0 >= 500.0 && f0 < 600.0) {
            sprintf(f_file, "%s/jpl.500-600", pP->jplDir);
        } else {
            sprintf(f_file, "Don't have a JPL file for f=%f GHz.", f0);
            send_line(f_file);
            return;
        }
        strcpy(t_file, GetTmpFile("jpl"));
        offset = 0.2 + image*0.1;
    } else if (strncmp(str, "IDENT", 5)==0) {
        sprintf(f_file, "%s/%s", pP->identDir, pP->identFile);
        strcpy(t_file, GetTmpFile("ident"));
        offset = 0.3;
    } else if (strncmp(str, "sgrb2", 5)==0) {
        sprintf(f_file, "%s/%s", pP->identDir, str);
        strcpy(t_file, GetTmpFile(str));
        offset = 0.1;
    } else if (strncmp(str, "orion", 5)==0) {
        sprintf(f_file, "%s/%s", pP->identDir, str);
        strcpy(t_file, GetTmpFile(str));
        offset = 0.0;
    } else if (strncmp(str, "LOVAS", 5)==0) {
        sprintf(f_file, "%s/%s", pP->lovasDir, pP->lovasFile);
        strcpy(t_file, GetTmpFile("lovas"));
        offset = 0.4 + image*0.1;
    } else {
        return;
    }
    if ((fpr = fopen(f_file, "r")) == NULL) {
        sprintf(foo, "Unable to open %s for reading.", f_file);
        PostErrorDialog(NULL, foo);
        return;
    }
    sprintf(foo, "%s %s", pP->unixRmCmd, t_file);
    XS_system(foo, 0);
    if ((fpw = fopen(t_file, "w")) == NULL) {
        sprintf(foo, "Unable to open %s for writing.", t_file);
        PostErrorDialog(NULL, foo);
        if (fpr) fclose(fpr);
        return;
    }

    while((fgets(buf, MAXBUFSIZE, fpr)) != NULL) {
        buf[strlen(buf)-1] = '\0';
        if (strncmp(str, "SLAIM", 5)==0) { 
            obtain_slaim_data(buf, &data);
        } 
        if (strncmp(str, "JPL", 3)==0) {
            obtain_jpl_data(buf, &data);
        } 
        if (strncmp(str, "IDENT", 5)==0) {
            if (buf[0] != ' ') continue;
            obtain_ident_data(buf, &data);
        }
        if (strncmp(str, "sgrb2", 5)==0 || strncmp(str, "orion", 5)==0) {
            if (str[0] == '#' || str[0] == '!') continue;
            obtain_sgrb2_data(buf, &data);
        }
        if (strncmp(str, "LOVAS", 5)==0) {
            obtain_lovas_data(buf, &data);
        }
        if (data.fre_cal >= 1000.0*fl && data.fre_cal <= 1000.0*fu) {
            fprintf(fpw, "%s\n", buf);
            if (make_marks >= 1 && nmark < MAXMARK) {
                mark_xunit = vP->xunit;
		if (image) {
                  f0 = SpecUnitConv(mark_xunit, UNIT_FRE,
		                     f2LO - data.fre_cal/1000.0);
		} else {
                  f0 = SpecUnitConv(mark_xunit, UNIT_FRE, data.fre_cal/1000.0);
		}
                marks[nmark].mode = 0;
                marks[nmark].x = f0;
                if (offset == 0.0) {
                    marks[nmark].y = data.line_str;
                    marks[nmark].align = 0.0;
                    marks[nmark].type  = MARK_TYPE_LINE;
                } else {
                    marks[nmark].y = vP->yupper - offset*vP->yrange;
                    marks[nmark].align = 0.5;
                    marks[nmark].type  = MARK_TYPE_ARROW;
                }
                marks[nmark].angle = 90.0;
                marks[nmark].xlength = 20;
                marks[nmark].ylength = 20;
                marks[nmark].tagged = 0;
                njoin = nmark + data.joined;
                if (njoin != nmark && njoin >= 0 && njoin < MAXMARK)
                    marks[nmark].mark = &marks[njoin];
                else
                    marks[nmark].mark = NULL;
                marks[nmark].dir = MARK_DIR_DOWN;
                strcpy(marks[nmark].label, data.mol);
                nmark++;
            }
            i++;
        }
    }
    fclose(fpw);
    fclose(fpr);
    if (make_marks <= 0) {
        sprintf(foo, "%s %s &", pP->editor, t_file);
        XS_system(foo, 1);
    }
}

static void obtain_slaim_data(char *str, SLAIM *d)
{
    string tmp;
    int i, j, n, j1, j2;
    
    for (i=0; i<11; i++) {
        j1 = sl_first[i];
        j2 = sl_last[i];
        n = 0;
        for (j=j1; j<=j2; j++) {
            if (n > 0 || (n == 0 && str[j] != ' ')) {
                tmp[n] = str[j];
                n++;
            }
        }
        j = n - 1;
        while (tmp[j] == ' ' && j > 0) j--;
        tmp[j+1] = '\0';
        switch (i) {
            case 0:
                strcpy(d->mol, tmp);
                break;
            case 1:
                sscanf(tmp, "%lf", &(d->fre_cal));
                break;
            case 2:
                strcpy(d->fre_unc, tmp);
                break;
            case 3:
                d->fre_obs = UNDEF;
                if (strlen(tmp) > 0) sscanf(tmp, "%lf", &(d->fre_obs));
                break;
            case 4:
                d->e_lower = UNDEF;
                if (strlen(tmp) > 0) {
                    sscanf(tmp, "%lf", &(d->e_lower));
                    d->e_lower *= INVCMTOKELVIN;
                }
                break;
            case 5:
                strcpy(d->tra, tmp);
                break;
            case 6:
                strcpy(d->tra_hf, tmp);
                break;
            case 7:
                d->rel_hf = UNDEF;
                if (strlen(tmp) > 0) sscanf(tmp, "%lf", &(d->rel_hf));
                break;
            case 8:
                d->line_str = UNDEF;
                if (strlen(tmp) > 0) sscanf(tmp, "%lf", &(d->line_str));
                break;
            case 9:
                d->log_aul = UNDEF;
                if (strlen(tmp) > 0) {
                    sscanf(tmp, "%lf", &(d->log_aul));
                    d->log_aul = 1.0/pow(10.0, d->log_aul);
                }
                break;
            case 10:
                strcpy(d->refs, tmp);
                break;
        }
    }
    d->joined = 0;
}

static void obtain_jpl_data(char *str, SLAIM *d)
{
    string tmp;
    int i, j, n, j1, j2;
    
    for (i=0; i<10; i++) {
        j1 = jp_first[i];
        j2 = jp_last[i];
        n = 0;
        for (j=j1; j<=j2; j++) {
            if (n > 0 || (n == 0 && str[j] != ' ')) {
                tmp[n] = str[j];
                n++;
            }
        }
        j = n - 1;
        while (tmp[j] == ' ' && j > 0) j--;
        tmp[j+1] = '\0';
        switch (i) {
            case 0:
                strcpy(d->mol, tmp);
                break;
            case 1:
                sscanf(tmp, "%lf", &(d->fre_cal));
                break;
            case 2:
                strcpy(d->fre_unc, tmp);
                break;
            case 3:
                d->log_aul = UNDEF;
                if (strlen(tmp) > 0) {
                    sscanf(tmp, "%lf", &(d->log_aul));
                    d->log_aul = 1.0/pow(10.0, d->log_aul);
                }
                break;
            case 4:
                break;
            case 5:
                d->e_lower = UNDEF;
                if (strlen(tmp) > 0) {
                    sscanf(tmp, "%lf", &(d->e_lower));
                    d->e_lower *= INVCMTOKELVIN;
                }
                break;
            case 6:
                d->rel_hf = UNDEF;
                if (strlen(tmp) > 0) sscanf(tmp, "%lf", &(d->rel_hf));
                break;
            case 7:
                strcpy(d->refs, tmp);
                break;
            case 8:
                strcpy(d->tra_hf, tmp);
                break;
            case 9:
                strcpy(d->tra, tmp);
                break;
        }
    }
    d->joined = 0;
}

static void obtain_ident_data(char *str, SLAIM *d)
{
    string tmp;
    int i, j, n, j1, j2;
    
    for (i=0; i<2; i++) {
        j1 = id_first[i];
        j2 = id_last[i];
        n = 0;
        for (j=j1; j<=j2; j++) {
            if (n > 0 || (n == 0 && str[j] != ' ')) {
                tmp[n] = str[j];
                n++;
            }
        }
        j = n - 1;
        while (tmp[j] == ' ' && j > 0) j--;
        tmp[j+1] = '\0';
        switch (i) {
            case 1:
                strcpy(d->mol, tmp);
                break;
            case 0:
                sscanf(tmp, "%lf", &(d->fre_cal));
                d->fre_cal = d->fre_cal*1000.0;
                break;
        }
    }
    d->joined = 0;
}

static void obtain_sgrb2_data(char *str, SLAIM *d)
{
    int n = 0;
    string tmp;
    char *p;
    
    strcpy(tmp, str);
    
    p = strtok(tmp, "&");
    
    d->joined = 0;
    strcpy(d->mol, "");
    
    while (p) {
        if (n == 0) {
            d->fre_cal = atof(p);
        } else if (n == 1) {
            d->line_str = atof(p);
        } else if (n == 2) {
            strcpy(d->mol, p);
        } else if (n == 3) {
            d->joined = atoi(p);
        } else {
            break;
        }
        p = strtok(NULL, "&\n");
        n++;
    }
}

static void obtain_lovas_data(char *str, SLAIM *d)
{
    char tmp[80];
    int i, j, n, j1, j2;
    
    for (i=0; i<2; i++) {
        j1 = lo_first[i];
        j2 = lo_last[i];
        n = 0;
        for (j=j1; j<=j2; j++) {
            if (n > 0 || (n == 0 && str[j] != ' ')) {
                tmp[n] = str[j];
                n++;
            }
        }
        j = n - 1;
        while (tmp[j] == ' ' && j > 0) j--;
        tmp[j+1] = '\0';
        switch (i) {
            case 1:
                strcpy(d->mol, tmp);
                break;
            case 0:
                sscanf(tmp, "%lf", &(d->fre_cal));
                break;
        }
    }
    d->joined = 0;
}
