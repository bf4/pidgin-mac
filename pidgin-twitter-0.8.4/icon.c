#include "pidgin-twitter.h"
#include "purple_internal.h"

extern GHashTable *icon_hash[];
extern GRegex *regp[];

/* prototypes */
static void insert_icon_at_mark(GtkTextMark *requested_mark, gpointer user_data);
static void insert_requested_icon(const gchar *user_name, gint service);
static void got_icon_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data, const gchar *url_text, gsize len, const gchar *error_message);
static GdkPixbuf *make_scaled_pixbuf(const gchar *url_text, gsize len);
static void got_page_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data, const gchar *url_text, gsize len, const gchar *error_message);

/* functions */
static void
insert_icon_at_mark(GtkTextMark *requested_mark, gpointer user_data)
{
    got_icon_data *gotdata = (got_icon_data *)user_data;

    gchar *user_name = gotdata->user_name;
    gint service = gotdata->service;

    GList *win_list;
    GtkIMHtml *target_imhtml = NULL;
    GtkTextBuffer *target_buffer = NULL;
    GtkTextIter insertion_point;
    icon_data *data = NULL;
    GHashTable *hash = NULL;

    twitter_debug("called: service = %d\n", service);

    /* find the conversation that contains the mark  */
    for(win_list = pidgin_conv_windows_get_list(); win_list;
            win_list = win_list->next) {
        PidginWindow *win = win_list->data;
        GList *conv_list;

        for(conv_list = pidgin_conv_window_get_gtkconvs(win); conv_list;
                conv_list = conv_list->next) {
            PidginConversation *conv = conv_list->data;
            PurpleConversation *purple_conv = conv->active_conv;

            gint service = get_service_type(purple_conv);

            if(service != unknown_service) {
                GtkIMHtml *current_imhtml = GTK_IMHTML(conv->imhtml);
                GtkTextBuffer *current_buffer = gtk_text_view_get_buffer(
                     GTK_TEXT_VIEW(current_imhtml));

                if(current_buffer == gtk_text_mark_get_buffer(requested_mark)) {
                     target_imhtml = current_imhtml;
                     target_buffer = current_buffer;
                     break;
                }
            }
        }
    }

    if(!(target_imhtml && target_buffer)) {
        return;
    }

    /* insert icon to the mark */
    gtk_text_buffer_get_iter_at_mark(target_buffer,
                                     &insertion_point, requested_mark);

    /* insert icon */
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
    }

    if(hash)
        data = (icon_data *)g_hash_table_lookup(hash, user_name);

    /* in this function, we put an icon for pending marks. we should
     * not invalidate the icon here, otherwise it may result in
     * thrashing. --yaz */

    if(!data || !data->pixbuf) {
        return;
    }

    /* insert icon actually */
    if(purple_prefs_get_bool(OPT_SHOW_ICON)) {
        gtk_text_buffer_insert_pixbuf(target_buffer,
                                      &insertion_point,
                                      data->pixbuf);
        data->use_count++;
    }
    gtk_text_buffer_delete_mark(target_buffer, requested_mark);
    requested_mark = NULL;
}

static void
insert_requested_icon(const gchar *user_name, gint service)
{
    icon_data *data = NULL;
    GList *mark_list = NULL;
    GHashTable *hash = NULL;

    twitter_debug("called\n");

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
        data = (icon_data *)g_hash_table_lookup(hash, user_name);

    if(!data)
        return;

    mark_list = data->request_list;

    got_icon_data *gotdata = g_new0(got_icon_data, 1);
    gotdata->user_name = g_strdup(user_name);
    gotdata->service = service;

    twitter_debug("about to insert icon for pending requests\n");

    if(mark_list) {
        g_list_foreach(mark_list, (GFunc) insert_icon_at_mark, gotdata);
        mark_list = g_list_remove_all(mark_list, NULL);
        g_list_free(mark_list);
        data->request_list = NULL;
    }

    g_free(gotdata->user_name);
    g_free(gotdata);
}

