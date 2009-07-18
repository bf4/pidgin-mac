#ifndef _PIDGIN_TWITTER_H_
#define _PIDGIN_TWITTER_H_

#define _BSD_SOURCE
#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <time.h>
#include <locale.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libxml/xmlreader.h>

#include <gtkplugin.h>
#include <util.h>
#include <debug.h>
#include <connection.h>
#include <version.h>
#include <sound.h>
#include <gtkconv.h>
#include <gtkimhtml.h>

#include "util.h"
#include "prefs.h"
#include "twitter_api.h"
#include "icon.h"
#include "main.h"

/* regp id */
enum {
    RECIPIENT = 0,
    SENDER,
    SENDER_FFEED,
    COMMAND,
    PSEUDO,
    USER,
    CHANNEL_WASSR,
    TAG_TWITTER,
    TAG_IDENTICA,
    GROUP_IDENTICA,
    IMAGE_TWITTER,
    IMAGE_WASSR,
    IMAGE_IDENTICA,
    IMAGE_JISKO,
    IMAGE_FFEED,
    SIZE_128_WASSR,
    EXCESS_LF,
    TRAIL_HASH
};

/* service id */
enum {
    unknown_service = -1,
    twitter_service,
    wassr_service,
    identica_service,
    jisko_service,
    ffeed_service
};

/* container to hold icon data */
typedef struct _icon_data {
    GdkPixbuf *pixbuf;      /* icon pixmap */
    gboolean requested;     /* TRUE if download icon has been requested */
    GList *request_list;    /* marker list */
    PurpleUtilFetchUrlData *fetch_data; /* icon fetch data */
    const gchar *img_type;  /* image type */
    gchar *icon_url;        /* url for the user's icon */
    gint use_count;         /* usage count */
    time_t mtime;           /* mtime of file */
} icon_data;

/* used by got_icon_cb */
typedef struct _got_icon_data {
    gchar *user_name;
    gint service;
} got_icon_data;

/* used by eval */
typedef struct _eval_data {
    gint which;
    gint service;
} eval_data;

/* container for api based retrieve */
typedef struct _status {
    gchar *created_at;
    gchar *text;
    gchar *screen_name;
    gchar *profile_image_url;
    time_t time;
    guint64 id;
} status_t;

typedef struct _source {
    guint id;
    PurpleConversation *conv;
} source_t;

/* container for api based post */
typedef struct twitter_message {
    PurpleAccount *account;
    char *status;
    time_t time;
} twitter_message_t;

#define PLUGIN_ID	            "gtk-honeyplanet-pidgin_twitter"
#define PLUGIN_NAME	            "pidgin-twitter"

/* options */
#define OPT_PIDGINTWITTER 		"/plugins/pidgin_twitter"
#define OPT_TRANSLATE_RECIPIENT OPT_PIDGINTWITTER "/translate_recipient"
#define OPT_TRANSLATE_SENDER    OPT_PIDGINTWITTER "/translate_sender"
#define OPT_TRANSLATE_CHANNEL   OPT_PIDGINTWITTER "/translate_channel"
#define OPT_PLAYSOUND_RECIPIENT OPT_PIDGINTWITTER "/playsound_recipient"
#define OPT_PLAYSOUND_SENDER    OPT_PIDGINTWITTER "/playsound_sender"
#define OPT_SOUNDID_RECIPIENT   OPT_PIDGINTWITTER "/soundid_recipient"
#define OPT_SOUNDID_SENDER      OPT_PIDGINTWITTER "/soundid_sender"
#define OPT_ESCAPE_PSEUDO       OPT_PIDGINTWITTER "/escape_pseudo"
#define OPT_USERLIST_RECIPIENT  OPT_PIDGINTWITTER "/userlist_recipient"
#define OPT_USERLIST_SENDER     OPT_PIDGINTWITTER "/userlist_sender"
#define OPT_COUNTER             OPT_PIDGINTWITTER "/counter"
#define OPT_SUPPRESS_OOPS       OPT_PIDGINTWITTER "/suppress_oops"
#define OPT_PREVENT_NOTIFICATION OPT_PIDGINTWITTER "/prevent_notification"
#define OPT_ICON_DIR            OPT_PIDGINTWITTER "/icon_dir"
#define OPT_API_BASE_POST       OPT_PIDGINTWITTER "/api_base_post"
#define OPT_SCREEN_NAME_TWITTER OPT_PIDGINTWITTER "/screen_name_twitter"
#define OPT_SCREEN_NAME_WASSR   OPT_PIDGINTWITTER "/screen_name_wassr"
#define OPT_SCREEN_NAME_IDENTICA OPT_PIDGINTWITTER "/screen_name_identica"
#define OPT_SCREEN_NAME_JISKO   OPT_PIDGINTWITTER "/screen_name_jisko"
#define OPT_SCREEN_NAME_FFEED   OPT_PIDGINTWITTER "/screen_name_ffeed"
#define OPT_PASSWORD_TWITTER    OPT_PIDGINTWITTER "/password_twitter"
#define OPT_SHOW_ICON           OPT_PIDGINTWITTER "/show_icon"
#define OPT_ICON_SIZE           OPT_PIDGINTWITTER "/icon_size"
#define OPT_UPDATE_ICON         OPT_PIDGINTWITTER "/update_icon"
#define OPT_ICON_MAX_COUNT      OPT_PIDGINTWITTER "/icon_max_count"
#define OPT_ICON_MAX_DAYS       OPT_PIDGINTWITTER "/icon_max_days"
#define OPT_API_BASE_GET_INTERVAL OPT_PIDGINTWITTER "/api_base_get_interval"
#define OPT_LOG_OUTPUT          OPT_PIDGINTWITTER "/log_output"
#define OPT_FILTER              OPT_PIDGINTWITTER "/filter"
#define OPT_FILTER_EXCLUDE_REPLY OPT_PIDGINTWITTER "/filter_exclude_reply"
#define OPT_FILTER_TWITTER      OPT_PIDGINTWITTER "/filter_twitter"
#define OPT_FILTER_WASSR        OPT_PIDGINTWITTER "/filter_wassr"
#define OPT_FILTER_IDENTICA     OPT_PIDGINTWITTER "/filter_identica"
#define OPT_FILTER_JISKO        OPT_PIDGINTWITTER "/filter_jisko"
#define OPT_FILTER_FFEED        OPT_PIDGINTWITTER "/filter_ffeed"
#define OPT_STRIP_EXCESS_LF     OPT_PIDGINTWITTER "/strip_excess_lf"
#define OPT_RETRIEVE_COUNT      OPT_PIDGINTWITTER "/retrieve_count"

