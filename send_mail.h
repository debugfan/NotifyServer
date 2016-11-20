#ifndef SEND_MAIL_H_INCLUDED
#define SEND_MAIL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

int send_mail(const char *mail_server, int use_ssl, const char *username, const char *password,
	const char *from, const char *to, const char *cc,
	const char *subject, const char *message);

#ifdef __cplusplus
}
#endif

#endif // SEND_MAIL_H_INCLUDED
