#ifndef DOWNLOAD_VIDEOS_H
#define DOWNLOAD_VIDEOS_H

#include <stdio.h>

#include <postgresql/libpq-fe.h>

struct addrNode {
	char url[512];
	struct addrNode *next;
};

void
download_videos(int);

void
getDefaultVideoURLs(struct addrNode**, PGconn*, char*, int);

void
getSQLDateStr(char*, int);

int
addAddrNode(struct addrNode**, char*);

void
destroyAddrNodeList(struct addrNode*);

void
getVideosMT(struct addrNode*, int);

void
writePastHeader(char*, FILE*);

#endif // DOWNLOAD_VIDEOS_H
