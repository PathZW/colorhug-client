/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009-2011 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <colord.h>
#include <math.h>
#include <gusb.h>
#include <libsoup/soup.h>
#include <stdlib.h>
#include <lcms2.h>

#include "ch-common.h"
#include "ch-math.h"

typedef struct {
	GtkApplication	*application;
	GtkBuilder	*builder;
	gboolean	 done_get_cal;
	GUsbContext	*usb_ctx;
	GUsbDevice	*device;
	GUsbDeviceList	*device_list;
	SoupSession	*session;
	guint16		 calibration_map[6];
	gboolean	 in_use[CH_CALIBRATION_MAX];
	guint		 ccmx_idx;
	guint8		 ccmx_buffer[64];
} ChCcmxPrivate;

enum {
	COLUMN_DESCRIPTION,
	COLUMN_INDEX,
	COLUMN_LAST
};

static void	 ch_ccmx_get_calibration_idx		(ChCcmxPrivate *priv);
static void	 ch_ccmx_refresh_calibration_data	(ChCcmxPrivate *priv);

/**
 * ch_ccmx_error_dialog:
 **/
static void
ch_ccmx_error_dialog (ChCcmxPrivate *priv,
		      const gchar *title,
		      const gchar *message)
{
	GtkWindow *window;
	GtkWidget *dialog;

	window = GTK_WINDOW(gtk_builder_get_object (priv->builder, "dialog_ccmx"));
	dialog = gtk_message_dialog_new (window,
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 "%s", title);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  "%s", message);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

/**
 * ch_ccmx_activate_cb:
 **/
static void
ch_ccmx_activate_cb (GApplication *application, ChCcmxPrivate *priv)
{
	GtkWindow *window;
	window = GTK_WINDOW (gtk_builder_get_object (priv->builder, "dialog_ccmx"));
	gtk_window_present (window);
}

/**
 * ch_ccmx_close_button_cb:
 **/
static void
ch_ccmx_close_button_cb (GtkWidget *widget, ChCcmxPrivate *priv)
{
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "dialog_ccmx"));
	gtk_widget_destroy (widget);
}

/**
 * ch_ccmx_set_combo_from_index:
 **/
static void
ch_ccmx_set_combo_from_index (GtkComboBox *combo, guint idx)
{
	gboolean ret;
	GtkTreeIter iter;
	GtkTreeModel *model;
	guint idx_tmp;

	model = gtk_combo_box_get_model (combo);
	ret = gtk_tree_model_get_iter_first (model, &iter);
	while (ret) {
		gtk_tree_model_get (model, &iter,
				    COLUMN_INDEX, &idx_tmp,
				    -1);
		if (idx == idx_tmp) {
			gtk_combo_box_set_active_iter (combo,
						       &iter);
			break;
		}
		ret = gtk_tree_model_iter_next (model, &iter);
	}
}

/**
 * ch_ccmx_get_calibration_map_cb:
 **/
static void
ch_ccmx_get_calibration_map_cb (GObject *source,
				GAsyncResult *res,
				gpointer user_data)
{
	ChCcmxPrivate *priv = (ChCcmxPrivate *) user_data;
	gboolean ret;
	GError *error = NULL;
	GtkWidget *widget;
	GUsbDevice *device = G_USB_DEVICE (source);

	/* get data */
	ret = ch_device_write_command_finish (device, res, &error);
	if (!ret) {
		ch_ccmx_error_dialog (priv,
				       _("Failed to get the calibration map"),
				       error->message);
		g_error_free (error);
		goto out;
	}

	/* setup UI */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_progress"));
	gtk_widget_hide (widget);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_import"));
	gtk_widget_show (widget);

	/* select the right checkboxes */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "combobox_lcd"));
	ch_ccmx_set_combo_from_index (GTK_COMBO_BOX (widget), priv->calibration_map[0]);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "combobox_crt"));
	ch_ccmx_set_combo_from_index (GTK_COMBO_BOX (widget), priv->calibration_map[1]);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "combobox_projector"));
	ch_ccmx_set_combo_from_index (GTK_COMBO_BOX (widget), priv->calibration_map[2]);

	/* we've setup */
	priv->done_get_cal = TRUE;
