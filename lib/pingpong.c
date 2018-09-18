#include "pingpong.h"

// variaveis de ambiente
#define MAIN_ID 0

unsigned int current_task_id = MAIN_ID, // Identification of the task that is being executed
             running_tasks = 0, // Number of running tasks in a moment
             created_tasks = 0,
             created_users = 0,
             verbose = true;	// Display info about task_switch() and task_exit() on stdout

static user_t root, common_user;

static task_t dispatcher,
       main_task;

static void task_print(task_t* task) // Private Function used to display info when verbose is true
{
    printf("%d", task->id);
    return;
}

static int scheduler(queue_t *aux_task_list)
{
    unsigned int id = MAIN_ID;
    short int p_aux = 32767;
    task_t *aux_task = (task_t*)aux_task_list, *aux_ptr;

    while((queue_t*)(aux_task->next) != aux_task_list)
        if(aux_task->priority < p_aux)
        {
            p_aux = aux_task->priority;
            id = aux_task->id;
            aux_ptr = aux_task;
        }

    if(aux_task->priority < p_aux)
    {
        p_aux = aux_task->priority;
        id = aux_task->id;
        aux_ptr = aux_task;
    }

    aux_ptr->priority = aux_ptr->priority + 1;
    aux_ptr = NULL;

    return id;
}

static void dispatcher_body() // dispatcher é uma tarefa
{
    int next_id = MAIN_ID;
    task_t *aux_task = (task_t*)(common_user.task_list);

    while (queue_size(common_user.task_list)) //
    {
        next_id = scheduler((task_t*)common_user.task_list) ; // scheduler é uma função

        while((queue_t*)(aux_task->next) != common_user.task_list && next_id != aux_task->id)
            aux_task = aux_task->next;

        if(next_id != aux_task->id)
            printf("Erro [dispatcher] #1: Scheduler retornou um id invalido");
        else
            task_switch(aux_task); // transfere controle para a tarefa "aux_task"
    }

    task_exit(0) ; // encerra a tarefa dispatcher*/
}

static void task_change_owner(task_t* task, user_t* user)
{
    if(!user)
        printf("Erro[task_change_owner()] #1: user invalido\n");

    task_t *aux_task = (task_t*)(task->owner->task_list);

    while((queue_t*) (aux_task->next) != task->owner->task_list && aux_task->id != task->id)
        aux_task = aux_task->next;

    if(aux_task->id == task->id)
    {
        queue_append(&(user->task_list), queue_remove(&(task->owner->task_list), (queue_t*)task));
        task->owner = user;
    }
    else
        printf("Erro[task_cjange_owner] #2: task nao encontrada\n");

    return;
}

static int user_create(char *name, user_t *user)
{
    if(strlen(name) < USER_NAME_SIZE)
    {
        if(!user)
            user = (user_t*)malloc(sizeof(user_t));

        user->id = ++created_users;

        strcat(user->name, name);

        return user->id;
    }
    else
    {
        printf("Erro[user_create()] #1: nome invalido");
        return -1;
    }
}

static int main_task_create()
{
    main_task.context = (ucontext_t*) malloc(sizeof(ucontext_t));

    if(getcontext(main_task.context) < 0) // If fail throw the error
        return -1;

    main_task.id = MAIN_ID; // main id is always zero
    main_task.next = main_task.prev = NULL;
    queue_append(&(root.task_list), (queue_t*)&main_task); // Append as head
    running_tasks++;
    main_task.owner = &root;
    main_task.priority = 0;

    return 0;
}

static int is_root_task(int task_id)
{
    task_t *aux_task = (task_t*)root.task_list;

    while((queue_t*) (aux_task->next) != root.task_list && aux_task->id != task_id)
        aux_task = aux_task->next;

    if(aux_task->id != task_id)
        return 0;
    else
        return 1;
}

void pingpong_init ()
{
    setvbuf(stdout, 0, _IONBF, 0);

    //Criando os Usuarios do S.O
    user_create("root", &root);
    user_create("common_user", &common_user);

    // Inicializando tasks do S.O

    // MAIN
    if(main_task_create())
        printf("Erro [pingpong_init()] #1: Nao foi possivel criar main\n");

    if(verbose)
        printf("Main criada\n");

    // DISPATCHER
    task_create(&dispatcher, (void*)dispatcher_body, NULL);
    task_change_owner(&dispatcher, &root);

    return;
}

void task_yield ()
{
    task_switch(&dispatcher);
}

int task_create (task_t *n_task, void (*start_func)(void *), void *arg)
{
    if(!n_task)
        n_task = (task_t*) malloc(sizeof(task_t));

    if(!n_task->context)
        n_task->context = (ucontext_t*) malloc(sizeof(ucontext_t));

    if(getcontext(n_task->context) < 0) // If fail throw the error
    {
        printf("Erro [task_create()] #1: falha ao atribuir contexto");
        return -1;
    }

    n_task->id = ++created_tasks;	// The task id is the number of created tasks +1
    n_task->context->uc_stack.ss_sp = (char*) malloc(STACKSIZE);
    n_task->context->uc_stack.ss_size = STACKSIZE;
    n_task->context->uc_stack.ss_flags = 0;
    n_task->context->uc_link = 0;

    makecontext(n_task->context, (void*)start_func, 1, arg);

    n_task->next = n_task->prev = NULL;
    queue_append(&(common_user.task_list), (queue_t*)n_task);
    n_task->owner = &common_user;
    n_task->priority = 0;

    if(verbose)
    {
        queue_print("[task_create()]\troot TID: ", root.task_list, (void*)task_print);
        queue_print("[task_create()]\tuser TID: ", common_user.task_list, (void*)task_print);
    }


    running_tasks++;

    return n_task->id;
}

void task_exit (int exitCode)
{
    user_t *aux_user;

    if(is_root_task(task_id()))
        aux_user = &root;
    else
        aux_user = &common_user;

    task_t* aux_task;

    for(aux_task = (task_t*)(aux_user->task_list); aux_task && (queue_t*)(aux_task->next) != aux_user->task_list; aux_task = aux_task->next)
        if(aux_task->id == current_task_id)
        {
            free(aux_task->context);
            aux_task = (task_t*) queue_remove(&(aux_user->task_list), (queue_t*)aux_task);
            free(aux_task->next);
            free(aux_task->prev);
            running_tasks--;
        }

    if(aux_task && aux_task->id == current_task_id)
    {
        free(aux_task->context);
        aux_task = (task_t*) queue_remove(&(aux_user->task_list), (queue_t*)aux_task);
        free(aux_task->next);
        free(aux_task->prev);
        running_tasks--;
    }
    else if(aux_task)
        printf("Erro [task_exit() #2]: Tarefa não encontrada\n");

    if(verbose)
    {
        queue_print("[task_exit()]\troot TID: ", root.task_list, (void*)task_print);
        queue_print("[task_exit()]\tuser TID: ", common_user.task_list, (void*)task_print);
    }

    if(!aux_user->task_list || task_switch((task_t*)(aux_user->task_list)) < 0) // A cabeça da lista de tarefas sempre é a main()
    {
        if(verbose)
            printf("[task_exit() #1]: Não é possível voltar para main()\n");

        printf("Finalizando o Sistema\n");
    }

    return;
}

int task_switch (task_t *n_task)
{
    user_t *aux_user;

    if(is_root_task(task_id()))
        aux_user = &root;
    else
        aux_user = &common_user;


    task_t *aux;

    for(aux = (task_t*)(aux_user->task_list); (queue_t*)(aux->next) != aux_user->task_list; aux = aux->next)
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
