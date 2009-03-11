/*
 * pidgin - Xfire Protocol Plugin
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

#include "gfire.h"

#include "gf_network.h"
#include "gf_packet.h"

#include "cipher.h"



void gfire_add_header(guint8 *packet, int length, int type, int atts)
{
	guint8 buffer[5] = { 0,0,0,0,(guint8) atts };
	guint16 l = GUINT16_TO_LE((guint16) length);
	guint16 t = GUINT16_TO_LE((guint16) type);

	memcpy(buffer,&l,2);
	memcpy(buffer+2,&t,2);
	memcpy(packet,buffer,XFIRE_HEADER_LEN);
}


int gfire_add_att_name(guint8 *packet,int packet_length, char *att)
{
	packet[packet_length] = (guint8)strlen(att);//set att length
	memcpy(packet+packet_length+1,att,strlen(att)); //set attname
	return packet_length+1+strlen(att);
}

int gfire_read_attrib(GList **values, guint8 *buffer, int packet_len, const char *name,
					gboolean dynamic, gboolean binary, int bytes_to_first, int bytes_between, int vallen)
{
	int index = 0; int i=0; int ali = 0; int alen = 0;
	gchar tmp[100];
	guint16 numitems = 0; guint16 attr_len = 0;
	guint8 *str;

	memset(tmp, 0x00, 100);

	alen = strlen(name);
	memcpy(tmp, buffer + index, alen);
	index += strlen(name);
	if ( 0 == g_ascii_strcasecmp(name, tmp)) {
		index += 2;
		memcpy(&numitems, buffer + index, 2);
		numitems = GUINT16_FROM_LE(numitems);
		index += 2;
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "Looking for %d %s's in packet.\n", numitems, NN(name));
	} else {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: %s signature isn't in the correct position.\n", NN(name));
		return -1;
	} 

	/* if we are copying a string make sure we have a space for the trailing \0 */
	if (binary) ali = 0;
		else ali = 1;

	for (i = 0; i < numitems; i++) {
		if (dynamic) {
			memcpy(&attr_len, buffer + index, 2);
			attr_len = GUINT16_FROM_LE(attr_len); index+=2;
		} else attr_len = vallen;

		if (dynamic && (attr_len == 0)) str = NULL;
			else {
				str = g_malloc0(sizeof(char) * (attr_len+ali));
				memcpy(str, buffer + index, attr_len);
				if (!binary) str[attr_len] = 0x00;
			}
		index += attr_len;
		*values = g_list_append(*values,(gpointer *)str);
		if ( index > packet_len ) {
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: pkt 131: more friends then packet length.\n");
			return -1;
		}
	}
	return index;
}


	static void hashSha1(char* input,char* digest)
	//based on code from purple_util_get_image_filename in the pidgin 2.2.0 source
	{
	        PurpleCipherContext *context;                                                                                                                                            
		                                                                                                     
		context = purple_cipher_context_new_by_name("sha1", NULL);                                   
		if (context == NULL)  {                                                                                            
			purple_debug_error("util", "Could not find sha1 cipher\n");                          
			g_return_val_if_reached(NULL);                                                       
		}	                                                                                            
										                                                                                                     
	                                                                    
		purple_cipher_context_append(context, input, strlen(input));                                
		if (!purple_cipher_context_digest_to_str(context, 41, digest, NULL)) {                                                                                            
	    		purple_debug_error("util", "Failed to get SHA-1 digest.\n");                         
	    		g_return_val_if_reached(NULL);                                                       
		}                                                                                            
		purple_cipher_context_destroy(context);                                                      
		digest[40]=0;
	}
	

