#include "../libs/pinc.h"
#include "../include/structs.h"
#include "../include/timer.h"

#include <string.h>
#include <malloc.h>
#include <time.h>
#include <stdatomic.h>
#include <pthread.h>
#include <inttypes.h>

static struct voteStruct*       vStruct;
static struct mapsStruct*       mStruct;
static pthread_t                timerThread;

__cdecl void voteStart();

static void freeMapsArray(size_t count) {
    if (!mStruct || !mStruct->mapsArray) {

        return;

    }

    for (size_t i = 0; i < count; i++) {
        free((char*)mStruct->mapsArray[i]);

    }
    free(mStruct->mapsArray);

    mStruct->mapsArray = NULL;
    mStruct->mapsArraySize = 0;

}

int connectedPlayers() {
    int count = 0;
    int slots = Plugin_GetSlotCount();
    for (int i = 0; i < slots; i++) {
        client_t* cl = Plugin_GetClientForClientNum(i);
        if (cl && cl->state == CS_ACTIVE) {
            count++;

        }

    }

    return count;

}

PCL int OnInit() {
    vStruct = (struct voteStruct*)malloc(sizeof(struct voteStruct));
    if (!vStruct) {
        Plugin_PrintError("Failed to allocate memory for voteStruct instance\n");

        return 1;

    }
    pthread_mutex_init(&vStruct->mutex, NULL);

    atomic_store(&vStruct->invoked, 0);
    vStruct->gameType = NULL;
    vStruct->map = NULL;
    vStruct->startTime = 0;
    atomic_store(&vStruct->votedPlayerIdsIndex, 0);
    atomic_store(&vStruct->timerExit, 0);

    mStruct = (struct mapsStruct*)malloc(sizeof(struct mapsStruct));
    if (!mStruct) {
        Plugin_PrintError("Failed to allocate memory for mapsStruct instance\n");

        goto fail_vstruct;

    }
    mStruct->mapsArray = NULL;
    mStruct->mapsArraySize = 0;

    FILE* fp = fopen("maps.txt", "r");
    if (!fp) {
        Plugin_PrintError("Failed to open maps.txt file, returning...\n");

        goto fail_mstruct;

    }

    size_t mapsCap = 16;
    size_t mapsCount = 0;
    mStruct->mapsArray = (const char**)malloc(mapsCap * sizeof(const char*));
    if (!mStruct->mapsArray) {
        Plugin_PrintError("Failed to allocate memory for mapsArray\n");

        goto fail_fp;

    }

    char b[256];
    while (fgets(b, sizeof(b), fp)) {
        size_t mapLen = strlen(b);

        if (mapLen == sizeof(b) - 1 && b[mapLen - 1] != '\n') {
            int ch;
            while ((ch = fgetc(fp)) != '\n' && ch != EOF) {}

        }

        while (mapLen > 0 && (b[mapLen - 1] == '\n' || b[mapLen - 1] == '\r')) {
            mapLen--;
            b[mapLen] = '\0';

        }

        if (mapLen == 0) {
            continue;

        }

        if (strncmp(b, "mp_", 3) == 0) {
            memmove(b, b + 3, (mapLen - 2) * sizeof(char));
            mapLen -= 3;

        }

        if (mapLen == 0) {
            continue;

        }

        if (mapsCount == mapsCap) {
            mapsCap *= 2;
            const char** grown = (const char**)realloc(mStruct->mapsArray, mapsCap * sizeof(const char*));
            if (!grown) {
                Plugin_PrintError("Failed to grow mapsArray memory\n");

                goto fail_maps;

            }
            mStruct->mapsArray = grown;

        }

        char* mapCopy = (char*)malloc((mapLen + 1) * sizeof(char));
        if (!mapCopy) {
            Plugin_PrintError("Failed to allocate memory for a map name\n");

            goto fail_maps;

        }
        memcpy(mapCopy, b, mapLen + 1);

        mStruct->mapsArray[mapsCount++] = mapCopy;

    }
    fclose(fp);
    fp = NULL;

    if (mapsCount == 0) {
        Plugin_PrintError("No maps found at the specified file, exiting...\n");

        goto fail_maps;

    }

    const char** exact = (const char**)realloc(mStruct->mapsArray, mapsCount * sizeof(const char*));
    if (exact) {
        mStruct->mapsArray = exact;

    }
    mStruct->mapsArraySize = (int)mapsCount;

    if (pthread_create(&timerThread, NULL, voteTimer, vStruct) != 0) {
        Plugin_PrintError("Failed to create the vote timer thread\n");

        goto fail_maps;

    }

    Plugin_AddCommand("vote", voteStart, 1);

    return 0;

fail_maps:
    freeMapsArray(mapsCount);
fail_fp:
    if (fp) {
        fclose(fp);

    }
fail_mstruct:
    free(mStruct);
    mStruct = NULL;
fail_vstruct:
    pthread_mutex_destroy(&vStruct->mutex);
    free(vStruct);
    vStruct = NULL;

    return 1;

}

