#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

void
process_videos(void)
{
	AVFormatContext *pFormatContext;

	pFormatContext = avformat_alloc_context();

	avformat_close_input(&pFormatContext);

	return;
}
