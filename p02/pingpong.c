#include "pingpong.h"

// variaveis de ambiente

unsigned int currentTaskId = 0;
unsigned int currentRunningTasks = 0; //Manipular <<<<<<<<<<

void pingpong_init ()
{
    setvbuf(stdout, 0, _IONBF, 0);
    return;
}

int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
    if(queue_size(taskList) == 0)
    {
        task_t *mainTask = (task_t*) malloc(sizeof(task_t));

        mainTask->context = (ucontext_t*) malloc(sizeof(ucontext_t));
        mainTask->id = currentRunningTasks++;
        mainTask->next = NULL;
        mainTask->prev = NULL;

        if(getcontext(mainTask->context) < 0)
            return -1;

        queue_append(&taskList, (queue_t*)mainTask);
    }

    task = (task_t*) malloc(sizeof(task_t));
    task->context = (ucontext_t*) malloc(sizeof(ucontext_t));
    task->id = currentRunningTasks++;

    if(getcontext(task->context) < 0)
        return -1;

    task->context->uc_stack.ss_sp = (char*) malloc(STACKSIZE);
    task->context->uc_stack.ss_size = STACKSIZE;
    task->context->uc_stack.ss_flags = 0;
    task->context->uc_link = 0;

    makecontext(task->context, (void*)start_func, 1, (void*)arg);

    queue_append(&taskList, (queue_t*)task);

    return 0;
}

void task_exit (int exitCode)
{
    return;
}

int task_switch (task_t *task)
{
    task_t *aux  = (task_t*)taskList;

    for(aux = (task_t*)taskList; (queue_t*)aux->next != taskList; aux = aux->next)
        if(currentTaskId == aux->id)
            if(swapcontext(aux->context, task->context))
            {
                currentTaskId = task->id;
                return 0;
            }

    if(currentTaskId == aux->id)
            if(swapcontext(aux->context, task->context))
            {
                currentTaskId = task->id;
                return 0;
            }

    return -1;
}

int task_id ()
{
    return 0;
}

void task_suspend (task_t *task, task_t **queue)
{
    return;
}
