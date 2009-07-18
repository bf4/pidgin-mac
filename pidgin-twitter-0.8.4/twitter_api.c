#include "pidgin-twitter.h"

static GList *postedlist = NULL;
static GList *statuseslist = NULL;

/* prototypes */
static void parse_user(xmlNode *user, status_t *st);
static void read_timestamp(const char *str, struct tm *res);
static void parse_status(xmlNode *status, status_t *st);

static void free_status(status_t *st);
static gboolean is_posted_message(status_t *status, guint64 lastid);
static void get_status_with_api_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data, const gchar *url_text, size_t len, const gchar *error_message);
static void post_status_with_api_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data, const gchar *url_text, size_t len, const gchar *error_message);

#ifdef _WIN32
extern gboolean blink_state;
extern gboolean blink_modified;
#endif

/**************************/
/* API base get functions */
/**************************/
/* xml parser */
static void
parse_user(xmlNode *user, status_t *st)
{
    xmlNode *nptr;

    for(nptr = user->children; nptr != NULL; nptr = nptr->next) {
        if(nptr->type == XML_ELEMENT_NODE) {
            if(!xmlStrcmp(nptr->name, (xmlChar *)"screen_name")) {
                gchar *str = (gchar *)xmlNodeGetContent(nptr);
                st->screen_name = g_strdup(str);
                twitter_debug("screen_name=%s\n", st->screen_name);
                xmlFree(str);
            }
            else if(!xmlStrcmp(nptr->name, (xmlChar *)"profile_image_url")) {
                gchar *str = (gchar *)xmlNodeGetContent(nptr);
                st->profile_image_url = g_strdup(str);
                xmlFree(str);
            }
        }
    }
}

static gchar *day_of_week_name[] = {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat",
    NULL
};

static gchar *month_name[] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec",
    NULL
};

static void
read_timestamp(const char *str, struct tm *res)
{
    char day_of_week[4];
    char month[4];
    char time_offset[6];
    int day, hour, minute, second, year;
    int i;

    if(str == NULL || res == NULL)
        return;

    sscanf(str, "%s %s %d %d:%d:%d %s %d",
           day_of_week, month, &day,
           &hour, &minute, &second,
           time_offset, &year);

    for(i=0; i<7; i++) {
        if(!strcmp(day_of_week_name[i], day_of_week)) {
            res->tm_wday = i;
        }
    }
    for(i=0; i<12; i++) {
        if(!strcmp(month_name[i], month)) {
            res->tm_mon = i;
        }
    }

    res->tm_mday = day;
    res->tm_hour = hour;
    res->tm_min  = minute;
    res->tm_sec  = second;
    res->tm_year = year - 1900;
#ifndef _WIN32
    int offset   = atoi(time_offset);
    res->tm_gmtoff = -1 * (60 * 60 * offset / 100);
#endif

}

static void
parse_status(xmlNode *status, status_t *st)
{
    xmlNode *nptr;

    for(nptr = status->children; nptr != NULL; nptr = nptr->next) {
        if(nptr->type == XML_ELEMENT_NODE) {
            if(!xmlStrcmp(nptr->name, (xmlChar *)"created_at")) {
                gchar *str = (gchar *)xmlNodeGetContent(nptr);
                st->created_at = g_strdup(str);

                /* read time stamp */
                struct tm res;
                memset(&res, 0x00, sizeof(struct tm));
                read_timestamp(str, &res);
                tzset();
#ifdef _WIN32
                st->time = mktime(&res) - timezone;
#else
                st->time = mktime(&res) + res.tm_gmtoff;
#endif

                xmlFree(str);
            }
            else if(!xmlStrcmp(nptr->name, (xmlChar *)"id")) {
                gchar *str = (gchar *)xmlNodeGetContent(nptr);
                st->id = atoll(str);
                twitter_debug("id=%lld\n", st->id);
                xmlFree(str);
            }
            else if(!xmlStrcmp(nptr->name, (xmlChar *)"text")) {
                gchar *str = (gchar *)xmlNodeGetContent(nptr);
                st->text = g_strdup(str);
                xmlFree(str);
            }
            else if(!xmlStrcmp(nptr->name, (xmlChar *)"user")) {
                parse_user(nptr, st);
            }
        }
    }
}

