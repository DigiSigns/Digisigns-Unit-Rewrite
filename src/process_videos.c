#include <sys/types.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basic_dir_listing.h"

#define VIDEO_DIR "resources"

void
process_videos(void)
{
	system("python3 scripts/ffmpeg-binding.py");
}

