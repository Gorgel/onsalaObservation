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
#include <math.h>

#include <Xm/FileSB.h>

#include "defines.h"
#include "global_structs.h"
#include "menus.h"

#include "files.h"

/* #define DEBUG */

/* External declarations */

list      *get_listlist();
DataSetPtr new_dataset(list *, char *, DataSetPtr);
void       DeleteLastDataSet();

/* Local declarations */

void init_file_parameters()
{
    strcpy(data_dir,      pP->dataDir);
    strcpy(seco_dir,      pP->dataDir);
    strcpy(ps_dir,        pP->dataDir);
    strcpy(ants_dir,      pP->dataDir);
    strcpy(r_sin_f_dir,   pP->dataDir);
    strcpy(r_seq_f_dir,   pP->dataDir);
    strcpy(r_map_f_dir,   pP->dataDir);
    strcpy(r_2d_f_dir,    pP->dataDir);
    strcpy(r_3d_f_dir,    pP->dataDir);
    strcpy(wfits_dir,     pP->dataDir);
    strcpy(drp_dir,       pP->dataDir);
    strcpy(class_dir,     pP->dataDir);
    strcpy(wtab_dir,      pP->dataDir);
    strcpy(wsca_dir,      pP->dataDir);
    strcpy(rsca_dir,      pP->dataDir);
    strcpy(wmark_dir,     pP->dataDir);
    strcpy(rmark_dir,     pP->dataDir);
    strcpy(wbox_dir,      pP->dataDir);
    strcpy(rbox_dir,      pP->dataDir);
    strcpy(wstate_dir,    pP->dataDir);
    strcpy(rstate_dir,    pP->dataDir);
    strcpy(rholo_dir,     pP->dataDir);
    strcpy(rapex_dir,     pP->dataDir);
    strcpy(wgauss_dir,    pP->gaussDir);
    strcpy(rgauss_dir,    pP->gaussDir);
}

void SetFITSReadDir(char *dir, int type)
{
    if (type == 1) {
        strcpy(r_sin_f_dir,   dir);
        strcpy(r_seq_f_dir,   dir);
        strcpy(r_map_f_dir,   dir);
    } else if (type == 2) {
        strcpy(r_2d_f_dir,    dir);
    } else if (type == 3) {
        strcpy(r_3d_f_dir,    dir);
    }
}

char *GetWriteFITSDir()
{
    return wfits_dir;
}

static int bad_single_file(char *file, char *dir)
{
    char last;
    
    if (!file) return 1;
    
    if (dir) {
        if (strcmp(file, dir) == 0) return 2;
    } else {
        return 3;
    }
    
    last = file[strlen(file)-1];
    if (last == '/') return 1;
    
    return 0;
}

