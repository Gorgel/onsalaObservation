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
#ifndef LIST_H
#include "list.h"
#endif

#ifndef MENUS_H
#include "menus.h"
#endif

void  get_file();
void  set_file();
void  set_PS_file();
void  get_scales();
void  toggle_any();
void  set_show_mode();
void  redraw_graph();
void  gauss_reset();
void  box_reset();
void  mark_reset();
void  channel_mod();
void  mod_reset();
void  new_gaussian(), new_box();
void  do_fit(), do_baseline_fit();
void  reset_bl_parameters();
void  SetRmsFromBoxes();
void  remove_poly();
void  add_poly();
void  interpolate_from_poly();
void  PolynomialOrderDialog();
void  GaussiansToMarkers();
void  MathSelectedGaussian();
void  open_freq_file();
void  PostVLSRDialog();
void  PostImageFreqDialog();
void  PostShiftDialog();
void  PostVelShiftDialog();
void  obtain_map_info();
void  PostManualContourDialog();
void  PostRangeStepContourDialog();
void  PostContourDialog();
void  do_quit();
void  AverageScans();
void  scale_scans();
void  smooth_scans();
void  redres_scans();
void  invert_scans();
void  FoldSelect();
void  do_fft();
void  do_cor();
void  swap_spectrum_and_rms();
void  store_current_map();
void  StoreMultipleMaps();
void  StoreCurrentScatter();
void  manipulate_maps();
void  manipulate_datasets();
void  manipulate_spectra();
void  manipulate_scats();
void  ScatterPointOps();
void  manipulate_sources();
list *get_maplist();
list *get_scatterlist();
list *get_listlist();
void  MapDraw();
void  ScatterDraw();
void  SelectDataSet();
void  PostScatterOptionDialog();
void  MakeSingleSpeScatterPlot();
void  AdjustScale();
void  InvertScale();
void  MenuSavePrefsFile();
#ifdef APEX
void  EditHoloFit();
void  EditHoloData();
void  EditAPEXMap();
#endif
void  EditPrefsFile();
void  ViewPrefsFile();
void  PrintPostScriptFile();
void  ShowPostScriptFile();
void  DoMenuZoom();
void  ScaleMarkers();
void  DoClipScans();
void  PostFilterDialog();
void  ViewCurrentMapAsScatterPlot();
void  PostScatterTypeDialog();
void  FitAllGaussians();
void  PostSubMapDialog();
void  PostLabelDialog();
void  PostModifyHeaderDialog();
void  AttachContToMap();
void  AttachBoxesToDataset();
void  PostMapTypeDialog();
void  GaussViewer();
void  PostCoordDialog();
void  PostPreDialog();
void  PostTestMapDialog();
void  PostMacroEditDialog();
void  Post2DimFitDialog();
void  ExecuteMacro();
void  EditCurrentBaselineParameters();
void  PostDotDialog();
void  GaussModeDialog();
void  PostPolyCircleDialog();
void  MenuDeletePolyLines();
void  AttachScale();
/* Help menu functions */
void  About();
void  Version();
void  ReadMe();
void  ProblemsAndBugs();
void  WhatsNew();
void  Help();

/*** File Menu Items ***/
MenuItem OpenNativeFileMenuData[] = {
   {"Open primary...", &xmPushButtonGadgetClass,
    'p', "Ctrl<Key>O", "Ctrl+O", False, NULL, get_file, "read", NULL},
   {"Open secondary...", &xmPushButtonGadgetClass,
    's', "Ctrl<Key>2", "Ctrl+2", False, NULL, get_file, "secondary", NULL},
EOI};

