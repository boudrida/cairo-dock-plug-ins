/******************************************************************************

This file is a part of the cairo-dock program,
released under the terms of the GNU General Public License.

Written by Christophe Chapuis (for any bug report, please mail me to tofe@users.berlios.de)

******************************************************************************/

#include <string.h>
#include <cairo-dock.h>
#include <libetpan/libetpan.h>

#include "cd-mail-applet-struct.h"
#include "cd-mail-applet-etpan.h"

enum {
  POP3_STORAGE = 1,
  IMAP_STORAGE,
  NNTP_STORAGE,
  MBOX_STORAGE,
  MH_STORAGE,
  MAILDIR_STORAGE,
  FEED_STORAGE
};

#define _add_icon(pMailAccount)\
	if (pMailAccount->name != NULL)\
	{\
		pIcon = g_new0 (Icon, 1);\
		pIcon->acName = g_strdup (pMailAccount->name);\
		pIcon->acFileName = g_strdup (pMailAccount->iNbUnseenMails > 0 ? myConfig.cHasMailUserImage : myConfig.cNoMailUserImage);\
		if (pMailAccount->iNbUnseenMails>0)\
			pIcon->cQuickInfo = g_strdup_printf ("%d", pMailAccount->iNbUnseenMails);\
		else\
			pIcon->cQuickInfo = g_strdup ("...");\
		pIcon->fOrder = i;\
		pIcon->fScale = 1.;\
		pIcon->fAlpha = 1.;\
		pIcon->fWidthFactor = 1.;\
		pIcon->fHeightFactor = 1.;\
		pIcon->acCommand = g_strdup ("none");\
		pIcon->cParentDockName = g_strdup (myIcon->acName);\
		cd_debug (" + %s (%s)\n", pIcon->acName, pIcon->acFileName);\
		pIconList = g_list_append (pIconList, pIcon);\
		pMailAccount->icon = pIcon;\
	}

void cd_mail_create_pop3_params( GKeyFile *pKeyFile, gchar *pMailAccountName )
{
	g_key_file_set_string (pKeyFile, pMailAccountName, "type", "pop3");

	g_key_file_set_string (pKeyFile, pMailAccountName, "host", "pop3.myhost.org");
	g_key_file_set_comment (pKeyFile, pMailAccountName, "host", "s0 server address:", NULL);

	g_key_file_set_string (pKeyFile, pMailAccountName, "username", "myLogin");
	g_key_file_set_comment (pKeyFile, pMailAccountName, "username", "s0 username:", NULL);

	g_key_file_set_string (pKeyFile, pMailAccountName, "password", "");
	g_key_file_set_comment (pKeyFile, pMailAccountName, "password", "p0 password:\n{The password will be crypted.}", NULL);

	g_key_file_set_integer (pKeyFile, pMailAccountName, "port", 0);
	g_key_file_set_comment (pKeyFile, pMailAccountName, "port", "i0 port:\n{Enter 0 to use the default port. Default ports are 110 for POP3 or APOP and 995 for POP3S.}", NULL);

	g_key_file_set_boolean (pKeyFile, pMailAccountName, "use secure connection", FALSE);
	g_key_file_set_comment (pKeyFile, pMailAccountName, "use secure connection", "b0 use secure connection (SSL)", NULL);

	g_key_file_set_integer (pKeyFile, pMailAccountName, "timeout mn", 10);
	g_key_file_set_comment (pKeyFile, pMailAccountName, "timeout mn", "I0[1;30] timeout:\n{In minutes.}", NULL);
}