int gfire_send_auth(PurpleConnection *gc, int packet_len, int packet_id)
{

		

	char *passwd = (char *)purple_account_get_password(gc->account);
	char *name = (char *)purple_account_get_username(gc->account);
	char salt[41]; 					/*the salt we got from the server*/
	gfire_data *gfire = (gfire_data *)gc->proto_data;
	char secret[] = "UltimateArena";		 /*Secret string that is used to hash the passwd*/	
	char sha_string[41];
	char hash_it[100];
	char hash_final[81];
	int index = 0;

	/*
	 * packet_length 00 type(01) 00 numberOfAtts
	 * attribute_length 'name'  usernameLength_length usernameLength 00 username
	 * attribute_length 'password'  passwdLength_length passwdLength 00 cryptedPassword
	 */

	/* extract the salt from the packet and add a null terminator */
	memcpy(salt,gfire->buff_in+13, 40);
	salt[40]=0x00;
	
	int pkt_len = 97+strlen(name); /*Packet length is 97 + username length*/ 
	
	memset(gfire->buff_out,0x00,GFIRE_BUFFOUT_SIZE);
	gfire_add_header(gfire->buff_out, pkt_len, 1, 3); /*add header*/ 
	index += 5;
	
	index = gfire_add_att_name(gfire->buff_out,index, "name");/*add name*/
	gfire->buff_out[index++] = 0x01; 			/*username length length*/
	gfire->buff_out[index++] = (char)strlen(name); 	/*username length*/
	gfire->buff_out[index++] = 0x00;
	
	memcpy(gfire->buff_out+index, name, strlen(name)); 	/* add username */
	index += strlen(name);
	
	index = gfire_add_att_name(gfire->buff_out,index, "password");
	gfire->buff_out[index++] = 0x01; 			/*hashed passwd length length*/
	gfire->buff_out[index++] = 0x28; 			/*hashed passwd length, always 40 (SHA1)*/
	gfire->buff_out[index++] = 0x00;
	
	hash_it[0] = 0;
	strcat(hash_it,name);				/*create string: name+passwd+secret*/
	strcat(hash_it,passwd);
	strcat(hash_it,secret);

	hashSha1(hash_it,hash_final);
	memcpy(hash_final+40,salt,40);			/* mix it with the salt and rehash*/
	
	hash_final[80] = 0x00; 				/*terminate the string*/

	hashSha1(hash_final,sha_string);
			
	memcpy(gfire->buff_out+index,sha_string,strlen(sha_string));/*insert the hash of the passwd*/
	index += strlen(sha_string);
	
	/* added 09-08-2005 difference in login packet */ 
	index = gfire_add_att_name(gfire->buff_out,index, "flags");/*add flags*/ 
	gfire->buff_out[index++]=0x02; 
	
	// run memset once, fill 25 char's with 0's this is from a packet capture 
	// they tack on "flags" + 4 bytes that are 0x00 + "sid" + 16 bytes that are 0x00 
	
	index+=4; 
	index = gfire_add_att_name(gfire->buff_out,index, "sid");/*add sid*/ 
	gfire->buff_out[index++] = 0x03; 
	
	// rest of packet is 16 bytes filled with 0x00 
	index+= 16; 
	
	return index;
}


/* reads session information from intial login grabs our info */
void gfire_packet_130(PurpleConnection *gc, int packet_len)
{
	gfire_data *gfire = NULL;
	int index = XFIRE_HEADER_LEN + 1;
	char tmp[100] = "";
	guint16 slen = 0;


	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;
	memcpy(tmp, gfire->buff_in + index, strlen("userid"));
	tmp[strlen("userid")] = 0x00;
	index += strlen("userid") + 1;
	if (0 == g_ascii_strcasecmp("userid", tmp)) {
		if (!gfire->userid) g_free(gfire->userid);
		gfire->userid = g_malloc0(XFIRE_USERID_LEN);
		memcpy(gfire->userid, gfire->buff_in + index, XFIRE_USERID_LEN);
		index += XFIRE_USERID_LEN + 1;
	} else {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: pkt 130: userid not in correct position.\n");
		return;
	}
	index += 4;
	if (!gfire->sid) g_free(gfire->sid);
	gfire->sid = g_malloc0(XFIRE_SID_LEN);
	memcpy(gfire->sid, gfire->buff_in + index, XFIRE_SID_LEN);
	index += XFIRE_SID_LEN +6;

	memcpy(&slen, gfire->buff_in + index, sizeof(slen));
	index += sizeof(slen);
	slen = GUINT16_FROM_LE(slen);
	if (NULL != gfire->alias) g_free(gfire->alias);
	gfire->alias = g_malloc0(slen + 1);
	memcpy(gfire->alias, gfire->buff_in + index, slen);
	if (slen > 0) gfire->alias[slen] = 0x00;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(session): Our userid = %02x%02x%02x%02x, Our Alias = %s\n",
			NNA(gfire->userid, gfire->userid[0]), NNA(gfire->userid, gfire->userid[1]),
			NNA(gfire->userid, gfire->userid[2]), NNA(gfire->userid, gfire->userid[3]), NN(gfire->alias) );
// FIXME("gfire_packet_130");
}



