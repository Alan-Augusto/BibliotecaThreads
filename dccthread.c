#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>
#include "dlist.h"
#include "dccthread.h"
#include <signal.h>
#include <time.h>
#include <unistd.h>

/* Estrutura para representar uma thread */
struct dccthread {
    ucontext_t context;
    char name[DCCTHREAD_MAX_NAME_SIZE];
    int finished;
    int sleep;
    timer_t timer_id;
};

static dccthread_t main_thread; // Thread principal
static dccthread_t *current_thread; // Thread atual em execução
static dccthread_t *manager_thread; // Thread gerente

/* Lista de threads prontas para execução */
static struct dlist *ready_threads;

/* Máscara para o sinal SIGALRM */
sigset_t mask;

void timer_function(int signum) {
    /* Sempre que o temporizador dispara um sinal, chama dccthread_yield */
    dccthread_yield();
}

static void manager_function(int param) {
    /* Bloqueia o sinal SIGALRM */
    sigprocmask(SIG_BLOCK, &mask, NULL);

    /* Verifica se existem threads prontas para executar */
    while (!dlist_empty(ready_threads)) {
        /* Obtém a próxima thread da lista de threads prontas */
        dccthread_t *next_thread = dlist_pop_left(ready_threads);

        /* Coloca essa thread para executar */
        current_thread = next_thread;
        swapcontext(&(manager_thread->context), &(next_thread->context));
    }
    
    /* Desbloqueia o sinal SIGALRM */
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void dccthread_init(void (*func)(int), int param) {
    /* Configura o manipulador de sinal, que sempre que disparado executa a funcao timer_function */
    signal(SIGALRM, timer_function);

    /* Configura a estrutura para a criação do temporizador */
    struct sigevent sev;
    timer_t timerid;
    struct itimerspec its;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = &timerid;

    /* Cria o temporizador e configura seu disparo a cada 10ms */
    if (timer_create(CLOCK_PROCESS_CPUTIME_ID, &sev, &timerid) == -1) {
        printf("Erro ao criar o temporizador!\n");
	    exit(EXIT_FAILURE);
    }
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 10000000;  // 10ms em nanossegundos
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    /* Configura a máscara para o sinal SIGALRM */
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);

    /* Inicia o temporizador */
    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        printf("Erro ao iniciar o temporizador!\n");
	    exit(EXIT_FAILURE);
    }

    /* Inicializa a lista de threads a ser executada, inicialmente vazia */
    ready_threads = dlist_create();

    /* Inicializa a thread gerente e define que sua função padrão é a manager_function */
    manager_thread = malloc(sizeof(dccthread_t));
    getcontext(&(manager_thread->context));
    manager_thread->context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
    manager_thread->context.uc_stack.ss_size = THREAD_STACK_SIZE;
    manager_thread->context.uc_link = NULL;
    makecontext(&(manager_thread->context), (void (*)())manager_function, 1, param);

    /* Inicializa a thread principal, define que sua função padrão é a função passada por parâmetro
        e define que, ao terminar, deve voltar para a thread gerente */
    memset(&main_thread, 0, sizeof(dccthread_t));
    strcpy(main_thread.name, "main");
    getcontext(&(main_thread.context));
    main_thread.context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
    main_thread.context.uc_stack.ss_size = THREAD_STACK_SIZE;
    main_thread.context.uc_link = &(manager_thread->context);
    makecontext(&(main_thread.context), (void (*)())func, 1, param);

    /* Executa a thread principal */
    current_thread = &main_thread;
    setcontext(&(main_thread.context));

    abort(); // Apenas para remover warning do gcc
}

