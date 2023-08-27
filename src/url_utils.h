#ifndef URL_UTILS_H
#define URL_UTILS_H

void
getAddrFromUrl(char*, char*, int);

void
getPathFromUrl(char*, char*, int);

void
getFileNameFromPath(char*, char*, int);

void
fixRoute(char*, int);

void
unescapeFileName(char*);

#endif // URL_UTILS_H
