#ifndef VOTE_PLUGIN_STRUCTS_H
#define VOTE_PLUGIN_STRUCTS_H

#include <stdatomic.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

struct voteStruct {
    atomic_uchar                invoked;
    uint64_t                    invokerPlayerId;
    atomic_ullong               votedPlayerIds[256];
    atomic_short                votedPlayerIdsIndex;
    time_t                      startTime;
    const char*                 map;
    const char*                 gameType;

    atomic_uchar                timerExit;

    pthread_mutex_t             mutex;

};

struct mapsStruct {
    int             mapsArraySize;
    const char**    mapsArray;

};

#endif