static void fsel_OK(Widget w, fsel_struct *f, XmFileSelectionBoxCallbackStruct *cd)
{
    char *txt = NULL, *dir = NULL;
    string gzFile;
    int read_data = 0;
    DataSetPtr d = NULL;

    void write_PS_file(), send_line();
    void UpdateData(), SetStdView(), toggle_any();
    int read_file(char *, char *, DataSetPtr);
    int open_ants();
    int PostFileListDialog(Widget, fsel_struct *);
    int PostClassScanListDialog(char *, Widget, fsel_struct *);
    int open_binary_table();
    int LoadFITS(), LoadHolo(), LoadAPEXMap();
    int ReadScatterFile(), SaveScatterFile();
    int ReadMarkerFile(), SaveMarkerFile();
    int ReadGaussFile(), SaveGaussFile();
    int ReadBoxesFromFile(), SaveBoxesToFile();
    int ReadPSstruct(), SavePSstruct();
    int ReadState(), SaveState();
    int ViewGaussFile();
    void SaveAndViewSpecTable();
    void SaveAndViewMapTable();
    void SaveAndViewScatterTable();
    int GzippedFile(char *, char *);
    
    if (!cd || cd->reason != XmCR_OK) return;
    
    XmStringGetLtoR(cd->value, XmSTRING_DEFAULT_CHARSET, &txt);
    XmStringGetLtoR(cd->dir, XmSTRING_DEFAULT_CHARSET, &dir);
    
    if (strncmp(f->s, "read", 4) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(data_dir, dir);
            d = new_dataset(get_listlist(), "Native", NULL);
            if (d && !read_file("native", txt, d)) {
                vP->from = vP->to = d;
                strcpy(d->name, ((scanPtr)DATA(d->scanlist))->name);
                read_data = 1;
            } else {
                if (d) DeleteLastDataSet();
            }
        }
    } else if (strncmp(f->s, "seco", 4) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(seco_dir, dir);
            if (!read_file("native", txt, NULL)) {
                read_data = 2;
            }
        }
    } else if (strncmp(f->s, "fits", 4) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(r_sin_f_dir, dir);
            d = new_dataset(get_listlist(), "Single FITS", NULL);
            if (d && !read_file("fits", txt, d)) {
                vP->from = vP->to = d;
                strcpy(d->name, ((scanPtr)DATA(d->scanlist))->name);
                read_data = 1;
            } else {
                if (d) DeleteLastDataSet();
            }
        }
    } else if (strncmp(f->s, "rsca", 4) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(rsca_dir, dir);
            if (ReadScatterFile(txt)) {
                PostErrorDialog(w, "Open scatter file failed.");
            } else {
                read_data = 1;
            }
        }
    } else if (strncmp(f->s, "rbox", 4) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(rbox_dir, dir);
            if (ReadBoxesFromFile(txt, 'r')) {
                PostErrorDialog(w, "Open box file failed.");
            } else {
                read_data = 1;
            }
        }
    } else if (strncmp(f->s, "rmark", 5) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(rmark_dir, dir);
            if (ReadMarkerFile(txt, 'r')) {
                PostErrorDialog(w, "Open marker file failed.");
            } else {
                read_data = 1;
            }
        }
    } else if (strncmp(f->s, "rappmark", 8) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(rmark_dir, dir);
            if (ReadMarkerFile(txt, 'a')) {
                PostErrorDialog(w, "Append marker file failed.");
            } else {
                read_data = 1;
            }
        }
    } else if (strncmp(f->s, "rgauss", 6) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(rgauss_dir, dir);
            if (ReadGaussFile(txt)) {
                read_data = 0;
            } else {
                read_data = 1;
            }
        }
    } else if (strncmp(f->s, "vgauss", 6) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(rgauss_dir, dir);
            if (ViewGaussFile(txt, 0)) {
                read_data = 0;
            } else {
                read_data = 0;
            }
        }
    } else if (strncmp(f->s, "vsgauss", 7) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(rgauss_dir, dir);
            if (ViewGaussFile(txt, 1)) {
                read_data = 0;
            } else {
                read_data = 0;
            }
        }
    } else if (strncmp(f->s, "mapfits", 7) == 0 ||
               strncmp(f->s, "appmapfits", 10) == 0) {
        strcpy(r_map_f_dir, dir);
        PostFileListDialog(w, f);
        read_data = 3;
    } else if (strncmp(f->s, "bintab", 6) == 0 ||
               strncmp(f->s, "seqbintab", 9) == 0) {
        strcpy(r_map_f_dir, dir);
        if (GzippedFile(txt, gzFile)) {
            open_binary_table(w, gzFile, f->s);
            GzippedFile(gzFile, NULL);  /* Clean tmp file */
        } else {
            open_binary_table(w, txt, f->s);
        }
        read_data = 3;
    } else if (strncmp(f->s, "drp", 3) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(drp_dir, dir);
            d = new_dataset(get_listlist(), "Single DRP", NULL);
            if (d && !read_file("drp", txt, d)) {
                vP->from = vP->to = d;
                strcpy(d->name, ((scanPtr)DATA(d->scanlist))->name);
                read_data = 1;
            } else {
                if (d) DeleteLastDataSet();
            }
        }
    } else if (strncmp(f->s, "mapdrp", 6) == 0 ||
               strncmp(f->s, "appmapdrp", 9) == 0) {
        strcpy(drp_dir, dir);
        PostFileListDialog(w, f);
        read_data = 3;
    } else if (strncmp(f->s, "class", 5) == 0) {
         if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(class_dir, dir);
            PostClassScanListDialog(txt, w, f);
            read_data = 3;
	}
    } else if (strncmp(f->s, "2dfits", 6) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(r_2d_f_dir, dir);
            if (LoadFITS(txt, "array"))
                PostErrorDialog(w, "Open FITS array file failed.");
            else
                read_data = 3;
        }
    } else if (strncmp(f->s, "rholo", 5) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(rholo_dir, dir);
            if (LoadHolo(txt, "array"))
                PostErrorDialog(w, "Open holo array file failed.");
            else
                read_data = 3;
        }
    } else if (strncmp(f->s, "rapex", 5) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(rapex_dir, dir);
            if (LoadAPEXMap(txt, "array"))
                PostErrorDialog(w, "Open APEX array file failed.");
            else
                read_data = 3;
        }
    } else if (strncmp(f->s, "3dfits", 6) == 0) {
        if (bad_single_file(txt, dir)) {
            PostErrorDialog(w, "No file selected?");
        } else {
            strcpy(r_3d_f_dir, dir);
            if (LoadFITS(txt, "cube"))
                PostErrorDialog(w, "Open FITS cube file failed.");
            else
                read_data = 1;
        }
    } else if (strncmp(f->s, "seqfits", 7) == 0 ||
               strncmp(f->s, "appseqfits", 10) == 0) {
        strcpy(r_seq_f_dir, dir);
        PostFileListDialog(w, f);
        read_data = 3;
    } else if (strncmp(f->s, "seqdrp", 6) == 0 ||
               strncmp(f->s, "appseqdrp", 9) == 0) {
        strcpy(drp_dir, dir);
        PostFileListDialog(w, f);
        read_data = 3;
    } else if (strncmp(f->s, "ants", 4) == 0) {
        strcpy(ants_dir, dir);
        if (open_ants(w, txt, f->s))
            PostErrorDialog(w, "Open POPS file failed.");
        else
            read_data = 1;
    } else if (strncmp(f->s, "mapants", 7) == 0 ||
               strncmp(f->s, "appmapants", 10) == 0) {
        strcpy(ants_dir, dir);
        if (open_ants(w, txt, f->s))
            PostErrorDialog(w, "Open multiple POPS files failed.");
        else
            read_data = 3;
    } else if (strncmp(f->s, "seqants", 7) == 0 ||
               strncmp(f->s, "appseqants", 10) == 0) {
        strcpy(ants_dir, dir);
        if (open_ants(w, txt, f->s))
            PostErrorDialog(w, "Open a sequence of POPS files failed.");
        else
            read_data = 3;
    } else if (strncmp(f->s, "rsetup", 6)==0) {
        strcpy(ps_dir, dir);
        if (ReadPSstruct(txt))
            PostErrorDialog(w, "Reading PS values failed.");
        else
            read_data = 1;
    } else if (strncmp(f->s, "rstate", 6)==0) {
        strcpy(rstate_dir, dir);
        if (ReadState(txt))
            PostErrorDialog(w, "Loading the graphical state/view failed.");
        else
            read_data = 4;
    } else if (strncmp(f->s, "write", 5) == 0) {
        strcpy(data_dir, dir);
        write_file("native", txt);
    } else if (strncmp(f->s, "fwrite", 6) == 0) {
        strcpy(wfits_dir, dir);
        write_file("fits", txt);
    } else if (strncmp(f->s, "wsca", 4) == 0) {
        strcpy(wsca_dir, dir);
        SaveScatterFile(txt);
    } else if (strncmp(f->s, "wbox", 4) == 0) {
        strcpy(wbox_dir, dir);
        SaveBoxesToFile(txt);
    } else if (strncmp(f->s, "wmark", 5) == 0) {
        strcpy(wmark_dir, dir);
        SaveMarkerFile(txt);
    } else if (strncmp(f->s, "wgauss", 6) == 0) {
        strcpy(wgauss_dir, dir);
        SaveGaussFile(txt, "w");
    } else if (strncmp(f->s, "agauss", 6) == 0) {
        strcpy(wgauss_dir, dir);
        SaveGaussFile(txt, "a");
    } else if (strncmp(f->s, "f2Dwrite", 8) == 0) {
        strcpy(wfits_dir, dir);
        write_file("2dfits", txt);
    } else if (strncmp(f->s, "f3Dwrite", 8) == 0) {
        strcpy(wfits_dir, dir);
        write_file("3dfits", txt);
    } else if (strncmp(f->s, "wbintab", 7) == 0) {
        strcpy(wfits_dir, dir);
        write_file(f->s, txt);
    } else if (strncmp(f->s, "mfwrite", 7) == 0) {
        strcpy(wfits_dir, dir);
        write_all_files("fits", txt, dir);
    } else if (strncmp(f->s, "spetable", 8)==0) {
        strcpy(wtab_dir, dir);
        SaveAndViewSpecTable(txt);
    } else if (strncmp(f->s, "maptable", 8)==0) {
        strcpy(wtab_dir, dir);
        SaveAndViewMapTable(txt);
    } else if (strncmp(f->s, "scatable", 8)==0) {
        strcpy(wtab_dir, dir);
        SaveAndViewScatterTable(txt);
    } else if (strncmp(f->s, "wstate", 6)==0) {
        strcpy(wstate_dir, dir);
        if (SaveState(txt))
            PostErrorDialog(w, "Saving the state failed.");
    } else if (strncmp(f->s, "wsetup", 6)==0) {
        strcpy(ps_dir, dir);
        if (SavePSstruct(txt))
            PostErrorDialog(w, "Saving the PS setup failed.");
    } else if (strcmp(f->s, "ps") == 0 || strcmp(f->s, "cps") == 0 ||
               strcmp(f->s, "vps") == 0 || strcmp(f->s, "vcps") == 0 ||
               strcmp(f->s, "eps") == 0 || strcmp(f->s, "ceps") == 0 ||
               strcmp(f->s, "png") == 0 || strcmp(f->s, "tpng") == 0 ||
               strcmp(f->s, "gif") == 0 || strcmp(f->s, "vgif") == 0) {
        strcpy(ps_dir, dir);
        write_PS_file(txt, f->s);
    }
    
    XtFree(txt);
    XtFree(dir);
    
    if (read_data == 1) {
        UpdateData(SCALE_BOTH, REDRAW);
    } else if (read_data == 2) {
        if (draw.data_sec != 2)
            toggle_any(NULL, "sec", NULL);
    } else if (read_data == 4) {
        SetStdView();
        UpdateData(SCALE_NONE, REDRAW);
    }
}

static void fsel_CANCEL(Widget w, fsel_struct *f,
                        XmFileSelectionBoxCallbackStruct *cd)
{
    if (!f) return;
    if (f->x) {
        if (f->x->busy) {
            f->x->interupt = 1;
            printf("Busy flag (%d)!\n", f->x->busy);
            return;
        }
    }
    if (f->w)
        XtDestroyWidget(f->w);
}

static void SetFileSBInfo(XmString *xstr, char *dir, char *pat, char *ok, char *title)
{
    if (!xstr) return;
    
    xstr[0] = MKSTRING(dir);
    xstr[1] = MKSTRING(pat);
    xstr[2] = MKSTRING(ok);
    xstr[3] = MKSTRING(title);
}

#define FITS_PATTERN   "*.fits"
#define POPS_PATTERN   "DF*"
#define DRP_PATTERN    "*.*"
#define CLASS_PATTERN  "*.apex"
#define SCA_PATTERN    "*.sca"
#define MRK_PATTERN    "*.mrk"
#define BOX_PATTERN    "*.box"
#define GAUSS_PATTERN  "*gauss*"
#define TABLE_PATTERN  "*.table"
#define NATIVE_PATTERN "*.out"
#define STATE_PATTERN  "*.state"
#define HOLO_PATTERN   "*.holo1"
#define APEX_PATTERN   "*.dat"

