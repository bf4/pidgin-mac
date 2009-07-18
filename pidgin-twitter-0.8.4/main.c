/*
 * Pidgin-Twitter plugin.
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
#define PURPLE_PLUGINS 1

#include "pidgin-twitter.h"

/***********/
/* globals */
/***********/
GRegex *regp[NUM_REGPS];
GHashTable *icon_hash[NUM_SERVICES];
source_t source;
static gboolean suppress_oops = FALSE;
static GHashTable *conv_hash = NULL;
static GList *wassr_parrot_list = NULL;
static GList *identica_parrot_list = NULL;
static GList *ffeed_parrot_list = NULL;
#ifdef _WIN32
gboolean blink_state = FALSE;
gboolean blink_modified = FALSE;
#endif

/**************/
/* prototypes */
/**************/
static void cleanup_hash_entry_func(gpointer key, gpointer value, gpointer user_data);
static void init_plugin(PurplePlugin *plugin);
static gboolean load_plugin(PurplePlugin *plugin);
static gboolean unload_plugin(PurplePlugin *plugin);
static gboolean sending_im_cb(PurpleAccount *account, char *recipient, char **buffer, void *data);
static gboolean eval(const GMatchInfo *match_info, GString *result, gpointer user_data);
static void translate(gchar **str, gint which, gint service);
static void playsound(gchar **str, gint which);
static gboolean writing_im_cb(PurpleAccount *account, char *sender, char **buffer, PurpleConversation *conv, int flags, void *data);
static void insert_text_cb(GtkTextBuffer *textbuffer, GtkTextIter *position, gchar *new_text, gint new_text_length, gpointer user_data);
static void delete_text_cb(GtkTextBuffer *textbuffer, GtkTextIter *start_pos, GtkTextIter *end_pos, gpointer user_data);
static void detach_from_conv(PurpleConversation *conv, gpointer null);
static void delete_requested_icon_marks(PidginConversation *gtkconv, GHashTable *table);
static void attach_to_conv(PurpleConversation *conv, gpointer null);
static void conv_created_cb(PurpleConversation *conv, gpointer null);
static void deleting_conv_cb(PurpleConversation *conv);
static gboolean receiving_im_cb(PurpleAccount *account, char **sender, char **buffer, PurpleConversation *conv, PurpleMessageFlags *flags, void *data);
static void remove_marks_func(gpointer key, gpointer value, gpointer user_data);
static void cancel_fetch_func(gpointer key, gpointer value, gpointer user_data);
static gboolean displaying_im_cb(PurpleAccount *account, const char *who, char **message, PurpleConversation *conv, PurpleMessageFlags flags, void *data);
static void displayed_im_cb(PurpleAccount *account, const char *who, char *message, PurpleConversation *conv, PurpleMessageFlags flags);
#ifndef _WIN32
//extern gchar *sanitize_utf(const gchar *msg, gsize len, gsize *newlen) __attribute__ ((weak));
#endif


/*************/
/* functions */
/*************/
static gboolean
sending_im_cb(PurpleAccount *account, char *recipient, char **buffer,
              void *data)
{
    int utflen, bytes;
    int service = get_service_type_by_account(account, recipient);
    gchar *tmp, *plain;
    gchar *ffeed_tmp;
    gsize dummy;

    twitter_debug("called\n");

    if(service == unknown_service)
        return FALSE;

    /* strip all markups */
    tmp = strip_html_markup(*buffer);

#ifndef _WIN32
    /*if(sanitize_utf) {
        plain = sanitize_utf(tmp, -1, &dummy);
        g_free(tmp);
    }
    else*/
#endif
        plain = tmp;

    switch(service) {
    case wassr_service:
        /* store sending message to address parrot problem */
        wassr_parrot_list =
            g_list_prepend(wassr_parrot_list, g_strdup(plain));
        twitter_debug("wassr parrot pushed:%s\n", plain);
        break;
    case identica_service:
        /* store sending message to address parrot problem */
        identica_parrot_list =
            g_list_prepend(identica_parrot_list, g_strdup(plain));
        twitter_debug("identica parrot pushed:%s\n", plain);
        break;
    case ffeed_service:
        /* store sending message to address parrot problem */
        ffeed_parrot_list =
            g_list_prepend(ffeed_parrot_list, g_strdup(plain));
        twitter_debug("ffeed parrot pushed:%s\n", plain);

        /* prepend @me */
        ffeed_tmp = g_strdup_printf("@me %s", plain);
        g_free(plain);
        plain = ffeed_tmp;
        break;
    default:
        break;
    }

    g_free(*buffer);
    *buffer = plain;

    /* return here if the message is not to twitter */
    if(service != twitter_service)
        return FALSE;

    /* escape pseudo command */
    if(service == twitter_service &&
       purple_prefs_get_bool(OPT_ESCAPE_PSEUDO)) {
        escape(buffer);
    }

    /* update status with Twitter API instead of IM protocol */
    if (purple_prefs_get_bool(OPT_API_BASE_POST)) {
        if (buffer && *buffer) {
            post_status_with_api(account, buffer);
            (*buffer)[0] = '\0';
        }
        return FALSE;
    }

    /* try to suppress oops message */
    utflen = g_utf8_strlen(*buffer, -1);
    bytes = strlen(*buffer);
    twitter_debug("utflen = %d bytes = %d\n", utflen, bytes);
    if(bytes > 140 && utflen <= 140)
        suppress_oops = TRUE;

    return FALSE;
}