/* this function will be called when profile page has been retrieved */
static void
got_page_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data,
            const gchar *url_text, gsize len, const gchar *error_message)
{
    got_icon_data *gotdata = (got_icon_data *)user_data;
    gchar *user_name = gotdata->user_name;
    gint service = gotdata->service;
    GMatchInfo *match_info = NULL;
    icon_data *data = NULL;
    gchar *url = NULL;
    gint regp_id = -1;

    twitter_debug("called\n");

    if(service == twitter_service) {
        data = (icon_data *)g_hash_table_lookup(
            icon_hash[service], user_name);
        regp_id = IMAGE_TWITTER;
    }
    else if(service == wassr_service) {
        data = (icon_data *)g_hash_table_lookup(
            icon_hash[service], user_name);
        regp_id = IMAGE_WASSR;
    }
    else if(service == identica_service) {
        data = (icon_data *)g_hash_table_lookup(
            icon_hash[service], user_name);
        regp_id = IMAGE_IDENTICA;
    }
    else if(service == jisko_service) {
        data = (icon_data *)g_hash_table_lookup(
            icon_hash[service], user_name);
        regp_id = IMAGE_JISKO;
    }
    else if(service == ffeed_service) {
        data = (icon_data *)g_hash_table_lookup(
            icon_hash[service], user_name);
        regp_id = IMAGE_FFEED;
    }

    /* retrieved nothing or got a bad response */
    if(!url_text ||
       (!strstr(url_text, "HTTP/1.1 200 OK") &&
        !strstr(url_text, "HTTP/1.0 200 OK"))) {
        if(data) {
            data->requested = FALSE;
            data->fetch_data = NULL;
        }
        g_free(gotdata->user_name);
        g_free(gotdata);
        return;
    }

    /* setup image url */
    g_regex_match(regp[regp_id], url_text, 0, &match_info);
    if(!g_match_info_matches(match_info)) {
        g_match_info_free(match_info);

        if(service == twitter_service) {
            twitter_debug("fall back to twitter default icon: %s\n",
                          gotdata->user_name);
            url = g_strdup(TWITTER_DEFAULT_ICON_URL);
        }
        else if(service == identica_service) {
            twitter_debug("fall back to identica default icon: %s\n",
                          gotdata->user_name);
            url = g_strdup(IDENTICA_DEFAULT_ICON_URL);
        }
        else if(service == jisko_service) {
            twitter_debug("fall back to jisko default icon: %s\n",
                          gotdata->user_name);
            url = g_strdup(JISKO_DEFAULT_ICON_URL);
        }
        else if(service == ffeed_service) {
            twitter_debug("fall back to ffeed default icon: %s\n",
                          gotdata->user_name);
            url = g_strdup(FFEED_DEFAULT_ICON_URL);
        }
        else {
            twitter_debug("no image url found\n");
            if(data) {
                data->requested = FALSE;
                data->fetch_data = NULL;
            }
            g_free(gotdata->user_name);
            g_free(gotdata);
            return;
        }
    }
    else {
        url = g_match_info_fetch(match_info, 1);
        g_match_info_free(match_info);
    }

    /* separate url into basename and the rest */
    gchar *slash = strrchr(url, '/');
    *slash = '\0';

    gchar *lower = g_ascii_strdown(slash+1, -1);

    if(strstr(lower, ".png"))
        data->img_type = "png";
    else if(strstr(lower, ".gif"))
        data->img_type = "gif";
    else if(strstr(lower, ".jpg") || strstr(lower, ".jpeg"))
        data->img_type = "jpg";

    g_free(lower);

    gchar *tmp;
    /* url encode basename. twitter needs this. */
    if(service == twitter_service)
        tmp = g_strdup_printf("%s/%s", url,
                              purple_url_encode(slash+1));
    else if(service == wassr_service) {
        gchar *tmp0 = NULL;
        tmp0 = g_regex_replace(regp[SIZE_128_WASSR], slash+1,
                                     -1, 0, ".64.", 0, NULL);
        tmp = g_strdup_printf("http://wassr.jp%s/%s", url,
                              tmp0 ? tmp0 : slash+1);
        g_free(tmp0);
    }
    else {
        tmp = g_strdup_printf("%s/%s", url, slash+1);
    }

    g_free(url);
    url = tmp;

    /* if requesting icon url is the same as old, return. */
    if(data && data->pixbuf &&
       url && data->icon_url && !strcmp(data->icon_url, url)) {
        twitter_debug("old url = %s new url = %s\n", data->icon_url, url);
        data->requested = FALSE;
        data->fetch_data = NULL;
        g_free(url);
        return;
    }

    if(data && data->pixbuf) {
        gdk_pixbuf_unref(data->pixbuf);
        data->pixbuf = NULL;
    }

    g_free(data->icon_url);
    data->icon_url = g_strdup(url);

    data->use_count = 0;
    data->mtime = time(NULL); /* xxx is there a better way? */

    twitter_debug("requested url=%s\n", url);

    /* request fetch image */
    if(url) {
        /* reuse gotdata. just pass given one */
        /* gotdata will be released in got_icon_cb */
        data->fetch_data =
            purple_util_fetch_url_request(url, TRUE, NULL, TRUE, NULL,
                                          FALSE, got_icon_cb, gotdata);
        twitter_debug("request %s's icon\n", user_name);
        g_free(url);
    }
}

