#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstdlib>
#include <ctime>

using namespace std;

int* criarMemoriaCompartilhada(); 
sem_t* criarSemaforo(); 
void criarProcessosFilhos(int nprocess, int* id, sem_t* sem); 
void aguardarProcessos(int nprocess); 

int main() {
    int nprocess;
    
    int *id = criarMemoriaCompartilhada();
    if (id == MAP_FAILED) {
        cout << "Erro ao criar memória compartilhada" << endl;
        return 0;
    }

    sem_t* sem = criarSemaforo();
    if (sem == SEM_FAILED) {
        cout << "Erro na criação do semaforo" << endl;
        return 0;
    }

    cout << "Digite a quantidade de processos filhos a serem criados:" << endl;
    cin >> nprocess;

    *id = 0;

    criarProcessosFilhos(nprocess, id, sem);
    aguardarProcessos(nprocess);

    munmap(id, sizeof(int));
    sem_close(sem);
    sem_unlink("/semaforo");

    return 0;
}

int* criarMemoriaCompartilhada() {
    int* memoria = (int*) mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    return memoria;
}

sem_t* criarSemaforo() {
    sem_t* semaforo = (sem_t*) sem_open("/semaforo", O_CREAT, 0644, 1);
    
    return semaforo;
}

void criarProcessosFilhos(int nprocess, int* id, sem_t* sem) {
    pid_t pid;
    
    for (int i = 0; i < nprocess; i++) {
        pid = fork();
        if (pid < 0) {
            cout << "Erro ao criar processo" << endl;
            return;
        }
        if (pid == 0) {
            sleep(1);
            srand(getpid());
            cout << "PID do processo filho: " << getpid() << endl;
            int espera = (rand() % 10) + 1;
            sleep(espera);
            sem_wait(sem);
            (*id)++;
            cout << "Processo: " << *id << "; Tempo de espera(s): " << espera << endl;
            sem_post(sem);
            _exit(0);
        }
    }
}

void aguardarProcessos(int nprocess) {
    for (int i = 0; i < nprocess; i++) {
        wait(nullptr);
    }
}