static void
free_status(status_t *st)
{
    g_free(st->created_at);
    g_free(st->text);
    g_free(st->screen_name);
    g_free(st->profile_image_url);
}

static gboolean
is_posted_message(status_t *status, guint64 lastid)
{
    GList *pp = g_list_first(postedlist);
    gboolean rv = FALSE;

    while(pp) {
        GList *next;
        status_t *posted = (status_t *)pp->data;

        next = g_list_next(pp);

        if(posted->id == status->id) {
            rv = TRUE;
        }

        if(posted->id <= lastid) {
            free_status(posted);
            g_free(pp->data);
            postedlist = g_list_delete_link(postedlist, pp);
        }

        pp = next;
    }

    return rv;
}

static void
get_status_with_api_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data,
                        const gchar *url_text, size_t len,
                        const gchar *error_message)

{
    xmlDocPtr doc;
    xmlNode *nptr, *nptr2;
    static guint64 lastid = 0;
    PurpleConversation *conv;
    GList *stp;
    const gchar *start;

    g_return_if_fail(url_text != NULL);

    conv = (PurpleConversation *)user_data;
    if(!conv)
        return;

    /* skip to the beginning of xml */
    start = strstr(url_text, "<?xml");

    doc = xmlRecoverMemory(start, len - (start - url_text));
    if(doc == NULL)
        return;

#ifdef _WIN32
    /* suppress notification of incoming messages. */
    if(purple_prefs_get_bool(OPT_PREVENT_NOTIFICATION)) {
        if(!blink_modified) {
            blink_modified = TRUE;
            blink_state = purple_prefs_get_bool(OPT_PIDGIN_BLINK_IM);
            purple_prefs_set_bool(OPT_PIDGIN_BLINK_IM, FALSE);
        }
    }
#endif

     for(nptr = doc->children; nptr != NULL; nptr = nptr->next) {
        if(nptr->type == XML_ELEMENT_NODE &&
           !xmlStrcmp(nptr->name, (xmlChar *)"statuses")) {

            for(nptr2 = nptr->children; nptr2 != NULL; nptr2 = nptr2->next) {
                if(nptr2->type == XML_ELEMENT_NODE &&
                    !xmlStrcmp(nptr2->name, (xmlChar *)"status")) {
                    status_t *st = g_new0(status_t, 1);
                    statuseslist = g_list_prepend(statuseslist, st);
                    parse_status(nptr2, st);
                }
            }
        }
     }

     xmlFreeDoc(doc);
     xmlCleanupParser();

     /* process statuseslist */
     stp = g_list_first(statuseslist);
     while(stp) {
         GList *next;
         status_t *st = (status_t *)stp->data;

         next = g_list_next(stp);

         if(st->id > lastid && !is_posted_message(st, lastid)) {
             gchar *msg = NULL;
             gchar *sender = NULL;

             sender = g_strdup("twitter@twitter.com");

             PurpleMessageFlags flag = PURPLE_MESSAGE_RECV;

             msg = g_strdup_printf("%s: %s", st->screen_name, st->text);

             /* apply filter */
             if(purple_prefs_get_bool(OPT_FILTER)) {
                 apply_filter(&sender, &msg, &flag, twitter_service);
             }
             if(sender && msg) {
                 purple_conv_im_write(conv->u.im,
                                      sender,
                                      msg,
                                      flag,
                                      st->time);
             }
             lastid = st->id;

             g_free(sender);
             g_free(msg);
         }

         free_status(st);
         g_free(stp->data);
         statuseslist = g_list_delete_link(statuseslist, stp);

         stp = next;
     }
}

