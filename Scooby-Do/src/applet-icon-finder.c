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
#include <string.h>

#include "applet-struct.h"
#include "applet-session.h"
#include "applet-appli-finder.h"
#include "applet-icon-finder.h"

static inline gboolean _cd_do_icon_match (Icon *pIcon, const gchar *cCommandPrefix, int length)
{
	gboolean bMatch = FALSE;
	if (pIcon->cBaseURI != NULL)
	{
		gchar *cFile = g_path_get_basename (pIcon->cCommand);
		bMatch = (cFile && g_ascii_strncasecmp (cCommandPrefix, cFile, length) == 0);
		g_free (cFile);
	}
	else if (pIcon->cCommand)
	{
		bMatch = (g_ascii_strncasecmp (cCommandPrefix, pIcon->cCommand, length) == 0);
		if (!bMatch)
		{
			gchar *str = strchr (pIcon->cCommand, '-');  // on se limite au 1er tiret.
			if (str && *(str-1) != ' ')  // on verifie qu'il n'est pas un tiret d'option
			{
				str ++;
				bMatch = (g_strncasecmp (str, cCommandPrefix, length) == 0);
			}
			if (!bMatch && pIcon->cName)
				bMatch = (g_ascii_strncasecmp (cCommandPrefix, pIcon->cName, length) == 0);
		}
	}
	return bMatch;
}

static void _find_icon_in_dock_with_command (Icon *pIcon, CairoDock *pDock, gpointer *data)
{
	gchar *cCommandPrefix = data[0];
	int length = GPOINTER_TO_INT (data[1]);
	Icon *pAfterIcon = data[2];
	Icon **pFoundIcon = data[3];
	CairoDock **pFoundDock = data[4];
	Icon **pFirstIcon = data[5];
	CairoDock **pFirstParentDock = data[6];
	if (pDock == g_pMainDock || *pFoundIcon != NULL) // on a deja cherche dans le main dock, ou deja trouve ce qu'on cherchait.
		return ;
	
	gboolean bFound = _cd_do_icon_match (pIcon, cCommandPrefix, length);
	if (bFound)
	{
		if (pAfterIcon == NULL)
		{
			*pFoundIcon = pIcon;
			*pFoundDock = pDock;
		}
		else
		{
			if (*pFirstIcon == NULL)  // on garde une trace de la 1ere icone pour boucler dans la liste.
			{
				*pFirstIcon = pIcon;
				*pFirstParentDock = g_pMainDock;
			}
			if (pIcon == pAfterIcon)
			{
				data[2] = NULL;
			}
		}
	}
}
Icon *cd_do_search_icon_by_command (const gchar *cCommandPrefix, Icon *pAfterIcon, CairoDock **pDock)
{
	g_return_val_if_fail (cCommandPrefix != NULL, NULL);
	
	//\_________________ on cherche en premier dans le main dock, car il est deja visible.
	int length = strlen (cCommandPrefix);
	Icon *pIcon, *pFirstIcon = NULL;
	CairoDock *pParentDock, *pFirstParentDock = NULL;
	GList *ic;
	for (ic = g_pMainDock->icons; ic != NULL; ic = ic->next)
	{
		pIcon = ic->data;
		if (pIcon->cCommand && g_ascii_strncasecmp (cCommandPrefix, pIcon->cCommand, length) == 0)
		{
			if (pAfterIcon == NULL)
			{
				*pDock = g_pMainDock;
				return pIcon;
			}
			else
			{
				if (pFirstIcon == NULL)  // on garde une trace de la 1ere icone pour boucler dans la liste.
				{
					pFirstIcon = pIcon;
					pFirstParentDock = g_pMainDock;
				}
				if (pIcon == pAfterIcon)
				{
					pAfterIcon = NULL;
				}
			}
		}
	}
	
	//\_________________ si on a rien trouve on cherche dans tous les docks.
	pIcon = NULL;
	*pDock = NULL;
	gpointer data[7];
	data[0] = (gchar *)cCommandPrefix;
	data[1] = GINT_TO_POINTER (length);
	data[2] = pAfterIcon;
	data[3] = &pIcon;
	data[4] = pDock;
	data[5] = &pFirstIcon;
	data[6] = &pFirstParentDock;
	cairo_dock_foreach_icons_in_docks ((CairoDockForeachIconFunc) _find_icon_in_dock_with_command, data);
	
	if (pIcon == NULL)
	{
		pIcon = pFirstIcon;
		*pDock = pFirstParentDock;
	}
	return pIcon;
}