/* reads buddy list from server and populates purple blist */
void gfire_packet_131(PurpleConnection *gc, int packet_len)
{
	int index = 0;
	int itmp = 0;
	int i = 0;
	gfire_buddy *gf_buddy = NULL;
	GList *friends = NULL;
	GList *nicks = NULL;
	GList *userids = NULL;
	GList *f,*n,*u;
	gchar uids[(XFIRE_USERID_LEN*2)+1];
	gfire_data *gfire = (gfire_data *)gc->proto_data;

	if (packet_len < 16) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: packet 131 recv'd but way too short?!? %d bytes\n",
			packet_len);
		return;
	}
	index = 6;
	itmp = gfire_read_attrib(&friends, gfire->buff_in + index, packet_len - index, "friends", TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! friends");
		return;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&nicks, gfire->buff_in + index, packet_len - index, "nick", TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! nicks");
		return;
	}	
	index+= itmp + 1;	
	itmp = gfire_read_attrib(&userids, gfire->buff_in + index, packet_len - index, "userid", FALSE, TRUE, 0, 0, 
							XFIRE_USERID_LEN);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! userids");
		return;
	}	

	friends = g_list_first(friends);
	nicks = g_list_first(nicks);
	userids = g_list_first(userids);
	
	f = friends; u = userids; n = nicks;
	while (NULL != f) {
		gf_buddy = g_malloc0(sizeof(gfire_buddy));
		gfire->buddies = g_list_append(gfire->buddies, (gpointer *) gf_buddy);
		gf_buddy->name = (gchar *)f->data;
		gf_buddy->alias = (gchar *)n->data;
		gf_buddy->userid = (guint8 *)u->data;
		if (NULL == gf_buddy->alias) gf_buddy->alias = g_strdup(gf_buddy->name);
		/* make userid into string */
		for(i = 0;i < XFIRE_USERID_LEN; i++) g_sprintf(uids + (i*2), "%02x", gf_buddy->userid[i]);
		uids[(XFIRE_USERID_LEN*2)+1]=0x00;
		gf_buddy->uid_str = g_strdup(uids);
		f->data = NULL;
		u->data = NULL;
		n->data = NULL; /* so we don't free stuff we don't want to */
		f = g_list_next(f);
		u = g_list_next(u);
		n = g_list_next(n);
	}

	g_list_free(friends);
//	g_list_free(userids); /* segfaults if we free this? why? */ //FIXME
	g_list_free(nicks);

	n = gfire->buddies;
	while (n != NULL) {
		gf_buddy = (gfire_buddy *)n->data;
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "buddy info: %s, %s, %02x%02x%02x%02x, %s\n",
					NN(gf_buddy->name), NN(gf_buddy->uid_str), NNA(gf_buddy->userid, gf_buddy->userid[0]),
					NNA(gf_buddy->userid, gf_buddy->userid[1]), NNA(gf_buddy->userid, gf_buddy->userid[2]),
					NNA(gf_buddy->userid, gf_buddy->userid[3]), NN(gf_buddy->alias));
		n = g_list_next(n);
	}
}


