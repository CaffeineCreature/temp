#include <iostream>
#include <pthread.h>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <cstdio>

pthread_mutex_t dealerMutex;
pthread_mutex_t playerMutex;
std::vector<pthread_cond_t> playerTurns(6);

struct gameData {
    int round;
    std::vector<int> cards;
};

struct playerData {
    int id;
    gameData* game;
};

void* playerThread(void* par){
    auto pData = (playerData *)par;
    const int id = pData->id;
    const auto pGame = pData->game;

    while(pGame->round < 1){
        //pthread_mutex_lock(&playerMutex);
        if(pGame->round % 6 == id){
            pthread_mutex_lock(&dealerMutex);

            printf("Player%i is Dealer\n", id);

            for(int i = 1; i <6;i++){
                pthread_mutex_lock(&playerMutex);
                printf("Player%i is signaled\n", i);
                pthread_cond_signal(&playerTurns[i]);
                pthread_cond_wait(&playerTurns[id], &playerMutex);
                pthread_mutex_unlock(&playerMutex);
            }

            pGame->round++;

            pthread_mutex_unlock(&dealerMutex);
        } else {
            printf("Player%i is in waiting\n", id);
            pthread_mutex_lock(&playerMutex);
            pthread_cond_wait(&playerTurns[id],&playerMutex);
            printf("Player%i is in play\n", id);

            pthread_cond_signal(&playerTurns[pGame->round%6]);
            printf("Dealer is signaled by %i\n", id);
            pthread_mutex_unlock(&playerMutex);
        }
        //pthread_mutex_unlock(&playerMutex);
    }

    pthread_exit(0);
}

int main() {
    //std::cout << "Hello, World!" << std::endl;
    std::vector<pthread_t> players(6);
    std::vector<playerData> dataSet(6);

    // Game variables
    gameData game;
    game.round = 0;
    game.cards = std::vector<int>(52);
    std::iota(game.cards.begin(),game.cards.end(), 0);

    pthread_mutex_init(&dealerMutex, NULL);
    pthread_mutex_init(&playerMutex, NULL);

    for(auto t: playerTurns){
        auto err = pthread_cond_init(&t, NULL);
        if(err) {
            printf("COND_ERROR: %i", err);
            return err;
        }
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for(int i = 0; i < 6; i++){
        dataSet[i].id = i;
        dataSet[i].game = &game;

        int err = pthread_create(&players[i],&attr,playerThread,&dataSet[i]);
        if(err){
            std::printf("CREATE_ERROR[%i] %i\n",i,err);
            return err;
        }
    }

    pthread_attr_destroy(&attr);
    for(int i = 0; i < 6; i++){
        int err = pthread_join(players[i],NULL);
        if(err){
            std::printf("JOIN_ERROR[%i] %i\n",i,err);
            return err;
        }
    }

    return 0;
}
