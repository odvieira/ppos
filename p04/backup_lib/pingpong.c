#include "pingpong.h"

// variaveis de ambiente
#define MAIN_ID 0
#define DISP_ID 1
#define ROOT_ID 0

unsigned int current_task_id = MAIN_ID, // Identification of the task that is being executed
             running_tasks = MAIN_ID, // Number of running tasks in a moment
             created_tasks = MAIN_ID,
             created_users = ROOT_ID,
             verbose = false;	// Display info about task_switch() and task_exit() on stdout

static user_t root, common_user;

static task_t dispatcher,
       main_task;

static queue_t *suspended_tasks;

static void task_print(task_t* task) // Private Function used to display info when verbose is true
{
    printf("%d", task->id);
    return;
}

static int scheduler(task_t *aux_task_list)
{
    unsigned int id = MAIN_ID;
    short int priority_aux = 20;
    int age_aux = 0;
    task_t *aux_task = aux_task_list, *aux_ptr = NULL;

    if(verbose == true)
    {
        printf("Scheduler Iniciado\n");
        queue_print("TIDs in Scheduler: ", (queue_t*)aux_task_list, (void*)task_print);
    }

    while(aux_task->next->id != aux_task_list->id)
    {
        if(aux_task->priority - aux_task->age < priority_aux - age_aux)
        {
            priority_aux = aux_task->priority;
            age_aux = aux_task->age;
            id = aux_task->id;

            aux_ptr = aux_task;
        }

        aux_task->age = aux_task->age + 1;
        aux_task = aux_task->next;
    }

    if(aux_task->priority - aux_task->age < priority_aux - age_aux)
    {
        priority_aux = aux_task->priority;
        age_aux = aux_task->age;
        id = aux_task->id;

        aux_ptr = aux_task;
    }

    aux_task->age = aux_task->age + 1;

    aux_ptr->age = 0;
    queue_append((queue_t**)(&aux_task_list), queue_remove((queue_t**)(&aux_task_list), (queue_t*)aux_ptr));
    aux_ptr = NULL;

    if(verbose == true)
        printf("Scheduler Finalizado\n");

    return id;
}