GList *gfire_read_buddy_online(PurpleConnection *gc, int packet_len)
{
	guchar tmp[100];
	int index = 0;
	int itmp = 0;
	int i = 0;
	guint8 *str = NULL;
	GList *tmp_list = NULL;
	GList *userids = NULL;
	GList *sids = NULL;
	GList *ret = NULL;
	GList *u,*s;
	gfire_buddy *gf_buddy = NULL;
	gfire_data *gfire = (gfire_data *)gc->proto_data;

	memset((void *)tmp, 0x00, 100);

	if (packet_len < 16) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: packet 132 recv'd but way too short?!? %d bytes\n",
			packet_len);
		return NULL;
	}

	index = 6;
	itmp = gfire_read_attrib(&userids, gfire->buff_in + index, packet_len - index, "userid", FALSE, TRUE, 0, 0, 
							XFIRE_USERID_LEN);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! userids");
		return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&sids, gfire->buff_in + index, packet_len - index, "sid", FALSE, TRUE, 0, 0, 
							XFIRE_SID_LEN);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! sids");
		return NULL;
	}	

	userids = g_list_first(userids);
	sids = g_list_first(sids);
	u = userids; s = sids;

	while ( NULL != u ) {
		tmp_list = gfire_find_buddy_in_list(gfire->buddies, u->data, GFFB_UIDBIN);
		if (NULL == tmp_list) {
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: pkt 132: NULL pointer from buddy find (userid).\n");
//FIXME add cleanup code
			return NULL;
		}
		gf_buddy = (gfire_buddy *)tmp_list->data;
		str = (guint8 *) s->data;
		if (NULL != gf_buddy->sid) g_free(gf_buddy->sid); 
		gf_buddy->sid = str;
		/* covert sid to string */
		for(i = 0;i < XFIRE_SID_LEN;i++) g_sprintf((gchar *)tmp + (i*2), "%02x", str[i]);
		tmp[XFIRE_SID_LEN*2]=0x00;
		if (NULL != gf_buddy->sid_str) g_free(gf_buddy->sid_str); 
		gf_buddy->sid_str = g_strdup((gchar *)tmp);
		/* clear away state, xfire presence state isn't persistent (unlike purple) */
		gf_buddy->away = FALSE;
		if (NULL != gf_buddy->away_msg) g_free(gf_buddy->away_msg);
		gf_buddy->away_msg = NULL;
		ret = g_list_append(ret, (gpointer *)gf_buddy);
		g_free(u->data); s->data = NULL; u->data = NULL;
		u = g_list_next(u); s = g_list_next(s);

		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(on/offline): got info for %s -> %s\n", NN(gf_buddy->name),
					NN(gf_buddy->sid_str));

	}

	g_list_free(userids);
	g_list_free(sids);
	return ret;
}



int gfire_get_im(PurpleConnection *gc, int packet_len)
{
	guint8 sid[16], peermsg;
	gchar tmp[100] = "";
	int index,i = 0;
	guint16 ml = 0;
	guint32 msgtype,imindex = 0;
	gfire_im *im = NULL;
	gfire_data *gfire = (gfire_data*)gc->proto_data;

	if (packet_len < 16) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: packet 133 recv'd but way too short?!? %d bytes\n",
			packet_len);
		return 0;
	}

	index = 6;
	/* check the packet for sid signature */
	memcpy(tmp, gfire->buff_in + index, strlen("sid"));
	tmp[strlen("sid")]=0x00;
	if ( 0 == g_ascii_strcasecmp("sid",(char *)tmp)) {
		index+= strlen("sid") + 1;
		memcpy(&sid, gfire->buff_in + index, XFIRE_SID_LEN);
		index+= XFIRE_SID_LEN + 1;
	} else {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: pkt 133: sid signature isn't in the correct position.\n");
		return 0;
	} 

	/* get peer message */
	memcpy(tmp, gfire->buff_in + index, strlen("peermsg"));
	tmp[strlen("peermsg")]=0x00;
	if ( 0 == g_ascii_strcasecmp("peermsg",(char *)tmp)) {
		index+= strlen("peermsg") + 1;
		memcpy(&peermsg, gfire->buff_in + index, sizeof(peermsg));
		index+= 2;
	} else {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: pkt 133: peermsg signature isn't in the correct position.\n");
		return 0;
	} 
	
	/* get messsage type */
	memcpy(tmp, gfire->buff_in + index, strlen("msgtype"));
	tmp[strlen("msgtype")]=0x00;
	if ( 0 == g_ascii_strcasecmp("msgtype",(char *)tmp)) {
		index+= strlen("msgtype") + 1;
		memcpy(&msgtype, gfire->buff_in + index, sizeof(msgtype));
		index+= sizeof(msgtype) + 1;
		msgtype = GUINT32_FROM_LE(msgtype);
	} else {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: pkt 133: msgtype signature isn't in the correct position.\n");
		return 0;
	} 

	switch (msgtype)
	{
		case 0:
			/* got an im */
			/* get imindex */
			memcpy(tmp, gfire->buff_in + index, strlen("imindex"));
			tmp[strlen("imindex")]=0x00;
			if ( 0 == g_ascii_strcasecmp("imindex",(char *)tmp)) {
				index+= strlen("imindex") + 1;
				memcpy(&imindex, gfire->buff_in + index, sizeof(imindex));
				index+= sizeof(imindex);
				imindex = GUINT32_FROM_LE(imindex);
			} else {
				purple_debug(PURPLE_DEBUG_MISC, "gfire",
					"ERROR: pkt 133: imindex signature isn't in the correct position.\n");
				return 0;
			} 
			index++;
			if (index > packet_len) {
				purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: IM packet says IM but packet to short.\n");
				return 0;
			}
			/* get im */
			memcpy(tmp, gfire->buff_in + index, strlen("im"));
			tmp[strlen("im")]=0x00;
			if ( 0 == g_ascii_strcasecmp("im",(char *)tmp)) {
				index+= strlen("im") + 1;
				/* get im length */
				memcpy(&ml, gfire->buff_in + index, sizeof(ml));
				index+= sizeof(ml);
				ml = GUINT16_FROM_LE(ml);
				purple_debug(PURPLE_DEBUG_MISC, "gfire", "IM: im length = %d\n", ml);
				/* get im string */
				gchar *im_str = g_malloc0(sizeof(gchar) * ml+1);
				memcpy(im_str, gfire->buff_in + index, ml);
				im_str[ml] = 0x00;
				purple_debug(PURPLE_DEBUG_MISC, "gfire", "IM:(recvd): %s\n", NN(im_str));

				im = g_malloc0(sizeof(gfire_im));
				gfire->im = im;
				im->type = msgtype;
				im->peer = peermsg;
				im->index = imindex;
				im->im_str = im_str;
				/* covert sid to string */
				for(i = 0;i < XFIRE_SID_LEN;i++) g_sprintf((gchar *)tmp + (i*2), "%02x", sid[i]);
				tmp[XFIRE_SID_LEN*2]=0x00;
				im->sid_str = g_strdup(tmp);

				/* make response packet */
				memcpy(gfire->buff_out, gfire->buff_in, packet_len);
				gfire_add_header(gfire->buff_out, 62, 2, 2);
				gfire->buff_out[35] = 0x02;
				gfire->buff_out[45] = 0x01;
				return 62;
			} else {
				purple_debug(PURPLE_DEBUG_MISC, "gfire",
					"ERROR: pkt 133: im signature isn't in the correct position.\n");
				return 0;
			} 
			return 0;
		break;

		case 1:
			/* got an ack packet from a previous IM sent */
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "IM ack packet recieved.\n");
			return 0;
		break;

		case 2:
			/* got typing from other user */
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "(im): Got typing info.\n");
			im = g_malloc0(sizeof(gfire_im));
			gfire->im = im;
			im->type = msgtype;
			im->peer = peermsg;
			im->index = 0;
			im->im_str = NULL;
			/* covert sid to string */
			for(i = 0;i < XFIRE_SID_LEN;i++) g_sprintf((gchar *)tmp + (i*2), "%02x", sid[i]);
			tmp[XFIRE_SID_LEN*2]=0x00;
			im->sid_str = g_strdup(tmp);
			return 0;
		break;

		default:
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "unknown IM msgtype = %d.\n", msgtype);
			return 0;
	}	
	return 0;
}


