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

#include <X11/IntrinsicP.h>
#include <Xm/MessageB.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>

#include "defines.h"
#include "global_structs.h"

/*** External variables ***/
extern GLOBAL *gp;

/*** Local variables ***/
static char *about_str = "\
                    " PKGNAME "\n\
Spectral Line Reduction Package for Astronomy\n\n\
                  Author\n\
                P. Bergman\n\
             pbergman@chalmers.se\n\n\
          Onsala Space Observatory\n\
                    at\n\
       Chalmers University of Technology\n\
             Gothenburg, Sweden\n\n\n\
XS sources and some executables can be found at\n\
    ftp://yggdrasil.oso.chalmers.se/pub/xs";
                  
static char *readme_str = "\
                           XS License\n\n\
Copyright (C) 2000-2012  P. Bergman\n\n\
This program is free software; you can redistribute it and/or modify it under\n\
the terms of the GNU General Public License as published by the Free Software\n\
Foundation; either version 2 of the License, or (at your option) any later\n\
version.\n\n\
This program is distributed in the hope that it will be useful, but WITHOUT\n\
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS\n\
FOR A PARTICULAR PURPOSE. See the GNU General Public License below for more\n\
details.\n\n\n\
GNU GENERAL PUBLIC LICENSE\n\
\n\
Version 2, June 1991\n\
\n\
Copyright (C) 1989, 1991 Free Software Foundation, Inc. 675 Mass Ave,\n\
Cambridge, MA 02139, USA. Everyone is permitted to copy and distribute verbatim\n\
copies of this license document, but changing it is not allowed.\n\
\n\
Preamble\n\
\n\
The licenses for most software are designed to take away your freedom to share\n\
and change it. By contrast, the GNU General Public License is intended to\n\
guarantee your freedom to share and change free software--to make sure the\n\
software is free for all its users. This General Public License applies to most\n\
of the Free Software Foundation's software and to any other program whose\n\
authors commit to using it. (Some other Free Software Foundation software is\n\
covered by the GNU Library General Public License instead.) You can apply it to\n\
your programs, too.\n\
\n\
When we speak of free software, we are referring to freedom, not price. Our\n\
General Public Licenses are designed to make sure that you have the freedom to\n\
distribute copies of free software (and charge for this service if you wish),\n\
that you receive source code or can get it if you want it, that you can change\n\
the software or use pieces of it in new free programs; and that you know you\n\
can do these things.\n\
\n\
To protect your rights, we need to make restrictions that forbid anyone to deny\n\
you these rights or to ask you to surrender the rights. These restrictions\n\
translate to certain responsibilities for you if you distribute copies of the\n\
software, or if you modify it.\n\
\n\
For example, if you distribute copies of such a program, whether gratis or for\n\
a fee, you must give the recipients all the rights that you have. You must make\n\
sure that they, too, receive or can get the source code. And you must show them\n\
these terms so they know their rights.\n\
\n\
We protect your rights with two steps: (1) copyright the software, and (2)\n\
offer you this license which gives you legal permission to copy, distribute\n\
and/or modify the software.\n\
\n\
Also, for each author's protection and ours, we want to make certain that\n\
everyone understands that there is no warranty for this free software. If the\n\
software is modified by someone else and passed on, we want its recipients to\n\
know that what they have is not the original, so that any problems introduced\n\
by others will not reflect on the original authors' reputations.\n\
\n\
Finally, any free program is threatened constantly by software patents. We wish\n\
to avoid the danger that redistributors of a free program will individually\n\
obtain patent licenses, in effect making the program proprietary. To prevent\n\
this, we have made it clear that any patent must be licensed for everyone's\n\
free use or not licensed at all.\n\
\n\
The precise terms and conditions for copying, distribution and modification\n\
follow.\n\
\n\
GNU GENERAL PUBLIC LICENSE TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND\n\
MODIFICATION\n\
\n\
0. This License applies to any program or other work which contains a notice\n\
placed by the copyright holder saying it may be distributed under the terms of\n\
this General Public License. The \"Program\", below, refers to any such program\n\
or work, and a \"work based on the Program\" means either the Program or any\n\
derivative work under copyright law: that is to say, a work containing the\n\
Program or a portion of it, either verbatim or with modifications and/or\n\
translated into another language. (Hereinafter, translation is included without\n\
limitation in the term \"modification\".) Each licensee is addressed as \"you\".\n\
\n\
Activities other than copying, distribution and modification are not covered by\n\
this License; they are outside its scope. The act of running the Program is not\n\
restricted, and the output from the Program is covered only if its contents\n\
constitute a work based on the Program (independent of having been made by\n\
running the Program). Whether that is true depends on what the Program does.\n\
\n\
1. You may copy and distribute verbatim copies of the Program's source code as\n\
you receive it, in any medium, provided that you conspicuously and\n\
appropriately publish on each copy an appropriate copyright notice and\n\
disclaimer of warranty; keep intact all the notices that refer to this License\n\
and to the absence of any warranty; and give any other recipients of the\n\
Program a copy of this License along with the Program.\n\
\n\
You may charge a fee for the physical act of transferring a copy, and you may\n\
at your option offer warranty protection in exchange for a fee.\n\
\n\
2. You may modify your copy or copies of the Program or any portion of it, thus\n\
forming a work based on the Program, and copy and distribute such modifications\n\
or work under the terms of Section 1 above, provided that you also meet all of\n\
these conditions:\n\
\n\
a) You must cause the modified files to carry prominent notices stating that\n\
you changed the files and the date of any change.\n\
\n\
b) You must cause any work that you distribute or publish, that in whole or in\n\
part contains or is derived from the Program or any part thereof, to be\n\
licensed as a whole at no charge to all third parties under the terms of this\n\
License.\n\
\n\
c) If the modified program normally reads commands interactively when run, you\n\
must cause it, when started running for such interactive use in the most\n\
ordinary way, to print or display an announcement including an appropriate\n\
copyright notice and a notice that there is no warranty (or else, saying that\n\
you provide a warranty) and that users may redistribute the program under these\n\
conditions, and telling the user how to view a copy of this License.\n\
(Exception: if the Program itself is interactive but does not normally print\n\
such an announcement, your work based on the Program is not required to print\n\
an announcement.)\n\
\n\
These requirements apply to the modified work as a whole. If identifiable\n\
sections of that work are not derived from the Program, and can be reasonably\n\
considered independent and separate works in themselves, then this License, and\n\
its terms, do not apply to those sections when you distribute them as separate\n\
works. But when you distribute the same sections as part of a whole which is a\n\
work based on the Program, the distribution of the whole must be on the terms\n\
of this License, whose permissions for other licensees extend to the entire\n\
whole, and thus to each and every part regardless of who wrote it.\n\
\n\
Thus, it is not the intent of this section to claim rights or contest your\n\
rights to work written entirely by you; rather, the intent is to exercise the\n\
right to control the distribution of derivative or collective works based on\n\
the Program.\n\
\n\
In addition, mere aggregation of another work not based on the Program with the\n\
Program (or with a work based on the Program) on a volume of a storage or\n\
distribution medium does not bring the other work under the scope of this\n\
License.\n\
\n\
3. You may copy and distribute the Program (or a work based on it, under\n\
Section 2) in object code or executable form under the terms of Sections 1 and\n\
2 above provided that you also do one of the following:\n\
\n\
a) Accompany it with the complete corresponding machine-readable source code,\n\
which must be distributed under the terms of Sections 1 and 2 above on a medium\n\
customarily used for software interchange; or,\n\
\n\
b) Accompany it with a written offer, valid for at least three years, to give\n\
any third party, for a charge no more than your cost of physically performing\n\
source distribution, a complete machine-readable copy of the corresponding\n\
source code, to be distributed under the terms of Sections 1 and 2 above on a\n\
medium customarily used for software interchange; or,\n\
\n\
c) Accompany it with the information you received as to the offer to distribute\n\
corresponding source code. (This alternative is allowed only for noncommercial\n\
distribution and only if you received the program in object code or executable\n\
form with such an offer, in accord with Subsection b above.)\n\
\n\
The source code for a work means the preferred form of the work for making\n\
modifications to it. For an executable work, complete source code means all the\n\
source code for all modules it contains, plus any associated interface\n\
definition files, plus the scripts used to control compilation and installation\n\
of the executable. However, as a special exception, the source code distributed\n\
need not include anything that is normally distributed (in either source or\n\
binary form) with the major components (compiler, kernel, and so on) of the\n\
operating system on which the executable runs, unless that component itself\n\
accompanies the executable.\n\
\n\
If distribution of executable or object code is made by offering access to copy\n\
from a designated place, then offering equivalent access to copy the source\n\
code from the same place counts as distribution of the source code, even though\n\
third parties are not compelled to copy the source along with the object code.\n\
\n\
4. You may not copy, modify, sublicense, or distribute the Program except as\n\
expressly provided under this License. Any attempt otherwise to copy, modify,\n\
sublicense or distribute the Program is void, and will automatically terminate\n\
your rights under this License. However, parties who have received copies, or\n\
rights, from you under this License will not have their licenses terminated so\n\
long as such parties remain in full compliance.\n\
\n\
5. You are not required to accept this License, since you have not signed it.\n\
However, nothing else grants you permission to modify or distribute the Program\n\
or its derivative works. These actions are prohibited by law if you do not\n\
accept this License. Therefore, by modifying or distributing the Program (or\n\
any work based on the Program), you indicate your acceptance of this License to\n\
do so, and all its terms and conditions for copying, distributing or modifying\n\
the Program or works based on it.\n\
\n\
6. Each time you redistribute the Program (or any work based on the Program),\n\
the recipient automatically receives a license from the original licensor to\n\
copy, distribute or modify the Program subject to these terms and conditions.\n\
You may not impose any further restrictions on the recipients' exercise of the\n\
rights granted herein. You are not responsible for enforcing compliance by\n\
third parties to this License.\n\
\n\
7. If, as a consequence of a court judgment or allegation of patent\n\
infringement or for any other reason (not limited to patent issues), conditions\n\
are imposed on you (whether by court order, agreement or otherwise) that\n\
contradict the conditions of this License, they do not excuse you from the\n\
conditions of this License. If you cannot distribute so as to satisfy\n\
simultaneously your obligations under this License and any other pertinent\n\
obligations, then as a consequence you may not distribute the Program at all.\n\
For example, if a patent license would not permit royalty-free redistribution\n\
of the Program by all those who receive copies directly or indirectly through\n\
you, then the only way you could satisfy both it and this License would be to\n\
refrain entirely from distribution of the Program.\n\
\n\
If any portion of this section is held invalid or unenforceable under any\n\
particular circumstance, the balance of the section is intended to apply and\n\
the section as a whole is intended to apply in other circumstances.\n\
\n\
It is not the purpose of this section to induce you to infringe any patents or\n\
other property right claims or to contest validity of any such claims; this\n\
section has the sole purpose of protecting the integrity of the free software\n\
distribution system, which is implemented by public license practices. Many\n\
people have made generous contributions to the wide range of software\n\
distributed through that system in reliance on consistent application of that\n\
system; it is up to the author/donor to decide if he or she is willing to\n\
distribute software through any other system and a licensee cannot impose that\n\
choice.\n\
\n\
This section is intended to make thoroughly clear what is believed to be a\n\
consequence of the rest of this License.\n\
\n\
8. If the distribution and/or use of the Program is restricted in certain\n\
countries either by patents or by copyrighted interfaces, the original\n\
copyright holder who places the Program under this License may add an explicit\n\
geographical distribution limitation excluding those countries, so that\n\
distribution is permitted only in or among countries not thus excluded. In such\n\
case, this License incorporates the limitation as if written in the body of\n\
this License.\n\
\n\
9. The Free Software Foundation may publish revised and/or new versions of the\n\
General Public License from time to time. Such new versions will be similar in\n\
spirit to the present version, but may differ in detail to address new problems\n\
or concerns.\n\
\n\
Each version is given a distinguishing version number. If the Program specifies\n\
a version number of this License which applies to it and \"any later version\",\n\
you have the option of following the terms and conditions either of that\n\
version or of any later version published by the Free Software Foundation. If\n\
the Program does not specify a version number of this License, you may choose\n\
any version ever published by the Free Software Foundation.\n\
\n\
10. If you wish to incorporate parts of the Program into other free programs\n\
whose distribution conditions are different, write to the author to ask for\n\
permission. For software which is copyrighted by the Free Software Foundation,\n\
write to the Free Software Foundation; we sometimes make exceptions for this.\n\
Our decision will be guided by the two goals of preserving the free status of\n\
all derivatives of our free software and of promoting the sharing and reuse of\n\
software generally.\n\
\n\
NO WARRANTY\n\
\n\
11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY FOR\n\
THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE\n\
STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE\n\
PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,\n\
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND\n\
FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND\n\
PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU\n\
ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.\n\
\n\
12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL\n\
ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR REDISTRIBUTE THE\n\
PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY\n\
GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR\n\
INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA\n\
BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A\n\
FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER\n\
OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.\n\
\n\
END OF TERMS AND CONDITIONS\n\
\n\
";

