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
#define CARDLEN   80
#define MAXARGS   20

#define FILE_NATIVE  0
#define FILE_FITS    1
#define FILE_ANTS    2
#define FILE_DRP     3
#define FILE_UNKNOWN 4

/*** External variables ***/
extern VIEW  *vP;
extern BLINE  bl;
extern MARK   marks[MAXMARK];
extern BOX    boxar[MAXBOX], regs[MAXBOX];
extern MOD    mods[MAXMOD];
extern int    nmark, nreg, nbox, nmod;
extern DRAW   draw;
extern USER  *pP;

void PostErrorDialog(Widget, char *);
void PostWarningDialog(Widget, char *);
void ManageDialogCenteredOnPointer(Widget);

list    scan_iterator(list, DataSetPtr);
int     count_scans(DataSetPtr);
scanPtr new_scan(DataSetPtr, int);

/*** Local variables ***/
static string data_dir, ps_dir, ants_dir, wfits_dir, drp_dir, class_dir;
static string r_sin_f_dir, r_seq_f_dir, r_map_f_dir, r_2d_f_dir, r_3d_f_dir;
static string wtab_dir, seco_dir;
static string rsca_dir, wsca_dir;
static string wmark_dir, rmark_dir;
static string wbox_dir, rbox_dir;
static string wgauss_dir, rgauss_dir;
static string wstate_dir, rstate_dir;
static string rholo_dir, rapex_dir;
static FDATA fd;
FDATA *fP = &fd;

#define nFileTypes 14

static char *FileType[] = {
    "native",
    "fits",
    "ants",
    "drp",
    "scatter",
    "marker",
    "gaussian",
    "boxes",
    "graph",
    "state",
    "holography",
    "pointing",
    "class",
    "unknown"
};

static char *FileSuffix[] = {
    "out",
    "fits",
    "ants",
    "drp",
    "sca",
    "mrk",
    "gauss",
    "box",
    "gra",
    "state",
    "holo1",
    "tpoint",
    "apex",
    ""
};

