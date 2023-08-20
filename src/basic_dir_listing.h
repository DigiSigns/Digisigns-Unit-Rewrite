#ifndef BASIC_DIR_LISTING_H
#define BASIC_DIR_LISTING_H

#include <dirent.h>

int
noHiddenFiles(const struct dirent*);

int
compareFileNames(const struct dirent**, const struct dirent**);

int
clearDir(const char*);

#endif // BASIC_DIR_LISTING_H