out:
	return;
}

/**
 * ch_ccmx_get_calibration_cb:
 **/
static void
ch_ccmx_get_calibration_cb (GObject *source,
			    GAsyncResult *res,
			    gpointer user_data)
{
	ChCcmxPrivate *priv = (ChCcmxPrivate *) user_data;
	gboolean ret;
	GError *error = NULL;
	GtkListStore *list_store;
	GtkTreeIter iter;
	guint8 types;
	const gchar *description;
	GUsbDevice *device = G_USB_DEVICE (source);

	/* get data */
	ret = ch_device_write_command_finish (device, res, &error);
	if (!ret) {
		g_debug ("ignoring: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* get the types this is suitable for */
	types = priv->ccmx_buffer[4*9];
	description = (const gchar *) priv->ccmx_buffer + (4*9) + 1;

	/* is suitable for LCD */
	list_store = GTK_LIST_STORE (gtk_builder_get_object (priv->builder, "liststore_lcd"));
	if ((types & CH_CALIBRATION_TYPE_LCD) > 0) {
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    COLUMN_DESCRIPTION, description,
				    COLUMN_INDEX, priv->ccmx_idx,
				    -1);
	}

	/* is suitable for CRT */
	list_store = GTK_LIST_STORE (gtk_builder_get_object (priv->builder, "liststore_crt"));
	if ((types & CH_CALIBRATION_TYPE_CRT) > 0) {
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    COLUMN_DESCRIPTION, description,
				    COLUMN_INDEX, priv->ccmx_idx,
				    -1);
	}

	/* is suitable for projector */
	list_store = GTK_LIST_STORE (gtk_builder_get_object (priv->builder, "liststore_projector"));
	if ((types & CH_CALIBRATION_TYPE_PROJECTOR) > 0) {
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
				    COLUMN_DESCRIPTION, description,
				    COLUMN_INDEX, priv->ccmx_idx,
				    -1);
	}
out:
	/* track if it's in use so we can find a spare slot on import */
	priv->in_use[priv->ccmx_idx] = ret;

	/* read the next chunk */
	priv->ccmx_idx++;
	ch_ccmx_get_calibration_idx (priv);
	return;
}

/**
 * ch_ccmx_get_calibration_idx:
 **/
static void
ch_ccmx_get_calibration_idx (ChCcmxPrivate *priv)
{
	/* hit hardware */
	if (priv->ccmx_idx < CH_CALIBRATION_MAX) {
		ch_device_write_command_async (priv->device,
					       CH_CMD_GET_CALIBRATION,
					       (const guint8 *) &priv->ccmx_idx,
					       sizeof(priv->ccmx_idx),
					       priv->ccmx_buffer,
					       60,
					       NULL, /* cancellable */
					       ch_ccmx_get_calibration_cb,
					       priv);
		goto out;
	}

	/* get calibration map */
	ch_device_write_command_async (priv->device,
				       CH_CMD_GET_CALIBRATION_MAP,
				       NULL, /* buffer_in */
				       0, /* buffer_in_len */
				       (guint8 *) priv->calibration_map, /* buffer_out */
				       sizeof(priv->calibration_map), /* buffer_out_len */
				       NULL, /* cancellable */
				       ch_ccmx_get_calibration_map_cb,
				       priv);
out:
	return;
}

/**
 * ch_ccmx_get_profile_filename:
 **/
static gchar *
ch_ccmx_get_profile_filename (GtkWindow *window)
{
	gchar *filename = NULL;
	GtkWidget *dialog;
	GtkFileFilter *filter;

	/* TRANSLATORS: dialog for chosing the correction matrix */
	dialog = gtk_file_chooser_dialog_new (_("Select correction matrix"), window,
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_file_chooser_set_create_folders (GTK_FILE_CHOOSER(dialog), FALSE);

	/* setup the filter */
	filter = gtk_file_filter_new ();
	/* TRANSLATORS: filter name on the file->open dialog */
	gtk_file_filter_set_name (filter, _("Supported CCMX files"));
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(dialog), filter);

	/* setup the all files filter */
	filter = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filter, "*.ccmx");
	/* TRANSLATORS: filter name on the file->open dialog */
	gtk_file_filter_set_name (filter, _("Correction matrices"));
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(dialog), filter);

	/* did user choose file */
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(dialog));

	/* we're done */
	gtk_widget_destroy (dialog);

	/* or NULL for missing */
	return filename;
}