#ifdef _WIN32
#define OPT_PIDGIN_BLINK_IM     PIDGIN_PREFS_ROOT "/win32/blink_im"
#endif

/* formats and templates */
#define RECIPIENT_FORMAT_TWITTER "%s@<a href='http://twitter.com/%s'>%s</a>"
#define SENDER_FORMAT_TWITTER   "%s<a href='http://twitter.com/%s'>%s</a>: "
#define RECIPIENT_FORMAT_WASSR  "%s@<a href='http://wassr.jp/user/%s'>%s</a>"
#define SENDER_FORMAT_WASSR     "%s<a href='http://wassr.jp/user/%s'>%s</a>: "
#define RECIPIENT_FORMAT_IDENTICA "%s@<a href='http://identi.ca/%s'>%s</a>"
#define SENDER_FORMAT_IDENTICA  "%s<a href='http://identi.ca/%s'>%s</a>: "
#define RECIPIENT_FORMAT_JISKO  "%s@<a href='http://jisko.net/%s'>%s</a>"
#define SENDER_FORMAT_JISKO     "%s<a href='http://jisko.net/%s'>%s</a>: "
#define RECIPIENT_FORMAT_FFEED  "%s@<a href='http://friendfeed.com/%s'>%s</a>"
#define SENDER_FORMAT_FFEED     "%s<a href='http://friendfeed.com/%s'>%s</a>: "
#define CHANNEL_FORMAT_WASSR    "%s<a href='http://wassr.jp/channel/%s'>%s</a> "
#define CHANNEL_FORMAT_IDENTICA "%s<a href='http://identi.ca/tag/%s'>%s</a> "
#define TAG_FORMAT_TWITTER      "%s<a href='http://twitter.com/search?q=%%23%s'>#%s</a>"
#define TAG_FORMAT_IDENTICA     "#<a href='http://identi.ca/tag/%s'>%s</a>"
#define GROUP_FORMAT_IDENTICA   "!<a href='http://identi.ca/group/%s'>%s</a>"

#define DEFAULT_LIST            "(list of users: separated with ' ,:;')"
#define OOPS_MESSAGE            "<body>Oops! Your update was over 140 characters. We sent the short version to your friends (they can view the entire update on the web).<BR></body>"
#define EMPTY                   ""

