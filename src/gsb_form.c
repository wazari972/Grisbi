/* ************************************************************************** */
/*                                                                            */
/*     copyright (c)	2000-2008 C�dric Auger (cedric@grisbi.org)	      */
/*			2004-2008 Benjamin Drieu (bdrieu@april.org) 	      */
/*			http://www.grisbi.org   			      */
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

/**
 * \file gsb_form.c
 * functions working on the transactions and scheduled form
 */


#include "include.h"

/*START_INCLUDE*/
#include "gsb_form.h"
#include "./accueil.h"
#include "./dialog.h"
#include "./gsb_calendar_entry.h"
#include "./gsb_currency.h"
#include "./gsb_data_account.h"
#include "./gsb_data_budget.h"
#include "./gsb_data_category.h"
#include "./gsb_data_currency.h"
#include "./gsb_data_form.h"
#include "./gsb_data_mix.h"
#include "./gsb_data_payee.h"
#include "./gsb_data_payment.h"
#include "./gsb_data_report.h"
#include "./gsb_data_scheduled.h"
#include "./gsb_data_transaction.h"
#include "./utils_dates.h"
#include "./gsb_form_scheduler.h"
#include "./gsb_form_transaction.h"
#include "./gsb_form_widget.h"
#include "./gsb_fyear.h"
#include "./navigation.h"
#include "./menu.h"
#include "./tiers_onglet.h"
#include "./gsb_payment_method.h"
#include "./gsb_real.h"
#include "./gsb_reconcile.h"
#include "./gsb_scheduler.h"
#include "./gsb_scheduler_list.h"
#include "./gsb_transactions_list.h"
#include "./utils_editables.h"
#include "./gtk_combofix.h"
#include "./traitement_variables.h"
#include "./utils_str.h"
#include "./transaction_list.h"
#include "./transaction_list_select.h"
#include "./utils_operations.h"
#include "./fenetre_principale.h"
#include "./gsb_data_payment.h"
#include "./gtk_combofix.h"
#include "./gsb_data_transaction.h"
#include "./gsb_form_scheduler.h"
#include "./gsb_data_form.h"
#include "./include.h"
#include "./erreur.h"
#include "./structures.h"
#include "./gsb_form_widget.h"
/*END_INCLUDE*/

/*START_STATIC*/
static gboolean gsb_form_activate_expander ( GtkWidget *expander,
				      gpointer null );
static void gsb_form_check_auto_separator ( GtkWidget *entry );
static gint gsb_form_check_for_transfer ( const gchar *entry_string );
static gboolean gsb_form_get_categories ( gint transaction_number,
				   gint new_transaction,
				   gboolean is_transaction );
static gint gsb_form_get_element_expandable ( gint element_number );
static gboolean gsb_form_hide ( void );
static  gboolean gsb_form_size_allocate ( GtkWidget *widget,
					 GtkAllocation *allocation,
					 gpointer null );
static void gsb_form_take_datas_from_form ( gint transaction_number,
				     gboolean is_transaction );
static gboolean gsb_form_validate_form_transaction ( gint transaction_number,
					      gboolean is_transaction );
/*END_STATIC*/

/*START_EXTERN*/
extern GtkWidget * navigation_tree_view ;
extern GtkTreeSelection * selection ;
extern GtkWidget *window ;
/*END_EXTERN*/



/** label of the last statement */
GtkWidget *label_last_statement = NULL;

/** the expander */
static GtkWidget *form_expander = NULL;

/** the 3 parts of the form :
 * for scheduled transactions
 * for transactions
 * the buttons valid/cancel */
static GtkWidget *form_scheduled_part;
GtkWidget *form_transaction_part;
static GtkWidget *form_button_part;

/* block the size allocate signal to avoid to move several times
 * the tree view when open the form */
static gboolean block_size_allocate = FALSE;


/** when the automatic complete transaction is done
 * for a split of transaction, we propose to recover too
 * the children with that button */
GtkWidget *form_button_recover_split;

/** need to set the 2 buttons valid/cancel here and cannot
 * just show/hide the form_button_part because of the split button */
static GtkWidget *form_button_valid;
static GtkWidget *form_button_cancel;

/** THE form */
static GtkWidget *transaction_form;

/** to avoid recursive allocate size */
gint saved_allocation_size;


/**
 * the value transaction_form is static,
 * that function return it
 *
 * \param
 *
 * \return a pointer to the transaction form
 * */
GtkWidget *gsb_form_get_form_widget ( void )
{
    return transaction_form;
}

/**
 * Create an empty form in an GtkExpander.
 *
 * \return Expander that contains form.
 * */
GtkWidget *gsb_form_new ( void )
{
    /* Create the expander */
    form_expander = gtk_expander_new ( "" );
    g_signal_connect ( G_OBJECT (form_expander ), "destroy",
    		G_CALLBACK ( gtk_widget_destroyed), &form_expander );
    gtk_expander_set_expanded ( GTK_EXPANDER ( form_expander ),
				etat.formulaire_toujours_affiche );
    g_signal_connect_after( G_OBJECT(form_expander),
			    "activate",
			    G_CALLBACK (gsb_form_activate_expander),
			    NULL );

    gsb_form_create_widgets ();

    return form_expander;
}

/**
 * return the address of the scheduler part widget
 *
 * \param
 *
 * \return a pointer to a widget (the scheduler part of the form)
 * */
GtkWidget *gsb_form_get_scheduler_part ( void )
{
    return form_scheduled_part;
}

/**
 * return the address of the button part widget
 *
 * \param
 *
 * \return a pointer to a widget (the scheduler part of the form)
 * */
GtkWidget *gsb_form_get_button_part ( void )
{
    return form_button_part;
}


/**
 *  Do the grunt job of creating widgets in for the Grisbi form.
 *  Fills them in form_expander, a GtkExpander normally created by
 *  gsb_form_new().  It is reentrant, that is calling it over and over
 *  will reinitialize form content and delete previous content.
 */
void gsb_form_create_widgets ()
{
    GtkWidget * hbox, * label, * separator, * hbox_buttons;
    GtkWidget * child = gtk_bin_get_child ( GTK_BIN(form_expander) );

    devel_debug (NULL);

    gsb_form_widget_free_list ();
    if ( child && GTK_IS_WIDGET(child) )
    {
	gtk_container_remove ( GTK_CONTAINER(form_expander), child );
    }

    /* Expander has a composite label */
    hbox = gtk_hbox_new ( FALSE, 0 );
    gtk_expander_set_label_widget ( GTK_EXPANDER(form_expander), hbox );

    /* set the label transaction form */
    label = gtk_label_new ( NULL );
    gchar* tmpstr = g_strconcat ( "<span weight=\"bold\">", 
						       _("Transaction/Scheduled _form"),
						       "</span>", NULL );
    gtk_label_set_markup_with_mnemonic ( GTK_LABEL ( label ), tmpstr );
    g_free ( tmpstr );
    gtk_box_pack_start ( GTK_BOX ( hbox ),
			 label,
			 FALSE, FALSE, 0 );

    /* set the last statement label */
    label_last_statement = gtk_label_new ( NULL );
    g_signal_connect ( G_OBJECT (label_last_statement ), "destroy",
    		G_CALLBACK ( gtk_widget_destroyed), &label_last_statement );
    gtk_box_pack_end ( GTK_BOX ( hbox ),
		       label_last_statement,
		       FALSE, FALSE, 0 );

    /* Create form inside the expander :
     * the form is 3 parts :
     * top : the values specific for the scheduled transactions
     * middle : the values for transactions and scheduled transactions
     * bottom : the buttons valid/cancel */

    transaction_form = gtk_vbox_new ( FALSE, 5 );
    g_signal_connect ( G_OBJECT (transaction_form ), "destroy",
		       G_CALLBACK ( gtk_widget_destroyed), &transaction_form );
    gtk_container_add ( GTK_CONTAINER ( form_expander ),
			transaction_form );

    /* play with that widget to tell to the tree view to scroll to keep the selection visible */
    g_signal_connect_after ( G_OBJECT (transaction_form),
		       "size-allocate",
		       G_CALLBACK (gsb_form_size_allocate),
		       NULL );

    /* the scheduled part is a table of SCHEDULED_WIDTH col x SCHEDULED_HEIGHT rows */
    form_scheduled_part = gtk_table_new ( SCHEDULED_HEIGHT, 
					  SCHEDULED_WIDTH,
					  FALSE );
    g_signal_connect ( G_OBJECT (form_scheduled_part ), "destroy",
		       G_CALLBACK ( gtk_widget_destroyed), &form_scheduled_part );
    gtk_table_set_col_spacings ( GTK_TABLE (form_scheduled_part),
				 6 );
    gtk_box_pack_start ( GTK_BOX (transaction_form),
			 form_scheduled_part,
			 FALSE, FALSE,
			 0 );
    gsb_form_scheduler_create (form_scheduled_part);

    /* add a separator between the scheduled and transaction part */
    separator = gtk_hseparator_new ();
    gtk_box_pack_start ( GTK_BOX (transaction_form),
			 separator,
			 FALSE, FALSE,
			 0 );

    /* the transactions part is a variable table,
     * so set to 1x1 for now, it will change when we show it */
    form_transaction_part = gtk_table_new ( 1, 1, FALSE );
    g_signal_connect ( G_OBJECT (form_transaction_part ), "destroy",
		       G_CALLBACK ( gtk_widget_destroyed), &form_transaction_part );
    gtk_table_set_col_spacings ( GTK_TABLE (form_transaction_part), 6 );
    g_signal_connect ( G_OBJECT (form_transaction_part),
		       "size-allocate",
		       G_CALLBACK (gsb_form_allocate_size),
		       NULL );
    gtk_box_pack_start ( GTK_BOX (transaction_form),
			 form_transaction_part,
			 FALSE, FALSE,
			 0 );

    /* the buttons part is a hbox, with the recuperate child split
     * on the left and valid/cancel on the right */
    form_button_part = gtk_vbox_new ( FALSE, 0 );
    g_signal_connect ( G_OBJECT (form_button_part ), "destroy",
    		G_CALLBACK ( gtk_widget_destroyed), &form_button_part );
    gtk_box_pack_start ( GTK_BOX (transaction_form), form_button_part, FALSE, FALSE, 0 );

    /* add a separator between the transaction and button part */
    separator = gtk_hseparator_new ();
    gtk_box_pack_start ( GTK_BOX (form_button_part), separator, FALSE, FALSE, 0 );

    /* Hbox containing buttons */
    hbox_buttons = gtk_hbox_new ( FALSE, 0 );
    gtk_box_pack_start ( GTK_BOX (form_button_part), hbox_buttons, FALSE, FALSE, 0 );

    /* create the check button to recover the children of splits */
    form_button_recover_split = gtk_check_button_new_with_label ( _("Recover the children"));
    g_signal_connect ( G_OBJECT (form_button_recover_split ), "destroy",
    		G_CALLBACK ( gtk_widget_destroyed), &form_button_recover_split );
    gtk_box_pack_start ( GTK_BOX (hbox_buttons),
			 form_button_recover_split,
			 FALSE, FALSE,
			 0 );

    /* create the valid/cancel buttons */
    form_button_valid = gtk_button_new_from_stock (GTK_STOCK_OK);
    g_signal_connect ( G_OBJECT (form_button_valid ), "destroy",
    		G_CALLBACK ( gtk_widget_destroyed), &form_button_valid );
    gtk_button_set_relief ( GTK_BUTTON (form_button_valid), GTK_RELIEF_NONE );
    g_signal_connect ( G_OBJECT (form_button_valid), "clicked",
		       G_CALLBACK (gsb_form_finish_edition), NULL );
    gtk_box_pack_end ( GTK_BOX (hbox_buttons), form_button_valid, FALSE, FALSE, 0 );

    form_button_cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
    g_signal_connect ( G_OBJECT (form_button_cancel ), "destroy",
    		G_CALLBACK ( gtk_widget_destroyed), &form_button_cancel );
    gtk_button_set_relief ( GTK_BUTTON (form_button_cancel), GTK_RELIEF_NONE );
    g_signal_connect ( G_OBJECT (form_button_cancel), "clicked",
		       G_CALLBACK (gsb_form_escape_form), NULL );
    gtk_box_pack_end ( GTK_BOX (hbox_buttons), form_button_cancel, FALSE, FALSE, 0 );

    /* Kludge : otherwise, GtkExpander won't give us as many space
       as we need. */
    gtk_widget_set_size_request ( hbox, 2048, -1 );

    gtk_widget_show_all ( hbox );
    gtk_widget_show_all ( transaction_form );
    gtk_widget_hide ( form_scheduled_part );

    if ( ! etat.affiche_boutons_valider_annuler )
    {
	gtk_widget_hide_all ( form_button_part );
    }

    gsb_form_show ( FALSE );
}

