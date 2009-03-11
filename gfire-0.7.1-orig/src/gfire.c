/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008,	    Laurent De Marez <laurentdemarez@gmail.com>
 *
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

#include "gfire.h"
#include "gf_network.h"
#include "gf_packet.h"
#include "gf_games.h"

static const char *gfire_blist_icon(PurpleAccount *a, PurpleBuddy *b);
static const char *gfire_blist_emblems(PurpleBuddy *b);		
static void gfire_blist_tooltip_text(PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info, gboolean full);
static GList *gfire_status_types(PurpleAccount *account);
static int gfire_im_send(PurpleConnection *gc, const char *who, const char *what, PurpleMessageFlags flags);
static void gfire_login(PurpleAccount *account);
static void gfire_login_cb(gpointer data, gint source, PurpleInputCondition cond);
static void gfire_add_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group);
static void gfire_add_buddies(PurpleConnection *gc, GList *buddies, GList *groups);
static void gfire_remove_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group);
static void gfire_remove_buddies(PurpleConnection *gc, GList *buddies, GList *groups);
void gfire_buddy_menu_profile_cb(PurpleBlistNode *node, gpointer *data);
GList * gfire_node_menu(PurpleBlistNode *node);

void gfire_join_game(PurpleConnection *gc, const gchar *sip, int sport, int game);
void gfire_game_watch_cb(GPid pid, int status, gpointer *data);
char *gfire_escape_html(const char *html);

static PurplePlugin *_gfire_plugin = NULL;



static const char *gfire_blist_icon(PurpleAccount *a, PurpleBuddy *b)
{
	return "gfire";
}


static const char *gfire_blist_emblems(PurpleBuddy *b)	
{
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	GList *tmp = NULL;
	PurplePresence *p = NULL;
	PurpleConnection *gc = NULL;
	char *emblems[4] = {NULL,NULL,NULL,NULL};
	int i = 0;

	if (!b || (NULL == b->account) || !(gc = purple_account_get_connection(b->account)) ||
					     !(gfire = (gfire_data *) gc->proto_data) || (NULL == gfire->buddies))
		return NULL;

	tmp = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)b->name, GFFB_NAME);
	if ((NULL == tmp) || (NULL == tmp->data)) return NULL;
	gf_buddy = (gfire_buddy *)tmp->data;

	p = purple_buddy_get_presence(b);

	if (purple_presence_is_online(p) == FALSE) {

			return "offline";	

	} else {
		if (gf_buddy->away)

			return "away";

		if (0 != gf_buddy->gameid)

			return "game";
	
	}
	return NULL;
}


static char *gfire_status_text(PurpleBuddy *buddy)
{
	char msg[100];
	GList *gfbl = NULL;
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	PurplePresence *p = NULL;
	PurpleConnection *gc = NULL;
	char *game_name = NULL;

	if (!buddy || (NULL == buddy->account) || !(gc = purple_account_get_connection(buddy->account)) ||
		!(gfire = (gfire_data *) gc->proto_data) || (NULL == gfire->buddies))
		return NULL;

	gfbl = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)buddy->name, GFFB_NAME);
	if (NULL == gfbl) return NULL;

	gf_buddy = (gfire_buddy *)gfbl->data;
	p = purple_buddy_get_presence(buddy);

	if (TRUE == purple_presence_is_online(p)) {
		if (0 != gf_buddy->gameid) {
			game_name = gfire_game_name(gc, gf_buddy->gameid);
			g_sprintf(msg, "Playing %s", game_name);
			g_free(game_name);
			return g_strdup(msg);
		}
		if (gf_buddy->away) {
			g_sprintf(msg,"%s", gf_buddy->away_msg);
			return gfire_escape_html(msg);
		}
	}
	return NULL;
}


static void gfire_blist_tooltip_text(PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info, gboolean full)	
{
	PurpleConnection *gc = NULL;
	gfire_data *gfire = NULL;
	GList *gfbl = NULL;
	gfire_buddy *gf_buddy = NULL;
	PurplePresence *p = NULL;
	gchar *game_name = NULL;
	guint32 *magic = NULL;
	gchar ipstr[16] = "";

	if (!buddy || (NULL == buddy->account) || !(gc = purple_account_get_connection(buddy->account)) ||
		!(gfire = (gfire_data *) gc->proto_data) || (NULL == gfire->buddies))
		return;

	gfbl = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)buddy->name, GFFB_NAME);
	if (NULL == gfbl) return;

	gf_buddy = (gfire_buddy *)gfbl->data;
	p = purple_buddy_get_presence(buddy);

	if (TRUE == purple_presence_is_online(p)) {

			purple_notify_user_info_add_pair(user_info,"Username",gf_buddy->name);

		if (0 != gf_buddy->gameid) {
			game_name = gfire_game_name(gc, gf_buddy->gameid);

				purple_notify_user_info_add_pair(user_info,"Game",game_name);

			g_free(game_name);
			magic = (guint32 *)gf_buddy->gameip;
			if ((NULL != gf_buddy->gameip) && (0 != *magic)) {
				
				g_sprintf(ipstr, "%d.%d.%d.%d", gf_buddy->gameip[3], gf_buddy->gameip[2],
						gf_buddy->gameip[1], gf_buddy->gameip[0]);

					gchar * value = g_strdup_printf("%s:%d", ipstr, gf_buddy->gameport);
					purple_notify_user_info_add_pair(user_info,"Server",value);
					g_free(value);

			}
		}
		if (gf_buddy->away) {
			char * escaped_away = gfire_escape_html(gf_buddy->away_msg);

				purple_notify_user_info_add_pair(user_info,"Away",escaped_away);

			g_free(escaped_away);
		}
	}
}