static GdkPixbuf *
make_scaled_pixbuf(const gchar *url_text, gsize len)
{
    /* make pixbuf */
    GdkPixbufLoader *loader;
    GdkPixbuf *src = NULL, *dest = NULL;
    gint size;

    g_return_val_if_fail(url_text != NULL, NULL);
    g_return_val_if_fail(len > 0, NULL);

    loader = gdk_pixbuf_loader_new();
    gdk_pixbuf_loader_write(loader, (guchar *)url_text, len, NULL);
    gdk_pixbuf_loader_close(loader, NULL);

    src = gdk_pixbuf_loader_get_pixbuf(loader);
    if(!src)
        return NULL;

    size = purple_prefs_get_int(OPT_ICON_SIZE);
    if(size == 0)
        size = DEFAULT_ICON_SIZE;

    dest = gdk_pixbuf_scale_simple(src, size, size, GDK_INTERP_HYPER);
    gdk_pixbuf_unref(src);

    return dest;
}

static gchar *ext_list[] = {
    "png",
    "gif",
    "jpg",
    NULL
};

/* this function will be called when requested icon has been retrieved */
static void
got_icon_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data,
            const gchar *url_text, gsize len, const gchar *error_message)
{
    got_icon_data *gotdata = (got_icon_data *)user_data;
    gchar *user_name = gotdata->user_name;
    gint service = gotdata->service;

    icon_data *data = NULL;
    GHashTable *hash = NULL;
    GdkPixbuf *pixbuf = NULL;
    const gchar *dirname = NULL;

    twitter_debug("called: service = %d\n", service);

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
    }

    if(hash)
        data = (icon_data *)g_hash_table_lookup(hash, user_name);

    if(!data) {
        twitter_debug("cannot retrieve icon_data from hash (should not be called)\n");
        goto fin_got_icon_cb;
    }

    /* return if download failed */
    if(!url_text) {
        twitter_debug("downloading %s's icon failed : %s\n",
                      user_name, error_message);

        data->requested = FALSE;

        goto fin_got_icon_cb;
    }

    /* remove download request */
    data->requested = FALSE;
    data->fetch_data = NULL;

    /* return if user's icon has been downloaded */
    if(data->pixbuf) {
        twitter_debug("%s's icon has already been downloaded\n",
                      user_name);
        goto fin_got_icon_cb;
    }

    pixbuf = make_scaled_pixbuf(url_text, len);

    if(!pixbuf) {
        twitter_debug("cannot make pixbuf from downloaded data\n");

        /* ask to download default icon instead */
        got_icon_data *gotdata2 = g_new0(got_icon_data, 1);
        const gchar *url = NULL;

        gotdata2->user_name = g_strdup(gotdata->user_name);
        gotdata2->service = service;

        switch(service) {
        case twitter_service:
            url = TWITTER_DEFAULT_ICON_URL;
            break;
        case identica_service:
            url = IDENTICA_DEFAULT_ICON_URL;
            break;
        case jisko_service:
            url = JISKO_DEFAULT_ICON_URL;
        case ffeed_service:
            url = FFEED_DEFAULT_ICON_URL;
            break;
        }

        g_free(data->icon_url);
        data->icon_url = g_strdup(url);

        data->requested = TRUE;
        data->fetch_data =
            purple_util_fetch_url_request(url, TRUE, NULL, TRUE, NULL,
                                          FALSE, got_icon_cb, gotdata2);
        goto fin_got_icon_cb;
    }

    data->pixbuf = pixbuf;

    twitter_debug("new icon pixbuf = %p size = %d\n",
                  pixbuf,
                  gdk_pixbuf_get_rowstride(pixbuf) *
                  gdk_pixbuf_get_height(pixbuf));

    if(hash)
        g_hash_table_insert(hash, g_strdup(user_name), data);

    /* find out extension */
    gchar *slash = strrchr(url_data->url, '/');
    gchar *lower = g_ascii_strdown(slash+1, -1);

    if(strstr(lower, ".png"))
        data->img_type = "png";
    else if(strstr(lower, ".gif"))
        data->img_type = "gif";
    else if(strstr(lower, ".jpg") || strstr(lower, ".jpeg"))
        data->img_type = "jpg";

    g_free(lower);

    dirname = purple_prefs_get_string(OPT_ICON_DIR);

    /* store retrieved image to a file in icon dir */
    if(ensure_path_exists(dirname)) {
        gchar *filename = NULL;
        gchar *path = NULL;
        const gchar *suffix = NULL;
        gchar **extp;

        switch(service) {
        case twitter_service:
            suffix = "twitter";
            break;
        case wassr_service:
            suffix = "wassr";
            break;
        case identica_service:
            suffix = "identica";
            break;
        case jisko_service:
            suffix = "jisko";
            break;
        case ffeed_service:
            suffix = "ffeed";
            break;
        default:
            twitter_debug("unknown service\n");
            break;
        }

        /* remove old file first */
        for(extp = ext_list; *extp; extp++) {
            filename = g_strdup_printf("%s_%s.%s",
                                       user_name, suffix, *extp);
            path = g_build_filename(dirname, filename, NULL);
            g_remove(path);

            g_free(filename);
            g_free(path);
        }

        /* setup path */
        filename = g_strdup_printf("%s_%s.%s",
                                   user_name, suffix, data->img_type);

        path = g_build_filename(dirname, filename, NULL);
        g_free(filename); filename = NULL;

        g_file_set_contents(path, url_text, len, NULL);
        g_free(path); path = NULL;

        data->mtime = time(NULL);
    }

    twitter_debug("Downloading %s's icon has been complete.\n",
        user_name);

    /* Insert the icon to messages that has been received. */
    insert_requested_icon(user_name, service);