/**
 * this function is used to move the tree view when open the form,
 * without that, the selection will be hidden by the form,
 * this moves it to show if necessary the selection
 *
 * \param widget
 * \param allocation
 * \param null
 *
 * \return FALSE
 * */
static gboolean gsb_form_size_allocate ( GtkWidget *widget,
					 GtkAllocation *allocation,
					 gpointer null )
{
    if (gsb_form_is_visible () && !block_size_allocate)
    {
	block_size_allocate = TRUE;
	gsb_transactions_list_set_row_align (-1.0);
    }
    else
	block_size_allocate = FALSE;
    return FALSE;
}


/**
 * fill the form according to the transaction or scheduled transaction given in param
 *
 * \param transaction_number the number of the transaction or scheduled transaction
 * \param is_transaction TRUE if transaction, FALSE if scheduled transaction
 * 	that param could be taken from the navigation bar but i prefer set in param for now
 * \param grab_focus if TRUE, after filling the form, the date will grab the focus ; if FALSE, nothing change with focus
 *
 * \return FALSE
 * */
gboolean gsb_form_fill_by_transaction ( gint transaction_number,
					gint is_transaction,
					gboolean grab_focus )
{
    gint mother_number;
    gint account_number;
    gint focus_to;
    gint is_split;
    GSList *tmp_list;

    devel_debug_int (transaction_number);

    if ( !transaction_number )
	return FALSE;

    /* get the account */
    account_number = gsb_data_mix_get_account_number (transaction_number, is_transaction);
    mother_number = gsb_data_mix_get_mother_transaction_number (transaction_number, is_transaction);
    is_split = gsb_data_mix_get_split_of_transaction (transaction_number, is_transaction);

    if (!is_transaction)
    {
	/* we need to set up the part of scheduler form here because changing the account
	 * button will change the form */
	gsb_form_scheduler_set (transaction_number);
    }

    /* if here account number = -1, it's because it's a white line or there were a problem ; 
     * in all case, get the current account number */
    if (account_number == -1)
	account_number = gsb_form_get_account_number ();

    /* show and prepare the form */
    gsb_form_show (TRUE);

    /* if the transaction is the white line, we set the date and necessary stuff and go away
     * for that use the button_press function, so clicking on a form's field or do enter/double click
     * on white line will do the same */
    if ( transaction_number == -1 )
    {
	GtkWidget *date_entry;

	/* if we can't touch the focus, it's because we just select a transaction, and
	 * for now, select the white line, so keep the form blank */
	if (!grab_focus)
	    return FALSE;

	date_entry = gsb_form_widget_get_widget (TRANSACTION_FORM_DATE);

	if ( gsb_form_widget_check_empty (date_entry))
	    /* for now,  transaction_number_in_form is NULL so can call the next function
	     * wich will prepare the form for a new transaction */
	    gsb_form_button_press_event ( date_entry,
					  NULL,
					  GINT_TO_POINTER (TRANSACTION_FORM_DATE));

	gtk_entry_select_region ( GTK_ENTRY (date_entry), 0, -1);
	gtk_widget_grab_focus ( GTK_WIDGET (date_entry));
	return FALSE;
    }

    /* set the number of the transaction in the form, can be -2, -3...
     * for white line split
     * that must be defined before gsb_form_change_sensitive_buttons
     * because without that number, others functions cannot get the account
     * number for an execution of scheduled in the home page
     * but after gsb_form_show because it will set that value to 0...*/
    g_object_set_data ( G_OBJECT ( transaction_form ),
			"transaction_number_in_form",
			GINT_TO_POINTER (transaction_number));

    gsb_form_change_sensitive_buttons (TRUE);

    /* by default, a function before changed all the form from non sensitive to sensitive,
     * but for split or split child, some widgets need to be unsensitive/hidden */
    gsb_form_set_sensitive (is_split, mother_number);

    /* fill what is necessary in the form */
    tmp_list = gsb_form_widget_get_list ();

    while (tmp_list)
    {
	struct_element *element;

	element = tmp_list -> data;

	if (mother_number
	    &&
	    transaction_number < 0)
	{
	    /* we are on a split white line, we fill only few fields
	     * with the element_number of the mother */
	    switch (element -> element_number)
	    {
		case TRANSACTION_FORM_DATE:
		case TRANSACTION_FORM_VALUE_DATE:
		case TRANSACTION_FORM_PARTY:
		case TRANSACTION_FORM_TYPE:
		case TRANSACTION_FORM_CHEQUE:
		case TRANSACTION_FORM_DEVISE:
		case TRANSACTION_FORM_BANK:
		    gsb_form_fill_element ( element -> element_number,
					    account_number,
					    mother_number,
					    is_transaction );
		    break;
	    }
	}
	else
	    /* normal transaction */
	    gsb_form_fill_element ( element -> element_number,
				    account_number,
				    transaction_number,
				    is_transaction );

	tmp_list = tmp_list -> next;
    }

    /* for a transaction, need to check if marked R, and do some stuff with that */
    if (is_transaction)
    {
	gint contra_transaction_number;

	/* get the contra transaction */
	contra_transaction_number = gsb_data_transaction_get_contra_transaction_number (transaction_number);

	/* if the transaction is marked R, cannot change the amounts */
	/* if the contra transaction is marked R, cannot change category and amounts */
	if ( gsb_data_transaction_get_marked_transaction (transaction_number) == OPERATION_RAPPROCHEE
	     ||
	     ( contra_transaction_number
	       &&
	       gsb_data_transaction_get_marked_transaction (contra_transaction_number) == OPERATION_RAPPROCHEE ))
	{
	    gtk_widget_set_sensitive ( gsb_form_widget_get_widget (TRANSACTION_FORM_DEBIT),
				       FALSE );
	    gtk_widget_set_sensitive ( gsb_form_widget_get_widget (TRANSACTION_FORM_CREDIT),
				       FALSE );

	    /* if it's a transfer, we cannot change too the category */
	    if (contra_transaction_number)
		gtk_widget_set_sensitive ( gsb_form_widget_get_widget (TRANSACTION_FORM_CATEGORY),
					   FALSE );
	}	
    }

    /* we take focus only if asked */
    if (grab_focus)
    {
	/* the form is full, if it's not a split, we give the focus to the date
	 * else, we give the focus to the first free form element */
	if (mother_number)
	    focus_to = gsb_form_widget_next_element ( account_number,
						      TRANSACTION_FORM_DATE,
						      GSB_RIGHT );
	else
	    focus_to = TRANSACTION_FORM_DATE;

	gtk_entry_select_region ( GTK_ENTRY ( gsb_form_widget_get_widget (focus_to)),
				  0,
				  -1);
	gtk_widget_grab_focus ( gsb_form_widget_get_widget (focus_to));
    }
    return FALSE;
}


/**
 * set the form sensitive, depend if it's a split child or normal transaction
 * in fact, the form is always sensitive, so just unsensitive what is needed for split child
 * or split mother
 *
 * \param split TRUE if is split of transaction (mother)
 * \param split_child TRUE if it's a child
 *
 * \return FALSE
 * */
gboolean gsb_form_set_sensitive ( gboolean split,
				  gboolean split_child)
{
    GSList *tmp_list;

    /* for now, come here only for split or split child */
    if (!split
	&&
	!split_child)
	return FALSE;

    tmp_list = gsb_form_widget_get_list ();

    while (tmp_list)
    {
	struct_element *element;

	element = tmp_list -> data;

	/* for a split, hide the exercice and the budget */
	if (split)
	{
	    switch (element -> element_number)
	    {
		case TRANSACTION_FORM_EXERCICE:
		case TRANSACTION_FORM_BUDGET:
		case TRANSACTION_FORM_VOUCHER:
		    gtk_widget_set_sensitive ( element -> element_widget,
					       FALSE );
		    break;
	    }
	}

	/* for a child of split, cannot change the date, payee... */
	if ( split_child )
	{
	    /* mixed widgets for transactions and scheduled transactions */
	    switch (element -> element_number)
	    {
		case TRANSACTION_FORM_DATE:
		case TRANSACTION_FORM_VALUE_DATE:
		case TRANSACTION_FORM_PARTY:
		case TRANSACTION_FORM_DEVISE:
		case TRANSACTION_FORM_TYPE:
		case TRANSACTION_FORM_CHEQUE:
		case TRANSACTION_FORM_BANK:
		    gtk_widget_set_sensitive ( element -> element_widget,
					       FALSE );
		    break;
	    }
	    /* specific widgets for scheduled transactions */
	    gtk_widget_hide (gsb_form_get_scheduler_part ());
	}
	tmp_list = tmp_list -> next;
    }
    return FALSE;
}


/**
 * fill the element_number in the form according to the given transaction or scheduled transaction
 * this is the best way to fill the form : send here the element number, the transaction or scheduled number
 * and this function will fill the good element with the good value in the form
 *
 * \param element_number the element we want to fill
 * \param account_number used for currency and method of payment
 * \param transaction_number the number of transaction or scheduled transaction to fill the element
 * \param is_transaction TRUE if transaction, FALSE if scheduled transaction
 *
 * \return
 * */
