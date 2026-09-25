#ifndef VOTE_PLUGIN_TIMER_H
#define VOTE_PLUGIN_TIMER_H

#include "../include/structs.h"

void* voteTimer(void* arg);
void endVote(struct voteStruct* vStruct, int changeMap);
int connectedPlayers();

#endif