static GList *gfire_status_types(PurpleAccount *account)
{
	PurpleStatusType *type;
	GList *types = NULL;

	type = purple_status_type_new(PURPLE_STATUS_AVAILABLE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	type = purple_status_type_new_with_attrs(
		PURPLE_STATUS_AWAY, NULL, NULL, FALSE, TRUE, FALSE,
		"message", "Message", purple_value_new(PURPLE_TYPE_STRING),
		NULL);
	types = g_list_append(types, type);

	type = purple_status_type_new(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	return types;
}


static void gfire_login(PurpleAccount *account)
{
	gfire_data *gfire;
	int	err = 0;

	PurpleConnection *gc = purple_account_get_connection(account);
	/* set connection flags for chats and im's tells purple what we can and can't handle */
	gc->flags = 0 |	PURPLE_CONNECTION_NO_BGCOLOR | PURPLE_CONNECTION_NO_FONTSIZE
				| PURPLE_CONNECTION_NO_URLDESC | PURPLE_CONNECTION_NO_IMAGES;

	purple_connection_update_progress(gc, "Connecting", 0, XFIRE_CONNECT_STEPS);	
	
	gc->proto_data = gfire = g_new0(gfire_data, 1);
	gfire->fd = -1;
	gfire->buff_out = gfire->buff_in = NULL;
	/* load game xml from user dir */
	gfire_parse_games_file(gc, g_build_filename(purple_user_dir(), "gfire_games.xml", NULL));
	gfire_parse_launchinfo_file(gc, g_build_filename(purple_user_dir(), "gfire_launch.xml", NULL));

	err = purple_proxy_connect(
				NULL,
				account,
				purple_account_get_string(account, "server", XFIRE_SERVER),
				purple_account_get_int(account, "port", XFIRE_PORT),
				gfire_login_cb, gc);
	err = !err;


	if (err || !purple_account_get_connection(account)) {
			purple_connection_error(gc, "Couldn't create socket");
			return;
	}
}


static void gfire_login_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	PurpleConnection *gc = data;
	guint8 packet[1024];
	int length;
	PurpleAccount *account = purple_connection_get_account(gc);
	gfire_data *gfire = (gfire_data *)gc->proto_data;
	gfire->buddies = NULL;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "connected source=%d\n",source);
	if (!g_list_find(purple_connections_get_all(), gc)) {
		close(source);
		return;
	}

	if (source < 0) {
		purple_connection_error(gc, "Unable to connect to host");
		return;
	}

	gfire->fd = source;

	/* Update the login progress status display */
	
	purple_connection_update_progress(gc, "login", 1, XFIRE_CONNECT_STEPS);	

	gfire_send(gc, (const guint8 *)"UA01", 4); /* open connection */
	
	length = gfire_initialize_connection(packet,purple_account_get_int(account, "version", XFIRE_PROTO_VERSION));
	gfire_send(gc, packet, length);
	
	gc->inpa = purple_input_add(gfire->fd, PURPLE_INPUT_READ, gfire_input_cb, gc);
}


void gfire_close(PurpleConnection *gc)
{
	gfire_data *gfire = NULL;
	GList *buddies = NULL;
	gfire_buddy *b = NULL;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONNECTION: close requested.\n");
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: closing, but either no GC or ProtoData.\n");
		return;
	}

	if (gc->inpa) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: removing input handler\n");
		purple_input_remove(gc->inpa);
	}

	if (gfire->xqf_source > 0) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: removing xqf file watch callback\n");
		g_source_remove(gfire->xqf_source);
	}

	if (gfire->fd >= 0) {	
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: closing source file descriptor\n");
		close(gfire->fd);
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: freeing buddy list\n");
	buddies = gfire->buddies;
	while (NULL != buddies) {
		b = (gfire_buddy *)buddies->data;
		if (NULL != b->away_msg) g_free(b->away_msg);
		if (NULL != b->name) g_free(b->name);
		if (NULL != b->alias) g_free(b->alias);
		if (NULL != b->userid) g_free(b->userid);
		if (NULL != b->uid_str) g_free(b->uid_str);
		if (NULL != b->sid) g_free(b->sid);
		if (NULL != b->sid_str) g_free(b->sid_str);
		if (NULL != b->gameip) g_free(b->gameip);
		g_free(b);
		buddies->data = NULL;
 		buddies = g_list_next(buddies);
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: freeing gfire struct\n");
	if (NULL != gfire->im) {
		if (NULL != gfire->im->sid_str) g_free(gfire->im->sid_str);
		if (NULL != gfire->im->im_str) g_free(gfire->im->im_str);
		g_free(gfire->im); gfire->im = NULL;
	}
	if (NULL != gfire->email) g_free(gfire->email);
	if (NULL != gfire->buff_out) g_free(gfire->buff_out);
	if (NULL != gfire->buff_in) g_free(gfire->buff_in);
	if (NULL != gfire->buddies) g_list_free(gfire->buddies);
	if (NULL != gfire->xml_games_list) xmlnode_free(gfire->xml_games_list);
	if (NULL != gfire->xml_launch_info) xmlnode_free(gfire->xml_launch_info);
	if (NULL != gfire->userid) g_free(gfire->userid);
	if (NULL != gfire->sid) g_free(gfire->sid);
	if (NULL != gfire->alias) g_free(gfire->alias);

	g_free(gfire);
	gc->proto_data = NULL;
}


static int gfire_im_send(PurpleConnection *gc, const char *who, const char *what, PurpleMessageFlags flags)
{
	PurplePresence *p = NULL;
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	GList *gfbl = NULL;
	PurpleAccount *account = NULL;
	PurpleBuddy *buddy = NULL;
	int packet_len = 0;

	if (!gc || !(gfire = (gfire_data*)gc->proto_data))
		return 1;

	gfbl = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)who, GFFB_NAME);
	if (NULL == gfbl) return 1;

	gf_buddy = (gfire_buddy *)gfbl->data;
	account = purple_connection_get_account(gc);
	buddy = purple_find_buddy(account, gf_buddy->name);
	if(buddy == NULL){
		purple_conv_present_error(who, account, "Message could not be sent . Buddy not in contact list");
		return 1;
	}

	p = purple_buddy_get_presence(buddy);

	if (TRUE == purple_presence_is_online(p)) {
		/* in 2.0 the gtkimhtml stuff started escaping special chars & now = &amp;
		** XFire native clients don't handle it. */
		what = purple_unescape_html(what);
		packet_len = gfire_create_im(gc, gf_buddy, what);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(im send): %s: %s\n", NN(buddy->name), NN(what));
		gfire_send(gc, gfire->buff_out, packet_len);
		g_free((void *)what);
		return 1;
	} else {
		purple_conv_present_error(who, account, "Message could not be sent. Buddy offline");
		return 1;
	}
}