void gsb_form_fill_element ( gint element_number,
			     gint account_number,
			     gint transaction_number,
			     gboolean is_transaction )
{
    GtkWidget *widget;
    gchar *char_tmp;
    gint number;

    widget = gsb_form_widget_get_widget (element_number);
    if (!widget)
	return;

    switch (element_number)
    {
	case TRANSACTION_FORM_OP_NB:
	    gtk_label_set_text ( GTK_LABEL ( widget ),
				 char_tmp = utils_str_itoa (transaction_number));
	    g_free (char_tmp);
	    break;

	case TRANSACTION_FORM_DATE:
	    gsb_form_entry_get_focus (widget);
	    gsb_calendar_entry_set_date ( widget, gsb_data_mix_get_date (transaction_number, is_transaction));
	    break;

	case TRANSACTION_FORM_VALUE_DATE:
	    /* value_date only for transactions */
	    if ( is_transaction
		 &&
		 gsb_data_transaction_get_value_date (transaction_number))
	    {
		gsb_form_entry_get_focus (widget);
		gsb_calendar_entry_set_date ( widget, gsb_data_transaction_get_value_date (transaction_number));
	    }
	    break;

	case TRANSACTION_FORM_EXERCICE:
	    gsb_fyear_set_combobox_history ( widget,
					     gsb_data_mix_get_financial_year_number (transaction_number, is_transaction));
	    break;

	case TRANSACTION_FORM_PARTY:
	    if ( gsb_data_mix_get_party_number (transaction_number, is_transaction))
	    {
		gsb_form_entry_get_focus (widget);
		gtk_combofix_set_text ( GTK_COMBOFIX ( widget ),
					gsb_data_payee_get_name ( gsb_data_mix_get_party_number (transaction_number, is_transaction), TRUE ));
	    }
	    break;

	case TRANSACTION_FORM_DEBIT:
	    if (gsb_data_mix_get_amount (transaction_number, is_transaction).mantissa < 0)
	    {
		char_tmp = gsb_real_get_string_with_currency (gsb_real_abs (gsb_data_mix_get_amount (transaction_number, is_transaction)),
							      gsb_data_mix_get_currency_number (transaction_number, is_transaction),
							      FALSE );

		gsb_form_entry_get_focus (widget);
		gtk_entry_set_text ( GTK_ENTRY ( widget ),
				     char_tmp );
		g_free (char_tmp);
	    }
	    break;

	case TRANSACTION_FORM_CREDIT:
	    if (gsb_data_mix_get_amount (transaction_number, is_transaction).mantissa >= 0)
	    {
		char_tmp = gsb_real_get_string_with_currency (gsb_data_mix_get_amount (transaction_number, is_transaction),
							      gsb_data_mix_get_currency_number (transaction_number, is_transaction),
							      FALSE );

		gsb_form_entry_get_focus (widget);
		gtk_entry_set_text ( GTK_ENTRY ( widget ),
				     char_tmp );
		g_free (char_tmp);
	    }
	    break;

	case TRANSACTION_FORM_CATEGORY:
	    if (gsb_data_mix_get_split_of_transaction (transaction_number, is_transaction))
	    {
		/* it's a split of transaction */
		gsb_form_entry_get_focus (widget);
		gtk_combofix_set_text ( GTK_COMBOFIX (widget),
					_("Split of transaction") );
	    }
	    else
	    {
		if (gsb_data_mix_get_transaction_number_transfer (transaction_number, is_transaction))
		{
		    /* it's a transfer */
		    gsb_form_entry_get_focus (widget);

		    /* check first the deleted account, don't worry with scheduled transactions because
		     * gsb_data_mix_get_transaction_number_transfer cannot be -1 for them */
		    if ( gsb_data_transaction_get_contra_transaction_number (transaction_number) == -1
			 &&
			 gsb_data_transaction_get_contra_transaction_account (transaction_number) == -1 )
			gtk_combofix_set_text ( GTK_COMBOFIX ( widget ),
						_("Transfer : Deleted account") );
		    else
		    {
			gtk_combofix_set_text ( GTK_COMBOFIX (widget),
						char_tmp = g_strconcat ( _("Transfer : "),
						 gsb_data_account_get_name (gsb_data_mix_get_account_number_transfer (transaction_number, is_transaction)), NULL ));
			g_free (char_tmp);
		    }
		}
		else
		{
		    /* it's a normal category */
		    char_tmp = gsb_data_category_get_name ( gsb_data_mix_get_category_number (transaction_number, is_transaction),
							    gsb_data_mix_get_sub_category_number (transaction_number, is_transaction),
							    NULL );
		    if (char_tmp)
		    {
			gsb_form_entry_get_focus (widget);
			gtk_combofix_set_text ( GTK_COMBOFIX ( widget ),
						char_tmp );
			g_free (char_tmp);
		    }
		}
	    }
	    break;

	case TRANSACTION_FORM_BUDGET:
	    char_tmp = gsb_data_budget_get_name ( gsb_data_mix_get_budgetary_number (transaction_number, is_transaction),
						  gsb_data_mix_get_sub_budgetary_number (transaction_number, is_transaction),
						  NULL );
	    if (char_tmp)
	    {
		gsb_form_entry_get_focus (widget);
		gtk_combofix_set_text ( GTK_COMBOFIX ( widget ),
					char_tmp );
		g_free (char_tmp);
	    }
	    break;

	case TRANSACTION_FORM_NOTES:
	    if (gsb_data_mix_get_notes (transaction_number, is_transaction))
	    {
		gsb_form_entry_get_focus (widget);
		gtk_entry_set_text ( GTK_ENTRY ( widget ),
				     gsb_data_mix_get_notes (transaction_number, is_transaction));
	    }
	    break;

	case TRANSACTION_FORM_TYPE:
	    /* one small thing here : normally if the transaction is a debit, we set the
	     * negative payment method. but the problem : if we are on a child of split,
	     * that child contains the payment method of the mother, and if the child has not
	     * the same sign of the mother (rare but possible), grisbi will not find the good payment
	     * method for that child because it's not on the good sign... so for a child of split,
	     * we set the payment box of the mother */
	    number = gsb_data_mix_get_mother_transaction_number (transaction_number, is_transaction);
	    if (!number)
		/* it's not a child split, so set number to transaction_number */
		number = transaction_number;

	    /* ok, now number contains either the transaction_number, either the mother transaction number,
	     * we can check the sign with it */
	    if (gsb_data_mix_get_amount (number, is_transaction).mantissa < 0)
		gsb_payment_method_create_combo_list ( widget,
						       GSB_PAYMENT_DEBIT,
						       account_number, 0 );
	    else
		gsb_payment_method_create_combo_list ( widget,
						       GSB_PAYMENT_CREDIT,
						       account_number, 0 );

	    /* don't show the cheque entry for a child of split */
	    if ( GTK_WIDGET_VISIBLE (widget))
	    {
		number = gsb_data_mix_get_method_of_payment_number (transaction_number, is_transaction);

		/* we show the cheque entry only for transactions */
		if (gsb_payment_method_set_combobox_history ( widget,
							      number )
		    &&
		    gsb_data_payment_get_show_entry (number)
		    &&
		    is_transaction
		    &&
		    !gsb_data_mix_get_mother_transaction_number (transaction_number, is_transaction))
		    gtk_widget_show (gsb_form_widget_get_widget (TRANSACTION_FORM_CHEQUE));
	    }
	    else
		gtk_widget_hide ( gsb_form_widget_get_widget (TRANSACTION_FORM_CHEQUE));
	    break;

	case TRANSACTION_FORM_DEVISE:
	    number = gsb_data_mix_get_currency_number (transaction_number, is_transaction);

	    gsb_currency_set_combobox_history ( widget,
						number);
	    if (is_transaction)
		gsb_form_transaction_check_change_button ( number,
							   account_number );
	    break;

	case TRANSACTION_FORM_BANK:
	    if ( gsb_data_mix_get_bank_references (transaction_number, is_transaction))
	    {
		gsb_form_entry_get_focus (widget);
		gtk_entry_set_text ( GTK_ENTRY ( widget ),
				     gsb_data_mix_get_bank_references (transaction_number, is_transaction));
	    }
	    break;

	case TRANSACTION_FORM_VOUCHER:
	    if ( gsb_data_mix_get_voucher (transaction_number, is_transaction))
	    {
		gsb_form_entry_get_focus (widget);
		gtk_entry_set_text ( GTK_ENTRY ( widget ),
				     gsb_data_mix_get_voucher (transaction_number, is_transaction));
	    }
	    break;

	case TRANSACTION_FORM_CONTRA:
	    if (gsb_data_mix_get_transaction_number_transfer (transaction_number, is_transaction))
	    {
		number = gsb_data_mix_get_account_number_transfer (transaction_number, is_transaction);

		/* only if not closed account */
		if (number != -1)
		{
		    if ( gsb_data_mix_get_amount (transaction_number, is_transaction).mantissa < 0 )
			gsb_payment_method_create_combo_list ( widget,
							       GSB_PAYMENT_CREDIT,
							       number, 0);
		    else
			gsb_payment_method_create_combo_list ( widget,
							       GSB_PAYMENT_DEBIT,
							       number, 0);

		    if (GTK_WIDGET_VISIBLE (widget))
		    {
			gint method;

			if (is_transaction)
			    method = gsb_data_transaction_get_method_of_payment_number (gsb_data_transaction_get_contra_transaction_number (transaction_number));
			else
			    method = gsb_data_scheduled_get_contra_method_of_payment_number (transaction_number);

			gsb_payment_method_set_combobox_history ( widget,
								  method );
		    }
		}
	    }
	    break;

	case TRANSACTION_FORM_MODE:
	    if ( gsb_data_mix_get_automatic_transaction (transaction_number, is_transaction))
		gtk_label_set_text ( GTK_LABEL ( widget ),
				     _("Auto"));
	    else
		gtk_label_set_text ( GTK_LABEL ( widget ),
				     _("Manual"));
	    break;
    }
}


/**
 * show or hide the expander
 *
 * \param visible TRUE or FALSE
 * \param transactions_list TRUE if we are on transactions, FALSE if we are on scheduler
 *
 * return FALSE
 * */
gboolean gsb_form_set_expander_visible ( gboolean visible,
					 gboolean transactions_list )
{
    if ( visible )
    {
	gtk_widget_show (form_expander);

	if (transactions_list)
	    gtk_widget_show (label_last_statement);
	else
	    gtk_widget_hide (label_last_statement);
    }
    else
	gtk_widget_hide (form_expander);

    return FALSE;
}



/**
 * show/hide the form according to the expander,
 * this will emit a signal as if the user changed it by himself
 *
 * \param
 *
 * \return FALSE
 * */
gboolean gsb_form_switch_expander ( void )
{
    gtk_expander_set_expanded ( GTK_EXPANDER (form_expander),
				!gsb_form_is_visible ());

    return FALSE;
}



/**
 * Called when the state of the expander changes
 *
 * \param expander	Expanded that triggered signal.
 * \param null		Not used.
 *
 * \return FALSE
 * */
gboolean gsb_form_activate_expander ( GtkWidget *expander,
				      gpointer null )
{
    devel_debug (NULL);

    if ( gtk_expander_get_expanded (GTK_EXPANDER (expander)))
    {
	gsb_form_widget_free_list ();
	gsb_form_show ( TRUE );
	etat.formulaire_toujours_affiche = TRUE;
    }
    else
    {
	gsb_form_show ( FALSE );
	etat.formulaire_toujours_affiche = FALSE;
    }
    gsb_menu_update_view_menu ( gsb_gui_navigation_get_current_account () );

    return FALSE;
}



/**
 * show the form, detect automaticly what we need to show, even for transactions,
 * scheduled_transactions and the buttons valid/cancel
 *
 * \param show TRUE if we want to automatickly turn on the expander, 
 * 		FALSE if we don't want, so just let it
 *
 * \return FALSE
 * */
gboolean gsb_form_show ( gboolean show )
{
    gint origin;

    devel_debug_int (show);

    origin = gsb_form_get_origin ();

    /* show or hide the scheduler part */
    switch (origin)
    {
	case ORIGIN_VALUE_OTHER:
	case ORIGIN_VALUE_HOME:
	    return FALSE;
	    break;

	case ORIGIN_VALUE_SCHEDULED:
	    gsb_form_scheduler_set_frequency (2);
	    gtk_widget_show (form_scheduled_part);
	    break;

	default:
	    gtk_widget_hide (form_scheduled_part);
    }

    gsb_form_fill_from_account (origin);
    gtk_widget_show (form_transaction_part);

    if ( ! gsb_form_is_visible ( ) && show )
	gtk_expander_set_expanded (GTK_EXPANDER (form_expander),
				   TRUE );

    /* FIXME :transform that to be local variable here */
    if ( etat.affiche_boutons_valider_annuler )
	gtk_widget_show (form_button_part);

    return FALSE;
}



/**
 * Check if transactions form is visible.
 *
 * \return TRUE if transactions forms is expanded, FALSE otherwise.
 */
gboolean gsb_form_is_visible ()
{
    return gtk_expander_get_expanded ( GTK_EXPANDER ( form_expander ) );
}



/**
 * hide the form, depending to the variable etat.formulaire_toujours_affiche
 * if the user wants the form always showed, that function does nothing
 * else, destroy the form and hide the expander
 *
 * \param
 *
 * \return FALSE
 * */
gboolean gsb_form_hide ( void )
{
    gsb_form_escape_form ();
    return FALSE;
}


/**
 * return the current account number according,
 * if we are on transactions list, return the current account,
 * if we are on scheduling list, return the selected account on button
 * if we are on welcome page, it's only if it's an execution of scheduled
 *
 * \param origin
 *
 * \return the account number or -2 if problem (-1 is reserved to get account from the button)
 * */
gint gsb_form_get_account_number ( void )
{
    gint account_number;
    gint origin;

    origin = gsb_form_get_origin ();

    switch (origin)
    {
	case ORIGIN_VALUE_OTHER:
	    return -2;
	    break;

	case ORIGIN_VALUE_HOME:
	    /* we are on the home page, we need to check if the form is showed,
	     * if yes, we get the account number of the scheduled showed in that form */
	    if ( transaction_form && GTK_WIDGET_VISIBLE (transaction_form))
	    {
		gint scheduled_number;

		scheduled_number = GPOINTER_TO_INT ( g_object_get_data ( G_OBJECT (transaction_form),
									 "transaction_number_in_form" ));
		if (!scheduled_number)
		    return -2;
		account_number = gsb_data_scheduled_get_account_number (scheduled_number);
	    }
	    else
		return -2;
	    break;

	case ORIGIN_VALUE_SCHEDULED:
	    account_number = gsb_form_scheduler_get_account ();
	    break;
	    
	default:
	    account_number = origin;
    }
    return account_number;
}


/**
 * get the page where we are and return a number according that page :
 * ORIGIN_VALUE_OTHER : we are neither on scheduled transactions, neither transactions list, neither welcome page
 * ORIGIN_VALUE_HOME : we are on the welcome page
 * ORIGIN_VALUE_SCHEDULED : we are on the scheduled transactions
 *  0 to x : the account number where we are
 *  that function is called at each beginning to know where we are and what to do...
 *
 *  \param
 *
 *  \return the number where we are
 *  */
gint gsb_form_get_origin ( void )
{
    gint account_number;

    switch (gsb_gui_navigation_get_current_page())
    {
	case GSB_ACCOUNT_PAGE:
	    account_number = gsb_gui_navigation_get_current_account ();

	    if ( account_number != -1)
		return account_number;
	    break;

	case GSB_SCHEDULER_PAGE:
	    return ORIGIN_VALUE_SCHEDULED;
	    break;

	case GSB_HOME_PAGE:
	    return ORIGIN_VALUE_HOME;
	    break;
    }
    return ORIGIN_VALUE_OTHER;
}




/**
 * fill the form according to the account_number :
 *
 * \param account_number the number of account or -1 to get it from the account button
 * 
 * \return FALSE
 * */