int gfire_create_im(PurpleConnection *gc, gfire_buddy *buddy, const char *msg)
{
	int length = 68+strlen(msg);
	int index = 0;
	gfire_data *gfire = NULL;
	guint32	msgtype = 0;
	guint32 imindex = 0;
	guint16 slen = strlen(msg);

	buddy->im++;
	imindex = GUINT32_TO_LE(buddy->im);
	msgtype = GUINT32_TO_LE(msgtype);
	slen = GUINT16_TO_LE(slen);
	gfire = (gfire_data *)gc->proto_data;	
	gfire_add_header(gfire->buff_out, length, 2, 2);/*add header*/
	index += 5;	

	gfire->buff_out[index] = strlen("sid"); index++;
	memcpy(gfire->buff_out + index, "sid", strlen("sid"));
	index+= strlen("sid");
	gfire->buff_out[index++] = 0x03;
	memcpy(gfire->buff_out+index,buddy->sid, XFIRE_SID_LEN);
	index += XFIRE_SID_LEN;

	gfire->buff_out[index] = strlen("peermsg"); index++;
	memcpy(gfire->buff_out + index, "peermsg", strlen("peermsg"));
	index+= strlen("peermsg");
	gfire->buff_out[index++] = 0x05;
	gfire->buff_out[index++] = 0x03;

	gfire->buff_out[index] = strlen("msgtype"); index++;
	memcpy(gfire->buff_out + index, "msgtype", strlen("msgtype"));
	index+= strlen("msgtype");
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, &msgtype, sizeof(msgtype));
	index+= sizeof(msgtype);
	
	gfire->buff_out[index] = strlen("imindex"); index++;
	memcpy(gfire->buff_out + index, "imindex", strlen("imindex"));
	index+= strlen("imindex");
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, &imindex, sizeof(imindex));
	index+= sizeof(imindex);

	gfire->buff_out[index] = strlen("im"); index++;
	memcpy(gfire->buff_out + index, "im", strlen("im"));
	index+= strlen("im");
	gfire->buff_out[index++] = 0x01;
	memcpy(gfire->buff_out + index, &slen, sizeof(slen));
	index+= sizeof(slen);
	
	memcpy(gfire->buff_out+index, msg, strlen(msg));
	index += strlen(msg);

	return index;

}


