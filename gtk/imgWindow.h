/*
 * imgWindow.h
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * This file is part of "OpenSkyImager".
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <locale.h>
#include "imgBase.h"
#include "ttylist.h"
#include "gtkTools.h"
#include "imgPixbuf.h"
#include "imgFitsio.h"
#include "imgAvi.h"
#include "imgCamio.h"
#include "imgCFWio.h"
#include "gtkversions.h"

#define TAB_IMAGE    0
#define TAB_SETTINGS 1

#define TAB_CCD       0
#define TAB_COOLING   1
#define TAB_FILENAME  2
#define TAB_SCRIPTING 3
#define TAB_TIMELAPSE 4
#define TAB_HEADER    5
#define TAB_CFW       6
#define TAB_CALC      7

#define CFW_SLOTS     10

#define FITHDR_SLOTS  26
#define HDR_SWCREATE  0
#define HDR_INSTRUME  1
#define HDR_EXPTIME   2
#define HDR_EXPOSURE  3
#define HDR_GAIN      4
#define HDR_OFFSET    5
#define HDR_XBINNING  6
#define HDR_YBINNING  7
#define HDR_CCDTEMP   8
#define HDR_SETTEMP   9
#define HDR_PSZX      10
#define HDR_PSZY      11
#define HDR_FILTER    12
#define HDR_TELESCOP  13
#define HDR_FOCALLEN  14
#define HDR_APTDIA    15
#define HDR_IPANGX    16
#define HDR_IPANGY    17
#define HDR_IMAGETYP  18
#define HDR_FRAMENO   19
#define HDR_SITELAT   20
#define HDR_SITELONG  21
#define HDR_DATEOBS   22
#define HDR_DATE      23
#define HDR_OBSERVER  24
#define HDR_OBJECT    25

#ifdef DECLARE_WINDOW

	// Decorations
	#if GTK_MAJOR_VERSION == 3
	GdkRGBA clrSelected = { 0.050966005, 0.62745098, 0.784313725, 1.0 };   //Blue based
	GdkRGBA clrKill     = { 0.784313725, 0.509803922, 0.509803922, 1.0 };  //Red based
	GdkRGBA clrFbk      = { 0.784313725, 0.196078431, 0.196078431, 1.0 };  //Red based
	#else
	GdkColor clrSelected = { 0, 0x8282, 0xA0A0, 0xC8C8 };  //Blue based
	GdkColor clrKill     = { 0, 0xC8C8, 0x8282, 0x8282 };  //Red based
	GdkColor clrFbk      = { 0, 0xC8C8, 0x3232, 0x3232 };  //Red based
	#endif

	// Widgets
	GtkWidget *window, *swindow, *fixed, *imgevent;
	GtkWidget *image, *histogram, *fwhmroi;
	GtkWidget *cmd_settings, *cmd_about, *cmd_capture, *cmd_focus, *cmd_load, *cmd_run, *cmd_hold, *cmd_fit, *cmd_histogram;
	GtkWidget *hsc_maxadu, *hsc_minadu;
	GtkWidget *box_main, *pnd_main, *pnd_left, *box_top_left, *box_bot_left, *tab_right, *tab_settings;
	GtkWidget *imgstatus, *imgstatec, *imgstafit;
	GtkWidget *frm_histogram;
	GtkWidget *spn_histogram;
	GtkWidget *box_ccd, *box_cooling, *box_filename, *box_scripting, *box_timelapse, *box_header, *box_cfw, *box_calc;
	GtkWidget *cmb_exptime, *spn_expnum, *spn_shots, *pbr_expnum, *pbr_exptime;
	GtkWidget *cmb_camera, *cmb_bin, *cmb_csize, *cmb_dspeed, *cmb_mode, *lbl_mode, *cmb_amp, *cmb_denoise, *cmb_depth, *cmb_debayer;
	GtkWidget *cmd_camera, *cmd_setcamlst, *cmd_updcamlst, *cmd_resetcam;
	GtkWidget *cmd_tecenable, *cmd_tecdisable, *cmd_tecauto, *cmd_tecmanual, *spn_tectgt, *spn_tecspd, *vsc_tectemp, *vsc_tecpwr, *frm_tecgraph, *tecgraph;
	GtkWidget *cmd_saveas, *cmd_dateadd, *cmd_timeadd, *cmb_flt, *cmd_fltadd, *cmb_fmt;
	GtkWidget *txt_fitfolder, *txt_fitbase;
	GtkWidget *cmd_audela, *cmd_iris, *cmd_zerofc;
	GtkWidget *cmd_tlenable;
	GtkWidget *rbt_tlstart, *rbt_tlend, *lbl_tlstart, *lbl_tlend, *spn_tlhstart, *spn_tlhend, *spn_tlmstart, *spn_tlmend, *spn_tlsstart, *spn_tlsend, *cmd_tlcalendar, *cal_tldpick, *hsc_tlperiod, *spn_tlperiod;
	GtkWidget *cmb_cfw, *cmb_cfwtty, *cmd_cfwtty, *cmd_cfw, *cmb_cfwcfg, *cmd_cfwrst, *cmb_cfwwhl[CFW_SLOTS], *cmd_cfwwhl[CFW_SLOTS], *cfwmsg;
	GtkWidget *hsc_offset, *hsc_gain;
	GtkWidget *lbl_fbkimg, *lbl_fbktec, *lbl_fbkfps, *lbl_fbkfwhm;
	GdkCursor* watchCursor;
	GdkPixbuf *tecpixbuf = NULL;
	GdkPixbuf *icopixbuf = NULL;
	
	//Threads
	GThread *thd_capture = NULL;
	#if GLIB_MINOR_VERSION >= 32
	GRWLock  thd_caplock;
	GRWLock  thd_teclock;
	GRWLock  pixbuf_lock;
	#else
	GStaticRWLock thd_caplock;
	GStaticRWLock thd_teclock;
	GStaticRWLock pixbuf_lock;
	#endif

	// FifoManager
	gboolean fifomode = FALSE;
	int  fifofd = -1, fifotag = -1;
	GIOChannel *fifoch = NULL;
	char fifopath[256] = "/tmp/OpenSkyImager";
	int savejpg = 0;
	
	// Flags
	int fit = 0, hst = 0;
	double imgratio = 1., icoratio = 1.;
	int capture = 0;
	int run = 0, hold = 0, expose = 0, readout = 0, runerr = 0;
	int expnum = 0, shots = 0;
	double shotfract = 0., expfract = 0.;
	int fullcamlist = 0;	
	int uibytepix = 1;
	int scrmaxadu = 255;
	int scrminadu = 0;
	char *fltstr = "---|L|R|G|B|Dk|Ff|Bs|IR|UV|CLS|Ha|S2|O3|Hb|CH4|KaK|Kont|:0";
	char *fmtstr = "1-.fit|2-.avi|3-.fit + .avi|:0";
	int  savefmt = 1;
	guint tmrstatusbar = -1;
	guint tmrstatusfit = -1;
	guint tmrstatustec = -1;
	guint tmrimgrefresh = -1;
	guint tmraducheck = -1;
	guint tmrfrmrefresh = -1;
	guint tmrhstrefresh = -1;
	guint tmrcapprgrefresh = -1;
	guint tmrexpprgrefresh = -1;
	guint tmrtecrefresh = -1;
	guint tmrtecpwr = -1;
	char fitfolder[1024];
	char fitbase[1024];
	char fitflt[16];
	double fps = 0.;
	int fitdateadd = 0, fittimeadd = 0, audelanaming = 0, irisnaming = 0, zerofc = 0, tlenable = 0, tlcalendar = 0;
	int tlpick = 0, tlperiod = 1;
	guint tmrtlrefresh = -1;
	char imgmsg[2560];
	char imgfbk[16];
	char tecfbk[16];
	char fpsfbk[16];
	char fwhmfbk[64];
	int fwhmx = 0, fwhmy = 0, fwhmv = 0, fwhms = 64, fwhmp = 4096, pfwhm = 1, fwhmlblh = 0, fwhmlblw = 0;
	double afwhm = 0.;
	int tecrun = 0, tecprerun = 0, tecspeed = 0;
	struct tm tlstart, tlend;
	
	// Locale definitions
	struct lconv *sysloc;
	
	// Cpu
	int cpucores = 1;
	
	// Fits header
	fit_rowhdr fithdr[FITHDR_SLOTS];
#else
	// Decorations
	#if GTK_MAJOR_VERSION == 3
	extern GdkRGBA clrSelected;
	extern GdkRGBA clrKill;
	extern GdkRGBA clrFbk;
	#else
	extern GdkColor clrSelected;  
	extern GdkColor clrKill;
	extern GdkColor clrFbk; 
	#endif

	// Widgets
	extern GtkWidget *window, *swindow, *fixed, *imgevent;
	extern GtkWidget *image, *histogram, *fwhmroi;
	extern GtkWidget *cmd_settings, *cmd_about, *cmd_capture, *cmd_focus, *cmd_load, *cmd_run, *cmd_hold, *cmd_fit, *cmd_histogram;
	extern GtkWidget *hsc_maxadu, *hsc_minadu;
	extern GtkWidget *box_main, *pnd_main, *pnd_left, *box_top_left, *box_bot_left, *tab_right, *tab_settings;
	extern GtkWidget *imgstatus, *imgstatec, *imgstafit;
	extern GtkWidget *frm_histogram;
	extern GtkWidget *spn_histogram;
	extern GtkWidget *box_ccd, *box_cooling, *box_filename, *box_scripting, *box_timelapse, *box_header, *box_cfw, *box_calc;
	extern GtkWidget *cmb_exptime, *spn_expnum, *spn_shots, *pbr_expnum, *pbr_exptime;
	extern GtkWidget *cmb_camera, *cmb_bin, *cmb_csize, *cmb_dspeed, *cmb_mode, *lbl_mode, *cmb_amp, *cmb_denoise, *cmb_depth, *cmb_debayer;
	extern GtkWidget *cmd_camera, *cmd_setcamlst, *cmd_updcamlst, *cmd_resetcam;
	extern GtkWidget *cmd_tecenable, *cmd_tecdisable, *cmd_tecauto, *cmd_tecmanual, *spn_tectgt, *spn_tecspd, *vsc_tectemp, *vsc_tecpwr, *frm_tecgraph, *tecgraph;
	extern GtkWidget *hsc_offset, *hsc_gain;
	extern GtkWidget *cmd_saveas, *cmd_dateadd, *cmd_timeadd, *cmb_flt, *cmd_fltadd, *cmb_fmt;
	extern GtkWidget *txt_fitfolder, *txt_fitbase;
	extern GtkWidget *cmd_audela, *cmd_iris, *cmd_zerofc;
	extern GtkWidget *cmd_tlenable;
	extern GtkWidget *rbt_tlstart, *rbt_tlend, *lbl_tlstart, *lbl_tlend, *spn_tlhstart, *spn_tlhend, *spn_tlmstart, *spn_tlmend, *spn_tlsstart, *spn_tlsend, *cmd_tlcalendar, *cal_tldpick, *hsc_tlperiod, *spn_tlperiod;
	extern GtkWidget *cmb_cfw, *cmb_cfwtty, *cmd_cfwtty, *cmd_cfw, *cmb_cfwcfg, *cmd_cfwrst, *cmb_cfwwhl[CFW_SLOTS], *cmd_cfwwhl[CFW_SLOTS], *cfwmsg;
	extern GtkWidget *lbl_fbkimg, *lbl_fbktec, *lbl_fbkfps, *lbl_fbkfwhm;
	extern GdkCursor* watchCursor;
	extern GdkPixbuf *tecpixbuf;
	extern GdkPixbuf *icopixbuf;
	
	//Threads
	extern GThread *thd_capture;
	#if GLIB_MINOR_VERSION >= 32
	extern GRWLock  thd_caplock;
	extern GRWLock  thd_teclock;
	extern GRWLock  pixbuf_lock;
	#else
	extern GStaticRWLock thd_caplock;
	extern GStaticRWLock thd_teclock;
	extern GStaticRWLock pixbuf_lock;
	#endif

	// FifoManager
	extern gboolean fifomode;
	extern int  fifofd, fifotag;
	extern GIOChannel *fifoch;
	extern char fifopath[256];
	extern int savejpg;

	// Flags
	extern int fit, hst;
	extern double imgratio, icoratio;
	extern int capture;
	extern int run, hold, expose, readout, runerr;
	extern int expnum, shots;
	extern double shotfract, expfract;
	extern int fullcamlist;	
	extern int uibytepix;
	extern int scrmaxadu;
	extern int scrminadu;
	extern char *fltstr;
	extern char *fmtstr;
	extern int  savefmt;
	extern guint tmrstatusbar;
	extern guint tmrstatusfit;
	extern guint tmrstatustec;
	extern guint tmrimgrefresh;
	extern guint tmraducheck;
	extern guint tmrfrmrefresh;
	extern guint tmrhstrefresh;
	extern guint tmrcapprgrefresh;
	extern guint tmrexpprgrefresh;
	extern guint tmrtecrefresh;
	extern guint tmrtecpwr;
	extern char fitfolder[1024];
	extern char fitbase[1024];
	extern char fitflt[16];
	extern double fps;
	extern int fitdateadd, fittimeadd, audelanaming, irisnaming, zerofc, tlenable, tlcalendar;
	extern guint tmrtlrefresh;
	extern int tlpick, tlperiod;
	extern char imgmsg[2560];
	extern char imgfbk[16];
	extern char tecfbk[16];
	extern char fpsfbk[16];
	extern char fwhmfbk[64];
	extern int fwhmx, fwhmy, fwhmv, fwhms, fwhmp, pfwhm, fwhmlblh, fwhmlblw;
	extern double afwhm;
	extern int tecrun, tecprerun, tecspeed;
	extern struct tm tlstart, tlend;
	
	// Locale definitions
	extern struct lconv *sysloc;

	// Cpu
	extern int cpucores;
	
	// Fits header
	extern fit_rowhdr fithdr[FITHDR_SLOTS];
#endif
	
void imgwin_build();
gpointer fiforead();