static void gfire_set_status(PurpleAccount *account, PurpleStatus *status)
{
	PurpleConnection *gc = NULL;
	gfire_data *gfire = NULL;
	char *msg = NULL;
        int freemsg=0;
	if (!purple_status_is_active(status))
		return;

	gc = purple_account_get_connection(account);
	gfire = (gfire_data *)gc->proto_data;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(status): got status change to name: %s id: %s\n",
		NN(purple_status_get_name(status)),
		NN(purple_status_get_id(status)));

	if (purple_status_is_available(status)) {
		if (gfire->away) msg = "";
		gfire->away = FALSE;
	} else {
		gfire->away = TRUE;
		msg =(char *)purple_status_get_attr_string(status, "message");
		if ((msg == NULL) || (*msg == '\0')) {
			msg = "(AFK) Away From Keyboard";
                } else {
			msg = purple_unescape_html(msg);
                        freemsg = 1;
                }
	}
	gfire_send_away(gc, msg);
        if (freemsg) g_free(msg);
}


static void gfire_add_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group)
{
	gfire_data *gfire = NULL;
	int packet_len = 0;
	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !buddy || !buddy->name) return;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "Adding buddy: %s\n", NN(buddy->name));
	packet_len = gfire_add_buddy_create(gc, buddy->name);
	gfire_send(gc, gfire->buff_out, packet_len);

}


static void gfire_add_buddies(PurpleConnection *gc, GList *buddies, GList *groups)
{
	FIXME("gfire_add_buddies");
}


static void gfire_remove_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group)
{
	gfire_data *gfire = NULL;
	int packet_len = 0;
	gfire_buddy *gf_buddy = NULL;
	GList *b = NULL;
	PurpleAccount *account = NULL;
	gboolean buddynorm = TRUE;
	char tmp[255] = "";

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !buddy || !buddy->name) return;

	if (!(account = purple_connection_get_account(gc))) return;

	b = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)buddy->name, GFFB_NAME);
	if (b == NULL) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_remove_buddy: buddy find returned NULL\n");
		return;
	}
	gf_buddy = (gfire_buddy *)b->data;
	if (!gf_buddy) return;
	
	buddynorm = purple_account_get_bool(account, "buddynorm", TRUE);
	if (buddynorm) {
		g_sprintf(tmp, "Not Removing %s", NN(buddy->name));
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_remove_buddy: buddynorm TRUE not removing buddy %s.\n",
					NN(buddy->name));
		purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_INFO, "Xfire Buddy Removal Denied", tmp, "Account settings are set to not remove buddies\n" "The buddy will be restored on your next login", NULL, NULL);
		return;
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "Removing buddy: %s\n", NN(gf_buddy->name));
	packet_len = gfire_remove_buddy_create(gc, gf_buddy);
	gfire_send(gc, gfire->buff_out, packet_len);

}


void gfire_remove_buddies(PurpleConnection *gc, GList *buddies, GList *groups)
{
	FIXME("gfire_remove_buddies");
}

GList *gfire_find_buddy_in_list( GList *blist, gpointer *data, int mode )
{
	gfire_buddy *b = NULL;
	guint8 *u = NULL;
	guint8 *f = NULL;
	gchar *n = NULL;

	if ((NULL == blist) || (NULL == data)) return NULL;

	blist = g_list_first(blist);
	switch(mode)
	{
		case GFFB_NAME:
			n = (gchar *)data;
			while (NULL != blist){
				b = (gfire_buddy *)blist->data;
				if ( 0 == g_ascii_strcasecmp(n, b->name)) return blist;
				blist = g_list_next(blist);
			}
			return NULL;
		break;
		case GFFB_ALIAS:
			n = (gchar *)data;
			while (NULL != blist){
				b = (gfire_buddy *)blist->data;
				if ( 0 == g_ascii_strcasecmp(n, b->alias)) return blist;
				blist = g_list_next(blist);
			}
			return NULL;
		break;
		case GFFB_USERID:
			n = (gchar *)data;
			while (NULL != blist){
				b = (gfire_buddy *)blist->data;
				if ( 0 == g_ascii_strcasecmp(n, b->uid_str)) return blist;
				blist = g_list_next(blist);
			}
			return NULL;
		break;
		case GFFB_UIDBIN:
			u = (guint8 *)data;
			while (NULL != blist) {
				b = (gfire_buddy *)blist->data;
				f = b->userid;
				if ( (u[0] == f[0]) && (u[1] == f[1]) && (u[2] == f[2]) && (u[3] == f[3]))
					return blist;
				blist = g_list_next(blist);
			}
			return NULL;
		break;
		case GFFB_SIDS:
			n = (gchar *)data;
			while (NULL != blist){
				b = (gfire_buddy *)blist->data;
				if (!(NULL == b->sid_str) && (0 == g_ascii_strcasecmp(n, b->sid_str)))
					return blist;
				blist = g_list_next(blist);
			}
			return NULL;
		break;
		case GFFB_SIDBIN:
			while (NULL != blist){
				b = (gfire_buddy *)blist->data;
				if ((NULL != b->sid) && (memcmp(b->sid, data, XFIRE_SID_LEN) == 0))
					return blist;
				blist = g_list_next(blist);
			}
			return NULL;
		break;
		default:
			/* mode not implemented */
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: gfire_find_buddy_in_list, called with invalid mode\n");
			return NULL;
	}
}


