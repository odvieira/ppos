// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional

#ifndef __DATATYPES__
#define __DATATYPES__

#define STACKSIZE 32768
#define USER_NAME_SIZE 15

#include <ucontext.h>

#include "queue.h"

typedef struct int_list
{
    struct int_list *prev, *next; //Aponta para o elemento anterior e seguinte na fila respectivamente
    int value;
} int_list;

typedef struct user_t
{
    unsigned int id;
    char name[USER_NAME_SIZE];
    queue_t *task_list;
} user_t;

typedef enum status_t
{
    SUSPENDED,
    SLEEPING,
    READY,
    FINISHED
} status_t;

typedef enum boolean_t
{
    false,
    true
} boolean_t;

// Estrutura que define uma tarefa
typedef struct task_t
{
    struct task_t *prev, *next; // para usar com a biblioteca de filas (cast)
    int id; // ID da tarefa
    ucontext_t *context;
    user_t *owner;
    short int priority;
    status_t status;
    int age;
    unsigned int processor_time;
    unsigned int start;
    unsigned int total_time;
    unsigned int activations;
    int dependency;
    int_list *dependent;
    int exit_code;
    long long int sleep_time;
} task_t;

// estrutura que define um semáforo
typedef struct
{
    // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
    // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
    // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
    // preencher quando necessário
} mqueue_t ;

#endif