static char *problem_str = "\
Since this software currently is in a major development phase there\n\
are, most likely, several problems and/or bugs.\n\
Please report them to the author.\n\n\
Known problems:                        | Status:\n\
------------------------------------------------------------------------------\n\
    (1) No written manual              | When the time permits.\n\
    (2) Contour lines do not work      |\n\
        properly when displaying maps  |\n\
        with orthographic projections  |\n\
------------------------------------------------------------------------------\n\
";

static char *news_str = "\
                         Most recent additions\n\
------------------------------------------------------------------------------\n\
 Date      Addition\n\
------------------------------------------------------------------------------\n\
980630 Map projections added in the contour dialog.\n\
980701 Now possible to filter the scans using the system temperature.\n\
       What's new (this help).\n\
       Hiding sliders in contour level dialog.\n\
       Drawing ellipses or circles when fitting 2-dim Gaussians.\n\
980702 Blanking (or unblanking) of map data inside/outside of closed\n\
       polylines.\n\
       Map sigma/area/luminosity are displayed in the baseline/moment\n\
       trackers to the left. The values correspond to the emission\n\
       inside all closed polylines. The values are only displayed when\n\
       the map is shown using gray or false color pixels.\n\
980728 It is now possible to add/subtract and reset individual Gaussian\n\
       fits to all spectra in a map. See the Gauss menu.\n\
980803 Added the left/right orthographic map projection option.\n\
980806 Prepared for Help buttons in dialogs.\n\
980814 Added options for maximum, minimum, and RMS intensity in the\n\
       dialog for map types.\n\
980817 Added Lorentzian fit for scatter plots.\n\
980821 Fixed a bug reading binary FITS tables.\n\
       Added several help buttons.\n\
       New version: 1.01\n\
980828 Fixed consistency with DRP coordinate system, CSystem.\n\
       Added support for reading JPL frequency file for the range\n\
       125-165 GHz.\n\
980904 Made it possible to change the number of plots in the x-direction\n\
       also for multiple map plots.\n\
980905 It's now possible to swap locations (using middle button) of map\n\
       plots when showing several map plots simultaneously.\n\
980907 Added an option to select the number of digits in the contour\n\
       labels.\n\
980917 Fixed so that older DRP format could be converted properly.\n\
980922 It is now possible to show multiple scatter plots (both on a grid\n\
       and as overlays). It it also possible to swap locations of the\n\
       plots using the middle button of scatter plots similar to spectra\n\
       and maps. Fixed some minor bugs for multiple plots in general.\n\
       New version: 1.02\n\
981005 Scan numbers are now kept when saving multiple FITS files.\n\
981017 Added new map task: scale map. It makes it possible to scale the\n\
       axes as well as the data.\n\
981023 Added info on resolution in the Current Data Info Header.\n\
981026 It is now possible to scroll the data window using the arrow keys.\n\
981027 Now marker labels will not get printed if outside the viewing area.\n\
981105 Added the possibility to fit the first few Fourier terms to a\n\
       scatter plot. Increased the max. order of polynomials as well.\n\
981110 It is now possible to fit a sin curve simultaneously with a\n\
       standard polynomial when fitting baselines. This may be useful when\n\
       removing standing wave ripples in spectra. A new menu item was also\n\
       added: 'Edit current coefficients' which enables the user to modify\n\
       the fitting parameters via a dialog. New version: 1.03\n\
981111 It is now possible to averaging and join spectra with different\n\
       extents. Note, however, that the resolutions must be same.\n\
       Further help buttons were also added (in smooth and redres).\n\
981113 In the dialog invoked by 'Edit moment box with cursor' it is now\n\
       possible to move the box using a timer (i.e. create a 'movie').\n\
       It useful to quickly display maps as a function of the position\n\
       of the moment box.\n\
981125 It is now possible to use the cancel button in the file open dialogs\n\
       during file listing/filtering/sorting without crashing the program.\n\n\
981214          *********** New version 1.10 ***********\n\
       Version 1.10 is quite different from previous versions (1.03 and\n\
       earlier). The main difference is that scans are now being allocated\n\
       memory in accord with their actual size and not to a fixed number\n\
       of channels. This will make it possible to read large data sets (e.g.\n\
       cubes) without having to compile special versions of the package.\n\
       Another large difference is that data read will be stored into\n\
       data sets. That is; single scans, sequences or multiple scans will\n\
       be selectable from a new menu (called 'Data sets') in a way maps or\n\
       scatter plots used to work. Several commands working on data sets\n\
       have been added. Moreover, most tasks working on scans (e.g. FFT,\n\
       averaging, smoothing, fold etc.) will create a new data set so that\n\
       the original data set is left untouched. A few tasks will still\n\
       work directly on the current data set. These are: subtracting/\n\
       adding baselines, subtracting/adding gaussians, coordinates changes,\n\
       inverting.\n\
981215 It is now possible to attach/detach baseline and moment boxes to\n\
       different data sets.\n\
981222 Added a task that interpolates the data within moment boxes using\n\
       the polynomial fit of the data in the baseline boxes. To the\n\
       interpolated values also an RMS noise is added with a magnitude\n\
       equal to the noise in the baseline boxes. This task is useful\n\
       when removing ripple in the Fourier-plane, where ripples usually\n\
       show up as peaks.\n\
981228          *********** New version 1.11 ***********\n\
       It possible to clear all boxes (both baseline and moment boxes)\n\
       from the keyboard by pressing 'A' while the cursor is in the graph\n\
       window.\n\
990114 Fixed a bug that made it impossible to add and/or subtract selected\n\
       gaussians (in single spectrum mode).\n\
990115 Added a new \"graph option\" to show a header above a spectrum.\n\
       The header contains information about the spectrum.\n\
990116 Compiled with the latest Lesstif package (version 0.87.1, see\n\
       http://www.lesstif.org/). XS compiles nicely. Most functions seem\n\
       to work, although there are some small problems in the layout of\n\
       menus (e.g. menu accelerators are not shown properly) as well as\n\
       the layout of menu options. They are blank at first, i.e. the default\n\
       values are not shown. It seems as if Lesstif 0.87.1 can be used.\n\
990117          *********** New version 1.12 ***********\n\
       Dot markers, used in maps and scatter plots, can now be set\n\
       individually for maps and scatter plots, respectively. The button\n\
       for invoking the dialog to set type of dot and size of dot can be\n\
       found under the 'Graph menu'.\n\
990121 Added the option to fit Gaussians in a relative way. This means\n\
       that the second and subsequent Gaussians are always given relative\n\
       to the first entered Gaussian. See under the 'Gauss menu'.\n\
       Moreover, it is now possible to redefine the bindings of the\n\
       mouse buttons when used on the graphical window, see under the\n\
       menu 'Prefs'->'Edit preferences'->'Other prefs...'. This comes\n\
       in handy if you lack a three button mouse.\n\
990127 Fixed a bug that caused xs to crash sometimes when reading a FITS\n\
       cube.\n\
       The number of channels is not limited upwards anylonger (apart\n\
       from memory limitations of course).\n\
990128          *********** New version 1.13 ***********\n\
990129 Position offsets are now correctly shown in labels and listings\n\
       when the scans are read in sequential mode.\n\
990204 It is now possible to specify a memory warning limit from the menu\n\
       'Prefs'->'Edit preferences'->'Other prefs...'. If the read data\n\
       occupy more than this limit the background of the label displaying\n\
       the amount of read data (in Mb, in the 'Current data information'\n\
       display), the background of the label will turn red.\n\
       Adding polyline options under the 'Moment' menu. Unfortunately,\n\
       there is no space for a separate 'Polyline' pulldown menu. It is\n\
       now possible to blank grey scale or false color pixels inside\n\
       closed polylines directly within the 'Contour levels...' popup\n\
       dialog. Contour lines are unaffected (for now).\n\
990305          *********** New version 1.14 ***********\n\
       Apart from a few small bugfixes and minor layout fixes, the changes\n\
	   in version 1.14 are not visible. Almost all dialogs have been\n\
	   rewritten to use standard routines (see dialogs.c).\n\
990307 Added the possibility to calculate the covariance (or correlation)\n\
       function (as a funtion of the distance) of a map.\n\
       Possible to FFT a scatter plot (of course, this is only useable for\n\
       scatter plots with equidistant x-values.\n\
990314 Fixed a bug that prevented polylines to be removed properly from under\n\
       the 'Moment' menu. The option 'Remove polyline'->'All' was causing\n\
       the program to crash occasionally.\n\
990316 Made the map scaling more general. Tidied up code related to map\n\
       rotation. The wedge can now be put in four different locations.\n\
990317 Fixed a bug that prevented gaussians to be saved as a file. Also\n\
       modified code related to the reading of gaussians.\n\
990318          *********** New version 1.15 ***********\n\
       Cleaned up the code dealing with scans and data sets. The JPL\n\
       frequency files now have different names. Support for reading\n\
       JPL spectral line catalog from 15 to 600 GHz has been added.\n\
990325 Better handling of command line arguments, see xs -help\n\
990326 Grey scale and false colors now follow non-linear spaced contours.\n\
990328 It is now possible to tag (select) points in a scatter plot, see under\n\
       'Scatter'. Certain operations can be performed on the selected points.\n\
       Especially useful is that if the scatter plot was made from scans, a\n\
       new data set consisting of the scans corresponding to the selected\n\
       points can be created.\n\
990409          *********** New version 1.16 ***********\n\
       Removed inconsitencies resulting from the definition of the azimuth\n\
       angle used at OSO (Az=0 at south) whereas the normal definition uses\n\
       Az=0 in the northern direction. Also added support for using xs in\n\
       server mode (start with 'xs -s'). In server mode xs is sensitive\n\
       to certain properties (see autoread.h) set in the X-server. Entire\n\
       FITS files or messages can be sent to xs in this way, both over a\n\
       remote link or locally.\n\
990412 Cleaned up the server code.\n\
990416 Fixed a bug that caused FITS cubes to be saved improperly if the\n\
       number of channels were reduced from the original no of channels when\n\
       the data were read.\n\
990417          *********** New version 1.17 ***********\n\
990610 Added the possibility to change the velocity scale for spectra from\n\
       LSR scale to heliocentric scale (and vice versa). See under menu\n\
       'Data sets'.\n\
990826 Fixed a bug when reading DRP scans on HP architectures. Also, if a\n\
       DRP scan is byte-swapped (i.e. saved on a machine with different\n\
       byte order) it will now be read correctly.\n\
990908 Added some support for multi-threading in the MEM deconvolution\n\
       calculations.\n\
990908 Checking for NaN as blank values in FITS files (float format).\n\
991021 Fixed a conflict when both NaN values and the BLANK keyword are\n\
       present in a (float formatted) FITS file. Now NaN values will take\n\
       precedence over the BLANK keyword.\n\
991031 Added a secondary top label. Made the location of the special inner\n\
       frame more general when plotting multiple spectra. Now it can be\n\
       chosen from the top right, top left, bottom right, or bottom left\n\
       locations. See 'Graph'->'Set size of multiple plot...'. The special\n\
       inner frame options are now also available for multiple maps and\n\
       multiple scatter plots.\n\
991103          *********** New version 1.18 ***********\n\
       Added a special option to set the appearance of the position markers\n\
       in the PostScript window.\n\
991105 If no .xsrc is found in the home directory xs now tries to read a\n\
       file XS_LIB/.xsrc if the environment variable XS_LIB is set. Also,\n\
       the frequency files should reside in the XS_LIB directory. Other\n\
       locations can be specified in the .xsrc file in the home directory\n\
       of the user.\n\
2000-01-11\n\
       Made it possible to divide scans with the main beam efficiency stored\n\
       in the header (OSO and, possibly, SEST data only).\n\
2000-03-01      *********** New version 1.19 ***********\n\
2000-03-03\n\
       Fixed a bug when reading FLOAT or DOUBLE formatted FITS scans. If the\n\
       keywords BZERO and BSCALE where not present in FLOAT or DOUBLE scans\n\
       strange scaling of the data would appear. BZERO and BSCALE are not\n\
       needed (and should not be used) with the FLOAT or DOUBLE format.\n\
       Added the ability to save baseline boxes and moment regions as file.\n\
       The functionality is found under menu 'Baseline'.\n\
       Also fixed a bug that occured when smoothing scans using the boxcar\n\
       method. The bug caused an error in the x-axis scaling.\n\
2000-03-22\n\
       Added better support for data taken in Galactic coordinates. Not\n\
       yet complete.\n\
2000-03-30      *********** New version 1.2.0 ***********\n\
2000-03-30\n\
       XS license is now GPL (see 'Help'->'License').\n\
2000-04-10\n\
       Made it possible to obtain the spectral correlation coeffcient as a\n\
       function of channel difference.\n\
2000-04-13\n\
       It is now possible to precess coordinates (both for maps and spectra).\n\
       Added also the functionality to combine two maps, centered on different\n\
       coordinates, into a single map.\n\
2000-10\n\
       Updated all fitting routines.\n\
2000-11-25\n\
       Removed inconsistencies for reference pixels in FITS files.\n\
2000-12-01\n\
       Fixed a bug that prevented of saving a FITS cube directly after\n\
       reading single-dish scans (without viewing them first).\n\
2000-12-04\n\
       New FFT routines including a 2-dimensional FFT so that maps also\n\
       can be transformed.\n\
2000-12-04      *********** New version 1.2.1 ***********\n\
2000-12-05\n\
       Fixed the fitting routines for scatter plots, the Chi^2 values\n\
       were not updated properly. Added also atmospheric opacity to the\n\
       list of plot options.\n\
2000-12-06\n\
       Added the possibility to break up menus with many items. See\n\
       under menu 'Prefs'->'Edit preferences'->'Other prefs'. This\n\
       will make it easier to select menu items in case of small screens.\n\
2000-12-10\n\
       Improved the appearance of the PS viewer. It is now possible to save\n\
       the state of all PS variables and parameters to a file. This will\n\
       make it easier to restore a PS layout without having to go through\n\
       all the menus manually. The info is stored in the file as binary\n\
       data and can thus not normally be exported to other platforms. It\n\
       may also not work between different version of " PKGNAME ".\n\
2000-12-11      *********** New version 1.2.2 ***********\n\
       The graphical state (except PS parameters) can be saved in a binary\n\
       file. The file can be loaded via a file browser, thus making it easier\n\
       to restore a customized view.\n\
2001-01-25\n\
       Copyrights in each file changed to included 2001. The spectral x-axis\n\
       can now be displayed using MHz as well. See 'Graph'->'Change scales...'.\n\
2001-02-10\n\
       Fixed a bug for the menu breaking. This bug could cause a crash when\n\
       selecting menus with changing items (i.e. menus with scans, maps, or\n\
       scatter plots). Static menus were not affected.\n\
       The possibility of breaking menus with 'more...' items was introduced\n\
       in version 1.2.1.\n\
2001-09-04\n\
       It is now possible to fit and subtract a sinc curve (sin(x)/x) from a\n\
       baseline.\n\
       This was added entirely for ODIN spectra which sometimes show a sinc\n\
       like interference in the AOS.\n\
2001-09-07\n\
       Under manipulate data sets a new functions was added. The first scan\n\
       in one data set can now be used as a reference for another data set.\n\
       The function will subract the reference scan from all scans in the\n\
       other data sets. It is also now possible to calculate the correlation\n\
       between a sequence of scans channelwise. This option is found under\n\
       the 'Reduction'->'FFT data'->'Sequence correlation' menu.\n\
2002-01-23      *********** New version 1.2.5 ***********\n\
       Minor bugfixes, such as " PKGNAME " will not crash during rms averaging even\n\
       if no baseline boxes are used.  Also, when showing a large number of\n\
       map spectra " PKGNAME " will not draw spectra if they will have a tiny\n\
       appearance on the screen. This to avoid the large time it takes to\n\
       draw something one cannot discern anyway.  The size (in pixels) when\n\
       the spectra will be shown can be specified under 'Preferences'.\n\
2002-02-18\n\
       It is now possible to save plots with GIF and PNG formats (provided\n\
       that the corresponding PGPLOT drivers exist).  Also, one can now\n\
       manually select an offset position (in addition to a map spacing) on\n\
       which the map grid will be based, see 'Type of map...'.\n\
2002-02-20      *********** New version 1.2.6 ***********\n\
2002-02-21\n\
       Minor changes in map.c to speed up the drawing of many spectra. It is\n\
       quite an improvement if the number of spectra shown is larger than a\n\
       few hundreds. Also, when shifting spectra the new data set of shifted\n\
       scans become the current data set. Tested to compile against the 2.1\n\
       clone of LessTif 0.93.18. It seems to work ok with just a few minor\n\
       layout differences compared to OpenMotif 2.1.\n\
2002-02-21      *********** New version 1.2.7 ***********\n\
       Added support to read the FITS binary table format used for the Odin\n\
       data. In scatter plots, the statistics of the points can now be\n\
       displayed, see 'Scatter'->'Set scatter plot options...'.\n\
2002-03-07      *********** New version 1.2.8 ***********\n\
2002-08-12\n\
       Fixed a bug when reading FITS cubes with blanked positions.\n\
2002-09-19\n\
       When reading ODIN satellite binary tables, the spectra were read in\n\
       with wrong sign of the frequency channel width in case of data taken\n\
       in lower sideband mode (i.e. LO freq. > Sky freq.). It should now be\n\
       fixed.\n\
2003-01-09\n\
       Changed the SEST coordinates, and made it possible to read Mopra FITS\n\
       files. Also some other minor changes.\n\
2003-01-09      *********** New version 1.2.9 ***********\n\
2003-01-20\n\
       It is now possible to attach an individual scaling for scans in\n\
       multiple plots. See the attach/detach options under the 'Graph' menu.\n\
2003-04-14\n\
       FITS images with BITPIX=8 can now be read. Apparently, some programs\n\
       still use BITPIX=8 (for images).\n\
2003-05-16\n\
       Improved the way of fitting gaussians to several spectra. The editing\n\
       menu now contains more options (use 'e' to get the editing menu when\n\
       you place the cursor on the spectrum in question).\n\
2003-11-24\n\
       Added menu entries to handle marker scaling, see the 'Marker' menu.\n\
2003-12-04\n\
       Made it possible to represent the position axis in velocity-position\n\
       diagrams with RA or Decl. values in addition to the distance. Using\n\
       RA or Decl. as position axis unit works properly only if the polyline\n\
       is a straight line (i.e. only two points).\n\
2004-02-03\n\
       Changed the copyright year range to include 2004. FITS maps containing\n\
       blanked positions are read properly. Temporarily swapping the axes in\n\
       contour plots caused the gray/colour scale to disappear in the Post-\n\
       Script plot window. This is now fixed.\n\
2004-02-03      *********** New version 1.2.10 ***********\n\
2004-02-11\n\
       Fixed a bug introduced 2004-02-03. That change made the gray/colour shade\n\
       in PostScript plots to disappear in horizontal maps (i.e. in Az, El). It\n\
       should now work for all kind of maps.\n\
2004-03-10\n\
       Fixed the harmless bug that a line, indicating start and end points for\n\
       vel-pos plots, appeared in contour plots even if all polylines were\n\
       removed.  Also removed the fixed upper limit of 10000 channels when\n\
       joining spectra with different center frequencies.  The resulting\n\
       maximum no of channels is now only determined by the amount of\n\
       memory available.\n\
2004-04-02\n\
       Adopted the frequency file reading to the changed format of the new\n\
       Lovas/SLAIM lists of molecular transitions.\n\
2004-05-13\n\
       Changed the map gridding in 'Graph'->'Type of map...' so that new\n\
       centre grid offsets are always used when offsets are set using the\n\
       current grid values.\n\
2004-05-13      *********** New version 1.2.11 ***********\n\
2005-01-27\n\
       Added conversion from galactic coordinates (Gal. long. and lat.)\n\
       to equatorial coordinates (R.A. and Decl.) in the precession menu.\n\
2005-04-10\n\
       Changed the formatting in getScanStr() for the file listing. It\n\
       now should appear aligned independent of coordinates.\n\
2005-04-13\n\
       One can now also make position-velocity maps (and not only velocity-\n\
       position maps).\n\
2005-2008       *********** New versions 1.2.12/13 ***********\n\
       Modified to read special APEX maps and holography data.\n\
2008-12-01      *********** New version 1.2.14 ***********\n\
       Extended the clipping possibility of spectra to also include clipping\n\
       data in the y-direction. One can now also combine data and rms FITS\n\
       files into one data set from separatively saved scans. Added the\n\
       possibility to sort data according to polarization.\n\
2009-12-01\n\
       Keyword IMAGEFREQ now read also for FITS cubes.\n\
2010-01-21\n\
       Fixing radians to degree bug for rotated maps (POSANG).\n\
2011-11-10\n\
       Adding a function to invert the y-values for scatter plots.\n\
2011-11-11      *********** New version 1.2.15 ***********\n\
2012-05-05\n\
       Removing the comment field separator in the written FITS files so that\n\
       pyfits can open the created FITS files. Also removed newline at end.\n\
2012-05-05      *********** New version 1.2.16 ***********\n\
2012-11-12\n\
       It is now possible to read Gildas/CLASS scans into xs.\n\
2012-11-13      *********** New version 1.2.17 ***********\n\
2012-11-20\n\
       Fixed bug which caused swapped RA and Dec from CLASS files.\n\
       Also, the delta_f for CLASS files is now consistent with xs.\n\
       Added new functions: 'Select scan(s)...' from 'Data sets' menu and\n\
       'Filter Dataset(s)...' from the 'Data sets'->'Manipulate Dataset(s)'\n\
       menu.\n\
------------------------------------------------------------------------------\n\
";

