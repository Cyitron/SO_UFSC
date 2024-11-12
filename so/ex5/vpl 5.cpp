#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

using namespace std;

const int numFilosofos = 5;
char estados[numFilosofos] = {'P', 'P', 'P', 'P', 'P'};
pthread_t threads[numFilosofos];
pthread_mutex_t mut[numFilosofos];
int ids[numFilosofos];

void mostrarEstados();
void estadoPensando(int id);
void estadoFaminto(int id);
void estadoComendo(int id);
void* inicia(void* arg);

int main() {
    srand(time(nullptr));

    for (int i = 0; i < numFilosofos; i++) {
        pthread_mutex_init(&mut[i], nullptr);
        ids[i] = i;
    }

    for (int i = 0; i < numFilosofos; i++) {
        pthread_create(&threads[i], nullptr, inicia, (void*)&ids[i]);
    }

    for (int i = 0; i < numFilosofos; i++) {
        pthread_join(threads[i], nullptr);
    }

    for (int i = 0; i < numFilosofos; i++) {
        pthread_mutex_destroy(&mut[i]);
    }

    return 0;
}

void mostrarEstados() {
    for (int i = 0; i < numFilosofos; i++) {
        cout << estados[i] << "; ";
    }
    cout << endl;
}

void estadoPensando(int id) {
    estados[id] = 'P';
    mostrarEstados();
    sleep(rand() % 3 + 1);
}

void estadoFaminto(int id) {
    estados[id] = 'F';
    mostrarEstados();
    sleep(4);
}

void estadoComendo(int id) {
    estados[id] = 'C';
    mostrarEstados();
    sleep(4);
}

void* inicia(void* arg) {
    int id = *(int*)arg;
    int garfoEsq = id;
    int garfoDir = (id + 1) % numFilosofos;

    while (true) {
        estadoPensando(id);
        estadoFaminto(id);
        pthread_mutex_lock(&mut[garfoEsq]);
        pthread_mutex_lock(&mut[garfoDir]);
        estadoComendo(id);
        pthread_mutex_unlock(&mut[garfoEsq]);
        pthread_mutex_unlock(&mut[garfoDir]);
    }
    pthread_exit(nullptr);
}

