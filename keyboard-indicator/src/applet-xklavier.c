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

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <libxklavier/xklavier.h>

#include "applet-struct.h"
#include "applet-draw.h"
#include "applet-xklavier.h"


void cd_xkbd_set_prev_next_group (int iDelta)
{
	Display *dsp = (Display*)cairo_dock_get_Xdisplay (); // const
	XklEngine *pEngine = xkl_engine_get_instance (dsp);  // const
	Window Xid = cairo_dock_get_current_active_window ();
	if (Xid == 0)
		Xid = DefaultRootWindow (dsp);
	XklState state;
	gboolean bSuccess = xkl_engine_get_state (pEngine, Xid, &state);
	g_return_if_fail (bSuccess);
	cd_debug ("keyboard current state : %d;%d +%d", state.group, state.indicators, iDelta);
	
	int i=0, n = xkl_engine_get_num_groups (pEngine);
	g_return_if_fail (n > 0);
	int iCurrentGroup = MAX (0, MIN (n-1, state.group));  // on blinde car libxklavier peut bugger en 64bits.
	const gchar **pGroupNames = xkl_engine_get_groups_names (pEngine);
	do  // on passe au groupe suivant/precedent en sautant les faux (-).
	{
		i ++;
		iCurrentGroup += iDelta;  // xkl_engine_get_next_group ne marche pas.
		if (iCurrentGroup == n)
			iCurrentGroup = 0;
		else if (iCurrentGroup < 0)
			iCurrentGroup = n - 1;
	} while (i < n && (pGroupNames[iCurrentGroup] == NULL || *pGroupNames[iCurrentGroup] == '-'));
	
	state.group = iCurrentGroup;
	cd_debug ("keyboard new state : %d", state.group);
	xkl_engine_allow_one_switch_to_secondary_group (pEngine);  // sert a quoi ??
	xkl_engine_save_state (pEngine, Xid, &state);
	xkl_engine_lock_group (pEngine, state.group);  // sert a quoi ??
}

void cd_xkbd_set_group (int iNumGroup)
{
	Display *dsp = (Display*)cairo_dock_get_Xdisplay (); // const
	XklEngine *pEngine = xkl_engine_get_instance (dsp); // const
	Window Xid = cairo_dock_get_current_active_window ();
	if (Xid == 0)
		Xid = DefaultRootWindow (dsp);
	XklState state;
	gboolean bSuccess = xkl_engine_get_state (pEngine, Xid, &state);
	g_return_if_fail (bSuccess);
	cd_debug ("keyboard current state : %d;%d", state.group, state.indicators);
	
	state.group = iNumGroup;
	
	xkl_engine_allow_one_switch_to_secondary_group (pEngine);  // sert a quoi ??
	xkl_engine_save_state (pEngine, Xid, &state);
	xkl_engine_lock_group (pEngine, state.group);  // sert a quoi ??
}