static gboolean
eval(const GMatchInfo *match_info, GString *result, gpointer user_data)
{
    eval_data *data = (eval_data *)user_data;
    gint which = data->which;
    gint service = data->service;
    gchar sub[SUBST_BUF_SIZE];

    twitter_debug("which = %d service = %d\n", which, service);

    if(which == RECIPIENT) {
        gchar *match1 = g_match_info_fetch(match_info, 1); /* preceding \s */
        gchar *match2 = g_match_info_fetch(match_info, 2); /* recipient */
        const gchar *format = NULL;

        switch(service) {
        case twitter_service:
            format = RECIPIENT_FORMAT_TWITTER;
            break;
        case wassr_service:
            format = RECIPIENT_FORMAT_WASSR;
            break;
        case identica_service:
            format = RECIPIENT_FORMAT_IDENTICA;
            break;
        case jisko_service:
            format = RECIPIENT_FORMAT_JISKO;
            break;
        case ffeed_service:
            format = RECIPIENT_FORMAT_FFEED;
            break;
        default:
            twitter_debug("unknown service\n");
            break;
        }
        g_snprintf(sub, SUBST_BUF_SIZE, format, match1 ? match1: "", match2, match2);
        g_free(match1);
        g_free(match2);
    }
    else if(which == SENDER) {
        gchar *match1 = g_match_info_fetch(match_info, 1); /* preceding CR|LF */
        gchar *match2 = g_match_info_fetch(match_info, 2); /* sender */
        const gchar *format = NULL;

        switch(service) {
        case twitter_service:
            format = SENDER_FORMAT_TWITTER;
            break;
        case wassr_service:
            format = SENDER_FORMAT_WASSR;
            break;
        case identica_service:
            format = SENDER_FORMAT_IDENTICA;
            break;
        case jisko_service:
            format = SENDER_FORMAT_JISKO;
            break;
        default:
            twitter_debug("unknown service\n");
            break;
        }

        g_snprintf(sub, SUBST_BUF_SIZE, format, match1 ? match1: "", match2, match2);

        g_free(match1);
        g_free(match2);
    }
    else if(which == SENDER_FFEED) {
        gchar *match1 = g_match_info_fetch(match_info, 1); /* preceding CR|LF */
        gchar *match2 = g_match_info_fetch(match_info, 2); /* sender */
        const gchar *format = NULL;

        format = SENDER_FORMAT_FFEED;
        g_snprintf(sub, SUBST_BUF_SIZE, format, match1 ? match1: "", match2, match2);

        g_free(match1);
        g_free(match2);
    }
    else if(which == CHANNEL_WASSR && service == wassr_service) {
        gchar *match1 = g_match_info_fetch(match_info, 1); /* before channel */
        gchar *match2 = g_match_info_fetch(match_info, 2); /* channel */
        const gchar *format = CHANNEL_FORMAT_WASSR;

        g_snprintf(sub, SUBST_BUF_SIZE, format, match1 ? match1: "", match2, match2);

        g_free(match1);
        g_free(match2);
    }
    else if(which == TAG_TWITTER && service == twitter_service) {
        gchar *match1 = g_match_info_fetch(match_info, 1); /* white space */
        gchar *match2 = g_match_info_fetch(match_info, 2); /* search tag */
        const gchar *format = TAG_FORMAT_TWITTER;
        g_snprintf(sub, SUBST_BUF_SIZE, format, match1 ? match1: "", match2, match2);
        g_free(match1);
        g_free(match2);
    }
    else if(which == TAG_IDENTICA && service == identica_service) {
        gchar *match = g_match_info_fetch(match_info, 1);
        gchar *link = g_ascii_strdown(match, -1);
        purple_str_strip_char(link, '-');
        purple_str_strip_char(link, '_');
        g_snprintf(sub, SUBST_BUF_SIZE, TAG_FORMAT_IDENTICA, link, match);
        g_free(match);
        g_free(link);
    }
    else if(which == GROUP_IDENTICA && service == identica_service) {
        gchar *match = g_match_info_fetch(match_info, 1);
        gchar *link = g_ascii_strdown(match, -1);
        purple_str_strip_char(link, '-');
        purple_str_strip_char(link, '_');
        g_snprintf(sub, SUBST_BUF_SIZE, GROUP_FORMAT_IDENTICA, link, match);
        g_free(match);
        g_free(link);
    }
    else if(which == EXCESS_LF) {
        g_snprintf(sub, SUBST_BUF_SIZE, "%s", "\n");
    }
    else if(which == TRAIL_HASH) {
        g_snprintf(sub, SUBST_BUF_SIZE, "%s", ""); /* xxx --yaz */
    }

    g_string_append(result, sub);
    twitter_debug("sub = %s\n", sub);

    return FALSE;
}

static void
translate(gchar **str, gint regp_id, gint service)
{
    gchar *newstr;
    eval_data *data = g_new0(eval_data, 1);

    data->which = regp_id;
    data->service = service;

    newstr = g_regex_replace_eval(regp[regp_id],  /* compiled regex */
                                  *str,  /* subject string */
                                  -1,    /* length of the subject string */
                                  0,     /* start position */
                                  0,     /* match options */
                                  eval,  /* function to be called for each match */
                                  data,  /* user data */
                                  NULL); /* error handler */

    g_free(data); data = NULL;

    twitter_debug("which = %d *str = %s newstr = %s\n", regp_id, *str, newstr);

    g_free(*str);
    *str = newstr;
}

static void
playsound(gchar **str, gint which)
{
    GMatchInfo *match_info;
    const gchar *list = NULL;
    gchar **candidates = NULL, **candidate = NULL;

    list = purple_prefs_get_string(which ? OPT_USERLIST_SENDER :
                                   OPT_USERLIST_RECIPIENT);
    g_return_if_fail(list != NULL);
    if(strstr(list, DEFAULT_LIST))
        return;

    candidates = g_strsplit_set(list, " ,:;", 0);
    g_return_if_fail(candidates != NULL);

    g_regex_match(regp[which], *str, 0, &match_info);
    while(g_match_info_matches(match_info)) {
        gchar *user = NULL;
        if(which == RECIPIENT)
            user = g_match_info_fetch(match_info, 2);
        else if(which == SENDER)
            user = g_match_info_fetch(match_info, 2);
        twitter_debug("user = %s\n", user);

        for(candidate = candidates; *candidate; candidate++) {
            if(!strcmp(*candidate, ""))
                continue;
            twitter_debug("candidate = %s\n", *candidate);
            if(!strcmp(user, *candidate)) {
                twitter_debug("match. play sound\n");
                purple_sound_play_event(purple_prefs_get_int
                                        (which ? OPT_SOUNDID_SENDER :
                                         OPT_SOUNDID_RECIPIENT), NULL);
                break;
            }
        }
        g_free(user);
        g_match_info_next(match_info, NULL);
    }
    g_strfreev(candidates);
    g_match_info_free(match_info);
}