__cdecl void voteStart() {
    int invokerSlot = Plugin_Cmd_GetInvokerSlot();
    if (invokerSlot < 0) {
        Plugin_Printf("The vote command can only be used by players.\n");

        return;

    }

    uint64_t invokerPlayerId = Plugin_GetPlayerID((unsigned int)invokerSlot);

    if (!atomic_load(&vStruct->invoked)) {
        if (
            (Plugin_Cmd_Argc() == 4) &&
            (strcmp(Plugin_Cmd_Argv(1), "map") == 0)
           ) {
            char* mapName = Plugin_Cmd_Argv(2);
            size_t mapNameLen = strlen(mapName);
            if (mapNameLen > 256) {
                Plugin_ChatPrintf(
                        invokerSlot,
                        "Please do not spam!\n"
                );

                return;

            }

            char* mapNameCopy = (char*)malloc((mapNameLen + 1) * sizeof(char));
            if (!mapNameCopy) {
                Plugin_PrintError("Failed to allocate memory for the map name copy\n");

                return;

            }
            memcpy(mapNameCopy, mapName, mapNameLen);
            mapNameCopy[mapNameLen] = '\0';

            if (strncmp(mapName, "mp_", 3) == 0) {
                memmove(mapNameCopy, mapNameCopy + 3, (mapNameLen - 2) * sizeof(char));

            }

            int mapFound = 0;
            for (int i = 0; i < mStruct->mapsArraySize; i++) {
                if (strcmp(mapNameCopy, mStruct->mapsArray[i]) == 0) {
                    mapFound = 1;

                    break;

                }

            }
            if (!mapFound) {
                Plugin_ChatPrintf(
                        invokerSlot,
                        "The entered map %s is not available, please try another one\n",
                        mapNameCopy
                );

                free(mapNameCopy);

                return;

            }

            const char* gameTypes[] = {
                "sd",
                "war",
                "dm",
                "sabotage"

            };
            size_t numGameTypes = sizeof(gameTypes) / sizeof(gameTypes[0]);

            char* gameType = Plugin_Cmd_Argv(3);
            size_t gameTypeLen = strlen(gameType);
            if (gameTypeLen > 256) {
                Plugin_ChatPrintf(invokerSlot, "Please do not spam!\n");

                free(mapNameCopy);

                return;

            }

            int gameTypeFound = 0;
            char* gameTypeCopy = NULL;
            for (size_t i = 0; i < numGameTypes; i++) {
                if (strcmp(gameType, gameTypes[i]) == 0) {
                    gameTypeCopy = (char*)malloc((gameTypeLen + 1) * sizeof(char));
                    if (!gameTypeCopy) {
                        Plugin_PrintError("Failed to allocate memory for the game type copy\n");

                        free(mapNameCopy);

                        return;

                    }
                    memcpy(gameTypeCopy, gameType, gameTypeLen);
                    gameTypeCopy[gameTypeLen] = '\0';
                    gameTypeFound = 1;

                    break;

                }

            }
            if (!gameTypeFound) {
                Plugin_ChatPrintf(invokerSlot, "The provided gametype is unknown, please try again\n");

                free(mapNameCopy);

                return;

            }

            pthread_mutex_lock(&vStruct->mutex);
            if (atomic_load(&vStruct->invoked)) {
                pthread_mutex_unlock(&vStruct->mutex);

                free(mapNameCopy);
                free(gameTypeCopy);

                return;

            }
            vStruct->map = mapNameCopy;
            vStruct->gameType = gameTypeCopy;
            vStruct->invokerPlayerId = invokerPlayerId;
            vStruct->startTime = time(NULL);
            atomic_store(&vStruct->invoked, 1);
            pthread_mutex_unlock(&vStruct->mutex);

        } else {
            Plugin_ChatPrintf(invokerSlot, "^1Invalid usage, ^7usage: %s map <mapname> <mode>\n", Plugin_Cmd_Argv(0));

            return;

        }

    }

    pthread_mutex_lock(&vStruct->mutex);

    int votedPlayerIdsIndex = atomic_load(&vStruct->votedPlayerIdsIndex);
    for (int i = 0; i < votedPlayerIdsIndex; i++) {
        if (atomic_load(&vStruct->votedPlayerIds[i]) == invokerPlayerId) {
            pthread_mutex_unlock(&vStruct->mutex);

            Plugin_ChatPrintf(invokerSlot, "You have already voted!\n");

            return;

        }

    }
    if (votedPlayerIdsIndex >= (int)(sizeof(vStruct->votedPlayerIds) / sizeof(vStruct->votedPlayerIds[0]))) {
        pthread_mutex_unlock(&vStruct->mutex);

        Plugin_ChatPrintf(invokerSlot, "The vote has been cancelled because its capacity was reached\n");

        endVote(vStruct, 0);

        return;

    }
    atomic_store(&vStruct->votedPlayerIds[votedPlayerIdsIndex], invokerPlayerId);
    atomic_fetch_add(&vStruct->votedPlayerIdsIndex, 1);
    votedPlayerIdsIndex = atomic_load(&vStruct->votedPlayerIdsIndex);

    unsigned char thresholdMet = ((votedPlayerIdsIndex * 2) > connectedPlayers()) ? 1 : 0;

    if (thresholdMet) {
        atomic_store(&vStruct->invoked, 0);

    }

    pthread_mutex_unlock(&vStruct->mutex);

    if (thresholdMet) {
        endVote(vStruct, 1);

    }

}

