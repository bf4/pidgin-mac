#include "pidgin-twitter.h"

extern GRegex *regp[];

/* prototypes */
static gchar *twitter_memrchr(const gchar *s, int c, size_t n);


/* functions */

/* this function has been taken from autoaccept plugin */
gboolean
ensure_path_exists(const char *dir)
{
	if (!g_file_test(dir, G_FILE_TEST_IS_DIR)) {
		if (purple_build_dir(dir, S_IRUSR | S_IWUSR | S_IXUSR))
			return FALSE;
	}

	return TRUE;
}

static gchar *
twitter_memrchr(const gchar *s, int c, size_t n)
{
    int nn = n;

    g_return_val_if_fail(s != NULL, NULL);

    while(nn+1) {
        if((int)*(s+nn) == c)
            return (gchar *)(s+nn);
        nn--;
    }
    return NULL;
}

static gchar *html_tags[] = {
    "<a ",
    "</a>",
    "<b>",
    "</b>",
    "<p>",
    "</p>",
    "<div ",
    "</div>",
    "<span ",
    "</span>",
    "<body>",
    "<body ",
    "</body>",
    "<i>",
    "</i>",
    "<font ",
    "</font>",
    "<br>",
    "<br/>",
    "<img ",
    "<html>",
    "<html ",
    "</html>",
    NULL
};

gchar *
strip_html_markup(const gchar *src)
{
    gchar *head, *tail;     /* head and tail of html */
    gchar *begin, *end;     /* begin:<  end:> */
    gchar *html, *str;      /* copied src and string to be returned */
/*    gchar *vis1, *vis2; */     /* begin and end of address part */
    gchar *startp;          /* starting point marker */
    gchar **tagp;           /* tag iterator */
    gchar *tmp, *tmp2;      /* scratches */

    g_return_val_if_fail(src != NULL, NULL);

    const gchar *ptr, *ent;
    gchar *ptr2;
    gint entlen;

    /* unescape &x; */
    html = g_malloc0(strlen(src) + 1);
    ptr2 = html;
    for(ptr = src; *ptr; ) {
        if(*ptr == '&') {
            ent = purple_markup_unescape_entity(ptr, &entlen);
            if(ent != NULL) {
                while(*ent) {
                    *ptr2++ = *ent++;
                }
                ptr += entlen;
            }
            else {
                *ptr2++ = *ptr++;
            }
        }
        else {
            *ptr2++ = *ptr++;
        }
    } /* for */

    str = g_strdup("\0");

    head = html;
    tail = head + strlen(html);
    startp = head;

loop:
    begin = NULL;
    end = NULL;

    if(startp >= tail) {
        g_free(html);
        return str;
    }

    end = strchr(startp, '>');
    if(end) {
        begin = twitter_memrchr(startp, '<', end - startp);
        if(begin < startp)
            begin = NULL;

        if(!begin) { /* '>' found but no corresponding '<' */
            tmp = g_strndup(startp, end - startp + 1); /* concat until '>' */
            tmp2 = g_strconcat(str, tmp, NULL);
            g_free(str);
            g_free(tmp);
            str = tmp2;
            startp = end + 1;
            goto loop;
        }
    }
    else { /* neither '>' nor '<' were found */
        tmp = g_strconcat(str, startp, NULL); /* concat the rest */
        g_free(str);
        str = tmp;
        g_free(html);
        return str;
    }

    /* here, both < and > are found */
    /* concatenate leading part to dest */
    tmp = g_strndup(startp, begin - startp);
    tmp2 = g_strconcat(str, tmp, NULL);
    g_free(tmp);
    g_free(str);
    str = tmp2;

    /* find tag */
    for(tagp = html_tags; *tagp; tagp++) {
        if(!g_ascii_strncasecmp(begin, *tagp, strlen(*tagp))) {
            /* we found a valid tag */
            /* if tag is <a href=, extract address. */
#if 0
            if(!strcmp(*tagp, "<a href=")) {
                vis1 = NULL; vis2 = NULL;

                vis1 = strchr(begin, '\'');
                if(vis1)
                    vis2 = strchr(vis1+1, '\'');
                if(!vis1) {
                    vis1 = strchr(begin, '\"');
                    if(vis1)
                        vis2 = strchr(vis1+1, '\"');
                }
                if(vis1 && vis2) {
                    *vis2 = '\0';
                    /* generate "[ http://example.com/ ] anchor " */
                    tmp = g_strconcat(str, "[ ", vis1+1, " ]", " ", NULL);
                    g_free(str);
                    str = tmp;
                }
                startp = end + 1;
                goto loop;
            } /* <a href= */
            else {
                /* anything else: discard whole <>. */
                startp = end + 1;
                goto loop;
            }
#else
            /* anything else: discard whole <>. */
            startp = end + 1;
            goto loop;
#endif
        }  /* valid tag */
    }

    /* no valid tag was found: copy <brabra> */
    tmp = g_strndup(begin, end - begin + 1);
    tmp2 = g_strconcat(str, tmp, NULL);
    g_free(tmp);
    g_free(str);
    str = tmp2;
    startp = end + 1;
    goto loop;
}

