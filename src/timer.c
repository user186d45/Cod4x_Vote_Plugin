#include "../libs/pinc.h"
#include "../include/timer.h"

#include <malloc.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>

void endVote(struct voteStruct* vStruct, int changeMap) {
    char b[1024];

    pthread_mutex_lock(&vStruct->mutex);

    if (!changeMap && !atomic_load(&vStruct->invoked)) {
        pthread_mutex_unlock(&vStruct->mutex);

        return;

    }

    if (changeMap && vStruct->map && vStruct->gameType) {
        Plugin_Printf("Changing map to %s", vStruct->map);

        snprintf(b, sizeof(b), "set g_gametype %s;", vStruct->gameType);
        Plugin_Cbuf_AddText(b);

        snprintf(b, sizeof(b), "map mp_%s;", vStruct->map);
        Plugin_Cbuf_AddText(b);

    }

    free((char*)vStruct->gameType);
    vStruct->gameType = NULL;

    free((char*)vStruct->map);
    vStruct->map = NULL;

    atomic_store(&vStruct->invoked, 0);
    atomic_store(&vStruct->votedPlayerIdsIndex, 0);
    vStruct->startTime = 0;

    pthread_mutex_unlock(&vStruct->mutex);

}

void* voteTimer(void* arg) {
	struct timespec dur200000000 = {0, 200000000};
	struct timespec dur500000000 = {0, 500000000};

    struct voteStruct* vStruct = (struct voteStruct*)arg;

    while (!atomic_load(&vStruct->timerExit)) {
        if (!atomic_load(&vStruct->invoked)) {
            Plugin_Printf("No votes in progress");
            nanosleep(&dur500000000, NULL);

        } else {
            Plugin_Printf("Vote in progress, will finish in 30 seconds");

            time_t startTime;
            pthread_mutex_lock(&vStruct->mutex);
            startTime = vStruct->startTime;
            pthread_mutex_unlock(&vStruct->mutex);

            if (difftime(time(NULL), startTime) > 30) {
                endVote(vStruct, 0);

            }

            nanosleep(&dur200000000, NULL);

        }

    }

    return NULL;

}