gboolean gsb_form_fill_from_account ( gint account_number )
{
    gint row, column;
    gint rows_number, columns_number;
    gint form_account_number;

    devel_debug_int (account_number);

    /* free the form if necessary */
    gsb_form_widget_free_list ();

    /* account_number can be -1 if come here from the accounts choice button,
     * and -2 if there were a problem with the origin */
    switch (account_number)
    {
	case -2:
	    return FALSE;
	    break;
	    
	case-1:
	    account_number = gsb_form_scheduler_get_account ();
	    if (account_number == -2 )
		return FALSE;
	    break;
    }

    /* if each account has a separate form, get it here,
     * else, get the form of the first account */
    if (etat.formulaire_distinct_par_compte)
	form_account_number = account_number;
    else
	form_account_number = gsb_data_account_first_number ();

    rows_number = gsb_data_form_get_nb_rows (form_account_number);
    columns_number = gsb_data_form_get_nb_columns (form_account_number);

    gtk_table_resize ( GTK_TABLE (form_transaction_part),
		       rows_number,
		       columns_number );

    for ( row=0 ; row < rows_number ; row++ )
	for ( column=0 ; column < columns_number ; column++ )
	{
	    GtkWidget *widget;
	    gint element = gsb_data_form_get_value ( form_account_number, column, row );

	    /* we use form_account_number to find what is shown or not in the form,
	     * but to fill the form we need to use account_number because some elements
	     * are account dependant (ex method of payment) */
		widget = gsb_form_widget_create ( element, account_number );

	    if ( !widget )
		continue;

	    gtk_table_attach ( GTK_TABLE (form_transaction_part),
			       widget,
			       column, column+1,
			       row, row+1,
			       gsb_form_get_element_expandable ( element ),
			       gsb_form_get_element_expandable ( element ),
			       0, 0);
	    /* we want to show all the widget created except the method of payment
	     * and contra, they know themselves if they have to be shown or not,
	     * so just let them */
	    if (element != TRANSACTION_FORM_TYPE
		&&
		element != TRANSACTION_FORM_CONTRA )
		gtk_widget_show (widget);
	}
    gsb_form_clean (account_number);
    return FALSE;
}



/**
 * clean the form according to the account_number
 * and set the default values
 *
 * \param account number
 *
 * \return FALSE
 * */
gboolean gsb_form_clean ( gint account_number )
{
    GSList *tmp_list;

    devel_debug_int (account_number);

    /* clean the transactions widget */
    tmp_list = gsb_form_widget_get_list ();

    while (tmp_list)
    {
	struct_element *element;

	element = tmp_list -> data;

	/* better to protect here if widget != NULL (bad experience...) */
	if (element -> element_widget)
	{
	    /* some widgets can be set unsensitive because of the children of splits,
	     * so resensitive all to be sure */
	    gtk_widget_set_sensitive ( element -> element_widget, TRUE );

	    switch (element -> element_number)
	    {
		case TRANSACTION_FORM_DATE:
		    gsb_form_widget_set_empty ( element -> element_widget,
						TRUE );
		    gtk_entry_set_text ( GTK_ENTRY ( element -> element_widget ),
					 _("Date") );
		    break;

		case TRANSACTION_FORM_VALUE_DATE:
		    gsb_form_widget_set_empty ( element -> element_widget,
						TRUE );
		    gtk_entry_set_text ( GTK_ENTRY ( element -> element_widget ),
					 _("Value date") );
		    break;

		case TRANSACTION_FORM_EXERCICE:
		    /* editing a transaction can show some fyear wich shouldn't be showed,
		     * so hide them here */
		    gsb_fyear_update_fyear_list ();

		    /* set the combo_box on 'Automatic' */
		    gsb_fyear_set_combobox_history ( element -> element_widget,
						     0 );

		    gtk_widget_set_sensitive ( GTK_WIDGET ( element -> element_widget ),
					       FALSE );
		    break;

		case TRANSACTION_FORM_PARTY:
		    gsb_form_widget_set_empty ( GTK_COMBOFIX ( element -> element_widget ) -> entry,
						TRUE );
		    gtk_combofix_set_text ( GTK_COMBOFIX ( element -> element_widget ),
					    _("Payee") );
		    break;

		case TRANSACTION_FORM_DEBIT:
		    gsb_form_widget_set_empty ( element -> element_widget,
						TRUE );
		    gtk_entry_set_text ( GTK_ENTRY ( element -> element_widget ),
					 _("Debit") );
		    break;

		case TRANSACTION_FORM_CREDIT:
		    gsb_form_widget_set_empty ( element -> element_widget,
						TRUE );
		    gtk_entry_set_text ( GTK_ENTRY ( element -> element_widget ),
					 _("Credit") );
		    break;

		case TRANSACTION_FORM_CATEGORY:
		    gsb_form_widget_set_empty ( GTK_COMBOFIX ( element -> element_widget ) -> entry,
						TRUE );
		    gtk_combofix_set_text ( GTK_COMBOFIX ( element -> element_widget ),
					    _("Categories : Sub-categories") );
		    break;

		case TRANSACTION_FORM_FREE:
		    break;

		case TRANSACTION_FORM_BUDGET:
		    gsb_form_widget_set_empty ( GTK_COMBOFIX ( element -> element_widget ) -> entry,
						TRUE );
		    gtk_combofix_set_text ( GTK_COMBOFIX ( element -> element_widget ),
					    _("Budgetary line") );
		    break;

		case TRANSACTION_FORM_NOTES:
		    gsb_form_widget_set_empty ( element -> element_widget,
						TRUE );
		    gtk_entry_set_text ( GTK_ENTRY ( element -> element_widget ),
					 _("Notes") );
		    break;

		case TRANSACTION_FORM_TYPE:
		    gsb_payment_method_set_combobox_history ( element -> element_widget,
							      gsb_data_account_get_default_debit (account_number));
		    gtk_widget_set_sensitive ( GTK_WIDGET ( element -> element_widget ),
					       FALSE );
		    break;

		case TRANSACTION_FORM_CONTRA:
		    gtk_widget_hide ( element -> element_widget );
		    break;

		case TRANSACTION_FORM_CHEQUE:
		    gsb_form_widget_set_empty ( element -> element_widget,
						TRUE );
		    gtk_entry_set_text ( GTK_ENTRY ( element -> element_widget ),
					 _("Cheque/Transfer number") );
		    break;

		case TRANSACTION_FORM_DEVISE:
		    gsb_currency_set_combobox_history ( element -> element_widget,
							gsb_data_account_get_currency (account_number));
		    gtk_widget_set_sensitive ( GTK_WIDGET ( element -> element_widget ),
					       FALSE );
		    break;

		case TRANSACTION_FORM_CHANGE:
		    gtk_widget_hide ( element -> element_widget );

		    break;

		case TRANSACTION_FORM_BANK:
		    gsb_form_widget_set_empty ( element -> element_widget,
						TRUE );
		    gtk_entry_set_text ( GTK_ENTRY ( element -> element_widget ),
					 _("Bank references") );
		    break;

		case TRANSACTION_FORM_VOUCHER:
		    gsb_form_widget_set_empty ( element -> element_widget,
						TRUE );
		    gtk_entry_set_text ( GTK_ENTRY ( element -> element_widget ),
					 _("Voucher") );
		    break;

		case TRANSACTION_FORM_OP_NB:
		    gtk_label_set_text ( GTK_LABEL ( element -> element_widget ),
					 NULL );
		    break;

		case TRANSACTION_FORM_MODE:
		    gtk_label_set_text ( GTK_LABEL ( element -> element_widget ),
					 NULL );
		    break;
	    }
	}
	tmp_list = tmp_list -> next;
    }
    g_object_set_data ( G_OBJECT ( transaction_form ),
			"transaction_number_in_form",
			NULL );

    /* don't show the recover button */
    gtk_widget_hide (form_button_recover_split);

    return FALSE;
}




/**
 * Determine element is expandable or not in a GtkTable.
 *
 * \param element_number 
 * 
 * \return
 */
gint gsb_form_get_element_expandable ( gint element_number )
{
    switch ( element_number )
    {
	case TRANSACTION_FORM_DEVISE:
	case TRANSACTION_FORM_TYPE:
	case TRANSACTION_FORM_OP_NB:
	case TRANSACTION_FORM_MODE:
	    return GTK_SHRINK;

	default:
	    return GTK_EXPAND | GTK_FILL;
    }
}


/**
 * called when an entry get the focus, if the entry is free,
 * set it normal and erase the help content
 *
 * \param entry
 * \param ev
 *
 * \return FALSE
 * */
gboolean gsb_form_entry_get_focus ( GtkWidget *entry )
{
    /* the entry can be a combofix or a real entry */
    if (GTK_IS_COMBOFIX ( entry ))
    {
	if ( gsb_form_widget_check_empty (GTK_COMBOFIX (entry) -> entry))
	{
	    gtk_combofix_set_text ( GTK_COMBOFIX (entry), "" );
	    gsb_form_widget_set_empty ( GTK_COMBOFIX (entry) -> entry,
					FALSE );
	}
    }
    else
    {
	if ( gsb_form_widget_check_empty (entry) )
	{
	    gtk_entry_set_text ( GTK_ENTRY (entry), "" );
	    gsb_form_widget_set_empty ( entry,
					FALSE );
	}
    }
    return FALSE;
}


/**
 * called when an entry lose the focus
 *
 * \param entry
 * \param ev
 * \param ptr_origin a pointer gint wich is the number of the element
 *
 * \return FALSE
 * */