void gfire_new_buddy(PurpleConnection *gc, gchar *uid_str, gchar *alias, gchar *name)
{
	PurpleBuddy *buddy = NULL;
	PurpleAccount *account = NULL;
	PurpleGroup *default_purple_group = NULL;

	account = purple_connection_get_account(gc);
	default_purple_group = purple_find_group(GFIRE_DEFAULT_GROUP_NAME);
	/* upgrade code, so we can migrate 1.5 buddy lists to 2.0 */
	/* previous version used uid_str to name buddy */
	buddy = purple_find_buddy(account, uid_str);
	if (NULL != buddy) {
		/* remove old settings */
		purple_blist_alias_buddy(buddy, NULL);
		purple_blist_node_remove_setting((PurpleBlistNode *)buddy, "username");
		purple_blist_node_remove_setting((PurpleBlistNode *)buddy, "imindex");
		purple_blist_node_remove_setting((PurpleBlistNode *)buddy, "game");
		purple_blist_node_remove_setting((PurpleBlistNode *)buddy, "port");
		purple_blist_node_remove_setting((PurpleBlistNode *)buddy, "sid");
		/* rename the buddy and continue as usual */
		purple_blist_rename_buddy(buddy, g_strdup(name));
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "migrating xfire 1.5 buddy %s, to 2.0 buddy %s\n",
					NN(uid_str), NN(name));
	}
	buddy = purple_find_buddy(account, name);	
	if (buddy== NULL){
		if (NULL == default_purple_group) {
			default_purple_group = purple_group_new(GFIRE_DEFAULT_GROUP_NAME);
			purple_blist_add_group(default_purple_group, NULL);
		}
		buddy = purple_buddy_new(account, g_strdup(name), g_strdup(alias));
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(buddylist): buddy %s not found in Pidgin buddy list, adding.\n",
					NN(name));
		purple_blist_add_buddy(buddy, NULL, default_purple_group, NULL);
	}else{
		purple_blist_server_alias_buddy(buddy, g_strdup(alias));
	}
}


void gfire_new_buddies(PurpleConnection *gc)
{
	gfire_data *gfire = (gfire_data *)gc->proto_data;
	gfire_buddy *b = NULL;
	GList *tmp = gfire->buddies;

	while (NULL != tmp) {
		b = (gfire_buddy *)tmp->data;
		gfire_new_buddy(gc, b->uid_str, b->alias, b->name);
		tmp = g_list_next(tmp);
	}
}


void gfire_handle_im(PurpleConnection *gc)
{
	gfire_data *gfire = NULL;
	gfire_im	*im = NULL;
	GList		*gfbl = NULL;
	gfire_buddy	*gf_buddy = NULL;
	PurpleAccount *account = NULL;
	PurpleBuddy *buddy = NULL;

	if ( !gc || !(gfire = (gfire_data *)gc->proto_data) || !(im = gfire->im) )
		return;
	purple_debug(PURPLE_DEBUG_MISC, "gfire", "im_handle: looking for sid %s\n", NN(im->sid_str));
	gfbl = gfire_find_buddy_in_list(gfire->buddies, (gpointer *) im->sid_str, GFFB_SIDS);
	if (NULL == gfbl) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "im_handle: buddy find returned NULL\n");
		g_free(im->im_str);
		g_free(im->sid_str);
		g_free(im);
		gfire->im = NULL;
		return;
	}
	gf_buddy = (gfire_buddy *)gfbl->data;
	account = purple_connection_get_account(gc);
	buddy = purple_find_buddy(account, gf_buddy->name);
	if (NULL == buddy) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "im_handle: PIDGIN buddy find returned NULL for %s\n",
					NN(gf_buddy->name));
		g_free(im->im_str);
		g_free(im->sid_str);
		g_free(im);
		gfire->im = NULL;
		return;
	}

	if (PURPLE_BUDDY_IS_ONLINE(buddy)) {

		switch (im->type)
		{
			case 0:	/* got an im */
				serv_got_im(gc, buddy->name, gfire_escape_html(im->im_str),0, time(NULL));
			break;
			case 1: /* got an ack packet... we shouldn't be here */
			break;
			case 2: /* got typing */
				serv_got_typing(gc, buddy->name, 30, PURPLE_TYPING);
			break;
		}
	}

	if (NULL != im->im_str) g_free(im->im_str);
	if (NULL != im->sid_str) g_free(im->sid_str);
	g_free(im);
	gfire->im = NULL;
}


/* connection keep alive.  We send a packet to the xfire server
 * saying we are still around.  Otherwise they will forcibly close our connection
 * purple allows for this, but calls us every 30 seconds.  We keep track of *all* sent
 * packets.  We only need to send this keep alive if we haven't sent anything in a long(tm)
 * time.  So we watch and wait.
*/
void gfire_keep_alive(PurpleConnection *gc){
	static int count = 1;
	int packet_len;
	gfire_data *gfire = NULL;
	GTimeVal gtv;

	g_get_current_time(&gtv);
	if ((purple_connection_get_state(gc) != PURPLE_DISCONNECTED) &&
		(NULL != (gfire = (gfire_data *)gc->proto_data)) &&
		((gtv.tv_sec - gfire->last_packet) > XFIRE_KEEPALIVE_TIME))
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "send keep_alive packet (PING)\n");
		packet_len = gfire_ka_packet_create(gc);
		if (packet_len > 0)	gfire_send(gc, gfire->buff_out, packet_len);
		count = 1;
	}
	count++;
}