MenuItem OpenFitsFileMenuData[] = {
   {"Open single...", &xmPushButtonGadgetClass,
    's', "Ctrl<Key>F", "Ctrl+F", False, NULL, get_file, "fits", NULL},
   {"Open multiple (map)...", &xmPushButtonGadgetClass,
    'm', "Ctrl<Key>M", "Ctrl+M", False, NULL, get_file, "mapfits", NULL},
   {"Open sequence...", &xmPushButtonGadgetClass,
    'q', "Ctrl<Key>S", "Ctrl+S", False, NULL, get_file, "seqfits", NULL},
   {"Append to multiple...", &xmPushButtonGadgetClass,
    'a', NULL, NULL, False, NULL, get_file, "appmapfits", NULL},
   {"Append to sequence...", &xmPushButtonGadgetClass,
    'p', NULL, NULL, False, NULL, get_file, "appseqfits", NULL},
 MENUSEPARATOR,
  {"Open FITS array...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_file, "2dfits", NULL},
  {"Open FITS cube...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_file, "3dfits", NULL},
 MENUSEPARATOR,
   {"Open binary table as multiple...", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, get_file, "bintab", NULL},
   {"Open binary table as sequence...", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, get_file, "seqbintab", NULL},
EOI};

MenuItem OpenClassFileMenuData[] = {
   {"Open CLASS file...", &xmPushButtonGadgetClass,
    's', NULL, NULL, False, NULL, get_file, "class", NULL},
EOI};

MenuItem OpenDrpFileMenuData[] = {
   {"Open single...", &xmPushButtonGadgetClass,
    's', NULL, NULL, False, NULL, get_file, "drp", NULL},
   {"Open multiple (map)...", &xmPushButtonGadgetClass,
    'm', NULL, NULL, False, NULL, get_file, "mapdrp", NULL},
   {"Open sequence...", &xmPushButtonGadgetClass,
    'q', NULL, NULL, False, NULL, get_file, "seqdrp", NULL},
   {"Append to multiple...", &xmPushButtonGadgetClass,
    'a', NULL, NULL, False, NULL, get_file, "appmapdrp", NULL},
   {"Append to sequence...", &xmPushButtonGadgetClass,
    'p', NULL, NULL, False, NULL, get_file, "appseqdrp", NULL},
EOI};

MenuItem OpenAntsFileMenuData[] = {
   {"Open single...", &xmPushButtonGadgetClass,
    's', NULL, NULL, False, NULL, get_file, "ants", NULL},
   {"Open single (2nd receiver)...", &xmPushButtonGadgetClass,
    'i', NULL, NULL, False, NULL, get_file, "ants2", NULL},
   {"Open multiple...", &xmPushButtonGadgetClass,
    'm', NULL, NULL, False, NULL, get_file, "mapants", NULL},
   {"Open multiple (2nd receiver)...", &xmPushButtonGadgetClass,
    'u', NULL, NULL, False, NULL, get_file, "mapants2", NULL},
   {"Open sequence...", &xmPushButtonGadgetClass,
    's', NULL, NULL, False, NULL, get_file, "seqants", NULL},
   {"Open sequence (2nd receiver)...", &xmPushButtonGadgetClass,
    'e', NULL, NULL, False, NULL, get_file, "seqants2", NULL},
   {"Append multiple...", &xmPushButtonGadgetClass,
    'm', NULL, NULL, False, NULL, get_file, "appmapants", NULL},
   {"Append multiple (2nd receiver)...", &xmPushButtonGadgetClass,
    'u', NULL, NULL, False, NULL, get_file, "appmapants2", NULL},
   {"Append sequence...", &xmPushButtonGadgetClass,
    's', NULL, NULL, False, NULL, get_file, "appseqants", NULL},
   {"Append sequence (2nd receiver)...", &xmPushButtonGadgetClass,
    'e', NULL, NULL, False, NULL, get_file, "appseqants2", NULL},
EOI};

MenuItem SaveBTFitsFileMenuData[] = {
  {"Odin format", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, set_file, "wbintab_Odin", NULL},
EOI};

MenuItem SaveFitsFileMenuData[] = {
  {"Save single FITS...", &xmPushButtonGadgetClass,
   'i', NULL, NULL, False, NULL, set_file, "fwrite", NULL},
  {"Save FITS array...", &xmPushButtonGadgetClass,
   'a', NULL, NULL, False, NULL, set_file, "f2Dwrite", NULL},
  {"Save FITS cube...", &xmPushButtonGadgetClass,
   'c', NULL, NULL, False, NULL, set_file, "f3Dwrite", NULL},
  {"Save multiple FITS...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_file, "mfwrite", NULL},
 MENUSEPARATOR,
  {"Save dataset as FITS binary table using", &xmCascadeButtonGadgetClass,
   'F', NULL, NULL, True, NULL, NULL, NULL, SaveBTFitsFileMenuData},
EOI};

MenuItem OpenFreqFileMenuData[] = {
   {"SLAIM...", &xmPushButtonGadgetClass,
    'S', NULL, NULL, False, NULL, open_freq_file, "SLAIM", NULL},
   {"Lovas...", &xmPushButtonGadgetClass,
    'L', NULL, NULL, False, NULL, open_freq_file, "LOVAS", NULL},
   {"JPL...", &xmPushButtonGadgetClass,
    'J', NULL, NULL, False, NULL, open_freq_file, "JPL", NULL},
EOI};

MenuItem PrintPSFileMenuData[] = {
   {"PS landscape", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PrintPostScriptFile, "ps", NULL},
   {"PS portrait", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PrintPostScriptFile, "vps", NULL},
   {"Color PS landscape", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PrintPostScriptFile, "cps", NULL},
   {"Color PS portrait", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PrintPostScriptFile, "vcps", NULL},
EOI};

MenuItem SavePSFileMenuData[] = {
   {"PS landscape", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, set_PS_file, "ps", NULL},
   {"PS portrait", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, set_PS_file, "vps", NULL},
   {"Color PS landscape", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, set_PS_file, "cps", NULL},
   {"Color PS portrait", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, set_PS_file, "vcps", NULL},
EOI};

MenuItem FileMenuData[] = {
  {"Open native", &xmCascadeButtonGadgetClass,
   'O', NULL, NULL, True, NULL, NULL, NULL, OpenNativeFileMenuData},
  {"Open FITS", &xmCascadeButtonGadgetClass,
   'F', NULL, NULL, True, NULL, NULL, NULL, OpenFitsFileMenuData},
  {"Open CLASS", &xmCascadeButtonGadgetClass,
   'C', NULL, NULL, True, NULL, NULL, NULL, OpenClassFileMenuData},
  {"Open DRP", &xmCascadeButtonGadgetClass,
   'D', NULL, NULL, True, NULL, NULL, NULL, OpenDrpFileMenuData},
  {"Open POPS", &xmCascadeButtonGadgetClass,
   'P', NULL, NULL, True, NULL, NULL, NULL, OpenAntsFileMenuData},
 MENUSEPARATOR,
  {"Set FITS/DRP/CLASS filter...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostFilterDialog, NULL, NULL},
  {"Modify header...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostModifyHeaderDialog, NULL, NULL},
 MENUSEPARATOR,
  {"Open frequency file", &xmCascadeButtonGadgetClass,
   'q', NULL, NULL, True, NULL, NULL, NULL, OpenFreqFileMenuData},
#ifdef APEX
  {"Open holography file...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_file, "rholo", NULL},
  {"Open APEX map file...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_file, "rapex", NULL},
#endif
 MENUSEPARATOR,
  {"Open scatter...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_file, "rsca", NULL},
  {"Save scatter...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_file, "wsca", NULL},
 MENUSEPARATOR,
  {"Open graphical state...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_file, "rstate", NULL},
  {"Save graphical state...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_file, "wstate", NULL},
 MENUSEPARATOR,
  {"Save native...", &xmPushButtonGadgetClass,
   'S', NULL, NULL, False, NULL, set_file, "write", NULL},
  {"Save FITS", &xmCascadeButtonGadgetClass,
   'F', NULL, NULL, True, NULL, NULL, NULL, SaveFitsFileMenuData},
 MENUSEPARATOR,
  {"Save current spectra as ASCII table...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_file, "spetable", NULL},
  {"Save current map as ASCII table...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_file, "maptable", NULL},
  {"Save current scatter plot as ASCII table...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_file, "scatable", NULL},
#ifdef HAVE_LIBPGPLOT
 MENUSEPARATOR,
  {"Show PostScript file", &xmPushButtonGadgetClass,
   ' ', "Ctrl<Key>P", "Ctrl+P", False, NULL, ShowPostScriptFile, "xwin", NULL},
  {"Save PostScript file...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NULL, NULL, SavePSFileMenuData},
  {"Print PostScript file", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, PrintPSFileMenuData},
#endif
 MENUSEPARATOR,
  {"Exit", &xmPushButtonGadgetClass,
   'x', "Ctrl<Key>Q", "Ctrl+Q", False, NULL, do_quit, "quit", NULL},
EOI};


/*** Graph Menu Items ***/

MenuItem ZoomMenuData[] = {
  {"Zoom x2.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, DoMenuZoom, "bin", NULL},
  {"Zoom only X x2.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, DoMenuZoom, "xin", NULL},
  {"Zoom only Y x2.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, DoMenuZoom, "yin", NULL},
  {"Zoom x0.5", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, DoMenuZoom, "bout", NULL},
  {"Zoom only X x0.5", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, DoMenuZoom, "xout", NULL},
  {"Zoom only Y x0.5", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, DoMenuZoom, "yout", NULL},
EOI};

MenuItem DefaultscaleMenuData[] = {
  {"x-axis", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, AdjustScale, "x", NULL},
  {"y-axis", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, AdjustScale, "y", NULL},
EOI};

MenuItem InvertMenuData[] = {
  {"x-axis", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, InvertScale, "x", NULL},
  {"y-axis", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, InvertScale, "y", NULL},
EOI};

CallbackData GMD_Xfixed = {NULL, "xfixed", &(view.fixed_x), NULL};
CallbackData GMD_Yfixed = {NULL, "yfixed", &(view.fixed_y), NULL};
CallbackData GMD_Zfixed = {NULL, "zfixed", &(view.fixed_z), NULL};
CallbackData GMD_Xauto = {NULL, "xauto", &(view.autoscale_x), NULL};
CallbackData GMD_Yauto = {NULL, "yauto", &(view.autoscale_y), NULL};
CallbackData GMD_attach = {NULL, "attached", &(view.use_attached_frame), NULL};

MenuItem FreezescaleMenuData[] = {
  {"x-scale", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&GMD_Xfixed, NULL},
  {"y-scale", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&GMD_Yfixed, NULL},
EOI};

MenuItem AutoscaleMenuData[] = {
  {"x-axis", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&GMD_Xauto, NULL},
  {"y-axis", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&GMD_Yauto, NULL},
EOI};

int SecondSpectrum = 2;
int AllSpectra = 3;

CallbackData TMD_Clear   = {NULL, "clear",   &draw.clear,     NULL};
CallbackData TMD_Update  = {NULL, "update",  &draw.update,    NULL};
CallbackData TMD_Data    = {NULL, "data",    &draw.data,      NULL};
CallbackData TMD_Rms     = {NULL, "rms",     &draw.data_rms,  NULL};
CallbackData TMD_Secone  = {NULL, "sec",     &draw.data_sec, &SecondSpectrum};
CallbackData TMD_Secall  = {NULL, "all",     &draw.data_sec, &AllSpectra};
CallbackData TMD_Frame   = {NULL, "frame",   &draw.frame,     NULL};
CallbackData TMD_WFrame  = {NULL, "Wframe",  &draw.wframe,    NULL};
CallbackData TMD_Labels  = {NULL, "labels",  &draw.labels,    NULL};
CallbackData TMD_WLabels = {NULL, "Wlabels", &draw.wlabels,   NULL};
CallbackData TMD_Ticks   = {NULL, "ticks",   &draw.ticks,     NULL};
CallbackData TMD_WTicks  = {NULL, "Wticks",  &draw.wticks,    NULL};
CallbackData TMD_Markers = {NULL, "markers", &draw.markers,   NULL};
CallbackData TMD_Histo   = {NULL, "histo",   &draw.histo,     NULL};
CallbackData TMD_SHisto  = {NULL, "shisto",  &draw.histo_sec, NULL};
CallbackData TMD_Zline   = {NULL, "zline",   &draw.zline,     NULL};
CallbackData TMD_Boxes   = {NULL, "boxes",   &draw.boxes,     NULL};
CallbackData TMD_Poly    = {NULL, "poly",    &draw.poly,      NULL};
CallbackData TMD_Gsum    = {NULL, "gsum",    &draw.gsum,      NULL};
CallbackData TMD_Gind    = {NULL, "gind",    &draw.gind,      NULL};
CallbackData TMD_XEbars  = {NULL, "xebars",  &draw.xebars,    NULL};
CallbackData TMD_YEbars  = {NULL, "yebars",  &draw.yebars,    NULL};
CallbackData TMD_PAxes   = {NULL, "paxes",   &draw.projaxes,  NULL};
CallbackData TMD_PNums   = {NULL, "pnums",   &draw.projnums,  NULL};
CallbackData TMD_Mult    = {NULL, "mult",    &draw.multiple,  NULL};
CallbackData TMD_Header  = {NULL, "header",  &draw.header,    NULL};

MenuItem ToggleMenuData[] = {
  {"Toggle window clear", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Clear, NULL},
  {"Toggle auto-updating", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Update, NULL},
  {"Toggle header", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Header, NULL},
  {"Toggle data", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Data, NULL},
  {"Toggle RMS", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Rms, NULL},
  {"Toggle secondary", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Secone, NULL},
  {"Toggle all as secs.", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Secall, NULL},
  {"Toggle frame", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Frame, NULL},
  {"Toggle labels", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Labels, NULL},
  {"Toggle tick marks", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Ticks, NULL},
  {"Toggle markers", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Markers, NULL},
  {"Toggle line/histo", &xmToggleButtonGadgetClass, 'h', "Ctrl<Key>H",
   "Ctrl+H", False, NULL, toggle_any, (XtPointer)&TMD_Histo, NULL},
  {"Toggle sec. line/histo", &xmToggleButtonGadgetClass, ' ', NULL,
   NULL, False, NULL, toggle_any, (XtPointer)&TMD_SHisto, NULL},
  {"Toggle zero line", &xmToggleButtonGadgetClass, 'z', "Ctrl<Key>Z",
   "Ctrl+Z", False, NULL, toggle_any, (XtPointer)&TMD_Zline, NULL},
  {"Toggle boxes", &xmToggleButtonGadgetClass, 'x', "Ctrl<Key>X",
   "Ctrl+X", False, NULL, toggle_any, (XtPointer)&TMD_Boxes, NULL},
  {"Toggle poly. fit", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Poly, NULL},
  {"Toggle Gauss sum", &xmToggleButtonGadgetClass,
   'G', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Gsum, NULL},
  {"Toggle Gauss ind.", &xmToggleButtonGadgetClass,
   'i', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Gind, NULL},
  {"Toggle wedge frame", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_WFrame, NULL},
  {"Toggle wedge labels", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_WLabels, NULL},
  {"Toggle wedge tick marks", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_WTicks, NULL},
  {"Toggle scatter overlay", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_Mult, NULL},
  {"Toggle X error bars", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_XEbars, NULL},
  {"Toggle Y error bars", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_YEbars, NULL},
  {"Toggle proj. axes", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_PAxes, NULL},
  {"Toggle proj. numbers", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&TMD_PNums, NULL},
EOI};

MenuItem ShowModeMenuData[] = {
  {"Single spectrum", &xmPushButtonGadgetClass,
   'S', "Alt<Key>1", "Alt+1", False, NULL, set_show_mode, "single", NULL},
  {"Multiple spectra", &xmPushButtonGadgetClass,
   'M', "Alt<Key>2", "Alt+2", False, NULL, set_show_mode, "map", NULL},
  {"Pos&Pos Contour map", &xmPushButtonGadgetClass,
   'P', "Alt<Key>3", "Alt+3", False, NULL, set_show_mode, "contour", NULL},
  {"Vel&Pos contour map", &xmPushButtonGadgetClass,
   'V', "Alt<Key>4", "Alt+4", False, NULL, set_show_mode, "velpos", NULL},
  {"Scatter plot", &xmPushButtonGadgetClass,
   'c', "Alt<Key>5", "Alt+5", False, NULL, set_show_mode, "scatter", NULL},
EOI};

MenuItem DotMarkerData[] = {
  {"In maps...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostDotDialog, "mapdot", NULL},
  {"In scatter plots...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostDotDialog, "scatterdot", NULL},
EOI};

MenuItem ManualContourData[] = {
  {"Manually...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostManualContourDialog, "menu", NULL},
  {"As range/step...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostRangeStepContourDialog, "menu", NULL},
EOI};

MenuItem GraphMenuData[] = {
  {"Redraw graph", &xmPushButtonGadgetClass,
   'R', "Ctrl<Key>R", "Ctrl+R", False, NULL, redraw_graph, "redraw", NULL},
  {"Update graph", &xmPushButtonGadgetClass,
   'U', "Ctrl<Key>U", "Ctrl+U", False, NULL, redraw_graph, "update", NULL},
 MENUSEPARATOR,
  {"Zoom options", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, ZoomMenuData},
  {"Freeze the", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, FreezescaleMenuData},
  {"Default scaling of", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, DefaultscaleMenuData},
  {"Temporarily invert", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, InvertMenuData},
  {"Change scales...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_scales, "Rescale", NULL},
 MENUSEPARATOR,
  {"Attach current scaling", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, AttachScale, "Attach", NULL},
  {"Detach scaling", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, AttachScale, "Detach", NULL},
 MENUSEPARATOR,
  {"Set size of multiple plot...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostSubMapDialog, "nxny", NULL},
  {"Autoscale in mult. plots", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, AutoscaleMenuData},
  {"Use attached scales if any", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&GMD_attach, NULL},
 MENUSEPARATOR,
  {"Type of labels...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostLabelDialog, "label", NULL},
  {"Graph options", &xmCascadeButtonGadgetClass,
   'G', NULL, NULL, True, NULL, NULL, NULL, ToggleMenuData},
  {"Dot markers", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, DotMarkerData},
 MENUSEPARATOR,
  {"Viewing modes", &xmCascadeButtonGadgetClass,
   'S', NULL, NULL, True, NULL, NULL, NULL, ShowModeMenuData},
  {"Type of map...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostMapTypeDialog, NULL, NULL},
  {"Show map info", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, obtain_map_info, "info", NULL},
 MENUSEPARATOR,
  {"Freeze contour levels", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&GMD_Zfixed, NULL},
  {"Set contour levels", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, ManualContourData},
  {"Contour levels...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostContourDialog, "menu", NULL},
EOI};


/*** Gauss Menu Items ***/

MenuItem RemoveGaussMenuData[] = {
  {"All", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, gauss_reset, "all", NULL},
  {"All map Gaussians", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, gauss_reset, "mapall", NULL},
  {"Last", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, gauss_reset, "latest", NULL},
  {"with cursor", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, gauss_reset, "cursor", NULL},
EOI};

MenuItem SaveGaussMenuData[] = {
  {"Write", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_file, "wgauss", NULL},
  {"Append", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_file, "agauss", NULL},
EOI};

MenuItem GaussMenuData[] = {
  {"Start Gauss viewer", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, GaussViewer, "open", NULL},
  {"Close Gauss viewer", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, GaussViewer, "close", NULL},
  {"Gaussian mode...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, GaussModeDialog, NULL, NULL},
 MENUSEPARATOR,
  {"Remove Gaussian", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, RemoveGaussMenuData},
  {"Add Gaussian", &xmPushButtonGadgetClass,
   'G', "Alt<Key>G", "Alt+G", False, NULL, new_gaussian, "draw", NULL},
 MENUSEPARATOR,
  {"Fit Gaussians", &xmPushButtonGadgetClass,
   'F', "Alt<Key>F", "Alt+F", False, NULL, do_fit, "", NULL},
  {"Fit Gaussians to map spectra", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, FitAllGaussians, "", NULL},
  {"Fit 2-dim Gaussian to map", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Post2DimFitDialog, NULL, NULL},
 MENUSEPARATOR,
  {"Add selected Gaussians", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MathSelectedGaussian, "add", NULL},
  {"Add Gaussians to map spectra", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MathSelectedGaussian, "mapadd", NULL},
  {"Subtract selected Gaussians", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MathSelectedGaussian, "sub", NULL},
  {"Subtract Gaussians from map spectra", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MathSelectedGaussian, "mapsub", NULL},
  {"Remove selected Gaussians", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MathSelectedGaussian, "rem", NULL},
 MENUSEPARATOR,
  {"Read Gaussians from file...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_file, "rgauss", NULL},
  {"Save Gaussians to file...", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, SaveGaussMenuData},
 MENUSEPARATOR,
  {"View Gaussians...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_file, "vgauss", NULL},
  {"Sort & View Gaussians...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_file, "vsgauss", NULL},
  {"Make all Gaussians markers", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, GaussiansToMarkers, "", NULL},
EOI};


/*** Baseline Menu Items ***/

MenuItem RemoveBaselineBoxMenuData[] = {
  {"All", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, box_reset, "ball", NULL},
  {"Last", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, box_reset, "blatest", NULL},
  {"with cursor", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, box_reset, "bcursor", NULL},
EOI};

MenuItem BaselineMenuData[] = {
  {"Remove baseline box", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, RemoveBaselineBoxMenuData},
  {"Add baseline box", &xmPushButtonGadgetClass,
   'b', "Alt<Key>B", "Alt+B", False, NULL, new_box, "box", NULL},
  {"Edit baseline box with cursor", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, box_reset, "bedit", NULL},
 MENUSEPARATOR,
  {"Set RMS data from boxes", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, SetRmsFromBoxes, "", NULL},
 MENUSEPARATOR,
  {"Reset parameters", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, reset_bl_parameters, "", NULL},
  {"Edit current coefficients", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, EditCurrentBaselineParameters, "", NULL},
 MENUSEPARATOR,
  {"Set polynomial order...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PolynomialOrderDialog, "", NULL},
  {"Fit baseline", &xmPushButtonGadgetClass,
   'f', "Alt<Key>V", "Alt+V", False, NULL, do_baseline_fit, "", NULL},
  {"Subtract polynomial fit", &xmPushButtonGadgetClass,
   ' ', "Alt<Key>C", "Alt+C", False, NULL, remove_poly, "", NULL},
  {"Add polynomial fit", &xmPushButtonGadgetClass,
   ' ', "Alt<Key>A", "Alt+A", False, NULL, add_poly, "", NULL},
  {"Interpolate from pol. fit", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, interpolate_from_poly, "", NULL},
 MENUSEPARATOR,
  {"Read boxes from file...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_file, "rbox", NULL},
  {"Save boxes to file...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_file, "wbox", NULL},
EOI};


/*** Moment Menu Items ***/

MenuItem RemoveMomentBoxMenuData[] = {
  {"All", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, box_reset, "mall", NULL},
  {"Last", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, box_reset, "mlatest", NULL},
  {"with cursor", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, box_reset, "mcursor", NULL},
EOI};

MenuItem RemovePolyLineMenuData[] = {
  {"All", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MenuDeletePolyLines, "all", NULL},
  {"Last", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MenuDeletePolyLines, "last", NULL},
EOI};

MenuItem MomentMenuData[] = {
  {"Remove moment box", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, RemoveMomentBoxMenuData},
  {"Add moment box", &xmPushButtonGadgetClass,
   'k', "Alt<Key>M", "Alt+M", False, NULL, new_box, "mom", NULL},
  {"Edit moment box with cursor", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, box_reset, "medit", NULL},
 MENUSEPARATOR,
  {"Remove polyline", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, RemovePolyLineMenuData},
  {"Create polyline circle...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostPolyCircleDialog, "circle", NULL},
EOI};


/*** Marker Menu Items ***/
CallbackData MMD_tagging = {NULL, "tagmarkers", &view.tag_markers, NULL};
CallbackData MMD_join    = {NULL, "joinmarkers", &view.join_markers, NULL};
CallbackData MMD_remove  = {NULL, "removejoint", &view.remove_joint, NULL};

MenuItem RemoveMarkerMenuData[] = {
  {"All", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, mark_reset, "all", NULL},
  {"Last", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, mark_reset, "latest", NULL},
  {"with cursor", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, mark_reset, "rcursor", NULL},
EOI};

MenuItem MarkImageFreqFileMenuData[] = {
   {"SLAIM", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, open_freq_file, "imSLAIM", NULL},
   {"Lovas", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, open_freq_file, "imLOVAS", NULL},
   {"JPL", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, open_freq_file, "imJPL", NULL},
EOI};

MenuItem MarkFreqFileMenuData[] = {
   {"SLAIM", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, open_freq_file, "mSLAIM", NULL},
   {"Lovas", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, open_freq_file, "mLOVAS", NULL},
   {"JPL", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, open_freq_file, "mJPL", NULL},
   {"Ident", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, open_freq_file, "mIDENT", NULL},
#ifdef SGRIDENT
 MENUSEPARATOR,
   {"SgrB2(M)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, open_freq_file, "sgrb2m", NULL},
   {"SgrB2(N)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, open_freq_file, "sgrb2n", NULL},
   {"SgrB2(NW)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, open_freq_file, "sgrb2nw", NULL},
#endif
#ifdef ORIONIDENT
 MENUSEPARATOR,
   {"Orion", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, open_freq_file, "orion", NULL},
#endif
EOI};

MenuItem ScaleMarkerMenuData[] = {
  {"all markers with 0.5", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScaleMarkers, "0.5", NULL},
  {"all markers with 0.9", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScaleMarkers, "0.9", NULL},
  {"all markers with 1/0.9", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScaleMarkers, "1.1", NULL},
  {"all markers with 2.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScaleMarkers, "2.0", NULL},
  {"tagged markers with 0.5", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScaleMarkers, "t0.5", NULL},
  {"tagged markers with 0.9", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScaleMarkers, "t0.9", NULL},
  {"tagged markers with 1/0.9", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScaleMarkers, "t1.1", NULL},
  {"tagged markers with 2.0", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScaleMarkers, "t2.0", NULL},
EOI};

MenuItem MarkAlignMenuData[] = {
   {"upwards", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, mark_reset, "up", NULL},
   {"centered", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, mark_reset, "center", NULL},
   {"downwards", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, mark_reset, "down", NULL},
EOI};

MenuItem MarkerMenuData[] = {
  {"Remove marker", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, RemoveMarkerMenuData},
  {"Add new marker with cursor", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, mark_reset, "acursor", NULL},
  {"Edit marker with cursor", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, mark_reset, "ecursor", NULL},
 MENUSEPARATOR,
  {"Open marker file...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_file, "rmark", NULL},
  {"Append marker file...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, get_file, "rappmark", NULL},
  {"Save marker file...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, set_file, "wmark", NULL},
 MENUSEPARATOR,
  {"Join two markers", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&MMD_join, NULL},
  {"Remove marker joint", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&MMD_remove, NULL},
 MENUSEPARATOR,
  {"Toggle marker tagging", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&MMD_tagging, NULL},
  {"Untag all markers", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, mark_reset, "untag", NULL},
 MENUSEPARATOR,
  {"Make freq markers", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, MarkFreqFileMenuData},
  {"Make image freq markers", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, MarkImageFreqFileMenuData},
 MENUSEPARATOR,
  {"Removed tagged markers", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, mark_reset, "rtagged", NULL},
  {"Scale", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, ScaleMarkerMenuData},
  {"Align tagged markers", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, MarkAlignMenuData},
EOI};


/*** Reduction Menu Items ***/

MenuItem AverageTypeMenuData[] = {
   {"System temp. & Int. time", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, AverageScans, "A tsys", NULL},
   {"Integration time only", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, AverageScans, "A time", NULL},
   {"RMS box (accum)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, AverageScans, "A accrms", NULL},
   {"RMS box (add)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, AverageScans, "A addrms", NULL},
   {"Equal weights", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, AverageScans, "A same", NULL},
EOI};

MenuItem AveragePosTypeMenuData[] = {
   {"System temp. & Int. time", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, AverageScans, "P tsys", NULL},
   {"Integration time only", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, AverageScans, "P time", NULL},
   {"RMS box (accum)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, AverageScans, "P accrms", NULL},
   {"Equal weights", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, AverageScans, "P same", NULL},
EOI};

MenuItem FFTTypeMenuData[] = {
   {"Forward FFT (Re & Im)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, do_fft, "cartesian", NULL},
   {"Forward FFT (Ampl. & phase)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, do_fft, "polar", NULL},
   {"Inverse FFT", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, do_fft, "inverse", NULL},
 MENUSEPARATOR,
   {"Channel correlation", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, do_cor, "", NULL},
   {"Sequence correlation", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, do_cor, "seq", NULL},
EOI};

MenuItem SpeConvolveMenuData[] = {
  {"Use antenna temp. unit", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_spectra, "ConvolveTant", NULL},
  {"Use Jy/beam unit", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_spectra, "ConvolveJansky", NULL},
EOI};

MenuItem ReductionMenuData[] = {
#ifdef MACRO
  {"Edit macro...", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, PostMacroEditDialog, "edit", NULL},
  {"Execute macro", &xmPushButtonGadgetClass,
    ' ', "Ctrl<Key>E", "Ctrl+E", False, NULL, ExecuteMacro, "menu", NULL},
 MENUSEPARATOR,
#endif
  {"Average all scans", &xmCascadeButtonGadgetClass,
    ' ', NULL, NULL, True, NULL, NULL, NULL, AverageTypeMenuData},
  {"Average scans over positions", &xmCascadeButtonGadgetClass,
    ' ', NULL, NULL, True, NULL, NULL, NULL, AveragePosTypeMenuData},
  {"Join all scans (RMS)", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, AverageScans, "J rms", NULL},
  {"Add all scans", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, AverageScans, "Aadd", NULL},
  {"Subtract scans sequentially", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, AverageScans, "S", NULL},
 MENUSEPARATOR,
  {"Invert scans", &xmPushButtonGadgetClass,
    'i', "Alt<Key>I", "Alt+I", False, NULL, invert_scans, "invert", NULL},
  {"Fold scans...", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, FoldSelect, "fold", NULL},
  {"Scale & bias scans...", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, scale_scans, "scale", NULL},
  {"Clip scans...", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, DoClipScans, "clip", NULL},
  {"Smooth scans...", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, smooth_scans, "smooth", NULL},
  {"Redres scans...", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, redres_scans, "smooth", NULL},
 MENUSEPARATOR,
  {"Regrid scans...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_spectra, "Regrid", NULL},
  {"Project cube...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_spectra, "Project", NULL},
  {"Convolve scans...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NULL, NULL, SpeConvolveMenuData},
 MENUSEPARATOR,
  {"FFT scans", &xmCascadeButtonGadgetClass,
    ' ', NULL, NULL, True, NULL, NULL, NULL, FFTTypeMenuData},
  {"Swap Re&Im or Ampl&Phase", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, swap_spectrum_and_rms, "", NULL},
 MENUSEPARATOR,
  {"Remove all modifications", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, mod_reset, "all", NULL},
  {"Remove last modifications", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, mod_reset, "latest", NULL},
  {"Channel modification", &xmPushButtonGadgetClass,
    ' ', NULL, NULL, False, NULL, channel_mod, "Channel", NULL},
EOI};

/*** Data Menu Items ***/

MenuItem DataSetManipulateMenuData[] = {
  {"Show data set...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "Show", NULL},
  {"Rename data set(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "Rename", NULL},
  {"Filter data set(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "Filter", NULL},
  {"Sequentiate data set(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "Seq", NULL},
  {"Unsequentiate data set(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "Unseq", NULL},
  {"Merge data sets...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "Merge", NULL},
  {"Merge RMS and data scan...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "RMSmerge", NULL},
  {"Subtract ref. data set...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "Subtract", NULL},
  {"Subtr. & divide ref. data set...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "Sub+div", NULL},
 MENUSEPARATOR,
  {"Delete data set(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "Delete", NULL},
EOI};

/*** Scan Menu Items ***/

MenuItem ScanManipulateMenuData[] = {
  {"List scans...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "ScanList", NULL},
  {"Show scan...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "ScanShow", NULL},
  {"Select scan(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "ScanSelect", NULL},
  {"Delete scan(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_datasets, "ScanDelete", NULL},
EOI};

MenuItem DataMenuData[] = {
  {"Shift emission velocity...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostVLSRDialog, "vshift", NULL},
  {"Set image freq. scale...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostImageFreqDialog, "fimshift", NULL},
  {"Shift x-scale...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostShiftDialog, "shift", NULL},
  {"Change velocity scale...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostVelShiftDialog, "vshift", NULL},
  {"Change coordinates...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostCoordDialog, "change", NULL},
  {"Precess coordinates...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostPreDialog, "precess", NULL},
  {"Manipulate scans", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, ScanManipulateMenuData},
 MENUSEPARATOR,
  {"Attach boxes to data set", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, AttachBoxesToDataset, "attach", NULL},
  {"Detach boxes from data set", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, AttachBoxesToDataset, "deattach", NULL},
 MENUSEPARATOR,
  {"Manipulate data sets", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, DataSetManipulateMenuData},
 MENUSEPARATOR,
  {"Current data sets", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, get_listlist, SelectDataSet, NULL, NULL},
EOI};

/*** Map Menu Items ***/

MenuItem MapScatterMenuData[] = {
  {"Use radial distance as x-unit", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ViewCurrentMapAsScatterPlot, "Distance", NULL},
  {"Use x-axis as x-unit", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ViewCurrentMapAsScatterPlot, "x-axis", NULL},
  {"Use y-axis as x-unit", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ViewCurrentMapAsScatterPlot, "y-axis", NULL},
  {"Use position angle as x-unit", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ViewCurrentMapAsScatterPlot, "PosAngle", NULL},
  {"Use distance along polyline", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ViewCurrentMapAsScatterPlot, "PolyLine", NULL},
EOI};

MenuItem MapConvolveMenuData[] = {
  {"Use antenna temp. unit", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "ConvolveTant", NULL},
  {"Use Jy/beam unit", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "ConvolveJansky", NULL},
EOI};

MenuItem MapBlankMenuData[] = {
  {"Blank data < 3sigma in map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Blank", NULL},
  {"Unblank data < 3sigma in map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Unblank", NULL},
 MENUSEPARATOR,
  {"Blank pixels inside polylines", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "BlankInside", NULL},
  {"Unblank pixels inside polylines", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "UnblankInside", NULL},
 MENUSEPARATOR,
  {"Blank pixels outside polylines", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "BlankOutside", NULL},
  {"Unblank pixels outside polylines", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "UnblankOutside", NULL},
EOI};

MenuItem MapAveMenuData[] = {
  {"Avg in x-dir inside polylines in map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "AveXinside", NULL},
  {"Avg in y-dir inside polylines in map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "AveYinside", NULL},
  {"Avg in x-dir outside polylines in map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "AveXoutside", NULL},
  {"Avg in y-dir outside polylines in map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "AveYoutside", NULL},
EOI};

MenuItem MapManipulateMenuData[] = {
  {"Show map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Show", NULL},
  {"Show two maps overlayed...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Overlay", NULL},
  {"Rename map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Rename", NULL},
  {"Copy map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Copy", NULL},
  {"Change map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Change", NULL},
  {"Combine two maps...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Combine", NULL},
  {"Precess map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Precess", NULL},
  {"Blank & unblank data", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NULL, NULL, MapBlankMenuData},
  {"Swap data & error in map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Swap", NULL},
  {"FFT shift data in map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Shift", NULL},
  {"Forward FFT of map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "FFT", NULL},
  {"Inverse FFT of map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "iFFT", NULL},
#ifdef APEX
  {"Holo correct data in map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "NearF", NULL},
  {"Holo fit phase data in map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "HoloF", NULL},
#endif
  {"Replace data with S/N in map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "S/N", NULL},
  {"Merge data and error maps...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Merge", NULL},
  {"Edit beam in map...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Edit beam", NULL},
  {"Average maps in one dimension", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NULL, NULL, MapAveMenuData},
  {"Scale map...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Scale", NULL},
  {"Interpolate map...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Interpolate", NULL},
  {"Regrid map...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Regrid", NULL},
  {"Convolve map...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NULL, NULL, MapConvolveMenuData},
  {"Correlate map...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Correlate", NULL},
  {"MEM map...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "MEM", NULL},
  {"Map arithmetic...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Arithmetic", NULL},
 MENUSEPARATOR,
  {"Delete map(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Delete", NULL},
EOI};

MenuItem MapsMenuData[] = {
  {"Store current map...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, store_current_map, "store", NULL},
  {"Store multiple maps...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, StoreMultipleMaps, "store", NULL},
  {"Create a test map...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostTestMapDialog, "", NULL},
  {"Save maps as FITS cube...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_maps, "Cube", NULL},
  {"View current map as scatter plot", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, NULL, NULL, MapScatterMenuData},
 MENUSEPARATOR,
  {"Attach contour/grey data to map", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, AttachContToMap, "attach", NULL},
  {"Detach contour/grey data from map", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, AttachContToMap, "deattach", NULL},
 MENUSEPARATOR,
  {"Manipulate maps", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, MapManipulateMenuData},
 MENUSEPARATOR,
  {"Current maps", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, get_maplist, MapDraw, NULL, NULL},
EOI};


/*** Scatter Menu Items ***/
CallbackData SMD_tagging = {NULL, "tagscatterpts", &view.tag_scatters, NULL};

MenuItem ScatterTaggedMenuData[] = {
  {"Unselect all points", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScatterPointOps, "untag", NULL},
  {"Invert selection", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScatterPointOps, "invert", NULL},
 MENUSEPARATOR,
  {"New plot of selected points", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScatterPointOps, "select_tagged", NULL},
  {"New plot of unselected points", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScatterPointOps, "select_untagged", NULL},
 MENUSEPARATOR,
  {"New scans of selected points", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScatterPointOps, "select_tagged_as_scans", NULL},
  {"New scans of unselected points", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ScatterPointOps, "select_untagged_as_scans", NULL},
EOI};

MenuItem ScatterManipulateMenuData[] = {
  {"Show plots(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "Show", NULL},
  {"Rename plots(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "Rename", NULL},
  {"Copy plot(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "Copy", NULL},
  {"Swap x & y in plot(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "Swap", NULL},
  {"Sort x in plot(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "Sort", NULL},
  {"Scale plot(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "Scale", NULL},
  {"Invert y-scale in plot(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "InvertY", NULL},
  {"Bin plot(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "Bin", NULL},
  {"Merge & clip plot(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "Merge", NULL},
  {"Derive plot(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "d/dx", NULL},
  {"Forward FFT of plot(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "FFT", NULL},
  /* {"Inverse FFT of plot(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "iFFT", NULL}, */
  {"Plot arithmetic...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "Arithmetic", NULL},
 MENUSEPARATOR,
  {"Make pointing residuals of plot(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "poires", NULL},
 MENUSEPARATOR,
  {"Delete plot(s)...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, manipulate_scats, "Delete", NULL},
EOI};

MenuItem ScatterMenuData[] = {
  {"Store current scatter plot...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, StoreCurrentScatter, "store", NULL},
  {"View current spectrum as scatter plot...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MakeSingleSpeScatterPlot, "single", NULL},
  {"View current spectra as scatter plot...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostScatterTypeDialog, NULL, NULL},
  {"Set scatter plot options...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, PostScatterOptionDialog, "post", NULL},
 MENUSEPARATOR,
  {"Select scatter points", &xmToggleButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, toggle_any, (XtPointer)&SMD_tagging, NULL},
  {"Operate on sel. points", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, ScatterTaggedMenuData},
 MENUSEPARATOR,
  {"Manipulate scatter plots", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, ScatterManipulateMenuData},
 MENUSEPARATOR,
  {"Current scatter plot", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, get_scatterlist, ScatterDraw, NULL, NULL},
EOI};


/*** Preferences Menu Items ***/

MenuItem EditPrefsMenuData[] = {
  {"Data files...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, EditPrefsFile, "DataPrefs", NULL},
  {"Unix cmds...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, EditPrefsFile, "UnixPrefs", NULL},
  {"Freq. files...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, EditPrefsFile, "FreqfilePrefs", NULL},
  {"Window sizes...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, EditPrefsFile, "WindowPrefs", NULL},
  {"Astronomical info...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, EditPrefsFile, "AstroPrefs", NULL},
  {"Other prefs...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, EditPrefsFile, "OtherPrefs", NULL},
#ifdef APEX
  {"Holographic data setup...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, EditHoloData,  "HoloSetup", NULL},
  {"Holographic fit setup...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, EditHoloFit,   "HoloSetup", NULL},
  {"APEX map setup...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, EditAPEXMap,   "MapSetup", NULL},
#endif
EOI};

MenuItem PrefsMenuData[] = {
  {"View preferences in ~/.xsrc...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ViewPrefsFile, "view", NULL},
  {"Edit preferences", &xmCascadeButtonGadgetClass,
   ' ', NULL, NULL, True, NULL, NULL, NULL, EditPrefsMenuData},
 MENUSEPARATOR,
  {"Save preferences", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, MenuSavePrefsFile, "store", NULL},
EOI};


/*** Help Menu Items ***/

MenuItem HelpMenuData[] = {
  {"Mouse buttons...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "MouseButtons", NULL},
  {"Keyboard accelerators...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "AccelKeys", NULL},
 MENUSEPARATOR,
  {"File menu...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "FileMenu", NULL},
  {"Graph menu...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "GraphMenu", NULL},
  {"Gauss menu...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "GaussMenu", NULL},
  {"Baseline menu...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "BaselineMenu", NULL},
  {"Moment menu...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "MomentMenu", NULL},
  {"Marker menu...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "MarkerMenu", NULL},
  {"Reduction menu...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "ReductionMenu", NULL},
  {"Data sets menu...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "DatasetsMenu", NULL},
  {"Maps menu...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "MapsMenu", NULL},
  {"Scatter menu...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "ScatterMenu", NULL},
  {"Prefs menu...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "PrefsMenu", NULL},
  {"Help menu...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Help, "HelpMenu", NULL},
 MENUSEPARATOR,
  {"About...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, About, "", NULL},
  {"Version...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, Version, "", NULL},
  {"License...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ReadMe, "", NULL},
 MENUSEPARATOR,
  {"What's new...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, WhatsNew, "", NULL},
  {"Problems & bugs...", &xmPushButtonGadgetClass,
   ' ', NULL, NULL, False, NULL, ProblemsAndBugs, "", NULL},
EOI};


/*** Menu-Bar Items ***/

MenuBarItem MenuBarData[] = {
  {"File",      'F', True, FileMenuData},
  {"Graph",     'G', True, GraphMenuData},
  {"Gauss",     'a', True, GaussMenuData},
  {"Baseline",  'B', True, BaselineMenuData},
  {"Moment",    'M', True, MomentMenuData},
  {"Marker",    'r', True, MarkerMenuData},
  {"Reduction", 'R', True, ReductionMenuData},
  {"Data sets", 'D', True, DataMenuData},
  {"Maps",      'p', True, MapsMenuData},
  {"Scatter",   'S', True, ScatterMenuData},
  {"Prefs",     's', True, PrefsMenuData},
  {"Help",      'H', True, HelpMenuData},
EOI};
