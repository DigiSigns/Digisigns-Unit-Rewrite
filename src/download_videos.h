#ifndef DOWNLOAD_VIDEOS_H
#define DOWNLOAD_VIDEOS_H

struct addrNode {
	char url[512];
	struct addrNode *next;
};

void
downloadVideos(void);

int
addAddrNode(struct addrNode**, char*);

void
destroyAddrNodeList(struct addrNode*);

void
getAddrFromUrl(char*, char*, int, int);

void
getPathFromUrl(char*, char*, int);

void
getVideosMT(struct addrNode*);

void
fetchAddrsTLS(struct addrNode *list);

#endif //DOWNLOAD_VIDEOS_H