gboolean gsb_form_entry_lose_focus ( GtkWidget *entry,
				     GdkEventFocus *ev,
				     gint *ptr_origin )
{
    gchar *string;
    gint element_number;
    GtkWidget *widget;
    gint account_number;

    /* still not found, if change the content of the form, something come in entry
     * wich is nothing, so protect here */
    if ( !GTK_IS_WIDGET (entry)
	 ||
	 !GTK_IS_ENTRY (entry))
	return FALSE;

    /* remove the selection */
    gtk_editable_select_region ( GTK_EDITABLE ( entry ),
				 0,
				 0 );
    element_number = GPOINTER_TO_INT (ptr_origin);
    account_number = gsb_form_get_account_number ();

    /* sometimes the combofix popus stays showed, so remove here */
    if ( element_number == TRANSACTION_FORM_PARTY
	 ||
	 element_number == TRANSACTION_FORM_CATEGORY
	 ||
	 element_number == TRANSACTION_FORM_BUDGET )
    {
	widget = gsb_form_widget_get_widget (element_number);

	gtk_grab_remove ( GTK_COMBOFIX ( widget ) -> popup );
	gdk_pointer_ungrab ( GDK_CURRENT_TIME );
	gtk_widget_hide ( GTK_COMBOFIX ( widget ) ->popup );
    }

    /* string will be filled only if the field is empty */
    string = NULL;

    switch ( element_number )
    {
	case TRANSACTION_FORM_PARTY :
	    /* we complete the transaction */
	    if ( !gsb_form_transaction_complete_form_by_payee (gtk_entry_get_text (GTK_ENTRY (entry))))
		string = gsb_form_widget_get_name (TRANSACTION_FORM_PARTY);
	    break;

	case TRANSACTION_FORM_DEBIT :
	    /* we change the payment method to adapt it for the debit */
	    if ( strlen ( gtk_entry_get_text ( GTK_ENTRY (entry))))
	    {
		/* empty the credit */
		widget = gsb_form_widget_get_widget (TRANSACTION_FORM_CREDIT);
		if (!gsb_form_widget_check_empty (widget))
		{
		    gtk_entry_set_text ( GTK_ENTRY (widget),
					 gsb_form_widget_get_name (TRANSACTION_FORM_CREDIT));
		    gsb_form_widget_set_empty ( widget,
						TRUE );
		}

		widget = gsb_form_widget_get_widget ( TRANSACTION_FORM_TYPE);

		/* change the method of payment if necessary
		 * (if grey, it's a child of split so do nothing) */
		if ( widget
		     &&
		     GTK_WIDGET_SENSITIVE (widget))
		{
		    /* change the signe of the method of payment and the contra */
		    if ( gsb_payment_method_get_combo_sign (widget) == GSB_PAYMENT_CREDIT)
		    {
			gsb_payment_method_create_combo_list ( widget,
							       GSB_PAYMENT_DEBIT,
							       account_number, 0 );
			/* if there is no payment method, the last function hide it, but we have
			 * to hide the cheque element too */
			if ( !GTK_WIDGET_VISIBLE (widget))
			    gtk_widget_hide (gsb_form_widget_get_widget (TRANSACTION_FORM_CHEQUE));

			widget = gsb_form_widget_get_widget (TRANSACTION_FORM_CONTRA);
			if ( GTK_WIDGET_VISIBLE (widget))
			    gsb_payment_method_create_combo_list ( gsb_form_widget_get_widget (TRANSACTION_FORM_CONTRA),
								   GSB_PAYMENT_CREDIT,
								   account_number, 0 );
		    }
		}
		gsb_form_check_auto_separator (entry);
	    }
	    else
		string = gsb_form_widget_get_name (TRANSACTION_FORM_DEBIT);
	    break;

	case TRANSACTION_FORM_CREDIT :
	    /* we change the payment method to adapt it for the debit */
	    if ( strlen ( gtk_entry_get_text ( GTK_ENTRY (entry))))
	    {
		/* empty the credit */
		widget = gsb_form_widget_get_widget (TRANSACTION_FORM_DEBIT);
		if (!gsb_form_widget_check_empty (widget))
		{
		    gtk_entry_set_text ( GTK_ENTRY (widget),
					 gsb_form_widget_get_name (TRANSACTION_FORM_DEBIT));
		    gsb_form_widget_set_empty ( widget,
						TRUE );
		}
		widget = gsb_form_widget_get_widget ( TRANSACTION_FORM_TYPE);

		/* change the method of payment if necessary
		 * (if grey, it's a child of split so do nothing) */
		if ( widget
		     &&
		     GTK_WIDGET_SENSITIVE (widget))
		{
		    /* change the signe of the method of payment and the contra */
		    if ( gsb_payment_method_get_combo_sign (widget) == GSB_PAYMENT_DEBIT)
		    {
			gsb_payment_method_create_combo_list ( widget,
							       GSB_PAYMENT_CREDIT,
							       account_number, 0 );
			/* if there is no payment method, the last function hide it, but we have
			 * to hide the cheque element too */
			if ( !GTK_WIDGET_VISIBLE (widget))
			    gtk_widget_hide (gsb_form_widget_get_widget (TRANSACTION_FORM_CHEQUE));

			widget = gsb_form_widget_get_widget ( TRANSACTION_FORM_CONTRA);
			if ( GTK_WIDGET_VISIBLE (widget))
			    gsb_payment_method_create_combo_list ( widget,
								   GSB_PAYMENT_DEBIT,
								   account_number, 0 );
		    }
		}
		gsb_form_check_auto_separator (entry);
	    }
	    else
		string = gsb_form_widget_get_name (TRANSACTION_FORM_CREDIT);
	    break;

	case TRANSACTION_FORM_CATEGORY :
	    if ( strlen ( gtk_entry_get_text ( GTK_ENTRY (entry))))
	    {
		/* if it's a transafer, set the content of the contra combo */
		if ( gsb_data_form_check_for_value ( TRANSACTION_FORM_CONTRA ))
		{
		    /* if it's a transfer, set the contra_method of payment menu */
		    gint contra_account_number;

		    if ( (contra_account_number = gsb_form_check_for_transfer ( gtk_entry_get_text ( GTK_ENTRY (entry)))) >= 0
			 &&
			 contra_account_number != account_number )
		    {
			if ( gsb_form_widget_check_empty (gsb_form_widget_get_widget (TRANSACTION_FORM_CREDIT)))
			    /* there is something in debit */
			    gsb_payment_method_create_combo_list ( gsb_form_widget_get_widget (TRANSACTION_FORM_CONTRA),
								   GSB_PAYMENT_CREDIT,
								   contra_account_number, 0 );
			else
			    /* there is something in credit */
			    gsb_payment_method_create_combo_list ( gsb_form_widget_get_widget (TRANSACTION_FORM_CONTRA),
								   GSB_PAYMENT_DEBIT,
								   contra_account_number, 0 );
		    }
		    else
			gtk_widget_hide ( gsb_form_widget_get_widget (TRANSACTION_FORM_CONTRA));
		}
	    }
	    else
		string = gsb_form_widget_get_name (TRANSACTION_FORM_CATEGORY);
	    break;

	case TRANSACTION_FORM_CHEQUE :
	case TRANSACTION_FORM_BUDGET :
	case TRANSACTION_FORM_VOUCHER :
	case TRANSACTION_FORM_NOTES :
	case TRANSACTION_FORM_BANK :
	    if ( !strlen ( gtk_entry_get_text ( GTK_ENTRY ( entry ))))
		string = gsb_form_widget_get_name (element_number);
	    break;

	default :
	    break;
    }

    /* if string is not NULL, the entry is empty so set the empty field to TRUE */
    if ( string )
    {
	switch ( element_number)
	{
	    case TRANSACTION_FORM_PARTY :
	    case TRANSACTION_FORM_CATEGORY :
	    case TRANSACTION_FORM_BUDGET :
		/* need to work with the combofix to avoid some signals if we work
		 * directly on the entry */
		gtk_combofix_set_text ( GTK_COMBOFIX ( gsb_form_widget_get_widget (element_number) ),
					string );
		break;

	    default:
		gtk_entry_set_text ( GTK_ENTRY ( entry ), string );
		break;
	}
	gsb_form_widget_set_empty ( entry,
				    TRUE );
    }
    return FALSE;
}


/**
 * called when leave an amount entry, il the automatic separator
 * is on, set the separator in the entry if there is none
 *
 * \param entry
 *
 * \return
 * */
void gsb_form_check_auto_separator ( GtkWidget *entry )
{
    gint account_number;
    gchar *string;
    gint floating_point;
    gchar *tmp = NULL;
    gint i;

    if (!etat.automatic_separator
	||
	!entry )
	return;
    
    /* we need a my_strdup to permit to do the g_free later
     * because if strlen < floating point we need to
     * malloc another string */
    string = my_strdup (gtk_entry_get_text (GTK_ENTRY(entry)));

    account_number = gsb_form_get_account_number ();
    floating_point = gsb_data_currency_get_floating_point (gsb_data_account_get_currency (account_number));

    if ( strchr (string, '.')
	 ||
	 strchr (string, ','))
    {
	g_free (string);
	return;
    }

    /* if string is < the floating_point, increase it to have
     * 1 character more (to set the 0 before the .) */
    if (strlen(string) <= floating_point)
    {
	gchar *concat_tmp;

	tmp = g_malloc (floating_point - strlen(string) + 2);
	for (i=0 ; i<(floating_point - strlen(string) + 1) ; i++)
	    tmp[i] = '0';
	tmp[floating_point - strlen(string) + 1] = 0;
	concat_tmp = g_strconcat ( tmp,
				   string,
				   NULL );
	g_free (tmp);
	g_free (string);
	string = concat_tmp;
    }

    tmp = g_malloc ((strlen(string)+2) * sizeof (gchar));

    memcpy ( tmp, string, strlen(string) - floating_point);

    i = strlen(string) - floating_point;
    tmp[i] = '.';
    i++;
    memcpy ( tmp + i,
	     string + i - 1,
	     floating_point );
    i = i + floating_point;
    tmp[i] = 0;
    gtk_entry_set_text (GTK_ENTRY (entry),
			tmp );
    g_free (tmp);
    g_free (string);
}


/**
 * check if the string in the param is like "Transfer : account"
 * if yes, return the number of the account
 * if deleted account, return -2
 * if no return -1
 *
 * \param entry_string
 *
 * \return the number of the transfer_account
 * 		-1 if transfer to a non existant account
 * 		-2 if deleted account
 * 		-3 if not a transfer
 * */
gint gsb_form_check_for_transfer ( const gchar *entry_string )
{
    const gchar *account_name;

    if ( !entry_string
	 ||
	 strncmp ( entry_string,
		   _("Transfer : "),
		   strlen (_("Transfer : "))))
	 return -3;

    account_name = memchr ( entry_string,
			    ':',
			    strlen (entry_string));
    /* after the : there is a space before the account name */
    account_name = account_name + 2;

    if ( strcmp ( account_name,
		  _("Deleted account")))
	return gsb_data_account_get_no_account_by_name (account_name);

    else
	return -2;
}


/**
 * called when we press the button in an entry field in
 * the form
 * if "transaction_number_in_form" as data of transaction_form is NULL,
 * do the necessary to begin a new empty transaction
 *
 * \param entry wich receive the signal
 * \param ev can be NULL
 * \param ptr_origin a pointer to int on the element_number
 *
 * \return FALSE
 * */
gboolean gsb_form_button_press_event ( GtkWidget *entry,
				       GdkEventButton *ev,
				       gint *ptr_origin )
{
    gint element_number;
    GtkWidget *widget;

    element_number = GPOINTER_TO_INT (ptr_origin);

    /* we do the first part only if we click on the form directly, without double click or
     * entry in the transaction list,
     * in that case, transaction_number_in_form is 0 and set to -1, as a white line */
    if (!g_object_get_data (G_OBJECT (transaction_form), "transaction_number_in_form"))
    {
	GtkWidget *date_entry;

	/* set the new transaction number */
	g_object_set_data ( G_OBJECT (transaction_form),
			    "transaction_number_in_form",
			    GINT_TO_POINTER (-1));

	/* set the current date into the date entry */
	date_entry = gsb_form_widget_get_widget (TRANSACTION_FORM_DATE);
	if ( gsb_form_widget_check_empty (date_entry))
	{
	    gtk_entry_set_text ( GTK_ENTRY (date_entry),
				 gsb_date_today ());
	    gsb_form_widget_set_empty ( date_entry,
					FALSE );
	}

	/* set the form sensitive */
	gsb_form_change_sensitive_buttons (TRUE);

	/* if we are on scheduled transactions, show the scheduled part of form */
	if (gsb_form_get_origin () == ORIGIN_VALUE_SCHEDULED)
	    gsb_form_scheduler_sensitive_buttons (TRUE);


	/* set the number of cheque for the method of payment if necessary */
	widget = gsb_form_widget_get_widget (TRANSACTION_FORM_TYPE);

	if ( widget
	     &&
	     GTK_WIDGET_VISIBLE (widget))
	{
	    gint payment_number;
	    gint account_number;

	    account_number = gsb_form_get_account_number ();
	    payment_number = gsb_payment_method_get_selected_number (widget);
	    if ( gsb_data_payment_get_automatic_numbering ( payment_number ))
	    {
		widget = gsb_form_widget_get_widget (TRANSACTION_FORM_CHEQUE);

		gsb_form_entry_get_focus (widget);

		if ( !strlen (gtk_entry_get_text ( GTK_ENTRY (widget))))
		{
		    gchar* tmpstr = utils_str_itoa (gsb_data_payment_get_last_number (payment_number) + 1);
		    gtk_entry_set_text ( GTK_ENTRY (widget), tmpstr);
		    g_free ( tmpstr );
		}
	    }
	}
    }

    /* if ev is NULL, go away here
     * (means come from a function as gsb_form_finish_edition...) */
    if ( !ev )
	return FALSE;

    return FALSE;
}


/**
 * sensitive or unsensitive all the buttons on the form
 *
 * \param sensitive TRUE or FALSE
 *
 * \return FALSE
 * */
gboolean gsb_form_change_sensitive_buttons ( gboolean sensitive )
{
    if ( gsb_data_form_check_for_value ( TRANSACTION_FORM_TYPE ))
	gtk_widget_set_sensitive ( GTK_WIDGET ( gsb_form_widget_get_widget (TRANSACTION_FORM_TYPE)),
				   sensitive );

    if ( gsb_data_form_check_for_value ( TRANSACTION_FORM_DEVISE ))
	gtk_widget_set_sensitive ( GTK_WIDGET ( gsb_form_widget_get_widget (TRANSACTION_FORM_DEVISE)),
				   sensitive );

    if ( gsb_data_form_check_for_value ( TRANSACTION_FORM_EXERCICE ))
	gtk_widget_set_sensitive ( GTK_WIDGET ( gsb_form_widget_get_widget (TRANSACTION_FORM_EXERCICE)),
				   sensitive );
    return FALSE;
}


/**
 * called when press a key on an element of the form
 *
 * \param widget wich receive the signal
 * \param ev
 * \param ptr_origin a pointer number of the element
 * 
 * \return FALSE
 * */
