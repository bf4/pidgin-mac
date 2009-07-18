#include "pidgin-twitter.h"
#include "config.h"

extern GHashTable *icon_hash[];
extern source_t source;


/* prototypes */
static void icon_size_prefs_cb(const char *name, PurplePrefType type, gconstpointer val, gpointer data);
static void interval_prefs_cb(const char *name, PurplePrefType type, gconstpointer val, gpointer data);
static void text_changed_cb(gpointer *data);
static void bool_toggled_cb(gpointer *data);
static void spin_changed_cb(gpointer *data);
static void combo_changed_cb(gpointer *data);
static void disconnect_prefs_cb(GtkObject *object, gpointer data);
static void counter_prefs_cb(const char *name, PurplePrefType type, gconstpointer val, gpointer data);


static void
counter_prefs_cb(const char *name, PurplePrefType type,
                 gconstpointer val, gpointer data)
{
    gboolean enabled = purple_prefs_get_bool(OPT_COUNTER);

    if(enabled)
        attach_to_window();
    else
        detach_from_window();
}

static void
icon_size_prefs_cb(const char *name, PurplePrefType type,
                   gconstpointer val, gpointer data)
{
    int i;

    /* invalidate icon cache */
    for(i = twitter_service; i < NUM_SERVICES; i++) {
        g_hash_table_foreach(icon_hash[i],
                             (GHFunc)invalidate_icon_data_func, NULL);
    }
}

static void
interval_prefs_cb(const char *name, PurplePrefType type,
                   gconstpointer val, gpointer data)
{
    /* remove idle func */
    g_source_remove_by_user_data((gpointer)(source.conv));

    /* add idle func */
    if(purple_prefs_get_bool(OPT_API_BASE_POST)) {
        source.id = g_timeout_add_seconds(
            purple_prefs_get_int(OPT_API_BASE_GET_INTERVAL),
            get_status_with_api, (gpointer)(source.conv));
    }
}

static void
text_changed_cb(gpointer *data)
{
    const gchar *text;
    gchar *pref = (gchar *)g_object_get_data(G_OBJECT(data), "pref");
    text = gtk_entry_get_text(GTK_ENTRY(data));
    purple_prefs_set_string(pref, text);
}

static void
bool_toggled_cb(gpointer *data)
{
    gchar *pref = (gchar *)g_object_get_data(G_OBJECT(data), "pref");
    gboolean value = purple_prefs_get_bool(pref);
    purple_prefs_set_bool(pref, !value);
}

static void
spin_changed_cb(gpointer *data)
{
    gchar *pref = (gchar *)g_object_get_data(G_OBJECT(data), "pref");

    twitter_debug("called\n");

    purple_prefs_set_int(pref,
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data)));
}

static void
combo_changed_cb(gpointer *data)
{
    gint position;
    gchar *pref = (gchar *)g_object_get_data(G_OBJECT(data), "pref");
    position = gtk_combo_box_get_active(GTK_COMBO_BOX(data));
    purple_prefs_set_int(pref, position);
}

static void
disconnect_prefs_cb(GtkObject *object, gpointer data)
{
	PurplePlugin *plugin = (PurplePlugin *)data;

	purple_prefs_disconnect_by_handle(plugin);
}

static void
api_base_post_cb(const char *name, PurplePrefType type, gconstpointer value,
                 gpointer data)
{
    signed_on_cb(NULL);
    get_status_with_api((gpointer)(source.conv));
}