/* patterns */
#define P_RECIPIENT         "(^|\\s+|[.[:^print:]])@([-A-Za-z0-9_]+)"
#define P_SENDER            "^(\\r?\\n?)\\s*([-A-Za-z0-9_]+)(?:\\s*\\(.+\\))?: "
#define P_SENDER_FFEED      "^(\\r?\\n?)\\s*@[0-9]\\s*([-A-Za-z0-9_]+)\\s*"
#define P_COMMAND           "^(?:\\s*)([dDfFgGlLmMnNtTwW]{1}\\s+[A-Za-z0-9_]+)(?:\\s*\\Z)"
#define P_PSEUDO            "^\\s*(?:[\"#$%&'()*+,\\-./:;<=>?\\[\\\\\\]_`{|}~]|[^\\s\\x21-\\x7E])*([dDfFgGlLmMnNtTwW]{1})(?:\\Z|\\s+|[^\\x21-\\x7E]+\\Z)"
#define P_USER              "^.*?(?:<a .+?>)?([-A-Za-z0-9_]+)(?:</a>)?:"
#define P_CHANNEL           "^(.*?(?:<a .+?>)?[-A-Za-z0-9_]+(?:</a>)?: \\r?\\n?#)([A-Za-z0-9_]+) "
#define P_TAG_TWITTER       "(^|\\s+)#([-A-Za-z0-9_]+)"
#define P_TAG_IDENTICA      "#([-A-Za-z0-9_]+)"
#define P_GROUP_IDENTICA    "!([-A-Za-z0-9_]+)"
#define P_IMAGE_TWITTER     "<img .*=\"profile-(?:image|img)\".*src=\"(https?://.+?)\".*/>"
#define P_IMAGE_WASSR       "<div class=\"image\"><a href=\".+\"><img src=\"(.+)\" width=\".+?\" /></a></div>"
#define P_IMAGE_IDENTICA    "<img src=\"(https?://.+.identi.ca/.+)\" class=\"avatar profile photo\" width=\"96\" height=\"96\" alt=\"[A-Za-z0-9_]+\"/>"
#define P_IMAGE_JISKO       "<img src=\"(https?://jisko.net/users/.+/img/avatar/thumb_side\\..+)\" alt=\"Avatar\" />"
#define P_IMAGE_FFEED       "<img src=\"(https?://i.friendfeed.com/.+)\" alt=\""
#define P_SIZE_128_WASSR    "\\.128\\."
#define P_EXCESS_LF         "([\\r|\\n]{2,})"
#define P_TRAIL_HASH        "( #\\s+$)"

/* twitter API specific macros */
#define TWITTER_BASE_URL "http://twitter.com"
#define TWITTER_STATUS_GET "GET /statuses/friends_timeline.xml?count=%d HTTP/1.1\r\n" \
    "Host: twitter.com\r\n"                                          \
    "User-Agent: pidgin-twitter\r\n"                                 \
    "Authorization: Basic %s\r\n"
#define TWITTER_STATUS_POST "POST /statuses/update.xml HTTP/1.1\r\n" \
    "Host: twitter.com\r\n"                                          \
    "User-Agent: pidgin-twitter\r\n"                                 \
    "Authorization: Basic %s\r\n"                                    \
    "Content-Length: %d\r\n"
#define TWITTER_STATUS_FORMAT "status=%s&source=pidgintwitter"
#define TWITTER_DEFAULT_INTERVAL (60)
#define TWITTER_DEFAULT_ICON_URL "http://static.twitter.com/images/default_profile_bigger.png"
#define TWITTER_DEFAULT_RETRIEVE_COUNT (20)

/* wassr specific macros */
#define WASSR_POST_LEN (255)

/* identica specific macros */
#define IDENTICA_POST_LEN (140)
#define IDENTICA_DEFAULT_ICON_URL "http://theme.identi.ca/identica/default-avatar-profile.png"

/* jisko specific macro */
#define JISKO_DEFAULT_ICON_URL "http://jisko.net/static/img/avatar/default_note.png"

/* ffeed specific macro */
#define FFEED_DEFAULT_ICON_URL "http://friendfeed.com/static/images/nomugshot-large.png"

/* size of substitution buffer */
#define SUBST_BUF_SIZE (32 * 1024)

/* misc macros */
#define DEFAULT_ICON_SIZE (48)
#define DEFAULT_ICON_MAX_COUNT (50)
#define DEFAULT_ICON_MAX_DAYS (7)
#define DAYS_TO_SECONDS(d) ((d) * 86400)
#define NUM_REGPS (18)
#define NUM_SERVICES (5)          /* twitter, wassr, identica, jisko, ffeed. */

/* debug macros */
#define twitter_debug(fmt, ...)	do { if(purple_prefs_get_bool(OPT_LOG_OUTPUT)) purple_debug(PURPLE_DEBUG_INFO, PLUGIN_NAME, "%s: %s():%4d:  " fmt, __FILE__, __FUNCTION__, (int)__LINE__, ## __VA_ARGS__); } while(0);
#define twitter_error(fmt, ...)	do { if(purple_prefs_get_bool(OPT_LOG_OUTPUT)) purple_debug(PURPLE_DEBUG_ERROR, PLUGIN_NAME, "%s: %s():%4d:  " fmt, __FILE__, __FUNCTION__, (int)__LINE__, ## __VA_ARGS__); } while(0);

#endif
