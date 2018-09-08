#include "pingpong.h"

// variaveis de ambiente
#define MAIN_ID 0

unsigned int current_task_id = MAIN_ID, running_tasks = 1;

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

        main_task->id = MAIN_ID;

        queue_append(&task_list, (queue_t*)main_task);
    }

    task->context = (ucontext_t*) malloc(sizeof(ucontext_t));

    if(getcontext(task->context) < 0)
        return -1;

    task->id = running_tasks;	//O id da nova tarefa é o #tasks pois o id = (0, #tasks - 1)
    task->context->uc_stack.ss_sp = (char*) malloc(STACKSIZE);
    task->context->uc_stack.ss_size = STACKSIZE;
    task->context->uc_stack.ss_flags = 0;
    task->context->uc_link = 0;

    makecontext(task->context, (void*)start_func, 1, arg);

    queue_append(&task_list, (queue_t*)task);
    running_tasks = queue_size(task_list);	// garante precisão ainda que uma task tenha sido removida

    return task->id;
}

void task_exit (int exitCode)
{
    task_t* aux;

    for(aux = (task_t*)task_list; aux && (queue_t*)(aux->next) != task_list; aux = aux->next)
        if(aux->id == current_task_id)
        {
            free(aux->context);
            aux = (task_t*) queue_remove(&task_list, (queue_t*)aux);
            free(aux->next);
            free(aux->prev);
            running_tasks = queue_size(task_list);
        }

    if(aux && aux->id == current_task_id)
    {
        free(aux->context);
        aux = (task_t*) queue_remove(&task_list, (queue_t*)aux);
        free(aux->next);
        free(aux->prev);
        running_tasks = queue_size(task_list);
    }
    else if(aux)
        printf("Erro [task_exit() #2]: Tarefa não encontrada\n");

    current_task_id = MAIN_ID;

    if(task_switch((task_t*)task_list) < 0) // A cabeça da lista de tarefas sempre é a main()
        printf("Erro [task_exit() #1]: Não é possível voltar para main()\n");

    return;
}

int task_switch (task_t *task)
{
    task_t *aux;

    for(aux = (task_t*)task_list; (queue_t*)(aux->next) != task_list; aux = aux->next)
        if(current_task_id == aux->id)
        {
            current_task_id = task->id;
            if(swapcontext(aux->context, task->context) < 0)
                return -1;
            return 0;
        }

    if(current_task_id == aux->id)
    {
        current_task_id = task->id;
        if(swapcontext(aux->context, task->context) < 0)
            return -1;
        return 0;
    }

    return -1;
}

int task_id ()
{
    return current_task_id;
}