gboolean gsb_form_key_press_event ( GtkWidget *widget,
				    GdkEventKey *ev,
				    gint *ptr_origin )
{
    gint element_number;
    gint account_number;
    gint element_suivant;

    element_number = GPOINTER_TO_INT (ptr_origin);
    account_number = gsb_form_get_account_number ();

    /* if etat.entree = 1, entry finish the transaction, else does as tab */
    if ( !etat.entree
	 &&
	 ( ev -> keyval == GDK_Return 
	   ||
	   ev -> keyval == GDK_KP_Enter ))
	ev->keyval = GDK_Tab ;

    switch ( ev -> keyval )
    {
	case GDK_Escape :
	    gsb_form_escape_form ();
	    break;

	case GDK_Up:
	    element_suivant = gsb_form_widget_next_element ( account_number,
							     element_number,
							     GSB_UP );
	    gsb_form_widget_set_focus ( element_suivant );
	    return TRUE;
	    break;

	case GDK_Down:
	    element_suivant = gsb_form_widget_next_element ( account_number,
							     element_number,
							     GSB_DOWN );
	    gsb_form_widget_set_focus ( element_suivant );
	    return TRUE;
	    break;

	case GDK_ISO_Left_Tab:
	    element_suivant = gsb_form_widget_next_element ( account_number,
							     element_number,
							     GSB_LEFT );
	    gsb_form_widget_set_focus ( element_suivant );
	    return TRUE;
	    break;

	case GDK_Tab :
	    element_suivant = gsb_form_widget_next_element ( account_number,
							     element_number,
							     GSB_RIGHT );

	    if ( element_suivant == -2 )
		gsb_form_finish_edition();
	    else
		gsb_form_widget_set_focus ( element_suivant );
	    return TRUE;
	    break;

	case GDK_KP_Enter :
	case GDK_Return :

	    /* need to check here if we are performing a scheduled transaction in home page
	     * or another transaction, because if we are in home page, cannot finish like that,
	     * we need to finish with the dialog wich contains the form */
	    if (gsb_gui_navigation_get_current_page () == GSB_HOME_PAGE)
	    {
		GtkWidget *tmp_widget;

		/* ok, we are on the home page. the best way is to send the response ok
		 * to the dialog */
		tmp_widget = widget;

		do
		{
		    tmp_widget = gtk_widget_get_parent (GTK_WIDGET (tmp_widget));
		}
		while ( GTK_IS_WIDGET (tmp_widget)
			&&
			!GTK_IS_DIALOG (tmp_widget));
		gtk_dialog_response (GTK_DIALOG (tmp_widget), GTK_RESPONSE_OK);
		return TRUE;
	    }
	    else
	    {
		gsb_form_finish_edition();
		return TRUE;
	    }
	    break;

	case GDK_KP_Add:
	case GDK_plus:
	case GDK_equal:		/* This should make all our US users happy */

	    /* increase the check of 1 */
	    if (element_number == TRANSACTION_FORM_CHEQUE)
	    {
		increment_decrement_champ ( widget,
					    1 );
		return TRUE;
	    }
	    break;

	case GDK_KP_Subtract:
	case GDK_minus:

	    /* decrease the check of 1 */
	    if (element_number == TRANSACTION_FORM_CHEQUE)
	    {
		increment_decrement_champ ( widget,
					    -1 );
		return TRUE;
	    }
	    break;
    }
    return FALSE;
}



/** 
 * called when the user finishes the edition of the form
 * this function works for transaction and scheduled transactions,
 * it add/modify the transaction in the form
 * 
 * \param none
 * 
 * \return FALSE
 * */
gboolean gsb_form_finish_edition ( void )
{
    gint transaction_number;
    GSList *payee_list;
    GSList *list_tmp;
    gint account_number;
    gint new_transaction;
    gboolean is_transaction;
    gboolean execute_scheduled = FALSE;
    gint saved_scheduled_number = 0;

    devel_debug (NULL);

    /* get the number of the transaction, stored in the form (<0 if new) */
    transaction_number = GPOINTER_TO_INT (g_object_get_data ( G_OBJECT ( transaction_form ),
							      "transaction_number_in_form" ));

    /* set a debug here, if transaction_number is 0, should look for where it comes */
    if (!transaction_number)
	notice_debug ("Coming in gsb_form_finish_edition with a 0 number of transaction. This is a bug,\nplease try to do it again and report the bug.");

    account_number = gsb_form_get_account_number ();

    /* check if we finish a transaction or a scheduled transaction
     * we have to decide if it's a transaction or a scheduled transaction,
     * and if it's a scheduled, if we execute it (and create transaction) or work on it*/
    if (gsb_form_get_origin () == ORIGIN_VALUE_SCHEDULED
	||
	gsb_form_get_origin () == ORIGIN_VALUE_HOME )
    {
	if (g_object_get_data ( G_OBJECT (transaction_form), "execute_scheduled"))
	   {
	       /* we want to execute the scheduled transaction */
	       is_transaction = TRUE;
	       execute_scheduled = TRUE;

	       /* we need to keep the number of scheduled, to check later if there is
		* some children and modifie the scheduled transaction */
	       saved_scheduled_number = transaction_number;
	       /* as it's a new transaction, do the same as a white line */
	       transaction_number = -1;
	   }
	else
	    is_transaction = FALSE;
    }
    else
	is_transaction = TRUE;

    /* a new transaction has a number < 0
     * -1 for the general white line
     *  -2, -3, ... for the white lines of scheduled transactions
     *  or it's an execution of scheduled transaction */
    if ( transaction_number < 0 ) 
	new_transaction = 1;
    else
	new_transaction = 0;

    /* the current widget has to lose the focus to make all the changes if necessary */
    gtk_widget_grab_focus (navigation_tree_view);
    
    /* check if the datas are ok */
    if ( !gsb_form_validate_form_transaction (transaction_number, is_transaction))
	return FALSE;

    /* if the party is a report, we make as transactions as the number of parties in the
     * report. So we create a list with the party's numbers or -1 if it's a normal
     * party */
    payee_list = gsb_form_transaction_get_parties_list_from_report ();

    /* now we go throw the list */
    list_tmp = payee_list;

    while ( list_tmp )
    {
	if ( GPOINTER_TO_INT (list_tmp -> data) == -1 )
	    /* it's a normal party, we set the list_tmp to NULL */
	    list_tmp = NULL;
	else
	{
	    /* it's a report, so each time we come here we set the parti's combofix to the
	     * party of the report */

	    if ( !list_tmp -> data )
	    {
		dialogue_error ( _("No payee selected for this report."));
		return FALSE;
	    }
	    else
	    {
		gtk_combofix_set_text ( GTK_COMBOFIX ( gsb_form_widget_get_widget (TRANSACTION_FORM_PARTY)),
					gsb_data_payee_get_name ( GPOINTER_TO_INT (list_tmp -> data), TRUE ));

		/* if it's not the first party and the method of payment has to change its number (cheque),
		 * we increase the number. as we are in a party's list, it's always a new transactio, 
		 * so we know that it's not the first if transaction_number is not 0 */
		if ( transaction_number )
		{
		    gint payment_number;

		    /* needn't to use mix here because can only be a transaction */
		    payment_number = gsb_data_transaction_get_method_of_payment_number (transaction_number);

		    if ( gsb_data_payment_get_automatic_numbering (payment_number)
			 &&
			 gsb_data_form_check_for_value ( TRANSACTION_FORM_CHEQUE ))
		    {
			gchar* tmpstr = utils_str_itoa (gsb_data_payment_get_last_number (payment_number) + 1);
			gtk_entry_set_text ( GTK_ENTRY ( gsb_form_widget_get_widget (TRANSACTION_FORM_CHEQUE)), tmpstr);
			g_free ( tmpstr ) ;
		    }
		}
		list_tmp = list_tmp -> next;
	    }
	}

	/* now we create the transaction if necessary and set the mother in case of child of split */
	if ( new_transaction )
	{
	    /* it's a new transaction, we create it, and set the mother if necessary */
	    gint mother_transaction = 0;

	    /* if we are on a white child (ie number < -1, -1 is only for the general white line), get the mother of transaction */
	    if ( transaction_number < -1 )
		mother_transaction = gsb_data_mix_get_mother_transaction_number (transaction_number, is_transaction);

	    transaction_number = gsb_data_mix_new_transaction (account_number, is_transaction);

	    gsb_data_mix_set_mother_transaction_number ( transaction_number,
							 mother_transaction,
							 is_transaction );
	}
	else
	{
	    /* it's not a new transaction, if it's not a child split,
	     * we remove the amount of that transaction from the balance of the account,
	     * because later, the amount will be add again to the balance */
	    if ( is_transaction
		 &&
		 !gsb_data_transaction_get_mother_transaction_number (transaction_number))
		gsb_data_account_set_current_balance ( account_number,
						       gsb_real_sub ( gsb_data_account_get_current_balance (account_number),
								      gsb_data_transaction_get_adjusted_amount (transaction_number, -1)));
	}

	/* take the datas in the form, except the category */
	gsb_form_take_datas_from_form ( transaction_number, is_transaction );

	/* perhaps the currency button is not shown
	 * in that case, we give the account currency to that transaction */
	if ( new_transaction
	     &&
	     !gsb_form_widget_get_widget (TRANSACTION_FORM_DEVISE))
	    gsb_data_mix_set_currency_number ( transaction_number,
					       gsb_data_account_get_currency (account_number),
					       is_transaction );

	/* get the category and do the stuff with that (contra-transaction...) */
	gsb_form_get_categories ( transaction_number,
				  new_transaction,
				  is_transaction );

	/* for the rest we need to split for transactions/scheduled */
	if (is_transaction)
	{
	    /* it's a transaction or an execution of scheduled transaction */
	    if ( new_transaction )
	    {
		gint split_transaction_number;

		gsb_transactions_list_append_new_transaction (transaction_number, TRUE);

		/* if it's a real new transaction and if it's a split, we ask if the user wants
		 * to recover previous children */
		if ( gsb_data_transaction_get_split_of_transaction (transaction_number)
		     &&
		     !execute_scheduled
		     &&
		     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (form_button_recover_split))
		     &&
		     (split_transaction_number = gsb_form_transactions_look_for_last_party ( gsb_data_transaction_get_party_number (transaction_number),
												 transaction_number,
												 gsb_data_transaction_get_account_number(transaction_number))))
		    gsb_form_transaction_recover_splits_of_transaction ( transaction_number,
									     split_transaction_number);
	    }
	    else
	    {
		/* update a transaction */
		gsb_transactions_list_update_transaction (transaction_number);

		/* we are on a modification of transaction, but if the modified transaction is a split
		 * and has no children (ie only white line), we assume the user wants now fill the children, so we will do the
		 * same as for a new transaction : open the expander and select the white line */
		if (transaction_list_get_n_children (transaction_number) == 1)
		{
		    new_transaction = TRUE;
		    gsb_transactions_list_switch_expander (transaction_number);
		}
	    }
	}
	else
	{
	    /* it's a scheduled transaction */
	    gsb_form_scheduler_get_scheduler_part (transaction_number);

	    if (new_transaction)
		gsb_scheduler_list_append_new_scheduled ( transaction_number,
							  gsb_scheduler_list_get_end_date_scheduled_showed ());
	    else
		gsb_scheduler_list_update_transaction_in_list (transaction_number);

	    /* needed for the two in case of we change the date */
	    gsb_scheduler_list_set_background_color (gsb_scheduler_list_get_tree_view ());
	}
    }
    g_slist_free ( payee_list );

    /* if it's a reconciliation and we modify a transaction, check
     * the amount of marked transactions */
    if ( is_transaction
	 &&
	 etat.equilibrage )
    {
	if (new_transaction)
	    /* we are reconciling and it's a new transaction, so need to show the checkbox */
	    transaction_list_show_toggle_mark (TRUE);
	else
	{
	    /* we are reconciling and it's a modification of transaction, need to recalculate the marked balance */
	    gsb_data_account_calculate_marked_balance (account_number);
	    gsb_reconcile_update_amounts (NULL, NULL);
	}
    }

    /* if we executed a scheduled transation, need to increase the date of the scheduled
     * and execute the children if it's a split */
    if (execute_scheduled)
    {
	gint increase_result;

	/* first, check if it's a scheduled split and execute the childrent */
	if (gsb_data_scheduled_get_split_of_scheduled (saved_scheduled_number))
	    gsb_scheduler_execute_children_of_scheduled_transaction ( saved_scheduled_number,
								      transaction_number );

	/* now we can increase the scheduled transaction */
	increase_result = gsb_scheduler_increase_scheduled (saved_scheduled_number);

	/* the next step is to update the list, but do it only if we are on the scheduled
	 * list, else we needn't because the update will be done when going to that list */
	if (gsb_gui_navigation_get_current_page () == GSB_SCHEDULER_PAGE)
	{
	    if (increase_result)
		gsb_scheduler_list_update_transaction_in_list (saved_scheduled_number);

	    gsb_scheduler_list_set_background_color (gsb_scheduler_list_get_tree_view ());
	}
    }

    /* if it was a new transaction, do the stuff to do another new transaction */
    if ( new_transaction
	 &&
	 !execute_scheduled)
    {
	/* we are on a new transaction, if that transaction is a split,
	 * we give the focus to the new white line created for that and
	 * edit it, for that we need to open the transaction to select the
	 * white line, and set it as current transaction */
	if (gsb_data_mix_get_split_of_transaction (transaction_number, is_transaction))
	{
	    /* it's a split */
	    gint white_line_number;

	    white_line_number = gsb_data_mix_get_white_line (transaction_number, is_transaction);
	    if (is_transaction)
		transaction_list_select (white_line_number);
	    else
		gsb_scheduler_list_select (white_line_number);
	}

	/* it was a new transaction, we save the last date entry */
	gsb_date_set_last_date (gtk_entry_get_text ( GTK_ENTRY (gsb_form_widget_get_widget (TRANSACTION_FORM_DATE))));

	/* we need to use edit_transaction to make a new child split if necessary */
	if (is_transaction)
	    gsb_transactions_list_edit_transaction (gsb_data_account_get_current_transaction_number (account_number));
	else
	    gsb_scheduler_list_edit_transaction (gsb_scheduler_list_get_current_scheduled_number ());
    }
    else
	gsb_form_hide ();

    /* show the warnings */
    if (is_transaction)
    {
	affiche_dialogue_soldes_minimaux ();
	update_transaction_in_trees (transaction_number);
    }

    modification_fichier ( TRUE );
    return FALSE;
}