static gboolean
writing_im_cb(PurpleAccount *account, char *sender, char **buffer,
              PurpleConversation *conv, int flags, void *data)
{
    twitter_debug("called\n");

    gint service = get_service_type(conv);

    /* check if the conversation is between twitter */
    if(service == unknown_service)
        return FALSE;

    /* add screen_name if the current message is posted by myself */
    if (flags & PURPLE_MESSAGE_SEND) {
        gchar *m = NULL;
        const char *screen_name = NULL;

        switch(service) {
        case twitter_service:
            screen_name = purple_prefs_get_string(OPT_SCREEN_NAME_TWITTER);
            break;
        case wassr_service:
            screen_name = purple_prefs_get_string(OPT_SCREEN_NAME_WASSR);
            break;
        case identica_service:
            screen_name = purple_prefs_get_string(OPT_SCREEN_NAME_IDENTICA);
            break;
        case jisko_service:
            screen_name = purple_prefs_get_string(OPT_SCREEN_NAME_JISKO);
            break;
        case ffeed_service:
            screen_name = purple_prefs_get_string(OPT_SCREEN_NAME_FFEED);
            break;
        }

        if (screen_name) {
            m = g_strdup_printf("%s: %s", screen_name, *buffer);
            g_free(*buffer);
            *buffer = m;
        }
    }

    /* strip all markups */
    strip_markup(buffer, TRUE);

    /* playsound */
    if(purple_prefs_get_bool(OPT_PLAYSOUND_SENDER)) {
        playsound(buffer, SENDER);
    }
    if(purple_prefs_get_bool(OPT_PLAYSOUND_RECIPIENT)) {
        playsound(buffer, RECIPIENT);
    }

    /* translate */
    if(purple_prefs_get_bool(OPT_TRANSLATE_SENDER)) {
        if(service == ffeed_service)
            translate(buffer, SENDER_FFEED, service);
        else
            translate(buffer, SENDER, service);
    }
    if(purple_prefs_get_bool(OPT_TRANSLATE_RECIPIENT)) {
        translate(buffer, RECIPIENT, service);
    }
    if(service == wassr_service &&
       purple_prefs_get_bool(OPT_TRANSLATE_CHANNEL)) {
        translate(buffer, CHANNEL_WASSR, service);
    }
    if(service == identica_service &&
       purple_prefs_get_bool(OPT_TRANSLATE_CHANNEL)) {
        translate(buffer, TAG_IDENTICA, service);
    }
    if(service == twitter_service &&
       purple_prefs_get_bool(OPT_TRANSLATE_CHANNEL)) {
        translate(buffer, TAG_TWITTER, service);
    }
    if(service == identica_service &&
       purple_prefs_get_bool(OPT_TRANSLATE_CHANNEL)) {
        translate(buffer, GROUP_IDENTICA, service);
    }

    /* escape pseudo command (to show the same result as sending message) */
    if(service == twitter_service &&
       purple_prefs_get_bool(OPT_ESCAPE_PSEUDO)) {
        escape(buffer);
    }

    if(purple_prefs_get_bool(OPT_STRIP_EXCESS_LF)) {
        translate(buffer, EXCESS_LF, service);
    }

    if(service == ffeed_service) {
        translate(buffer, TRAIL_HASH, service);
    }

    return FALSE;
}

static void
insert_text_cb(GtkTextBuffer *textbuffer, GtkTextIter *position,
               gchar *new_text, gint new_text_length, gpointer user_data)
{
    PurpleConversation *conv = (PurpleConversation *)user_data;
    PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);

    GtkWidget *box, *counter = NULL;
    gchar *markup = NULL;
    gint service = get_service_type(conv);
    guint count;

    g_return_if_fail(gtkconv != NULL);

    switch(service) {
    case twitter_service:
    case identica_service:
    case jisko_service:
    case ffeed_service:
        count = gtk_text_buffer_get_char_count(textbuffer) +
            (unsigned int)g_utf8_strlen(new_text, -1);
        markup = g_markup_printf_escaped("<span color=\"%s\">%u</span>",
                                         count <= 140 ? "black" : "red", count);
        break;
    case wassr_service:
        count = gtk_text_buffer_get_char_count(textbuffer) +
            (unsigned int)g_utf8_strlen(new_text, -1);
        markup = g_markup_printf_escaped("<span color=\"%s\">%u</span>",
                                         count <= 255 ? "black" : "red", count);
        break;
    default:
        twitter_debug("unknown service\n");
        break;
    }

    box = gtkconv->toolbar;
    counter = g_object_get_data(G_OBJECT(box), PLUGIN_ID "-counter");
    if(counter)
        gtk_label_set_markup(GTK_LABEL(counter), markup);

    g_free(markup);
}

static void
delete_text_cb(GtkTextBuffer *textbuffer, GtkTextIter *start_pos,
               GtkTextIter *end_pos, gpointer user_data)
{
    PurpleConversation *conv = (PurpleConversation *)user_data;
    PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
    GtkWidget *box, *counter = NULL;
    gchar *markup = NULL;
    gint service = get_service_type(conv);
    guint count = 0;

    g_return_if_fail(gtkconv != NULL);

    switch(service) {
    case twitter_service:
    case identica_service:
    case jisko_service:
    case ffeed_service:
        count= gtk_text_buffer_get_char_count(textbuffer) -
            (gtk_text_iter_get_offset(end_pos) -
             gtk_text_iter_get_offset(start_pos));
        markup = g_markup_printf_escaped("<span color=\"%s\">%u</span>",
                                         count <= 140 ? "black" : "red", count);
        break;
    case wassr_service:
        count= gtk_text_buffer_get_char_count(textbuffer) -
            (gtk_text_iter_get_offset(end_pos) -
             gtk_text_iter_get_offset(start_pos));
        markup = g_markup_printf_escaped("<span color=\"%s\">%u</span>",
                                         count <= 255 ? "black" : "red", count);
        break;
    default:
        twitter_debug("unknown service\n");
        break;
    }

    box = gtkconv->toolbar;
    counter = g_object_get_data(G_OBJECT(box), PLUGIN_ID "-counter");
    if(counter)
        gtk_label_set_markup(GTK_LABEL(counter), markup);

    g_free(markup);
}

void
detach_from_window(void)
{
    GList *list;

    /* find twitter conv window out and detach from that */
    for(list = pidgin_conv_windows_get_list(); list; list = list->next) {
        PidginWindow *win = list->data;
        PurpleConversation *conv =
            pidgin_conv_window_get_active_conversation(win);
        gint service = get_service_type(conv);
        switch(service) {
        case twitter_service:
        case wassr_service:
        case identica_service:
        case jisko_service:
        case ffeed_service:
            detach_from_conv(conv, NULL);
            break;
        default:
            twitter_debug("unknown service\n");
            break;
        }
    }
}