void cd_mail_retrieve_pop3_params (CDMailAccount *mailaccount, GKeyFile *pKeyFile, gchar *mailbox_name)
{
  if( !mailaccount || !pKeyFile || !mailbox_name ) return;

  gboolean bFlushConfFileNeeded = FALSE;

  mailaccount->driver = POP3_STORAGE;
  mailaccount->storage = mailstorage_new(NULL);
  mailaccount->folder = NULL;
  mailaccount->server = NULL;
  mailaccount->port = 0;
  mailaccount->connection_type = CONNECTION_TYPE_PLAIN;
  mailaccount->user = NULL;
  mailaccount->password = NULL;
  mailaccount->auth_type = POP3_AUTH_TYPE_TRY_APOP;
  mailaccount->path = NULL;
  mailaccount->timeout = 0;
  
  if (g_key_file_has_key (pKeyFile, mailbox_name, "host", NULL))
  {
    mailaccount->server = CD_CONFIG_GET_STRING (mailbox_name, "host");
  }
  if (g_key_file_has_key (pKeyFile, mailbox_name, "username", NULL))
  {
    mailaccount->user = CD_CONFIG_GET_STRING (mailbox_name, "username");
  }
  if (g_key_file_has_key (pKeyFile, mailbox_name, "password", NULL))
  {
    gchar *encryptedPassword = CD_CONFIG_GET_STRING (mailbox_name, "password");
    cairo_dock_decrypt_string( encryptedPassword,  &(mailaccount->password) );

    if( encryptedPassword ) g_free(encryptedPassword);
  }
  mailaccount->connection_type = CD_CONFIG_GET_BOOLEAN_WITH_DEFAULT (mailbox_name, "use secure connection", FALSE)?CONNECTION_TYPE_TLS:CONNECTION_TYPE_PLAIN;
  mailaccount->port = CD_CONFIG_GET_INTEGER_WITH_DEFAULT (mailbox_name, "port", 0);

  mailaccount->timeout = CD_CONFIG_GET_INTEGER_WITH_DEFAULT (mailbox_name, "timeout mn", 10);
}

void cd_mail_create_imap_params( GKeyFile *pKeyFile, gchar *pMailAccountName )
{
	g_key_file_set_string (pKeyFile, pMailAccountName, "type", "imap");

	g_key_file_set_string (pKeyFile, pMailAccountName, "host", "imap.myhost.org");
	g_key_file_set_comment (pKeyFile, pMailAccountName, "host", "s0 server address:", NULL);

	g_key_file_set_string (pKeyFile, pMailAccountName, "username", "myLogin");
	g_key_file_set_comment (pKeyFile, pMailAccountName, "username", "s0 username:", NULL);

	g_key_file_set_string (pKeyFile, pMailAccountName, "password", "");
	g_key_file_set_comment (pKeyFile, pMailAccountName, "password", "p0 password:", NULL);

	g_key_file_set_integer (pKeyFile, pMailAccountName, "port", 0);
	g_key_file_set_comment (pKeyFile, pMailAccountName, "port", "i0 port:\n{Enter 0 to use the default port. Default ports are 143 for IMAP4 and 993 for IMAP4 over SSL.}", NULL);

	g_key_file_set_boolean (pKeyFile, pMailAccountName, "use secure connection", FALSE);
	g_key_file_set_comment (pKeyFile, pMailAccountName, "use secure connection", "b0 use secure connection (SSL)", NULL);

	g_key_file_set_string (pKeyFile, pMailAccountName, "server_directory", "Inbox");
	g_key_file_set_comment (pKeyFile, pMailAccountName, "server_directory", "s0 directory on server:", NULL);

	g_key_file_set_integer (pKeyFile, pMailAccountName, "timeout mn", 10);
	g_key_file_set_comment (pKeyFile, pMailAccountName, "timeout mn", "I0[1;30] timeout:\n{In minutes.}", NULL);
}