/**
 * ch_ccmx_set_calibration_cb:
 **/
static void
ch_ccmx_set_calibration_cb (GObject *source,
			    GAsyncResult *res,
			    gpointer user_data)
{
	ChCcmxPrivate *priv = (ChCcmxPrivate *) user_data;
	gboolean ret;
	GError *error = NULL;
	GUsbDevice *device = G_USB_DEVICE (source);

	/* get data */
	ret = ch_device_write_command_finish (device, res, &error);
	if (!ret) {
		ch_ccmx_error_dialog (priv,
				       _("Failed to set the CCMX calibration"),
				       error->message);
		g_error_free (error);
		goto out;
	}

	/* update the combos */
	ch_ccmx_refresh_calibration_data (priv);
out:
	return;
}

/**
 * ch_ccmx_set_calibration_file:
 **/
static gboolean
ch_ccmx_set_calibration_file (ChCcmxPrivate *priv,
			      guint16 cal_idx,
			      const gchar *filename,
			      GError **error)
{
	cmsHANDLE ccmx = NULL;
	const gchar *description;
	const gchar *sheet_type;
	gboolean ret;
	gchar *ccmx_data = NULL;
	gsize ccmx_size;
	guint8 buffer[60];
	guint i, j;

	/* load file */
	ret = g_file_get_contents (filename,
				   &ccmx_data,
				   &ccmx_size,
				   error);
	if (!ret)
		goto out;
	ccmx = cmsIT8LoadFromMem (NULL, ccmx_data, ccmx_size);
	if (ccmx == NULL) {
		ret = FALSE;
		g_set_error (error, 1, 0, "Cannot open %s", filename);
		goto out;
	}

	/* select correct sheet */
	sheet_type = cmsIT8GetSheetType (ccmx);
	if (g_strcmp0 (sheet_type, "CCMX   ") != 0) {
		ret = FALSE;
		g_set_error (error, 1, 0, "%s is not a CCMX file [%s]",
			     filename, sheet_type);
		goto out;
	}

	/* get the description from the ccmx file */
	description = CMSEXPORT cmsIT8GetProperty(ccmx, "DISPLAY");
	if (description == NULL) {
		ret = FALSE;
		g_set_error_literal (error, 1, 0,
				     "CCMX file does not have DISPLAY");
		goto out;
	}

	/* write the index */
	memcpy (buffer + 0, &cal_idx, 2);

	/* write the calibration values */
	for (j = 0; j < 3; j++) {
		for (i = 0; i < 3; i++) {
			cal_idx = sizeof (guint16) + (((j * 3) + i) * sizeof (ChPackedFloat));
			ch_double_to_packed_float (cmsIT8GetDataRowColDbl(ccmx, j, i),
						   (ChPackedFloat *) buffer + cal_idx);
		}
	}

	/* write the description */
	cal_idx = sizeof (guint16) + 9 * sizeof (ChPackedFloat);
	g_utf8_strncpy ((gchar *) buffer + cal_idx,
			description,
			24);

	/* set to HW */
	ch_device_write_command_async (priv->device,
				       CH_CMD_SET_CALIBRATION,
				       buffer, /* buffer_in */
				       sizeof(buffer), /* buffer_in_len */
				       NULL, /* buffer_out */
				       0, /* buffer_out_len */
				       NULL, /* cancellable */
				       ch_ccmx_set_calibration_cb,
				       priv);
	if (!ret)
		goto out;
out:
	g_free (ccmx_data);
	if (ccmx != NULL)
		cmsIT8Free (ccmx);
	return ret;
}

/**
 * ch_ccmx_import_button_cb:
 **/
