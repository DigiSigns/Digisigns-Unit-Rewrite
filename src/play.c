#include <errno.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

#include <curl/curl.h>
#include <vlc/vlc.h>

#include "main.h"
#include "play.h"
#include "status_update.h"

static inline int
msToNs(int a)
{
    return a * 1000000;
}

int
play()
{
    FILE *videoPtr;
    libvlc_instance_t *inst;
    libvlc_media_player_t *mp;
    libvlc_media_t *m;
    thrd_t thrd;

    const struct timespec initWait = { .tv_nsec = msToNs(250) };
    const struct timespec regWait = { .tv_nsec = msToNs(50) };
    
    if (!(videoPtr = fopen(VIDEO_PATH, "r"))) {
        fprintf(stderr,
                "Error - could't open file %s: %s\n",
                VIDEO_PATH,
                strerror(errno)
               );
        return -1;
    }
    fclose(videoPtr);

    thrd_create(&thrd,
                backgroundStatusUpdate,
                NULL
               );

    inst = libvlc_new(0, NULL);
    m = libvlc_media_new_path(inst, VIDEO_PATH);
    mp = libvlc_media_player_new_from_media(m);
    libvlc_set_fullscreen(mp, 1);

    for (;;) {
        libvlc_media_player_play(mp);
        thrd_sleep(&initWait, NULL);
        while (libvlc_media_player_is_playing(mp)) {
            thrd_sleep(&regWait, NULL);
        }
    }

    // should never be reached
    thrd_join(thrd, NULL);

    return 0;
}
