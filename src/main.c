#include "download_videos.h"
#include "process_videos.h"

#define NUM_THREADS 0

int
main(void)
{
	download_videos(NUM_THREADS);
	process_videos();

	return 0;
}