GtkWidget *
prefs_get_frame(PurplePlugin *plugin)
{
    GtkBuilder *builder;
    GError *err = NULL;
    gchar *filename;
    GtkWidget *window, *notebook, *e;
    const gchar *text;
    GtkSpinButton *spin;
    GtkObject *adjust;
    gint value;
#ifdef _WIN32
    extern char binary_prefs_ui_start[];
    extern char binary_prefs_ui_size[];
#endif

    builder = gtk_builder_new();

#ifdef _WIN32
    gtk_builder_add_from_string(builder, binary_prefs_ui_start,
                                (int)binary_prefs_ui_size, NULL);
#else
    filename = g_build_filename(DATADIR,
                                "pidgin-twitter", "prefs.ui", NULL);
    gtk_builder_add_from_file(builder, filename, &err);
#endif

    if(err) {
        twitter_debug("%s\n", err->message);
        g_free(filename);
        return NULL;
    }

    g_free(filename);

    gtk_builder_connect_signals(builder, NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "prefswindow"));
    notebook = GTK_WIDGET(gtk_builder_get_object(builder, "prefsnotebook"));

    gtk_container_remove(GTK_CONTAINER(window), notebook);

    g_signal_connect(notebook, "destroy",
                     G_CALLBACK(disconnect_prefs_cb), plugin);


    /**********************/
    /* connect to signals */
    /**********************/

    /****************/
    /* account page */
    /****************/
    e = GTK_WIDGET(gtk_builder_get_object (builder, "account_twitter"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_SCREEN_NAME_TWITTER);
    text = purple_prefs_get_string(OPT_SCREEN_NAME_TWITTER);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "account_wassr"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_SCREEN_NAME_WASSR);
    text = purple_prefs_get_string(OPT_SCREEN_NAME_WASSR);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "account_identica"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_SCREEN_NAME_IDENTICA);
    text = purple_prefs_get_string(OPT_SCREEN_NAME_IDENTICA);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "account_jisko"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_SCREEN_NAME_JISKO);
    text = purple_prefs_get_string(OPT_SCREEN_NAME_JISKO);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "account_ffeed"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_SCREEN_NAME_FFEED);
    text = purple_prefs_get_string(OPT_SCREEN_NAME_FFEED);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "account_api"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_API_BASE_POST);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_API_BASE_POST));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);
    purple_prefs_connect_callback(plugin, OPT_API_BASE_POST, /* xxx divide? */
                                  api_base_post_cb, NULL);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "account_api_password"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_PASSWORD_TWITTER);

    gtk_entry_set_visibility(GTK_ENTRY(e), FALSE);
    if (gtk_entry_get_invisible_char(GTK_ENTRY(e)) == '*')
        gtk_entry_set_invisible_char(GTK_ENTRY(e), PIDGIN_INVISIBLE_CHAR);

    text = purple_prefs_get_string(OPT_PASSWORD_TWITTER);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);


    /* interval spin */
    e = GTK_WIDGET(gtk_builder_get_object (builder,
                       "account_api_get_interval_spin"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_API_BASE_GET_INTERVAL);

    spin = GTK_SPIN_BUTTON(e);

    value = purple_prefs_get_int(OPT_API_BASE_GET_INTERVAL);
    twitter_debug("spin value = %d\n", value);

	adjust = gtk_adjustment_new(value, 40, 3600, 10, 100, 100);
    gtk_spin_button_set_adjustment(spin, GTK_ADJUSTMENT(adjust));
    gtk_widget_set_size_request(GTK_WIDGET(spin), 50, -1);

    if(value == 0) {
        value = TWITTER_DEFAULT_INTERVAL;
        purple_prefs_set_int(OPT_API_BASE_GET_INTERVAL, value);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(e), (gdouble)value);
    g_signal_connect(e, "value-changed",
                     G_CALLBACK(spin_changed_cb), &e);
    purple_prefs_connect_callback(plugin, OPT_API_BASE_GET_INTERVAL,
                                  interval_prefs_cb, NULL);


    /* count spin */
    e = GTK_WIDGET(gtk_builder_get_object (builder,
                       "account_api_get_count_spin"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_RETRIEVE_COUNT);

    spin = GTK_SPIN_BUTTON(e);

    value = purple_prefs_get_int(OPT_RETRIEVE_COUNT);
    twitter_debug("spin value = %d\n", value);

	adjust = gtk_adjustment_new(value, 20, 200, 10, 100, 100);
    gtk_spin_button_set_adjustment(spin, GTK_ADJUSTMENT(adjust));
    gtk_widget_set_size_request(GTK_WIDGET(spin), 50, -1);

    if(value == 0) {
        value = TWITTER_DEFAULT_RETRIEVE_COUNT;
        purple_prefs_set_int(OPT_RETRIEVE_COUNT, value);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(e), (gdouble)value);
    g_signal_connect(e, "value-changed",
                     G_CALLBACK(spin_changed_cb), &e);


    /********************/
    /* translation page */
    /********************/
    e = GTK_WIDGET(gtk_builder_get_object (builder, "translation_recipient"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_TRANSLATE_RECIPIENT);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_TRANSLATE_RECIPIENT));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "translation_sender"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_TRANSLATE_SENDER);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_TRANSLATE_SENDER));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "translation_channel"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_TRANSLATE_CHANNEL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_TRANSLATE_CHANNEL));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);



    /***************/
    /* filter page */
    /***************/
    e = GTK_WIDGET(gtk_builder_get_object (builder, "filter_filter_check"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_FILTER);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_FILTER));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "filter_exclude_reply_check"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_FILTER_EXCLUDE_REPLY);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_FILTER_EXCLUDE_REPLY));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "filter_twitter"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_FILTER_TWITTER);
    text = purple_prefs_get_string(OPT_FILTER_TWITTER);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "filter_wassr"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_FILTER_WASSR);
    text = purple_prefs_get_string(OPT_FILTER_WASSR);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "filter_identica"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_FILTER_IDENTICA);
    text = purple_prefs_get_string(OPT_FILTER_IDENTICA);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "filter_jisko"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_FILTER_JISKO);
    text = purple_prefs_get_string(OPT_FILTER_JISKO);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "filter_ffeed"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_FILTER_FFEED);
    text = purple_prefs_get_string(OPT_FILTER_FFEED);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);



    /*************/
    /* icon page */
    /*************/
    e = GTK_WIDGET(gtk_builder_get_object (builder, "icon_show_icon"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_SHOW_ICON);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_SHOW_ICON));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);

    /* icon size spin */
    e = GTK_WIDGET(gtk_builder_get_object (builder, "icon_icon_size_spin"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_ICON_SIZE);

    spin = GTK_SPIN_BUTTON(e);

    value = purple_prefs_get_int(OPT_ICON_SIZE);
    twitter_debug("spin value = %d\n", value);

	adjust = gtk_adjustment_new(value, 16, 128, 4, 4, 4);
    gtk_spin_button_set_adjustment(spin, GTK_ADJUSTMENT(adjust));
    gtk_widget_set_size_request(GTK_WIDGET(spin), 50, -1);

    if(value == 0) {
        value = DEFAULT_ICON_SIZE;
        purple_prefs_set_int(OPT_ICON_SIZE, value);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(e), (gdouble)value);
    g_signal_connect(e, "value-changed",
                     G_CALLBACK(spin_changed_cb), &e);
    purple_prefs_connect_callback(plugin, OPT_ICON_SIZE,
                                  icon_size_prefs_cb, NULL);

    /* enable update */
    e = GTK_WIDGET(gtk_builder_get_object (builder, "icon_enable_update"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_UPDATE_ICON);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_UPDATE_ICON));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);

    /* max count spin */
    e = GTK_WIDGET(gtk_builder_get_object (builder, "icon_max_count_spin"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_ICON_MAX_COUNT);

    spin = GTK_SPIN_BUTTON(e);

    value = purple_prefs_get_int(OPT_ICON_MAX_COUNT);
    twitter_debug("spin value = %d\n", value);

	adjust = gtk_adjustment_new(value, 2, 10000, 1, 10, 10);
    gtk_spin_button_set_adjustment(spin, GTK_ADJUSTMENT(adjust));
    gtk_widget_set_size_request(GTK_WIDGET(spin), 50, -1);

    if(value == 0) {
        value = DEFAULT_ICON_MAX_COUNT;
        purple_prefs_set_int(OPT_ICON_MAX_COUNT, value);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(e), (gdouble)value);
    g_signal_connect(e, "value-changed",
                     G_CALLBACK(spin_changed_cb), &e);


    /* max days spin */
    e = GTK_WIDGET(gtk_builder_get_object (builder, "icon_max_days_spin"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_ICON_MAX_DAYS);

    spin = GTK_SPIN_BUTTON(e);

    value = purple_prefs_get_int(OPT_ICON_MAX_DAYS);
    twitter_debug("spin value = %d\n", value);

	adjust = gtk_adjustment_new(value, 1, 180, 1, 10, 10);
    gtk_spin_button_set_adjustment(spin, GTK_ADJUSTMENT(adjust));
    gtk_widget_set_size_request(GTK_WIDGET(spin), 50, -1);

    if(value == 0) {
        value = DEFAULT_ICON_MAX_DAYS;
        purple_prefs_set_int(OPT_ICON_MAX_DAYS, value);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(e), (gdouble)value);
    g_signal_connect(e, "value-changed",
                     G_CALLBACK(spin_changed_cb), &e);



    /**************/
    /* sound page */
    /**************/
    e = GTK_WIDGET(gtk_builder_get_object (builder, "sound_recip_check"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_PLAYSOUND_RECIPIENT);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_PLAYSOUND_RECIPIENT));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "sound_recip_list"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_USERLIST_RECIPIENT);
    text = purple_prefs_get_string(OPT_USERLIST_RECIPIENT);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);

    /* recipient combobox */
    e = GTK_WIDGET(gtk_builder_get_object (builder, "sound_recip_combo"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(e),
                             purple_prefs_get_int(OPT_SOUNDID_RECIPIENT));
    g_object_set_data(G_OBJECT(e), "pref", OPT_SOUNDID_RECIPIENT);
    g_signal_connect(e, "changed",
                     G_CALLBACK(combo_changed_cb), &e);



    e = GTK_WIDGET(gtk_builder_get_object (builder, "sound_send_check"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_PLAYSOUND_SENDER);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_PLAYSOUND_SENDER));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "sound_send_list"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_USERLIST_SENDER);
    text = purple_prefs_get_string(OPT_USERLIST_SENDER);
    gtk_entry_set_text(GTK_ENTRY(e), text);
    g_signal_connect(e, "changed",
                     G_CALLBACK(text_changed_cb), &e);

    /* sender combobox */
    e = GTK_WIDGET(gtk_builder_get_object (builder, "sound_send_combo"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(e),
                             purple_prefs_get_int(OPT_SOUNDID_RECIPIENT));
    g_object_set_data(G_OBJECT(e), "pref", OPT_SOUNDID_SENDER);
    g_signal_connect(e, "changed",
                     G_CALLBACK(combo_changed_cb), &e);




    /****************/
    /* utility page */
    /****************/
    e = GTK_WIDGET(gtk_builder_get_object (builder, "utility_counter"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_COUNTER);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_COUNTER));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);
    purple_prefs_connect_callback(plugin, OPT_COUNTER,
                                  counter_prefs_cb, NULL);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "utility_pseudo"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_ESCAPE_PSEUDO);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_ESCAPE_PSEUDO));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "utility_oops"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_SUPPRESS_OOPS);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_SUPPRESS_OOPS));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "utility_strip_excess_lf"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_STRIP_EXCESS_LF);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_STRIP_EXCESS_LF));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "utility_notify"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_PREVENT_NOTIFICATION);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_PREVENT_NOTIFICATION));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);

    e = GTK_WIDGET(gtk_builder_get_object (builder, "utility_log_output"));
    g_object_set_data(G_OBJECT(e), "pref", OPT_LOG_OUTPUT);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e),
                                 purple_prefs_get_bool(OPT_LOG_OUTPUT));
    g_signal_connect(e, "toggled",
                     G_CALLBACK(bool_toggled_cb), &e);


    /* all done */
    gtk_widget_show_all(notebook);
    return notebook;
}