/* status fetching function. it will be called periodically. */
gboolean
get_status_with_api(gpointer data)
{
    twitter_debug("called\n");

    /* fetch friends time line */
    char *request, *header;
    char *basic_auth, *basic_auth_encoded;
    gint count = purple_prefs_get_int(OPT_RETRIEVE_COUNT);

    if(count < TWITTER_DEFAULT_RETRIEVE_COUNT)
        count = TWITTER_DEFAULT_RETRIEVE_COUNT;

    /* if disabled, just return */
    if(!purple_prefs_get_bool(OPT_API_BASE_POST))
        return TRUE;

    const char *screen_name =
        purple_prefs_get_string(OPT_SCREEN_NAME_TWITTER);
    const char *password =
        purple_prefs_get_string(OPT_PASSWORD_TWITTER);

    if (!screen_name || !password || !screen_name[0] || !password[0]) {
        twitter_debug("screen_name or password is empty\n");
        return TRUE;
    }

    /* auth */
    basic_auth = g_strdup_printf("%s:%s", screen_name, password);
    basic_auth_encoded = purple_base64_encode((unsigned char *)basic_auth,
                                              strlen(basic_auth));
    g_free(basic_auth);

    /* header */

    header = g_strdup_printf(TWITTER_STATUS_GET, count, basic_auth_encoded);
    request = g_strconcat(header, "\r\n", NULL);

    /* invoke fetch */
    purple_util_fetch_url_request(TWITTER_BASE_URL, FALSE,
                                  NULL, TRUE, request, TRUE,
                                  get_status_with_api_cb, data);

    g_free(header);
    g_free(basic_auth_encoded);
    g_free(request);

    return TRUE;
}

/****************************/
/* API based post functions */
/****************************/
static void
post_status_with_api_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data,
                        const gchar *url_text, size_t len,
                        const gchar *error_message)
{
    twitter_message_t *tm = (twitter_message_t *)user_data;
    gchar *msg = NULL;
    char *p1 = NULL, *p2 = NULL;
    int error = 1;
    PurpleConversation *conv;

    conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_ANY,
                                                 "twitter@twitter.com",
                                                 tm->account);
    if (!conv) {
        twitter_debug("failed to get conversation\n");
        goto fin;
    }

    if (error_message) {
        /* connection failed or something */
        msg = g_strdup_printf("Local error: %s", error_message);
    } else {
        int code = -1;

        if ((strncmp(url_text, "HTTP/1.0", strlen("HTTP/1.0")) == 0
             || strncmp(url_text, "HTTP/1.1", strlen("HTTP/1.1")) == 0)) {

            p1 = strchr(url_text, ' ');

            if (p1) {
                p1++;
                p2 = strchr(p1, ' ');
                if (p2)
                    p2++;
                else
                    p2 = NULL;
            }
        }

        code = atoi(p1);

        if (code == 200) {
            error = 0;
        } else {
            switch (code) {
            case 400:
                msg = g_strdup("Invalid request. Too many updates?");
                break;
            case 401:
                msg = g_strdup("Authorization failed.");
                break;
            case 403:
                msg = g_strdup("Your update has been refused by Twitter server "
                               "for some reason.");
                break;
            case 404:
                msg = g_strdup("Requested URI is not found.");
                break;
            case 500:
                msg = g_strdup("Server error.");
                break;
            case 502:
                msg = g_strdup("Twitter is down or under maintenance.");
                break;
            case 503:
                msg = g_strdup("Twitter is extremely crowded. "
                               "Try again later.");
                break;
            default:
                msg = g_strdup_printf("Unknown error. (%d %s)",
                                      code, p2 ? p2 : "");
                break;
            }
        }
    }

    if (!error) {
        purple_conv_im_write(conv->u.im,
                             purple_account_get_username(tm->account),
                             tm->status, PURPLE_MESSAGE_SEND, tm->time);

        /* cache message ID that posted via API */
        gchar *start = NULL;
        xmlDocPtr doc;
        xmlNode *nptr;

        start = strstr(url_text, "<?xml");

        if(!start)
            goto fin;

        doc = xmlRecoverMemory(start, len - (start - url_text));
        if(doc == NULL)
            return;

        /* enqueue posted message to postedlist */
        for(nptr = doc->children; nptr != NULL; nptr = nptr->next) {
            if(nptr->type == XML_ELEMENT_NODE &&
               !xmlStrcmp(nptr->name, (xmlChar *)"status")) {
                status_t *st = g_new0(status_t, 1);
                postedlist = g_list_prepend(postedlist, st);
                parse_status(nptr, st);
            }
        }

        xmlFreeDoc(doc);
        xmlCleanupParser();

    } else {
        gchar *m;
        m = g_strdup_printf("%s<BR>%s",
                            msg, tm->status);
        /* FIXME: too strong. it should be more smart */
        purple_conv_im_write(conv->u.im,
                             purple_account_get_username(tm->account),
                             m, PURPLE_MESSAGE_ERROR, time(NULL));
        g_free(m);
    }

 fin:
    if (msg)
        g_free(msg);

    if (tm) {
        if (tm->status)
            g_free(tm->status);
        g_free(tm);
    }

}