PCL void OnPlayerDC(client_t* client, const char* reason) {
    (void)reason;

    if (!atomic_load(&vStruct->invoked)) {

        return;

    }

    uint64_t disconnectedPlayerId = Plugin_GetPlayerID(NUMFORCLIENT(client));

    pthread_mutex_lock(&vStruct->mutex);

    int index = atomic_load(&vStruct->votedPlayerIdsIndex);
    for (int i = 0; i < index; ) {
        if (atomic_load(&vStruct->votedPlayerIds[i]) == disconnectedPlayerId) {
            index--;
            atomic_store(&vStruct->votedPlayerIds[i], atomic_load(&vStruct->votedPlayerIds[index]));
            atomic_fetch_sub(&vStruct->votedPlayerIdsIndex, 1);

        } else {
            i++;

        }

    }

    pthread_mutex_unlock(&vStruct->mutex);

}

PCL void OnInfoRequest(pluginInfo_t *info){
    info->handlerVersion.major = PLUGIN_HANDLER_VERSION_MAJOR;
    info->handlerVersion.minor = PLUGIN_HANDLER_VERSION_MINOR;

    info->pluginVersion.major = 2;
    info->pluginVersion.minor = 0;
    strncpy(info->fullName,"Cod4X Vote Plugin", 18);
    strncpy(info->shortDescription,"Vote Plugin for changing maps.", 31);
    strncpy(info->longDescription,"This plugin is used to change the map of the game. Coded my LM40 ( DevilHunter )", 81);
}

PCL void OnTerminate() {
    atomic_store(&vStruct->timerExit, 1);

    pthread_join(timerThread, NULL);

    endVote(vStruct, 0);

    pthread_mutex_destroy(&vStruct->mutex);
    free(vStruct);
    vStruct = NULL;

    freeMapsArray(mStruct->mapsArraySize);

    free(mStruct);
    mStruct = NULL;

}