GList *gfire_game_status(PurpleConnection *gc, int packet_len)
{
	int index = XFIRE_HEADER_LEN + 1;
	int itmp = 0;
	GList *btmp = NULL;
	gfire_buddy *gf_buddy = NULL;
	GList *ret = NULL;
	GList *sids = NULL;
	GList *gameids = NULL;
	GList *gameips = NULL;
	GList *gameports = NULL;
	GList *s, *g, *ip, *gp; 
	gfire_data *gfire = (gfire_data *)gc->proto_data;

	itmp = gfire_read_attrib(&sids, gfire->buff_in + index, packet_len - index, "sid", FALSE, TRUE, 0, 0, 
							XFIRE_SID_LEN);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! sids");
		return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&gameids, gfire->buff_in + index, packet_len - index, "gameid", FALSE, TRUE, 0, 0, 
							XFIRE_GAMEID_LEN);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! gameid");
		return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&gameips, gfire->buff_in + index, packet_len - index, "gip", FALSE, TRUE, 0, 0, 
							XFIRE_GAMEIP_LEN);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! gip");
		return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&gameports, gfire->buff_in + index, packet_len - index, "gport", FALSE, TRUE, 0, 0, 
							XFIRE_GAMEPORT_LEN);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! gport");
		return NULL;
	}	
	gameids = g_list_first(gameids); sids = g_list_first(sids); gameips = g_list_first(gameips);
	gameports = g_list_first(gameports);
	g = gameids; s = sids; ip = gameips; gp = gameports;
	
	while ( NULL != s ){
		btmp = gfire_find_buddy_in_list(gfire->buddies, s->data, GFFB_SIDBIN);
		if (NULL == btmp) {
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "WARN: pkt 135: (gameinfo) could not find sid in buddy list.\n");
		} else {
			gf_buddy = (gfire_buddy *)btmp->data;
			memcpy(&(gf_buddy->gameid),g->data, XFIRE_GAMEID_LEN);
			gf_buddy->gameid = GUINT32_FROM_LE(gf_buddy->gameid);
			memcpy(&(gf_buddy->gameport),gp->data, XFIRE_GAMEPORT_LEN);
			gf_buddy->gameport = GUINT32_FROM_LE(gf_buddy->gameport);
			gf_buddy->gameip = (guint8 *)ip->data;
			ret = g_list_append(ret, (gpointer *)gf_buddy);

			purple_debug(PURPLE_DEBUG_MISC, "gfire", "(gameinfo): %s, is playing %d on %d.%d.%d.%d:%d\n",
						NN(gf_buddy->name), gf_buddy->gameid, NNA(gf_buddy->gameip, gf_buddy->gameip[3]),
						NNA(gf_buddy->gameip, gf_buddy->gameip[2]), NNA(gf_buddy->gameip, gf_buddy->gameip[1]),
						NNA(gf_buddy->gameip, gf_buddy->gameip[0]), gf_buddy->gameport);

		}
		g_free(s->data); g_free(g->data); g_free(gp->data);
		s->data = g->data = gp->data = NULL;
		s = g_list_next(s); g = g_list_next(g); ip = g_list_next(ip); gp = g_list_next(gp);
	}
	
	g_list_free(gameids);
	g_list_free(gameports);
	g_list_free(sids);
	g_list_free(gameips);

	return ret;
}


/*send keep alive packet to the server*/
int gfire_ka_packet_create(PurpleConnection *gc)
{
	gfire_data *gfire = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return 0;

	int index = 0;
	gfire_add_header(gfire->buff_out + index, 26, 13, 2);
	index += 5;
	
	index = gfire_add_att_name(gfire->buff_out,index, "value");
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	
	index = gfire_add_att_name(gfire->buff_out,index, "stats");
	gfire->buff_out[index++] = 0x04;
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;
	return index;
}

