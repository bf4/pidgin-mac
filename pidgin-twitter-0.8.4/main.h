#ifndef _PIDGIN_TWITTER_MAIN_H_
#define _PIDGIN_TWITTER_MAIN_H_

void apply_filter(gchar **sender, gchar **buffer, PurpleMessageFlags *flags, int service);
gboolean is_twitter_account(PurpleAccount *account, const char *name);
void attach_to_window(void);
void detach_from_window(void);

#endif
