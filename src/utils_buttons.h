#ifndef UTILS_BUTTONS_H
#define UTILS_BUTTONS_H

typedef enum GSB_BUTTON_STYLE {
    GSB_BUTTON_TEXT,
    GSB_BUTTON_ICON,
    GSB_BUTTON_BOTH,
} GsbButtonStyle;


/*START_DECLARATION*/
void checkbox_set_value ( GtkWidget * checkbox, guint * data, gboolean update );
GtkWidget *cree_bouton_url ( const gchar *adr,
			     const gchar *inscription );
GtkWidget * new_button_with_label_and_image ( GsbButtonStyle style,
					      gchar * name, gchar * filename,
					      GCallback callback, gpointer data );
GtkWidget *new_checkbox_with_title ( gchar * label, guint * data, GCallback hook);
GtkWidget * new_int_spin_button ( gint * value, 
				  gdouble lower, gdouble upper, 
				  gdouble step_increment, gdouble page_increment, 
				  gdouble page_size, 
				  gdouble climb_rate, guint digits,
				  GCallback hook );
GtkWidget * new_radiogroup ( gchar * choice1, gchar * choice2, guint * data, GCallback hook );
GtkWidget *new_radiogroup_with_title ( GtkWidget * parent,
				       gchar * title, gchar * choice1, gchar * choice2,
				       guint * data, GCallback hook );
GtkWidget * new_spin_button ( gdouble * value, 
			      gdouble lower, gdouble upper, 
			      gdouble step_increment, gdouble page_increment, 
			      gdouble page_size, 
			      gdouble climb_rate, guint digits,
			      GCallback hook );
GtkWidget * new_stock_button_with_label ( GsbButtonStyle style,
					  gchar * stock_id, gchar * name, 
					  GCallback callback, gpointer data );
GtkWidget * new_stock_button_with_label_menu ( GsbButtonStyle style,
					       gchar * stock_id, gchar * name, 
					       GCallback callback, gpointer data );
gboolean set_int_from_menu ( GtkWidget * menu, gint * dummy);
void set_popup_position (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data);
void spin_button_set_value ( GtkWidget * spin, gint * value );
void spin_button_set_value_double ( GtkWidget * spin, gdouble * value );
/*END_DECLARATION*/

#endif