static char *mouse_hlp = "\
                      Mouse Button Info\n\
Left button:\n\
    Zooming (click-and-drag). Use \"Update graph\" under the Graph menu to\n\
    restore the default view.\n\
    Under Marker menu to manually enter the position of markers.\n\n\
Middle button:\n\
    Selecting (click-and-drag) regions for baseline and moment boxes. A\n\
    region (\"box\") is stored by adding it via \"Add box\" in the baseline\n\
    moment menus, respectively.\n\
    Channel modification under Reduction menu.\n\n\
    Dragging and switching spectra in multiple spectra plots.\n\n\
    To enter points for polylines in contour diagrams. By pressing the left\n\
    button you leave the mode.\n\n\
Right button:\n\
    Selecting (click-and-drag) a Gaussian curve. A Gaussian can be stored\n\
    via \"Add Gaussian\" in the Gauss menu.\n\n\
    Selecting a two-dimensional (elliptical, circular, or ring-like) Gaussian\n\
    in contour diagrams.\n\
";

static char *accel_hlp = "\
                      Keyboard accelerators\n\
The keyboard accelerators listed below are only invoked if the cursor is\n\
located within the graphical display. These accelerators are different from\n\
the standard Motif menu accelerators.\n\
\n\
     Character      Function\n\
         r          Redraw graph.\n\
         u          Update graph.\n\
         x          Update only the x-scaling.\n\
         X          Toggle between different x- scale units.\n\
         y          Update only the y-scaling.\n\
         h          Show spectrum header information.\n\
         >          Zoom out a factor of 2.\n\
         <          Zoom in a factor of 2.\n\
         g          Add a new gaussian.\n\
         f          Fit gaussian(s).\n\
         v          Fit polynomial(s).\n\
                    In contour diagrams pressing v will generate a dialog\n\
                    for making a velocity-position diagram along the nearest\n\
                    polyline.\n\
         s          subtract polynomial fit, if spectra shown. Otherwise\n\
	            it will toggle select scatter points.\n\
         b          Add new baseline box.\n\
         m          Add new moment box.\n\
         n          Select the next spectrum in the sequence (or map)\n\
         c          Invoke channel modification.\n\
                    In contour diagrams pressing c will result in closing\n\
                    the nearest open polyline (if any).\n\
         o          In contour diagrams pressing o will open up the nearest\n\
                    polyline.\n\
         d          Delete spectrum at cursor position (only in\n\
                    multiple/sequence plots). Ask for confirmation.\n\
                    In contour diagrams pressing d will result in deleting\n\
                    the nearest polyline (if any).\n\
         D          Delete spectrum at cursor position (only in\n\
                    multiple/sequence plots). Do _not_ ask for confirmation.\n\
                    Use with caution, since it is easy to remove spectra\n\
                    that are not supposed to be deleted.\n\
         e          Invoke a dialog to edit Gaussian fit parameters\n\
                    when Gaussians have been fitted to the spectrum in\n\
                    each map position. In single spectrum mode only\n\
                    the visible spectrum will be affected, in multiple\n\
                    view mode the parameters affected are those for spectrum\n\
                    over which the cursor in placed when 'e' is pressed.\n\
                    In contour diagrams pressing e will result in an edit\n\
                    dialog for editing the points of a polyline.\n\
         2          Toggle secondary spectrum view.\n\
         A          Clear all boxes.\n\
         H          Toggle line/histogram mode.\n\
         S          Toggle sum of gaussians view mode.\n\
         I          Toggle individual gaussian view mode.\n\
         Z          Toggle zero line.\n\
         B          Toggle boxes.\n\
         P          Toggle polynomial view.\n\
         M          Toggle markers.\n\
         C          Swap foreground and background colors.\n\
\n\
     <Left arrow>   Scroll left by 10% of current window width.\n\
     <Right arrow>  Scroll right by 10% of current window width.\n\
     <Up arrow>     Scroll up by 10% of current window height.\n\
     <Down arrow>   Scroll down by 10% of current window height.\n\
                    By holding down the shift key while pressing an arrow\n\
                    key the amount of scroll is 1%, the Control key results\n\
                    in 2.5% and the Alt key gives 100%, i.e. a full window.\n\
\n\
     <Delete>       Same as 'd' above.\n\
     <Backspace>    Same as 'D' above.\n\
\n\
     <F1>           Select single spectrum mode.\n\
     <F2>           Select single multiple spectrum mode.\n\
     <F3>           Select single position-position map mode.\n\
     <F4>           Select single velocity-position mode.\n\
     <F5>           Select single scatter plot mode.\n\
";