static void
detach_from_conv(PurpleConversation *conv, gpointer null)
{
    PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
    GtkWidget *box, *counter = NULL, *sep = NULL;

    g_signal_handlers_disconnect_by_func(G_OBJECT(gtkconv->entry_buffer),
                                         (GFunc) insert_text_cb, conv);
    g_signal_handlers_disconnect_by_func(G_OBJECT(gtkconv->entry_buffer),
                                         (GFunc) delete_text_cb, conv);

    box = gtkconv->toolbar;

    /* remove counter */
    counter = g_object_get_data(G_OBJECT(box), PLUGIN_ID "-counter");
    if(counter) {
        gtk_container_remove(GTK_CONTAINER(box), counter);
        g_object_unref(counter);
        g_object_set_data(G_OBJECT(box), PLUGIN_ID "-counter", NULL);
    }

    /* remove separator */
    sep = g_object_get_data(G_OBJECT(box), PLUGIN_ID "-sep");
    if(sep) {
        gtk_container_remove(GTK_CONTAINER(box), sep);
        g_object_unref(sep);
        g_object_set_data(G_OBJECT(box), PLUGIN_ID "-sep", NULL);
    }

    gtk_widget_queue_draw(pidgin_conv_get_window(gtkconv)->window);
}

static void
remove_marks_func(gpointer key, gpointer value, gpointer user_data)
{
    icon_data *data = (icon_data *)value;
    GtkTextBuffer *text_buffer = (GtkTextBuffer *)user_data;
    GList *mark_list = NULL;
    GList *current;

    if(!data)
        return;

    if(data->request_list)
        mark_list = data->request_list;

    /* remove the marks in its GtkTextBuffers */
    current = g_list_first(mark_list);
    while(current) {
        GtkTextMark *current_mark = current->data;
        GtkTextBuffer *current_text_buffer =
            gtk_text_mark_get_buffer(current_mark);
        GList *next;

        next = g_list_next(current);

        if(!current_text_buffer)
            continue;

        if(text_buffer) {
            if(current_text_buffer == text_buffer) {
                /* the mark will be freed in this function */
                gtk_text_buffer_delete_mark(current_text_buffer,
                                            current_mark);
                current->data = NULL;
                mark_list = g_list_delete_link(mark_list, current);
            }
        }
        else {
            gtk_text_buffer_delete_mark(current_text_buffer, current_mark);
            current->data = NULL;
            mark_list = g_list_delete_link(mark_list, current);
        }

        current = next;
    }

    data->request_list = mark_list;
}

static void
delete_requested_icon_marks(PidginConversation *conv, GHashTable *table) {
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(conv->imhtml));

    g_hash_table_foreach(table,
                         (GHFunc)remove_marks_func,
                         (gpointer)text_buffer);
}

void
attach_to_window(void)
{
    GList *list;

    twitter_debug("called\n");

    /* find twitter conv window out and attach to that */
    for(list = pidgin_conv_windows_get_list(); list; list = list->next) {
        PidginWindow *win = list->data;
        PurpleConversation *conv =
            pidgin_conv_window_get_active_conversation(win);
        gint service = get_service_type(conv);
        /* only attach to twitter conversation window */
        switch(service) {
        case twitter_service:
        case wassr_service:
        case identica_service:
        case jisko_service:
        case ffeed_service:
            attach_to_conv(conv, NULL);
            break;
        default:
            twitter_debug("unknown service\n");
            break;
        }
    }
}

static void
attach_to_conv(PurpleConversation *conv, gpointer null)
{
    PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
    GtkWidget *box, *sep, *counter, *menus;
    GtkIMHtml *imhtml;

    box = gtkconv->toolbar;
    imhtml = GTK_IMHTML(gtkconv->imhtml);

    /* Disable widgets that decorate or add link to composing text
     * because Twitter cannot receive marked up string. For lean-view
     * and wide-view, see pidgin/gtkimhtmltoolbar.c.
     */
    menus = g_object_get_data(G_OBJECT(box), "lean-view");
    if(menus) {
        gtk_widget_set_sensitive(GTK_WIDGET(menus), FALSE);
    }
    menus = g_object_get_data(G_OBJECT(box), "wide-view");
    if(menus) {
        gtk_widget_set_sensitive(GTK_WIDGET(menus), FALSE);
    }

    purple_conversation_set_features(
        gtkconv->active_conv,
        purple_conversation_get_features(gtkconv->active_conv) &
        ~PURPLE_CONNECTION_HTML);

    /* check if the counter is enabled */
    if(!purple_prefs_get_bool(OPT_COUNTER))
        return;

    /* get counter object */
    counter = g_object_get_data(G_OBJECT(box), PLUGIN_ID "-counter");
    g_return_if_fail(counter == NULL);

    /* make counter object */
    counter = gtk_label_new(NULL);
    gtk_widget_set_name(counter, "counter_label");
    gtk_label_set_text(GTK_LABEL(counter), "0");
    gtk_box_pack_end(GTK_BOX(box), counter, FALSE, FALSE, 0);
    gtk_widget_show_all(counter);
    g_object_set_data(G_OBJECT(box), PLUGIN_ID "-counter", counter);

    /* make separator object */
    sep = gtk_vseparator_new();
    gtk_box_pack_end(GTK_BOX(box), sep, FALSE, FALSE, 0);
    gtk_widget_show_all(sep);
    g_object_set_data(G_OBJECT(box), PLUGIN_ID "-sep", sep);

    /* connect to signals */
    g_signal_connect(G_OBJECT(gtkconv->entry_buffer), "insert_text",
                     G_CALLBACK(insert_text_cb), conv);
    g_signal_connect(G_OBJECT(gtkconv->entry_buffer), "delete_range",
                     G_CALLBACK(delete_text_cb), conv);

    /* redraw window */
    gtk_widget_queue_draw(pidgin_conv_get_window(gtkconv)->window);
}

static void
conv_created_cb(PurpleConversation *conv, gpointer null)
{
    PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);

    twitter_debug("called\n");

    g_return_if_fail(gtkconv != NULL);

    gint service = get_service_type(conv);
    /* only attach to twitter conversation window */
    switch(service) {
    case twitter_service:
        get_status_with_api((gpointer)conv);
        source.id = g_timeout_add_seconds(
            purple_prefs_get_int(OPT_API_BASE_GET_INTERVAL),
            get_status_with_api, (gpointer)conv);
        source.conv = conv;
        attach_to_conv(conv, NULL);
        break;
    case wassr_service:
    case identica_service:
    case jisko_service:
    case ffeed_service:
        attach_to_conv(conv, NULL);
        break;
    default:
        twitter_debug("unknown service\n");
        break;
    }
}