void cd_mail_retrieve_imap_params (CDMailAccount *mailaccount, GKeyFile *pKeyFile, gchar *mailbox_name)
{
  if( !mailaccount || !pKeyFile || !mailbox_name ) return;

  gboolean bFlushConfFileNeeded = FALSE;

  mailaccount->driver = IMAP_STORAGE;
  mailaccount->storage = mailstorage_new(NULL);
  mailaccount->folder = NULL;
  mailaccount->server = NULL;
  mailaccount->port = 0;
  mailaccount->connection_type = CONNECTION_TYPE_PLAIN;
  mailaccount->user = NULL;
  mailaccount->password = NULL;
  mailaccount->auth_type = IMAP_AUTH_TYPE_PLAIN;
  mailaccount->path = g_strdup("/");
  mailaccount->timeout = 0;
  
  if (g_key_file_has_key (pKeyFile, mailbox_name, "host", NULL))
  {
    mailaccount->server = CD_CONFIG_GET_STRING (mailbox_name, "host");
  }
  if (g_key_file_has_key (pKeyFile, mailbox_name, "username", NULL))
  {
    mailaccount->user = CD_CONFIG_GET_STRING (mailbox_name, "username");
  }
  if (g_key_file_has_key (pKeyFile, mailbox_name, "password", NULL))
  {
    gchar *encryptedPassword = CD_CONFIG_GET_STRING (mailbox_name, "password");
    cairo_dock_decrypt_string( encryptedPassword,  &(mailaccount->password) );

    if( encryptedPassword ) g_free(encryptedPassword);
  }
  mailaccount->timeout = CD_CONFIG_GET_INTEGER_WITH_DEFAULT (mailbox_name, "timeout mn", 10);
  mailaccount->port = CD_CONFIG_GET_INTEGER_WITH_DEFAULT (mailbox_name, "port", 0);

  mailaccount->connection_type = CD_CONFIG_GET_BOOLEAN_WITH_DEFAULT (mailbox_name, "use secure connection", FALSE)?CONNECTION_TYPE_TLS:CONNECTION_TYPE_PLAIN;

  /* CONNECTION_TYPE_TLS ? CONNECTION_TYPE_STARTTLS ? */

  if (g_key_file_has_key (pKeyFile, mailbox_name, "server_directory", NULL))
  {
    mailaccount->path = CD_CONFIG_GET_STRING (mailbox_name, "server_directory");
  }
}

void cd_mail_create_mbox_params( GKeyFile *pKeyFile, gchar *pMailAccountName )
{
	g_key_file_set_string (pKeyFile, pMailAccountName, "type", "mbox");

	g_key_file_set_string (pKeyFile, pMailAccountName, "filename", "");
	g_key_file_set_comment (pKeyFile, pMailAccountName, "filename", "s0 path of mbox file:", NULL);

	g_key_file_set_integer (pKeyFile, pMailAccountName, "timeout mn", 10);
	g_key_file_set_comment (pKeyFile, pMailAccountName, "timeout mn", "I0[1;30] timeout:\n{In minutes.}", NULL);
}

void cd_mail_retrieve_mbox_params (CDMailAccount *mailaccount, GKeyFile *pKeyFile, gchar *mailbox_name)
{
	if( !mailaccount || !pKeyFile || !mailbox_name ) return;

	gboolean bFlushConfFileNeeded = FALSE;

	mailaccount->driver = MBOX_STORAGE;
	mailaccount->storage = mailstorage_new(NULL);
	mailaccount->folder = NULL;
	mailaccount->server = NULL;
	mailaccount->port = 0;
	mailaccount->connection_type = CONNECTION_TYPE_PLAIN;
	mailaccount->user = NULL;
	mailaccount->password = NULL;
	mailaccount->auth_type = POP3_AUTH_TYPE_PLAIN;
	mailaccount->timeout = 0;
	if (g_key_file_has_key (pKeyFile, mailbox_name, "filename", NULL))
		mailaccount->path = CD_CONFIG_GET_STRING_WITH_DEFAULT (mailbox_name, "filename", "/");
	if (mailaccount->path == NULL)
		mailaccount->path = g_strdup("/");
	mailaccount->timeout = CD_CONFIG_GET_INTEGER_WITH_DEFAULT (mailbox_name, "timeout mn", 10);

	//{"filename", "ctime", "size", "interval", NULL, NULL}
}

void cd_mail_create_mh_params( GKeyFile *pKeyFile, gchar *pMailAccountName )
{
	g_key_file_set_string (pKeyFile, pMailAccountName, "type", "mh");

	g_key_file_set_integer (pKeyFile, pMailAccountName, "timeout mn", 10);
	g_key_file_set_comment (pKeyFile, pMailAccountName, "timeout mn", "I0[1;30] timeout:\n{In minutes.}", NULL);
}

void cd_mail_retrieve_mh_params (CDMailAccount *mailaccount, GKeyFile *pKeyFile, gchar *mailbox_name)
{
  if( !mailaccount || !pKeyFile || !mailbox_name ) return;

  gboolean bFlushConfFileNeeded = FALSE;

  mailaccount->driver = MH_STORAGE;
  mailaccount->storage = mailstorage_new(NULL);
  mailaccount->folder = NULL;
  mailaccount->server = NULL;
  mailaccount->port = 0;
  mailaccount->connection_type = CONNECTION_TYPE_PLAIN;
  mailaccount->user = NULL;
  mailaccount->password = NULL;
  mailaccount->auth_type = POP3_AUTH_TYPE_PLAIN;
  mailaccount->path = g_strdup("/");
  mailaccount->timeout = 0;

  mailaccount->timeout = CD_CONFIG_GET_INTEGER_WITH_DEFAULT (mailbox_name, "timeout mn", 10);
}

