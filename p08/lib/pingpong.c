#include "pingpong.h"

// variaveis de ambiente
#define MAIN_ID 0
#define DISP_ID 1
#define ROOT_ID 0

unsigned int current_task_id = MAIN_ID, // Identification of the task that is being executed
             running_tasks = MAIN_ID, // Number of running tasks in a moment
             created_tasks = MAIN_ID,
             created_users = ROOT_ID,
             quantum = 20,
             clock_var = 0,
             atom_lock = 0;

boolean_t    verbose = false,	// Display info about task_switch() and task_exit() on stdout
             time_show = true;

static user_t root, common_user;

static task_t dispatcher,
       main_task,
       ext_task;

static queue_t *suspended_tasks;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action;

// estrutura de inicialização to timer
struct itimerval timer;

static void task_print(task_t* task) // Private Function used to display info when verbose is true
{
    printf("%d", task->id);
    return;
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

static task_t* task_getcurrent()
{
    user_t *user;

    if(root.task_list && task_owner(task_id()) == root.id)
    {
        user = &root;
    }
    else if(common_user.task_list && task_owner(task_id()) == common_user.id)
    {
        user = &common_user;
    }
    else
    {
        perror("task_getcurrent(): INVALID TASK #1");
        return NULL;
    }

    task_t *c_task = (task_t*)(user->task_list);

    while((queue_t*)(c_task->next) != user->task_list && current_task_id != c_task->id)
        c_task = c_task->next;

    if(current_task_id == c_task->id)
    {
        return c_task;
    }
    else
    {
        perror("task_getcurrent(): TASK NOT FOUND");
        return NULL;
    }
}

static task_t* task_getsus(int tid)
{
    task_t* it = (task_t*)suspended_tasks;

    while(it->next != (task_t*)suspended_tasks && it->id != tid)
        it = it->next;

    if(it->id == tid)
        return it;

    perror("task_getsus(): TASK NOT FOUND\n");

    return NULL;
}

static int task_wake()
{
    task_t  *caller_task = task_getcurrent();

    if(current_task_id == caller_task->id && suspended_tasks != NULL && caller_task->dependent != NULL)
    {
        atom_lock = 1;
        if(verbose)
        {
            printf("task_wake(): Exists an dependent task, waking it...\n");
            queue_print("[task_wake()]\tsusp TID: ", suspended_tasks, (void*)task_print);
        }

        task_t *iterator_task = (task_t*)suspended_tasks;

        int_list *iterator_id = caller_task->dependent;

        while(suspended_tasks != NULL &&\
                iterator_task->next != (task_t*)suspended_tasks)
        {
            while(iterator_id != NULL &&\
                    iterator_id->next != caller_task->dependent)
            {
                if(iterator_id->value == iterator_task->id)
                {
                    iterator_task->dependency = -1;
                    queue_remove((queue_t**)(&(caller_task->dependent)), (queue_t*)iterator_id);
                    task_resume(iterator_task);

                    if(verbose)
                        printf("Task %d woke\n", iterator_id->value);
                }
                iterator_id = iterator_id->next;
            }

            if(iterator_id != NULL &&\
                    iterator_id->value == iterator_task->id)
            {
                iterator_task->dependency = -1;
                queue_remove((queue_t**)(&(caller_task->dependent)), (queue_t*)iterator_id);
                task_resume(iterator_task);

                if(verbose)
                    printf("Task %d woke\n", iterator_id->value);
            }

            iterator_task = iterator_task->next;
        }

        if(suspended_tasks != NULL)
        {
            while(iterator_id != NULL &&\
                    iterator_id->next != caller_task->dependent)
            {
                if(iterator_id->value == iterator_task->id)
                {
                    iterator_task->dependency = -1;
                    queue_remove((queue_t**)(&(caller_task->dependent)), (queue_t*)iterator_id);
                    task_resume(iterator_task);

                    if(verbose)
                        printf("Task %d woke\n", iterator_id->value);
                }
                iterator_id = iterator_id->next;
            }

            if(iterator_id != NULL &&\
                    iterator_id->value == iterator_task->id)
            {
                iterator_task->dependency = -1;
                queue_remove((queue_t**)(&(caller_task->dependent)), (queue_t*)iterator_id);
                task_resume(iterator_task);

                if(verbose)
                    printf("Task %d woke\n", iterator_id->value);
            }
        }

        if(verbose)
            printf("task_wake(): finalizado\n");

        atom_lock = 0;
    }

    return 0;
}

static int scheduler(task_t *aux_task_list)
{
    unsigned int id = DISP_ID;
    short int priority_din = -21;
    task_t *aux_task = aux_task_list, *aux_ptr = NULL;

    if(verbose == true)
    {
        printf("Scheduler Iniciado\n");
        queue_print("TIDs in Scheduler: ", (queue_t*)aux_task_list, (void*)task_print);
    }

    while(aux_task->next->id != aux_task_list->id)
    {
        if(aux_task->priority + aux_task->age > priority_din)
        {
            aux_ptr = aux_task;
            priority_din = aux_task->priority + aux_task->age;
            id = aux_task->id;
        }

        if(aux_task->priority + aux_task->age > -20 && aux_task->priority + aux_task->age < 20)
            aux_task->age = aux_task->age + 1;

        aux_task = aux_task->next;
    }

    if(aux_task->priority + aux_task->age > priority_din)
    {
        aux_ptr = aux_task;
        priority_din = aux_task->priority + aux_task->age;
        id = aux_task->id;
    }

    if(aux_task->priority + aux_task->age > -20 && aux_task->priority + aux_task->age < 20)
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
    current_task_id = DISP_ID;
    int next_id = MAIN_ID;
    task_t *aux_task;

    while (queue_size(common_user.task_list)) //
    {
        dispatcher.activations = dispatcher.activations + 1;
        aux_task = (task_t*)(common_user.task_list);
        next_id = scheduler((task_t*)common_user.task_list); // scheduler é uma função

        if(verbose == true)
            printf("next_id obtido %d\n", next_id);

        while(aux_task->next->id != ((task_t*)(common_user.task_list))->id && next_id != aux_task->id)
            aux_task = aux_task->next;

        if(next_id != aux_task->id)
        {
            task_switch(&dispatcher);
        }
        else
        {
            task_switch(aux_task); // transfere controle para a tarefa "aux_task"
        }
    }

    dispatcher.total_time = abs(systime() - aux_task->start);
    if(time_show == true)
        printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", \
               dispatcher.id, dispatcher.total_time, dispatcher.processor_time, dispatcher.activations);

    if(task_switch(&main_task) < 0)
    {
        if(verbose == true)
            printf("[task_exit() #1]: Não é possível voltar para main()\n");

        printf("Finalizando o Sistema\n");

        return;
    }

    task_exit(0) ; // encerra a tarefa dispatcher*/
}

static int task_change_owner(task_t* task, user_t* user)
{
    if(!user)
    {
        printf("Erro[task_change_owner()] #1: user invalido\n");
        return -1;
    }

    task_t *aux_task = (task_t*)(task->owner->task_list);

    while((queue_t*) (aux_task->next) != task->owner->task_list && aux_task->id != task->id)
        aux_task = aux_task->next;

    if(aux_task->id == task->id)
    {
        queue_append(&(user->task_list), queue_remove(&(task->owner->task_list), (queue_t*)task));
        task->owner = user;
    }
    else
    {
        printf("Erro[task_change_owner] #2: task nao encontrada\n");
        return -1;
    }

    return 0;
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
    main_task.dependency = -1;
    main_task.dependent = NULL;

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

void handler (int signum)
{
    clock_var++;

    if(quantum <= 0)
    {
        task_t* aux_task = task_getcurrent();

        if(aux_task->id == task_id())
        {
            aux_task->processor_time += 20;
            aux_task->activations = aux_task->activations + 1;
            quantum = 20;

            if(task_owner(task_id()) == common_user.id && !atom_lock)
                task_yield();
        }

        return;
    }

    quantum--;

    return;
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

    if(task_change_owner(&main_task, &common_user))
        printf("Erro[pingpong_init() #]: Task_owner nao pode ser trocado para main");

    // DISPATCHER
    task_create(&dispatcher, (void*)dispatcher_body, NULL);
    task_change_owner(&dispatcher, &root);

    // registra a a��o para o sinal de timer SIGALRM
    action.sa_handler = handler ;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
    //timer.it_value.tv_sec  = 3 ;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
    //timer.it_interval.tv_sec  = 20;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }

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
    n_task->dependency = -1;
    n_task->dependent = NULL;

    if(verbose == true)
    {
        queue_print("[task_create()]\troot TID: ", root.task_list, (void*)task_print);
        queue_print("[task_create()]\tuser TID: ", common_user.task_list, (void*)task_print);
    }

    running_tasks++;

    n_task->processor_time = 0;
    n_task->activations = 0;
    n_task->start = systime();

    return n_task->id;
}

void task_exit (int exitCode)
{
    if(queue_size(common_user.task_list) != 1 && task_id() == MAIN_ID)
    {
        task_yield();
    }

    if(task_wake() < 0)
    {
        perror("task_exit(): Can't wake dependents\n");
        return;
    }

    task_t* aux_task = task_getcurrent();

    if(aux_task->id == current_task_id)
    {
        aux_task->total_time = systime() - aux_task->start;
        if(time_show == true)
            printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", \
                   aux_task->id, aux_task->total_time, aux_task->processor_time, aux_task->activations);

        aux_task = (task_t*) queue_remove(&(aux_task->owner->task_list), (queue_t*)aux_task);
        free(aux_task->next);
        free(aux_task->prev);
        ext_task = *aux_task;
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
        queue_print("[task_exit()]\tsusp TID: ", suspended_tasks, (void*)task_print);
    }

    if(!(current_task_id = DISP_ID) || swapcontext(ext_task.context, dispatcher.context) < 0)
        perror("nao foi possivel voltar para o dispatcher\n");

    return;
}

int task_switch (task_t *n_task)
{
    if(this_task_is_suspended(task_id()) == false)
    {
        task_t* aux_task = task_getcurrent();

        if(current_task_id == aux_task->id)
        {
            if(verbose == true)
                printf("c_tid = %d \t n_tid = %d\n",current_task_id, n_task->id);

            current_task_id = n_task->id;

            if(swapcontext(aux_task->context, n_task->context) < 0)
                return -1;
        }
    }
    else
    {
        task_t* aux_task = task_getsus(task_id());

        if(current_task_id == aux_task->id)
        {
            if(verbose == true)
                printf("c_tid = %d \t n_tid = %d\n",current_task_id, n_task->id);

            current_task_id = DISP_ID;

            if(swapcontext(aux_task->context, dispatcher.context) < 0)
                return -1;
        }
    }


    return 0;
}

int task_id ()
{
    return current_task_id;
}

void task_setprio (task_t *n_task, int prio)
{
    if(n_task == NULL)
    {
        if(this_task_is_suspended(task_id()) == false)
        {
            n_task = task_getcurrent();

            n_task->priority=prio;

            return;
        }
        else
        {
            task_t *aux_task = (task_t*)suspended_tasks;

            while((queue_t*)(aux_task->next) != suspended_tasks && current_task_id != aux_task->id)
                aux_task = aux_task->next;

            aux_task->priority=prio;

            return;
        }
    }
    else
    {
        n_task->priority = prio;
        return;
    }
}

int task_getprio (task_t *n_task)
{
    if(n_task==NULL)
    {
        if(this_task_is_suspended(task_id()) == false)
        {
            n_task = task_getcurrent();
        }
        else
        {
            task_t *aux_task = (task_t*)suspended_tasks;

            while((queue_t*)(aux_task->next) != suspended_tasks && current_task_id != aux_task->id)
                aux_task = aux_task->next;

            return aux_task->priority;
        }
    }

    return n_task->priority;
}

void task_suspend (task_t *n_task, task_t **queue)
{
    if(!n_task)
    {
        n_task = task_getcurrent();
    }

    if(verbose)
    {
        if(!suspended_tasks)
            printf("task_suspend(): NEW suspended queue task initialized\n");
    }

    queue_append(&suspended_tasks, queue_remove((queue_t**)(&(n_task->owner->task_list)), (queue_t*)n_task));

    if(verbose)
    {
        queue_print("TIDs suspended: ", (queue_t*)suspended_tasks, (void*)task_print);
    }

    return;
}

void task_resume (task_t *n_task)
{
    if(!n_task)
    {
        task_t* aux_task = task_getcurrent();

        queue_append((queue_t**)(&(aux_task->owner->task_list)), queue_remove(&suspended_tasks, (queue_t*)aux_task));

        return;
    }

    queue_append((queue_t**)(&(n_task->owner->task_list)), queue_remove(&suspended_tasks, (queue_t*)n_task));

    return;
}

unsigned int systime()
{
    return clock_var;
}

int task_join (task_t *n_task)
{
    if(n_task == NULL)
    {
        perror("task_join(): INVALID TARGET");
        return -1;
    }

    if(n_task->id == task_id())
    {
        perror("task_join(): DEADLOCK DETECTED [CALLER = TARGET]");
        return -1;
    }

    if(this_task_is_suspended(task_id()) == true)
    {
        perror("task_join(): task already suspended [it shouldn't be here]\n");
        return -1;
    }

    task_t* caller_task = task_getcurrent();

    if(current_task_id == caller_task->id)
    {
        if(verbose)
        {
            printf("JOIN started : caller[%d] in target[%d]\n", caller_task->id, n_task->id);
        }

        if(n_task->dependency == caller_task->id)
        {
            perror("task_join(): DEADLOCK DETECTED #2");
            return -1;
        }

        atom_lock = 1;

        int_list* dep = (int_list*)malloc(sizeof(int_list));
        dep->next = dep->prev = NULL;
        dep->value = caller_task->id;

        caller_task->dependency = n_task->id;

        task_suspend(caller_task, (task_t**)(&(suspended_tasks)));

        queue_append((queue_t**)(&(n_task->dependent)), (queue_t*)dep);

        atom_lock = 0;

        task_yield();
    }

    return 0;
}