static void
deleting_conv_cb(PurpleConversation *conv)
{
    PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);

    twitter_debug("called\n");

    g_return_if_fail(gtkconv != NULL);

    gint service = get_service_type(conv);
    GHashTable *hash = NULL;

    /* only attach to twitter conversation window */
    switch(service) {
    case twitter_service:
        if(purple_prefs_get_bool(OPT_API_BASE_POST)) {
            g_source_remove_by_user_data((gpointer)conv);
            source.id = 0;
            source.conv = NULL;
        }
        hash = icon_hash[service];
        break;
    case wassr_service:
    case identica_service:
    case jisko_service:
    case ffeed_service:
        hash = icon_hash[service];
        break;
    default:
        twitter_debug("unknown service\n");
        break;
    }

    if(hash)
        delete_requested_icon_marks(gtkconv, hash);
}

void
apply_filter(gchar **sender, gchar **buffer, PurpleMessageFlags *flags, int service)
{
    GMatchInfo *match_info;
    const gchar *list = NULL;
    gchar *screen_name = NULL;
    gchar **candidates = NULL, **candidate = NULL;

    g_return_if_fail(*sender != NULL);
    g_return_if_fail(*buffer != NULL);

    gchar *plain = strip_html_markup(*buffer);

    switch(service) {
    case twitter_service:
    default:
        list = purple_prefs_get_string(OPT_FILTER_TWITTER);
        screen_name = g_strdup_printf("@%s", purple_prefs_get_string(OPT_SCREEN_NAME_TWITTER));
        break;
    case wassr_service:
        list = purple_prefs_get_string(OPT_FILTER_WASSR);
        screen_name = g_strdup_printf("@%s", purple_prefs_get_string(OPT_SCREEN_NAME_WASSR));
        break;
    case identica_service:
        list = purple_prefs_get_string(OPT_FILTER_IDENTICA);
        screen_name = g_strdup_printf("@%s", purple_prefs_get_string(OPT_SCREEN_NAME_IDENTICA));
        break;
    case jisko_service:
        list = purple_prefs_get_string(OPT_FILTER_JISKO);
        screen_name = g_strdup_printf("@%s", purple_prefs_get_string(OPT_SCREEN_NAME_JISKO));
    case ffeed_service:
        list = purple_prefs_get_string(OPT_FILTER_FFEED);
        screen_name = g_strdup_printf("@%s", purple_prefs_get_string(OPT_SCREEN_NAME_FFEED));
        break;
    }
    g_return_if_fail(list != NULL);
    if(strstr(list, DEFAULT_LIST))
        return;

    /* find @myself */
    if(purple_prefs_get_bool(OPT_FILTER_EXCLUDE_REPLY) &&
       strstr(plain, screen_name)) {
        g_free(plain);
        g_free(screen_name);
        return;
    }
    else
        g_free(screen_name);

    candidates = g_strsplit_set(list, " ,:;", 0);
    g_return_if_fail(candidates != NULL);

    g_regex_match(regp[SENDER], plain, 0, &match_info);
    while(g_match_info_matches(match_info)) {
        gchar *user = NULL;
        user = g_match_info_fetch(match_info, 2);
        twitter_debug("user = %s\n", user);

        for(candidate = candidates; *candidate; candidate++) {
            if(!strcmp(*candidate, ""))
                continue;
            twitter_debug("candidate = %s\n", *candidate);
            if(!strcmp(user, *candidate)) {
                twitter_debug("match. filter %s\n", user);
                /* pidgin should handle this flag properly --yaz */
//                *flags |= PURPLE_MESSAGE_INVISIBLE;

                /* temporal workaround */
                g_free(*sender); *sender = NULL;
                g_free(*buffer); *buffer = NULL;
                break;
            }
        }

        g_free(user);
        g_match_info_next(match_info, NULL);
    }

    g_free(plain);
    g_strfreev(candidates);
    g_match_info_free(match_info);
}