void cd_mail_create_maildir_params( GKeyFile *pKeyFile, gchar *pMailAccountName )
{
	g_key_file_set_string (pKeyFile, pMailAccountName, "type", "maildir");

	g_key_file_set_string (pKeyFile, pMailAccountName, "path", "");
	g_key_file_set_comment (pKeyFile, pMailAccountName, "path", "s0 path to mail directory:", NULL);

	g_key_file_set_integer (pKeyFile, pMailAccountName, "timeout mn", 10);
	g_key_file_set_comment (pKeyFile, pMailAccountName, "timeout mn", "I0[1;30] timeout:\n{In minutes.}", NULL);
}

void cd_mail_retrieve_maildir_params (CDMailAccount *mailaccount, GKeyFile *pKeyFile, gchar *mailbox_name)
{
  if( !mailaccount || !pKeyFile || !mailbox_name ) return;

  gboolean bFlushConfFileNeeded = FALSE;

  mailaccount->driver = MAILDIR_STORAGE;
  mailaccount->storage = mailstorage_new(NULL);
  mailaccount->folder = NULL;
  mailaccount->server = NULL;
  mailaccount->port = 0;
  mailaccount->connection_type = CONNECTION_TYPE_PLAIN;
  mailaccount->user = NULL;
  mailaccount->password = NULL;
  mailaccount->auth_type = POP3_AUTH_TYPE_PLAIN;
  mailaccount->path = g_strdup("/");
  mailaccount->timeout = 0;

  if (g_key_file_has_key (pKeyFile, mailbox_name, "path", NULL))
  {
    mailaccount->path = CD_CONFIG_GET_STRING (mailbox_name, "path");
  }
  mailaccount->timeout = CD_CONFIG_GET_INTEGER_WITH_DEFAULT (mailbox_name, "timeout mn", 10);

  //{"path", "mtime", "interval", NULL, NULL, NULL, NULL}
}

void cd_mail_create_gmail_params( GKeyFile *pKeyFile, gchar *pMailAccountName )
{
	g_key_file_set_string (pKeyFile, pMailAccountName, "type", "gmail");

	g_key_file_set_string (pKeyFile, pMailAccountName, "username", "myLogin");
	g_key_file_set_comment (pKeyFile, pMailAccountName, "username", "s0 username:", NULL);

	g_key_file_set_string (pKeyFile, pMailAccountName, "password", "");
	g_key_file_set_comment (pKeyFile, pMailAccountName, "password", "p0 password:", NULL);

	g_key_file_set_integer (pKeyFile, pMailAccountName, "timeout mn", 10);
	g_key_file_set_comment (pKeyFile, pMailAccountName, "timeout mn", "I0[1;30] timeout:\n{In minutes.}", NULL);
}