GList *gfire_read_buddy_status(PurpleConnection *gc, int packet_len)
{
	int index = XFIRE_HEADER_LEN + 1;
	int itmp = 0;
	GList *btmp = NULL;
	gfire_buddy *gf_buddy = NULL;
	GList *ret = NULL;
	GList *sids = NULL;
	GList *msgs = NULL;
	GList *s, *m; 
	gfire_data *gfire = (gfire_data *)gc->proto_data;

	itmp = gfire_read_attrib(&sids, gfire->buff_in + index, packet_len - index, "sid", FALSE, TRUE, 0, 0, 
							XFIRE_SID_LEN);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! sids");
		return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&msgs, gfire->buff_in + index, packet_len - index, "msg", TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! msgs");
		return NULL;
	}	

	msgs = g_list_first(msgs); sids = g_list_first(sids);
	m = msgs; s = sids;
	
	while ( NULL != s ){
		btmp = gfire_find_buddy_in_list(gfire->buddies, s->data, GFFB_SIDBIN);
		if (NULL == btmp) {
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "WARN: pkt 154: (away status) could not find sid in buddy list.\n");
		} else {
			gf_buddy = (gfire_buddy *)btmp->data;
			if (NULL != m->data) {
				/* got an away message */
				gf_buddy->away = TRUE;
				gf_buddy->away_msg = m->data;
			} else {
				/* no message, user is back */
				gf_buddy->away = FALSE;
				if (NULL != gf_buddy->away_msg) g_free(gf_buddy->away_msg);
				gf_buddy->away_msg = NULL;
			}
			ret = g_list_append(ret, (gpointer *)gf_buddy);

			purple_debug(PURPLE_DEBUG_MISC, "gfire","(away): %s, is away/back with msg %s\n",
						NN(gf_buddy->name), NN(gf_buddy->away_msg));
		}
		g_free(s->data);
		s->data = NULL;
		s = g_list_next(s); m = g_list_next(m);
	}
	g_list_free(msgs);
	g_list_free(sids);
	return ret;
}


GList *gfire_read_invitation(PurpleConnection *gc, int packet_len)
{
	int index = XFIRE_HEADER_LEN + 1;
	int itmp = 0;
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	GList *names = NULL;
	GList *nicks = NULL;
	GList *msgs = NULL;
	GList *ret = NULL;
	GList *n, *a, *m;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data))
		return NULL;

	itmp = gfire_read_attrib(&names, gfire->buff_in + index, packet_len - index, "name", TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! msgs");
		return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&nicks, gfire->buff_in + index, packet_len - index, "nick", TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! msgs");
		return NULL;
	}	
	index += itmp + 1;
	itmp = gfire_read_attrib(&msgs, gfire->buff_in + index, packet_len - index, "msg", TRUE, FALSE, 0, 0, 0);
	if (itmp < 1 ) {
//FIXME add mem cleanup code
FIXME("gfire_read_attrib returned < 1! msgs");
		return NULL;
	}	

	names = g_list_first(names); nicks = g_list_first(nicks); msgs = g_list_first(msgs);
	n = names; a = nicks; m = msgs;
	
	while (NULL != n){
		gf_buddy = g_malloc0(sizeof(gfire_buddy));
		ret = g_list_append(ret, gf_buddy);
		gf_buddy->name = (char *)n->data;
		gf_buddy->alias = (char *)a->data;
		gf_buddy->uid_str = (char *)m->data; /* yeah its ugly but it'll work for this */
		n->data = a->data = m->data = NULL;
		n = g_list_next(n); a = g_list_next(a); m = g_list_next(m);
	}
	g_list_free(names);
	g_list_free(nicks);
	g_list_free(msgs);
	return ret;
}


int gfire_invitation_deny(PurpleConnection *gc, char *name)
{
	gfire_data *gfire = NULL;
	int len = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !name) return 0;


	/* same as the invitation except packet ID in the header */
	len = gfire_invitation_accept(gc, name);
	gfire_add_header(gfire->buff_out, len, 8, 1);
	return len;
}


int gfire_invitation_accept(PurpleConnection *gc, char *name)
{
	gfire_data *gfire = NULL;
	int index = XFIRE_HEADER_LEN;
	guint16 slen = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !name) return 0;

	slen = strlen(name);
	slen = GUINT16_TO_LE(slen);
	gfire->buff_out[index++] = strlen("name");
	memcpy(gfire->buff_out + index, "name", strlen("name"));
	index += strlen("name");
	gfire->buff_out[index++] = 0x01;
	memcpy(gfire->buff_out + index, &slen, sizeof(slen));
	index += sizeof(slen);
	memcpy(gfire->buff_out + index, name, strlen(name));
	index += strlen(name);
	
	gfire_add_header(gfire->buff_out, index, 7, 1);
	return index;
}