static gboolean
receiving_im_cb(PurpleAccount *account, char **sender, char **buffer,
                PurpleConversation *conv, PurpleMessageFlags *flags, void *data)
{
    twitter_debug("called\n");
    twitter_debug("buffer = %s suppress_oops = %d\n", *buffer, suppress_oops);

    gint service;

    service = get_service_type_by_account(account, *sender);
    twitter_debug("service = %d\n", service);

#ifdef _WIN32
    /* suppress notification of incoming messages. */
    if(service != unknown_service &&
       purple_prefs_get_bool(OPT_PREVENT_NOTIFICATION)) {
        if(!blink_modified) {
            blink_modified = TRUE;
            blink_state = purple_prefs_get_bool(OPT_PIDGIN_BLINK_IM);
            purple_prefs_set_bool(OPT_PIDGIN_BLINK_IM, FALSE);
        }
    }
    else {
        if(blink_modified) {
            purple_prefs_set_bool(OPT_PIDGIN_BLINK_IM, blink_state);
            blink_modified = FALSE;
        }
    }
#endif

    if(service == wassr_service) {
        gchar *stripped = strip_html_markup(*buffer);
        /* suppress annoying completion message from wassr */
        if(strstr(*buffer, "<body>チャンネル投稿完了:")) {
            twitter_debug("clearing channel parrot message\n");
            g_free(*sender); *sender = NULL;
            g_free(*buffer); *buffer = NULL;
        }
        /* discard parrot message */
        else {
            GList *current = g_list_first(wassr_parrot_list);
            while(current) {
                GList *next = g_list_next(current);

                if(strstr(stripped, current->data)) {
                    twitter_debug("parrot clearing: buf = %s post = %s\n",
                                  *buffer, (char *)current->data);
                    g_free(*sender); *sender = NULL;
                    g_free(*buffer); *buffer = NULL;
                    g_free(current->data);
                    current->data = NULL;
                    wassr_parrot_list =
                        g_list_delete_link(wassr_parrot_list, current);
                    break;
                }

                current = next;
            }
        }
        g_free(stripped);
    }
    else if(service == identica_service) {
        /* discard parrot message */
        gchar *stripped = strip_html_markup(*buffer);
        GList *current = g_list_first(identica_parrot_list);
        while(current) {
            GList *next = g_list_next(current);

            if(strstr(stripped, current->data)) {
                twitter_debug("identica parrot clearing: buf = %s post = %s\n",
                              *buffer, (char *)current->data);
                g_free(*sender); *sender = NULL;
                g_free(*buffer); *buffer = NULL;
                g_free(current->data);
                current->data = NULL;
                identica_parrot_list =
                    g_list_delete_link(identica_parrot_list, current);
                break;
            }

            current = next;
        }
        g_free(stripped);
    }
    else if(service == ffeed_service) {
        /* discard parrot message */
        gchar *stripped = strip_html_markup(*buffer);
        GList *current = g_list_first(ffeed_parrot_list);
        while(current) {
            GList *next = g_list_next(current);

            if(strstr(stripped, current->data)) {
                twitter_debug("ffeed parrot clearing: buf = %s post = %s\n",
                              *buffer, (char *)current->data);
                g_free(*sender); *sender = NULL;
                g_free(*buffer); *buffer = NULL;
                g_free(current->data);
                current->data = NULL;
                ffeed_parrot_list =
                    g_list_delete_link(ffeed_parrot_list, current);
                break;
            }

            current = next;
        }
        g_free(stripped);
    }

    /* filtering */
    if(purple_prefs_get_bool(OPT_FILTER)) {
        apply_filter(sender, buffer, flags, service);
    }

    /* return here if it is not twitter */
    if(service != twitter_service) {
        return FALSE;
    }

    /* if we use api, discard all incoming IM messages. */
    if(purple_prefs_get_bool(OPT_API_BASE_POST)) {
        g_free(*sender); *sender = NULL;
        g_free(*buffer); *buffer = NULL;
    }

    if(!suppress_oops || !purple_prefs_get_bool(OPT_SUPPRESS_OOPS))
        return FALSE;

    if(strstr(*buffer, OOPS_MESSAGE)) {
        twitter_debug("clearing sender and buffer\n");
        g_free(*sender); *sender = NULL;
        g_free(*buffer); *buffer = NULL;
        suppress_oops = FALSE;
    }
    return FALSE;
}


static gboolean
displaying_im_cb(PurpleAccount *account, const char *who, char **message,
                 PurpleConversation *conv, PurpleMessageFlags flags,
                 void *unused)
{
    GtkIMHtml *imhtml;
    GtkTextBuffer *text_buffer;
    gint service = get_service_type(conv);
    gint linenumber = 0;

    twitter_debug("called\n");

    if(service == unknown_service) {
        twitter_debug("neither twitter nor wassr conv\n");
        return FALSE;
    }

    /* get text buffer */
    imhtml = GTK_IMHTML(PIDGIN_CONVERSATION(conv)->imhtml);
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(imhtml));

    /* store number of lines */
    linenumber = gtk_text_buffer_get_line_count(text_buffer);
    g_hash_table_insert(conv_hash, conv, GINT_TO_POINTER(linenumber));
    twitter_debug("conv = %p linenumber = %d\n", conv, linenumber);

    return FALSE;
}

static void
displayed_im_cb(PurpleAccount *account, const char *who, char *message,
                PurpleConversation *conv, PurpleMessageFlags flags)
{
    GMatchInfo *match_info = NULL;
    gchar *user_name = NULL;
    GtkIMHtml *imhtml;
    GtkTextBuffer *text_buffer;
    GtkTextIter insertion_point;
    gint service = get_service_type(conv);
    icon_data *data = NULL;
    gint linenumber;
    GHashTable *hash = NULL;
    gboolean renew = FALSE;

    twitter_debug("called\n");

    if(service == unknown_service) {
        twitter_debug("unknown service\n");
        return;
    }

    /* get user's name */
    g_regex_match(regp[USER], message, 0, &match_info);
    if(!g_match_info_matches(match_info)) {
        twitter_debug("message was not matched : %s\n", message);
        g_match_info_free(match_info);
        return;
    }

    user_name = g_match_info_fetch(match_info, 1);
    g_match_info_free(match_info);

    /* insert icon */
    imhtml = GTK_IMHTML(PIDGIN_CONVERSATION(conv)->imhtml);
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(imhtml));

    /* get GtkTextIter in the target line */
    linenumber = GPOINTER_TO_INT(g_hash_table_lookup(conv_hash, conv));
    gtk_text_buffer_get_iter_at_line(text_buffer,
                                     &insertion_point,
                                     linenumber);

    switch(service) {
    case twitter_service:
    case wassr_service:
    case identica_service:
    case jisko_service:
    case ffeed_service:
        hash = icon_hash[service];
        break;
    default:
        twitter_debug("unknown service\n");
        break;
    }

    if(hash)
        data = g_hash_table_lookup(hash, user_name);

    if(data) {
        /* check validity of icon */
        int count_thres = purple_prefs_get_int(OPT_ICON_MAX_COUNT);
        int days_thres =
            DAYS_TO_SECONDS(purple_prefs_get_int(OPT_ICON_MAX_DAYS));

        if(data->use_count > count_thres ||
           (data->mtime && ((time(NULL) - data->mtime)) > days_thres)) {
            twitter_debug("count=%d mtime=%d\n",
                          data->use_count, (int)(data->mtime));
            renew = TRUE;
            request_icon(user_name, service, renew);
        }
    }

    /* if we don't have the icon for this user, put a mark instead and
     * request the icon */
    if(!data || !data->pixbuf) {
        twitter_debug("%s's icon is not in memory.\n", user_name);
        mark_icon_for_user(gtk_text_buffer_create_mark(
                               text_buffer, NULL, &insertion_point, FALSE),
                           user_name, service);
        /* request to attach icon to the buffer */
        request_icon(user_name, service, renew);
        g_free(user_name); user_name = NULL;
        return;
    }

    /* if we have the icon for this user, insert the icon immediately */
    if(purple_prefs_get_bool(OPT_SHOW_ICON)) {
        gtk_text_buffer_insert_pixbuf(text_buffer,
                                      &insertion_point,
                                      data->pixbuf);
        data->use_count++;
    }
    g_free(user_name); user_name = NULL;

    twitter_debug("reach end of function\n");
}

