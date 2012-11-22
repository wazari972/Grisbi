#ifndef GSB_WEBOOB_H
#define GSB_WEBOOB_H

/* START_INCLUDE_H */
#include "import.h"
/* END_INCLUDE_H */

/* START_DECLARATION */
G_MODULE_EXPORT extern void weboob_plugin_register (void);
G_MODULE_EXPORT extern gpointer weboob_plugin_run (GtkWidget *assistant,
                                                   struct imported_file *imported);
/* END_DECLARATION */

#endif /* GSB_WEBOOB_H */