static char *filemenu_hlp = "\
-------------------------------------------------------------------------------\n\
                               File Menu\n\
-------------------------------------------------------------------------------\n\
Open native\n\
    Starts a file browser for opening a file in native format. Files in native\n\
    format ends with \".out\". This file format is only kept here for backward\n\
    compability, and will be removed at some point. When saving data: use FITS\n\
    instead.\n\
Open FITS\n\
    Starts a file browser for opening FITS files. The options are \"single\",\n\
    \"multiple\", and \"sequence\". Option \"single\" is used when reading\n\
    single dish FITS files one at the time. Options \"multiple\" should be used\n\
    when reading in several files in which the data can be gridded (i.e. map).\n\
    Use \"sequence\" when you want to read several files that do not comprise\n\
    into a map. The normal suffix for FITS files, \".fits\", is used.\n\
    Also 2-dimensional (map, image) and 3-dimensional (cubes) FITS files\n\
    can be opened. There is also support for reading SEST on-the-fly mapping\n\
    binary tables.  Also, the format of the Odin binary table format is\n\
    supported.\n\
Open CLASS\n\
    Opens a file browser for Gildas/CLASS data scans. It handles both\n\
    spectroscopic data scans and continuum drift scans. See\n\
    \"http://www.iram.fr/IRAMFR/GILDAS/\" for more information. The file name\n\
    suffix defaults here to \".apex\" but is different for other telescopes.\n\
    The filter can be used to read scans selectively from the file. The\n\
    resulting dataset will be a sequence (even if it contains map data).\n\
Open DRP\n\
    Starts a file browser for reading files stored with the old SEST/OSO Data\n\
    Reduction Package (DRP). See \"http://nain.oso.chalmers.se/\" for more\n\
    information about DRP. The file suffix is \".drp\".\n\
Open POPS\n\
    Starts a file browser for reading very old OSO POPS files. This file format is\n\
    only kept here for backward compability and will most likely be removed in\n\
    future releases of this software.\n\
Set FITS/DRP/CLASS filter\n\
    Invokes a fill-in form for a FITS/DRP filter. If the leftmost button in the\n\
    filter form is activated, only scans/files that match the corresponding item\n\
    (source name, number of channels etc.) will be read.\n\
Modify header\n\
    Invokes a dialog where certain header parameters can be modified.\n\
Open frequency file\n\
    View a subsection (the extent of the current view) of a molecular line\n\
    catalog. The supported line catalogs are:\n\
        JPL, R. L. Poynter & H. M. Pickett, 1985, \"Submillimeter, Millimeter,\n\
            and Microwave Spectral Line Catalog\", Applied Optics 24,\n\
            2335-2240, see \"http://spec.jpl.nasa.gov\"\n\
        SLAIM (F. Lovas' Spectral Line Atlas of Interstellar Molecules)\n\
        Lovas F., 1992, \"Recommended Rest Frequencies for Observed\n\
            Interstellar Microwave Transitions\",\n\
            J. Phys. Chem. Ref. Data, 21, 181, see\n\
            \"http://physlab.nist.gov/PhysRefData/micro/html/intro.html\"\n\
    Please note that the catalogs themselves are not included in this package,\n\
    they must be obtained from the sources listed above.\n\
Open scatter\n\
    Will open a scatter file. Any ASCII file will do as long as it contains\n\
    four columns of data (x, y, sigma_x, sigma_y).\n\
Save scatter\n\
    Will save a scatter plot to a file. The scatter plot will be saved as an\n\
    ASCII file. After some header info the data will be saved in four columns\n\
     x, y, sigma_x, sigma_y.\n\
Open graphical state\n\
    Will load the entire graphical state (except PostScript parameters).\n\
Save graphical state\n\
    Will save the graphical state to a binary file.\n\
Save native\n\
    Save a spectrum in a native file format. Avoid using this format since it\n\
    may not be supported in future releases.\n\
Save FITS\n\
    Save single FITS\n\
        Save a spectrum using single dish FITS format.\n\
    Save FITS array\n\
        Save a map using FITS map format.\n\
    Save FITS cube\n\
        Save a data cube using FITS cube format.\n\
    Save multiple FITS\n\
        Save multiple single dish FITS files (e.g. an entire map as individual\n\
        single dish FITS files).\n\
Save current spectra as ASCII table\n\
    Save a text file with information about the current spectra. It will\n\
    also open the file using the selected editor.\n\
Save current map as ASCII table\n\
    Save a text file with information about the current map. It will\n\
    also open the file using the selected editor.\n\
Save current scatter plot as ASCII table\n\
    Save a text file with information about the current scatter plot. It\n\
    will also open the file using the selected editor.\n\
Show PostScript file\n\
    Start up the PGPLOT viewer. " PKGNAME " uses the PGPLOT PostScript drivers\n\
    to save/print PostScript files. Several PGPLOT options can be set from\n\
    menus within the viewer. PGPLOT is not included in the package and must\n\
    be obtained from \"http://www.astro.caltech.edu/~tjp/pgplot\" if PostScript\n\
    printing/saving is required. The recommended PGPLOT version is >= 5.1.1\n\
    and " PKGNAME " makes use of the pgplot (FORTRAN), cpgplot (C-binder), and\n\
    XmPgplot (Motif driver) libraries.\n\
    PGPLOT, by Tim Pearson, is copyrighted by California Institute of\n\
    Technology.\n\
Save PostScript file\n\
    Save a PostScript file on disk.\n\
Print PostScript file\n\
    Send a plot (via PGPLOT) to the printer.\n\
Exit\n\
    Exit from " PKGNAME ".\n\
-------------------------------------------------------------------------------\n\
";