void cd_mail_retrieve_gmail_params (CDMailAccount *mailaccount, GKeyFile *pKeyFile, gchar *mailbox_name)
{
  if( !mailaccount || !pKeyFile || !mailbox_name ) return;

  gboolean bFlushConfFileNeeded = FALSE;

  mailaccount->driver = FEED_STORAGE;
  mailaccount->storage = mailstorage_new(NULL);
  mailaccount->folder = NULL;
  mailaccount->server = NULL;
  mailaccount->port = 443;
  mailaccount->connection_type = CONNECTION_TYPE_PLAIN;
  mailaccount->user = NULL;
  mailaccount->password = NULL;
  mailaccount->auth_type = POP3_AUTH_TYPE_PLAIN;
  mailaccount->path = NULL;
  mailaccount->timeout = 0;
  
  if (g_key_file_has_key (pKeyFile, mailbox_name, "username", NULL))
  {
    mailaccount->user = CD_CONFIG_GET_STRING (mailbox_name, "username");
  }
  if (g_key_file_has_key (pKeyFile, mailbox_name, "password", NULL))
  {
    gchar *encryptedPassword = CD_CONFIG_GET_STRING (mailbox_name, "password");
    cairo_dock_decrypt_string( encryptedPassword,  &(mailaccount->password) );

    if( encryptedPassword ) g_free(encryptedPassword);
  }

  gchar *user_without_column = NULL;
  gchar *password_without_column = NULL;

  if( mailaccount->user )
  {
    gchar **splitString = g_strsplit(mailaccount->user, ":", 0);
    user_without_column = g_strjoinv("%3A", splitString);
    g_strfreev( splitString );
  }
  if( mailaccount->password )
  {
    gchar **splitString = g_strsplit(mailaccount->password, ":", 0);
    password_without_column = g_strjoinv("%3A", splitString);
    g_strfreev( splitString );
  }

  if( user_without_column && password_without_column )
  {
    mailaccount->path = g_strconcat("https://", user_without_column, ":", password_without_column, "@mail.google.com/mail/feed/atom", NULL);
  }
  else
  {
    mailaccount->path = g_strdup( "https://mail.google.com/mail/feed/atom" );
  }
  mailaccount->timeout = CD_CONFIG_GET_INTEGER_WITH_DEFAULT (mailbox_name, "timeout mn", 10);

  g_free( user_without_column );
  g_free( password_without_column );
}

void cd_mail_create_feed_params( GKeyFile *pKeyFile, gchar *pMailAccountName )
{
	g_key_file_set_string (pKeyFile, pMailAccountName, "type", "feed");

	g_key_file_set_string (pKeyFile, pMailAccountName, "path", "http://www.cairo-dock.org/rss/cd_svn.xml");
	g_key_file_set_comment (pKeyFile, pMailAccountName, "path", "s0 address of feed:", NULL);

	g_key_file_set_integer (pKeyFile, pMailAccountName, "timeout mn", 10);
	g_key_file_set_comment (pKeyFile, pMailAccountName, "timeout mn", "I0[1;30] timeout:\n{In minutes.}", NULL);
}

void cd_mail_retrieve_feed_params (CDMailAccount *mailaccount, GKeyFile *pKeyFile, gchar *mailbox_name)
{
  if( !mailaccount || !pKeyFile || !mailbox_name ) return;

  extern int mailstream_debug;
  mailstream_debug = 1;

  gboolean bFlushConfFileNeeded = FALSE;

  mailaccount->driver = FEED_STORAGE;
  mailaccount->storage = mailstorage_new(NULL);
  mailaccount->folder = NULL;
  mailaccount->server = NULL;
  mailaccount->port = 443;
  mailaccount->connection_type = CONNECTION_TYPE_PLAIN;
  mailaccount->user = NULL;
  mailaccount->password = NULL;
  mailaccount->auth_type = POP3_AUTH_TYPE_PLAIN;
  mailaccount->path = NULL;
  mailaccount->timeout = 0;
  
  if (g_key_file_has_key (pKeyFile, mailbox_name, "path", NULL))
  {
    mailaccount->path = CD_CONFIG_GET_STRING (mailbox_name, "path");
  }
  mailaccount->timeout = CD_CONFIG_GET_INTEGER_WITH_DEFAULT (mailbox_name, "timeout mn", 10);
}

/*
{
  {POP3_STORAGE, "pop3", {"host", "username", "password", "auth_type", "timeout mn", "port", NULL}},
  {IMAP_STORAGE, "imap", {"host", "username", "password", "auth_type", "timeout mn", "port", "server_directory"}},
  {NNTP_STORAGE, "nntp", {NULL, NULL, NULL, NULL, NULL, NULL, NULL}},
  {MBOX_STORAGE, "mbox", {"filename", "ctime", "size", "interval", NULL, NULL}},
  {MH_STORAGE, "mh", {"timeout mn", NULL, NULL, NULL, NULL, NULL, NULL}},
  {MAILDIR_STORAGE, "maildir", {"path", "mtime", "interval", NULL, NULL, NULL, NULL}},
  {FEED_STORAGE, "feed", {"username", "password", "timeout mn", NULL, NULL, NULL, NULL}},
  {FEED_STORAGE, "gmail", {"username", "password", "timeout mn", NULL, NULL, NULL, NULL}},
};
*/


