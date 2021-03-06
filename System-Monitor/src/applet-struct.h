/**
* This file is a part of the Cairo-Dock project
*
* Copyright : (C) see the 'copyright' file.
* E-mail    : see the 'copyright' file.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 3
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef __CD_APPLET_STRUCT__
#define  __CD_APPLET_STRUCT__

#include <cairo-dock.h>

#define CD_SYSMONITOR_NB_MAX_VALUES 6

#define CD_SYSMONITOR_PROC_FS "/proc"

typedef enum _CDSysmonitorDisplayType {
	CD_SYSMONITOR_GAUGE=0,
	CD_SYSMONITOR_GRAPH,
	CD_SYSMONITOR_BAR,
	CD_SYSMONITOR_NB_TYPES
	} CDSysmonitorDisplayType; 

typedef struct {
	gint iLowerLimit;
	gint iUpperLimit;
	gint iAlertLimit;
	gboolean bAlert;
	gboolean bAlertSound;
	} CDAlertParam;

struct _AppletConfig {
	gchar *defaultTitle;
	gint iCheckInterval;
	gdouble fSmoothFactor;
	gboolean bShowCpu;
	gboolean bShowRam;
	gboolean bShowSwap;
	gboolean bShowNvidia;
	gboolean bShowCpuTemp;
	gboolean bShowFanSpeed;
	gboolean bShowFreeMemory;
	
	CairoDockInfoDisplay iInfoDisplay;
	gchar *cGThemePath;
	
	CDSysmonitorDisplayType iDisplayType;
	CairoDockTypeGraph iGraphType;
	gdouble fLowColor[3];
	gdouble fHigholor[3];
	gdouble fBgColor[4];
	gboolean bMixGraph;
	
	gint iNbDisplayedProcesses;
	gint iProcessCheckInterval;
	gboolean bTopInPercent;
	///CairoDockLabelDescription *pTopTextDescription;
	
	gchar *cSystemMonitorCommand;
	gchar *cSystemMonitorClass;
	gboolean bStealTaskBarIcon;
	gdouble fUserHZ;
	
	gchar *cSoundPath;
	gint iLowerLimit;
	gint iUpperLimit;
	gint iAlertLimit;
	gboolean bAlert;
	gboolean bAlertSound;
	RendererRotateTheme iRotateTheme;
} ;

typedef struct {
	gint iPid;
	gchar *cName;
	gint iCpuTime;
	gdouble fCpuPercent;
	gdouble iMemAmount;
	gdouble fLastCheckTime;
	} CDProcess;

typedef struct {
	GHashTable *pProcessTable;
	CDProcess **pTopList;
	GTimer *pTopClock;
	gboolean bSortTopByRam;
	gint iNbDisplayedProcesses;
	gdouble fUserHZ;
	gulong iMemPageSize;
	gint iNbCPU;
	GldiModuleInstance *pApplet;
	} CDTopSharedMemory;
	
struct _AppletData {
	// infos, constantes.
	gint iNbCPU;
	gulong iMemPageSize;
	gint iFrequency;
	gchar *cModelName;
	gchar *cGPUName;
	gint iVideoRam;
	gchar *cDriverVersion;
	
	GldiTask *pPeriodicTask;
	// shared memory for the main thread.
	gboolean bInitialized;
	gboolean bAcquisitionOK;
	GTimer *pClock;
	long long int cpu_user, cpu_user_nice, cpu_system, cpu_idle;
	unsigned long long ramTotal, ramFree, ramUsed, ramBuffers, ramCached;
	unsigned long long swapTotal, swapFree, swapUsed;
	gint iGPUTemp;
	gint iCPUTemp;
	gint iFanSpeed;
	gdouble fCpuPercent;
	gdouble fPrevCpuPercent;
	gdouble fRamPercent,fSwapPercent;
	gdouble fPrevRamPercent, fPrevSwapPercent;
	gdouble fGpuTempPercent;
	gdouble fPrevGpuTempPercent;
	gdouble fCpuTempPercent;
	gdouble fPrevCpuTempPercent;
	gdouble fFanSpeedPercent;
	gdouble fPrevFanSpeedPercent;
	gdouble fMaxFanSpeed;
	gboolean bNeedsUpdate;
	gint iTimerCount;
	gboolean bCpuTempAlarm;
	gboolean bFanAlarm;
	gint iCPUTempMin, iCPUTempMax;
	// end of shared memory.
	gboolean bAlerted;
	gboolean bCPUAlerted;
	gboolean bFanAlerted;
	gint iCount;  // pour sous-echantilloner les acquisitions de valeurs moins variables.
	
	// 'top' variables.
	guint iNbProcesses;  // last total number of processes.
	gboolean bSortTopByRam;  // current state of the cpu/ram button on the dialog.
	CairoDialog *pTopDialog;
	GldiTask *pTopTask;
} ;


#endif