/* string utilities */
void
escape(gchar **str)
{
    GMatchInfo *match_info = NULL;
    gchar *newstr = NULL, *match = NULL;
    gboolean flag = FALSE;

    /* search genuine command */
    g_regex_match(regp[COMMAND], *str, 0, &match_info);
    while(g_match_info_matches(match_info)) {
        match = g_match_info_fetch(match_info, 1);
        twitter_debug("command = %s\n", match);
        g_free(match);
        g_match_info_next(match_info, NULL);
        flag = TRUE;
    }
    g_match_info_free(match_info);
    match_info = NULL;

    if(flag)
        return;

    /* if not found, check pseudo command */
    g_regex_match(regp[PSEUDO], *str, 0, &match_info);
    while(g_match_info_matches(match_info)) {
        match = g_match_info_fetch(match_info, 1);
        twitter_debug("pseudo = %s\n", match);
        g_free(match);
        g_match_info_next(match_info, NULL);
        flag = TRUE;
    }
    g_match_info_free(match_info);
    match_info = NULL;

    /* if there is pseudo one, escape it */
    if(flag) {
        /* put ". " to the beginning of buffer */
        newstr = g_strdup_printf(". %s", *str);
        twitter_debug("*str = %s newstr = %s\n", *str, newstr);
        g_free(*str);
        *str = newstr;
    }
}

void
strip_markup(gchar **str, gboolean escape)
{
    gchar *plain;

    plain = strip_html_markup(*str);
    g_free(*str);
    if(escape) {
        *str = g_markup_escape_text(plain, -1);
        g_free(plain);
    }
    else {
        *str = plain;
    }
    twitter_debug("result=%s\n", *str);
}

gboolean
is_twitter_account(PurpleAccount *account, const char *name)
{
    const gchar *proto = purple_account_get_protocol_id(account);

    if(g_strstr_len(name,  19, "twitter@twitter.com") &&
       g_strstr_len(proto, 11, "prpl-jabber")) {
        return TRUE;
    }

    return FALSE;
}

gboolean
is_twitter_conv(PurpleConversation *conv)
{
    g_return_val_if_fail(conv != NULL, FALSE);

    const char *name = purple_conversation_get_name(conv);
    PurpleAccount *account = purple_conversation_get_account(conv);

    return is_twitter_account(account, name);
}

gboolean
is_wassr_account(PurpleAccount *account, const char *name)
{
    const gchar *proto = purple_account_get_protocol_id(account);

    if(g_strstr_len(name,  18, "wassr-bot@wassr.jp") &&
       g_strstr_len(proto, 11, "prpl-jabber")) {
        return TRUE;
    }

    return FALSE;
}

gboolean
is_wassr_conv(PurpleConversation *conv)
{
    g_return_val_if_fail(conv != NULL, FALSE);

    const char *name = purple_conversation_get_name(conv);
    PurpleAccount *account = purple_conversation_get_account(conv);

    return is_wassr_account(account, name);
}

gboolean
is_identica_account(PurpleAccount *account, const char *name)
{
    const gchar *proto = purple_account_get_protocol_id(account);

    if(g_strstr_len(name,  16, "update@identi.ca") &&
       g_strstr_len(proto, 11, "prpl-jabber")) {
        return TRUE;
    }

    return FALSE;
}

gboolean
is_identica_conv(PurpleConversation *conv)
{
    g_return_val_if_fail(conv != NULL, FALSE);

    const char *name = purple_conversation_get_name(conv);
    PurpleAccount *account = purple_conversation_get_account(conv);

    return is_identica_account(account, name);
}

gboolean
is_jisko_account(PurpleAccount *account, const char *name)
{
    const gchar *proto = purple_account_get_protocol_id(account);

    if(g_strstr_len(name,  16, "bot@jisko.net") &&
       g_strstr_len(proto, 11, "prpl-jabber")) {
        return TRUE;
    }

    return FALSE;
}

gboolean
is_jisko_conv(PurpleConversation *conv)
{
    g_return_val_if_fail(conv != NULL, FALSE);

    const char *name = purple_conversation_get_name(conv);
    PurpleAccount *account = purple_conversation_get_account(conv);

    return is_jisko_account(account, name);
}

gboolean
is_ffeed_account(PurpleAccount *account, const char *name)
{
    const gchar *proto = purple_account_get_protocol_id(account);

    if(g_strstr_len(name,  22, "ff@chat.friendfeed.com") &&
       g_strstr_len(proto, 11, "prpl-jabber")) {
        return TRUE;
    }

    return FALSE;
}

gboolean
is_ffeed_conv(PurpleConversation *conv)
{
    g_return_val_if_fail(conv != NULL, FALSE);

    const char *name = purple_conversation_get_name(conv);
    PurpleAccount *account = purple_conversation_get_account(conv);

    return is_ffeed_account(account, name);
}

gint
get_service_type_by_account(PurpleAccount *account, const char *sender)
{
    gint service = unknown_service;

    g_return_val_if_fail(account != NULL, unknown_service);
    g_return_val_if_fail(sender != NULL, unknown_service);

    if(is_twitter_account(account, sender))
        service = twitter_service;
    else if(is_wassr_account(account, sender))
        service = wassr_service;
    else if(is_identica_account(account, sender))
        service = identica_service;
    else if(is_jisko_account(account, sender))
        service = jisko_service;
    else if(is_ffeed_account(account, sender))
        service = ffeed_service;

    return service;
}

gint
get_service_type(PurpleConversation *conv)
{
    gint service = unknown_service;

    g_return_val_if_fail(conv != NULL, unknown_service);

    if(is_twitter_conv(conv))
        service = twitter_service;
    else if(is_wassr_conv(conv))
        service = wassr_service;
    else if(is_identica_conv(conv))
        service = identica_service;
    else if(is_jisko_conv(conv))
        service = jisko_service;
    else if(is_ffeed_conv(conv))
        service = ffeed_service;

    return service;
}
