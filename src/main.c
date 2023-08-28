#include <stdio.h>
#include <stdlib.h>

#include "basic_dir_listing.h"
#include "download_videos.h"
#include "main.h"
#include "play.h"
#include "process_videos.h"
#include "status_update.h"


#define NUM_THREADS 0

int
main(void)
{
	clearDir(DATA_DIR PRE_PROC);
	clearDir(DATA_DIR POST_PROC);
    printf("Downloading videos\n");
	download_videos(NUM_THREADS);
    printf("Processing videos\n");
	process_videos();
    play();

	return 0;
}
