/* ************************************************************************** */
/*                                                                            */
/*     Copyright (C)	2006-2006 Benjamin Drieu (bdrieu@april.org)	      */
/* 			http://www.grisbi.org				      */
/*                                                                            */
/*  This program is free software; you can redistribute it and/or modify      */
/*  it under the terms of the GNU General Public License as published by      */
/*  the Free Software Foundation; either version 2 of the License, or         */
/*  (at your option) any later version.                                       */
/*                                                                            */
/*  This program is distributed in the hope that it will be useful,           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/*  GNU General Public License for more details.                              */
/*                                                                            */
/*  You should have received a copy of the GNU General Public License         */
/*  along with this program; if not, write to the Free Software               */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*                                                                            */
/* ************************************************************************** */

#include "include.h"

/*START_INCLUDE*/
#include "gsb_plugins.h"
#include "dialog.h"
#include "include.h"
#include "plugins/gnucash/gnucash.h"
#include "plugins/ofx/ofx.h"
#include "plugins/openssl/openssl.h"
/*END_INCLUDE*/

/*START_EXTERN*/
/*END_EXTERN*/

/*START_STATIC*/
/*END_STATIC*/


/** List of registered plugins.  It should contain gsb_plugin structures. */
static GSList * plugins = NULL;

/**
 *
 *
 */
void gsb_plugins_scan_dir ( const char *dirname )
{
    gsb_plugin *plugin = NULL;

    plugin = g_malloc0 ( sizeof ( gsb_plugin ) );
    plugin -> name = "gnucash";
    plugin -> plugin_register = &gnucash_plugin_register;
    plugin -> plugin_run =      &gnucash_plugin_run;
    plugin -> plugin_release =  &gnucash_plugin_release;
    plugin -> plugin_register ();
    plugins = g_slist_append ( plugins, plugin );

    plugin = g_malloc0 ( sizeof ( gsb_plugin ) );
    plugin -> name = "ofx";
    plugin -> plugin_register = &ofx_plugin_register;
    plugin -> plugin_run =      &ofx_plugin_run;
    plugin -> plugin_release =  &ofx_plugin_release;
    plugin -> plugin_register ();
    plugins = g_slist_append ( plugins, plugin );

    plugin = g_malloc0 ( sizeof ( gsb_plugin ) );
    plugin -> name = "openssl";
    plugin -> plugin_register = &openssl_plugin_register;
    plugin -> plugin_run =      &openssl_plugin_run;
    plugin -> plugin_release =  &openssl_plugin_release;
    plugin -> plugin_register ();
    plugins = g_slist_append ( plugins, plugin );
}



/**
 *
 *
 *
 */
gsb_plugin * gsb_plugin_find ( gchar * plugin_name )
{
    GSList * tmp = plugins;

    g_return_val_if_fail ( plugin_name, NULL );
    
    while ( tmp )
    {
	gsb_plugin * plugin = (gsb_plugin *) tmp -> data;

	if ( ! strcmp ( plugin_name, plugin -> name ) )
	{
	    return plugin;
	}

	tmp = tmp -> next;
    }

    return NULL;
}



/**
 * Get activated plugins.
 *
 * \return A newly-allocated string representing activated plugins.
 */
gchar * gsb_plugin_get_list ()
{
    gchar * list = NULL;
    gchar * old_list = NULL;
    GSList * tmp = plugins;

    while ( tmp )
    {
	gsb_plugin * plugin = (gsb_plugin *) tmp -> data;

	if ( ! list )
	{
	    list = g_strconcat ( _("with plugins"), " ", plugin -> name, NULL );
	}
	else
	{
	    old_list = list;
	    list = g_strconcat ( list, ", ", plugin -> name, NULL );
	    g_free(old_list);
	}

	tmp = tmp -> next;
    }
    if (! list)
    	list = g_strdup("no plugin");

    return list;
}



/**
 *
 *
 *
 */
void gsb_plugins_release ( )
{
    GSList * tmp = plugins;

    while ( tmp )
    {
	gsb_plugin * plugin = (gsb_plugin *) tmp -> data;

	plugin -> plugin_release ();

	tmp = tmp -> next;
    }        
}



/* Local Variables: */
/* c-basic-offset: 4 */
/* End: */