/* ************************************************************************** */
/*                                                                            */
/*     Copyright (C)	2012-2012 Kevin Pouget (grisbi@kevin.poouget.me)      */
/* 			http://www.grisbi.org				                              */
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "include.h"

/*START_INCLUDE*/
#include "weboob.h"
#include "dialog.h"
#include "gsb_real.h"
#include "utils_str.h"
#include "erreur.h"
/*END_INCLUDE*/

/*START_EXTERN*/
/*END_EXTERN*/

/*START_STATIC*/
static GSList *import_weboob_data (GtkWidget *assistant, struct imported_file *imported);
/*END_STATIC*/


static struct import_format weboob_format =
{
    "Weboob",
    "Weboob/Boobank import",
    "weboob",
    (import_function) import_weboob_data,
};



#ifndef ENABLE_STATIC
/** Module name. */
G_MODULE_EXPORT const gchar plugin_name[] = "weboob";
#endif



/** Initialization function. */
G_MODULE_EXPORT extern void weboob_plugin_register (void)
{
    devel_debug ("Initializating weboob plugin");
    register_import_format (&weboob_format);
}



/** Main function of module. */
G_MODULE_EXPORT extern gpointer weboob_plugin_run (GtkWidget *assistant,
				                                   struct imported_file *imported)
{
    return import_weboob_data (assistant, imported);
}

/**
 *
 *
 *
 *
 */
GSList *import_weboob_data (GtkWidget *assistant, struct imported_file *imported)
{
    
    return NULL;
}
/* *******************************************************************************/

/* Local Variables: */
/* c-basic-offset: 4 */
/* End: */