static void
ch_ccmx_import_button_cb (GtkWidget *widget, ChCcmxPrivate *priv)
{
	gboolean ret;
	gchar *filename;
	GError *error = NULL;
	GtkWindow *window;
	guint i;

	window = GTK_WINDOW (gtk_builder_get_object (priv->builder, "dialog_ccmx"));
	filename = ch_ccmx_get_profile_filename (window);
	if (filename == NULL)
		goto out;

	/* import the file into a spare slot */
	for (i = 0; i < CH_CALIBRATION_MAX; i++) {
		if (!priv->in_use[i])
			break;
	}
	if (i == CH_CALIBRATION_MAX) {
		ch_ccmx_error_dialog (priv,
				      _("No space left on device"),
				      _("All 64 slots are used up!"));
		goto out;
	}

	/* load this ccmx file as the new calibration */
	ret = ch_ccmx_set_calibration_file (priv, i, filename, &error);
	if (!ret) {
		ch_ccmx_error_dialog (priv,
				       _("Failed to load file"),
				       error->message);
		g_error_free (error);
		return;
	}
out:
	g_free (filename);
}

/**
 * ch_ccmx_refresh_calibration_data:
 **/
static void
ch_ccmx_refresh_calibration_data (ChCcmxPrivate *priv)
{
	GtkListStore *list_store;

	/* clear existing */
	list_store = GTK_LIST_STORE (gtk_builder_get_object (priv->builder, "liststore_lcd"));
	gtk_list_store_clear (list_store);
	list_store = GTK_LIST_STORE (gtk_builder_get_object (priv->builder, "liststore_crt"));
	gtk_list_store_clear (list_store);
	list_store = GTK_LIST_STORE (gtk_builder_get_object (priv->builder, "liststore_projector"));
	gtk_list_store_clear (list_store);

	/* get latest from device */
	priv->ccmx_idx = 0;
	ch_ccmx_get_calibration_idx (priv);
}

/**
 * ch_ccmx_got_device:
 **/
static void
ch_ccmx_got_device (ChCcmxPrivate *priv)
{
	gboolean ret;
	GError *error = NULL;
	GtkWidget *widget;

	/* open device */
	ret = g_usb_device_open (priv->device, &error);
	if (!ret) {
		ch_ccmx_error_dialog (priv,
				       _("Failed to open device"),
				       error->message);
		g_error_free (error);
		return;
	}
	ret = g_usb_device_set_configuration (priv->device,
					      CH_USB_CONFIG,
					      &error);
	if (!ret) {
		ch_ccmx_error_dialog (priv,
				       _("Failed to set configuration"),
				       error->message);
		g_error_free (error);
		return;
	}
	ret = g_usb_device_claim_interface (priv->device,
					    CH_USB_INTERFACE,
					    G_USB_DEVICE_CLAIM_INTERFACE_BIND_KERNEL_DRIVER,
					    &error);
	if (!ret) {
		ch_ccmx_error_dialog (priv,
				       _("Failed to claim interface"),
				       error->message);
		g_error_free (error);
		return;
	}

	/* update the UI */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_connect"));
	gtk_widget_hide (widget);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_header"));
	gtk_widget_show (widget);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "grid_data"));
	gtk_widget_show (widget);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_progress"));
	gtk_widget_show (widget);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_msg"));
	gtk_label_set_label (GTK_LABEL (widget), _("Getting calibration from device..."));

	/* start getting the calibration matrices */
	ch_ccmx_refresh_calibration_data (priv);
}

/**
 * ch_ccmx_set_combo_simple_text:
 **/
static void
ch_ccmx_set_combo_simple_text (GtkWidget *combo_box)
{
	GtkCellRenderer *renderer;
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      "wrap-mode", PANGO_WRAP_WORD_CHAR,
		      "width-chars", 60,
		      NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
					"text", COLUMN_DESCRIPTION,
					NULL);
}

/**
 * ch_ccmx_set_calibration_map_cb:
 **/
static void
ch_ccmx_set_calibration_map_cb (GObject *source,
				GAsyncResult *res,
				gpointer user_data)
{
	ChCcmxPrivate *priv = (ChCcmxPrivate *) user_data;
	gboolean ret;
	GError *error = NULL;
	GtkWidget *widget;
	GUsbDevice *device = G_USB_DEVICE (source);

	/* get data */
	ret = ch_device_write_command_finish (device, res, &error);
	if (!ret) {
		ch_ccmx_error_dialog (priv,
				       _("Failed to set the calibration map"),
				       error->message);
		g_error_free (error);
		goto out;
	}

	/* update UI */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_close"));
	gtk_widget_set_sensitive (widget, TRUE);
out:
	return;
}