/**
 * check if the transaction/scheduled transaction in the form is correct
 *
 * \param transaction_number can be -2 for a new daughter scheduled transaction, 0 for a new transaction
 * 		or any number corresponding to an older transaction
 * \param is_transaction TRUE if it's for a transaction, FALSE for a scheduled transaction
 *
 * \return TRUE or FALSE
 * */
gboolean gsb_form_validate_form_transaction ( gint transaction_number,
					      gboolean is_transaction )
{
    gint account_number;
    GtkWidget *widget;
    gint mother_number;

    devel_debug_int (transaction_number);

    account_number = gsb_form_get_account_number ();

    /* begin to work with dates */
    widget = gsb_form_widget_get_widget (TRANSACTION_FORM_DATE);

    /* check the date exists */
    if (gsb_form_widget_check_empty (widget))
    {
	dialogue_error ( _("You must enter a date.") );
	return (FALSE);
    }

    /* check the date ok */
    if ( !gsb_date_check_entry (widget))
    {
        gchar* tmpstr = g_strdup_printf ( _("Invalid date %s"),
					   gtk_entry_get_text (GTK_ENTRY (widget)));
	dialogue_error ( tmpstr );
	g_free(tmpstr);
	gtk_entry_select_region ( GTK_ENTRY (widget),
				  0,
				  -1);
	gtk_widget_grab_focus (widget);
	return (FALSE);
    }

    /* work with value date */
    widget = gsb_form_widget_get_widget (TRANSACTION_FORM_VALUE_DATE);
    if ( widget
	 &&
	 !gsb_form_widget_check_empty (widget)
	 &&
	 !gsb_date_check_entry (widget))
    {
        gchar* tmpstr = g_strdup_printf ( _("Invalid value date %s"),
					   gtk_entry_get_text (GTK_ENTRY (widget)));
	dialogue_error ( tmpstr );
	g_free ( tmpstr );
	gtk_entry_select_region ( GTK_ENTRY (widget),
				  0,
				  -1);
	gtk_widget_grab_focus (widget);
	return (FALSE);
    }

    /* now work with the categories */
    widget = gsb_form_widget_get_widget (TRANSACTION_FORM_CATEGORY);

    mother_number = gsb_data_mix_get_mother_transaction_number (transaction_number, is_transaction);

    /* check if it's a daughter split that the category is not a split of transaction */
    if ( widget
	 &&
	 !gsb_form_widget_check_empty (GTK_COMBOFIX (widget) -> entry)
	 &&
	 mother_number
	 &&
	 !strcmp ( gtk_combofix_get_text (GTK_COMBOFIX (widget)),
		   _("Split of transaction")))
    {
	dialogue_error ( _("You cannot set split of transaction in category for a daughter of a split of transaction.") );
	return (FALSE);
    }

    /* check it's not a transfer on itself and the contra-account exists
     * !!! widget is already set to the category, don't change it before */
    if ( widget
	 &&
	 !gsb_form_widget_check_empty (GTK_COMBOFIX (widget) -> entry))
    {
	gint account_transfer;

	account_transfer = gsb_form_check_for_transfer (gtk_combofix_get_text (GTK_COMBOFIX (widget)));

	switch (account_transfer)
	{
	    /* if the check returns -3, it's not a transfer, so it's a normal category */
	    case -3:
		break;

		/* if the check returns -2, it's a deleted account (refuse for scheduled transaction)*/
	    case -2:
		if (is_transaction)
		{
		    gsb_data_transaction_set_category_number ( transaction_number,
							       0 );
		    gsb_data_transaction_set_sub_category_number ( transaction_number,
								   0 );
		    gsb_data_transaction_set_contra_transaction_account ( transaction_number,
								       -1);
		    gsb_data_transaction_set_contra_transaction_number ( transaction_number,
									   -1);
		}
		else
		{
		    dialogue_error ( _("Cannot associate a transfer to a deleted account in a scheduled transaction."));
		    return FALSE;
		}
		break;

		/* if the check returns -1, it's a non existant account */
	    case -1:
		dialogue_error ( _("There is no associated account for this transfer or associated account is invalid.") );
		return (FALSE);
		break;

		/* all other values are a number of account */
	    default :
		if (account_transfer == account_number)
		{
		    dialogue_error ( _("Can't issue a transfer its own account.") );
		    return (FALSE);
		}
		if ( gsb_data_account_get_closed_account (account_transfer) )
		{
		    dialogue_error ( _("Can't issue a transfer on a closed account." ));
		    return ( FALSE );
		}
	}
    }

    /* for the automatic method of payment entry (especially cheques)
     * check if not used before (only for new transaction) */
    widget = gsb_form_widget_get_widget (TRANSACTION_FORM_CHEQUE);

    if ( widget
	 &&
	 GTK_WIDGET_VISIBLE (widget))
    {
	GtkWidget *combo_box;
	gint payment;

	combo_box = gsb_form_widget_get_widget (TRANSACTION_FORM_TYPE);
	payment = gsb_payment_method_get_selected_number (combo_box);

	if (gsb_data_payment_get_automatic_numbering (payment))
	{
	    /* check if there is something in */

	    if (gsb_form_widget_check_empty (widget))
	    {
		if (!question ( _("Selected method of payment has an automatic incremental number\nbut doesn't contain any number.\nContinue anyway?")))
		    return (FALSE);
	    }
	    else
	    {
		/* check that the number is not used */
		gint tmp_transaction_number;
		
		tmp_transaction_number = gsb_data_transaction_check_content_payment (payment, utils_str_atoi (gtk_entry_get_text (GTK_ENTRY (widget))));

		if ( tmp_transaction_number
		     &&
		     tmp_transaction_number != transaction_number
		     &&
		     !question ( _("Warning: this cheque number is already used.\nContinue anyway?")))
		    return FALSE;
	    }
	}
    }

    /* check if the payee is a report, it must be a new transaction */
    widget = gsb_form_widget_get_widget (TRANSACTION_FORM_PARTY);

    if (widget)
    {
	if (!strncmp ( gtk_combofix_get_text (GTK_COMBOFIX (widget)),
		       _("Report"),
		       strlen (_("Report"))))
	{
	    gchar **tab_char;

	    /* check if it's a new transaction */
	    if (transaction_number > 0)
	    {
		dialogue_error ( _("A transaction with a multiple payee must be a new one.") );
		return (FALSE);
	    }
	    if (transaction_number < 0)
	    {
		dialogue_error ( _("A transaction with a multiple payee cannot be a split child.") );
		return (FALSE);
	    }

	    /* check if the report exists */
	    tab_char = g_strsplit ( gtk_combofix_get_text (GTK_COMBOFIX (widget)),
				    " : ",
				    2 );

	    if (!tab_char[1])
	    {
		dialogue_error  ( _("The word \"Report\" is reserved. Please use another one."));
		g_strfreev (tab_char);
		return FALSE;
	    }

	    if (!gsb_data_report_get_report_by_name (tab_char[1]))
	    {
		dialogue_error ( _("Invalid multiple payee.") );
		g_strfreev (tab_char);
		return (FALSE);
	    }
	    g_strfreev (tab_char);
	}
    }
    return ( TRUE );
}


/**
 * get the datas in the form and set them in the transaction/scheduled transaction in param
 * 
 * \param transaction_number the transaction to modify
 * \param is_transaction TRUE if it's for a transaction, FALSE for a scheduled transaction
 * 
 * \return
 * */
void gsb_form_take_datas_from_form ( gint transaction_number,
				     gboolean is_transaction )
{
    GSList *tmp_list;
    GDate *date;

    devel_debug_int (transaction_number);

    /* we set a date variable to avoid to parse 2 times, one time for the date,
     * and perhaps a second time with the financial year
     * (cannot take it from the transaction if the fyear field is before the date field...) */
    /* get the date first, because the financial year will use it and it can set before the date in the form */
    date = gsb_calendar_entry_get_date (gsb_form_widget_get_widget (TRANSACTION_FORM_DATE));

    tmp_list = gsb_form_widget_get_list ();

    while (tmp_list)
    {
	struct_element *element;

	element = tmp_list -> data;

	switch (element -> element_number)
	{
	    case TRANSACTION_FORM_DATE:
		/* set date before */
		gsb_data_mix_set_date ( transaction_number, date, is_transaction );

		break;

	    case TRANSACTION_FORM_VALUE_DATE:
		if (gsb_form_widget_check_empty (element -> element_widget)) 
		    gsb_data_mix_set_value_date ( transaction_number, NULL, is_transaction );
		else
		    gsb_data_mix_set_value_date ( transaction_number,
						  gsb_calendar_entry_get_date (element -> element_widget),
						  is_transaction );
		break;

	    case TRANSACTION_FORM_EXERCICE:
		/* FIXME : voir pour faire la diff�rence date/date de valeur � partir de la conf ? pour la date pour choisir l'exo*/
		gsb_data_mix_set_financial_year_number ( transaction_number,
							 gsb_fyear_get_fyear_from_combobox ( element -> element_widget, date ), is_transaction);
		break;

	    case TRANSACTION_FORM_PARTY:
		if (gsb_form_widget_check_empty (element -> element_widget)) 
		    gsb_data_mix_set_party_number ( transaction_number, 0, is_transaction );
		else
		{
		    gsb_data_mix_set_party_number ( transaction_number, gsb_data_payee_get_number_by_name ( gtk_combofix_get_text ( GTK_COMBOFIX ( element -> element_widget )), TRUE ), is_transaction);
		    gsb_payee_update_combofix ();
		}
		break;

	    case TRANSACTION_FORM_DEBIT:
		if (!gsb_form_widget_check_empty (element -> element_widget)) 
		    gsb_data_mix_set_amount ( transaction_number, gsb_real_opposite (gsb_utils_edit_calculate_entry ( element -> element_widget )), is_transaction);
		break;

	    case TRANSACTION_FORM_CREDIT:
		if (!gsb_form_widget_check_empty (element -> element_widget)) 
		    gsb_data_mix_set_amount ( transaction_number, gsb_utils_edit_calculate_entry ( element -> element_widget ), is_transaction);
		break;

	    case TRANSACTION_FORM_BUDGET:
		if (gsb_form_widget_check_empty (element -> element_widget)) 
		{
		    gsb_data_mix_set_budgetary_number ( transaction_number, 0, is_transaction );
		    gsb_data_mix_set_sub_budgetary_number ( transaction_number, 0, is_transaction );
		}
		else
		    gsb_data_budget_set_budget_from_string ( transaction_number,
							     gtk_combofix_get_text ( GTK_COMBOFIX ( element -> element_widget )),
							     is_transaction );
		break;

	    case TRANSACTION_FORM_NOTES:
		if (gsb_form_widget_check_empty (element -> element_widget)) 
		    gsb_data_mix_set_notes ( transaction_number, NULL, is_transaction );
		else
		{
		    gchar* tmpstr = my_strdup ( gtk_entry_get_text ( GTK_ENTRY ( element -> element_widget )));
		    gsb_data_mix_set_notes ( transaction_number, tmpstr , is_transaction);
		    g_free ( tmpstr );
		}
		break;

	    case TRANSACTION_FORM_TYPE:
		/* set the type only if visible */
		if ( GTK_WIDGET_VISIBLE ( gsb_form_widget_get_widget (TRANSACTION_FORM_TYPE)))
		{
		    GtkWidget *widget_tmp;
		    gint payment_number;

		    payment_number = gsb_payment_method_get_selected_number (element -> element_widget);

		    gsb_data_mix_set_method_of_payment_number ( transaction_number, payment_number, is_transaction);

		    /* set the number of cheque only if visible */
		    widget_tmp = gsb_form_widget_get_widget (TRANSACTION_FORM_CHEQUE);
		    if ( GTK_WIDGET_VISIBLE (widget_tmp)
			 &&
			 !gsb_form_widget_check_empty (widget_tmp))
		    {
			gsb_data_mix_set_method_of_payment_content ( transaction_number, gtk_entry_get_text (GTK_ENTRY (widget_tmp)), is_transaction);

			/* get the last number to increase next time */
			if ( is_transaction
			     &&
			     gsb_data_payment_get_automatic_numbering (payment_number))
			    gsb_data_payment_set_last_number ( payment_number,
							       utils_str_atoi (gtk_entry_get_text (GTK_ENTRY (widget_tmp))));
		    }
		    else
			gsb_data_mix_set_method_of_payment_content ( transaction_number, NULL, is_transaction);
		}
		else
		{
		    gsb_data_mix_set_method_of_payment_number ( transaction_number, 0, is_transaction );
		    gsb_data_mix_set_method_of_payment_content ( transaction_number, NULL, is_transaction);
		}
		break;

	    case TRANSACTION_FORM_DEVISE:
		gsb_data_mix_set_currency_number ( transaction_number, gsb_currency_get_currency_from_combobox (element -> element_widget), is_transaction);
		if (is_transaction)
		    gsb_currency_check_for_change ( transaction_number );

		break;

	    case TRANSACTION_FORM_BANK:
		if (gsb_form_widget_check_empty (element -> element_widget)) 
		    gsb_data_mix_set_bank_references ( transaction_number, NULL, is_transaction);
		else
		{
		    gchar* tmpstr = my_strdup ( gtk_entry_get_text (GTK_ENTRY(element -> element_widget)));
		    gsb_data_mix_set_bank_references ( transaction_number, tmpstr, is_transaction);
		    g_free ( tmpstr );
		}
		break;

	    case TRANSACTION_FORM_VOUCHER:
		if (gsb_form_widget_check_empty (element -> element_widget)) 
		    gsb_data_mix_set_voucher ( transaction_number, NULL, is_transaction);
		else
		{
		    gchar* tmpstr = my_strdup ( gtk_entry_get_text ( GTK_ENTRY ( element -> element_widget )));
		    gsb_data_mix_set_voucher ( transaction_number, tmpstr, is_transaction);
		    g_free ( tmpstr );
		}
		break;
	}
	tmp_list = tmp_list -> next;
    }
}



