#include <stdlib.h>
#include <threads.h>

#include "main.h"
#include "status_update.h"

static inline int
hoursToSeconds(int min)
{
    return min * 60 * 60;
}

int
backgroundStatusUpdate(void *args)
{
    thrd_sleep(&(const struct timespec){ .tv_sec = 4 }, NULL);
    system("pacmd suspend 1");
    thrd_sleep(&(const struct timespec){ .tv_sec = 1 }, NULL);
    system("pacmd suspend 0");
    for (;;) {
#ifndef DEBUG
        system("sudo ip link set wlan0 down");
        thrd_sleep(&(const struct timespec){ .tv_sec = hoursToSeconds(1) },
                   NULL
                  );
        system("sudo ip link set wlan0 up");
#else
        thrd_sleep(&(const struct timespec){ .tv_sec = hoursToSeconds(24) },
                   NULL
                  );
#endif
    }
}