/**
 * ch_ccmx_combo_changed_cb:
 **/
static void
ch_ccmx_combo_changed_cb (GtkComboBox *combo, ChCcmxPrivate *priv)
{
	gboolean ret;
	GtkTreeIter iter;
	GtkTreeModel *model;
	guint idx_tmp;
	guint cal_index;
	GtkWidget *widget;

	/* not yet setup UI */
	if (!priv->done_get_cal)
		return;

	/* change this on the device */
	ret = gtk_combo_box_get_active_iter (combo, &iter);
	if (!ret)
		return;
	model = gtk_combo_box_get_model (combo);
	gtk_tree_model_get (model, &iter,
			    COLUMN_INDEX, &idx_tmp,
			    -1);

	/* update the map */
	cal_index = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo),
							"colorhug-ccmx-idx"));
	priv->calibration_map[cal_index] = idx_tmp;

	/* update UI */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_close"));
	gtk_widget_set_sensitive (widget, FALSE);

	/* hit hardware */
	ch_device_write_command_async (priv->device,
				       CH_CMD_SET_CALIBRATION_MAP,
				       (const guint8 *) priv->calibration_map,
				       sizeof(priv->calibration_map),
				       NULL, /* buffer_out */
				       0, /* buffer_out_len */
				       NULL, /* cancellable */
				       ch_ccmx_set_calibration_map_cb,
				       priv);
}

/**
 * ch_ccmx_startup_cb:
 **/
static void
ch_ccmx_startup_cb (GApplication *application, ChCcmxPrivate *priv)
{
	GError *error = NULL;
	gint retval;
	GtkWidget *main_window;
	GtkWidget *widget;
	GdkPixbuf *pixbuf;

	/* get UI */
	priv->builder = gtk_builder_new ();
	retval = gtk_builder_add_from_file (priv->builder,
					    CH_DATA "/ch-ccmx.ui",
					    &error);
	if (retval == 0) {
		g_warning ("failed to load ui: %s",
			   error->message);
		g_error_free (error);
		goto out;
	}

	/* add application specific icons to search path */
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
					   CH_DATA G_DIR_SEPARATOR_S "icons");

	main_window = GTK_WIDGET (gtk_builder_get_object (priv->builder, "dialog_ccmx"));
	gtk_application_add_window (priv->application, GTK_WINDOW (main_window));
	gtk_widget_set_size_request (main_window, 400, 100);

	/* Hide window first so that the dialogue resizes itself without redrawing */
	gtk_widget_hide (main_window);

	/* buttons */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_close"));
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (ch_ccmx_close_button_cb), priv);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_import"));
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (ch_ccmx_import_button_cb), priv);

	/* setup logo image */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "image_logo"));
	gtk_image_set_from_icon_name (GTK_IMAGE (widget),
				      "colorhug-gray",
				      GTK_ICON_SIZE_DIALOG);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "image_usb"));

	/* setup comboboxes */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "combobox_lcd"));
	g_object_set_data (G_OBJECT (widget),
			   "colorhug-ccmx-idx",
			   GINT_TO_POINTER (0));
	g_signal_connect (widget, "changed",
			  G_CALLBACK (ch_ccmx_combo_changed_cb), priv);
	ch_ccmx_set_combo_simple_text (widget);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "combobox_crt"));
	g_object_set_data (G_OBJECT (widget),
			   "colorhug-ccmx-idx",
			   GINT_TO_POINTER (1));
	g_signal_connect (widget, "changed",
			  G_CALLBACK (ch_ccmx_combo_changed_cb), priv);
	ch_ccmx_set_combo_simple_text (widget);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "combobox_projector"));
	g_object_set_data (G_OBJECT (widget),
			   "colorhug-ccmx-idx",
			   GINT_TO_POINTER (2));
	g_signal_connect (widget, "changed",
			  G_CALLBACK (ch_ccmx_combo_changed_cb), priv);
	ch_ccmx_set_combo_simple_text (widget);

	/* setup USB image */
	pixbuf = gdk_pixbuf_new_from_file_at_scale (CH_DATA
						    G_DIR_SEPARATOR_S "icons"
						    G_DIR_SEPARATOR_S "usb.svg",
						    -1, 48, TRUE, &error);
	g_assert (pixbuf != NULL);
	gtk_image_set_from_pixbuf (GTK_IMAGE (widget), pixbuf);
	g_object_unref (pixbuf);

	/* hide all unused widgets until we've connected with the device */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "grid_data"));
	gtk_widget_hide (widget);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_progress"));
	gtk_widget_hide (widget);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_header"));
	gtk_widget_hide (widget);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_import"));
	gtk_widget_hide (widget);

	/* is the colorhug already plugged in? */
	g_usb_device_list_coldplug (priv->device_list);

	/* setup the session */
	priv->session = soup_session_sync_new_with_options (SOUP_SESSION_USER_AGENT, "colorhug-ccmx",
							    SOUP_SESSION_TIMEOUT, 5000,
							    NULL);
	if (priv->session == NULL) {
		ch_ccmx_error_dialog (priv,
				      _("Failed to setup networking"),
				      NULL);
		goto out;
	}

	/* show main UI */
	gtk_widget_show (main_window);
