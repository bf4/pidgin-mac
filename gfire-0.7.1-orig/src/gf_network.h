/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Laurent De Marez <laurentdemarez@gmail.com>
 *
 * (These sources are derived from previous sources created by 
 * asraniel@fryx.ch, and have been later modified by keith@penguingurus.com)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef _GF_NETWORK_H
#define _GF_NETWORK_H


#define GFIRE_BUFFOUT_SIZE 65535
#define GFIRE_BUFFIN_SIZE  65535

void gfire_send(PurpleConnection *gc, const guint8 *packet, int size);
int gfire_initialize_connection(guint8 *packet, int version);
void gfire_input_cb(gpointer data, gint source, PurpleInputCondition condition);
void gfire_send_away(PurpleConnection *gc, const char *msg);
void gfire_process_invitation(PurpleConnection *gc, GList *invites);


#endif /* _GF_NETWORK_H */