static char *OpenSingleFits_Help = "\
                         Open Single FITS file\n\
                         ---------------------\n\
From this dialog you can open one 1-dimensional FITS file. The default suffix\n\
is *.fits. You may change the filter string and press the Filter button to\n\
change the selected files. In the directory list you may switch to the parent\n\
directory by double-clicking on '..'. After selecting 'Open' the FITS file is\n\
loaded. The read scan will become a new data set. Use the append option if\n\
newly read spectra are to be appended to current data set.\
";
static char *OpenMultipleFits_Help = "\
                        Open Multiple FITS files\n\
                        ------------------------\n\
From this file selection dialog you will be able to start a new list dialog.\n\
Just open a file and all matching files in that directory will appear in a\n\
special list dialog in which FITS header information is displayed for each\n\
spectrum. Spectra can then be selected individually. The read spectra will\n\
be gridded. This option is therefore useful when reading map data. Note however\n\
that the read scans will become a new data set. Use the 'Append Multiple...'\n\
option if the new spectra are to be appended to current the data set.\
";
static char *AppendMultipleFits_Help = "\
                       Append Multiple FITS files\n\
                       --------------------------\n\
From this file selection dialog you will be able to start a new list dialog.\n\
Just open a file and all matching files in that directory will appear in a\n\
special list dialog in which FITS header information is displayed for each\n\
spectrum. Spectra can then be selected individually. The read spectra will\n\
be gridded together with any previously read spectra. This option is therefore\n\
useful when adding map spectra to previously read data set.\
";
static char *OpenSequenceFits_Help = "\
                        Open a Sequence FITS files\n\
                        --------------------------\n\
From this file selection dialog you will be able to start a new list dialog.\n\
Just open a file and all matching files in that directory will appear in a\n\
special list dialog in which FITS header information is displayed for each\n\
spectrum. Spectra can then be selected individually. The read spectra will\n\
not be gridded. This option is therefore useful when reading several spectra\n\
that do not represent a map but, for instance, spectra of the same transition\n\
in several different sources.\
";
static char *AppendSequenceFits_Help = "\
                        Append a Sequence FITS files\n\
                        ----------------------------\n\
From this file selection dialog you will be able to start a new list dialog.\n\
Just open a file and all matching files in that directory will appear in a\n\
special list dialog in which FITS header information is displayed for each\n\
spectrum. Spectra can then be selected individually. The read spectra will\n\
not be gridded but appended to the current data set. This option is\n\
therefore useful when reading several spectra that do not represent a map but,\n\
for instance, spectra of the same transition in several different sources.\
";
static char *OpenClass_Help = "\
                         Open CLASS file\n\
                         --------------------\n\
From this dialog you can open one CLASS (the data reduction package in Gildas)\n\
file. The default suffix here is *.apex, since CLASS files at APEX have this\n\
extension. IRAM telescopes have different extensions You may change the filter\n\
string and press the Filter button to change the selected files. In the\n\
directory list you may switch to the parent directory by double-clicking\n\
on '..'. After selecting 'Open' the CLASS file is loaded. Since a CLASS file\n\
usually contains several scans a dialog scan listing will appear.\
";
static char *OpenSingleDRP_Help = "\
                         Open Single DRP file\n\
                         --------------------\n\
From this dialog you can open one DRP (the SEST data reduction package) file.\n\
The default suffix is *.drp. You may change the filter string and press the\n\
Filter button to change the selected files. In the directory list you may\n\
switch to the parent directory by double-clicking on '..'. After selecting\n\
'Open' the DRP file is loaded. The read scan will become a new data set. Use\n\
the append option if read scans are to be appended to the current data set.\
";
static char *OpenMultipleDRP_Help = "\
                        Open Multiple DRP files\n\
                        -----------------------\n\
From this file selection dialog you will be able to start a new list dialog.\n\
Just open a file and all matching files in that directory will appear in a\n\
special list dialog in which DRP header information is displayed for each\n\
spectrum. Spectra can then be selected individually. The read spectra will\n\
be gridded. This option is therefore useful when reading map data. The read\n\
scans will be saved under a new data set. Use the 'Append Multiple...' option\n\
if the new spectra are to be appended to the current data set.\
";
static char *AppendMultipleDRP_Help = "\
                       Append Multiple DRP files\n\
                       -------------------------\n\
From this file selection dialog you will be able to start a new list dialog.\n\
Just open a file and all matching files in that directory will appear in a\n\
special list dialog in which DRP header information is displayed for each\n\
spectrum. Spectra can then be selected individually. The read spectra will\n\
be gridded together with those in the current data set. This option is\n\
therefore useful when adding map spectra to previously read map spectra.\
";
static char *OpenSequenceDRP_Help = "\
                        Open a Sequence DRP files\n\
                        -------------------------\n\
From this file selection dialog you will be able to start a new list dialog.\n\
Just open a file and all matching files in that directory will appear in a\n\
special list dialog in which DRP header information is displayed for each\n\
spectrum. Spectra can then be selected individually. The read spectra will\n\
not be gridded. This option is therefore useful when reading several spectra\n\
that do not represent a map but, for instance, spectra of the same transition\n\
in several different sources.\
";
static char *AppendSequenceDRP_Help = "\
                        Append a Sequence DRP files\n\
                        ---------------------------\n\
From this file selection dialog you will be able to start a new list dialog.\n\
Just open a file and all matching files in that directory will appear in a\n\
special list dialog in which DRP header information is displayed for each\n\
spectrum. Spectra can then be selected individually. The read spectra will\n\
not be gridded but appended to previously read spectra. This option is\n\
therefore useful when reading several spectra that do not represent a map but,\n\
for instance, spectra of the same transition in several different sources.\
";
static char *OpenMultipleBinTabFits_Help = "\
                Open Binary Table FITS file as multiple\n\
                ---------------------------------------\n\
From this dialog you can open one Binary Table FITS file. The default suffix\n\
is *.fits. You may change the filter string and press the Filter button to\n\
change the selected files. In the directory list you may switch to the parent\n\
directory by double-clicking on '..'. After selecting 'Open' the FITS file is\n\
loaded. Note that previously loaded spectra (not maps) will disappear.\n\
Currently, only files saved from the on-the-fly mapping at SEST work.\n\
The scans in the binary table will become a new data set.\n\
";
static char *Open2DFits_Help = "\
                        Open 2D FITS image file\n\
                        -----------------------\n\
From this dialog you can open one 2-dimensional FITS file. The default suffix\n\
is *.fits. You may change the filter string and press the Filter button to\n\
change the selected files. In the directory list you may switch to the parent\n\
directory by double-clicking on '..'. After selecting 'Open' the map (image)\n\
will be loaded (if read successfully). The map name will be shown under the\n\
'Maps' pulldown menu from which it can always be slected until manually\n\
deleted.\
";
static char *Open3DFits_Help = "\
                          Open 3D FITS cube file\n\
                          ----------------------\n\
From this dialog you can open one 3-dimensional FITS file. The default suffix\n\
is *.fits. You may change the filter string and press the Filter button to\n\
change the selected files. In the directory list you may switch to the parent\n\
directory by double-clicking on '..'. After selecting 'Open' the cube will\n\
be loaded (if read successfully) as a number of scans. The cube will become\n\
a new data set. This way of representing a FITS cube is not very efficient for\n\
cubes with many pixels (e.g. FITS cubes from interferometers). The main aim of\n\
"PKGNAME" is to handle unevenly spaced map data from a non-rotating imaging\n\
receiver.\
";
static char *OpenScatterFile_Help = "\
                           Open a Scatter file\n\
                           -------------------\n\
From this dialog you can open one "PKGNAME" scatter file. The default suffix\n\
is *.sca. You may change the filter string and press the Filter button to\n\
change the selected files. In the directory list you may switch to the parent\n\
directory by double-clicking on '..'. After selecting 'Open' the scatter file\n\
will be loaded (if read successfully) and its name appear in the pulldown menu\n\
'Scatter' from which it can be selected until manually deleted.\n\
The scatter file should consist of four columns of data:\n\
     x1   y1   ex1   ey1\n\
     x2   y2   ex2   ey2\n\
     x3   y3   ex3   ey3\n\
           etc.\n\
The columns should be space (or tab) separated but need not be exactly aligned.\n\
The first two columns represent the x and y values. The latter two are the\n\
corresponding 1-sigma errors. Rows starting with # or ! are regarded as\n\
comments and skipped.\
";
static char *OpenBoxFile_Help = "\
                            Open a Box file\n\
                            ---------------\n\
From this dialog you can open one "PKGNAME" box file. The default suffix\n\
is *.box. You may change the filter string and press the Filter button to\n\
change the selected files. In the directory list you may switch to the parent\n\
directory by double-clicking on '..'. After selecting 'Open' the box file\n\
will be loaded (if read successfully) and the new boxes will be used with\n\
the current spectral data. Both baseline boxes and moment regions will be\n\
save in the same file.\n\
The box file should consist of four columns of data:\n\
     B   0   b0   e0\n\
     B   1   b1   e1\n\
             .\n\
             .\n\
             .\n\
     B   n   bn   en\n\
     R   0   b0   e0\n\
             .\n\
             .\n\
             .\n\
     R   m   bm   em\n\
The columns should be space (or tab) separated but need not be exactly aligned.\n\
The first entry is a letter B or R representing rms boxes and moment regions,\n\
respectively. The second entry is just a running number. The 3rd and 4th column\n\
are the first and last channel numbers that define the box.\n\
Rows starting with # or ! are regarded as\n\
comments and skipped.\
";
static char *OpenStateFile_Help = "\
                            Open a state file\n\
                            ---------------\n\
From this dialog you can open one "PKGNAME" state file. The default suffix\n\
is *.state. You may change the filter string and press the Filter button to\n\
change the selected files. In the directory list you may switch to the parent\n\
directory by double-clicking on '..'. After selecting 'Open' the state file\n\
will be loaded (if read successfully) and the new graphical state will be used\n\
with the current spectral data. The file contains binary data, and is thus\n\
not editable, describing the entire graphical state (excluding the PostScript\n\
parameters).\
";
static char *OpenMarkerFile_Help = "\
                           Open a Marker file\n\
                           -------------------\n\
From this dialog you can open one "PKGNAME" marker file. The default suffix\n\
is *.mrk. You may change the filter string and press the Filter button to\n\
change the selected files. In the directory list you may switch to the parent\n\
directory by double-clicking on '..'. After selecting 'Open' the marker file\n\
will be loaded (if read successfully) and its content shown in current graph.\n\
If opened in append mode the content of the file will be appended to the\n\
markers already read.\n\
The marker file should start with a integer value on the first line describing\n\
the type of plot it is supposed to be used for. The currently accepted values\n\
are:\n\
             0           Spectrum with frequency [GHz] x-scale\n\
             1           Spectrum with velocity [km/s] x-scale\n\
             2           Spectrum with channel x-scale\n\
             3-4         Any map or scatter plot\n\
             5           Spectrum with offset frequency [MHZ] x-scale\n\
This number works only as a guide and "PKGNAME" tries to show the markers\n\
regardless of the actual value. See also first column data below.\n\n\
The rest of the file should contain 13 space-separated columns of data. The\n\
columns represent:\n\
    column 1     Integer defining the plotting mode for the marker. The\n\
                 available modes are:\n\
                     0 Plot if current view is a spectrum\n\
                     1 Plot if view is multiple spectra\n\
                     3 Plot if view is a map\n\
                     4 Plot if view is a vel-position map\n\
                     5 Plot if view is a pos-velocity map\n\
                     6 Plot if view is a scatter diagram\n\
    column 2     x-coordinate of marker\n\
    column 3     y-coordinate of marker\n\
    column 4     Type of marker. Options are:\n\
                     0 Arrow\n\
                     1 Line\n\
                     2 Square\n\
                     3 Circle\n\
                     4 Empty marker\n\
                     5 Special PGPLOT marker\n\
    column 5     If col 4 has the number 5 the number here specifies\n\
                 the kind of PGPLOT marker (only visible in the PostScript\n\
                 view/print). Please consult the PGPLOT manual.\n\
    column 6     Marker direction. Options are:\n\
                     0 Downward marker\n\
                     1 Upward marker\n\
                     2 Left marker\n\
                     3 Right marker\n\
                 Note that the direction of some kind markers (i.e. circles)\n\
                 is undefined. The placement of the label string relative the\n\
                 marker is however always dependent on the direction.\n\
    column 7     Marker size in x-direction (default size is 20).\n\
    column 8     Marker size in y-direction (default size is 20).\n\
    column 9     Label alignment. 0.0 is left justified and 1.0 is right\n\
                 justified. Note that negative values and values larger than 1\n\
                 are allowed.\n\
    column 10    Label angle (only for PS plots). Angle entered in degrees.\n\
    column 11    Indicate whether the marker is tagged or not. If 1 the marker\n\
                 is tagged and if zero it is untagged. The usage of tagged\n\
                 markers is taht within "PKGNAME" certain operations (like\n\
                 aligning) on tagged markers can be performed.\n\
    column 12    Indicate whether the marker is to be joined with another\n\
                 marker through a line. The value here, if >= 0, indicates\n\
                 the running number of the marker it is to be joined with.\n\
                 The number must be smaller than the total number of markers\n\
                 and, of course, also smaller than the maximum number of\n\
                 markers allowed, which is 500. The number 0 refers to the\n\
                 first marker.\n\
    column 13    The label of the marker. To allow for label strings of\n\
                 arbitrary length and to allow for spaces within the string,\n\
                 the label string should be enclosed within double quotes, i.e.\n\
                                  \"CO(J=1-0)  NGC4945\"\n\
                 To include a double quote in the string prepend it with the\n\
                 escape character backslash (\\). Use a double backslash (\\\\)\n\
                 to enter the backslash sign. The entry may be omitted which is\n\
                 the same as the empty string \"\".\n\n\
The easiest way of generating a marker file is to interactingly place the\n\
markers within "PKGNAME" and then save the markers using the 'Save marker file'\n\
option. This file can then be edited with any text editor.\
";

static int  get_native(char *, FDATA *);
static void write_file(char *, char *);
static void write_all_files(char *, char *, char *);
static void nospace_strcpy(char *, char *);
