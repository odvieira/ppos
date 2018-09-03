#include "pingpong.h"

void pingpong_init ()
{
    //setvbuf(stdout, 0, _IONBF, 0);
    return;
}

int task_create (task_t *task, void (*start_func)(void *), void *arg)
{

    task = (task_t*) malloc(sizeof(task_t));

    queue_append(&taskList, (queue_t*)task);

    if(getcontext(&task->context) < 0)
        return -1;

    task->context.uc_stack.ss_sp = (char*) malloc(STACKSIZE);
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;

    makecontext(&task->context, (void*)start_func, 1, (void*)arg);

    return 0;
}

void task_exit (int exitCode)
{
    return;
}

int task_switch (task_t *task)
{
    task_t *aux;
    ucontext_t ucont;

    if(getcontext(&ucont) < 0)
        return -1;

    for(aux = (task_t*)taskList; (queue_t*)aux->next != taskList; aux = aux->next)
        if(1)
            swapcontext(&(aux->context), &(task->context));

    return 0;
}

int task_id ()
{
    return 0;
}

void task_suspend (task_t *task, task_t **queue)
{
    return;
}