void get_file(Widget menu_w, char *cmd, XtPointer call_data)
{
    Widget fselbox, w = menu_w;
    XmString item[4];
    Arg wargs[10];
    int n = 0;
    char *hstr = NULL;
    fsel_struct *fp;
    static fsel_struct fs[32];
    
    void help_dialog();
    
    while (!XtIsWMShell(w))
        w = XtParent(w);
    
    if (strncmp(cmd, "fits", 4) == 0) {
        SetFileSBInfo(item, r_sin_f_dir, FITS_PATTERN, "Open", "Open single FITS file"); 
        hstr = OpenSingleFits_Help;
        fp = &fs[0];
    } else if (strncmp(cmd, "2dfits", 6) == 0) {
        SetFileSBInfo(item, r_2d_f_dir, FITS_PATTERN, "Open", "Open a FITS array file"); 
        hstr = Open2DFits_Help;
        fp = &fs[1];
    } else if (strncmp(cmd, "rholo", 5) == 0) {
        SetFileSBInfo(item, rholo_dir, HOLO_PATTERN, "Open", "Open a holo array file"); 
        fp = &fs[29];
    } else if (strncmp(cmd, "rapex", 5) == 0) {
        SetFileSBInfo(item, rapex_dir, APEX_PATTERN, "Open", "Open a APEX array file"); 
        fp = &fs[30];
    } else if (strncmp(cmd, "rsca", 4) == 0) {
        SetFileSBInfo(item, rsca_dir, SCA_PATTERN, "Open", "Open a scatter file"); 
        hstr = OpenScatterFile_Help;
        fp = &fs[2];
    } else if (strncmp(cmd, "rbox", 4) == 0) {
        SetFileSBInfo(item, rbox_dir, BOX_PATTERN, "Open", "Open a box file"); 
        hstr = OpenBoxFile_Help;
        fp = &fs[27];
    } else if (strncmp(cmd, "rstate", 6) == 0) {
        SetFileSBInfo(item, rstate_dir, STATE_PATTERN, "Open", "Open a state file"); 
        hstr = OpenStateFile_Help;
        fp = &fs[28];
    } else if (strncmp(cmd, "rmark", 5) == 0) {
        SetFileSBInfo(item, rmark_dir, MRK_PATTERN, "Open", "Open a marker file"); 
        hstr = OpenMarkerFile_Help;
        fp = &fs[3];
    } else if (strncmp(cmd, "rappmark", 8) == 0) {
        SetFileSBInfo(item, rmark_dir, MRK_PATTERN, "Append", "Append from a marker file"); 
        hstr = OpenMarkerFile_Help;
        fp = &fs[26];
    } else if (strncmp(cmd, "rgauss", 6) == 0) {
        SetFileSBInfo(item, rgauss_dir, GAUSS_PATTERN, "Open", "Open a Gaussian file"); 
        fp = &fs[4];
    } else if (strncmp(cmd, "vgauss", 6) == 0) {
        SetFileSBInfo(item, rgauss_dir, GAUSS_PATTERN, "View", "View a Gaussian file"); 
        fp = &fs[5];
    } else if (strncmp(cmd, "vsgauss", 7) == 0) {
        SetFileSBInfo(item, rgauss_dir, GAUSS_PATTERN, "Sort&View", "View a sorted Gaussian file"); 
        fp = &fs[6];
    } else if (strncmp(cmd, "3dfits", 6) == 0) {
        SetFileSBInfo(item, r_3d_f_dir, FITS_PATTERN, "Open", "Open a FITS cube file"); 
        hstr = Open3DFits_Help;
        fp = &fs[7];
    } else if (strncmp(cmd, "mapfits", 7) == 0) {
        SetFileSBInfo(item, r_map_f_dir, FITS_PATTERN, "Open", "Open multiple FITS files"); 
        hstr = OpenMultipleFits_Help;
        fp = &fs[8];
    } else if (strncmp(cmd, "appmapfits", 10) == 0) {
        SetFileSBInfo(item, r_map_f_dir, FITS_PATTERN, "Append", "Append multiple FITS files"); 
        hstr = AppendMultipleFits_Help;
        fp = &fs[9];
    } else if (strncmp(cmd, "bintab", 6) == 0) {
        SetFileSBInfo(item, r_map_f_dir, FITS_PATTERN, "Open", "Open a binary table as multiple"); 
        hstr = OpenMultipleBinTabFits_Help;
        fp = &fs[10];
    } else if (strncmp(cmd, "seqbintab", 9) == 0) {
        SetFileSBInfo(item, r_map_f_dir, FITS_PATTERN, "Open", "Open a binary table as a sequence"); 
        fp = &fs[11];
    } else if (strncmp(cmd, "seqfits", 7) == 0) {
        SetFileSBInfo(item, r_seq_f_dir, FITS_PATTERN, "Open", "Open a sequence of FITS files"); 
        hstr = OpenSequenceFits_Help;
        fp = &fs[12];
    } else if (strncmp(cmd, "appseqfits", 10) == 0) {
        SetFileSBInfo(item, r_seq_f_dir, FITS_PATTERN, "Append", "Append a sequence of FITS files"); 
        hstr = AppendSequenceFits_Help;
        fp = &fs[13];
    } else if (strncmp(cmd, "ants", 4) == 0) {
        SetFileSBInfo(item, ants_dir, POPS_PATTERN, "Open", "Open a single OSO-POPS file"); 
        fp = &fs[14];
    } else if (strncmp(cmd, "mapants", 7) == 0) {
        SetFileSBInfo(item, ants_dir, POPS_PATTERN, "Open", "Open multiple OSO-POPS files"); 
        fp = &fs[15];
    } else if (strncmp(cmd, "appmapants", 10) == 0) {
        SetFileSBInfo(item, ants_dir, POPS_PATTERN, "Append", "Append multiple OSO-POPS files"); 
        fp = &fs[16];
    } else if (strncmp(cmd, "seqants", 7) == 0) {
        SetFileSBInfo(item, ants_dir, POPS_PATTERN, "Open", "Open a sequence of OSO-POPS files"); 
        fp = &fs[17];
    } else if (strncmp(cmd, "appseqants", 10) == 0) {
        SetFileSBInfo(item, ants_dir, POPS_PATTERN, "Append", "Append a sequence of OSO-POPS files"); 
        fp = &fs[18];
    } else if (strncmp(cmd, "drp", 3) == 0) {
        SetFileSBInfo(item, drp_dir, DRP_PATTERN, "Open", "Open a single DRP file"); 
        hstr = OpenSingleDRP_Help;
        fp = &fs[19];
    } else if (strncmp(cmd, "mapdrp", 6) == 0) {
        SetFileSBInfo(item, drp_dir, DRP_PATTERN, "Open", "Open multiple DRP files"); 
        hstr = OpenMultipleDRP_Help;
        fp = &fs[20];
    } else if (strncmp(cmd, "appmapdrp", 9) == 0) {
        SetFileSBInfo(item, drp_dir, DRP_PATTERN, "Append", "Append multiple DRP files"); 
        hstr = AppendMultipleDRP_Help;
        fp = &fs[21];
    } else if (strncmp(cmd, "seqdrp", 6) == 0) {
        SetFileSBInfo(item, drp_dir, DRP_PATTERN, "Open", "Open a sequence of DRP files"); 
        hstr = OpenSequenceDRP_Help;
        fp = &fs[22];
    } else if (strncmp(cmd, "appseqdrp", 9) == 0) {
        SetFileSBInfo(item, drp_dir, DRP_PATTERN, "Append", "Append a sequence of DRP files"); 
        hstr = AppendSequenceDRP_Help;
        fp = &fs[23];
    } else if (strncmp(cmd, "class", 5) == 0) {
        SetFileSBInfo(item, class_dir, CLASS_PATTERN, "Open", "Open a CLASS file"); 
        hstr = OpenClass_Help;
        fp = &fs[31];
    } else {
        if (strncmp(cmd, "read", 4) == 0) {
            SetFileSBInfo(item, data_dir, "*", "Open", "Open a native file"); 
            fp = &fs[24];
        } else {
            SetFileSBInfo(item, seco_dir, "*", "Open", "Open a secondary native file"); 
            fp = &fs[25];
        }
    }

    XtSetArg(wargs[n], XmNdirectory, item[0]); n++;
    XtSetArg(wargs[n], XmNpattern, item[1]); n++;
    XtSetArg(wargs[n], XmNokLabelString, item[2]); n++;    
    XtSetArg(wargs[n], XmNdialogTitle, item[3]); n++;    

    fselbox = XmCreateFileSelectionDialog(w, cmd, wargs, n);
    
    fp->w = fselbox;
    fp->x = NULL;
    strcpy(fp->s, cmd);

    XtAddCallback(fselbox, XmNokCallback,
                  (XtCallbackProc)fsel_OK, fp);
    XtAddCallback(fselbox, XmNcancelCallback,
                  (XtCallbackProc)fsel_CANCEL, fp);
    if (hstr) XtAddCallback(fselbox, XmNhelpCallback,
                            (XtCallbackProc)help_dialog, hstr);
    
    n = 4;
    while (n > 0) XmStringFree(item[--n]);
    
    ManageDialogCenteredOnPointer(fselbox);
}