static char *graphmenu_hlp = "\
-------------------------------------------------------------------------------\n\
                            Graph Menu\n\
-------------------------------------------------------------------------------\n\
Redraw graph\n\
    Redraw the graph window without changing the scales.\n\
Update graph\n\
    Redraw graph and revert the x and y scaling to the default scales (normally\n\
    the entire view of the spectrum or map).\n\
Zoom options\n\
    Options for zooming in and out.\n\
Freeze the x-scale\n\
    Toggles the freeze state of the x-scaling.\n\
Freeze the y-scale\n\
    Toggles the freeze state of the y-scaling.\n\
Default x-scaling\n\
    Redraw the plot using default x-scaling.\n\
Default y-scaling\n\
    Redraw the plot using default y-scaling.\n\
Temporarily invert\n\
    Will invert the scaling on the x and y axes, respectively. The inverted\n\
    plotting mode will only last until an \"Update graph\" is performed.\n\
Change scales\n\
    Posts a dialog for changing the (spectral) x-unit.\n\
Attach current scaling\n\
    Will attach the current scaling to the plot. This scaling will then be\n\
    used for this plot in multiple plots.\n\
Detach scaling\n\
    Will remove any attached scaling from the current plot.\n\
Set size of multiple plot\n\
    Posts a dialog for multiple spectra plots. The options include\n\
    Number of plots in the x direction, x and y margins, position\n\
    of source name within every frame.\n\
Autoscale x-axis in multiple plots\n\
    Useful when spectra with different x-ranges are plotted in a\n\
    multiple spectra plot.\n\
Autoscale y-axis in multiple plots\n\
    Useful when spectra with different y-ranges are plotted in a\n\
    multiple spectra plot.\n\
Use attached scale if any\n\
    If selected, attached scaling will be used for the plots when available.\n\
    Otherwise, use the standard options for scaling.\n\
Type of top label\n\
    Various options for the top label.\n\
Graph options\n\
    Various options for the plot appearance.\n\
Dot markers\n\
    Various options for how to mark observed positions in a map or scatter\n\
    plot.\n\
Viewing modes\n\
    Select different viewing modes. The different modes are\n\
        (1) Single spectrum (startup mode)\n\
        (2) Multiple spectra, shows all map spectra on a grid\n\
        (3) Position-position map (contours and/or gray scales)\n\
        (4) Velocity-position map (contours and/or gray scales)\n\
        (5) Scatter plot (Integrated int. vs distance from a point)\n\
Type of map\n\
    Creates a dialog where parameters that affect the map appearance, such\n\
    as type of map (RA/Dec or Az/El), which parameter to use as z-value\n\
    (integrated intensity, mean intensity, etc.), and map position angle\n\
    (counted eastward from north).\n\
Show map info\n\
    Prints some information about a map in the message log window.\n\
Freeze contour levels\n\
    Toggles the freeze state of the contour levels\n\
Set contour levels\n\
    Posts a dialog in which the contour levels can be specified in different\n\
    ways.\n\
Contour levels\n\
    More options for contour/grey-scale maps.\n\
-------------------------------------------------------------------------------\n\
";

static char *gaussmenu_hlp = "\
-------------------------------------------------------------------------------\n\
                             Gauss Menu\n\
-------------------------------------------------------------------------------\n\
Start Gauss viewer\n\
    Start up a viewer that displays the fitted Gaussian parameters.\n\
Close Gauss viewer\n\
    Close the Gauss viewer.\n\
Gaussian mode...\n\
    Dialog for changing the mode in which the Gaussians are fitted. In absolute\n\
    mode (default) all Gaussians are treated similarily. In relative mode the\n\
    first Gaussian entered acts as a reference to subsequently entered\n\
    Gaussians. The relative mode is useful when fitting a group of Gaussians\n\
    that should have the same width, or when the differences between all\n\
    Gaussians should be fixed.\n\
Remove Gaussian\n\
    Various ways to remove a Gaussian.\n\
Add Gaussian\n\
    Will add a Gaussian. The Gaussian must have been selected using the right\n\
    mouse button first. After adding it the values will show up in the\n\
    \"Gaussian Fits\" window.\n\
Fit Gaussians\n\
    Will fit the Gaussians listed in the \"Gaussian Fits\" window to the\n\
    current spectrum. The values in the window will be updated.\n\
Fit Gaussians to map spectra\n\
    Will fit a single Gaussian in each map position.\n\
Fit 2-dim Gaussian to map\n\
    Will fit a single 2-dimensional Gaussian to a contour map.\n\
Add selected Gaussians\n\
    Will add the selected Gaussians to the current spectrum. Gaussians may be\n\
    selected using the leftmost toggle button in the \"Gauss Fits\" window.\n\
Add Gaussians to map spectra\n\
    Will add the individual Gaussians to each spectrum in a map.\n\
Subtract selected Gaussians\n\
    As above, but use subtraction instead of addition.\n\
Subtract Gaussians from map spectra\n\
    Will subtract the individual Gaussians from each map spectrum.\n\
Remove selected Gausssians\n\
    Remove the selected Gaussian entirely from the fit.\n\
Read Gaussians from file\n\
    Will start a file browser from which a file with saved Gaussians can be\n\
    read.\n\
Save Gaussians to file\n\
    Write (or append) the Gaussians to a special file using a file browser.\n\
    The file will be saved in the Gauss directory as selected in the\n\
    preferences. The first entry is the used unit (Frequency, Velocity, or\n\
    Channel) and the following entries define each Gaussian according to:\n\
           Center(Center unc.)  Amplitude(Ampl. unc.)  Width(Width unc.)\n\
    Note that when using the Frequency unit the width is still given in\n\
    velocity units.\n\
View Gaussians\n\
    View Gaussians saved in a file using the editor.\n\
Sort & View Gaussians\n\
    As above, but sort them first.\n\
Make all Gaussians markers\n\
    Make markers of all fitted Gaussians.\n\
-------------------------------------------------------------------------------\n\
";

static char *baselinemenu_hlp = "\
-------------------------------------------------------------------------------\n\
                            Baseline Menu\n\
-------------------------------------------------------------------------------\n\
Remove baseline box\n\
    Various ways of removing an active baseline box.\n\
Add baseline box\n\
    Will make the selected baseline box active. The box (or region) is\n\
    selected with the middle mouse button.\n\
Edit baseline box with cursor\n\
    After selection of this option your next click with the middle mouse button\n\
    inside a baseline box in the plot window will invoke a dialog enabling you\n\
    to change the location of the baseline box.\n\
Set RMS data from boxes\n\
    Will set the RMS data vector of all spectra to the value that is calculated\n\
    within the currently selected RMS boxes. After reading spectra the RMS\n\
    value is usually calculated from integration time, system temperature, and\n\
    band-width.\n\
Reset parameters\n\
    Will zero all polynomial coefficients (for all spectra).\n\
Edit current coefficients\n\
    Will start a small dialog in which it is possible to edit the coefficients\n\
    for the current baseline fit.\n\
Set polynomial order\n\
    Invokes a dialog for setting the polynomial order as well as\n\
    type of polynomial.\n\
Fit baseline\n\
    Will fit a polynomial to the currently viewed spectrum/spectra.\n\
Subtract polynomial fit\n\
    Subtract the polynomial fit from the data.\n\
Add polynomial fit\n\
    Add the polynomial fit to the data.\n\
Interpolate from pol. fit\n\
    Will use the the fit within the baseline boxes to interpolate the values\n\
    within the moment boxes. The interpolation only takes place if the mean\n\
    value within a moment box deviates significantly (> 3sigma) from the\n\
    polynomial value. Also an RMS noise (equal to that in the baseline boxes)\n\
    is added.\n\
Read boxes from file\n\
    Read a box file. If you already have current boxes these will be\n\
    replaced by the read boxes. The suffix *.box is used for box files.\n\
Save boxes to file\n\
    Will start a file browser that will let you save all the current boxes.\n\
    Both baseline boxes and moment regions will be saved to the same file.\n\
    The suffix *.box should preferably be used for box files. The box\n\
    files are saved as ASCII files, and can thus be easily edited.\n\
-------------------------------------------------------------------------------\n\
";

