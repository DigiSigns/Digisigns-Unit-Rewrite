#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <netdb.h>
#include <netinet/in.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <postgresql/libpq-fe.h>

#include "download_videos.h"
#include "tpool.h"

#define PROD_CONN_STR "user=postgres password=DigiSigns1029 host=db.udlsgnjteywvklskdwvx.supabase.co port=5432 dbname=postgres"

int
main(void)
{
	downloadVideos();	

	return 0;
}

void
downloadVideos(void)
{
	char const *connStr, *uid, *sid;
	PGconn *conn;
	PGresult *res;
	int numFields;
	char buf[512];
	struct addrNode *list; 

	conn = PQconnectdb(PROD_CONN_STR);
	if (PQstatus(conn) != CONNECTION_OK) {
		fprintf(stderr, "%s\n", PQerrorMessage(conn));
		goto EXIT;
	}

	// TODO: add the correct UID, SID, and date to query
	res = PQexec(conn,
				 "SELECT distinct video_url FROM videos;"
				);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "query failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
	}

	list = NULL;
	numFields = PQnfields(res);
	for (int i = 0; i < PQntuples(res); ++i)
		for (int j = 0; j < numFields; ++j)
			addAddrNode(&list, PQgetvalue(res, i, j));
	fetchAddrsTLS(list);

	PQclear(res);
	
	destroyAddrNodeList(list);
EXIT:
	PQfinish(conn);
}

int
addAddrNode(struct addrNode **list, char *url)
{
	if (list == NULL) {
		fprintf(stderr, "WTF? List pointer doesn't exist\n");
		return 1;
	}
	/* 
	 * catching weird db things, just skipping if there isn't actually
	 * a url to keep track of
	*/
	if (!strlen(url)) {
		return 0;
	}

	struct addrNode *node = malloc(sizeof(struct addrNode));
	strncpy(node->url, url, sizeof(node->url) - 1);
	node->next = *list;
	*list = node;
	return 0;
}

void
destroyAddrNodeList(struct addrNode *list)
{
	struct addrNode *node;

	node = list;
	while (node != NULL) {
		node = list->next;
		free(list);
		list = node;
	}
}

void
getAddrFromUrl(char *inUrl, char *outBuf, int outBufSize, int port)
{
	char *token, portStr[16], *inUrlCopy;

	memset(outBuf, 0, outBufSize);
	memset(portStr, 0, sizeof(portStr));
	inUrlCopy = malloc(strlen(inUrl) + 1);
	memcpy(inUrlCopy, inUrl, strlen(inUrl) + 1);

	snprintf(portStr, sizeof(portStr), "%d", port);

	token = strtok(inUrlCopy, "/");
	token = strtok(NULL, "/");

	strncat(outBuf, token, outBufSize - strnlen(outBuf, sizeof(outBuf)) - 1);
	free(inUrlCopy);
}

void
getPathFromUrl(char *inUrl, char *outBuf, int outBufSize)
{
	int numSlashesFound, inUrlSize;

	memset(outBuf, 0, outBufSize);
	numSlashesFound = 0;
	inUrlSize = strlen(inUrl);
	for(int i = 0; i < inUrlSize; ++i) {
		if (inUrl[i] == '/')
			++numSlashesFound;
		if (numSlashesFound == 3) {
			strncat(outBuf, inUrl + i, outBufSize - strlen(outBuf) - 1);
			return;
		}
	}
}

// TODO: add file output with correct name
void
getVideoFromURLWithTLS(void *args)
{
	char *URL;
	struct sockaddr_in sin;
	struct hostent *hp;
	SSL_CTX *ctx;
	SSL *ssl;
	BIO *out;
	int bytesRecd, s;
	char inBuf[1024], hostname[512], path[512];

	URL = args;

	OPENSSL_init_ssl(OPENSSL_INIT_NO_LOAD_SSL_STRINGS, NULL);

	if (SSL_library_init() < 0) {
		fprintf(stderr, "Could not init OpenSSL\n");
		goto ERR_0;
	}

	ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx) {
		fprintf(stderr, "ctx\n");
		goto ERR_0;
	}

	getAddrFromUrl(URL, hostname, sizeof(hostname), 443);
	getPathFromUrl(URL, path, sizeof(path));

	printf("generated hostname: %s\n", hostname);

	// Standard BSD socket active connect
	if (!(hp = gethostbyname(hostname))) {
		fprintf(stderr, "Standard conn failed\n");
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
	sin.sin_port = htons(443);

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Couldn't make socket\n");
		goto ERR_1;
	}

	if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) == -1) {
		fprintf(stderr, "Couldn't connect to server\n");
		goto ERR_2;
	}

	// creating SSL handle and associating with previously connected socket's fd
	ssl = SSL_new(ctx);
	if (!ssl) {
		fprintf(stderr, "ssl\n");
		goto ERR_3;
	}
	SSL_set_fd(ssl, s);
	SSL_set_tlsext_host_name(ssl, hostname);
	if (SSL_connect(ssl) <= 0) {
		fprintf(stderr, "handshake\n");
		goto ERR_3;
	}

	memset(inBuf, 0 , sizeof(inBuf));
	strncat(inBuf, "GET ", sizeof(inBuf) - strlen(inBuf) - 1);
	strncat(inBuf, path, sizeof(inBuf) - strlen(inBuf) - 1);
	strncat(inBuf, " HTTP/1.1\r\n", sizeof(inBuf) - strlen(inBuf) - 1);
	strncat(inBuf, "Host: ", sizeof(inBuf) - strlen(inBuf) - 1);
	strncat(inBuf, hostname, sizeof(inBuf) - strlen(inBuf) - 1);
	strncat(inBuf, "\r\n", sizeof(inBuf) - strlen(inBuf) - 1);
	strncat(inBuf, "Accept: */*\r\n\r\n", sizeof(inBuf) - strlen(inBuf) - 1);

	printf("%s\n", inBuf);
	SSL_write(ssl, inBuf, strlen(inBuf));

	memset(inBuf, 0, sizeof(inBuf));
	bytesRecd = SSL_read(ssl, inBuf, sizeof(inBuf));
	inBuf[bytesRecd] = 0;
	printf("%s\n", inBuf);
	memset(inBuf, 0, sizeof(inBuf));

	SSL_shutdown(ssl);
