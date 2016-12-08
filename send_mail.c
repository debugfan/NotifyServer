/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2016, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/* <DESC>
 * SMTP example using SSL
 * </DESC>
 */

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include "time_utils.h"
#include "memory_stream.h"

/* This is a simple example showing how to send mail using libcurl's SMTP
 * capabilities. It builds on the smtp-mail.c example to add authentication
 * and, more importantly, transport security to protect the authentication
 * details from being snooped.
 *
 * Note that this example requires libcurl 7.20.0 or above.
 */

#ifndef IF_NULL
#define IF_NULL(a, b) ((a) == NULL ? (b) : (a))
#endif // IF_NULL

void build_mail_date_string(char *buf, int len)
{
    struct  tm *lt;
    struct  tm *gmt;
    time_t  tt;
    time_t  tt_local;
    time_t  tt_gmt;
    double  tzonediffFloat;
    int     tzonediffWord;
    char    tzoneBuf[16];

    /* Calculating time diff between GMT and localtime */
    tt       = time(0);
    lt       = localtime(&tt);
    tt_local = mktime(lt);
    gmt      = gmtime(&tt);
    tt_gmt   = mktime(gmt);
    tzonediffFloat = difftime(tt_local, tt_gmt);
    tzonediffWord  = (int)(tzonediffFloat/3600.0);

    if((double)(tzonediffWord * 3600) == tzonediffFloat)
      snprintf(tzoneBuf, 15, "%+03d00", tzonediffWord);
    else
      snprintf(tzoneBuf, 15, "%+03d30", tzonediffWord);

    lt       = localtime(&tt);
    // example: "Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n"
    snprintf(buf, len, "%s %02d %s %04d %02d:%02d:%02d %s",
             DayShortNames[lt->tm_wday],
             lt->tm_mday, MthShortNames[lt->tm_mon], lt->tm_year + 1900,
             lt->tm_hour, lt->tm_min, lt->tm_sec,
             tzoneBuf);
}

void generate_simple_uid(char *buf, int len)
{
    static unsigned int number = 0;
    time_t tt = time(NULL);
    snprintf(buf, len, "%ld.%d", tt, number++);
}

const char *str_if_null(const char *s, const char *v)
{
    return s == NULL ? v : s;
}

int send_mail(const char *mail_server, int use_ssl, const char *username, const char *password,
	const char *from, const char *to, const char *cc,
	const char *subject, const char *message)
{
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  char mail_url[260];
  char cc_line[256];
  char message_id_part[64] = "0123456789abcdef";
  char message_id[256];
  char mail_time[64];
  char mail_header[4096];
  memory_stream_t message_stream;

  curl = curl_easy_init();
  if(curl) {
    /* Set username and password */
    curl_easy_setopt(curl, CURLOPT_USERNAME, username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password);

    /* This is the URL for your mailserver. Note the use of smtps:// rather
     * than smtp:// to request a SSL based connection. */
    if(use_ssl != 0) {
        sprintf(mail_url, "smtps://%s", mail_server);
    }
    else {
        sprintf(mail_url, "smtp://%s", mail_server);
    }
    curl_easy_setopt(curl, CURLOPT_URL, mail_url);

    /* If you want to connect to a site who isn't using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you. */
#define SKIP_PEER_VERIFICATION
#ifdef SKIP_PEER_VERIFICATION
    if(use_ssl != 0)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }
#endif

    /* If the site you're connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcurl will refuse to connect. You can skip
     * this check, but this will make the connection less secure. */
#ifdef SKIP_HOSTNAME_VERIFICATION
    if(use_ssl != 0)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
#endif

    /* Note that this option isn't strictly required, omitting it will result
     * in libcurl sending the MAIL FROM command with empty sender data. All
     * autoresponses should have an empty reverse-path, and should be directed
     * to the address in the reverse-path which triggered them. Otherwise,
     * they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
     * details.
     */
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, IF_NULL(from, username));

    /* Add two recipients, in this particular case they correspond to the
     * To: and Cc: addressees in the header, but they could be any kind of
     * recipient. */
    recipients = curl_slist_append(recipients, to);
    if(cc != NULL) {
        recipients = curl_slist_append(recipients, cc);
        sprintf(cc_line, "Cc: %s\r\n", cc);
    }
    else {
        cc_line[0] = '\0';
    }
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    /* We're using a callback function to specify the payload (the headers and
     * body of the message). You could just use the CURLOPT_READDATA option to
     * specify a FILE pointer to read from. */
    generate_simple_uid(message_id_part, sizeof(message_id_part));
    sprintf(message_id, "<%s@%s>", message_id_part, mail_server);
    build_mail_date_string(mail_time, sizeof(mail_time));
    sprintf(mail_header,
        "Date: %s\r\n"
        "To: %s\r\n"
        "From: %s\r\n"
        "%s"
        "Message-ID: %s\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Subject: %s\r\n"
        "\r\n", /* empty line to divide headers from body, see RFC5322 */
        mail_time,
        to,
        IF_NULL(from, username),
        cc_line,
        message_id,
        IF_NULL(subject, ""));

    memory_stream_init(&message_stream);
    memory_stream_write(mail_header, strlen(mail_header), 1, &message_stream);
    const char *html_prefix = "<html>\r\n"
                "<head>\r\n"
                "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\r\n"
                "</head>\r\n"
                "<body>\r\n";
    const char *html_postfix = "</body>\r\n"
                        "</html>\r\n";
    memory_stream_write(html_prefix, strlen(html_prefix), 1, &message_stream);
    memory_stream_write(IF_NULL(message, ""), strlen(IF_NULL(message, "")), 1, &message_stream);
    memory_stream_write(html_postfix, strlen(html_postfix), 1, &message_stream);
    memory_stream_rewind(&message_stream);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, memory_stream_read);
    curl_easy_setopt(curl, CURLOPT_READDATA, &message_stream);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    /* Since the traffic will be encrypted, it is very useful to turn on debug
     * information within libcurl to see what is happening during the
     * transfer */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    /* Send the message */
    res = curl_easy_perform(curl);

    /* Check for errors */
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    // Close and clean up memory stream
    memory_stream_cleanup(&message_stream);

    /* Free the list of recipients */
    curl_slist_free_all(recipients);

    /* Always cleanup */
    curl_easy_cleanup(curl);
  }

  return (int)res;
}