void gfire_update_buddy_status(PurpleConnection *gc, GList *buddies, int status)
{
	gfire_buddy *gf_buddy = NULL;
	GList *b = g_list_first(buddies);
	PurpleBuddy *gbuddy = NULL;

	if (!buddies || !gc || !gc->account) {
		if (buddies) g_list_free(buddies);
		return;
	}

	while ( NULL != b ) {
		gf_buddy = (gfire_buddy *)b->data;
		if ((NULL != (gf_buddy = (gfire_buddy *)b->data)) && (NULL != gf_buddy->name)) {
			gbuddy = purple_find_buddy(gc->account, gf_buddy->name);
			if (NULL == gbuddy) { b = g_list_next(b); continue; }
			switch (status)
			{
				case GFIRE_STATUS_ONLINE:
					if ( 0 == g_ascii_strcasecmp(XFIRE_SID_OFFLINE_STR,gf_buddy->sid_str)) {

						purple_prpl_got_user_status(gc->account, gf_buddy->name, "offline", NULL);
					} else {
						purple_prpl_got_user_status(gc->account, gf_buddy->name, "available", NULL);
					}

				break;
				case GFIRE_STATUS_GAME:

					if ( gf_buddy->gameid > 0 ) {
						purple_prpl_got_user_status(gc->account, gf_buddy->name, "available", NULL);
					} else {
						purple_prpl_got_user_status(gc->account, gf_buddy->name, "available", NULL);
					}

				break;
				case GFIRE_STATUS_AWAY:
					if ( gf_buddy->away ) {

						purple_prpl_got_user_status(gc->account, gf_buddy->name, "away", NULL);
					} else {
						purple_prpl_got_user_status(gc->account, gf_buddy->name, "available", NULL);
					}

				break;
				default:
					purple_debug(PURPLE_DEBUG_MISC, "gfire", "update_buddy_status: Unknown mode specified\n");
			}
		}
		b = g_list_next(b);
	}
	g_list_free(buddies);
}



void gfire_buddy_add_authorize_cb(void *data, const char *msg)
{
	PurpleConnection *gc = NULL;
	gfire_buddy *b = (gfire_buddy *)data;
	gfire_data *gfire = NULL;
	int packet_len = 0;

	if (!b) {
		if (b->name) g_free(b->name);
		if (b->alias) g_free(b->alias);
		if (b->uid_str) g_free(b->uid_str);
		g_free(b);
 		return;
	}
	gc = (PurpleConnection *)b->sid;
	b->sid = NULL;
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) {
		if (b->name) g_free(b->name);
		if (b->alias) g_free(b->alias);
		if (b->uid_str) g_free(b->uid_str);
		g_free(b);
 		return;
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "Authorizing buddy invitation: %s\n", NN(b->name));
	packet_len = gfire_invitation_accept(gc, b->name);
	gfire_send(gc, gfire->buff_out, packet_len);

	if (b->name) g_free(b->name);
	if (b->alias) g_free(b->alias);
	if (b->uid_str) g_free(b->uid_str);
	g_free(b);

}


void gfire_buddy_add_deny_cb(void *data, const char *msg)
{
	PurpleConnection *gc = NULL;
	gfire_buddy *b = (gfire_buddy *)data;
	gfire_data *gfire = NULL;
	int packet_len = 0;

	if (!b) {
		if (b->name) g_free(b->name);
		if (b->alias) g_free(b->alias);
		if (b->uid_str) g_free(b->uid_str);
		g_free(b);
 		return;
	}
	gc = (PurpleConnection *)b->sid;
	b->sid = NULL;
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) {
		if (b->name) g_free(b->name);
		if (b->alias) g_free(b->alias);
		if (b->uid_str) g_free(b->uid_str);
		g_free(b);
 		return;
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "Denying buddy invitation: %s\n", NN(b->name));
	packet_len = gfire_invitation_deny(gc, b->name);
	gfire_send(gc, gfire->buff_out, packet_len);

	if (b->name) g_free(b->name);
	if (b->alias) g_free(b->alias);
	if (b->uid_str) g_free(b->uid_str);
	g_free(b);

}


void gfire_buddy_menu_profile_cb(PurpleBlistNode *node, gpointer *data)
{
	PurpleBuddy *b =(PurpleBuddy *)node;
	if (!b || !(b->name)) return;

	char uri[256] = "";
	g_sprintf(uri, "%s%s", XFIRE_PROFILE_URL, b->name);
	purple_notify_uri((void *)_gfire_plugin, uri);
 }


 /**
  * Callback function for pidgin buddy list right click menu.  This callback
  * is what the interface calls when the Join Game ... menu option is selected
  *
  * @param node		BuddyList node provided by interface
  * @param data		Gpointer data (not used)
*/
 void gfire_buddy_menu_joingame_cb(PurpleBlistNode *node, gpointer *data)
{
 	int game = 0;
	gchar *serverip = NULL;
 	int serverport = 0;
	PurpleBuddy *b = (PurpleBuddy *)node;
 	PurpleConnection *gc = NULL;

	if (!b || !b->account || !(gc = purple_account_get_connection(b->account))) return;

 	game = gfire_get_buddy_game(gc, b);
	if (game && gfire_game_playable(gc, game)) {
 		serverport = gfire_get_buddy_port(gc, b);
 		if (serverport) serverip = (gchar *)gfire_get_buddy_ip(gc, b);
 		
 		gfire_join_game(gc, serverip, serverport, game);
 	}
 }
 



/*
 *	purple callback function.  Not used directly.  Purple calls this callback
 *	when user right clicks on Xfire buddy (but before menu is displayed)
 *	Function adds right click "Join Game . . ." menu option.  If game is
 *	playable (configured through launch.xml), and user is not already in
 *	a game.
 *
 *	@param	node		Pidgin buddy list node entry that was right clicked
 *
 *	@return	Glist		list of menu items with callbacks attached (or null)
*/
GList * gfire_node_menu(PurpleBlistNode *node)
{
	GList *ret = NULL;
	PurpleMenuAction *me = NULL;
	PurpleBuddy *b =(PurpleBuddy *)node;
	gfire_buddy *gb = NULL;
	GList *l = NULL;
	PurpleConnection *gc = NULL;
	gfire_data *gfire = NULL;

	if (!b || !b->account || !(gc = purple_account_get_connection(b->account)) ||
					     !(gfire = (gfire_data *) gc->proto_data))
		return NULL;


	if (PURPLE_BLIST_NODE_IS_BUDDY(node)) {
		l = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)b->name, GFFB_NAME);
		if (!l) return NULL; /* can't find the buddy? not our plugin? */
		gb = (gfire_buddy *)l->data;

		if (gb && !gfire->gameid && gb->gameid && gfire_game_playable(gc, gb->gameid)){
			 /* don't show menu if we are in game */

			me = purple_menu_action_new("Join Game ...",
					PURPLE_CALLBACK(gfire_buddy_menu_joingame_cb),NULL, NULL);

			if (!me) {
				return NULL;
			}
			ret = g_list_append(ret, me);
		}

		me = purple_menu_action_new("Xfire Profile",
			PURPLE_CALLBACK(gfire_buddy_menu_profile_cb),NULL, NULL);

		if (!me) {
			return NULL;
		}
		ret = g_list_append(ret, me);
 	}
	return ret;

 }