static char *momentmenu_hlp = "\
-------------------------------------------------------------------------------\n\
                               Moment Menu\n\
-------------------------------------------------------------------------------\n\
Remove moment box\n\
    Various ways of removing an active moment box. The moment boxes differ from\n\
    baseline boxes both in colour and line style.\n\
Add moment box\n\
    Will make the selected moment box active. The box (or region) is\n\
    selected using the middle mouse button.\n\
Edit moment box with cursor\n\
    After selection of this option your next click with the middle mouse button\n\
    inside a moment box in the plot window will invoke a dialog enabling you to\n\
    change the location of the moment box. It is also possible to let a timer\n\
    control the movement of the box. Useful when quickly displaying maps in\n\
    different velocity intervals.\n\
Remove polyline\n\
    Here you can remove either the last entered polyline or all entered poly-\n\
    lines.\n\
Create polyline circle\n\
    From this dialog you can create a polyline circle.\n\
-------------------------------------------------------------------------------\n\
";

static char *markermenu_hlp = "\
-------------------------------------------------------------------------------\n\
                              Marker Menu\n\
-------------------------------------------------------------------------------\n\
Remove marker\n\
    Various ways of removing a marker.\n\
Add new marker with cursor\n\
    After selection of this option, your next click with the left mouse button\n\
    in the plot window will invoke a dialog enabling you to add a marker at the\n\
    position where you \"clicked\".\n\
Edit marker with cursor\n\
    After selection of this option, your next click with the left mouse button\n\
    near an existing marker in the plot window will invoke a dialog that\n\
    enables you to change the location/type etc. of the marker.\n\
Open marker file\n\
    Read a marker file. If you already have current markers these will be\n\
    removed. The suffix *.mrk is used for marker files.\n\
Append marker file\n\
    Read a marker file but append the read markers to already (if any) existing\n\
    markers.\n\
Save marker file\n\
    Will start a file browser that will let you save the current markers.\n\
    The suffix *.mrk should preferably be used for marker files. The marker\n\
    files are saved as ASCII files, and can thus be easily edited.\n\
Join two markers\n\
    When this option is selected one can place the cursor and press the left\n\
    mouse button on two different markers, these two will then be connected\n\
    with a line. The option must be selected again if two other markers are\n\
    to be joined.\n\
Remove marker joint\n\
    When this option is invoked it will remove a marker-marker joint if the\n\
    cursor is plced on the marker and the left mouse button is pressed. The\n\
    option must be selected again if another joint is to be removed.\n\
Toggle marker tagging\n\
    When selected it allows you to select one or more markers by click-and-drag\n\
    with the left mouse button. All markers inside the region selected with the\n\
    mouse will be \"tagged\". Tagged markers will show up in the plotting\n\
    window with an extra box enclosing the marker. Tagged markers can thus\n\
    easily be distinguished from untagged. Special operations may be performed\n\
    on the tagged markers, see below. This special performance of the left\n\
    mouse button will prevent you from zooming using the mouse. To invoke\n\
    normal behaviour of the left mouse button, just select this option again.\n\
Untag all markers\n\
    Will untag all tagged markers.\n\
Make freq file markers\n\
    Will place markers (arrows + molecule name) using the frequencies listed in\n\
    the selected catalog. It will only look for entries falling in the\n\
    currently viewed frequency range.\n\
Removed tagged markers\n\
    Delete all markers that currently are tagged.\n\
Align markers\n\
    Will vertically align all tagged markers.\n\
-------------------------------------------------------------------------------\n\
";

#ifdef MACRO
#define REDUCTION_INTRO "\
-------------------------------------------------------------------------------\n\
                            Reduction Menu\n\
-------------------------------------------------------------------------------\n\
Edit macro\n\
    Here you can compose and run a macro. Please note that only a very limited\n\
    limited number of commands are available for use in the macro.\n\
    In the future all commands should be available. It will also be possible\n\
    to save macros.\n\
Execute macro\n\
    Will execute the macro."
#else
#define REDUCTION_INTRO "\
-------------------------------------------------------------------------------\n\
                            Reduction Menu\n\
-------------------------------------------------------------------------------\n"
#endif

static char *reductionmenu_hlp = REDUCTION_INTRO "\
Average all scans\n\
    Average all spectra in the currently selected data set. Four different kinds\n\
    of averaging are available:\n\
        (1) Use recorded system temperature (Tsys) and integration time (Tint)\n\
            as weight.\n\
            Weighting used: Tint / Tsys^2\n\
        (2) Use only recorded integration time as weight.\n\
            Weighting used: Tint\n\
        (3) Use the RMS value in the baseline boxes as weight. If no baselines\n\
            boxes are active " PKGNAME " will use an RMS value based on system\n\
            temperature and integration time (Tsys/sqrt(B Tint)).\n\
            There are two different schemes of RMS averaging. The accumulating\n\
            and adding schemes. They _may_ differ when the RMS value used in the\n\
            weighting is based on a _measured_ RMS from the base line boxes.\n\
            In the accumulating scheme the RMS value is remeasured in the\n\
            accumulated average when a scan has become accumulated. Thus, if a\n\
            large number of scans are being processed, low-level baseline\n\
            features may start to appear and _affect_ the RMS measured in the\n\
            accumulated average.\n\
            In the adding scheme this will not happen since the RMS weighting\n\
            factors are measured for each scan in the data set before the\n\
            averaging take place.\n\
        (4) Use the same weight for all spectra.\n\
Average scans over positions\n\
    As above, but only average scans in the same map position.\n\
Join scans (RMS):\n\
    Average all spectra read. It is different to the standard average task in\n\
    the sense that the spectra do not need to be aligned in terms of velocity or\n\
    frequency. It is necessary that the individual spectra all have the same\n\
    velocity or frequency resolution (depending on the x-unit currently showed)\n\
    before joining them together. Use 'Redres scans' (see below) to accomplish\n\
    this.\n\
Add all scans\n\
    Simply adds (not averages!) all scans together.\n\
Invert scans\n\
    Inverts the (spectral) x-axis.\n\
Fold scans\n\
    Will fold frequency switched scans. The invoked dialog contains fields for\n\
    frequency throw and frequency offset (if the FSW was asymmetric).\n\
Scale & bias scans\n\
    Will invoke a dialog for scaling and/or biasing scans.\n\
Clip scans\n\
    Will invoke a dialog for clipping scans (only in the x-axis).\n\
Smooth scans\n\
    Will invoke a dialog for smoothing scans (e.g. Boxcar/Hanning).\n\
Redres scans\n\
    Will invoke a dialog for reducing the spectral resolution of a spectrum.\n\
    The resulting resolution need not be an integer multiple of the initial\n\
    (and finer) resolution.\n\
Regrid scans\n\
    Task to spatially regrid map spectra.\n\
Project cube\n\
    A special task to project the emission from a thin spherical shell onto\n\
    a surface.\n\
Convolve scans\n\
    Task to spatially convolve map spectra.\n\
FFT scans\n\
    Calculate the FFT of the spectrum. Also the correlation coeffcient as\n\
    a function of channel difference can be obtained here.\n\
Swap Re&Im or Ampl&Phase\n\
    Will swap the real and imaginary parts (or amplitude and phase in case of\n\
    the polar version) of a FFTed spectrum. Useful when modifying the channel\n\
    values (see below) to remove sinoidal ripples etc.\n\
Remove all modifications\n\
    Will undo all channel value modifications.\n\
Remove last modification\n\
    Will undo the last channel value modification.\n\
Channel modification\n\
    After selection of this menu option, a channel value may be modified using\n\
    the middle mouse button. It is the position of the cursor when you \"click\"\n\
    that governs the new value.\n\
-------------------------------------------------------------------------------\n\
";

static char *datasetsmenu_hlp = "\
-------------------------------------------------------------------------------\n\
                              Data sets Menu\n\
-------------------------------------------------------------------------------\n\
Shift emission velocity\n\
    Posts a dialog for changing the VLSR velocity.\n\
Shift x-scale\n\
    Posts a dialog for changing the velocity (or frequency) x-scale\n\
    given a reference frequency (or velocity).\n\
Change velocity scale\n\
    Change the velocity scale between LSR and heliocentric scales. It will\n\
    use the coordinates (precessed to 1950 if necessary) of the spectrum.\n\
    Solar motion: v=20 km/s, RA=270.5 deg and Dec=+30.0 (1950).\n\
Change coordinates\n\
    Change offsets of spectra with respect to new offset positions and/or\n\
    new centre coordinates.\n\
Precess coordinates\n\
    Precess coordinates to the given epoch (J2000.0, B1950.0 etc.)\n\
Manipulate scans\n\
    Contains following tasks operating on scans:\n\
	Lists scans\n\
	    List all scans of the current data set.\n\
	Show scan\n\
	    Display a scan by selecting it from a list of all scans of the\n\
	    current data set. Useful when quickly looking through a data set.\n\
	Select scan(s)\n\
	    Select one or more scans to be copied from the current data set\n\
	    to a new data set.\n\
	Delete scan(s)\n\
	    Select one or more scans to be (permanently) deleted from the\n\
	    current data set.\n\
Attach boxes to data set\n\
    Attach current boxes (both baseline and moment boxes) to the current\n\
    data set. When switching between different data sets a prompt dialog\n\
    will appear if the newly selected data set has boxes attached to it.\n\
    You can in this pop-up dialog select whether the attached boxes should\n\
    be used or not. This is useful when the data sets have different number\n\
    of channels.\n\
Detach boxes from data set\n\
    Remove any attached boxes (see above) from the current data set.\n\
Manipulate data sets\n\
    Contains various tasks for manipulating data sets. The tasks include:\n\
    renaming, filtering, merging, combining data and RMS data, subtracting\n\
    ref. data, and deleting.\n\
";

static char *mapsmenu_hlp = "\
-------------------------------------------------------------------------------\n\
                              Maps Menu\n\
-------------------------------------------------------------------------------\n\
Store current map\n\
    Stores the current map into memory. You will be prompted to enter a name of\n\
    the map. This name will appear at the bottom of this menu from where it can\n\
    be selected later. The map will be accessible until you exit " PKGNAME " (or\n\
    manually removes the map, see below). Use \"Save FITS array\" under the\n\
    File menu to save the map on disk.\n\
Store multiple maps\n\
    Will invoke a dialog enabling you to store a sequence of maps. The sequence\n\
    is defined as a starting velocity, velocity increment, and number of maps.\n\
    Useful when making many maps over adjacent velocity intervals.\n\
Save maps as FITS cube\n\
    Will invoke a dialog in which you can select one or more stored maps to be\n\
    saved as a FITS cube.\n\
View current map as scatter plot\n\
    Will change viewing mode for the currently viewed map.\n\
Attach contour/grey data to map\n\
    Attach the current contour/grey scale data to the current map. Whenever\n\
    this map is selected for a contour/grey scale plot the attached contour\n\
    data will be used.\n\
Detach contour/grey data from map\n\
    Remove any attached contour/grey scale data from the current map.\n\
Manipulate maps\n\
    Various options for manipulating maps. The options include: showing,\n\
    copying, renaming, changing, convolving, deconvolving (MEM), and removing\n\
    stored maps.\n\
-------------------------------------------------------------------------------\n\
";

