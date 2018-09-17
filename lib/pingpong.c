#include "pingpong.h"

// variaveis de ambiente
#define MAIN_ID 0

unsigned int current_task_id = MAIN_ID, // Identification of the task that is being executed
             running_tasks = 1, // Number of running tasks in a moment
             verbose = false;	// Display info about task_switch() and task_exit() on stdout

task_t dispatcher,
       main_task;

static void task_print(task_t* task) // Private Function used to display info when verbose is true
{
    printf("%d", task->id);
    return;
}

static void dispatcher_body() // dispatcher é uma tarefa
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
    task_exit(0) ; // encerra a tarefa dispatcher*/
}

void pingpong_init ()
{
    setvbuf(stdout, 0, _IONBF, 0);

    task_list = NULL;

    main_task.context = (ucontext_t*) malloc(sizeof(ucontext_t));

    if(getcontext(main_task.context) < 0) // If fail throw the error
        return ;

    main_task.id = MAIN_ID; // main id is always zero
    main_task.label = (char*) malloc(sizeof(char)*TASK_LABEL);
    main_task.label = "main\0";

    // Appends
    main_task.next = main_task.prev = NULL;
    dispatcher.next = dispatcher.prev = NULL;
    queue_append(&task_list, (queue_t*)&main_task); // Append as head

    if(verbose)
        printf("Main criada\n");

    return;
}

void task_yield ()
{
    //task_switch(dispatcher);
}

int task_create (task_t *n_task, void (*start_func)(void *), void *arg)
{
    n_task->label = (char*) malloc(sizeof(char)*TASK_LABEL);
    sprintf(n_task->label, "task_%d", running_tasks);

    n_task->context = (ucontext_t*) malloc(sizeof(ucontext_t));

    if(getcontext(n_task->context) < 0) // If fail throw the error
        return -1;

    n_task->id = running_tasks;	// The task id is the number of running tasks before the new one was created
    n_task->context->uc_stack.ss_sp = (char*) malloc(STACKSIZE);
    n_task->context->uc_stack.ss_size = STACKSIZE;
    n_task->context->uc_stack.ss_flags = 0;
    n_task->context->uc_link = 0;

    makecontext(n_task->context, (void*)start_func, 1, arg);

    n_task->next = n_task->prev = NULL;
    queue_append(&task_list, (queue_t*)n_task);

    if(verbose)
    {
        printf("Criando task %s\n", n_task->label);
        queue_print("[task_create()]\tTID: ", task_list, (void*)task_print);
    }

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
        queue_print("[task_exit()]\tTID: ", task_list,(void*)task_print);

    if(!task_list || task_switch((task_t*)task_list) < 0) // A cabeça da lista de tarefas sempre é a main()
    {
        if(verbose)
            printf("[task_exit() #1]: Não é possível voltar para main()\n");

        printf("Finalizando o Sistema\n");
    }

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

    if(n_task)
    {
        current_task_id = n_task->id;	//Caso o switch tenha sido invocado por uma tarefa já encerrada [task_exit()]
        setcontext(n_task->context); // cabeça da lista é sempre a main
    }
    else
        return -1;

    return 0;
}

int task_id ()
{
    return current_task_id;
}