void
post_status_with_api(PurpleAccount *account, char **buffer)
{
    char *request, *status, *header;
    const char *url_encoded = g_uri_escape_string(*buffer, "!$'()*,;=:@/?#[]", FALSE);
    char *basic_auth, *basic_auth_encoded;

    twitter_message_t *tm;

    const char *screen_name =
        purple_prefs_get_string(OPT_SCREEN_NAME_TWITTER);
    const char *password = purple_prefs_get_string(OPT_PASSWORD_TWITTER);

    twitter_debug("tm.account: %s\n",
                  purple_account_get_username(account));

    if (!screen_name || !password || !screen_name[0] || !password[0]) {
        twitter_debug("screen_name or password is empty\n");
        return;
    }

    tm = g_new(twitter_message_t, 1);
    tm->account = account;
    tm->status = g_strdup(*buffer);
    tm->time = time(NULL);

    basic_auth = g_strdup_printf("%s:%s", screen_name, password);
    basic_auth_encoded = purple_base64_encode((unsigned char *)basic_auth,
                                              strlen(basic_auth));
    g_free(basic_auth);

    status = g_strdup_printf(TWITTER_STATUS_FORMAT, url_encoded);
    header = g_strdup_printf(TWITTER_STATUS_POST, basic_auth_encoded,
                             (int)strlen(status));

    request = g_strconcat(header, "\r\n", status, "\r\n", NULL);

    purple_util_fetch_url_request(TWITTER_BASE_URL, FALSE,
                                  NULL, TRUE, request, TRUE,
                                  post_status_with_api_cb, tm);

    g_free(header);
    g_free(basic_auth_encoded);
    g_free(status);
    g_free(request);

}

void
signed_on_cb(PurpleConnection *gc)
{
    PurpleAccount *account;
    PurpleBuddy *buddy;
    PurpleConversation *gconv;
    const char name[] = "twitter@twitter.com";

    if(!purple_prefs_get_bool(OPT_API_BASE_POST)) {
        twitter_debug("per prefs\n");
        return;
    }

    account = purple_connection_get_account(gc);
    if(!account) {
        twitter_debug("no account\n");
        return;
    }

    buddy = purple_find_buddy(account, name);
    if(!buddy) {
        twitter_debug("no buddy\n");
        return;
    }

    gconv = purple_find_conversation_with_account(
        PURPLE_CONV_TYPE_IM, name, account);
    if(!gconv) {
        gconv = purple_conversation_new(
            PURPLE_CONV_TYPE_IM, account, name);
        twitter_debug("new conv\n");
    }
}
