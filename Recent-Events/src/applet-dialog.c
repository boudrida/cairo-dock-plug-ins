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
#define __USE_POSIX
#include <time.h>

#include "applet-struct.h"
#include "applet-search.h"
#include "applet-dialog.h"

static void _on_got_events (ZeitgeistResultSet *events, GtkListStore *pModel);

static void _trigger_search (void)
{
	const gchar *cQuery = gtk_entry_get_text (GTK_ENTRY (myData.pEntry));
	CDEventType iCategory = myData.iCurrentCaterogy;
	GtkListStore *pModel = myData.pModel;
	
	int iSortType = 0;
	if (iCategory >= CD_EVENT_TOP_RESULTS)
	{
		iCategory = CD_EVENT_ALL;
		iSortType = 1;
	}
	
	gtk_list_store_clear (pModel);
	if (cQuery != NULL && *cQuery != '\0')
		cd_search_events (cQuery, iCategory, (CDOnGetEventsFunc) _on_got_events, pModel);
	else
		cd_find_recent_events (iCategory, iSortType, (CDOnGetEventsFunc) _on_got_events, pModel);
}

static void on_click_category_button (GtkButton *button, gpointer data)
{
	if (! gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (button)))
		return;
	myData.iCurrentCaterogy = GPOINTER_TO_INT (data);
	g_print ("filter on category %d\n", myData.iCurrentCaterogy);
	_trigger_search ();
}

#if (GTK_MAJOR_VERSION > 2 || GTK_MINOR_VERSION >= 16)
static void on_clear_filter (GtkEntry *pEntry, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer data)
{
	gtk_entry_set_text (pEntry, "");
	g_print ("relaunch the search...\n");
	_trigger_search ();
}
#endif

static void on_activate_filter (GtkEntry *pEntry, gpointer data)
{
	_trigger_search ();
}

static void _on_got_events (ZeitgeistResultSet *pEvents, GtkListStore *pModel)
{
	int i, n;
	ZeitgeistEvent *event;
	ZeitgeistSubject *subject;
	gint64 iTimeStamp;
	const gchar *cEventURI;
	guint id;
	gchar *cName = NULL, *cURI = NULL, *cIconName = NULL, *cPath = NULL;
	double fOrder;
	int iVolumeID;
	gboolean bIsDirectory;
	GdkPixbuf *pixbuf;
	GtkTreeIter iter;
	GHashTable *pHashTable = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);
	
	//\_____________ parse all the events.
	while (zeitgeist_result_set_has_next (pEvents))
	{
		event = zeitgeist_result_set_next (pEvents);
		iTimeStamp = zeitgeist_event_get_timestamp (event) / 1e3;
		id = zeitgeist_event_get_id (event);
		n = zeitgeist_event_num_subjects (event);
		if (n > 1)
			g_print (" +++ %s, %s, %d\n", zeitgeist_event_get_interpretation (event), zeitgeist_event_get_manifestation (event), n);
		for (i = 0; i < n; i++)
		{
			subject = zeitgeist_event_get_subject (event, i);
			
			//\_____________ prevent doubles.
			cEventURI = zeitgeist_subject_get_uri (subject);
			if (g_hash_table_lookup_extended  (pHashTable, cEventURI, NULL, NULL))
				continue;
			//g_print ("  %s:\n    %s, %s\n", cEventURI, zeitgeist_subject_get_interpretation (subject), zeitgeist_subject_get_manifestation (subject));
			
			//\_____________ get the text to display.
			const gchar *cText = zeitgeist_subject_get_text (subject);
			if (cText == NULL)  // skip empty texts (they are most of the times web page that redirect to another page, which is probably in the next event anyway).
				continue;
			
			//\_____________ find the icon.
			if (strncmp (cEventURI, "http", 4) == 0)  // gvfs is deadly slow to get info on distant URI...
			{
				cIconName = cairo_dock_search_icon_s_path ("text-html");
			}
			else
			{
				cairo_dock_fm_get_file_info (cEventURI, &cName, &cURI, &cIconName, &bIsDirectory, &iVolumeID, &fOrder, CAIRO_DOCK_FM_SORT_BY_DATE);
				g_free (cName);
				g_free (cURI);
			}
			if (cIconName != NULL)
				pixbuf = gdk_pixbuf_new_from_file_at_size (cIconName, 32, 32, NULL);
			else
				pixbuf = NULL;
			
			//\_____________ build the path to display.
			cPath = g_filename_from_uri (cEventURI, NULL, NULL);  // NULL for anything else than file://*
			
			const gchar *cDisplayedPath = (cPath ? cPath : cEventURI);
			
			gchar *cEscapedPath = NULL;
			if (strchr (cDisplayedPath, '&'))  // need to escape the '&' because gtk-tooltips use markups by default.
			{
				cEscapedPath = g_new0 (gchar, 5*strlen(cDisplayedPath));
				const gchar *str = cDisplayedPath;
				gchar *str2 = cEscapedPath;
				while (*str != '\0')
				{
					if (*str == '&')
					{
						strcpy (str2, "&amp;");
						str2 += 5;
					}
					else
					{
						*str2 = *str;
						*str2 ++;
					}
					str ++;
				}
			}
			
			//\_____________ store in the model.
			memset (&iter, 0, sizeof (GtkTreeIter));
			gtk_list_store_append (GTK_LIST_STORE (pModel), &iter);
			gtk_list_store_set (GTK_LIST_STORE (pModel), &iter,
				CD_MODEL_NAME, cText,
				CD_MODEL_URI, cEventURI,
				CD_MODEL_PATH, cEscapedPath?cEscapedPath:cDisplayedPath,
				CD_MODEL_ICON, pixbuf,
				CD_MODEL_DATE, iTimeStamp,
				CD_MODEL_ID, id, -1);
			g_free (cIconName);
			if (pixbuf)
				g_object_unref (pixbuf);
			g_free (cPath);
			g_free (cEscapedPath);
			
			g_hash_table_insert (pHashTable, (gchar*)cEventURI, NULL);  // cEventURI stays valid in this function.
		}
	}
	g_hash_table_destroy (pHashTable);
}