out:
	return;
}

/**
 * ch_ccmx_device_added_cb:
 **/
static void
ch_ccmx_device_added_cb (GUsbDeviceList *list,
			 GUsbDevice *device,
			 ChCcmxPrivate *priv)
{
	g_debug ("Added: %i:%i",
		 g_usb_device_get_vid (device),
		 g_usb_device_get_pid (device));
	if (g_usb_device_get_vid (device) == CH_USB_VID &&
	    g_usb_device_get_pid (device) == CH_USB_PID) {
		priv->device = g_object_ref (device);
		ch_ccmx_got_device (priv);
	}
}

/**
 * ch_ccmx_device_removed_cb:
 **/
static void
ch_ccmx_device_removed_cb (GUsbDeviceList *list,
			    GUsbDevice *device,
			    ChCcmxPrivate *priv)
{
	g_debug ("Removed: %i:%i",
		 g_usb_device_get_vid (device),
		 g_usb_device_get_pid (device));
	if (g_usb_device_get_vid (device) == CH_USB_VID &&
	    g_usb_device_get_pid (device) == CH_USB_PID) {
		if (priv->device != NULL)
			g_object_unref (priv->device);
		priv->device = NULL;
	}
}

/**
 * main:
 **/
int
main (int argc, char **argv)
{
	ChCcmxPrivate *priv;
	GOptionContext *context;
	int status = 0;
	gboolean ret;
	GError *error = NULL;

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gtk_init (&argc, &argv);

	context = g_option_context_new ("ColorHug CCMX loader");
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	ret = g_option_context_parse (context, &argc, &argv, &error);
	if (!ret) {
		g_warning ("failed to parse options: %s", error->message);
		g_error_free (error);
	}
	g_option_context_free (context);

	priv = g_new0 (ChCcmxPrivate, 1);
	priv->usb_ctx = g_usb_context_new (NULL);
	priv->device_list = g_usb_device_list_new (priv->usb_ctx);
	g_signal_connect (priv->device_list, "device-added",
			  G_CALLBACK (ch_ccmx_device_added_cb), priv);
	g_signal_connect (priv->device_list, "device-removed",
			  G_CALLBACK (ch_ccmx_device_removed_cb), priv);

	/* ensure single instance */
	priv->application = gtk_application_new ("com.hughski.ColorHug.Ccmx", 0);
	g_signal_connect (priv->application, "startup",
			  G_CALLBACK (ch_ccmx_startup_cb), priv);
	g_signal_connect (priv->application, "activate",
			  G_CALLBACK (ch_ccmx_activate_cb), priv);

	/* wait */
	status = g_application_run (G_APPLICATION (priv->application), argc, argv);

	g_object_unref (priv->application);
	if (priv->device_list != NULL)
		g_object_unref (priv->device_list);
	if (priv->usb_ctx != NULL)
		g_object_unref (priv->usb_ctx);
	if (priv->builder != NULL)
		g_object_unref (priv->builder);
	if (priv->session != NULL)
		g_object_unref (priv->session);
	g_free (priv);
	return status;
}