fin_got_icon_cb:
    g_free(gotdata->user_name);
    g_free(gotdata);
}

void
request_icon(const char *user_name, gint service, gboolean renew)
{
    gchar *url = NULL;

    /* look local icon cache for the requested icon */
    gchar *path = NULL;
    icon_data *data = NULL;
    GHashTable *hash = NULL;
    const gchar *suffix = NULL;

    switch(service) {
    case twitter_service:
        hash = icon_hash[service];
        suffix = "twitter";
        break;
    case wassr_service:
        hash = icon_hash[service];
        suffix = "wassr";
        break;
    case identica_service:
        hash = icon_hash[service];
        suffix = "identica";
        break;
    case jisko_service:
        hash = icon_hash[service];
        suffix = "jisko";
        break;
    case ffeed_service:
        hash = icon_hash[service];
        suffix = "ffeed";
        break;
    default:
        twitter_debug("unknown service\n");
        break;
    }

    if(!hash)
        return;

    /* since this function is called after mark_icon_for_user(), data
     * must exist here. */
    data = (icon_data *)g_hash_table_lookup(hash, user_name);

    /* if the image is in a hash, just return */
    if(data && data->pixbuf && !renew)
        return;

    /* check if saved file exists */
    if(suffix && !renew) {
        gchar *filename = NULL;
        gchar **extp;

        for(extp = ext_list; *extp; extp++) {
            filename = g_strdup_printf("%s_%s.%s", user_name, suffix, *extp);
            path = g_build_filename(purple_prefs_get_string(OPT_ICON_DIR),
                                    filename, NULL);
            g_free(filename);

            twitter_debug("path = %s\n", path);

            /* build image from file, if file exists */
            if(g_file_test(path, G_FILE_TEST_EXISTS)) {
                gchar *imgdata = NULL;
                size_t len;
                GError *err = NULL;
                GdkPixbuf *pixbuf = NULL;
                struct stat buf;

                if (!g_file_get_contents(path, &imgdata, &len, &err)) {
                    twitter_debug("Error reading %s: %s\n",
                                  path, err->message);
                    g_error_free(err);
                }

                if(stat(path, &buf))
                    data->mtime = buf.st_mtime;

                pixbuf = make_scaled_pixbuf(imgdata, len);
                g_free(imgdata);

                if(pixbuf) {
                    data->pixbuf = pixbuf;

                    twitter_debug("new icon pixbuf = %p size = %d\n",
                                  pixbuf,
                                  gdk_pixbuf_get_rowstride(pixbuf) *
                                  gdk_pixbuf_get_height(pixbuf));

                    data->img_type = *extp;

                    twitter_debug("icon data has been loaded from file\n");
                    insert_requested_icon(user_name, service);
                }

                g_free(path);
                return;
            }

            g_free(path);

        } /* for */
    } /* suffix */

    /* Return if user's icon has been requested already. */
    if(data->requested)
        return;
    else
        data->requested = TRUE;

    /* Create the URL for an user's icon. */
    switch(service) {
    case twitter_service:
        url = g_strdup_printf("http://twitter.com/%s", user_name);
        break;
    case wassr_service:
        url = g_strdup_printf("http://wassr.jp/user/%s", user_name);
        break;
    case identica_service: /* make use of Version0 API */
        url = g_strdup_printf("http://identi.ca/%s/avatar/96", user_name);
        break;
    case jisko_service:
        url = g_strdup_printf("http://jisko.net/%s", user_name);
        break;
    case ffeed_service:
        url = g_strdup_printf("http://friendfeed.com/%s", user_name);
        break;
    default:
        twitter_debug("unknown service\n");
        break;
    }

    if(url) {
        got_icon_data *gotdata = g_new0(got_icon_data, 1);
        gotdata->user_name = g_strdup(user_name);
        gotdata->service = service;

        /* gotdata will be released in got_icon_cb */
        if(service == twitter_service ||
           service == wassr_service ||
           service == jisko_service ||
           service == ffeed_service) {
            data->fetch_data =
                purple_util_fetch_url_request(url, TRUE, NULL, TRUE, NULL,
                                              TRUE, got_page_cb, gotdata);
        }
        else { /* typically, identica_service */
            data->fetch_data =
                purple_util_fetch_url_request(url, TRUE, NULL, TRUE, NULL,
                                              FALSE, got_icon_cb, gotdata);
        }
        g_free(url); url = NULL;

        twitter_debug("request %s's icon\n", user_name);
    }
}

void
mark_icon_for_user(GtkTextMark *mark, const gchar *user_name, gint service)
{
    icon_data *data = NULL;
    GHashTable *hash = NULL;

    twitter_debug("called\n");

    switch(service) {
    case twitter_service:
        hash = icon_hash[twitter_service];
        break;
    case wassr_service:
        hash = icon_hash[wassr_service];
        break;
    case identica_service:
        hash = icon_hash[identica_service];
        break;
    case jisko_service:
        hash = icon_hash[jisko_service];
        break;
    case ffeed_service:
        hash = icon_hash[ffeed_service];
        break;
    default:
        twitter_debug("unknown service\n");
        break;
    }

    if(hash)
        data = (icon_data *)g_hash_table_lookup(hash, user_name);

    /* proper place to allocate icon_data */
    if(!data) {
        data = g_new0(icon_data, 1);
        g_hash_table_insert(hash, g_strdup(user_name), data);
    }

    data->request_list = g_list_prepend(data->request_list, mark);
}

void
invalidate_icon_data_func(gpointer key, gpointer value, gpointer user_data)
{
    icon_data *data = (icon_data *)value;

    g_return_if_fail(data != NULL);

    g_object_unref(data->pixbuf);
    data->pixbuf = NULL;
}
