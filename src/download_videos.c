#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#include <curl/curl.h>
#include <postgresql/libpq-fe.h>

#include "download_videos.h"
#include "main.h"
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
	curl_global_init(CURL_GLOBAL_ALL);

	conn = PQconnectdb(PROD_CONN_STR);
	if (PQstatus(conn) != CONNECTION_OK) {
		fprintf(stderr, "%s\n", PQerrorMessage(conn));
		goto EXIT;
	}

	// TODO: add the correct UID, SID, and date to query
	res = PQexec(conn,
				 "SELECT DISTINCT video_url FROM videos WHERE video_url "
				 "NOT LIKE '%RealAssist%';"
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
	curl_global_cleanup();
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

static size_t
writeCallback(void *data, size_t size, size_t nmemb, void *usrPtr)
{
	FILE *file = (FILE*)usrPtr;
	return fwrite(data, size, nmemb, file);
}

static void
getVideoFromURL(void *args)
{
	CURL *curl;
	CURLcode res;
	FILE *file;
	char *url, path[512], fileName[512], errMsg[256];

	url = (char*)args;

	getPathFromUrl(url, path, sizeof(path));
	getFileNameFromPath(path, fileName, sizeof(fileName));

	if (!(curl = curl_easy_init())) {
		fprintf(stderr, "Couldn't initialize curl\n");
		goto ERR_0;
	}

	if (!(file = fopen(fileName, "w"))) {
		memset(errMsg, 0, sizeof(errMsg));
		strerror_r(errno, errMsg, (sizeof(errMsg) - 1));
		fprintf(stderr, "Couldn't open file %s: %s\n", fileName, errMsg);
		goto ERR_1;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)file);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
	res = curl_easy_perform(curl);

	fclose(file);

	if (res != CURLE_OK) {
		fprintf(stderr,
				"curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res)
			   );
		// delete opened file here? it'll be empty if this fails
		unlink(fileName);
	}

ERR_1:
	curl_easy_cleanup(curl);
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
		addWork(pool, getVideoFromURL, list->url);
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
