#include <iostream>
#include <pthread.h>

using namespace std;

long numero, limite;
bool ehPrimo = true;
const int THREAD_A = 1;
const int THREAD_B = 2;
pthread_t threadA, threadB;
pthread_mutex_t mutex;

void* checaNumero(void* argumentos);

 
int main() {
    pthread_mutex_init(&mutex, nullptr);
    pthread_t threadA, threadB;
    
    cout << "Escolha um número para verificar se é primo:" << endl;
    cin >> numero;
    
    if (numero <= 1) {
        cout << "Não é primo" << endl;
        return 0;
    }
    
    if (numero == 2 || numero == 3) {
        cout << "Número primo" << endl;
        return 0;
    }
    
    limite = numero / 2;
    
    pthread_create(&threadA, nullptr, checaNumero, (void*)&THREAD_A);
    pthread_create(&threadB, nullptr, checaNumero, (void*)&THREAD_B);
    pthread_mutex_destroy(&mutex);
    
    if(ehPrimo) {
    	cout << "Numero primo" << endl;
    } else {
    	cout << "Numero não é primo" << endl;
    }
    
    return 0;
}

void* checaNumero(void* args) {
    int idThread = *((int*)args);

    if (idThread == THREAD_A) {
        for (long i = 2; i <= limite; i++) {
            if (numero % i == 0) {
                pthread_mutex_lock(&mutex);
                ehPrimo = false;
                pthread_mutex_unlock(&mutex);
                pthread_exit(nullptr);
            }
        }
    } else {
        for (long i = limite + 1; i < numero; i++) {
            if (numero % i == 0) {
                pthread_mutex_lock(&mutex);
                ehPrimo = false;
                pthread_mutex_unlock(&mutex);
                pthread_exit(nullptr);
            }
        }
    }

    return nullptr;
}

