/*
 * System tray icon (aka docklet) plugin for Purple
 *
 * Copyright (C) 2007 Anders Hasselqvist
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "internal.h"
#include "pidgin.h"
#include "debug.h"
#include "prefs.h"
#include "pidginstock.h"
#include "gtkdocklet.h"

#include <gtk/gtkstatusicon.h>


/* globals */
GtkStatusIcon *docklet = NULL;

static void
docklet_gtk_status_clicked_cb(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
	purple_debug(PURPLE_DEBUG_INFO, "docklet", "button clicked %d\n", button);

#if defined(__APPLE__) && !defined(HAVE_X11) 
	/* You can only click left mouse button on MacOSX native GTK. Let that be the menu */ 
	pidgin_docklet_clicked(3); 
#else 
	pidgin_docklet_clicked(button); 
#endif
}

static void
docklet_gtk_status_update_icon(PurpleStatusPrimitive status, gboolean connecting, gboolean pending)
{
	const gchar *icon_name = NULL;

	switch (status) {
		case PURPLE_STATUS_OFFLINE:
			icon_name = PIDGIN_STOCK_TRAY_OFFLINE;
			break;
		case PURPLE_STATUS_AWAY:
			icon_name = PIDGIN_STOCK_TRAY_AWAY;
			break;
		case PURPLE_STATUS_UNAVAILABLE:
			icon_name = PIDGIN_STOCK_TRAY_BUSY;
			break;
		case PURPLE_STATUS_EXTENDED_AWAY:
			icon_name = PIDGIN_STOCK_TRAY_XA;
			break;
		case PURPLE_STATUS_INVISIBLE:
			icon_name = PIDGIN_STOCK_TRAY_INVISIBLE;
			break;
		default:
			icon_name = PIDGIN_STOCK_TRAY_AVAILABLE;
			break;
	}

	if (pending)
		icon_name = PIDGIN_STOCK_TRAY_PENDING;
	if (connecting)
		icon_name = PIDGIN_STOCK_TRAY_CONNECT;

	if(icon_name) {
		GtkWidget *win;
		GdkPixbuf *pixbuf;

		/* We do these steps because gtk_status_icon_set_from_stock()
		   only accepts icons of exactly size GTK_ICON_SIZE_SMALL_TOOLBAR.
		   Doing it this way we force GtkStatusIcon to scale the pixbuf
		   itself */
		
		/* Er, yeah, a hack, but it works :).
		   We need to have a widget for getting a style */
		win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_widget_realize(win);

		pixbuf = gtk_widget_render_icon(win, icon_name,
										GTK_ICON_SIZE_MENU, NULL);

		if (pixbuf == NULL)
		{
			gtk_widget_destroy(win);
			return;
		}

		gtk_status_icon_set_from_pixbuf(docklet, pixbuf);
		g_object_unref(pixbuf);
		gtk_widget_destroy(win);
	}
}

static gboolean
docklet_gtk_status_resize_icon(GtkStatusIcon *status_icon, gint size, gpointer user_data)
{
	/* Let GTK rescale for now */
	return FALSE;
}

static void
docklet_gtk_status_set_tooltip(gchar *tooltip)
{
	if (tooltip) {
		gtk_status_icon_set_tooltip(docklet, tooltip);
	} else {
		gtk_status_icon_set_tooltip(docklet, NULL);
	}
}

static void
docklet_gtk_status_position_menu(GtkMenu *menu, int *x, int *y, gboolean *push_in,
									gpointer user_data)
{
	gtk_status_icon_position_menu(menu, x, y, push_in, docklet);
}

static void
docklet_gtk_status_destroy()
{
	g_return_if_fail(docklet != NULL);

	pidgin_docklet_remove();
	
	g_object_unref(G_OBJECT(docklet));
	docklet = NULL;

	purple_debug(PURPLE_DEBUG_INFO, "docklet", "destroyed\n");
}

static void
docklet_gtk_status_create(gboolean recreate)
{
	if (docklet) {
		/* if this is being called when a tray icon exists, it's because
		   something messed up. try destroying it before we proceed,
		   although docklet_refcount may be all hosed. hopefully won't happen. */
		purple_debug(PURPLE_DEBUG_WARNING, "docklet", "trying to create icon but it already exists?\n");
		docklet_gtk_status_destroy();
	}

	docklet = gtk_status_icon_new();
	g_return_if_fail(docklet != NULL);

	g_signal_connect(G_OBJECT(docklet), "popup-menu", G_CALLBACK(docklet_gtk_status_clicked_cb), NULL);
	g_signal_connect(G_OBJECT(docklet), "size-changed", G_CALLBACK(docklet_gtk_status_resize_icon), NULL);

	pidgin_docklet_embedded();
	gtk_status_icon_set_visible(docklet, TRUE);
	purple_debug(PURPLE_DEBUG_INFO, "docklet", "created\n");
}

static void
docklet_gtk_status_create_ui_op()
{
	docklet_gtk_status_create(FALSE);
}


static void
docklet_gtk_status_set_blink(gboolean blinking)
{
	gtk_status_icon_set_blinking(docklet, blinking);
}

static struct docklet_ui_ops ui_ops =
{
	docklet_gtk_status_create_ui_op,
	docklet_gtk_status_destroy,
	docklet_gtk_status_update_icon,
	NULL,
	docklet_gtk_status_set_tooltip,
	docklet_gtk_status_position_menu,
	docklet_gtk_status_set_blink
};

void
docklet_ui_init()
{
	pidgin_docklet_set_ui_ops(&ui_ops);
}