static void gfire_change_nick(PurpleConnection *gc, const char *entry)
{
	gfire_data *gfire = NULL;
	int packet_len = 0;
	gfire_buddy *b = NULL;
	GList *l = NULL;
	PurpleBuddy *buddy = NULL;
	PurpleAccount *account = purple_connection_get_account(gc);

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;

	packet_len = gfire_create_change_alias(gc, (char *)entry);
	if (packet_len) {
		gfire_send(gc, gfire->buff_out, packet_len);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "Changed server nick (alias) to \"%s\"\n", NN(entry)); 
	} else {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire",
				"ERROR: During alias change, _create_change_alias returned 0 length\n"); 
		return;
	}
	purple_connection_set_display_name(gc, entry);

	l = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)gfire->userid, GFFB_UIDBIN);
	if (l) {
		/* we are in our own buddy list... change our server alias :) */
		b = (gfire_buddy *)l->data;
		buddy = purple_find_buddy(account, b->name);	
		if (buddy){
			purple_blist_server_alias_buddy(buddy, entry);
		}
	}
}


static void gfire_action_nick_change_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;

	PurpleAccount *account = purple_connection_get_account(gc);

	purple_request_input(gc, NULL, "Change XFire Nickname to what?","Blank entry clears your current alias", purple_connection_get_display_name(gc), FALSE, FALSE, NULL, "OK", G_CALLBACK(gfire_change_nick), "Cancel", NULL, account, NULL, NULL, gc);
}


static void gfire_action_reload_lconfig_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	gfire_data *gfire = NULL;
	const char *result = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;

	if (NULL != gfire->xml_launch_info) xmlnode_free(gfire->xml_launch_info);
	gfire->xml_launch_info = NULL;
	gfire_parse_launchinfo_file(gc, g_build_filename(purple_user_dir(), "gfire_launch.xml", NULL));

	if (NULL == gfire->xml_launch_info) {
		purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_ERROR, "Gfire XML Reload", "Reloading gfire_launch.xml", "Operation failed. File not found or content was incorrect", NULL, NULL);
	} else {
		purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_INFO, "Gfire XML Reload", "Reloading gfire_launch.xml","Reloading was successful.", NULL, NULL);

	}

}


static void gfire_action_reload_gconfig_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	gfire_data *gfire = NULL;
	const char *result = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;

	if (NULL != gfire->xml_games_list) xmlnode_free(gfire->xml_games_list);
	gfire->xml_games_list = NULL;
	gfire_parse_games_file(gc, g_build_filename(purple_user_dir(), "gfire_games.xml", NULL));

	if (NULL == gfire->xml_games_list) {
		result = "Operation failed. File not found or content was incorrect.";
	} else {
		result = "Reloading was successful.";
	}
	purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_INFO, "Gfire XML Reload","Reloading gfire_games.xml", result, NULL, NULL);

}


static void gfire_action_about_cb(PurplePlugin *plugin)
{
	purple_notify_message(plugin, PURPLE_NOTIFY_MSG_INFO, "About", "Gfire", "Version: "GFIRE_VERSION"\n"GFIRE_WEBSITE, NULL, NULL);

}

static void gfire_action_get_gconfig_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	gfire_data *gfire = NULL;
	const char *filename = g_build_filename(purple_user_dir(), "gfire_games.xml", NULL);

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;

	if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
		/* file doesn't exist */
		purple_util_fetch_url(GFIRE_GAMES_XML_URL, TRUE, "Purple-xfire", TRUE, gfire_xml_download_cb, (void *)gc);
		return FALSE;
	}

	if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
		/* file exist */
		g_remove(filename);
		purple_util_fetch_url(GFIRE_GAMES_XML_URL, TRUE, "Purple-xfire", TRUE, gfire_xml_download_cb, (void *)gc);
		return FALSE;
	}

	if (NULL != gfire->xml_games_list) xmlnode_free(gfire->xml_games_list);
	gfire->xml_games_list = NULL;
	gfire_parse_games_file(gc, filename);
}