static void dispatcher_body() // dispatcher é uma tarefa
{
    int next_id = MAIN_ID;
    task_t *aux_task;

    while (queue_size(common_user.task_list)) //
    {
        aux_task = (task_t*)(common_user.task_list);
        next_id = scheduler((task_t*)common_user.task_list); // scheduler é uma função

        if(verbose == true)
            printf("next_id obtido %d\n", next_id);

        while(aux_task->next->id != ((task_t*)(common_user.task_list))->id && next_id != aux_task->id)
            aux_task = aux_task->next;

        if(next_id != aux_task->id)
        {
            printf("Erro [dispatcher] #1: Scheduler retornou um id invalido\n");
        }
        else
        {
            task_switch(aux_task); // transfere controle para a tarefa "aux_task"
        }
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
        printf("Erro[task_change_owner] #2: task nao encontrada\n");

    return;
}

static int user_create(char *name, user_t *user)
{
    if(strlen(name) < USER_NAME_SIZE)
    {
        if(!user)
            user = (user_t*)malloc(sizeof(user_t));

        user->id = created_users++;

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

    main_task.id = created_tasks++; // main id is always zero
    main_task.next = main_task.prev = NULL;
    queue_append(&(root.task_list), (queue_t*)&main_task); // Append as head
    running_tasks++;
    main_task.owner = &root;
    main_task.priority = 0;
    main_task.status = READY;
    main_task.age = 0;

    return 0;
}

static boolean_t this_task_is_suspended(int task_id)
{
    task_t *aux_task;

    aux_task = (task_t*)suspended_tasks;

    if(!suspended_tasks)
        return false;

    while((queue_t*) (aux_task->next) != suspended_tasks && aux_task->id != task_id)
        aux_task = aux_task->next;

    if(aux_task->id == task_id)
        return true;


    return false;

}

static int task_owner(int task_id)
{
    task_t *aux_task;

    if(root.task_list != NULL)
    {
        aux_task = (task_t*)root.task_list;

        while((queue_t*) (aux_task->next) != root.task_list && aux_task->id != task_id)
            aux_task = aux_task->next;

        if(aux_task->id == task_id)
        {
            return root.id;
        }
    }

    if(common_user.task_list != NULL)
    {
        aux_task = (task_t*)common_user.task_list;

        while((queue_t*) (aux_task->next) != common_user.task_list && aux_task->id != task_id)
            aux_task = aux_task->next;

        if(aux_task->id == task_id)
        {
            return common_user.id;
        }
    }

    return -1;
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

    if(verbose == true)
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
    {
        n_task = (task_t*) malloc(sizeof(task_t));
    }

    n_task->context = (ucontext_t*) malloc(sizeof(ucontext_t));

    if(getcontext(n_task->context) < 0) // If fail throw the error
    {
        printf("Erro [task_create()] #1: falha ao atribuir contexto");
        return -1;
    }

    n_task->id = created_tasks++;	// The task id is the number of created tasks +1
    n_task->context->uc_stack.ss_sp = (char*) malloc(STACKSIZE);
    n_task->context->uc_stack.ss_size = STACKSIZE;
    n_task->context->uc_stack.ss_flags = 0;
    n_task->context->uc_link = 0;

    makecontext(n_task->context, (void*)start_func, 1, arg);

    n_task->next = n_task->prev = NULL;
    queue_append(&(common_user.task_list), (queue_t*)n_task);
    n_task->owner = &common_user;
    n_task->priority = 0;
    n_task->status = READY;
    n_task->age = 0;

    if(verbose == true)
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

    if(task_owner(task_id()) == root.id)
    {
        aux_user = &root;
    }
    else if(task_owner(task_id()) == common_user.id)
    {
        aux_user = &common_user;
    }
    else
        return;

    task_t* aux_task = (task_t*)(aux_user->task_list);

    while(aux_task->next->id != ((task_t*)(aux_user->task_list))->id && aux_task->id != current_task_id)
        aux_task = aux_task->next;

    if(aux_task->id == current_task_id)
    {
        free(aux_task->context);
        aux_task->context = NULL;
        aux_task = (task_t*) queue_remove(&(aux_user->task_list), (queue_t*)aux_task);
        free(aux_task->next);
        free(aux_task->prev);
        aux_task = NULL;
        running_tasks--;
    }
    else if(aux_task)
    {
        printf("Erro [task_exit() #2]: Tarefa não encontrada\n");
    }

    if(verbose == true)
    {
        queue_print("[task_exit()]\troot TID: ", root.task_list, (void*)task_print);
        queue_print("[task_exit()]\tuser TID: ", common_user.task_list, (void*)task_print);
    }

    if(task_owner(MAIN_ID) != root.id || task_switch((task_t*)root.task_list) < 0){
        {
            if(verbose == true)
                printf("[task_exit() #1]: Não é possível voltar para main()\n");

            printf("Finalizando o Sistema\n");
        }
    }
    /*else
        printf("Erro [task_exit()] #2: caso inesperado\n");*/

    return;
}

int task_switch (task_t *n_task)
{
    user_t *aux_user;

    if(this_task_is_suspended(task_id()) == false)
    {
        if(root.task_list && task_owner(task_id()) == root.id)
        {
            aux_user = &root;
        }
        else if(common_user.task_list && task_owner(task_id()) == common_user.id)
        {
            aux_user = &common_user;
        }
        else if(task_owner(MAIN_ID) == root.id && queue_size(common_user.task_list) == 0)
        {
            if(verbose == true)
            {
                printf("Task excluida, voltando pra main\n");
                printf("c_tid = %d \t n_tid = %d\n",current_task_id, n_task->id);
            }

            current_task_id = n_task->id;	//Caso o switch tenha sido invocado por uma tarefa já encerrada [task_exit()]
            setcontext(((task_t*)(root.task_list))->context); // cabeça da lista é sempre a main

            return 0;
        }
        else if(queue_size(common_user.task_list) > 0)
        {
            task_t *aux_task = (task_t*)root.task_list;

            while((queue_t*)(aux_task->next) != root.task_list && DISP_ID != aux_task->id)
                aux_task = aux_task->next;

            if(DISP_ID == aux_task->id)
            {
                current_task_id = MAIN_ID;
                task_yield();
                return 0;
            }

            return -1;
        }
        else
        {
            printf("Erro [task_switch()]: Task atual invalida\n");
            return -1;
        }

        task_t *aux_task = (task_t*)(aux_user->task_list);

        while((queue_t*)(aux_task->next) != aux_user->task_list && current_task_id != aux_task->id)
            aux_task = aux_task->next;

        if(current_task_id == aux_task->id)
        {
            if(verbose == true)
                printf("c_tid = %d \t n_tid = %d\n",current_task_id, n_task->id);

            current_task_id = n_task->id;

            if(swapcontext(aux_task->context, n_task->context) < 0)
                return -1;
        }
    }
    else // Caso a tarefa atual tenha sido suspensa antes da troca
    {
        task_t *aux_task = (task_t*)suspended_tasks;

        while((queue_t*)(aux_task->next) != suspended_tasks && current_task_id != aux_task->id)
            aux_task = aux_task->next;

        if(current_task_id == aux_task->id)
        {
            if(verbose == true)
                printf("c_tid = %d \t n_tid = %d\n",current_task_id, n_task->id);

            current_task_id = n_task->id;

            if(swapcontext(aux_task->context, n_task->context) < 0)
                return -1;
        }
    }



    return 0;
}

int task_id ()
{
    return current_task_id;
}

void task_setprio (task_t *task, int prio)
{
    if(task == NULL)
    {
        if(this_task_is_suspended(task_id()) == false)
        {
            user_t *aux_user;

            if(root.task_list && task_owner(task_id()) == root.id)
            {
                aux_user = &root;
            }
            else if(common_user.task_list && task_owner(task_id()) == common_user.id)
            {
                aux_user = &common_user;
            }
            else
            {
                printf("Erro [task_setprio(): Task atual invalida\n");
                return ;
            }

            task_t *aux_task = (task_t*)(aux_user->task_list);

            while((queue_t*)(aux_task->next) != aux_user->task_list && current_task_id != aux_task->id)
                aux_task = aux_task->next;

            aux_task->priority=prio;
        }
        else
        {
            task_t *aux_task = (task_t*)suspended_tasks;

            while((queue_t*)(aux_task->next) != suspended_tasks && current_task_id != aux_task->id)
                aux_task = aux_task->next;

            aux_task->priority=prio;
        }
    }
    else
    {
        task->priority = prio;
    }
}

int task_getprio (task_t *task)
{
    if(task==NULL)
    {
        if(this_task_is_suspended(task_id()) == false)
        {
            user_t *aux_user;

            if(root.task_list && task_owner(task_id()) == root.id)
            {
                aux_user = &root;
            }
            else if(common_user.task_list && task_owner(task_id()) == common_user.id)
            {
                aux_user = &common_user;
            }
            else
                return -1;

            task_t *aux_task = (task_t*)(aux_user->task_list);

            while((queue_t*)(aux_task->next) != aux_user->task_list && current_task_id != aux_task->id)
                aux_task = aux_task->next;

            return aux_task->priority;
        }
        else
        {
            task_t *aux_task = (task_t*)suspended_tasks;

            while((queue_t*)(aux_task->next) != suspended_tasks && current_task_id != aux_task->id)
                aux_task = aux_task->next;

            return aux_task->priority;
        }
    }

    return task->priority;
}

void task_suspend (task_t *n_task, task_t **queue)
{
    //queue_append((queue_t**)queue, queue_remove((queue_t**)(&(n_task->owner->task_list)), (queue_t*)n_task));
    queue_append(&suspended_tasks, queue_remove((queue_t**)(&(n_task->owner->task_list)), (queue_t*)n_task));
    return;
}
void task_suspend (task_t *n_task, task_t **queue) ;

void task_resume (task_t *n_task)
{
    queue_append((queue_t**)(&(n_task->owner->task_list)), queue_remove(&suspended_tasks, (queue_t*)n_task));
    return;
}