void cd_folders_free_apps_list (CairoDockModuleInstance *myApplet)
{
	if (myData.pAppList != NULL)
	{
		g_list_foreach (myData.pAppList, (GFunc) g_free, NULL);
		g_list_free (myData.pAppList);
		myData.pAppList = NULL;
	}
}

static void _cd_launch_with (GtkMenuItem *pMenuItem, const gchar *cExec)
{
	gchar *cPath = g_filename_from_uri (myData.cCurrentUri, NULL, NULL);
	cairo_dock_launch_command_printf ("%s \"%s\"", NULL, cExec, cPath);  // in case the program doesn't handle URI (geeqie, etc).
	g_free (cPath);
}

static void _cd_open_parent (GtkMenuItem *pMenuItem, gpointer data)
{
	gchar *cFolder = g_path_get_dirname (myData.cCurrentUri);
	cairo_dock_fm_launch_uri (cFolder);
	g_free (cFolder);
}
static void _cd_copy_location (GtkMenuItem *pMenuItem, gpointer data)
{
	GtkClipboard *pClipBoard;
	pClipBoard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);  // GDK_SELECTION_PRIMARY
	gtk_clipboard_set_text (pClipBoard, myData.cCurrentUri, -1);
}
static void _on_event_deleted (gpointer data)
{
	_trigger_search ();
}
static void _cd_delete_event (GtkMenuItem *pMenuItem, gpointer data)
{
	guint32 id = GPOINTER_TO_UINT (data);
	cd_delete_event (id, _on_event_deleted, NULL);
}
static gboolean _on_click_module_tree_view (GtkTreeView *pTreeView, GdkEventButton* pButton, gpointer data)
{
	//g_print ("%s ()\n", __func__);
	if ((pButton->button == 3 && pButton->type == GDK_BUTTON_RELEASE)  // right-click
	|| (pButton->button == 1 && pButton->type == GDK_2BUTTON_PRESS))  // double-click
	{
		g_print ("%s ()\n", __func__);
		// get the current selected line.
		GtkTreeSelection *pSelection = gtk_tree_view_get_selection (pTreeView);
		GtkTreeModel *pModel;
		GtkTreeIter iter;
		if (! gtk_tree_selection_get_selected (pSelection, &pModel, &iter))
			return FALSE;
		
		gchar *cName = NULL, *cUri = NULL;
		guint id = 0;
		gtk_tree_model_get (pModel, &iter,
			CD_MODEL_NAME, &cName,
			CD_MODEL_URI, &cUri,
			CD_MODEL_ID, &id, -1);
		
		//launch or build the menu.
		if (pButton->button == 1)  // double-click
		{
			cairo_dock_fm_launch_uri (cUri);
			g_free (cUri);
		}
		else  // right-click
		{
			GtkWidget *pMenu = gtk_menu_new ();
			g_free (myData.cCurrentUri);
			myData.cCurrentUri = cUri;
			
			GList *pApps = cairo_dock_fm_list_apps_for_file (cUri);
			if (pApps != NULL)
			{
				GtkWidget *pSubMenu = CD_APPLET_ADD_SUB_MENU_WITH_IMAGE (D_("Open with"), pMenu, GTK_STOCK_OPEN);
				
				cd_folders_free_apps_list (myApplet);
				
				GList *a;
				gchar **pAppInfo;
				gchar *cIconPath;
				gpointer *app;
				for (a = pApps; a != NULL; a = a->next)
				{
					pAppInfo = a->data;
					myData.pAppList = g_list_prepend (myData.pAppList, pAppInfo[1]);
					
					if (pAppInfo[2] != NULL)
						cIconPath = cairo_dock_search_icon_s_path (pAppInfo[2]);
					else
						cIconPath = NULL;
					CD_APPLET_ADD_IN_MENU_WITH_STOCK_AND_DATA (pAppInfo[0], cIconPath, _cd_launch_with, pSubMenu, pAppInfo[1]);
					g_free (cIconPath);
					g_free (pAppInfo[0]);
					g_free (pAppInfo[2]);
					g_free (pAppInfo);
				}
				g_list_free (pApps);
			}
			CD_APPLET_ADD_IN_MENU_WITH_STOCK_AND_DATA (D_("Open parent folder"), GTK_STOCK_DIRECTORY, _cd_open_parent, pMenu, NULL);
			
			CD_APPLET_ADD_IN_MENU_WITH_STOCK_AND_DATA (D_("Copy the location"), GTK_STOCK_COPY, _cd_copy_location, pMenu, NULL);
			
			CD_APPLET_ADD_IN_MENU_WITH_STOCK_AND_DATA (D_("Delete this event"), GTK_STOCK_REMOVE, _cd_delete_event, pMenu, GUINT_TO_POINTER (id));
			
			gtk_widget_show_all (pMenu);
			gtk_menu_popup (GTK_MENU (pMenu),
				NULL,
				NULL,
				NULL,  // popup on mouse.
				NULL,
				1,
				gtk_get_current_event_time ());
		}
	}
	return FALSE;
}

