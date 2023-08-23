#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#include <curl/curl.h>

#include "main.h"
#include "status_update.h"

static inline int
hoursToSeconds(int min)
{
    return min * 60 * 60;
}

char*
getTimeStr()
{
    static char buf[256];
    struct tm *tm;
    time_t raw;

    time(&raw);

    tm = localtime(&raw);

    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return buf;
}

void
sendStatusUpdate(void)
{
    CURL *curl;
    CURLcode res;
    char postBody[1024], *escapedBody;

    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error, couldn't initialize curl\n");
        return;
    }

    memset(postBody, 0, sizeof(postBody));
    snprintf(postBody,
             sizeof(postBody),
             "device_id=%s&host_id=%s&updated_time=\"%s\"",
             getenv("uid"),
             getenv("sid"),
             getTimeStr()
            );
    escapedBody = curl_easy_escape(curl, postBody, 0);

    curl_easy_setopt(curl,
                     CURLOPT_FOLLOWLOCATION,
                     1L
                    );
    curl_easy_setopt(curl,
                     CURLOPT_URL,
                     "https://stripe-apis-git-pre-prod-digi-signs.vercel.app/"
                     "api/v1/devices/update-devices"
                    );
    curl_easy_setopt(curl,
                     CURLOPT_USERAGENT,
                     "curl/7.68.0"
                    );
    curl_easy_setopt(curl,
                     CURLOPT_HTTPAUTH,
                     CURLAUTH_ANY
                    );
    curl_easy_setopt(curl,
                     CURLOPT_POSTFIELDS,
                     escapedBody
                    );
    curl_easy_setopt(curl,
                     CURLOPT_POST,
                     1L
                    );
    curl_easy_setopt(curl,
                     CURLOPT_VERBOSE,
                     1L
                    );
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr,
                "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res)
               );
    }
    curl_free(escapedBody);
    curl_easy_cleanup(curl);
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
        sendStatusUpdate();
#else
        thrd_sleep(&(const struct timespec){ .tv_sec = hoursToSeconds(1) },
                   NULL
                  );
#endif
    }
}
