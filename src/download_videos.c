#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <netdb.h>
#include <netinet/in.h>

#include <assert.h>
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
#include "url_utils.h"

#define PROD_CONN_STR "user=postgres password=DigiSigns1029 host=db.udlsgnjteywvklskdwvx.supabase.co port=5432 dbname=postgres"

void
download_videos(int numThreads)
{
	//char const *uid, *sid;
	PGconn *conn;
	PGresult *res;
	int numFields;
	struct addrNode *list; 

	//uid = getenv("uid");
	//sid = getenv("sid");

	//printf("Unit ID: %s\nStore ID: %s\n", uid, sid);

	conn = PQconnectdb(PROD_CONN_STR);
	if (PQstatus(conn) != CONNECTION_OK) {
		fprintf(stderr, "%s\n", PQerrorMessage(conn));
		goto EXIT;
	}

	// TODO: add the correct UID, SID, and date to query
	res = PQexec(conn,
				 "SELECT distinct video_url FROM videos limit 1;"
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

	getVideosMT(list, numThreads);

	PQclear(res);

	destroyAddrNodeList(list);
EXIT:
	PQfinish(conn);
}

int
addAddrNode(struct addrNode **list, char *url)
{
	struct addrNode *node;

	if (list == NULL) {
		fprintf(stderr, "WTF? List pointer doesn't exist\n");
		return 1;
	}

	/* 
	 * catching weird db things, just skipping if there isn't actually
	 * a url to keep track of
	*/
	if (!strlen(url)) {
		return 1;
	}

	node = malloc(sizeof(struct addrNode));
	memset(node, 0, sizeof(struct addrNode));
	memset(node->url, 0, sizeof(node->url));
	strncpy(node->url, url, sizeof(node->url) - 1);
	if (*list != NULL) {
		node->next = *list;
	}
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

// TODO: add file output with correct name
// used for each thread
void
getVideoFromURLWithTLS(void *args)
{
	struct sockaddr_in sin;
	struct hostent *hp;
	SSL_CTX *ctx;
	SSL *ssl;
	FILE *file;
	int bytesRecd, s, fileWriteBufferPtr;
	char fileWriteBuffer[1048576], inBuf[16384], hostname[512], path[512], fileName[1024], *URL;

	if (!args) {
		fprintf(stderr, "Args is null. Something is wrong here, I can feel it...\n");
		goto ERR_0;
	}

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

	SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION | SSL_MODE_RELEASE_BUFFERS);

	getAddrFromUrl(URL, hostname, sizeof(hostname));
	getPathFromUrl(URL, path, sizeof(path));
	getFileNameFromPath(path, fileName, sizeof(fileName));

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

	file = fopen(fileName, "w");
	if (!file) {
		fprintf(stderr, "Couldn't open file %s for writing!\n", fileName);
		goto ERR_4;
	}

	memset(inBuf, 0 , sizeof(inBuf));
	strncat(inBuf, "GET ", sizeof(inBuf) - strlen(inBuf) - 1);
	strncat(inBuf, path, sizeof(inBuf) - strlen(inBuf) - 1);
	strncat(inBuf, " HTTP/1.1\r\n", sizeof(inBuf) - strlen(inBuf) - 1);
	strncat(inBuf, "Host: ", sizeof(inBuf) - strlen(inBuf) - 1);
	strncat(inBuf, hostname, sizeof(inBuf) - strlen(inBuf) - 1);
	strncat(inBuf, "\r\n", sizeof(inBuf) - strlen(inBuf) - 1);
	strncat(inBuf, "Accept: */*\r\n\r\n", sizeof(inBuf) - strlen(inBuf) - 1);

	//printf("%s\n", inBuf);
	if (SSL_write(ssl, inBuf, strlen(inBuf)) <= 0) {
		fprintf(stderr, "Initial write to endpoint failed!\n");
		goto ERR_5;
	}


	memset(inBuf, 0, sizeof(inBuf));
	bytesRecd = SSL_read(ssl, inBuf, (sizeof(inBuf) - 1));
	if (bytesRecd <= 0) {
		fprintf(stderr, "Initial read from endpoint failed!\n");
		goto ERR_5;
	}
	inBuf[bytesRecd] = 0;
	//printf("%s\n", inBuf);


	if (bytesRecd > 0) {
		writePastHeader(inBuf, file);
	}

	fileWriteBufferPtr = 0;
	memset(inBuf, 0, sizeof(inBuf));
	while ((bytesRecd = SSL_read(ssl, inBuf, sizeof(inBuf))) > 0) {
		if (bytesRecd + fileWriteBufferPtr > sizeof(fileWriteBuffer)) {
			fwrite(fileWriteBuffer, 1, fileWriteBufferPtr, file);
			fileWriteBufferPtr = 0;
		}
		memcpy(fileWriteBuffer + fileWriteBufferPtr,
			   inBuf,
			   bytesRecd
			  );
		fileWriteBufferPtr += bytesRecd;
	}
	printf("Done downloading %s\n", fileName);

ERR_5:
	fclose(file);
ERR_4:
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
getVideosMT(struct addrNode *list, int numThreads)
{
	tpool_t *pool;
	pool = initPool(numThreads);
	while (list != NULL)
	{
		addWork(pool, getVideoFromURLWithTLS, list->url);
		list = list->next;
	}
	killPool(pool);
}

void
writePastHeader(char *inBuf, FILE *file)
{
	char *token;

	token = strtok(inBuf, "\r\n\r\n");
	if (token == NULL)
		return;
	token = strtok(NULL, "\r\n\r\n");
	if (token == NULL)
		return;
	fwrite(token, 1, strlen(token), file);
}
