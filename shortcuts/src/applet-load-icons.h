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


#ifndef __APPLET_LOAD_ICONS__
#define  __APPLET_LOAD_ICONS__


#include <cairo-dock.h>


void cd_shortcuts_set_icon_order_by_name (Icon *pNewIcon, GList *pIconsList);

void cd_shortcuts_get_shortcuts_data (CairoDockModuleInstance *myApplet);

gboolean cd_shortcuts_build_shortcuts_from_data (CairoDockModuleInstance *myApplet);


#endif
