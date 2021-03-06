#ifndef _MENU_H
#define _MENU_H (1)

#include <gtk/gtk.h>

enum view_menu_action {
  HIDE_SHOW_TRANSACTION_FORM = 0,
  HIDE_SHOW_GRID,
  HIDE_SHOW_RECONCILED_TRANSACTIONS,
  ONE_LINE_PER_TRANSACTION,
  TWO_LINES_PER_TRANSACTION,
  THREE_LINES_PER_TRANSACTION,
  FOUR_LINES_PER_TRANSACTION,
  HIDE_SHOW_CLOSED_ACCOUNTS,
};

/* START_INCLUDE_H */
/* END_INCLUDE_H */


/* START_DECLARATION */
gboolean affiche_derniers_fichiers_ouverts ( void );
void efface_derniers_fichiers_ouverts ( void );
gboolean gsb_gui_sensitive_menu_item ( gchar *item_name, gboolean state );
void gsb_gui_toggle_line_view_mode ( GtkRadioAction *action,
                        GtkRadioAction *current,
                        gpointer user_data );
gboolean gsb_gui_toggle_show_archived ( void );
gboolean gsb_gui_toggle_show_reconciled ( void );
GtkUIManager *gsb_menu_get_ui_manager ( void );
gboolean gsb_menu_set_block_menu_cb ( gboolean etat );
gboolean gsb_menu_set_menus_select_scheduled_sensitive ( gboolean sensitive );
gboolean gsb_menu_set_menus_select_transaction_sensitive ( gboolean sensitive );
void gsb_menu_set_menus_view_account_sensitive ( gboolean sensitive );
void gsb_menu_set_menus_with_file_sensitive ( gboolean sensitive );
gboolean gsb_menu_update_accounts_in_menus ( void );
gboolean gsb_menu_update_view_menu ( gint account_number );
GtkWidget *init_menus ( GtkWidget *vbox );
/* END_DECLARATION */
#endif
