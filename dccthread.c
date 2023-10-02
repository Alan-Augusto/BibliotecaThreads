#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>
#include "dlist.h"
#include "dccthread.h"

/* Estrutura para representar uma thread */
struct dccthread {
    ucontext_t context;
    char name[DCCTHREAD_MAX_NAME_SIZE];
    int finished;
};

static dccthread_t main_thread; // Thread principal
static dccthread_t *current_thread; // Thread atual em execução
static dccthread_t *manager_thread; // Thread gerente

/* Lista de threads prontas para execução */
static struct dlist *ready_threads;

static void manager_function(int param) {
    /* Verifica se existem threads prontas para executar */
    while (!dlist_empty(ready_threads)) {
        /* Obtém a próxima thread da lista de threads prontas */
        dccthread_t *next_thread = dlist_pop_left(ready_threads);

        /* Coloca essa thread para executar */
        current_thread = next_thread;
        swapcontext(&(manager_thread->context), &(next_thread->context));
    }
}

void dccthread_init(void (*func)(int), int param) {
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
    strcpy(new_thread->name, name);
    getcontext(&(new_thread->context));
    new_thread->context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
    new_thread->context.uc_stack.ss_size = THREAD_STACK_SIZE;
    new_thread->context.uc_link = &(manager_thread->context); 
    makecontext(&(new_thread->context), (void (*)())func, 1, param);

    /* Adiciona a thread recém criada à lista de threads prontas para executar */
    dlist_push_right(ready_threads, new_thread);

    return new_thread;
}

void dccthread_yield(void) {
    /* Retira a thread em execução da CPU e chama a thread gerente para escolher a próxima thread */
    dccthread_t *prev_thread = current_thread;
    dlist_push_right(ready_threads, current_thread);
    current_thread = NULL;
    swapcontext(&(prev_thread->context), &(manager_thread->context));
}

dccthread_t *dccthread_self(void) {
    /* Retorna a thread em execução */
    return current_thread;
}

const char *dccthread_name(dccthread_t *tid) {
    /* Retorna o nome de determinada thread */
    return tid->name;
}

void dccthread_exit(void) {
    /* Marca a thread atual como terminada */
    current_thread->finished = 1;

    /* Chama a função do gerente para escolher a próxima thread a ser executada */
    setcontext(&(manager_thread->context));
}

void dccthread_wait(dccthread_t *tid) {
    /* Verifica se a thread já terminou */
    if (tid->finished) {
        return;
    }

    /* Se a thread ainda não terminou, coloca a thread atual na lista de threads prontas e chama o gerente */
    dlist_push_right(ready_threads, current_thread);
    current_thread = NULL;
    setcontext(&(manager_thread->context));
}