static gboolean _cairo_dock_select_one_item_in_tree (GtkTreeSelection * selection, GtkTreeModel * model, GtkTreePath * path, gboolean path_currently_selected, gpointer *data)
{
	if (path_currently_selected)
		return TRUE;
	GtkTreeIter iter;
	if (! gtk_tree_model_get_iter (model, &iter, path))
		return FALSE;
	
	return TRUE;
}

#define DATE_BUFFER_LENGTH 50
static void _render_date (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *model,GtkTreeIter *iter, gpointer data)
{
	gint64 iDate = 0;
	gtk_tree_model_get (model, iter, CD_MODEL_DATE, &iDate, -1);
	
	time_t epoch = iDate;
	struct tm t;
	localtime_r (&epoch, &t);
	
	static gchar s_cDateBuffer[50];
	const gchar *cFormat;
	if (myConfig.b24Mode)
		cFormat = "%a %d %b, %R";
	else
		cFormat = "%a %d %b, %I:%M %p";
	strftime (s_cDateBuffer, DATE_BUFFER_LENGTH, cFormat, &t);
	
	g_object_set (cell, "text", s_cDateBuffer, NULL);
}

static inline GtkToolItem *_add_category_button (GtkWidget *pToolBar, const gchar *cLabel, const gchar *cIconName, int pos, GtkToolItem *group)
{
	GtkToolItem *pCategoryButton;
	if (group)
		pCategoryButton= gtk_radio_tool_button_new_from_widget (GTK_RADIO_TOOL_BUTTON (group));
	else
		pCategoryButton = gtk_radio_tool_button_new (NULL);
	gtk_tool_button_set_label (GTK_TOOL_BUTTON (pCategoryButton), cLabel);
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (pCategoryButton), cIconName);
	g_signal_connect (G_OBJECT (pCategoryButton), "toggled", G_CALLBACK(on_click_category_button), GINT_TO_POINTER (pos));
	gtk_toolbar_insert (GTK_TOOLBAR (pToolBar) , pCategoryButton, -1);
	return pCategoryButton;
}
#define MARGIN 3
static GtkWidget *cd_build_events_widget (void)
{
	GtkWidget *pMainBox = gtk_vbox_new (FALSE, MARGIN);
	
	// category toolbar.
	GtkWidget *pToolBar = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (pToolBar), GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style (GTK_TOOLBAR (pToolBar), GTK_TOOLBAR_BOTH);  // overwrite system preference (GTK_TOOLBAR_ICONS)
	gtk_toolbar_set_show_arrow (GTK_TOOLBAR (pToolBar), FALSE);  // force to display all the entries.
	gtk_box_pack_start (GTK_BOX (pMainBox), pToolBar, TRUE, TRUE, MARGIN);
	
	int i = 0;
	GtkToolItem *group = _add_category_button (pToolBar, D_("All"), "stock_all", i++, NULL);
	_add_category_button (pToolBar, D_("Document"), "document", i++, group);
	///_add_category_button (pToolBar, D_("Folder"), "folder", i++, group);
	_add_category_button (pToolBar, D_("Image"), "image", i++, group);
	_add_category_button (pToolBar, D_("Audio"), "sound", i++, group);
	_add_category_button (pToolBar, D_("Video"), "video", i++, group);
	_add_category_button (pToolBar, D_("Web"), "text-html", i++, group);
	_add_category_button (pToolBar, D_("Other"), "unknown", i++, group);
	_add_category_button (pToolBar, D_("Top Results"), "gtk-about", i++, group);
	
	// search entry.
	GtkWidget *pFilterBox = gtk_hbox_new (FALSE, CAIRO_DOCK_GUI_MARGIN);
	gtk_box_pack_start (GTK_BOX (pMainBox), pFilterBox, FALSE, FALSE, MARGIN);
	
	GtkWidget *pFilterLabel = gtk_label_new (D_("Look for events"));
	gtk_box_pack_start (GTK_BOX (pFilterBox), pFilterLabel, FALSE, FALSE, MARGIN);
	
	GtkWidget *pEntry = gtk_entry_new ();
	g_signal_connect (pEntry, "activate", G_CALLBACK (on_activate_filter), NULL);
	gtk_box_pack_start (GTK_BOX (pFilterBox), pEntry, FALSE, FALSE, MARGIN);
	gtk_widget_set_tooltip_text (pEntry, "The default boolean operator is AND. Thus the query foo bar will be interpreted as foo AND bar. To exclude a term from the result set prepend it with a minus sign - eg foo -bar. Phrase queries can be done by double quoting the string \"foo is a bar\". You can truncate terms by appending a *. ");
	
	#if (GTK_MAJOR_VERSION > 2 || GTK_MINOR_VERSION >= 16)
	gtk_entry_set_icon_activatable (GTK_ENTRY (pEntry), GTK_ENTRY_ICON_SECONDARY, TRUE);
	gtk_entry_set_icon_from_stock (GTK_ENTRY (pEntry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
	g_signal_connect (pEntry, "icon-press", G_CALLBACK (on_clear_filter), NULL);
	#endif
	myData.pEntry = pEntry;
	gtk_widget_grab_focus (pEntry);
	
	// model
	GtkListStore *pModel = gtk_list_store_new (CD_MODEL_NB_COLUMNS,
		G_TYPE_STRING,  /* CD_MODEL_NAME */
		G_TYPE_STRING,  /* CD_MODEL_URI */
		G_TYPE_STRING,  /* CD_MODEL_PATH */
		GDK_TYPE_PIXBUF,  /* CD_MODEL_ICON */
		G_TYPE_INT64,  /* CD_MODEL_DATE */
		G_TYPE_UINT);  /* CD_MODEL_ID */
	///gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (pModel), CD_MODEL_NAME, GTK_SORT_ASCENDING);
	myData.pModel = pModel;
	
	// tree-view
	GtkWidget *pOneWidget = gtk_tree_view_new ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (pOneWidget), GTK_TREE_MODEL (pModel));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (pOneWidget), TRUE);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (pOneWidget), TRUE);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (pOneWidget));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	gtk_tree_selection_set_select_function (selection,
		(GtkTreeSelectionFunc) _cairo_dock_select_one_item_in_tree,
		NULL,
		NULL);
	g_signal_connect (G_OBJECT (pOneWidget), "button-release-event", G_CALLBACK (_on_click_module_tree_view), NULL);  // pour le menu du clic droit
	g_signal_connect (G_OBJECT (pOneWidget), "button-press-event", G_CALLBACK (_on_click_module_tree_view), NULL);  // pour le menu du clic droit
	
	g_object_set (G_OBJECT (pOneWidget), "tooltip-column", CD_MODEL_PATH, NULL);
	
	GtkTreeViewColumn* col;
	GtkCellRenderer *rend;
	// icon
	rend = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (pOneWidget), -1, NULL, rend, "pixbuf", CD_MODEL_ICON, NULL);
	// file name
	rend = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("File name"), rend, "text", CD_MODEL_NAME, NULL);
	gtk_tree_view_column_set_max_width (col, MAX (600, g_desktopGeometry.iScreenWidth[CAIRO_DOCK_HORIZONTAL]*.67));
	gtk_tree_view_column_set_sort_column_id (col, CD_MODEL_NAME);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pOneWidget), col);
	// date
	rend = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Last access"), rend, "text", CD_MODEL_DATE, NULL);
	gtk_tree_view_column_set_cell_data_func (col, rend, (GtkTreeCellDataFunc)_render_date, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id (col, CD_MODEL_DATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pOneWidget), col);
	
	// barres de defilement
	GtkObject *adj = gtk_adjustment_new (0., 0., 100., 1, 10, 10);
	gtk_tree_view_set_vadjustment (GTK_TREE_VIEW (pOneWidget), GTK_ADJUSTMENT (adj));
	GtkWidget *pScrolledWindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set (pScrolledWindow, "height-request", 300, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pScrolledWindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (pScrolledWindow), pOneWidget);
	gtk_box_pack_start (GTK_BOX (pMainBox), pScrolledWindow, FALSE, FALSE, MARGIN);

	return pMainBox;
}

static void _on_dialog_destroyed (gpointer data)
{
	myData.pDialog = NULL;
	myData.pEntry = NULL;
	myData.iCurrentCaterogy = CD_EVENT_ALL;
	myData.pModel = NULL;	
}
void cd_toggle_dialog (void)
{
	if (myData.pDialog != NULL)
	{
		cairo_dock_dialog_unreference (myData.pDialog);
		myData.pDialog = NULL;
	}
	else
	{
		GtkWidget *pInteractiveWidget = cd_build_events_widget ();
		myData.pDialog = cairo_dock_show_dialog_full (D_("Browse and search in recent events"), myIcon, myContainer, 0, "same icon", pInteractiveWidget, NULL, myApplet, (GFreeFunc) _on_dialog_destroyed);
		gtk_widget_grab_focus (myData.pEntry);
		
		_trigger_search ();
	}
}