void cd_mail_init_accounts(CairoDockModuleInstance *myApplet)
{	
	if (myData.pMailAccounts == NULL)
		return ;
	g_print ("%s (%d comptes)\n", __func__, myData.pMailAccounts->len);
	
	CDMailAccount *pMailAccount;
  	int i, r;
	for (i = 0; i < myData.pMailAccounts->len; i ++)
	{
		pMailAccount = g_ptr_array_index (myData.pMailAccounts, i);
		if( !pMailAccount ) continue;
		
		// init this account
		switch (pMailAccount->driver) {
		case POP3_STORAGE:
		r = pop3_mailstorage_init(pMailAccount->storage, pMailAccount->server, pMailAccount->port,
			NULL, pMailAccount->connection_type,
			pMailAccount->auth_type, pMailAccount->user, pMailAccount->password,
			FALSE /*cached*/, NULL /*cache_directory*/, NULL /*flags_directory*/);
		break;

		case IMAP_STORAGE:
		r = imap_mailstorage_init(pMailAccount->storage, pMailAccount->server, pMailAccount->port,
			NULL, pMailAccount->connection_type,
			IMAP_AUTH_TYPE_PLAIN, pMailAccount->user, pMailAccount->password,
			FALSE /*cached*/, NULL /*cache_directory*/);
			break;

		case NNTP_STORAGE:
		r = nntp_mailstorage_init(pMailAccount->storage, pMailAccount->server, pMailAccount->port,
			NULL, pMailAccount->connection_type,
			NNTP_AUTH_TYPE_PLAIN, pMailAccount->user, pMailAccount->password,
			FALSE /*cached*/, NULL /*cache_directory*/, NULL /*flags_directory*/);
			break;

		case MBOX_STORAGE:
		r = mbox_mailstorage_init(pMailAccount->storage, pMailAccount->path,
			FALSE /*cached*/, NULL /*cache_directory*/, NULL /*flags_directory*/);
			break;

		case MH_STORAGE:
		r = mh_mailstorage_init(pMailAccount->storage, pMailAccount->path,
			FALSE /*cached*/, NULL /*cache_directory*/, NULL /*flags_directory*/);
			break;
	        
		case MAILDIR_STORAGE:
		r = maildir_mailstorage_init(pMailAccount->storage, pMailAccount->path,
			FALSE /*cached*/, NULL /*cache_directory*/, NULL /*flags_directory*/);
			break;
	        
		case FEED_STORAGE:
		r = feed_mailstorage_init(pMailAccount->storage, pMailAccount->path,
			FALSE /*cached*/, NULL /*cache_directory*/, NULL /*flags_directory*/);
			break;
		}

		//  if all is OK, then set a timeout for this mail account
		if (r == MAIL_NO_ERROR)
		{
			pMailAccount->pAccountMailTimer = cairo_dock_new_measure_timer (pMailAccount->timeout * 60,
				cd_mail_acquire_folder_data,
				cd_mail_read_folder_data,
				cd_mail_update_account_status,
				pMailAccount);
			cairo_dock_launch_measure (pMailAccount->pAccountMailTimer);
		}
		else
		{
			cd_warning ("mail : the mail account %s couldn't be initialized !", pMailAccount->name);
		}
	}
}

void cd_mail_free_account (CDMailAccount *pMailAccount)
{
	if (pMailAccount == NULL)
		return ;
	
	cairo_dock_free_measure_timer( pMailAccount->pAccountMailTimer );
	
	g_free( pMailAccount->name );
	g_free( pMailAccount->server );
	g_free( pMailAccount->user );
	g_free( pMailAccount->password );
	g_free( pMailAccount->path );

	if( pMailAccount->folder )
		mailfolder_free(pMailAccount->folder);
	if( pMailAccount->storage )
		mailstorage_free(pMailAccount->storage);

	g_free( pMailAccount );
}

void cd_mail_free_all_accounts (CairoDockModuleInstance *myApplet)
{
	if (myData.pMailAccounts == NULL)
		return ;
	CDMailAccount *pMailAccount;
	guint i;
	for (i = 0; i < myData.pMailAccounts->len; i ++)
	{
		pMailAccount = g_ptr_array_index (myData.pMailAccounts, i);
		cd_mail_free_account (pMailAccount);
	}
	g_ptr_array_free (myData.pMailAccounts, TRUE);
	myData.pMailAccounts = NULL;
}



