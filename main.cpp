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
    int turn;
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
        if(pGame->round % 6 == id){
            pthread_mutex_lock(&dealerMutex);

            printf("Player%i is Dealing\n", id);


            // Deal to players in order
            for(int i = 1; i <6;i++){
                pthread_mutex_lock(&playerMutex);
                printf("Player%i signaled\n", i);
                pGame->turn = i;
                pthread_cond_signal(&playerTurns[i]);
                // Wait for player to finish
                while(pGame->turn != id) {
                    pthread_cond_wait(&playerTurns[id], &playerMutex);
                }
                printf("idle turn:%i\n", pGame->turn);
                pthread_mutex_unlock(&playerMutex);
            }

            pGame->round++;
            // Deal to players in order

            pthread_mutex_unlock(&dealerMutex);
        } else {
            pthread_mutex_lock(&playerMutex);
            printf("Player%i waiting\n", id);
            // Wait for signal from dealer
            while(pGame->turn != id) {
                //printf("idle%i\n", id);
                pthread_cond_wait(&playerTurns[id], &playerMutex);
            }
            printf("Player%i in play\n", id);

            // Return control to dealer
            pGame->turn = pGame->round%6;
            pthread_cond_signal(&playerTurns[pGame->round%6]);
            printf("Dealer is signaled by %i\n", id);

            pthread_mutex_unlock(&playerMutex);
        }
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
    game.turn = 0;


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