int read_file(char *file_type, char *file_name, DataSetPtr dsp)
{
    int i, ierr, class=0;
    static int nspec = 0;
    string buf;
    scanPtr s;
    DataSetPtr d = dsp;

    void  send_line();
    int   get_ants(), get_fits(), get_drp();
    int   get_bintab();
    int   get_class(char *, FDATA *);
    int   filter_active();
    int   allow_fd(FDATA *);
    
    if (d) {
        nspec = 0;
        vP->to = d;
    } else {
        if (!vP->to) return -1;
        d = vP->to;
        nspec++;
    }
    
    fd.bl.norder = -1;
    fd.nm = 0;
    fd.nb = 0;
    fd.nr = 0;
    fd.nmark = 0;
    fd.xoff = 0.0;
    fd.yoff = 0.0;
    strcpy(fd.molecule, "");
#ifdef DEBUG 
    printf("read_file: Reading spectrum...  vP->mode=%d\n", vP->mode);
#endif
    if (strncmp(file_type, "native", 6)==0) {
        if (get_native(file_name, &fd) != 0) {
            sprintf(buf, "Error occured when reading %s (native).",
                    file_name);
            PostErrorDialog(NULL, buf);
            return 2;
        }
    } else if (strncmp(file_type, "fits", 4)==0 ||
               strncmp(file_type, "seqfits", 7)==0) {
        if (get_fits(file_name, &fd) != 0) {
            sprintf(buf, "Error occured when reading %s (FITS).", file_name);
            PostErrorDialog(NULL, buf);
            return 2;
        }
    } else if (strncmp(file_type, "drp", 3)==0 ||
               strncmp(file_type, "seqdrp", 6)==0) {
        if (get_drp(file_name, &fd) != 0) {
            sprintf(buf, "Error occured when reading %s (DRP).", file_name);
            PostErrorDialog(NULL, buf);
            return 2;
        }
    } else if (strncmp(file_type, "ants", 4)==0 ||
               strncmp(file_type, "seqants", 7)==0) {
        if (get_ants(&fd) != 0) {
            sprintf(buf, "Error occured when reading %s (POPS).", file_name);
            PostErrorDialog(NULL, buf);
            return 2;
        }
    } else if (strncmp(file_type, "bintab", 6)==0 ||
               strncmp(file_type, "seqbintab", 9)==0) {
        if (get_bintab(&fd) != 0) {
            sprintf(buf, "Error occured when reading %s (BINTAB).", file_name);
            PostErrorDialog(NULL, buf);
            return 2;
        }
    } else if (strncmp(file_type, "class", 5)==0 ||
               strncmp(file_type, "seqclass", 8)==0) {
        if ((ierr = get_class(file_name, &fd)) != 0) {
            sprintf(buf, "Error %d occured when loading %s (CLASS).", ierr, file_name);
            PostErrorDialog(NULL, buf);
#ifdef DEBUG
	    printf("ierr=%d\n", ierr);
#endif
            return 2;
	}
	class = 1;
    }
        
#ifdef DEBUG 
    printf("read_file: Spectrum read...  vP->mode=%d nochan=%d\n", vP->mode, fd.n);
#endif

    if (class && filter_active() && !allow_fd(&fd)) {
#ifdef DEBUG
        printf("Scan %d not read because not passing the filter.\n", fd.sno);
#endif
        sprintf(buf, "Scan %d skipped because not matching the filter settings.",
	        fd.sno);
	send_line(buf);
        return 3;
    }

    s = new_scan(d, fd.n);
    if (!s) {
        sprintf(buf, "Warning! Out of memory at %d spectra.",
                count_scans(d)+1);
        PostErrorDialog(NULL, buf);
        return 1;
    }
    vP->s = s;

    s->freq0 = fd.f0;
    s->vel0 = fd.v0;
    s->freqn = fd.fn;
    s->freqres = fd.fres/1000.0;
    s->velres = fd.vres;
    s->tsys = fd.tsys;
    s->tau = fd.tau;
    s->int_time = fd.int_time;
    s->vlsr = fd.vlsr;
    s->date = fd.date;
    s->LST  = fd.LST;
    s->scan_no = fd.sno;
    s->subscan = fd.subno;
    s->beameff = fd.beameff;
    s->polarization = fd.pol;
    s->firstIF = fd.firstIF;
    s->lofreq =  fd.lofreq;
    s->saved = 1;
    if (s->velres == 0.0) {
        s->velres = -SPEEDOFLIGHT*fd.fres/1000.0/((fd.f0 + fd.fn)/2.0);
    }
#ifdef DEBUG 
    printf("read_file: nochan=%d  vP->mode=%d\n", fd.n, vP->mode);
#endif
    for (i=0; i<s->nChan; i++) {
        s->d[i] = fd.d[i];
        s->e[i] = fd.e[i];
    }
    s->coordType = fd.coordType;
    s->epoch   = fd.epoch;
    s->equinox = fd.equinox;
    s->x0 = fd.x0;
    s->y0 = fd.y0;
    s->tx = fd.xoff;
    s->ty = fd.yoff;
    if (strncmp(file_type, "seq", 3) == 0) {
        if (vP->Nx > 0) {
            s->xoffset = -(double)(nspec % vP->Nx);
            s->yoffset = -(double)(nspec / vP->Nx);
        } else {
            s->xoffset = -(double)(nspec % SEQ_NSPEC);
            s->yoffset = -(double)(nspec / SEQ_NSPEC);
        }
        if (nspec == 0) {
            d->sequence = 1;
            d->gridded = 1;
            d->dx = -1.0;
            d->dy = 1.0;
            d->posAngle = 0.0;
        }
    } else {
        s->xoffset = fd.xoff;
        s->yoffset = fd.yoff;
        if (nspec == 0) {
            d->sequence = 0;
            d->gridded = 0;
            d->dx = 0.0;
            d->dy = 0.0;
            d->posAngle = RADTODEG * fd.posang;
        }
    }
    s->aoffset = fd.aoff;
    s->eoffset = fd.eoff;
    strcpy(s->name,     fd.sname);
    strcpy(s->molecule, fd.molecule);
    s->az = fd.az;
    s->el = fd.el;
    s->b = fd.b;
    s->fft = 0;
    s->nfft = 0;
    s->doublet = 0;
    s->scanType = fd.scanType;
    s->tair = fd.TAir;
    s->pair = fd.PAir;
    s->rair = fd.RAir;
  
#ifdef DEBUG 
    printf("read_file: sname=%s\n",fd.sname);
    printf("read_file: Position=(%.1lf,%.1lf)\n",fd.xoff,fd.yoff);
    printf("read_file: vP->mode=%d\n", vP->mode);
#endif
    if (nspec >= 1) return 0;

    if (strlen(fd.sname)) strcpy(vP->t_label, fd.sname);
    if (fd.bl.norder >= 0) {
        bl = fd.bl;
        for (i=0; i<=fd.bl.norder; i++)
            s->coeffs[i] = fd.coeffs[i];
    }
#ifdef DEBUG 
    printf("read_file: nbox=%d nreg=%d nmod=%d\n",fd.nb,fd.nr,fd.nm);
    printf("read_file: vP->mode=%d\n", vP->mode);
#endif
    if (fd.nm > 0) {
        nmod = fd.nm;
        if (nmod > MAXMOD) nmod = 0;
        for (i=0; i<nmod; i++) mods[i] = fd.mods[i];
    } else {
        nmod = 0;
    }
    if (fd.nb > 0) {
        draw.boxes = 1;
        nbox = fd.nb;
        if (nbox > MAXBOX) nbox = 0;
        for (i=0; i<nbox; i++) boxar[i] = fd.boxar[i];
    }
    if (fd.nr > 0) {
        draw.boxes = 1;
        nreg = fd.nr;
        if (nreg > MAXBOX) nreg = 0;
        for (i=0; i<nreg; i++) regs[i] = fd.regs[i];
    }
    if (fd.nmark > 0) {
        nmark = fd.nmark;
        draw.markers = 1;
        if (nmark > MAXMARK) nmark = 0;
        for (i=0; i<nmark; i++) marks[i] = fd.marks[i];
    }
  
#ifdef DEBUG 
    printf("read_file: Leaving...\n");
    printf("read_file: vP->mode=%d\n", vP->mode);
#endif
    return 0;
}

