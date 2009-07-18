#ifndef _PIDGIN_TWITTER_UTIL_H_
#define _PIDGIN_TWITTER_UTIL_H_

void escape(gchar **str);
void strip_markup(gchar **str, gboolean escape);
gchar *strip_html_markup(const gchar *src);
gboolean ensure_path_exists(const char *dir);

gboolean is_twitter_conv(PurpleConversation *conv);
gboolean is_wassr_account(PurpleAccount *account, const char *name);
gboolean is_wassr_conv(PurpleConversation *conv);
gboolean is_identica_account(PurpleAccount *account, const char *name);
gboolean is_identica_conv(PurpleConversation *conv);
gboolean is_jisko_account(PurpleAccount *account, const char *name);
gboolean is_jisko_conv(PurpleConversation *conv);

gint get_service_type(PurpleConversation *conv);
gint get_service_type_by_account(PurpleAccount *account, const char *sender);

#endif