ERR_3:
	SSL_free(ssl);
ERR_2:
	close(s);
ERR_1:
	SSL_CTX_free(ctx);
ERR_0:
	;
}

void
getVideosMT(struct addrNode *list)
{
	tpool_t *pool;
	pool = initPool(8);
	while (list != NULL)
	{
		addWork(pool, getVideoFromURLWithTLS, list->url);
	}
	killPool(pool);
}

void
fetchAddrsTLS(struct addrNode *list)
{
	struct sockaddr_in sin;
	struct hostent *hp;
	SSL_CTX *ctx;
	SSL *ssl;
	BIO *out;
	int bytesRecd, s;
	char inBuf[1024], hostname[512], path[512];

	OPENSSL_init_ssl(OPENSSL_INIT_NO_LOAD_SSL_STRINGS, NULL);

	if (SSL_library_init() < 0) {
		fprintf(stderr, "Could not init OpenSSL\n");
		goto ERR_0;
	}

	ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx) {
		fprintf(stderr, "ctx\n");
		goto ERR_0;
	}


	while (list != NULL) {
		getAddrFromUrl(list->url, hostname, sizeof(hostname), 443);
		getPathFromUrl(list->url, path, sizeof(path));

		printf("generated hostname: %s\n", hostname);

		if (!(hp = gethostbyname(hostname))) {
			fprintf(stderr, "Standard conn failed\n");
		}

		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
		sin.sin_port = htons(443);

		if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			fprintf(stderr, "Couldn't make socket\n");
			goto LOOP_ERR_0;
		}

		if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) == -1) {
			fprintf(stderr, "Couldn't connect to server\n");
			goto LOOP_ERR_1;
		}

		ssl = SSL_new(ctx);
		if (!ssl) {
			fprintf(stderr, "ssl\n");
			goto LOOP_ERR_2;
		}
		SSL_set_fd(ssl, s);
		SSL_set_tlsext_host_name(ssl, hostname);
		if (SSL_connect(ssl) <= 0) {
			fprintf(stderr, "handshake\n");
			goto LOOP_ERR_2;
		}

		memset(inBuf, 0 , sizeof(inBuf));
		strncat(inBuf, "GET ", sizeof(inBuf) - strlen(inBuf) - 1);
		strncat(inBuf, path, sizeof(inBuf) - strlen(inBuf) - 1);
		strncat(inBuf, " HTTP/1.1\r\n", sizeof(inBuf) - strlen(inBuf) - 1);
		strncat(inBuf, "Host: ", sizeof(inBuf) - strlen(inBuf) - 1);
		strncat(inBuf, hostname, sizeof(inBuf) - strlen(inBuf) - 1);
		strncat(inBuf, "\r\n", sizeof(inBuf) - strlen(inBuf) - 1);
		strncat(inBuf, "Accept: */*\r\n\r\n", sizeof(inBuf) - strlen(inBuf) - 1);

		printf("%s\n", inBuf);
		SSL_write(ssl, inBuf, strlen(inBuf));

		memset(inBuf, 0, sizeof(inBuf));
		bytesRecd = SSL_read(ssl, inBuf, sizeof(inBuf));
		inBuf[bytesRecd] = 0;
		printf("%s\n", inBuf);
		memset(inBuf, 0, sizeof(inBuf));

		SSL_shutdown(ssl);
LOOP_ERR_2:
		SSL_free(ssl);
LOOP_ERR_1:
		close(s);
LOOP_ERR_0:
		list = list->next;
	}

ERR_1:
	SSL_CTX_free(ctx);
ERR_0:
	;
}
