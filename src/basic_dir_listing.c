#include <sys/types.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "basic_dir_listing.h"

int
noHiddenFiles(const struct dirent *fileName)
{
	return (fileName[0].d_name[0] != '.');
}

int
compareFileNames(const struct dirent **a, const struct dirent **b)
{
	return strcmp(a[0]->d_name, b[0]->d_name);
}

int
clearDir(const char *dirName)
{
	struct dirent **nameList;
	int numFiles;
	char nameBuf[1024];

	numFiles = scandir(dirName, &nameList, noHiddenFiles, compareFileNames);
	if (numFiles < 0) {
		fprintf(stderr,
				"scandir() failed: %s\n",
				strerror(errno)
			   );
		return 1;
	}

	for (int i = 0; i < numFiles; ++i) {
		memset(nameBuf, 0, sizeof(nameBuf));
		strncat(nameBuf, dirName, (sizeof(nameBuf) - strlen(nameBuf) - 1));
		strncat(nameBuf,
				nameList[i]->d_name,
				(sizeof(nameBuf) - strlen(nameBuf) - 1)
			   );
		unlink(nameBuf);
	}

	return 0;
}
