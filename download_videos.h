#ifndef DOWNLOAD_VIDEOS_H
#define DOWNLOAD_VIDEOS_H

struct addrNode {
	char url[512];
	struct addrNode *next;
};

int
addAddrNode(struct addrNode**, char*);

void
destroyAddrNodeList(struct addrNode*);

void
fetchAddrsTLS(struct addrNode *list);

void
getAddrFromUrl(char*, char*, int, int);

void
getPathFromUrl(char*, char*, int);

#endif //DOWNLOAD_VIDEOS_H