static char *scattermenu_hlp = "\
-------------------------------------------------------------------------------\n\
                             Scatter Menu\n\
-------------------------------------------------------------------------------\n\
Store current scatter plot\n\
    Stores the current scatter plot into memory. You will be prompted to enter\n\
    a name of the plot. This name will appear at the bottom of this menu from\n\
    where it can be selected later. The plot will be accessible until you exit\n\
    " PKGNAME " (or manually removes it, see below). Currently there is no way of\n\
    saving the scatter plots on disk.\n\
View current spectrum as scatter plot\n\
    Transform the current spectrum into a scatter plot, useful when certain\n\
    curves needs to be fitted to the spectrum. One can also view the spectrum\n\
    with RMS error bars using this option.\n\
View current spectra as scatter plots\n\
    Various options to plot parameters from the currently read spectra\n\
Set scatter plot options\n\
    Various options for scatter plots. The dialog has fields for changing the\n\
    position from which the distance is calculated. You can also fit different\n\
    curves to the points in the scatter plot here.\n\
Select scatter points\n\
    Instead of zooming all scatter points within the selected area will become\n\
    tagged. To regain zooming mode, select this option again. Certain operations\n\
    can be performed on scatter polots with selected (or tagged) points. A tagged\n\
    point will be drawn with a different color (red).\n\
Operate on sel. points\n\
    Six different operations on scatter plots containing tagged points. The\n\
    operations include: unselecting all points (remove the tagging), invert\n\
    the selection (tagged points become untagged and vice versa), make a new\n\
    scatter plot of the selected (or not selected) points, and make a new\n\
    data set of scans connected to selected (or not selected) points. A\n\
    scatter plot has connected scans to its points if the scatter plot were\n\
    made from a set of scans (and the data set hasn't been removed). A scatter\n\
    plot read from file has not scans attached to it.\n\
Manipulate scatter plots\n\
    Various options for manipulating scatter plots. The options include:\n\
    showing, renaming, copying, deleting scatter plots.\n\
-------------------------------------------------------------------------------\n\
";

static char *prefsmenu_hlp = "\
-------------------------------------------------------------------------------\n\
                             Prefs Menu\n\
-------------------------------------------------------------------------------\n\
View preferences in ~/.xsrc\n\
    Will invoke an editor showing the content of the ~/.xsrc file. This file\n\
    contains the preferences read by " PKGNAME " at startup. If you do not have such\n\
    a file in your home directory " PKGNAME " will use default settings. These may\n\
    not be appropriate for your system.\n\
Edit preferences\n\
    Will invoke a dialog enabling you to change current preferences. Most\n\
    preferences will only be used initially by " PKGNAME ". Use \"Save preferences\"\n\
    below to save the new preferences into the startup file.\n\
Save preferences\n\
    Will save the current preferences into the startup file .xsrc which will\n\
    be located in your home directory. If you do not have this file you can\n\
    create it by selecting this option.\n\
-------------------------------------------------------------------------------\n\
";

static char *helpmenu_hlp = "\
-------------------------------------------------------------------------------\n\
                             Help Menu\n\
-------------------------------------------------------------------------------\n\
This menu contains the online help information about all menus in " PKGNAME " as well\n\
as some more general information.\n\
-------------------------------------------------------------------------------\n\
";

/* ManageDialogCenteredOnPointer() is a nifty routine used in Mark Edel's
*  neat text editor NEdit, here is the copyright... */
/* NEdit is now GPL */
void ManageDialogCenteredOnPointer(Widget dialogChild)
{
    Widget shell = XtParent(dialogChild);
    Window root, child;
    unsigned int mask;
    unsigned int width, height, borderWidth, depth;
    int x, y, winX, winY, maxX, maxY;
    Boolean mappedWhenManaged;

    /* If this feature is not enabled, just manage the dialog */
/*     if (!PointerCenteredDialogsEnabled) {
    	XtManageChild(dialogChild);
    	return;
    }
 */    
    /* Temporarily set value of XmNmappedWhenManaged
       to stop the dialog from popping up right away */
    XtVaGetValues(shell, XmNmappedWhenManaged, &mappedWhenManaged, NULL);
    XtVaSetValues(shell, XmNmappedWhenManaged, False, NULL);
    
    /* Manage the dialog */
    XtManageChild(dialogChild);

    /* Get the pointer position (x, y) */
    XQueryPointer(XtDisplay(shell), XtWindow(shell), &root, &child,
	    &x, &y, &winX, &winY, &mask);

    /* Translate the pointer position (x, y) into a position for the new
       window that will place the pointer at its center */
    XGetGeometry(XtDisplay(shell), XtWindow(shell), &root, &winX, &winY,
    	    &width, &height, &borderWidth, &depth);
    width += 2 * borderWidth;
    height += 2 * borderWidth;
    x -= width/2;
    y -= height/2;

    /* Ensure that the dialog remains on screen */
    maxX = XtScreen(shell)->width - width;
    maxY = XtScreen(shell)->height - height;
    if (x < 0) x = 0;
    if (x > maxX) x = maxX;
    if (y < 0) y = 0;
    if (y > maxY) y = maxY;

    /* Set desired window position in the DialogShell */
    XtVaSetValues(shell, XmNx, x, XmNy, y, NULL);
    
    /* Map the widget */
    XtMapWidget(shell);
    
    /* Restore the value of XmNmappedWhenManaged */
    XtVaSetValues(shell, XmNmappedWhenManaged, mappedWhenManaged, NULL);
}

void SetWaitingScale(Widget scale, int value)
{
    int current, size, set;
    
    int MyLoop(int);
    
    if (!scale) return;
    if (!XtIsRealized(scale)) return;
    if (!XtIsSubclass(scale, xmScaleWidgetClass)) return;
    
    XtVaGetValues(scale,
                  XmNvalue, &current,
                  XmNuserData, &size,
                  NULL);
    
    set = (100*(value-1))/(size-1);
    if (set < 0)   set = 0;
    if (set > 100) set = 100;
    
    if (set != current) XtVaSetValues(scale, XmNvalue, set, NULL);
    
    while (MyLoop(1));
}

Widget PostWaitingDialog(Widget w, char *msg_str, Widget *scale, int size)
{
    int n;
    Widget form, rc, label, parent = w;
    Arg wargs[10];
    
    if (!parent) parent = gp->top;
    
    while (!XtIsWMShell(parent))
        parent = XtParent(parent);
   
    n = 0;
    XtSetArg(wargs[n], XmNtitle, "Please wait"); n++;
    form  = XmCreateFormDialog(parent, "form", wargs, n);
    
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,    XmATTACH_FORM);  n++;
    XtSetArg(wargs[n], XmNleftAttachment,   XmATTACH_FORM);  n++;
    XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_FORM);  n++;
    XtSetArg(wargs[n], XmNrightAttachment,  XmATTACH_FORM);  n++;
    rc    = XtCreateManagedWidget("rowcol", xmRowColumnWidgetClass,
                                  form, wargs, n);
      
    label = XtCreateManagedWidget(msg_str, xmLabelWidgetClass,
                                  rc, NULL, 0);
    if (scale && size > 1) {
        n = 0;
        XtSetArg(wargs[n], XmNminimum,     0);  n++;
        XtSetArg(wargs[n], XmNmaximum,     100);  n++;
        XtSetArg(wargs[n], XmNorientation, XmHORIZONTAL);  n++;
        XtSetArg(wargs[n], XmNuserData,    size); n++;
        XtSetArg(wargs[n], XmNsensitive,   False);  n++;
#if XmVersion >= 2000
        XtSetArg(wargs[n], XmNeditable,     False);  n++;
        XtSetArg(wargs[n], XmNslidingMode,  XmTHERMOMETER);  n++;
        XtSetArg(wargs[n], XmNsliderVisual, XmFOREGROUND_COLOR);  n++;
#endif
        *scale = XtCreateManagedWidget("scale", xmScaleWidgetClass,
                                       rc, wargs, n);
    }
    
    ManageDialogCenteredOnPointer(form);

    return form;
}

static void ok_help_dialog(Widget w, Widget box, XmAnyCallbackStruct *cb)
{
    if (box) XtDestroyWidget(box);
}