void cd_do_change_current_icon (Icon *pIcon, CairoDock *pDock)
{
	//\_________________ on gere le cachage et le montrage du dock precedent et actuel.
	if (myData.pCurrentDock != NULL && pDock != myData.pCurrentDock && myData.pCurrentDock != g_pMainDock)  // on remet au repos dock precedemment anime.
	{
		cairo_dock_emit_leave_signal (myData.pCurrentDock);
	}
	if (pDock != NULL && pDock != g_pMainDock && pDock != myData.pCurrentDock)  // on montre le nouveau dock
	{
		if (pDock != NULL)
		{
			if (pDock->iRefCount > 0)
			{
				CairoDock *pParentDock = NULL;
				Icon *pPointingIcon = cairo_dock_search_icon_pointing_on_dock (pDock, &pParentDock);
				if (pPointingIcon != NULL)
				{
					cairo_dock_show_subdock (pPointingIcon, pParentDock);  // utile pour le montrage des sous-docks au clic.
				}
			}
			else
			{
				///cairo_dock_pop_up (pDock);
			}
			cairo_dock_emit_enter_signal (pDock);
		}
	}
	if (pDock != NULL)
	{
		
		gtk_window_present (GTK_WINDOW (pDock->container.pWidget));
	}
	
	//\_________________ on gere l'allumage et l'eteignage de l'icone precedente et actuelle.
	if (myData.pCurrentIcon != NULL && pIcon != myData.pCurrentIcon)  // on remet au repos l'icone precedemment anime.
	{
		myData.bIgnoreIconState = TRUE;
		cairo_dock_stop_icon_animation (myData.pCurrentIcon);
		myData.bIgnoreIconState = FALSE;
		cairo_dock_redraw_icon (myData.pCurrentIcon, CAIRO_CONTAINER (myData.pCurrentDock));  /// utile ?...
	}
	if (pIcon != NULL && myData.pCurrentIcon != pIcon)  // on anime la nouvelle icone.
	{
		int x = pIcon->fXAtRest + pIcon->fWidth/2 + (- pDock->fFlatDockWidth + pDock->iMaxDockWidth)/2;
		int y = pIcon->fDrawY + pIcon->fHeight/2 * pIcon->fScale;
		if (1||myData.pCurrentDock != pDock)
		{
			cairo_dock_emit_motion_signal (pDock,
				x,
				y);
		}
		else
		{
			myData.iPrevMouseX = myData.iMouseX;
			myData.iPrevMouseY = myData.iMouseY;
			myData.iMotionCount = 10;
		}
		myData.iMouseX = x;
		myData.iMouseY = y;
		cairo_dock_request_icon_animation (pIcon, pDock, myConfig.cIconAnimation, 1e6);  // interrompt l'animation de "mouse over".
		cairo_dock_launch_animation (CAIRO_CONTAINER (pDock));
		//if (myAccessibility.bShowSubDockOnClick)
		//	cairo_dock_show_subdock (pIcon, pDock, FALSE);
	}
	
	myData.pCurrentDock = pDock;
	myData.pCurrentIcon = pIcon;
	if (myData.pCurrentDock == NULL)
		gtk_window_present (GTK_WINDOW (g_pMainDock->container.pWidget));
}


void cd_do_search_current_icon (gboolean bLoopSearch)
{
	//\_________________ on cherche un lanceur correspondant.
	CairoDock *pDock;
	Icon *pIcon = cd_do_search_icon_by_command (myData.sCurrentText->str, (bLoopSearch ? myData.pCurrentIcon : NULL), &pDock);
	cd_debug ("found icon : %s\n", pIcon ? pIcon->cName : "none");
	
	//\_________________ on gere le changement d'icone/dock.
	cd_do_change_current_icon (pIcon, pDock);
}


gboolean cairo_dock_emit_motion_signal (CairoDock *pDock, int iMouseX, int iMouseY)
{
	static gboolean bReturn;
	static GdkEventMotion motion;
	motion.state = 0;
	motion.x = iMouseX;
	motion.y = iMouseY;
	motion.x_root = pDock->container.iWindowPositionX + pDock->container.iMouseX;
	motion.y_root = pDock->container.iWindowPositionY + pDock->container.iMouseY;
	motion.time = 0;
	motion.window = pDock->container.pWidget->window;
	motion.device = gdk_device_get_core_pointer ();
	g_signal_emit_by_name (pDock->container.pWidget, "motion-notify-event", &motion, &bReturn);
	return FALSE;
}



