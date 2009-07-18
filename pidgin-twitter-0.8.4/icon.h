#ifndef _PIDGIN_TWITTER_ICON_H_
#define _PIDGIN_TWITTER_ICON_H_

void invalidate_icon_data_func(gpointer key, gpointer value, gpointer user_data);
void request_icon(const char *user_name, gint service, gboolean renew);
void mark_icon_for_user(GtkTextMark *mark, const gchar *user_name, gint service);

#endif