gboolean cd_xkbd_keyboard_state_changed (CairoDockModuleInstance *myApplet, Window *pWindow)
{
	CD_APPLET_ENTER;
	cd_debug ("%s (window:%ld)", __func__, (pWindow ? *pWindow : 0));
	///if (pWindow == NULL)
	///	return CAIRO_DOCK_LET_PASS_NOTIFICATION;
	
	Display *dsp = cairo_dock_get_Xdisplay ();
	Window Xid = (pWindow ? *pWindow : 0);
	if (Xid == 0)
		Xid = DefaultRootWindow (dsp);
	gchar *cShortGroupName = NULL;
	const gchar *cCurrentGroup = NULL;
	///GString *sCurrentIndicator = NULL;
	gboolean bRedrawSurface = FALSE;
	
	if (Xid != 0)
	{
		// on recupere l'etat courant.
		XklEngine *pEngine = xkl_engine_get_instance (dsp); // const
		XklState state;
		gboolean bSuccess = xkl_engine_get_state (pEngine, Xid, &state);
		if (!bSuccess)
		{
			cd_warning ("xkl_engine_get_state() failed, we use the first keyboard layout as a workaround (%d, %d)", state.group, state.indicators);
			state.group = 0;
			state.indicators = 0;
		}
		guint32 indicators;
		#if (GTK_MAJOR_VERSION < 3)
		indicators = state.indicators;
		#else // it seems that we don't get the right indicators...
		GdkKeymap *pKeyMap = gdk_keymap_get_default ();
		// second bit for numlock, first bit for caps lock => 0011 = numlock & caps lock
		indicators = (gdk_keymap_get_num_lock_state (pKeyMap) << 1) + gdk_keymap_get_caps_lock_state (pKeyMap);
		#endif
		cd_debug ("group : %d -> %d ; indic : %d -> %d", myData.iCurrentGroup, state.group, myData.iCurrentIndic, indicators);
		
		if (myData.iCurrentGroup == state.group && myData.iCurrentIndic == indicators)
			CD_APPLET_LEAVE (CAIRO_DOCK_LET_PASS_NOTIFICATION);
			//return CAIRO_DOCK_LET_PASS_NOTIFICATION;
		if (myData.iCurrentGroup != state.group)
		{
			bRedrawSurface = TRUE;
			
			// on recupere le groupe courant.
			int n = xkl_engine_get_num_groups (pEngine);
			CD_APPLET_LEAVE_IF_FAIL (n > 0, CAIRO_DOCK_LET_PASS_NOTIFICATION);
			int iNewGroup = MAX (0, MIN (n-1, state.group));  // en 64bits, on dirait qu'on peut recuperer des nombres invraisemblables dans 'state' (bug de libxklavier ?).
			const gchar **pGroupNames = xkl_engine_get_groups_names (pEngine);
			CD_APPLET_LEAVE_IF_FAIL (pGroupNames != NULL, CAIRO_DOCK_LET_PASS_NOTIFICATION);
			cCurrentGroup = pGroupNames[iNewGroup];
			CD_APPLET_LEAVE_IF_FAIL (cCurrentGroup != NULL, CAIRO_DOCK_LET_PASS_NOTIFICATION);
			cd_debug (" group name : %s (%d groups)", cCurrentGroup, n);

			// build the displayed group name
			cShortGroupName = g_strndup (cCurrentGroup, 3);
			int index = 0;
			int i;
			for (i = 0; i < state.group; i ++)  // look for groups before us having the same name.
			{
				if (strncmp (cCurrentGroup, pGroupNames[i], 3) == 0)
					index ++;
			}
			if (index != 0)  // add a number if several groups have the same name.
			{
				gchar *tmp = cShortGroupName;
				cShortGroupName = g_strdup_printf ("%s%d", cShortGroupName, index+1);
				g_free (tmp);
			}
		}
		// on recupere l'indicateur courant.
		/**const gchar **pIndicatorNames = xkl_engine_get_indicators_names (pEngine);
		if (myConfig.bShowKbdIndicator && pIndicatorNames != NULL)
		{
			if (myData.iCurrentGroup == -1 && state.indicators == 0)  // c'est probablement un bug dans libxklavier qui fait que l'indicateur n'est pas defini au debut.
			{
				cd_debug ("on force le num lock");
				state.indicators = 2;  // num lock, enfin j'espere que c'est toujours le cas ...
				xkl_engine_save_state (pEngine, Xid, &state);
				xkl_engine_lock_group (pEngine, state.group);
			}
			for (i = 0; i < 2; i ++)  // on parcours le champ de bits, mais les indicateurs Compose/Suspend/Misc/Mail/Mouse Keys/etc ne nous interessent pas !
			{
				if ((state.indicators >> i) & 1)  // i-eme bit a 1.
				{
					if (sCurrentIndicator == NULL)
						sCurrentIndicator = g_string_new ("");
					g_string_append_printf (sCurrentIndicator, "%s%s", (sCurrentIndicator->len == 0 ? "" : "/"), pIndicatorNames[i]);
				}
			}
			cd_debug (" indicator name : %s", sCurrentIndicator?sCurrentIndicator->str:"none");
		}*/
		if (myConfig.bShowKbdIndicator && myData.iCurrentGroup == -1 && indicators == 0)  // c'est probablement un bug dans libxklavier qui fait que l'indicateur n'est pas defini au debut.
		{
			cd_debug ("on force le num lock");
			indicators = 2;  // num lock, should be enable on the second bit (0010 = 2)
			state.indicators = 2; // we force the numlock
			xkl_engine_save_state (pEngine, Xid, &state);
			xkl_engine_lock_group (pEngine, state.group);
		}
		
		// on se souvient de l'etat courant.
		myData.iCurrentGroup = state.group;
		myData.iCurrentIndic = indicators;
		
		/*for (i = 0; i < n; i ++)
		{
			cd_debug ("kbd group name %d : %s - %s", i, pGroupNames[i], pIndicatorNames[i]);
		}
		XklConfigRec *pConfigRec = xkl_config_rec_new ();
		xkl_config_rec_get_from_server (pConfigRec, pEngine);
		if (pConfigRec->layouts != NULL)
		{
			for (i = 0; pConfigRec->layouts[i] != NULL; i ++)
				cd_debug(" layout : %s", pConfigRec->layouts[i]);
		}
		if (pConfigRec->variants != NULL)
		{
			for (i = 0; pConfigRec->variants[i] != NULL; i ++)
				cd_debug(" variants : %s", pConfigRec->variants[i]);
		}
		if (pConfigRec->options != NULL)
		{
			for (i = 0; pConfigRec->options[i] != NULL; i ++)
				cd_debug(" options : %s", pConfigRec->options[i]);
		}*/
	}
	
	cd_xkbd_update_icon (cCurrentGroup, cShortGroupName, bRedrawSurface);
	g_free (cShortGroupName);
	CD_APPLET_LEAVE (CAIRO_DOCK_LET_PASS_NOTIFICATION);
}