void cd_mail_acquire_folder_data(CDMailAccount *pMailAccount)
{
	if( ! pMailAccount )
		return ;
	int r = 0;

	pMailAccount->dirtyfied = FALSE;

	/* get the folder structure */

	// create the folder, if not yet done
	if( pMailAccount->folder == NULL )
	{
		pMailAccount->folder = mailfolder_new(pMailAccount->storage, pMailAccount->name, NULL);
	}

	if( pMailAccount->storage && pMailAccount->folder )
	{
		/* Ensure the connection is alive */
		r = mailfolder_connect(pMailAccount->folder);
	    
		/* Fix initialization for feed storage */
		if( pMailAccount->driver == FEED_STORAGE )
		{
			if( pMailAccount->folder && pMailAccount->folder->fld_session && pMailAccount->folder->fld_session->sess_data )
				((struct feed_session_state_data *) (pMailAccount->folder->fld_session->sess_data))->feed_last_update = (time_t) -1;
		}

		/* retrieve the stats */
		if (r == MAIL_NO_ERROR)
		{
			uint32_t result_messages;
			uint32_t result_recent;
			uint32_t result_unseen;
		    
			//if( MAIL_NO_ERROR == mailsession_unseen_number(pMailAccount->folder->fld_session, pMailAccount->name, &result_unseen) )
			if( MAIL_NO_ERROR == mailfolder_status(pMailAccount->folder,
													&result_messages, &result_recent, &result_unseen) )
			{
				if( pMailAccount->iNbUnseenMails != (guint)result_unseen )
				{
					pMailAccount->iNbUnseenMails = (guint)result_unseen;
					pMailAccount->dirtyfied = TRUE;
				}
			}

			cd_debug( "result_messages = %d, result_recent = %d, result_unseen = %d", result_messages, result_recent, result_unseen );

			mailfolder_disconnect(pMailAccount->folder);
			mailstorage_disconnect(pMailAccount->storage);
		}
	}
}

/*
 * Extraire les donnees des mails (nombre, titres, resume eventuellement)
 * et les placer dans une structure dediee a l'affichage
 */
void cd_mail_read_folder_data(CDMailAccount *pMailAccount)
{
	if( !pMailAccount ) return;
	CairoDockModuleInstance *myApplet = pMailAccount->pAppletInstance;
	  
	if( pMailAccount->dirtyfied == TRUE )
	{
		cd_debug( "cd_mail: The mailbox %s has changed", pMailAccount->name );
		myData.bNewMailFound = TRUE;
	}
}

void cd_mail_update_account_status( CDMailAccount *pUpdatedMailAccount )
{
	if( !pUpdatedMailAccount ) return;
	CairoDockModuleInstance *myApplet = pUpdatedMailAccount->pAppletInstance;
	
	GList *pIconList = CD_APPLET_MY_ICONS_LIST;
	CairoContainer *pContainer = CD_APPLET_MY_ICONS_LIST_CONTAINER;
	Icon *pIcon = pUpdatedMailAccount->icon;
	g_return_if_fail (pIcon != NULL);
	
	cairo_t *pIconContext = cairo_create (pIcon->pIconBuffer);
	if (pUpdatedMailAccount->iNbUnseenMails > 0)
	{
		cairo_dock_set_quick_info_full (myDrawContext, pIcon, pContainer, "%d", pUpdatedMailAccount->iNbUnseenMails);
		
		cairo_dock_set_image_on_icon (pIconContext, myConfig.cHasMailUserImage, pIcon, pContainer);
	}
	else
	{
		cairo_dock_set_quick_info (myDrawContext, NULL, pIcon, cairo_dock_get_max_scale (pContainer));
		
		cairo_dock_set_image_on_icon (pIconContext, myConfig.cNoMailUserImage, pIcon, pContainer);
	}
	cairo_destroy (pIconContext);
	cairo_dock_redraw_icon (pIcon, pContainer);
}



