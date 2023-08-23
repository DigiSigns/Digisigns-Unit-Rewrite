#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "url_utils.h"


void
getAddrFromUrl(char *inUrl, char *outBuf, int outBufSize)
{
	char *token, *inUrlCopy;

	memset(outBuf, 0, outBufSize);
	inUrlCopy = malloc(strlen(inUrl) + 1);
	memcpy(inUrlCopy, inUrl, strlen(inUrl) + 1);


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

static inline int
MIN(int a, int b)
{
	return b ^ ((a ^ b) & -(a < b));
}

void
getFileNameFromPath(char *route, char *outBuf, int outBufSize)
{
	assert(route);
	assert(outBuf);
	int routeSize, copiedSize;
	char *routeCopy, *token;

	memset(outBuf, 0, outBufSize);

	routeSize = strlen(route) + 1;
	routeCopy = malloc(routeSize);
	memcpy(routeCopy, route, routeSize);

	token = strtok(routeCopy, "/");
	while (token != NULL) {
		copiedSize = MIN((outBufSize - 1), (strlen(token) + 1));
		memcpy(outBuf, token, copiedSize);
		token = strtok(NULL, "/");
	}
    free(routeCopy);
	unescapeFileName(outBuf);
}

void
unescapeFileName(char *fileName)
{
	int fileNameSize;
	char *fileNameCopy, *token;

	fileNameSize = strlen(fileName) + 1;
	fileNameCopy = malloc(fileNameSize);
	memcpy(fileNameCopy, fileName, fileNameSize);
	memset(fileName, 0, fileNameSize);

	strcat(fileName, DATA_DIR PRE_PROC);

	token = strtok(fileNameCopy, "%20");
	if (token != NULL) {
		strncat(fileName, token, (fileNameSize - strlen(fileName) - 1));
		token = strtok(NULL, "%20");
	}
	while(token != NULL) {
		strncat(fileName, "_", (fileNameSize - strlen(fileName) - 1));
		strncat(fileName, token, (fileNameSize - strlen(fileName) - 1));
		token = strtok(NULL, "%20");
	}

    token = strtok(fileName, "?");

    free(fileNameCopy);
}