/** 
 * get the category in the form, for transaction and scheduled transaction
 * do everything needed by that category (create contra-transaction...)
 * 
 * \param transaction_number the transaction which work with
 * \param new_transaction 1 if it's a new_transaction
 * \param is_transaction TRUE if it's for a transaction, FALSE for a scheduled transaction
 * 
 * \return FALSE
 * */
gboolean gsb_form_get_categories ( gint transaction_number,
				   gint new_transaction,
				   gboolean is_transaction )
{
    GtkWidget *category_combofix;

    devel_debug_int (transaction_number);

    if ( !gsb_data_form_check_for_value ( TRANSACTION_FORM_CATEGORY ))
	return FALSE;

    category_combofix = gsb_form_widget_get_widget (TRANSACTION_FORM_CATEGORY);

    if (!gsb_form_widget_check_empty (category_combofix))
    {
	const gchar *string;
	gint contra_transaction_number;

	string = gtk_combofix_get_text ( GTK_COMBOFIX (category_combofix));

	if ( strcmp ( string,
		      _("Split of transaction") ))
	{
	    /* it's not a split of transaction, if it was one, we delete the
	     * transaction's daughters */
	    gint account_transfer;

	    /* carreful : must set == 1 and not != 0 because if problem to get the result,
	     * that function returns -1 */
	    if ( !new_transaction
		 &&
		 gsb_data_mix_get_split_of_transaction (transaction_number, is_transaction) == 1)
	    {
		/* we try to modify a split to a non split transaction */
		GSList *children_list;

		/* get the number list of children */
		children_list = gsb_data_mix_get_children (transaction_number, TRUE, is_transaction);

		/* if there is some children, we ask to be sure and delete them */
		if (children_list)
		{
		    GSList *save_children_list;

		    if (!question_yes_no_hint ( _("Modifying a transaction"),
						_("You are trying to change a split of transaction to another kind of transaction.\nThere is some children to that transaction, if you continue, the children will be deleted.\nAre you sure ?"),
						GTK_RESPONSE_OK ))
			return FALSE;

		    save_children_list = children_list;

		    do
		    {
			gint transaction_number_tmp;
			transaction_number_tmp = GPOINTER_TO_INT (children_list -> data);

			if (is_transaction)
			    gsb_transactions_list_delete_transaction (transaction_number_tmp, FALSE);
			else
			    gsb_scheduler_list_delete_scheduled_transaction (transaction_number_tmp, FALSE);

			children_list = children_list -> next;
		    }
		    while ( children_list );
		    g_slist_free (save_children_list);
		}
		gsb_data_mix_set_split_of_transaction ( transaction_number,
							    0, is_transaction );
	    }

	    /* now, check if it's a transfer or a normal category */
	    account_transfer = gsb_form_check_for_transfer (string);

	    switch (account_transfer)
	    {
		/* if the check returns -3, it's not a transfer, so it's a normal category */
		case -3:
		    /* if it's a modification, check if before it was not a transfer and delete
		     * the contra-transaction if necessary
		     * that only for transaction, not scheduled */
		    if ( is_transaction
			 &&
			 !new_transaction
			 &&
			 (contra_transaction_number = gsb_data_transaction_get_contra_transaction_number (transaction_number)))
		    {
			/* it was a transfer, we delete the contra-transaction */
			gsb_data_transaction_set_contra_transaction_number ( contra_transaction_number,
									       0);
			gsb_transactions_list_delete_transaction (contra_transaction_number, FALSE );
			gsb_data_transaction_set_contra_transaction_number ( transaction_number,
									       0);
			gsb_data_transaction_set_contra_transaction_account ( transaction_number,
									   0);
		    }
		    gsb_data_category_fill_transaction_by_string ( transaction_number,
								   string, is_transaction );
		    break;

		case -2:
		    /* if the check returns -2, it's a deleted account,
		     * normally cannot come here if scheduled transaction, but in case,
		     * we set data_mix to protect */
		    gsb_data_mix_set_account_number_transfer ( transaction_number,
							       -1, is_transaction );
		    gsb_data_mix_set_transaction_number_transfer ( transaction_number,
								   -1, is_transaction );
		    /* we don't set any break here, so with the case -1 the 
		     * category will be set to 0 (!! let the case -1 after here) */

		case -1:
		    /* if the check returns -1, it's a transfert to non existant account,
		     * so do nothing */
		    gsb_data_mix_set_category_number ( transaction_number,
						       0, is_transaction );
		    gsb_data_mix_set_sub_category_number ( transaction_number,
							   0, is_transaction );
		    break;

		    /* all other values are a number of account */
		default :
		    /* need to check a lot of things and create the contra-transaction for a transaction,
		     * but nothing to do for a scheduled transaction because no contra-transaction */
		    if (is_transaction)
			gsb_form_transaction_validate_transfer ( transaction_number,
								 new_transaction,
								 account_transfer );
		    else
			gsb_data_scheduled_set_account_number_transfer (transaction_number, account_transfer);
	    }
	}
	else
	{
	    /* it's a split of transaction */
	    /* if it was a transfer, we delete the contra-transaction */
	    if ( is_transaction
		 &&
		 !new_transaction
		 &&
		 ( contra_transaction_number = gsb_data_transaction_get_contra_transaction_number (transaction_number)))
	    {
		gsb_data_transaction_set_contra_transaction_number ( contra_transaction_number,
								       0);
		gsb_transactions_list_delete_transaction (contra_transaction_number, FALSE );

		gsb_data_transaction_set_contra_transaction_number ( transaction_number,
								       0);
		gsb_data_transaction_set_contra_transaction_account ( transaction_number,
								   0);
	    }

	    /* if it's a modification of a transaction and it was not a split,
	     * but it is now, we add a white line as first child */
	    if (!new_transaction
		&&
		!gsb_data_mix_get_split_of_transaction (transaction_number, is_transaction))
	    {
		if (!is_transaction)
		    gsb_scheduler_list_append_new_scheduled ( gsb_data_scheduled_new_white_line (transaction_number),
							      gsb_data_scheduled_get_limit_date (transaction_number ));
	    }

	    gsb_data_mix_set_split_of_transaction ( transaction_number,
							1, is_transaction );
	    gsb_data_mix_set_category_number ( transaction_number,
					       0, is_transaction );
	    gsb_data_mix_set_sub_category_number ( transaction_number,
						   0, is_transaction );

	    /* normally the following widgets are hidden for a split, but if the user really
	     * want he can annoy us and set an budget for example, and after cry because problems
	     * in reports... so, erase here budget and financial year, if ever they are defined */
	    gsb_data_mix_set_budgetary_number (transaction_number, 0, is_transaction);
	    gsb_data_mix_set_sub_budgetary_number (transaction_number, 0, is_transaction);
	    gsb_data_mix_set_financial_year_number (transaction_number, 0, is_transaction);
	    gsb_data_mix_set_voucher (transaction_number, NULL, is_transaction);
	}
    }
    return FALSE;
}




/**
 * called when cancel a form
 *
 * \param 
 *
 * \return FALSE
 * */
gboolean gsb_form_escape_form ( void )
{
    devel_debug (NULL);

    switch ( gsb_form_get_origin ())
    {
	case ORIGIN_VALUE_OTHER:
	    notice_debug ("Should not come here... (gsb_form_escape_form)");
	    gtk_widget_grab_focus (navigation_tree_view);
	    break;

	case ORIGIN_VALUE_HOME:
	    gtk_widget_grab_focus (window);
	    break;

	case ORIGIN_VALUE_SCHEDULED:
	    gtk_widget_grab_focus (gsb_scheduler_list_get_tree_view ());
	    break;

	default:
	    gtk_widget_grab_focus (gsb_transactions_list_get_tree_view());
    }

    /* in all case clean the scheduler part of the form */
    gsb_form_scheduler_clean ();

    if ( etat.formulaire_toujours_affiche )
    {
	gsb_form_clean (gsb_form_get_account_number ());
    }
    else
    {
	gsb_form_widget_free_list ();
	gtk_expander_set_expanded ( GTK_EXPANDER (form_expander),
				    FALSE );
    }
    return FALSE;
}



/**
 * set the size of the columns in the form, according to the user conf
 * and the size of the window
 *
 * \param table the table wich receive the 'size-allocate' signal
 * \param allocation
 *
 * \return FALSE
 * */
gboolean gsb_form_allocate_size ( GtkWidget *table,
				  GtkAllocation *allocation,
				  gpointer null )
{
    gint row, column;
    gint account_number;
    GtkWidget *widget;

    if (! gsb_form_is_visible ( ) )
	return FALSE;

    account_number = gsb_form_get_account_number ();
    if ( account_number == -2 )
	return FALSE;

    if (saved_allocation_size == allocation -> width)
    {
	saved_allocation_size = 0;
	return FALSE;
    }
    saved_allocation_size = allocation -> width;

    /* set the size for all columns - 1 to avoid recursive call to that function,
     * so the last column size is set to be beautiful, but in fact, it's just the extra-space */
    for ( row=0 ; row < gsb_data_form_get_nb_rows (account_number) - 1 ; row++ )
	for ( column=0 ; column < gsb_data_form_get_nb_columns (account_number) ; column++ )
	{
	    widget = gsb_form_widget_get_widget ( gsb_data_form_get_value ( account_number,
									    column,
									    row ));
	    if ( widget )
		gtk_widget_set_usize ( widget,
				       gsb_data_form_get_width_column (account_number,
								       column ) * allocation -> width / 100,
				       FALSE );
	}

    for ( column = 0 ; column < 6 ; column++ )
    {
	gint width_percent = 0;

	widget = gsb_form_scheduler_get_element_widget(column);

	if (widget)
	{
	    switch (column)
	    {
		case SCHEDULED_FORM_ACCOUNT:
		    width_percent = 30;
		    break;

		case SCHEDULED_FORM_AUTO:
		    width_percent = 16;
		    break;

		case SCHEDULED_FORM_FREQUENCY_BUTTON:
		    width_percent = 16;
		    break;

		case SCHEDULED_FORM_LIMIT_DATE:
		    width_percent = 12;
		    break;

		case SCHEDULED_FORM_FREQUENCY_USER_ENTRY:
		    width_percent = 7;
		    break;

		case SCHEDULED_FORM_FREQUENCY_USER_BUTTON:
		    width_percent = 12;
		    break;
	    }
	    if (widget && GTK_IS_WIDGET (widget))
		gtk_widget_set_usize ( widget,
				       width_percent * allocation -> width / 100,
				       FALSE );
	}
    }
    return FALSE;
}