gboolean cd_mail_load_icons( CairoDockModuleInstance *myApplet )  // c'est bourrin, donc a utiliser avec parcimonie.
{
	CDMailAccount *pMailAccount;
	GList *pIconList = NULL;
	Icon *pIcon;
	guint i;
	int iNbIcons = 0;
	
	myData.iNbUnreadMails = 0;
	
	//\_______________________ On construit la liste des icones.
	if (myData.pMailAccounts != NULL)
	{
		for (i = 0; i < myData.pMailAccounts->len; i ++)
		{
			pMailAccount = g_ptr_array_index (myData.pMailAccounts, i);
			if( !pMailAccount )
				continue;
			
			myData.iNbUnreadMails += pMailAccount->iNbUnseenMails;
			_add_icon (pMailAccount);
			iNbIcons ++;
		}
	}
	
	cd_message( "%s () : %d new messages", __func__, myData.iNbUnreadMails );
	
	//\_______________________ On efface l'ancienne liste.
	CD_APPLET_DELETE_MY_ICONS_LIST;
	
	//\_______________________ On charge la nouvelle liste.
	gpointer pConfig[2] = {GINT_TO_POINTER (FALSE), GINT_TO_POINTER (FALSE)};
	CD_APPLET_LOAD_MY_ICONS_LIST (pIconList, myConfig.cRenderer, (iNbIcons > 1 ? "Caroussel" : "Simple"), (iNbIcons > 1 ? pConfig : NULL));
	
	//\_______________________ On redessine l'icone principale.
	cd_mail_draw_main_icon(myApplet);

	if (myDesklet)
		gtk_widget_queue_draw (myDesklet->pWidget);
	else
		CD_APPLET_REDRAW_MY_ICON;

	myData.bNewMailFound = FALSE;

	return TRUE;
}



void cd_mail_draw_main_icon (CairoDockModuleInstance *myApplet)
{
	g_return_if_fail (myDrawContext != NULL);
	
	gchar *cNewImage = NULL;
	if (myData.iNbUnreadMails <= 0)
	{
		cairo_dock_remove_dialog_if_any (myIcon);
		if( myData.bNewMailFound )
		{
			cairo_dock_show_temporary_dialog_with_icon (D_("No unread mail in your mailboxes"), myIcon, myContainer, 1500, "same icon");
		}
		//Chargement de l'image "pas de mail"
		cNewImage = myConfig.cNoMailUserImage;
	}
	else
	{
		if( myData.bNewMailFound )  // parmi les messages non lus, il y'en a des nouveaux => on le signale.
		{
			GString *ttip_str = g_string_sized_new(300);
			gint i;

			/* don't play more often than every 4 seconds... */
			time_t currentTime = time(NULL);
			if(currentTime-myData.timeEndOfSound > 4 &&
				myData.bNewMailFound == TRUE)
			{
				cairo_dock_play_sound(myConfig.cNewMailUserSound);
				myData.timeEndOfSound = time(NULL);
			}

			if (myData.iNbUnreadMails > 1)
				g_string_append_printf( ttip_str, D_("You have %d new mails :"), myData.iNbUnreadMails);
			else
				g_string_append_printf( ttip_str, D_("You have a new mail :"));

			if (myData.pMailAccounts != NULL)
			{
				CDMailAccount *pMailAccount;
				for (i = 0; i < myData.pMailAccounts->len; i++)
				{
					pMailAccount = g_ptr_array_index (myData.pMailAccounts, i);
					if( !pMailAccount ) continue;
					if (pMailAccount->iNbUnseenMails > 0) {
						g_string_append_printf(ttip_str, "\n    %d in %s",
							pMailAccount->iNbUnseenMails, pMailAccount->name);
					}
				}
			}
			cairo_dock_remove_dialog_if_any (myIcon);
			cairo_dock_show_temporary_dialog (ttip_str->str, myIcon, myContainer, 5000);

			g_string_free(ttip_str, TRUE);
		}
		
		cNewImage = myConfig.cHasMailUserImage;
	}

	if (CD_APPLET_MY_CONTAINER_IS_OPENGL && myDesklet)
	{
		if( myData.iNbUnreadMails > 0 )
		{
			cairo_dock_launch_animation (myContainer);
		}
		///cd_mail_render_3D_to_texture (myApplet);
	}
	else
	{
		CD_APPLET_SET_IMAGE_ON_MY_ICON (cNewImage);
	}

	CD_APPLET_SET_QUICK_INFO_ON_MY_ICON_PRINTF ("%d", myData.iNbUnreadMails);
	myData.bNewMailFound = FALSE;
}
