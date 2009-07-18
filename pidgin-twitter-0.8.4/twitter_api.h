#ifndef _PIDGIN_TWITTER_TWITTER_API_H_
#define _PIDGIN_TWITTER_TWITTER_API_H_

void post_status_with_api(PurpleAccount *account, char **buffer);
gboolean get_status_with_api(gpointer data);
void signed_on_cb(PurpleConnection *gc);

#endif
