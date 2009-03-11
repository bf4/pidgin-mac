/*
 *  config.c
 *  pidgin-macosx
 *
 *  Created by Albert Zeyer on 10.03.09.
 *  code under GPL
 *
 */

#include "config.h"

STATIC_PROTO_INIT
STATIC_PLUGIN_INIT

void static_proto_init(void) {
	purple_init_zephyr_plugin();
	purple_init_yahoo_plugin();
	purple_init_simple_plugin();
	purple_init_icq_plugin();
	purple_init_aim_plugin();
	purple_init_novell_plugin();
	purple_init_msn_plugin();
	purple_init_jabber_plugin();
	purple_init_irc_plugin();
}

void static_plugin_init(void) {
	purple_init_joinpart_plugin();
	purple_init_idle_plugin();
	purple_init_tcl_plugin();
	purple_init_ssl_gnutls_plugin();
	purple_init_ssl_plugin();
	purple_init_Autoaccept_plugin__optload(0);
	purple_init_buddynote_plugin();
	purple_init_filectl_plugin(0);
	purple_init_log_reader_plugin();
	purple_init_psychic_plugin();
	purple_init_spellcheck_plugin();
	purple_init_offlinemsg_plugin();
	purple_init_Markerline_plugin__optload(0);
	purple_init_raw_plugin__optload(0);
	purple_init_ConversationColors_plugin();
	purple_init_sendbutton_plugin__optload(0);
	purple_init_notify_plugin();
	purple_init_statenotify_plugin();
	purple_init_cap_plugin();
	purple_init_ticker_plugin__optload(0);
	purple_init_history_plugin();
	purple_init_contactpriority_plugin();
	purple_init_extplacement_plugin__optload(0);
	purple_init_gtkbuddynote_plugin();
	purple_init_iconaway_plugin__optload(0);
	purple_init_relnot_plugin__optload(0);
	purple_init_timestamp_format_plugin();
	//purple_init_interval_plugin__optload(0);
	purple_init_timestamp_plugin__optload(0);
	
	purple_init_otr_plugin();
	purple_init_facebook_plugin();
	purple_init_gfire_plugin();
}

