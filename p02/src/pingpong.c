#include "pingpong.h"

// variaveis de ambiente
#define MAIN_ID 0

unsigned int current_task_id = MAIN_ID, // Identification of the task that is being executed
             running_tasks = 1, // Number of running tasks in a moment
             verbose = false;	// Display info about task_switch() and task_exit() on stdout

void task_print(task_t* task) // Private Function used to display info when verbose is true
{
    printf("%d", task->id);
    return;
}

void pingpong_init ()
{
    task* dispatcher;
    task_create(dispatcher, (void*)dispatcher_body, (void*)0)
    setvbuf(stdout, 0, _IONBF, 0);
    return;
}

void dispatcher_body() // dispatcher é uma tarefa
{
    while (running_tasks > 0) // <<<<<<<<<< Conferir se faz sentido
    {
        next = scheduler() ; // scheduler é uma função
        if (next)
        {
            // ações antes de lançar a tarefa "next", se houverem
            task_switch (next) ; // transfere controle para a tarefa "next"
            // ações após retornar da tarefa "next", se houverem
        }
    }
    task_exit(0) ; // encerra a tarefa dispatcher
}

int task_create (task_t *n_task, void (*start_func)(void *), void *arg)
{
    if(queue_size(task_list) == 0) // The first task should be connected to main, which should be the list's head
    {
        task_t *main_task = (task_t*) malloc(sizeof(task_t));

        main_task->context = (ucontext_t*) malloc(sizeof(ucontext_t));

        if(getcontext(main_task->context) < 0) // If fail throw the error
            return -1;

        main_task->id = MAIN_ID; // main id is always zero

        queue_append(&task_list, (queue_t*)main_task); // Append as head
    }

    n_task->context = (ucontext_t*) malloc(sizeof(ucontext_t));

    if(getcontext(n_task->context) < 0) // If fail throw the error
        return -1;

    n_task->id = running_tasks;	// The task id is the number of running tasks before the new one was created
    n_task->context->uc_stack.ss_sp = (char*) malloc(STACKSIZE);
    n_task->context->uc_stack.ss_size = STACKSIZE;
    n_task->context->uc_stack.ss_flags = 0;
    n_task->context->uc_link = 0;

    makecontext(n_task->context, (void*)start_func, 1, arg);

    queue_append(&task_list, (queue_t*)n_task);
    running_tasks++;

    return n_task->id;
}

void task_exit (int exitCode)
{
    task_t* aux_task;

    for(aux_task = (task_t*)task_list; aux_task && (queue_t*)(aux_task->next) != task_list; aux_task = aux_task->next)
        if(aux_task->id == current_task_id)
        {
            free(aux_task->context);
            aux_task = (task_t*) queue_remove(&task_list, (queue_t*)aux_task);
            free(aux_task->next);
            free(aux_task->prev);
            running_tasks--;
        }

    if(aux_task && aux_task->id == current_task_id)
    {
        free(aux_task->context);
        aux_task = (task_t*) queue_remove(&task_list, (queue_t*)aux_task);
        free(aux_task->next);
        free(aux_task->prev);
        running_tasks--;
    }
    else if(aux_task)
        printf("Erro [task_exit() #2]: Tarefa não encontrada\n");

    if(verbose)
        queue_print("TID: ", task_list,(void*)task_print);

    if(task_switch((task_t*)task_list) < 0) // A cabeça da lista de tarefas sempre é a main()
        printf("Erro [task_exit() #1]: Não é possível voltar para main()\n");

    return;
}

int task_switch (task_t *n_task)
{
    task_t *aux;

    for(aux = (task_t*)task_list; (queue_t*)(aux->next) != task_list; aux = aux->next)
        if(current_task_id == aux->id)
        {
            if(verbose)
                printf("c_tid = %d \t n_tid = %d\n",current_task_id, n_task->id);
            current_task_id = n_task->id;
            if(swapcontext(aux->context, n_task->context) < 0)
                return -1;
            return 0;
        }

    if(current_task_id == aux->id)
    {
        current_task_id = n_task->id;
        if(swapcontext(aux->context, n_task->context) < 0)
            return -1;
        return 0;
    }

    current_task_id = n_task->id;	//Caso o switch tenha sido invocado por uma tarefa já encerrada [task_exit()]
    setcontext(n_task->context);

    return 0;
}

int task_id ()
{
    return current_task_id;
}
