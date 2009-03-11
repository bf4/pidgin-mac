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
#ifndef _GFIRE_H
#define _GFIRE_H

#define PURPLE_PLUGINS

#include <stddef.h>
#include <stdlib.h>
#include <glib-object.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <unistd.h>
#include <string.h>

#ifndef _WIN32
#include <errno.h>
#endif /* _WIN32 */

#include "util.h"
#include "server.h"
#include "notify.h"
#include "plugin.h"
#include "account.h"
#include "accountopt.h"
#include "blist.h"
#include "conversation.h"
#include "debug.h"
#include "prpl.h" 
#include "proxy.h"
#include "util.h"  
#include "version.h" 
#include "request.h"

#include "xmlnode.h"

#include "gf_debug.h"

#define GFIRE_WEBSITE "http://gfire.sf.net"
#define GFIRE_XQF_FILENAME "ingame.tmp"
#define GFIRE_DEFAULT_GROUP_NAME "Xfire"
#define GFIRE_VERSION "0.7.1"
#define GFIRE_GAMES_XML_URL "http://gfire.site40.net/files/gfire_games.xml"
#define XFIRE_HEADER_LEN 5
#define XFIRE_USERID_LEN 4
#define XFIRE_SID_LEN 16
#define XFIRE_GAMEID_LEN 4
#define XFIRE_GAMEPORT_LEN 2
#define XFIRE_GAMEIP_LEN 4
#define XFIRE_SERVER "cs.xfire.com"
#define XFIRE_PORT 25999
#define XFIRE_PROTO_VERSION 98
#define XFIRE_CONNECT_STEPS 3
#define XFIRE_SID_OFFLINE_STR "00000000000000000000000000000000"
#define XFIRE_KEEPALIVE_TIME 300  // see gfire_keep_alive for more info
#define XFIRE_PROFILE_URL "http://www.xfire.com/profile/"


typedef struct _gfire_data	gfire_data;
typedef struct _gfire_buddy	gfire_buddy;
typedef struct _gfire_im	gfire_im;

struct _gfire_data { 
	int fd; 
	gchar *email;
	guint8 *buff_out;
	guint8 *buff_in;
	GList	*buddies;
	gfire_im *im;				/* im struct, filled when we have an im to proccess */
	gboolean away;
	xmlnode *xml_games_list;
	xmlnode *xml_launch_info;
	gulong	last_packet;			/* time (in seconds) of our last packet */
	guint8	*userid;			/* our userid on the xfire network */
	guint8	*sid;				/* our session id for this connection */
	gchar 	*alias;				/* our current server alias */
	guint32	gameid;				/* our current game id */
	guint	xqf_source;			/* g_timeout_add source number for xqf callback */
};

struct _gfire_buddy {
	gboolean	away;	/* TRUE == buddy is away */
	gchar	*away_msg;	/* pointer to away message, null if not away */
	guint32	im;		/* im index ++'d on each im reception and and send */
	gchar 	*name;		/* name (xfire login id) */
	gchar 	*alias;		/* nick (xfire alias) */
	guint8 	*userid;	/* xfire user id binary */
	gchar	*uid_str;	/* xfire user id (string format hex) 8 chars + 1 0x00 */
	guint8	*sid;		/* sid binary form, length = XFIRE_SID_LEN */
	gchar	*sid_str;	/* sid string representation */
	guint32	gameid;		/* int game id */
	guint32	gameport;	/* int game port */
	guint8	*gameip;	/* char[4] game port, each byte is an octet */

};

struct _gfire_im {
	guint32	type;		/* msgtype 0 = im, 1 = ack packet, 2 = auth question? */
	guint8	peer;		/* peermsg from packet */
	guint32 index;		/* im index for this conversation */
	gchar	*sid_str;	/* sid (string format) of buddy that this im belongs to */
	gchar	*im_str;	/* im text */
};


/* gfire_find_buddy_in_list MODES */
#define GFFB_NAME	0	/* by name, pass pointer to string */
#define GFFB_ALIAS	1	/* by alias, pass pointer to string */
#define GFFB_USERID	2	/* by userid, pass pointer to string ver of uid */
#define GFFB_UIDBIN 4	/* by userid, pass binary string of userid */
#define GFFB_SIDS	8	/* by sid (as string) pass pointer of string */
#define GFFB_SIDBIN 16	/* by sid (binary data) pass pointer */

/* gfire_update_buddy_status TYPES */
#define GFIRE_STATUS_ONLINE		0	/* set buddies online / offline */
#define	GFIRE_STATUS_GAME		1	/* update game information */
#define	GFIRE_STATUS_AWAY		2	/* update away status */


void gfire_close(PurpleConnection *gc);
GList *gfire_find_buddy_in_list( GList *blist, gpointer *data, int mode );
void gfire_new_buddy(PurpleConnection *gc, gchar *uid_str, gchar *alias, gchar *name);
void gfire_new_buddies(PurpleConnection *gc);
void gfire_handle_im(PurpleConnection *gc);
void gfire_update_buddy_status(PurpleConnection *gc, GList *buddies, int status);
void gfire_buddy_add_authorize_cb(void *data, const char *msg);
void gfire_buddy_add_deny_cb(void *data, const char *msg);
int gfire_check_xqf_cb(PurpleConnection *gc);
void gfire_set_presence(PurpleConnection *gc);

#endif /* _GFIRE_H */