static int obtain_file_data(char *str, FDATA *d)
{
    char c, key[CARDLEN], arg[MAXARGS][CARDLEN], buf[150];
    char rest1[CARDLEN], rest2[CARDLEN];
    int i, j=0, k=0, mm=0, m=0, argcnt=0;
	
    void send_line();
	int CheckDataSize(int);

    for (i=0; i<(int)strlen(str); i++) {
        c = str[i];
        if (k >= 1) rest1[m++] = c;
        if (k >= 2) rest2[mm++] = c;
        if (c != ' ') {
            if (k >= MAXARGS) {
                sprintf(buf, "ERROR: Too many arguments (%s)\n", str);
                send_line(buf);
                return 1;
            }
            if (j >= CARDLEN-1) {
                sprintf(buf, "ERROR: Too long argument (%s)\n", str);
                send_line(buf);
                return 1;
            }
            arg[k][j] = c;
            if (k == 0) key[j] = c;
            j++;
            arg[k][j] = '\0';
            if (k == 0) key[j] = '\0';
            argcnt = 0;
        } else {
            if (argcnt == 0) {
                k++;
                argcnt = 1;
                j=0;
            }
        }
    }
    rest1[m++]  = '\0';
    rest2[mm++] = '\0';

    if (strncmp(key, "FREQ0", 5) == 0) {
        sscanf(arg[1], "%lf", &(d->f0));
    } else if (strncmp(key, "FREQN", 5) == 0) {
        sscanf(arg[1], "%lf", &(d->fn));
    } else if (strncmp(key, "FRES",  4) == 0) {
        sscanf(arg[1], "%lf", &(d->fres));
    } else if (strncmp(key, "VEL0",  4) == 0) {
        sscanf(arg[1], "%lf", &(d->v0));
    } else if (strncmp(key, "VRES",  4) == 0) {
        sscanf(arg[1], "%lf", &(d->vres));
    } else if (strncmp(key, "NCHAN", 5) == 0) {
        sscanf(arg[1], "%d", &(d->n));
		if (CheckDataSize(d->n) < d->n) {
            sprintf(buf, "Couldn't allocate enough memory, NChan=%d.\n", d->n);
            send_line(buf);
			return 1;
		}
    } else if (strncmp(key, "DATA:", 5) == 0) {
        sscanf(arg[1], "%d", &i);
        sscanf(arg[2], "%lf", &(d->d[i]));
        sscanf(arg[3], "%lf", &(d->e[i]));
        d->e[i] /= 1000.0;
    } else if (strncmp(key, "NORD",  4) == 0) {
        sscanf(arg[1], "%d", &(d->bl.norder));
    } else if (strncmp(key, "PTYPE",  5) == 0) {
        sscanf(arg[1], "%d", &(d->bl.pol_type));
    } else if (strncmp(key, "POLY:", 5) == 0) {
        sscanf(arg[1], "%d", &i);
        sscanf(arg[2], "%lf", &(d->coeffs[i]));
    } else if (strncmp(key, "NBOX",  4) == 0) {
        sscanf(arg[1], "%d", &(d->nb));
    } else if (strncmp(key, "BOX:",  4) == 0) {
        sscanf(arg[1], "%d", &i);
        sscanf(arg[2], "%d", &(d->boxar[i].begin));
        sscanf(arg[3], "%d", &(d->boxar[i].end));
    } else if (strncmp(key, "NMOM",  4) == 0) {
        sscanf(arg[1], "%d", &(d->nr));
    } else if (strncmp(key, "MBOX:",  5) == 0) {
        sscanf(arg[1], "%d", &i);
        sscanf(arg[2], "%d", &(d->regs[i].begin));
        sscanf(arg[3], "%d", &(d->regs[i].end));
    } else if (strncmp(key, "NMOD",  4) == 0) {
        sscanf(arg[1], "%d", &(d->nm));
    } else if (strncmp(key, "MOD:",  4) == 0) {
        sscanf(arg[1], "%d", &i);
        sscanf(arg[2], "%d", &(d->mods[i].chan));
        sscanf(arg[3], "%lf", &(d->mods[i].new));
        sscanf(arg[4], "%lf", &(d->mods[i].old));
    } else if (strncmp(key, "NMARK",  5) == 0) {
        sscanf(arg[1], "%d", &(d->nmark));
    } else if (strncmp(key, "MARK1",  5) == 0) {
        sscanf(arg[1], "%d", &i);
        sscanf(arg[2], "%lf", &(d->marks[i].x));
        sscanf(arg[3], "%lf", &(d->marks[i].y));
        sscanf(arg[4], "%lf", &(d->marks[i].align));
        if (k == 5)
            sscanf(arg[5], "%lf", &(d->marks[i].angle));
    } else if (strncmp(key, "MARK2",  5) == 0) {
        sscanf(arg[1], "%d", &i);
        sscanf(arg[2], "%lf", &(d->marks[i].ylength));
        sscanf(arg[3], "%d", &(d->marks[i].tagged));
    } else if (strncmp(key, "MARK3",  5) == 0) {
        sscanf(arg[1], "%d", &i);
        if (strcmp(rest2, "line")==0) {
            d->marks[i].type = MARK_TYPE_LINE;
        } else if (strcmp(rest2, "arrow")==0) {
            d->marks[i].type = MARK_TYPE_ARROW;
        } else if (strcmp(rest2, "square")==0) {
            d->marks[i].type = MARK_TYPE_SQUARE;
        } else if (strcmp(rest2, "circle")==0) {
            d->marks[i].type = MARK_TYPE_CIRCLE;
        } else {
            d->marks[i].type = MARK_TYPE_NONE;
        }
    } else if (strncmp(key, "MARK4",  5) == 0) {
        sscanf(arg[1], "%d", &i);
        if (strcmp(rest2, "up")==0) {
            d->marks[i].dir = MARK_DIR_UP;
        } else if (strcmp(rest2, "down")==0) {
            d->marks[i].dir = MARK_DIR_DOWN;
        } else if (strcmp(rest2, "left")==0) {
            d->marks[i].dir = MARK_DIR_LEFT;
        } else {
            d->marks[i].dir = MARK_DIR_RIGHT;
        }
    } else if (strncmp(key, "MARK5",  5) == 0) {
        sscanf(arg[1], "%d", &i);
        strcpy(d->marks[i].label, rest2);
    } else if (strncmp(key, "SNAME",  5) == 0) {
        strcpy(d->sname, rest1);
    } else if (strncmp(key, "MNAME",  5) == 0) {
        strcpy(d->molecule, rest1);
    } else {
        sprintf(buf, "WARNING: Unknown keyword %s in %s\n", key, str);
        send_line(buf);
        return 1;
    }
    return 0;
}