static GList *gfire_actions(PurplePlugin *plugin, gpointer context)
{
	GList *m = NULL;
	PurplePluginAction *act;

	act = purple_plugin_action_new("Change Nickname",
			gfire_action_nick_change_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new("Reload Launch Config",
			gfire_action_reload_lconfig_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new("Reload Game ID List",
			gfire_action_reload_gconfig_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new("Get Game ID List",
			gfire_action_get_gconfig_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new("About",
			gfire_action_about_cb);
	m = g_list_append(m, act);
	return m;
}


 /**
  * Joins a game in progress with buddy. spawns process, puts us in game with xfire
  * and adds a gsource watch on the child process so when it exits we can take
  * ourselves out of game.
  *
  * @param gc			pointer to our PurpleConnection for sending network traffic.
  * @param sip		server ip (quad dotted notation) of where the game is
  * @param sport		server port of where the game is located
  * @param game		the xfire game ID to join (looked up in launch options)
  *
  * asking to join a game that is not playable (not configured through launch options
  * or any other reason for an unplayable game is not defined.  You must check
  * the game playability with gfire_game_playable()
  *
  * @see gfire_game_playable
 */
 void gfire_join_game(PurpleConnection *gc, const gchar *sip, int sport, int game)
 {
 	/* test stuff */
 	int len = 0;
 	gfire_linfo *linfo =NULL;
 	gchar *command = NULL;
 	char **argvp;
 	int argcp = 0;
 	gboolean worked = FALSE;
 	GPid pid;
 	GError *gerr;
	gfire_data *gfire = NULL;
	const gchar nullip[4] = {0x00, 0x00, 0x00, 0x00};
	gboolean sworked = FALSE;
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;
 	
 	linfo = gfire_linfo_get(gc, game);
 	if (!linfo) {
 		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "Launch info struct not defined!\n");
 		return;
 	}
 	if (!sip) sip = (char *)&nullip;
 	command = gfire_linfo_get_cmd(linfo, (guint8 *)sip, sport, NULL);
	gerr = 0;
 	worked=sworked=g_shell_parse_argv(command, &argcp, &argvp, &gerr);
 	if (worked) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "Attempting to join game %d, on server %d.%d.%d.%d , at port %d\n",
				game, NNA(sip, sip[3]), NNA(sip, sip[2]), NNA(sip, sip[1]), NNA(sip, sip[0]), sport);
 		purple_debug(PURPLE_DEBUG_MISC, "gfire", "launch xml command parsed to:\n");
 		purple_debug(PURPLE_DEBUG_MISC, "gfire", "%s\n", NN(command));
 		gerr = 0;
		worked = g_spawn_async(linfo->c_wdir, argvp, NULL,
 						(GSpawnFlags)G_SPAWN_DO_NOT_REAP_CHILD,
 						NULL, NULL, &pid, &gerr);
	}
 	if (!worked) {
 		/* something went wrong! */
 		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "Launch failed, message: %s\n", NN(gerr->message));
 		g_free(command);
 		if (sworked) {
			g_strfreev(argvp);
		}
 		g_error_free (gerr);
		return;
 	}
 	
 	/* program seems to be running! */
 	gfire->gameid = game;
 	len = gfire_join_game_create(gc, game, sport, sip);
 	if (len) {
		gfire_send(gc, gfire->buff_out, len);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(game join): telling xfire our game info\n");
	}
 	/* need to watch pid so we can get out of game when game closes */
 	g_child_watch_add(pid, (GChildWatchFunc)gfire_game_watch_cb, (gpointer *)gc);
 	g_free(command);
 	g_strfreev(argvp);
//	g_free(sip);
 
}

 
 /**
  *  GSource watch pid callback for glib.  This function waits for a join game
  *  PID to exit.  This tells gfire to remove user out of game when the user
  *  quits the game. This function is not called directly but by glib.  The
  *  function prototype is set by glib.  This function does not return anything
  *
  *  @param      pid     Integer PID of the process to watch (wait for to die)
  *  @param      status  status of exiting pid
  *  @param      data    Pointer to data passed in by setup function
  *
 */
void gfire_game_watch_cb(GPid pid, int status, gpointer *data)
{
 	/* were now out of game, clean up pid send network message */
 
 	int len = 0;
	PurpleConnection *gc = (PurpleConnection *)data;
	gfire_data *gfire = NULL;
 
	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(game_watch_cb): Child has exited, reaping pid.\n");
 	g_spawn_close_pid(pid);
	/* we could have been disconnected */
	if ( PURPLE_CONNECTION_IS_VALID(gc) && PURPLE_CONNECTION_IS_CONNECTED(gc) ) {
		if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;

		len = gfire_join_game_create(gc, 0, 0, NULL);
		if (len) gfire_send(gc, gfire->buff_out, len);
		gfire->gameid = 0;
	}
}


/**
 *	g_timeout_add callback function to watch for $HOME/.purple/ingame.tmp
 *	file.  If this file is found we send join_game data to xfire network
 *	if we are in game and then the file is no longer present then take
 *	client out of game.  Only do this when in game status was met using
 *	ingame.tmp file.  The ingame.tmp file is a copied LaunchInfo.txt file
 *	from XQF.  The user must create a script to copy this file and then
 *	remove it once the game is quit.  We match the contents of the XQF
 *	file against our launch.xml.  Find the game and then send the network
 *	message.  This function is not called directy. It is called by glib.
 *
 *	@param		gc		pointer to purple connection struct for this connection
 *
 *	@return				returns TRUE always, except if gc connection state
 *						is set to disconnected.
**/
int gfire_check_xqf_cb(PurpleConnection *gc)
{
 
 	static char *filename = NULL;
 	static gboolean found = FALSE;
 	gfire_xqf_linfo *xqfs = NULL;
	int len = 0;
 	int game = 0;
	gfire_data *gfire = NULL;
	char *ipbin = NULL;
 
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return FALSE;

 	if (purple_connection_get_state(gc) != PURPLE_DISCONNECTED) {
 		if ((0 != gfire->gameid) && !found) return TRUE;
 		if (!filename) {
			filename = g_build_filename(purple_user_dir(), GFIRE_XQF_FILENAME, NULL);
 		}
 		if (g_file_test(filename, G_FILE_TEST_EXISTS)){
 			if (found) return TRUE;
 			/* file was found, but not previously */
 			found = TRUE;
 			xqfs = gfire_linfo_parse_xqf(filename);
 			if (!xqfs) return TRUE;
 			game = gfire_xqf_search(gc, xqfs);
 			if (!game) {
 				purple_debug(PURPLE_DEBUG_WARNING, "gfire", "(XQF cb): parsed ingame.tmp. No game match found.\n");
 				gfire_xqf_linfo_free(xqfs);
 				return TRUE;
 			}

			/* turn ip string into ip binary string in xfire format */
			ipbin = gfire_ipstr_to_bin(xqfs->ip);
 			len = gfire_join_game_create(gc, game, xqfs->port, ipbin);
 			if (len) gfire_send(gc, gfire->buff_out, len);
			g_free(ipbin);
 			gfire->gameid = game;
 			purple_debug(PURPLE_DEBUG_MISC, "gfire", "(XQF cb): Detected game join (%d) at (%s:%d)\n", game,
						NN(xqfs->ip), xqfs->port );
 			gfire_xqf_linfo_free(xqfs);
 		} else {
 			if (!found) return TRUE;
 			if (gfire->gameid) {
 				purple_debug(PURPLE_DEBUG_MISC, "xfire", "(XQF cb): Status file removed, sending out of game msg\n");
 				gfire->gameid = 0;
 				len = gfire_join_game_create(gc, 0, 0, NULL);
 				if (len) gfire_send(gc, gfire->buff_out, len);			
 			}
 			found = FALSE;
 		}
 		return TRUE;
 	}
 	purple_debug(PURPLE_DEBUG_ERROR, "gfire", "(XQF cb): Still running but GC says not connected!\n");
 	return FALSE;
 }