dccthread_t *dccthread_create(const char *name, void (*func)(int), int param) {
    /* Cria uma nova thread, define seu nome e função passsados por parâmetro 
        e define que, ao terminar, deve voltar para a thread gerente*/

    dccthread_t *new_thread = malloc(sizeof(dccthread_t));
    getcontext(&(new_thread->context));

    /* Bloqueia o sinal SIGALRM */
    sigprocmask(SIG_BLOCK, &mask, NULL);
    
    strcpy(new_thread->name, name);
    new_thread->context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
    new_thread->context.uc_stack.ss_size = THREAD_STACK_SIZE;
    new_thread->context.uc_link = &(manager_thread->context); 
    makecontext(&(new_thread->context), (void (*)())func, 1, param);

    /* Adiciona a thread recém criada à lista de threads prontas para executar */
    dlist_push_right(ready_threads, new_thread);

    /* Desbloqueia o sinal SIGALRM */
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    return new_thread;
}

void dccthread_yield(void) {
    /* Bloqueia o sinal SIGALRM */
    sigprocmask(SIG_BLOCK, &mask, NULL);

    /* Retira a thread em execução da CPU e chama a thread gerente para escolher a próxima thread */
    dccthread_t *prev_thread = current_thread;
    dlist_push_right(ready_threads, current_thread);
    current_thread = NULL;
    swapcontext(&(prev_thread->context), &(manager_thread->context));

    /* Desbloqueia o sinal SIGALRM */
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

dccthread_t *dccthread_self(void) {
    /* Retorna a thread em execução */
    return current_thread;
}

const char *dccthread_name(dccthread_t *thread) {
    /* Retorna o nome de determinada thread */
    return thread->name;
}

void dccthread_exit(void) {
    /* Bloqueia o sinal SIGALRM */
    sigprocmask(SIG_BLOCK, &mask, NULL);

    /* Marca a thread atual como terminada */
    current_thread->finished = 1;

    /* Chama a função do gerente para escolher a próxima thread a ser executada */
    swapcontext(&(current_thread->context), &(manager_thread->context));

    /* Desbloqueia o sinal SIGALRM */
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void dccthread_wait(dccthread_t *thread) {
    /* Bloqueia o sinal SIGALRM */
    sigprocmask(SIG_BLOCK, &mask, NULL);

    /* Verifica se a thread já terminou */
    while (!thread->finished) {
        /* Se a thread ainda não terminou, coloca a thread atual na lista de threads prontas e chama o gerente */
        dlist_push_right(ready_threads, current_thread);
        dccthread_t *prev_thread = current_thread;
        current_thread = NULL;
        swapcontext(&(prev_thread->context), &(manager_thread->context));

        /* Desbloqueia o sinal SIGALRM */
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
    }

    /* Desbloqueia o sinal SIGALRM */
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    return;
}

void wake_up_thread(int signum, siginfo_t *info, void *context) {
    /* Após temporizador, "acorda" a thread */
    dccthread_t *thread = info->si_value.sival_ptr;
    thread->sleep = 0;
}

void dccthread_sleep(struct timespec ts) {
    /* Coloca a thread para "dormir" e ativa o temporizador*/
    current_thread->sleep = 1;

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = wake_up_thread;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Configura a estrutura para a criação do temporizador */
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_value.sival_ptr = current_thread;
    
    /* Cria o temporizador usando CLOCK_REALTIME */
    if (timer_create(CLOCK_REALTIME, &sev, &current_thread->timer_id) == -1) {
        printf("Erro ao criar o temporizador!\n");
        exit(EXIT_FAILURE);
    }
    
    /* Configura o temporizador para disparar após o tempo especificado em ts */
    struct itimerspec its;
    its.it_value.tv_sec = ts.tv_sec;
    its.it_value.tv_nsec = ts.tv_nsec;
    its.it_interval.tv_sec = 0;  // Não repetir o temporizador
    its.it_interval.tv_nsec = 0;

    /* Inicia o temporizador */
    if (timer_settime(current_thread->timer_id, 0, &its, NULL) == -1) {
        printf("Erro ao iniciar o temporizador!\n");
        exit(EXIT_FAILURE);
    }

    while(current_thread->sleep){
        /* Thread segue ativa */
    }
}