static gboolean
load_plugin(PurplePlugin *plugin)
{
    int i;

    /* connect to signal */
    purple_signal_connect(purple_conversations_get_handle(),
                          "writing-im-msg",
                          plugin, PURPLE_CALLBACK(writing_im_cb), NULL);
    purple_signal_connect(purple_conversations_get_handle(),
                          "sending-im-msg",
                          plugin, PURPLE_CALLBACK(sending_im_cb), NULL);
    purple_signal_connect(purple_conversations_get_handle(),
                          "conversation-created",
                          plugin, PURPLE_CALLBACK(conv_created_cb), NULL);
    purple_signal_connect(purple_conversations_get_handle(),
                          "receiving-im-msg",
                          plugin, PURPLE_CALLBACK(receiving_im_cb), NULL);
    purple_signal_connect(pidgin_conversations_get_handle(),
                          "displaying-im-msg",
                          plugin, PURPLE_CALLBACK(displaying_im_cb), NULL);

    purple_signal_connect(pidgin_conversations_get_handle(),
                          "displayed-im-msg",
                          plugin, PURPLE_CALLBACK(displayed_im_cb), NULL);
    purple_signal_connect(purple_conversations_get_handle(),
                          "deleting-conversation",
                           plugin, PURPLE_CALLBACK(deleting_conv_cb), NULL);
    purple_signal_connect(purple_connections_get_handle(),
                          "signed-on",
                          plugin, PURPLE_CALLBACK(signed_on_cb), NULL);


    /* compile regex */
    regp[RECIPIENT] = g_regex_new(P_RECIPIENT, 0, 0, NULL);
    regp[SENDER]    = g_regex_new(P_SENDER,    0, 0, NULL);
    regp[SENDER_FFEED] = g_regex_new(P_SENDER_FFEED, 0, 0, NULL);
    regp[COMMAND]   = g_regex_new(P_COMMAND, G_REGEX_RAW, 0, NULL);
    regp[PSEUDO]    = g_regex_new(P_PSEUDO,  G_REGEX_RAW, 0, NULL);
    regp[USER]      = g_regex_new(P_USER, 0, 0, NULL);
    regp[CHANNEL_WASSR]  = g_regex_new(P_CHANNEL, 0, 0, NULL);
    regp[TAG_TWITTER]    = g_regex_new(P_TAG_TWITTER, 0, 0, NULL);
    regp[TAG_IDENTICA]   = g_regex_new(P_TAG_IDENTICA, 0, 0, NULL);
    regp[GROUP_IDENTICA] = g_regex_new(P_GROUP_IDENTICA, 0, 0, NULL);
    regp[IMAGE_TWITTER]  = g_regex_new(P_IMAGE_TWITTER, 0, 0, NULL);
    regp[IMAGE_WASSR]    = g_regex_new(P_IMAGE_WASSR, 0, 0, NULL);
    regp[IMAGE_IDENTICA] = g_regex_new(P_IMAGE_IDENTICA, 0, 0, NULL);
    regp[IMAGE_JISKO]    = g_regex_new(P_IMAGE_JISKO, 0, 0, NULL);
    regp[IMAGE_FFEED]    = g_regex_new(P_IMAGE_FFEED, 0, 0, NULL);
    regp[SIZE_128_WASSR] = g_regex_new(P_SIZE_128_WASSR, 0, 0, NULL);
    regp[EXCESS_LF] = g_regex_new(P_EXCESS_LF, 0, 0, NULL);
    regp[TRAIL_HASH] = g_regex_new(P_TRAIL_HASH, 0, 0, NULL);

    for(i = twitter_service; i < NUM_SERVICES; i++) {
        icon_hash[i] = g_hash_table_new_full(g_str_hash, g_str_equal,
                                              g_free, NULL);
    }

    conv_hash = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                              NULL, NULL);


    /* attach counter to the existing twitter window */
    if(purple_prefs_get_bool(OPT_COUNTER)) {
        attach_to_window();
    }

    return TRUE;
}

static void
cancel_fetch_func(gpointer key, gpointer value, gpointer user_data)
{
    icon_data *data = (icon_data *)value;

    if(!data)
        return;

    if(data->requested) {
        purple_util_fetch_url_cancel(data->fetch_data);
        data->fetch_data = NULL;
        data->requested = FALSE;
    }

    if(data->request_list) {
        twitter_debug("somehow, request_list != NULL\n");
    }
}

static void
cleanup_hash_entry_func(gpointer key, gpointer value, gpointer user_data)
{
    remove_marks_func(key, value, user_data);
    cancel_fetch_func(key, value, user_data);
}

static gboolean
unload_plugin(PurplePlugin *plugin)
{
    int i;
    GList *current;

    twitter_debug("called\n");

    /* disconnect from signal */
    purple_signal_disconnect(purple_conversations_get_handle(),
                             "writing-im-msg",
                             plugin, PURPLE_CALLBACK(writing_im_cb));
    purple_signal_disconnect(purple_conversations_get_handle(),
                             "sending-im-msg",
                             plugin, PURPLE_CALLBACK(sending_im_cb));
    purple_signal_disconnect(purple_conversations_get_handle(),
                             "conversation-created",
                             plugin, PURPLE_CALLBACK(conv_created_cb));
    purple_signal_disconnect(pidgin_conversations_get_handle(),
                             "displaying-im-msg",
                             plugin, PURPLE_CALLBACK(displaying_im_cb));
    purple_signal_disconnect(pidgin_conversations_get_handle(),
                             "displayed-im-msg",
                             plugin, PURPLE_CALLBACK(displayed_im_cb));
    purple_signal_disconnect(purple_conversations_get_handle(),
                             "receiving-im-msg",
                             plugin, PURPLE_CALLBACK(receiving_im_cb));
    purple_signal_disconnect(purple_conversations_get_handle(),
                             "deleting-conversation",
                             plugin, PURPLE_CALLBACK(deleting_conv_cb));
    purple_signal_disconnect(purple_connections_get_handle(),
                             "signed-on",
                             plugin, PURPLE_CALLBACK(signed_on_cb));

    /* unreference regp */
    for(i = 0; i < NUM_REGPS; i++) {
        g_regex_unref(regp[i]);
    }

    /* remove mark list in each hash entry */
    /* cancel request that has not been finished yet */
    for(i = twitter_service; i < NUM_SERVICES; i++) {
        /* delete mark list and stop requeset for each hash table */
        g_hash_table_foreach(icon_hash[i],
                             (GHFunc)cleanup_hash_entry_func, NULL);
        /* destroy hash table for icon_data */
        g_hash_table_destroy(icon_hash[i]);
    }

    g_hash_table_destroy(conv_hash);

    /* detach from twitter window */
    detach_from_window();

    /* free wassr_parrot_list */
    current = g_list_first(wassr_parrot_list);
    while(current) {
        GList *next;

        next = g_list_next(current);
        g_free(current->data);
        wassr_parrot_list =
            g_list_delete_link(wassr_parrot_list, current);

        current = next;
    }
    g_list_free(wassr_parrot_list);
    wassr_parrot_list = NULL;

    /* free identica_parot_list */
    current = g_list_first(identica_parrot_list);
    while(current) {
        GList *next;

        next = g_list_next(current);
        g_free(current->data);
        identica_parrot_list =
            g_list_delete_link(identica_parrot_list, current);

        current = next;
    }
    g_list_free(identica_parrot_list);
    identica_parrot_list = NULL;

    return TRUE;
}