Widget PostWaitDialog(Widget w, char *msg_str)
{
    int n;
    Arg wargs[5];
    Widget msg, parent = w;
    string buf;
    XmString xstr;
    
    int MyLoop(int);
   
    if (!parent) parent = gp->top;
    
    while (!XtIsWMShell(parent))
        parent = XtParent(parent);
    
    sprintf(buf, "%s message", PKGNAME);
    
    n = 0;
    XtSetArg(wargs[n], XmNtitle, buf); n++;
    msg = XmCreateMessageDialog(parent, "msg", wargs, n);
    XtUnmanageChild(XmMessageBoxGetChild(msg, XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(msg, XmDIALOG_HELP_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(msg, XmDIALOG_OK_BUTTON));
    
    xstr = MKSTRING(msg_str);
    XtVaSetValues(msg,
                  XmNmessageString, xstr,
                  NULL);
        
    XmStringFree(xstr);
    
    ManageDialogCenteredOnPointer(msg);
    
    while (MyLoop(1));
    
    return msg;
}

void PostMessageDialog(Widget w, char *msg_str)
{
    int n;
    Arg wargs[5];
    Widget msg, parent = w;
    string buf;
    XmString xstr;
    
    if (!parent) parent = gp->top;
    
    while (!XtIsWMShell(parent))
        parent = XtParent(parent);

    sprintf(buf, "%s message", PKGNAME);
    
    n = 0;
    XtSetArg(wargs[n], XmNtitle, buf); n++;
    msg = XmCreateMessageDialog(parent, "msg", wargs, n);
    XtUnmanageChild(XmMessageBoxGetChild(msg, XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(msg, XmDIALOG_HELP_BUTTON));
    
    xstr = MKSTRING(msg_str);
    XtVaSetValues(msg,
                  XmNmessageString, xstr,
                  NULL);
    XmStringFree(xstr);
    
    XtAddCallback(msg, XmNokCallback,
                  (XtCallbackProc)ok_help_dialog, msg);
    
    ManageDialogCenteredOnPointer(msg);
}

void PostWarningDialog(Widget w, char *warn_str)
{
    Widget msg, parent = w;
    XmString xstr;
    string buf;
    int n;
    Arg wargs[5];
    
    if (!parent) parent = gp->top;
    
    while (!XtIsWMShell(parent))
        parent = XtParent(parent);

    sprintf(buf, "%s warning", PKGNAME);
    
    n = 0;
    XtSetArg(wargs[n], XmNtitle, buf); n++;
    msg = XmCreateWarningDialog(parent, "msg", wargs, n);
    XtUnmanageChild(XmMessageBoxGetChild(msg, XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(msg, XmDIALOG_HELP_BUTTON));
    
    xstr = MKSTRING(warn_str);
    XtVaSetValues(msg,
                  XmNmessageString, xstr,
                  NULL);
    XmStringFree(xstr);
    
    XtAddCallback(msg, XmNokCallback,
                  (XtCallbackProc)ok_help_dialog, msg);
    
    ManageDialogCenteredOnPointer(msg);
}

void PostErrorDialog(Widget w, char *err_str)
{
    int n;
    Widget msg, parent = w;
    XmString xstr;
    string buf;
    Arg wargs[5];
    
    if (!parent) parent = gp->top;
    
    while (!XtIsWMShell(parent))
        parent = XtParent(parent);

    sprintf(buf, "%s error", PKGNAME);
    
    n = 0;
    XtSetArg(wargs[n], XmNtitle, buf); n++;
    msg = XmCreateErrorDialog(parent, "msg", wargs, n);
    XtUnmanageChild(XmMessageBoxGetChild(msg, XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(msg, XmDIALOG_HELP_BUTTON));
    
    XtVaSetValues(msg,
                  XmNmessageString, xstr = MKSTRING(err_str),
                  NULL);
    XmStringFree(xstr);
    
    XtAddCallback(msg, XmNokCallback,
                  (XtCallbackProc)ok_help_dialog, msg);
    
    ManageDialogCenteredOnPointer(msg);
}

struct qdcb {
    int pending;
    int answer;
};

static void ok_question_dialog(Widget w, struct qdcb *q,
                               XmAnyCallbackStruct *cb)
{
    q->pending = 0;
    q->answer = 1;
}

static void cancel_question_dialog(Widget w, struct qdcb *q,
                                   XmAnyCallbackStruct *cb)
{
    q->pending = 0;
    q->answer = 0;
}

int PostQuestionDialog(Widget w, char *err_str)
{
    Widget msg, parent = w, butt;
    XmString xstr;
    string buf;
    int n;
    Arg wargs[5];
    struct qdcb q;
    
    if (!parent) parent = gp->top;
    
    while (!XtIsWMShell(parent))
        parent = XtParent(parent);

    sprintf(buf, "%s query", PKGNAME);
    
    n = 0;
    XtSetArg(wargs[n], XmNtitle, buf); n++;
    msg = XmCreateQuestionDialog(parent, "msg", wargs, n);
    XtUnmanageChild(XmMessageBoxGetChild(msg, XmDIALOG_HELP_BUTTON));
    
    XtVaSetValues(msg,
                  XmNmessageString, xstr = MKSTRING(err_str),
                  NULL);
    XmStringFree(xstr);
    
    butt = XmMessageBoxGetChild(msg, XmDIALOG_OK_BUTTON);
    XtVaSetValues(butt, XmNlabelString, xstr = MKSTRING("Yes"), NULL);
    XmStringFree(xstr);
    
    butt = XmMessageBoxGetChild(msg, XmDIALOG_CANCEL_BUTTON);
    XtVaSetValues(butt, XmNlabelString, xstr = MKSTRING("No"), NULL);
    XmStringFree(xstr);
        
    q.pending = 1;
    q.answer = 0;
    XtAddCallback(msg, XmNokCallback,
                  (XtCallbackProc)ok_question_dialog, &q);
    XtAddCallback(msg, XmNcancelCallback,
                  (XtCallbackProc)cancel_question_dialog, &q);
    
    ManageDialogCenteredOnPointer(msg);
    
    while (q.pending)
        XtAppProcessEvent(XtWidgetToApplicationContext(msg), XtIMAll);
        
    if (msg) XtDestroyWidget(msg);
    
    return q.answer;
}

static void okCB(Widget w, Widget parent, XtPointer call_data)
{
    XtDestroyWidget(parent);
}

void PostScrolledMessageDialog(Widget w, char *msg_str, char *title)
{
    int n;
    Widget form, text, button, parent = w;
    string buf;
    Arg wargs[20];
    XmString xstr;    
    
    if (!parent) parent = gp->top;
    
    while (!XtIsWMShell(parent))
        parent = XtParent(parent);

    sprintf(buf, "%s message", PKGNAME);
   
    n = 0;
    XtSetArg(wargs[n], XmNtitle, buf); n++;
    form = XmCreateFormDialog(parent, "helpform", wargs, n);

    n = 0;
    XtSetArg(wargs[n], XmNhighlightThickness, 0);  n++;
    XtSetArg(wargs[n], XmNlabelString, xstr=MKSTRING("Ok")); n++;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_NONE);  n++;
    XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_FORM);  n++;
    button = XmCreatePushButtonGadget(form, "ok", wargs, n);
    XtAddCallback(button, XmNactivateCallback,
                  (XtCallbackProc)okCB, XtParent(form));
    XmStringFree(xstr);
    XtManageChild(button);
    
    XtVaSetValues(form, XmNdefaultButton, button, NULL);
    
    n = 0;
    XtSetArg(wargs[n], XmNrows, 15);  n++;
    XtSetArg(wargs[n], XmNcolumns, 80);  n++;
    XtSetArg(wargs[n], XmNresizeHeight, False);  n++;
    XtSetArg(wargs[n], XmNtraversalOn, False); n++;
    XtSetArg(wargs[n], XmNwordWrap, True);  n++;
    XtSetArg(wargs[n], XmNscrollHorizontal, False);  n++;
    XtSetArg(wargs[n], XmNeditMode, XmMULTI_LINE_EDIT);  n++;
    XtSetArg(wargs[n], XmNeditable, False);  n++;
    XtSetArg(wargs[n], XmNvalue, msg_str);  n++;
    XtSetArg(wargs[n], XmNhighlightThickness, 0);  n++;
    XtSetArg(wargs[n], XmNspacing, 0);  n++;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_FORM);  n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM);  n++;
    XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_WIDGET);  n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_FORM);  n++;
    XtSetArg(wargs[n], XmNbottomWidget, button);  n++;
    text = XmCreateScrolledText(form, "helptext", wargs, n);
    XtManageChild(text);
    
    if (title)
        XtVaSetValues(XtParent(form),
                      XmNtitle, title,
                      NULL);
                      
    ManageDialogCenteredOnPointer(form);
}

void About(Widget w, char *cmd, XtPointer call_data)
{
    PostMessageDialog(w, about_str);
}

void Version(Widget w, char *cmd, XtPointer call_data)
{
    string XSver, tif, pgstr;
    
    char version[2000];
    char *GetDateStr();
    DATE *XS_localtime();
    
#ifdef SISYFOS
    sprintf(XSver, "%s SISYFOS", PKGNAME);
#else
#ifdef ONTHEFLY
    sprintf(XSver, "%s On-The-Fly", PKGNAME);
#else
    strcpy(XSver, PKGNAME);
#endif
#endif

#ifdef LesstifVersion
    sprintf(tif, "LessTif(%3.2f)", 0.01 * (double)LesstifVersion);
#else
    strcpy(tif, "Motif");
#endif
    
#ifdef HAVE_LIBPGPLOT
    strcpy(pgstr, "with");
#else
    strcpy(pgstr, "without");
#endif

    sprintf(version,
#ifdef XS_BETA
"%s version %3.1fbeta\n\
Patchlevel %d (%s)\n\n\
Max. number of channels %s\n\n\
Compiled with X%02dR%d and %s %d.%d\n\n\
Linked %s PGPLOT libraries",
#else
"%s version %3.1f\n\
Patchlevel %d (%s)\n\n\
Max. number of channels read %s\n\n\
Compiled with X%02dR%d and %s %d.%d\n\n\
Linked %s PGPLOT libraries",
#endif
            XSver,
            XS_VERSION, XS_PATCH, XS_VERDATE, "is \"unlimited\"",
            XT_VERSION, XT_REVISION,
            tif, XmVERSION, XmREVISION,
            pgstr);
    PostMessageDialog(w, version);
}

void ReadMe(Widget w, char *cmd, XtPointer call_data)
{
    PostScrolledMessageDialog(w, readme_str, "License");
}

void ProblemsAndBugs(Widget w, char *cmd, XtPointer call_data)
{
    PostScrolledMessageDialog(w, problem_str, "Problems & Bugs");
}

void WhatsNew(Widget w, char *cmd, XtPointer call_data)
{
    PostScrolledMessageDialog(w, news_str, "What's new?");
}

void Help(Widget w, char *cmd, XtPointer call_data)
{
    if (strcmp(cmd, "MouseButtons")==0)
        PostMessageDialog(w, mouse_hlp);
    else if (strcmp(cmd, "AccelKeys")==0)
        PostScrolledMessageDialog(w, accel_hlp, "Keyboard Accelerators Help");
    else if (strcmp(cmd, "FileMenu")==0)
        PostScrolledMessageDialog(w, filemenu_hlp, "File Menu Help");
    else if (strcmp(cmd, "GraphMenu")==0)
        PostScrolledMessageDialog(w, graphmenu_hlp, "Graph Menu Help");
    else if (strcmp(cmd, "GaussMenu")==0)
        PostScrolledMessageDialog(w, gaussmenu_hlp, "Gauss Menu Help");
    else if (strcmp(cmd, "BaselineMenu")==0)
        PostScrolledMessageDialog(w, baselinemenu_hlp, "Baseline Menu Help");
    else if (strcmp(cmd, "MomentMenu")==0)
        PostScrolledMessageDialog(w, momentmenu_hlp, "Moment Menu Help");
    else if (strcmp(cmd, "MarkerMenu")==0)
        PostScrolledMessageDialog(w, markermenu_hlp, "Marker Menu Help");
    else if (strcmp(cmd, "ReductionMenu")==0)
        PostScrolledMessageDialog(w, reductionmenu_hlp, "Reduction Menu Help");
    else if (strcmp(cmd, "DatasetsMenu")==0)
        PostScrolledMessageDialog(w, datasetsmenu_hlp, "Data sets Menu Help");
    else if (strcmp(cmd, "MapsMenu")==0)
        PostScrolledMessageDialog(w, mapsmenu_hlp, "Maps Menu Help");
    else if (strcmp(cmd, "ScatterMenu")==0)
        PostScrolledMessageDialog(w, scattermenu_hlp, "Scatter Menu Help");
    else if (strcmp(cmd, "PrefsMenu")==0)
        PostScrolledMessageDialog(w, prefsmenu_hlp, "Prefs Menu Help");
    else if (strcmp(cmd, "HelpMenu")==0)
        PostScrolledMessageDialog(w, helpmenu_hlp, NULL);
}