static inline void _cd_do_search_matching_icons_in_dock (CairoDock *pDock)
{
	Icon *pIcon;
	GList *ic;
	for (ic = pDock->icons; ic != NULL; ic = ic->next)
	{
		pIcon = ic->data;
		if (_cd_do_icon_match (pIcon, myData.sCurrentText->str, myData.sCurrentText->len))
		{
			myData.pMatchingIcons = g_list_prepend (myData.pMatchingIcons, pIcon);
		}
	}
}
static void _cd_do_search_in_one_dock (Icon *pIcon, CairoDock *pDock, gpointer data)
{
	if (_cd_do_icon_match (pIcon, myData.sCurrentText->str, myData.sCurrentText->len))
	{
		myData.pMatchingIcons = g_list_prepend (myData.pMatchingIcons, pIcon);
	}
}
void cd_do_search_matching_icons (void)
{
	if (myData.sCurrentText->len == 0)
		return;
	cd_debug ("%s (%s)\n", __func__, myData.sCurrentText->str);
	gchar *str = strchr (myData.sCurrentText->str, ' ');  // on ne compte pas les arguments d'une eventuelle commande deja tapee.
	int length = myData.sCurrentText->len;
	if (str != NULL)
	{
		g_string_set_size (myData.sCurrentText, str - myData.sCurrentText->str + 1);
		cd_debug (" on ne cherchera que '%s' (len=%d)\n", myData.sCurrentText->str, myData.sCurrentText->len);
	}
		
	if (myData.pMatchingIcons == NULL)
	{
		if (myData.bSessionStartedAutomatically)  // on cherche dans le dock courant.
		{
			cd_debug ("on cherche dans le dock\n");
			_cd_do_search_matching_icons_in_dock (myData.pCurrentDock);
			myData.pMatchingIcons = g_list_reverse (myData.pMatchingIcons);
		}
		else
		{
			cd_debug ("on cherche tout\n");
			// on parcours tous les docks.
			cairo_dock_foreach_icons_in_docks ((CairoDockForeachIconFunc) _cd_do_search_in_one_dock, NULL);
			myData.pMatchingIcons = g_list_reverse (myData.pMatchingIcons);
			
			// on rajoute les icones ne venant pas du dock.
			cd_do_find_matching_applications ();
		}
	}
	else  // optimisation : on peut se contenter de chercher parmi les icones deja trouvees.
	{
		cd_debug ("on se contente d'enlever celles en trop\n");
		GList *ic, *next_ic;
		Icon *pIcon;
		ic = myData.pMatchingIcons;
		while (ic != NULL)
		{
			pIcon = ic->data;
			next_ic = ic->next;
			if (! _cd_do_icon_match (pIcon, myData.sCurrentText->str, myData.sCurrentText->len))
				myData.pMatchingIcons = g_list_delete_link (myData.pMatchingIcons, ic);
			ic = next_ic;
		}
	}
	myData.pCurrentMatchingElement = myData.pMatchingIcons;
	myData.iMatchingGlideCount = 0;
	myData.iPreviousMatchingOffset = 0;
	myData.iCurrentMatchingOffset = 0;
	if (myData.pCurrentApplicationToLoad != NULL)  // on va continuer le chargement sur la sous-liste.
		myData.pCurrentApplicationToLoad = myData.pMatchingIcons;  // comme l'ordre de la liste n'a pas ete altere, on n'est sur de ne pas sauter d'icone.
	cairo_dock_redraw_container (CAIRO_CONTAINER (myData.pCurrentDock));
	//g_print ("%d / %d\n", length , myData.sCurrentText->len);
	if (length != myData.sCurrentText->len)
		g_string_set_size (myData.sCurrentText, length);
}


void cd_do_select_previous_next_matching_icon (gboolean bNext)
{
	GList *pMatchingElement = myData.pCurrentMatchingElement;
	do
	{
		if (!bNext)
			myData.pCurrentMatchingElement = cairo_dock_get_previous_element (myData.pCurrentMatchingElement, myData.pMatchingIcons);
		else
			myData.pCurrentMatchingElement = cairo_dock_get_next_element (myData.pCurrentMatchingElement, myData.pMatchingIcons);
	} while (myData.pCurrentMatchingElement != pMatchingElement && ((Icon*)myData.pCurrentMatchingElement->data)->pIconBuffer == NULL);
	
	if (myData.pCurrentMatchingElement != pMatchingElement)  // on complete le texte et on redessine.
	{
		Icon *pIcon = myData.pCurrentMatchingElement->data;
		if (pIcon->cCommand && *pIcon->cCommand != *myData.sCurrentText->str)  // cas d'une commande avec un tiret.
			myData.iNbValidCaracters = 0;
		cd_do_delete_invalid_caracters ();
		
		if (pIcon->cBaseURI != NULL)
		{
			gchar *cFile = g_path_get_basename (pIcon->cCommand);
			g_string_assign (myData.sCurrentText, cFile);
			g_free (cFile);
		}
		else
			g_string_assign (myData.sCurrentText, pIcon->cCommand);
		
		
		cd_do_load_pending_caracters ();
		
		// on arme l'animation de decalage.
		myData.iMatchingGlideCount = 10;  // on rembobine l'animation.
		myData.iPreviousMatchingOffset = myData.iCurrentMatchingOffset;  // on part du point courant.
		CairoDock *pParentDock = cairo_dock_search_dock_from_name (pIcon->cParentDockName);
		int iWidth, iHeight;
		cairo_dock_get_icon_extent (pIcon, CAIRO_CONTAINER (pParentDock), &iWidth, &iHeight);
		if (iHeight != 0)
		{
			double fZoom = (double) g_pMainDock->container.iHeight/2 / iHeight;
			myData.iMatchingAimPoint += (bNext ? 1 : -1) * iWidth * fZoom;  // on cherche a atteindre le nouveau point.
		}
		
		// on repositionne les caracteres et on anime tout ca.
		cd_do_launch_appearance_animation ();
		cairo_dock_redraw_container (CAIRO_CONTAINER (g_pMainDock));
	}
}
