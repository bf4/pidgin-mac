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
#ifndef _GF_PACKET_H
#define _GF_PACKET_H


void gfire_add_header(guint8 *packet, int length, int type, int atts);
int gfire_add_att_name(guint8 *packet,int packet_length, char *att);
void gfire_packet_130(PurpleConnection *gc, int packet_len);
void gfire_packet_131(PurpleConnection *gc, int packet_len);
int gfire_send_auth(PurpleConnection *gc, int packet_len, int packet_id);
GList *gfire_read_buddy_online(PurpleConnection *gc, int packet_len);
int gfire_get_im(PurpleConnection *gc, int packet_len);
int gfire_create_im(PurpleConnection *gc, gfire_buddy *buddy, const char *msg);
GList *gfire_game_status(PurpleConnection *gc, int packet_len);
int gfire_ka_packet_create(PurpleConnection *gc);
GList *gfire_read_buddy_status(PurpleConnection *gc, int packet_len);
GList *gfire_read_invitation(PurpleConnection *gc, int packet_len);
int gfire_invitation_deny(PurpleConnection *gc, char *name);
int gfire_invitation_accept(PurpleConnection *gc, char *name);
int gfire_add_buddy_create(PurpleConnection *gc, char *name);
int gfire_remove_buddy_create(PurpleConnection *gc, gfire_buddy *b);
int gfire_create_change_alias(PurpleConnection *gc, char *name);
int gfire_join_game_create(PurpleConnection *gc, int game, int port, const char *ip);

#endif /* _GF_PACKET_H */
