#include "pingpong.h"

// variaveis de ambiente

unsigned int currentTaskId = 0;
unsigned int currentRunningTasks = 0;

void pingpong_init ()
{
    setvbuf(stdout, 0, _IONBF, 0);
    return;
}

int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
    if(queue_size(task_list) == 0)
    {
        task_t *main_task = (task_t*) malloc(sizeof(task_t));

        main_task->context = (ucontext_t*) malloc(sizeof(ucontext_t));

        if(getcontext(main_task->context) < 0)
            return -1;

        main_task->id = currentRunningTasks++;

        queue_append(&task_list, (queue_t*)main_task);
    }

    task->context = (ucontext_t*) malloc(sizeof(ucontext_t));

    if(getcontext(task->context) < 0)
        return -1;

    task->id = currentRunningTasks++;
    task->context->uc_stack.ss_sp = (char*) malloc(STACKSIZE);
    task->context->uc_stack.ss_size = STACKSIZE;
    task->context->uc_stack.ss_flags = 0;
    task->context->uc_link = 0;

    makecontext(task->context, (void*)start_func, 1, arg);

    queue_append(&task_list, (queue_t*)task);

    return task->id;
}

void task_exit (int exitCode)
{
    return;
}

int task_switch (task_t *task)
{
    task_t *aux;

    for(aux = (task_t*)task_list; (queue_t*)(aux->next) != task_list; aux = aux->next)
        if(currentTaskId == aux->id)
            {
                currentTaskId = task->id;
                if(swapcontext(aux->context, task->context) < 0)
					return -1;
                return 0;
            }

    if(currentTaskId == aux->id){
                currentTaskId = task->id;
                if(swapcontext(aux->context, task->context) < 0)
					return -1;
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