int gfire_add_buddy_create(PurpleConnection *gc, char *name)
{
	gfire_data *gfire = NULL;
	int index = XFIRE_HEADER_LEN;
	guint16 slen = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !name) return 0;

	slen = strlen(name);
	slen = GUINT16_TO_LE(slen);
	gfire->buff_out[index++] = strlen("name");
	memcpy(gfire->buff_out + index, "name", strlen("name"));
	index += strlen("name");
	gfire->buff_out[index++] = 0x01;
	memcpy(gfire->buff_out + index, &slen, sizeof(slen));
	index += sizeof(slen);
	memcpy(gfire->buff_out + index, name, strlen(name));
	index += strlen(name);
		
	gfire->buff_out[index++] = strlen("msg");
	memcpy(gfire->buff_out + index, "msg", strlen("msg"));
	index += strlen("msg");
	gfire->buff_out[index++] = 0x01;
	gfire->buff_out[index++] = 0x00;
	gfire->buff_out[index++] = 0x00;

	gfire_add_header(gfire->buff_out, index, 6, 2);

	return index;

}


int gfire_remove_buddy_create(PurpleConnection *gc, gfire_buddy *b)
{
	gfire_data *gfire = NULL;
	int index = XFIRE_HEADER_LEN;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !b) return 0;

	gfire_add_header(gfire->buff_out, 17, 9, 1);
	gfire->buff_out[index++] = strlen("userid");
	memcpy(gfire->buff_out + index, "userid", strlen("userid"));
	index += strlen("userid");
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, b->userid, 4);
	index += 4;

	return index;
}


/*
*Sends a nickname change to the server
*/
int gfire_create_change_alias(PurpleConnection *gc, char *name)
{
	gfire_data *gfire = NULL;
	int index = XFIRE_HEADER_LEN;
	guint16 slen = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return 0;

	if (! name ) name = "";
	slen = strlen(name);
	slen = GUINT16_TO_LE(slen);
	gfire->buff_out[index++] = strlen("nick");
	memcpy(gfire->buff_out + index, "nick", strlen("nick"));
	index += strlen("nick");
	gfire->buff_out[index++] = 0x01;
	memcpy(gfire->buff_out + index, &slen, sizeof(slen));
	index += sizeof(slen);
	memcpy(gfire->buff_out + index, name, strlen(name));
	index += strlen(name);
	gfire_add_header(gfire->buff_out, index, 14, 1);
	return index;	
}

/*
*Sends the packet when we join a game or leave it (gameid 00 00)
*/
int gfire_join_game_create(PurpleConnection *gc, int game, int port, const char *ip)
{
	int index = XFIRE_HEADER_LEN;
	gfire_data *gfire = NULL;
	guint32 gport = port;
	guint32 gameid = game;
	const char nullip[4] = {0x00, 0x00, 0x00, 0x00};
	
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return 0;
	if (!ip) ip = (char *)&nullip;

	gport = GUINT32_TO_LE(gport);
	gameid = GUINT32_TO_LE(gameid);

	gfire->buff_out[index++] = strlen("gameid");
	memcpy(gfire->buff_out + index, "gameid", strlen("gameid"));
	index += strlen("gameid");
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, &gameid, sizeof(gameid));
	index += sizeof(gameid);	

	gfire->buff_out[index++] = strlen("gip");
	memcpy(gfire->buff_out + index, "gip", strlen("gip"));
	index += strlen("gip");
	gfire->buff_out[index++] = 0x02;
	gfire->buff_out[index++] = ip[0];
	gfire->buff_out[index++] = ip[1];
	gfire->buff_out[index++] = ip[2];
	gfire->buff_out[index++] = ip[3];

	gfire->buff_out[index++] = strlen("gport");
	memcpy(gfire->buff_out + index, "gport", strlen("gport"));
	index += strlen("gport");
	gfire->buff_out[index++] = 0x02;
	memcpy(gfire->buff_out + index, &gport, sizeof(gport));
	index += sizeof(gport);

	gfire_add_header(gfire->buff_out, index, 4, 3);

	return index;
}