void gfire_set_presence(PurpleConnection *gc)
{
}


char *gfire_escape_html(const char *html)
{
	char *escaped = NULL;

	if (html != NULL) {
		const char *c = html;
		GString *ret = g_string_new("");
		while (*c) {
			if (!strncmp(c, "&", 1)) {
				ret = g_string_append(ret, "&amp;");
				c += 1;
			} else if (!strncmp(c, "<", 1)) {
				ret = g_string_append(ret, "&lt;");
				c += 1;
			} else if (!strncmp(c, ">", 1)) {
				ret = g_string_append(ret, "&gt;");
				c += 1;
			} else if (!strncmp(c, "\"", 1)) {
				ret = g_string_append(ret, "&quot;");
				c += 1;
			} else if (!strncmp(c, "'", 1)) {
				ret = g_string_append(ret, "&apos;");
				c += 1;
			} else if (!strncmp(c, "\n", 1)) {
				ret = g_string_append(ret, "<br>");
				c += 1;
			} else {
				ret = g_string_append_c(ret, c[0]);
				c++;
			}
		}

		escaped = ret->str;
		g_string_free(ret, FALSE);
	}
	return escaped;
}



/*
 * Plugin Initialization section
*/
static PurplePluginProtocolInfo prpl_info =
{

	OPT_PROTO_CHAT_TOPIC,			/* Protocol options  */
	NULL,					/* user_splits */
	NULL,					/* protocol_options */
	NO_BUDDY_ICONS,				/* icon_spec */
	gfire_blist_icon,			/* list_icon */
	gfire_blist_emblems,			/* list_emblems */
	gfire_status_text,			/* status_text */
	gfire_blist_tooltip_text,		/* tooltip_text */
	gfire_status_types,			/* away_states */
	gfire_node_menu,			/* blist_node_menu */
	NULL,					/* chat_info */
	NULL,					/* chat_info_defaults */
	gfire_login,				/* login */
	gfire_close,				/* close */
	gfire_im_send,				/* send_im */
	NULL,					/* set_info */
	NULL,					/* send_typing */
	NULL,					/* get_info */
	gfire_set_status,			/* set_status */
	NULL,					/* set_idle */
	NULL,					/* change_passwd */
	gfire_add_buddy,			/* add_buddy */
	gfire_add_buddies,			/* add_buddies */
	gfire_remove_buddy,			/* remove_buddy */
	gfire_remove_buddies,			/* remove_buddies */
	NULL,					/* add_permit */
	NULL,					/* add_deny */
	NULL,					/* rem_permit */
	NULL,					/* rem_deny */
	NULL,					/* set_permit_deny */
	NULL,					/* join_chat */
	NULL,					/* reject_chat */
	NULL,					/* get_chat_name */
	NULL,					/* chat_invite */
	NULL,					/* chat_leave */
	NULL,					/* chat_whisper */
	NULL,					/* chat_send */
	gfire_keep_alive,			/* keepalive */
	NULL,					/* register_user */
	NULL,					/* get_cb_info */
	NULL,					/* get_cb_away */
	NULL,					/* alias_buddy */
	NULL,					/* group_buddy */
	NULL,					/* rename_group */
	NULL,					/* buddy_free */
	NULL,					/* convo_closed */
	purple_normalize_nocase,		/* normalize */
	NULL,					/* set_buddy_icon */
	NULL,					/* remove_group */
	NULL,					/* get_cb_real_name */
	NULL,					/* set_chat_topic */
	NULL,					/* find_blist_chat */
	NULL,					/* roomlist_get_list */
	NULL,					/* roomlist_cancel */
	NULL,					/* roomlist_expand_category */
	NULL,					/* can_receive_file */
	NULL,					/* send_file */
	NULL,					/* new_xfer */
	NULL,					/* offline_message */
	NULL,					/* whiteboard_prpl_ops */
	NULL,					/* send_raw */
	NULL,					/* roomlist_room_serialize */
	NULL,					/* unregister_user */
	NULL,					/* send_attention */
	NULL,					/* attention_types */

	/* padding */
	NULL
};


static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL,				/**< type           */
	NULL,						/**< ui_requirement */
	0,						/**< flags          */
	NULL,						/**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,			/**< priority       */
	"prpl-xfire",					/**< id             */
	"Xfire",					/**< name           */
	GFIRE_VERSION,					/**< version        */
	"Xfire Protocol Plugin",			/**  summary        */
	"Xfire Protocol Plugin",			/**  description    */
	NULL,						/**< author         */
	GFIRE_WEBSITE,					/**< homepage       */
	NULL,						/**< load           */
	NULL,						/**< unload         */
	NULL,						/**< destroy        */
	NULL,						/**< ui_info        */
	&prpl_info,					/**< extra_info     */
	NULL,						/**< prefs_info     */
	gfire_actions,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void _init_plugin(PurplePlugin *plugin)
{
	PurpleAccountOption *option;

	option = purple_account_option_string_new("Server", "server",XFIRE_SERVER);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_int_new("Port", "port", XFIRE_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);
	
	option = purple_account_option_int_new("Version", "version", XFIRE_PROTO_VERSION);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);
	
	option = purple_account_option_bool_new("Don't delete buddies from server",
						"buddynorm", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);


	_gfire_plugin = plugin;
}

PURPLE_INIT_PLUGIN(gfire, _init_plugin, info);