static PidginPluginUiInfo ui_info = {
    prefs_get_frame,
	0,              /* page number - reserved	*/
	NULL,           /* reserved 1	*/
	NULL,           /* reserved 2	*/
	NULL,           /* reserved 3	*/
	NULL            /* reserved 4	*/
};

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,     /**< type	*/
    PIDGIN_PLUGIN_TYPE,         /**< ui_req	*/
    0,                          /**< flags	*/
    NULL,                       /**< deps	*/
    PURPLE_PRIORITY_DEFAULT,    /**< priority	*/
    PLUGIN_ID,                  /**< id     */
    "Pidgin-Twitter",           /**< name	*/
    "0.8.4",                    /**< version	*/
    "provides useful features for twitter", /**  summary	*/
    "provides useful features for twitter", /**  desc	*/
    "Yoshiki Yazawa, mikanbako, \nKonosuke Watanabe, IWATA Ray, \nmojin, umq, \nthe pidging-twitter team",     /**< author	*/
    "http://www.honeyplanet.jp/pidgin-twitter/",   /**< homepage	*/
    load_plugin,                /**< load	*/
    unload_plugin,              /**< unload	*/
    NULL,                       /**< destroy	*/
    &ui_info,                   /**< ui_info	*/
    NULL,                       /**< extra_info	*/
    NULL,                       /**< pref info	*/
    NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
    char *dirname = NULL;

    g_type_init();
    dirname = g_build_filename(purple_user_dir(), "pidgin-twitter", "icons", NULL);
    if(dirname)
        purple_prefs_add_string(OPT_ICON_DIR, dirname);
    g_free(dirname);

    /* add plugin preferences */
    purple_prefs_add_none(OPT_PIDGINTWITTER);
    purple_prefs_add_bool(OPT_TRANSLATE_RECIPIENT, TRUE);
    purple_prefs_add_bool(OPT_TRANSLATE_SENDER, TRUE);
    purple_prefs_add_bool(OPT_TRANSLATE_CHANNEL, TRUE);
    purple_prefs_add_bool(OPT_ESCAPE_PSEUDO, TRUE);
    purple_prefs_add_bool(OPT_STRIP_EXCESS_LF, TRUE);

    purple_prefs_add_bool(OPT_PLAYSOUND_RECIPIENT, TRUE);
    purple_prefs_add_bool(OPT_PLAYSOUND_SENDER, TRUE);
    purple_prefs_add_int(OPT_SOUNDID_RECIPIENT, PURPLE_SOUND_POUNCE_DEFAULT);
    purple_prefs_add_string(OPT_USERLIST_RECIPIENT, DEFAULT_LIST);
    purple_prefs_add_int(OPT_SOUNDID_SENDER, PURPLE_SOUND_POUNCE_DEFAULT);
    purple_prefs_add_string(OPT_USERLIST_SENDER, DEFAULT_LIST);

    purple_prefs_add_bool(OPT_COUNTER, TRUE);
    purple_prefs_add_bool(OPT_SUPPRESS_OOPS, TRUE);
    purple_prefs_add_bool(OPT_PREVENT_NOTIFICATION, FALSE);

    purple_prefs_add_bool(OPT_API_BASE_POST, TRUE);
    purple_prefs_add_int(OPT_API_BASE_GET_INTERVAL, TWITTER_DEFAULT_INTERVAL);
    purple_prefs_add_int(OPT_RETRIEVE_COUNT, TWITTER_DEFAULT_RETRIEVE_COUNT);
    purple_prefs_add_string(OPT_SCREEN_NAME_TWITTER, EMPTY);
    purple_prefs_add_string(OPT_PASSWORD_TWITTER, EMPTY);
    purple_prefs_add_string(OPT_SCREEN_NAME_WASSR, EMPTY);
    purple_prefs_add_string(OPT_SCREEN_NAME_IDENTICA, EMPTY);
    purple_prefs_add_string(OPT_SCREEN_NAME_JISKO, EMPTY);
    purple_prefs_add_string(OPT_SCREEN_NAME_FFEED, EMPTY);

    purple_prefs_add_bool(OPT_SHOW_ICON, TRUE);
    purple_prefs_add_int(OPT_ICON_SIZE, DEFAULT_ICON_SIZE);
    purple_prefs_add_bool(OPT_UPDATE_ICON, TRUE);
    purple_prefs_add_int(OPT_ICON_MAX_COUNT, DEFAULT_ICON_MAX_COUNT);
    purple_prefs_add_int(OPT_ICON_MAX_DAYS, DEFAULT_ICON_MAX_DAYS);
    purple_prefs_add_bool(OPT_LOG_OUTPUT, FALSE);

    purple_prefs_add_bool(OPT_FILTER, TRUE);
    purple_prefs_add_bool(OPT_FILTER_EXCLUDE_REPLY, TRUE);
    purple_prefs_add_string(OPT_FILTER_TWITTER, DEFAULT_LIST);
    purple_prefs_add_string(OPT_FILTER_WASSR, DEFAULT_LIST);
    purple_prefs_add_string(OPT_FILTER_IDENTICA, DEFAULT_LIST);
    purple_prefs_add_string(OPT_FILTER_JISKO, DEFAULT_LIST);
    purple_prefs_add_string(OPT_FILTER_FFEED, DEFAULT_LIST);
}

PURPLE_INIT_PLUGIN(pidgin_twitter, init_plugin, info)
