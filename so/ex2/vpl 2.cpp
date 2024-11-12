#include <iostream>
#include <pthread.h>

using namespace std;

int contador = 0;
pthread_t threads[128];
pthread_mutex_t mtx;

void* incrementar(void* arg);
void* inicializaThreads(void* arg);

int main() {
    pthread_mutex_init(&mtx, nullptr);
    pthread_t main_thread;
    pthread_create(&main_thread, nullptr, inicializaThreads, nullptr);
    pthread_join(main_thread, nullptr);
    pthread_mutex_destroy(&mtx);
    
    cout << "valor final: " << contador << endl;
    
    return 0;
}

void* incrementar(void* arg) {
    for (int i = 0; i < 1000; i++) {
        pthread_mutex_lock(&mtx);
        contador++;
        pthread_mutex_unlock(&mtx);
    }
    return nullptr;
}

void* inicializaThreads(void* arg) {
    for (int i = 0; i < 128; i++) {
        pthread_create(&threads[i], nullptr, incrementar, nullptr);
    }
    for (int i = 0; i < 128; i++) {
        pthread_join(threads[i], nullptr);
    }
    return nullptr;
}