static int get_native(char *fname, FDATA *d)
{
    int   err;
    string foo;
    char  buf[MAXBUFSIZE];
    FILE *fp, *fopen();
    void  send_line();

    if ((fp = fopen(fname, "r")) == NULL) {
        sprintf(foo, "ERROR! Unable to open %s for reading.\n", fname);
        send_line(foo);
        return -1;
    }
    sprintf(foo, "File %s is opened for reading.\n", fname);
    send_line(foo);
    d->bl.norder = -1;
    while ((fgets(buf, MAXBUFSIZE, fp)) != NULL) {
        buf[strlen(buf)-1] = '\0';
        if (buf[0] == '%' || buf[0] == '!' || buf[0] == '#') continue;
        err = obtain_file_data(buf, d);
        if (err != 0) {
            sprintf(foo, "Error occured when reading data from %s\n", fname);
            send_line(foo);
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    sprintf(foo, "File %s is closed.\n", fname);
    send_line(foo);
    d->vlsr = d->v0 + (double)(d->n/2) * d->vres;
    return 0;
}

static void put_native(char *file_name, scanPtr s)
{
    string buf;
    int i;
    FILE *fp, *fopen();
    void send_line();

    if (!s) return;
    
    if ((fp = fopen(file_name, "w")) == NULL) {
        sprintf(buf, "ERROR: Unable to open %s for writing.\n", file_name);
        send_line(buf);
        return;
    }
    sprintf(buf, "File %s is opened for writing.\n", file_name);
    send_line(buf);
    fprintf(fp, "SNAME %s\n", s->name);
    fprintf(fp, "FREQ0 %e\n", s->freq0);
    fprintf(fp, "FREQN %e\n", s->freqn);
    fprintf(fp, "FRES  %e\n", s->freqres*1000.0);
    fprintf(fp, "VEL0  %e\n", s->vel0);
    fprintf(fp, "VRES  %e\n", s->velres);
    fprintf(fp, "NCHAN %d\n", s->nChan);
    for (i=0; i<s->nChan; i++)
        fprintf(fp,"DATA: %5d %e %e\n", i, s->d[i], 1000.0*s->e[i]);
    fprintf(fp, "NORD %d\n", bl.norder);
    fprintf(fp, "PTYPE %d\n", bl.pol_type);
    for (i=0; i<=bl.norder; i++)
        fprintf(fp,"POLY: %d %e\n", i, s->coeffs[i]);
    fprintf(fp, "NMOD %d\n", nmod);
    for (i=0; i<nmod; i++)
        fprintf(fp,"MOD: %d %d %e %e\n", i, mods[i].chan,
                mods[i].new, mods[i].old);
    fprintf(fp, "NBOX %d\n", nbox);
    for (i=0; i<nbox; i++)
        fprintf(fp,"BOX: %d %d %d\n", i, boxar[i].begin, boxar[i].end);
    fprintf(fp, "NMOM %d\n", nreg);
    for (i=0; i<nreg; i++)
        fprintf(fp,"MBOX: %d %d %d\n", i, regs[i].begin, regs[i].end);
    fprintf(fp, "NMARK %d\n", nmark);
    for (i=0; i<nmark; i++) {
        fprintf(fp,"MARK1: %d %e %e %e %e\n", i,
                marks[i].x, marks[i].y, marks[i].align, marks[i].angle);
        fprintf(fp,"MARK2 %d %f %d\n", i, marks[i].ylength, marks[i].tagged);
        switch (marks[i].type) {
            case MARK_TYPE_ARROW:
                fprintf(fp,"MARK3 %d arrow\n", i);
                break;
            case MARK_TYPE_LINE:
                fprintf(fp,"MARK3 %d line\n", i);
                break;
            case MARK_TYPE_SQUARE:
                fprintf(fp,"MARK3 %d square\n", i);
                break;
            case MARK_TYPE_CIRCLE:
                fprintf(fp,"MARK3 %d circle\n", i);
                break;
            case MARK_TYPE_NONE:
                fprintf(fp,"MARK3 %d none\n", i);
                break;
        }
        switch (marks[i].dir) {
            case MARK_DIR_DOWN:
                fprintf(fp,"MARK4 %d down\n", i);
                break;
            case MARK_DIR_UP:
                fprintf(fp,"MARK4 %d up\n", i);
                break;
            case MARK_DIR_LEFT:
                fprintf(fp,"MARK4 %d left\n", i);
                break;
            case MARK_DIR_RIGHT:
                fprintf(fp,"MARK4 %d right\n", i);
                break;
        }
        fprintf(fp,"MARK5 %d %s\n", i, marks[i].label);
    }
    fclose(fp);
    sprintf(buf, "File %s is closed.\n", file_name);
    send_line(buf);
}

char *GetPolStr(int pol)
{
    static string buf;
    
    if (pol == POL_UNKNOWN) {
        strcpy(buf, "?");
    } else if (pol == POL_BOTH) {
        strcpy(buf, "BOTH");
    } else if (pol == POL_LCP) {
        strcpy(buf, "LCP");
    } else if (pol == POL_RCP) {
        strcpy(buf, "RCP");
    } else {
        strcpy(buf, "");
    }    
    
    return buf;
}

static void LoadScanInfo(FDATA *fd, scanPtr s)
{
    int i;
	
	int CheckDataSize(int);
    
    if (!fd || !s) return;
    
    fd->f0 = s->freq0;
    fd->v0 = s->vel0;
    fd->fn = s->freqn;
    fd->fres = 1000.0*s->freqres;
    fd->vres = s->velres;
    fd->n = s->nChan;
    fd->tsys = s->tsys;
    fd->tau = s->tau;
    fd->int_time = s->int_time;
    fd->vlsr = s->vlsr;
    fd->firstIF = s->firstIF;
    fd->lofreq = s->lofreq;
    if (CheckDataSize(s->nChan) < s->nChan) {
    	return;
    }
    for (i=0; i<s->nChan; i++) {
        fd->d[i] = s->d[i];
        fd->e[i] = s->e[i];
    }
    if (vP->from->sequence) {
        fd->xoff = s->tx;
        fd->yoff = s->ty;
    } else {
        fd->xoff = s->xoffset;
        fd->yoff = s->yoffset;
    }
    fd->aoff = s->aoffset;
    fd->eoff = s->eoffset;
    fd->epoch = s->epoch;
    fd->equinox = s->equinox;
    fd->x0 = s->x0;
    fd->y0 = s->y0;
    strcpy(fd->sname, s->name);
    strcpy(fd->molecule, s->molecule);
    fd->sno  = s->scan_no;
    fd->subno = s->subscan;
    fd->scanType = s->scanType;
    fd->date = s->date;
    fd->az   = s->az;
    fd->el   = s->el;
    fd->b    = s->b;
    fd->beameff = s->beameff;
    fd->pol = s->polarization;
}

static void write_file(char *file_type, char *file_name)
{
    string buf;
    list curr = NULL;
    
    void send_line(), LoadMapInfo(), SetMapInfo();
    int put_fits(), save_binary_table();
      
    if (strncmp(file_type, "native", 6)==0) {
        LoadScanInfo(&fd, vP->s);
        put_native(file_name, vP->s);
        if (vP->s) vP->s->saved = 1;
    } else if (strncmp(file_type, "fits", 4)==0) {
        LoadScanInfo(&fd, vP->s);
        put_fits(file_name, &fd, FITS_VECTOR);
        if (vP->s) vP->s->saved = 1;
    } else if (strncmp(file_type, "wbintab_Odin", 12)==0) {
        save_binary_table(file_name, vP->from, 1);
        while ( (curr=scan_iterator(curr, vP->from)) )
            ((scanPtr)DATA(curr))->saved = 1;
    } else if (strncmp(file_type, "2dfits", 6)==0) {
        LoadMapInfo(&fd, vP->m);
        put_fits(file_name, &fd, FITS_ARRAY);
        if (vP->m) vP->m->saved = 1;
    } else if (strncmp(file_type, "3dfits", 6)==0) {
        LoadScanInfo(&fd, vP->s);
        SetMapInfo(&fd);
        put_fits(file_name, &fd, FITS_CUBE);
        while ( (curr=scan_iterator(curr, vP->from)) )
            ((scanPtr)DATA(curr))->saved = 1;
        if (vP->m) vP->m->saved = 1;
    } else {
        sprintf(buf, "Unknown file type (%s).", file_type);
        PostErrorDialog(NULL, buf);
    }
}

static void write_all_files(char *file_type, char *file, char *dir)
{
    int n, dig;
    string fname, tmp, tag, suffix, format;
    list curr = NULL;
    scanPtr s = vP->s;
    
    if (count_scans(vP->from) <= 0) {
        PostWarningDialog(NULL, "There are no spectra to be saved.");
        return;
    }
    
    if (strcmp(file_type, "fits")==0) {
        strcpy(suffix, ".fits");
    } else {
        strcpy(suffix, ".out");
    }
    
    dig = (int)(1.0 + log10((double)count_scans(vP->from)));
    sprintf(format, "%s%dd", "%s_%0", dig);
    
    n = 0;
    while ( (curr = scan_iterator(curr, vP->from)) ) {
        vP->s = (scanPtr)DATA(curr);
        sprintf(tmp, format, vP->s->name, n+1);
        nospace_strcpy(tag, tmp);
        strcpy(fname, dir);
        strcat(fname, tag);
        strcat(fname, suffix);
        write_file(file_type, fname);
        n++;
    }
    
    vP->s = s;
}

void set_file(Widget wid, char *cmd, XtPointer call_data)
{
    Widget fselbox, w = wid;
    XmString xmstr[4];
    Arg wargs[5];
    int n;
    fsel_struct *fp;
    static fsel_struct fs[15];
    
    while (!XtIsWMShell(w))
        w = XtParent(w);

    if (strncmp(cmd, "fwrite", 6)==0) {
        SetFileSBInfo(xmstr, wfits_dir, FITS_PATTERN, "Save", "Save as a FITS file");
        fp = &fs[0];
    } else if (strncmp(cmd, "wsca", 4)==0) {
        SetFileSBInfo(xmstr, wsca_dir, SCA_PATTERN, "Save", "Save as a scatter file");
        fp = &fs[1];
    } else if (strncmp(cmd, "wbox", 4)==0) {
        SetFileSBInfo(xmstr, wbox_dir, BOX_PATTERN, "Save", "Save as a box file");
        fp = &fs[12];
    } else if (strncmp(cmd, "wstate", 6)==0) {
        SetFileSBInfo(xmstr, wstate_dir, STATE_PATTERN, "Save", "Save state as a file");
        fp = &fs[13];
    } else if (strncmp(cmd, "wmark", 5)==0) {
        SetFileSBInfo(xmstr, wmark_dir, MRK_PATTERN, "Save", "Save as a marker file");
        fp = &fs[2];
    } else if (strncmp(cmd, "wgauss", 6)==0) {
        SetFileSBInfo(xmstr, wgauss_dir, GAUSS_PATTERN, "Save", "Save Gaussians to file");
        fp = &fs[3];
    } else if (strncmp(cmd, "agauss", 6)==0) {
        SetFileSBInfo(xmstr, wgauss_dir, GAUSS_PATTERN, "Append", "Append Gaussians to file");
        fp = &fs[4];
    } else if (strncmp(cmd, "f2Dwrite", 8)==0) {
        SetFileSBInfo(xmstr, wfits_dir, FITS_PATTERN, "Save", "Save map as FITS array");
        fp = &fs[5];
    } else if (strncmp(cmd, "f3Dwrite", 8)==0) {
        SetFileSBInfo(xmstr, wfits_dir, FITS_PATTERN, "Save", "Save all scans as FITS cube");
        fp = &fs[6];
    } else if (strncmp(cmd, "mfwrite", 7)==0) {
        SetFileSBInfo(xmstr, wfits_dir, FITS_PATTERN, "Save", "Save all scans as FITS files");
        fp = &fs[7];
    } else if (strncmp(cmd, "wbintab", 7)==0) {
        SetFileSBInfo(xmstr, wfits_dir, FITS_PATTERN, "Save", "Save scans in dataset as FITS binary table");
        fp = &fs[14];
    } else if (strncmp(cmd, "spetable", 8)==0) {
        SetFileSBInfo(xmstr, wtab_dir, TABLE_PATTERN, "Save", "Save an ASCII table of current scans");
        fp = &fs[8];
    } else if (strncmp(cmd, "maptable", 8)==0) {
        SetFileSBInfo(xmstr, wtab_dir, TABLE_PATTERN, "Save", "Save an ASCII table of current map");
        fp = &fs[9];
    } else if (strncmp(cmd, "scatable", 8)==0) {
        SetFileSBInfo(xmstr, wtab_dir, TABLE_PATTERN, "Save", "Save an ASCII table of current scatter plot");
        fp = &fs[10];
    } else if (strncmp(cmd, "write", 5)==0) {
        SetFileSBInfo(xmstr, data_dir, NATIVE_PATTERN, "Save", "Save scan as a native file");
        fp = &fs[11];
    } else {
        return;
    }

    n = 0;
    XtSetArg(wargs[n], XmNdirectory, xmstr[0]); n++;
    XtSetArg(wargs[n], XmNpattern, xmstr[1]); n++;
    XtSetArg(wargs[n], XmNokLabelString, xmstr[2]); n++;    
    XtSetArg(wargs[n], XmNdialogTitle, xmstr[3]); n++;    
    
    fselbox = XmCreateFileSelectionDialog(w, cmd, wargs, n);
    
    fp->w = fselbox;
    fp->x = NULL;
    strcpy(fp->s, cmd);

    XtAddCallback(fselbox, XmNokCallback,
                  (XtCallbackProc)fsel_OK, fp);
    XtAddCallback(fselbox, XmNcancelCallback,
                  (XtCallbackProc)fsel_CANCEL, fp);
    
    n = 4;
    while (n > 0) XmStringFree(xmstr[--n]);
    
    ManageDialogCenteredOnPointer(fselbox);
}

static int pending;

typedef struct {
    char **f;
    char **d;
} charchar;

static void browse_file_OK(Widget w, char **str,
                           XmFileSelectionBoxCallbackStruct *cb)
{
    pending = 0;
    XmStringGetLtoR(cb->value, XmSTRING_DEFAULT_CHARSET, str);
}

static void browse_dir_OK(Widget w, char **str,
                          XmFileSelectionBoxCallbackStruct *cb)
{
    pending = 0;
    XmStringGetLtoR(cb->dir, XmSTRING_DEFAULT_CHARSET, str);
}

static void browse_CANCEL(Widget w, char **str,
                          XmFileSelectionBoxCallbackStruct *cb)
{
    pending = 0;
    *str = NULL;
}

/* Starts up a browser to obtain a file, after use the pointer
   char *txt, which points to the selected file, must be free'd by
   XtFree(txt). If no file was selected txt points to NULL. */
void BrowseFile(Widget wid, int type, char **txt)
{
    Widget fselbox, w = wid;
    XmString xmstr[3];
    Arg wargs[5];
    int n;
    
    while (!XtIsWMShell(w))
        w = XtParent(w);

    xmstr[0] = MKSTRING("./*");
    xmstr[1] = MKSTRING("Ok");
    xmstr[2] = MKSTRING("<Empty>");
    if (type == BROWSE_FILE)
        xmstr[2] = MKSTRING("Browse file");
    else if (type == BROWSE_DIR)
        xmstr[2] = MKSTRING("Browse directory");
    
    n = 0;
    XtSetArg(wargs[n], XmNdirMask, xmstr[n]); n++;
    XtSetArg(wargs[n], XmNokLabelString, xmstr[n]); n++;    
    XtSetArg(wargs[n], XmNdialogTitle, xmstr[n]); n++;
    
    pending = 1;
     
    n = 3;
    while (n > 0) XmStringFree(xmstr[--n]);
    
    fselbox = XmCreateFileSelectionDialog(w, "fsbox", wargs, n);

    if (type == BROWSE_FILE)
        XtAddCallback(fselbox, XmNokCallback,
                      (XtCallbackProc)browse_file_OK, txt);
    else if (type == BROWSE_DIR)
        XtAddCallback(fselbox, XmNokCallback,
                      (XtCallbackProc)browse_dir_OK, txt);
    XtAddCallback(fselbox, XmNcancelCallback,
                  (XtCallbackProc)browse_CANCEL, txt);
    
    ManageDialogCenteredOnPointer(fselbox);
    
    while (pending)
        XtAppProcessEvent(XtWidgetToApplicationContext(fselbox), XtIMAll);
        
    if (fselbox) XtDestroyWidget(fselbox);
}

void set_PS_file(Widget wid, char *cmd, XtPointer call_data)
{
    Widget fselbox, w = wid;
    XmString xmstr[3];
    Arg wargs[10];
    int n;
    string buf;
    static fsel_struct fs;
    
    while (!XtIsWMShell(w))
        w = XtParent(w);
    
    strcpy(buf, ps_dir);
    if (strcmp(cmd, "wsetup")==0) {
        strcat(buf, "/*.gra");
        xmstr[0] = MKSTRING(buf);
        xmstr[1] = MKSTRING("Save");
        xmstr[2] = MKSTRING("Save PostScript setup");
    } else if (strcmp(cmd, "rsetup")==0) {
        strcat(buf, "/*.gra");
        xmstr[0] = MKSTRING(buf);
        xmstr[1] = MKSTRING("Read");
        xmstr[2] = MKSTRING("Read PostScript setup");
    } else if (strcmp(cmd, "gif")==0 || strcmp(cmd, "vgif")==0) {
        strcat(buf, "/*.gif");
        xmstr[0] = MKSTRING(buf);
        xmstr[1] = MKSTRING("Save");
        xmstr[2] = MKSTRING("Save as GIF file");
    } else if (strcmp(cmd, "png")==0 || strcmp(cmd, "tpng")==0) {
        strcat(buf, "/*.png");
        xmstr[0] = MKSTRING(buf);
        xmstr[1] = MKSTRING("Save");
        xmstr[2] = MKSTRING("Save as PNG file");
    } else {
        strcat(buf, "/*.*ps");
        xmstr[0] = MKSTRING(buf);
        xmstr[1] = MKSTRING("Save");
        xmstr[2] = MKSTRING("Save as PostScript file");
    }

    n = 0;
    XtSetArg(wargs[n], XmNdirMask, xmstr[n]); n++;
    XtSetArg(wargs[n], XmNokLabelString, xmstr[n]); n++;    
    XtSetArg(wargs[n], XmNdialogTitle, xmstr[n]); n++;    
    
    fselbox = XmCreateFileSelectionDialog(w, cmd, wargs, n);
    
    fs.w = fselbox;
    fs.x = NULL;
    strcpy(fs.s, cmd);

    XtAddCallback(fselbox, XmNokCallback,
                  (XtCallbackProc)fsel_OK, &fs);
    XtAddCallback(fselbox, XmNcancelCallback,
                  (XtCallbackProc)fsel_CANCEL, &fs);
    
    n = 3;
    while (n > 0) XmStringFree(xmstr[--n]);
    
    ManageDialogCenteredOnPointer(fselbox);
}

int CheckCoordType()
{
    list curr=NULL;
    scanPtr s, first = (scanPtr)DATA(vP->from->scanlist);
    
    if (count_scans(vP->from) <= 0) return 0;
    
    if (vP->from->sequence) return 0;
    
    while ( (curr = scan_iterator(curr, vP->from)) ) {
        s = (scanPtr)DATA(curr);
        if (s->coordType != first->coordType) return 1;
    }
    
    return 0;
}

static void set_minmax(scanPtr S)
{
    int i;
    double s, r;
    
    if (!S) return;
    
    S->s_min = S->s_max = S->d[0];
    S->r_min = S->r_max = S->e[0];
    
    for (i=1; i<S->nChan; i++) {
        s = S->d[i];
        if (s < S->s_min) S->s_min = s;
        if (s > S->s_max) S->s_max = s;
        r = S->e[i];
        if (r < S->r_min) S->r_min = r;
        if (r > S->r_max) S->r_max = r;
    }
}

void UpdateData(int scaling, int redraw)
{
    list curr=NULL;
    
    void update_map_data(), set_gauss_data(), update_bl_data();
    void SetDefWindow(), draw_main(), update_mom_data();
    void UpdateHeaderInfo();
 
    if (count_scans(vP->from) > 0) {
        set_gauss_data(vP->s);

        while ( (curr = scan_iterator(curr, vP->from)) )
            set_minmax((scanPtr)DATA(curr));

        if (vP->m && count_scans(vP->from) > 1) {
            /* obtain_map_info(NULL, "map", NULL); */
            update_map_data();
        } else {
            curr = NULL;
            while ( (curr = scan_iterator(curr, vP->from)) ) {
                update_bl_data((scanPtr)DATA(curr));
                update_mom_data((scanPtr)DATA(curr));
            }
        }
    }
    
    if (scaling != SCALE_NONE)
        SetDefWindow(scaling);
        
    if (redraw == REDRAW)
        draw_main();
        
    UpdateHeaderInfo();
}

static void nospace_strcpy(char *s1, char *s2)
{
    while (*s2 != '\0') {
        if (*s2 != ' ') {
            *s1 = *s2;
            s1++;
        }
        s2++;
    }
    *s1 = '\0';
}

void keyword_strcpy(char *s1, char *s2)
{
    char *last = NULL;
    
    while (*s2 != '\0' && *s2 != '\n') {
        *s1 = *s2;
        if (*s2 == ' ') {
            last = s1;
        } else {
            if (last) last = NULL;
        }
        s1++;
        s2++;
    }
    if (last)
        *last = '\0';
    else
        *s1 = '\0';
}

/*
static char *insert_seq_no(char *str, char *suffix, char *format, int n)
{
    char *p;
    string tag;
    static string result;
    
    p = strstr(str, suffix);
    
    if (!p) return NULL;
    
    *p = '\0';
    
    sprintf(tag, format, n);
    
    strcpy(result, str);
    strcat(result, tag);
    strcat(result, suffix);
    
    return result;
}
 */

char *GetFileType(const char *file)
{
    int n;
    string tmp, suffix;
    char *p;
    
    strcpy(tmp, file);
    
    p = strtok(tmp, ".");
    
    while (p) {
        strcpy(suffix, p);
        p = strtok(NULL, ".");
    }
    
    for (n=0; n<nFileTypes; n++) {
        if (strcmp(suffix, FileSuffix[n])==0) {
            return FileType[n];
        }
    }
    
    return NULL;
}

char *StripPath(const char *file)
{
    static string tmp, suffix;
    char *p;
    
    strcpy(tmp, file);
    
    p = strtok(tmp, "/");
    
    while (p) {
        strcpy(suffix, p);
        p = strtok(NULL, "/");
    }
    
    return suffix;
}

char *StripSuffix(const char *file)
{
    int n;
    static string body;
    char *p;
    
    p = rindex(file, '.');
    n = p-file;
    
    strncpy(body, file, n);
    body[n] = '\0';
    
    return body;
}

char *GetSpeDesc(scanPtr s, int multiLine)
{
    static string desc = "";
    double fcent;
    DATE d;
    
    char *GetRAStr(double), *GetDECStr(double);
    char *GetLongStr(double), *GetLatStr(double);
    double xmap(scanPtr), ymap(scanPtr);
    
    if (!s) return desc;
    
    d = s->date;
    fcent = 500.0*(s->freq0 + s->freqn);
    
    if (s->coordType == COORD_TYPE_GAL) {
        sprintf(desc,
"%4d %12s %4d%02d%02d %2dh%02dm%02ds %s %s%c%-12s %8.1f %7.2f %4d (%6.1f,%6.1f)",
            s->scan_no,
            s->name,
            d.Year, d.Month, d.Day, d.Hour, d.Min, d.Sec,
            GetLongStr(s->x0), GetLatStr(s->y0),
            multiLine ? '\n' : ' ',
            s->molecule, fcent, s->vlsr,
            s->nChan,
            xmap(s), ymap(s));
    } else {
        sprintf(desc,
"%4d %12s %4d%02d%02d %2dh%02dm%02ds %s %s%c%-12s %8.1f %7.2f %4d (%6.1f,%6.1f)",
            s->scan_no,
            s->name,
            d.Year, d.Month, d.Day, d.Hour, d.Min, d.Sec,
            GetRAStr(s->x0), GetDECStr(s->y0),
            multiLine ? '\n' : ' ',
            s->molecule, fcent, s->vlsr,
            s->nChan,
            xmap(s), ymap(s));
    }
    
    return desc;
}

void SaveAndViewSpecTable(char *file)
{
    int n = 0;
    string buf;
    FILE *fp;
    list curr=NULL;
    
    void XS_system();
    
    if (count_scans(vP->from) <= 0) {
        PostWarningDialog(NULL, "There are no spectra to make a table of.");
        return;
    }
    
    fp = fopen(file, "w");
    
    if (!fp) {
        sprintf(buf, "Couldn't open file '%s' for writing table.", file);
        PostWarningDialog(NULL, buf);
        return;
    }
    
    while ( (curr = scan_iterator(curr, vP->from)) ) {
        fprintf(fp, "%5d %s\n", n+1, GetSpeDesc((scanPtr)DATA(curr), 0));
        n++;
    }
    
    fclose(fp);
    
    sprintf(buf, "%s %s &", pP->editor, file);
    XS_system(buf, 1);
    
    return;
}
