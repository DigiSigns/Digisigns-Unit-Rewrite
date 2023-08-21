#include "basic_dir_listing.h"
#include "download_videos.h"
#include "main.h"
#include "play.h"
#include "process_videos.h"


#define NUM_THREADS 8

int
main(void)
{
	clearDir(DATA_DIR PRE_PROC);
	clearDir(DATA_DIR POST_PROC);
    printf("Downloading videos\n");
	download_videos(NUM_THREADS);
    printf("Processing videos\n");
	process_videos();
    //play();
    printf("Video would play now\n");

	return 0;
}
