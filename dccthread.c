#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>
#include "dccthread.h"

// Estrutura para representar uma thread
struct dccthread {
    ucontext_t context;
    char name[DCCTHREAD_MAX_NAME_SIZE];
    int finished;
};

static dccthread_t main_thread; // Thread principal
static dccthread_t *current_thread; // Thread atual em execução
static dccthread_t *manager_thread; // Thread do gerenciador de threads

// Lista de threads prontas para execução
static struct dlist *ready_threads;

// Função do gerente de threads
static void manager_function(int param);

void dccthread_init(void (*func)(int), int param) {
    // Inicialize as estruturas internas da biblioteca
    ready_threads = dlist_create();

    // Inicialize a thread principal
    memset(&main_thread, 0, sizeof(dccthread_t));
    strcpy(main_thread.name, "main");

    // Inicialize a thread do gerente
    manager_thread = malloc(sizeof(dccthread_t));
    getcontext(&(manager_thread->context));
    manager_thread->context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
    manager_thread->context.uc_stack.ss_size = THREAD_STACK_SIZE;
    manager_thread->context.uc_link = NULL;
    makecontext(&(manager_thread->context), (void (*)())manager_function, 1, param);

    // Execute a thread principal
    setcontext(&(main_thread.context));
}

dccthread_t *dccthread_create(const char *name, void (*func)(int), int param) {
    dccthread_t *new_thread = malloc(sizeof(dccthread_t));
    getcontext(&(new_thread->context));
    new_thread->context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
    new_thread->context.uc_stack.ss_size = THREAD_STACK_SIZE;
    new_thread->context.uc_link = &(manager_thread->context); // Retorna para o gerente
    makecontext(&(new_thread->context), (void (*)())func, 1, param);
    strcpy(new_thread->name, name);

    // Adicione a nova thread à lista de threads prontas
    dlist_push_right(ready_threads, new_thread);

    return new_thread;
}

void dccthread_yield(void) {
    // Retire a thread atual da CPU e chame o gerente para escolher a próxima thread
    dccthread_t *prev_thread = current_thread;
    dlist_push_right(ready_threads, current_thread);
    current_thread = NULL;
    swapcontext(&(prev_thread->context), &(manager_thread->context));
}

dccthread_t *dccthread_self(void) {
    return current_thread;
}

const char *dccthread_name(dccthread_t *tid) {
    return tid->name;
}

static void manager_function(int param) {
    while (!dlist_empty(ready_threads)) {
        dccthread_t *next_thread = dlist_pop_left(ready_threads);
        current_thread = next_thread;
        swapcontext(&(manager_thread->context), &(current_thread->context));
    }

    // Todas as threads terminaram, incluindo a principal
    exit(0);
}
