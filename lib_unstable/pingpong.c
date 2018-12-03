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
             atom_lock = 0,
             time_to_check_sleep = 100,
             clock_var = 0;

boolean_t    verbose = true,	// Display info about task_switch() and task_exit() on stdout
             time_show = true;

static user_t root, common_user;

static task_t dispatcher,
       main_task,
       ext_task;

static queue_t  *suspended_tasks,
       *sleeping_tasks,
       *destroyed;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action;

// estrutura de inicialização to timer
struct itimerval timer;

static void task_print(task_t* task) // Private Function used to display info when verbose is true
{
    printf("%d", task->id);
    return;
}

static void print_qstatus()
{
    queue_print("[task_switch()]\troot TID: ", root.task_list, (void*)task_print);
    queue_print("[task_switch()]\tuser TID: ", common_user.task_list, (void*)task_print);
    queue_print("[task_switch()]\tsusp TID: ", suspended_tasks, (void*)task_print);
    queue_print("[task_switch()]\tslep TID: ", sleeping_tasks, (void*)task_print);
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
        if(verbose == true)
        {
            queue_print("[task_switch()]\troot TID: ", root.task_list, (void*)task_print);
            queue_print("[task_switch()]\tuser TID: ", common_user.task_list, (void*)task_print);
            queue_print("[task_switch()]\tsusp TID: ", suspended_tasks, (void*)task_print);
            queue_print("[task_switch()]\tslep TID: ", sleeping_tasks, (void*)task_print);

            printf("task_getcurrent: trying to get tid[%d]\n", current_task_id);

            printf("task_getcurrent: invalid task\n");
        }

        if(setcontext(dispatcher.context) < 0)
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

static int task_wake()
{
    task_t  *caller_task = task_getcurrent();

    atom_lock = 1;

    if(queue_size((queue_t*)(caller_task->dependent)) == 0)
    {
        if(verbose)
            printf("No one to wake\n");

        return 0;
    }

    if(current_task_id == caller_task->id && suspended_tasks != NULL && caller_task->dependent != NULL)
    {
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

static void check_sleeping_tasks()
{
    task_t  *it = (task_t*)sleeping_tasks;

    task_t *aux = NULL;

    unsigned int now = systime();

    while(sleeping_tasks != NULL && it->next != (task_t*)sleeping_tasks)
    {
        if(now >= it->sleep_time && it->sleep_time >= 0)
        {
            it->status = READY;

            aux = (task_t*)queue_remove(&sleeping_tasks, (queue_t*)it);

            if(verbose)
            {
                queue_print("TIDs sleeping: ", sleeping_tasks, (void*)task_print);
                printf("[TIME: %d] sleep time over for tid[%d]->sleep_time[%lld]\n",\
                       now, it->id, it->sleep_time);
                printf("Appending tid[%d] to %s (%p) from sleeping_tasks (%p)\n",\
                       aux->id, aux->owner->name,\
                       (void *)(aux->owner->task_list),\
                       (void *)&sleeping_tasks);
            }

            aux->sleep_time = -1;

            queue_append(&(it->owner->task_list), (queue_t*)aux);

            it = (task_t*) sleeping_tasks;

            if(verbose)
            {
                queue_print("TIDs sleeping [it copy]: ", (queue_t*)it, (void*)task_print);
            }
        }

        it = it->next;
    }

    if(sleeping_tasks != NULL && now >= it->sleep_time && it->sleep_time >= 0)
    {
        it->status = READY;

        aux = (task_t*)queue_remove(&sleeping_tasks, (queue_t*)it);

        if(verbose)
        {
            queue_print("TIDs sleeping: ", sleeping_tasks, (void*)task_print);
            printf("[TIME: %d] sleep time over for tid[%d]->sleep_time[%lld]\n",\
                   now, it->id, it->sleep_time);
            printf("Appending tid[%d] to %s (%p) from sleeping_tasks (%p)\n",\
                   aux->id, aux->owner->name,\
                   (void *)(aux->owner->task_list),\
                   (void *)&sleeping_tasks);
        }

        aux->sleep_time = -1;

        queue_append(&(it->owner->task_list), (queue_t*)aux);

        it = (task_t*) sleeping_tasks;

        if(verbose)
        {
            queue_print("TIDs sleeping [it copy]: ", (queue_t*)it, (void*)task_print);
        }
    }

    atom_lock = 0;
}

static void dispatcher_body() // dispatcher é uma tarefa
{
    current_task_id = DISP_ID;
    int next_id = MAIN_ID;
    task_t *aux_task;

    while (queue_size(common_user.task_list) || queue_size(sleeping_tasks) || queue_size(suspended_tasks)) //
    {
        if(queue_size(common_user.task_list))
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
                setcontext(dispatcher.context);
            }
            else
            {
                task_switch(aux_task); // transfere controle para a tarefa "aux_task"
            }
        }

        if(time_to_check_sleep <= 0 && sleeping_tasks)
        {
            time_to_check_sleep = 100;
            check_sleeping_tasks();
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

    task_exit(0) ; // encerra a tarefa dispatcher
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

void handler (int signum)
{
    clock_var++;
    time_to_check_sleep--;

    task_t* aux_task = task_getcurrent();

    aux_task->processor_time += 1;

    if(quantum <= 0)
    {
        if(aux_task->id == task_id())
        {
            aux_task->activations = aux_task->activations + 1;
            quantum = 20;

            if(task_owner(task_id()) == common_user.id)
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

    destroyed = (queue_t*)malloc(sizeof(queue_t));
    destroyed->next = destroyed->prev = NULL;

    return;
}

void task_yield ()
{
    if(atom_lock == 0 && current_task_id != DISP_ID)
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
    n_task->exit_code = 0;

    if(verbose == true)
    {
        queue_print("[task_create()]\troot TID: ", root.task_list, (void*)task_print);
        queue_print("[task_create()]\tuser TID: ", common_user.task_list, (void*)task_print);
    }

    running_tasks++;

    n_task->processor_time = 0;
    n_task->activations = 0;
    n_task->sleep_time = -1;
    n_task->start = systime();

    return n_task->id;
}

void task_exit (int exitCode)
{
    if(verbose)
        printf("void task_exit (int exitCode) [START]\n");

    atom_lock = 1;

    task_t* aux_task = task_getcurrent();

    if(aux_task->exit_code == 0)
        aux_task->exit_code = exitCode;

    if(verbose)
    {
        printf("Finishing TID[%d]\t", aux_task->id);
        queue_print("DEPENDENTS: ", (queue_t*)(aux_task->dependent), (void*)task_print);

        task_t* it = (task_t*) suspended_tasks;

        printf("Remaining task in SUSPENDED list\n");

        while(suspended_tasks && it->next != (task_t*)suspended_tasks)
        {
            printf("TID[%d] -> dependency[%d]\n", it->id, it->dependency);
            it = it->next;
        }

        if(suspended_tasks)
        {
            printf("TID[%d] -> dependency[%d]\n", it->id, it->dependency);
        }
    }

    if(task_wake() < 0)
    {
        perror("task_exit(): Can't wake dependents\n");

        atom_lock = 0;

        return;
    }

    while((queue_size(common_user.task_list) > 1 ||\
            queue_size(sleeping_tasks) > 0 ||\
            queue_size(suspended_tasks) > 0) &&\
            task_id() == MAIN_ID)
    {
        if(queue_size(common_user.task_list) == 1 && sleeping_tasks)
            task_join((task_t*)sleeping_tasks);

        if(queue_size(common_user.task_list) == 1 &&\
                queue_size(suspended_tasks) > 0 && queue_size(sleeping_tasks) == 0)
            task_resume((task_t*)suspended_tasks);

        atom_lock = 0;

        task_yield();
    }

    if(aux_task->id == current_task_id)
    {
        aux_task->status = FINISHED;

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
        queue_print("TIDs sleeping: ", sleeping_tasks, (void*)task_print);
        printf("void task_exit (int exitCode) [Trying to return to DISP]\n");
    }

    atom_lock = 0;

    if(!(current_task_id = DISP_ID) || swapcontext(ext_task.context, dispatcher.context) < 0)
        perror("nao foi possivel voltar para o dispatcher\n");

    return;
}

int task_switch (task_t *n_task)
{
    atom_lock = 1;

    if(verbose)
        printf("int task_switch (task_t *n_task) START\n");

    if(verbose == true)
    {
        queue_print("[task_switch()]\troot TID: ", root.task_list, (void*)task_print);
        queue_print("[task_switch()]\tuser TID: ", common_user.task_list, (void*)task_print);
        queue_print("[task_switch()]\tsusp TID: ", suspended_tasks, (void*)task_print);
        queue_print("[task_switch()]\tslep TID: ", sleeping_tasks, (void*)task_print);
    }

    if(task_owner(task_id()) != -1)
    {
        task_t* aux_task = task_getcurrent();

        if(current_task_id == aux_task->id)
        {
            if(verbose == true)
                printf("c_tid = %d \t n_tid = %d\n",current_task_id, n_task->id);

            current_task_id = n_task->id;

            if(verbose)
                printf("int task_switch [SWAP]\n");

            atom_lock = 0;

            if(swapcontext(aux_task->context, n_task->context) < 0)
                return -1;
        }
    }

    atom_lock = 0;

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
        if(task_owner(current_task_id) != -1)
        {
            n_task = task_getcurrent();

            n_task->priority=prio * (-1);

            return;
        }
        else
        {
            perror("task_setprio\n");
            exit(1);
        }
    }
    else
    {
        n_task->priority = prio * (-1);
        return;
    }
}

int task_getprio (task_t *n_task)
{
    if(n_task==NULL)
    {
        if(task_owner(task_id()) != -1)
            n_task = task_getcurrent();
        else
        {
            perror("task_getprio\n");
            exit(1);
        }
    }

    return n_task->priority * (-1);
}

void task_suspend (task_t *n_task, task_t **queue)
{
    fflush(stdin);
    atom_lock = 1;

    if(!n_task)
    {
        n_task = task_getcurrent();
    }

    if(verbose)
    {
        printf("Suspending tid[%d] (should be tid[%d]) to queue_adress: %p\n",\
               n_task->id, task_id(), (void *)queue);
        if(!suspended_tasks)
            printf("task_suspend(): NEW suspended queue task initialized\n");
    }
    fflush(stdin);

    if(n_task->status == READY)
        n_task->status = SUSPENDED;

    int id = n_task->id;

    task_t* aux = (task_t*)queue_remove((queue_t**)(&(n_task->owner->task_list)), (queue_t*)n_task);

    queue_append((queue_t**)(queue), (queue_t*)aux);

    if(verbose)
    {
        queue_print("TIDs suspended in this queue: ", (queue_t*)*queue, (void*)task_print);
    }

    fflush(stdin);

    if(current_task_id == id)
    {
        if(verbose)
            printf("task_suspend: c_tid = %d \t n_tid = %d\n",current_task_id, DISP_ID);

        fflush(stdin);

        current_task_id = DISP_ID;

        atom_lock = 0;

        if(swapcontext(aux->context, dispatcher.context) < 0)
        {
            printf("task_suspend: Can't save the current context\ncalling task_yield()\n");
            task_yield();
        }
    }

    return;
}

void task_resume (task_t *n_task)
{
    atom_lock = 1;

    if(!n_task)
    {
        n_task = task_getcurrent();
    }

    if(verbose)
        printf("task_resume: START\n");

    n_task->status = READY;

    task_t  *aux = (task_t*)queue_remove((queue_t**)(&(suspended_tasks)), (queue_t*)n_task);

    if(verbose)
    {
        queue_print("TIDs left on the list: ", (queue_t*)suspended_tasks, (void*)task_print);
        printf("Appending tid[%d] to %s (%p) from list (%p)\n",\
               aux->id, aux->owner->name,\
               (void *)&(aux->owner->task_list),\
               (void *)&suspended_tasks);
    }

    queue_append((queue_t**)(&(n_task->owner->task_list)), (queue_t*)aux);

    atom_lock = 0;

    return;
}

unsigned int systime()
{
    return clock_var;
}

int task_join (task_t *n_task)
{
    atom_lock = 1;

    if(n_task == NULL)
    {
        perror("task_join(): INVALID TARGET #1");

        atom_lock = 0;

        return -1;
    }

    if(n_task->status == FINISHED || n_task->status == SUSPENDED)
    {
        if(verbose)
            printf("task_join: tried to join an invalid task, returning\n");

        atom_lock = 0;

        return -1;
    }

    if(n_task->id == task_id())
    {
        perror("task_join(): DEADLOCK DETECTED [CALLER = TARGET]");

        atom_lock = 0;

        return -1;
    }

    task_t* caller_task = task_getcurrent();

    if(verbose)
    {
        printf("JOIN started : caller[%d] in target[%d]\n", caller_task->id, n_task->id);
    }

    if(n_task->dependency == caller_task->id)
    {
        perror("task_join(): DEADLOCK DETECTED #2");

        atom_lock = 0;

        return -1;
    }

    int_list* dep = (int_list*)malloc(sizeof(int_list));
    dep->next = dep->prev = NULL;
    dep->value = caller_task->id;

    caller_task->dependency = n_task->id;

    queue_append((queue_t**)(&(n_task->dependent)), (queue_t*)dep);

    task_suspend(caller_task, (task_t**)(&(suspended_tasks)));

    atom_lock = 0;

    task_yield();

    return n_task->exit_code;
}

void task_sleep (int t)
{
    atom_lock = 1;

    fflush(stdin);

    task_t *caller = task_getcurrent();

    if(t < 0)
        t = 0;

    caller->sleep_time = systime() + t * 100;

    fflush(stdin);


    if(verbose)
    {
        printf("task_sleep(): tid = %d \t now: %d \t sleep_time: %lld\n",\
               caller->id, systime(), caller->sleep_time);

        printf("caller id[%d] status changed to SLEEPING\n", caller->id);
    }

    fflush(stdin);


    caller->status = SLEEPING;

    task_suspend(caller, (task_t**)(&(sleeping_tasks)));

    atom_lock = 0;

    return;
}

int sem_create(semaphore_t* s, int value)
{
    if(!s)
        s = (semaphore_t*)malloc(sizeof(semaphore_t));

    s->sem_tasks = NULL;
    s->value = value;

    return 0;
}

int sem_down(semaphore_t* s)
{
    if(verbose)
    {
        printf("sem_down START\n");
        queue_print("TIDs in: ", s->sem_tasks, (void*)task_print);

    }

    if(!s)
    {
        printf("sem_down: invalid semaphore\n");
        return -1;
    }

    atom_lock = 1;

    s->value = s->value - 1;

    task_t *current_task = task_getcurrent();

    if(task_owner(current_task->id) == -1)
        printf("sem_down: tid[%d] invalid\n", current_task->id);

    if(s->value < 0)
    {
        current_task->status = SUSPENDED;

        current_task->exit_code = -1;

        task_t* aux = (task_t*)queue_remove((queue_t**)(&(current_task->owner->task_list)),\
                                            (queue_t*)current_task);

        queue_append((queue_t**)(&(s->sem_tasks)), (queue_t*)aux);

        if(verbose)
        {
            printf("sem_down [%p]: semaphore full suspending tid[%d]\n",\
                   s, current_task->id);
            printf("sem_down: c_tid = %d \t n_tid = %d\n",current_task_id, DISP_ID);
        }

        current_task_id = DISP_ID;

        if(verbose)
        {
            queue_print("TIDs in: ", s->sem_tasks, (void*)task_print);
            printf("sem_down END\n");
        }

        fflush(stdin);

        atom_lock = 0;

        if(swapcontext(aux->context, dispatcher.context) < 0)
            return -1;
    }
    else if(verbose)
    {
        printf("sem_down [%p] by tid [%d]\n",\
               s, current_task->id);
    }

    if(verbose)
    {
        queue_print("TIDs in: ", s->sem_tasks, (void*)task_print);
        printf("sem_down END\n");

    }

    return 0;
}

int sem_up(semaphore_t* s)
{
    if(verbose)
    {
        printf("sem_up START\n");
        queue_print("TIDs in: ", s->sem_tasks, (void*)task_print);

    }

    if(!s)
    {
        printf("sem_up: invalid semaphore\n");
        return -1;
    }

    s->value = s->value + 1;

    if(verbose)
    {
        printf("sem_up [%p] by tid [%d]\n",\
               s, current_task_id);
    }

    if(s->value < 1)
    {
        task_t *aux = (task_t*)s->sem_tasks;

        if(s->sem_tasks)
        {
            aux = (task_t*) queue_remove((queue_t**)(&(s->sem_tasks)),\
                                         (queue_t*)aux);

            if(aux)
            {
                aux->status = READY;

                aux->exit_code = 0;

                queue_append((queue_t**)(&(aux->owner->task_list)),\
                             (queue_t*) aux);
            }
            else
            {
                printf("sem_up: can't get task from sem_tasks\n");
                return -1;
            }
        }
        else
        {
            perror("invalid semaphore value\n");

            return -1;
        }
    }

    if(verbose)
    {
        queue_print("TIDs in: ", s->sem_tasks, (void*)task_print);
        printf("sem_up END\n");
    }

    return 0;
}

int sem_destroy(semaphore_t *s)
{
    if(verbose)
        printf("sem_destroy START\n");

    if(!s)
    {
        printf("sem_destroy: invalid semaphore\n");
        return -1;
    }

    atom_lock = 1;

    if(s->sem_tasks)
    {
        task_t *aux = (task_t*)s->sem_tasks;

        do
        {
            aux->status = READY;

            queue_append((queue_t**)(&(aux->owner->task_list)),\
                         queue_remove((queue_t**)(&(s->sem_tasks)),\
                                      (queue_t*)aux));
        }
        while(s->sem_tasks);
    }
    else if(verbose)
    {
        printf("sem_destroy: no task to wake\n");
    }

    s->sem_tasks = destroyed;

    atom_lock = 0;

    return 0;
}

int barrier_create (barrier_t *b, int N)
{
    atom_lock = 1;

    if(!b)
        b = (barrier_t*) malloc(sizeof(barrier_t));

    b->total_size = N;

    atom_lock = 0;

    return 0;

}

int barrier_join (barrier_t *b)
{
    atom_lock = 1;

    if(!b)
    {
        if(verbose)
            printf("barrier_join: barreira inexistente ou destruida\n");

        atom_lock = 0;

        return -1;
    }

    task_t *current_task = task_getcurrent();


    if(verbose)
    {
        printf("cid[%d]\n", current_task->id);
        queue_print("TIDs in this barrier: ", b->barrier_tasks, (void*)task_print);
    }

    if(queue_size(b->barrier_tasks) < b->total_size - 1)
    {
        current_task->status = SUSPENDED;

        task_t *aux = (task_t*)queue_remove((queue_t**)(&(current_task->owner->task_list)),\
                                            (queue_t*)current_task);

        queue_append((queue_t**)(&(b->barrier_tasks)), (queue_t*)aux);

        current_task_id = DISP_ID;

        if(verbose)
        {
            queue_print("[AFTER JOIN] TIDs in this barrier: ",\
                        b->barrier_tasks, (void*)task_print);
        }

        fflush(stdin);

        atom_lock = 0;

        fflush(stdin);

        if(swapcontext(aux->context, dispatcher.context) < 0)
            return -1;
    }
    else
    {
        task_t  *it, *aux;

        while(b->barrier_tasks)
        {
            it = (task_t*)b->barrier_tasks;

            aux = (task_t*)queue_remove((queue_t**)(&(b->barrier_tasks)),\
                                        (queue_t*)it);

            aux->status = READY;

            queue_append((queue_t**)(&(aux->owner->task_list)), (queue_t*)aux);
        }

        current_task_id = DISP_ID;

        if(verbose)
        {
            queue_print("[AFTER JOIN - BARRIER FILLED] TIDs in this barrier: ",\
                        b->barrier_tasks, (void*)task_print);
            print_qstatus();
        }

        fflush(stdin);

        atom_lock = 0;

        fflush(stdin);

        if(swapcontext(current_task->context, dispatcher.context) < 0)
            return -1;
    }

    return 0;
}

int barrier_destroy (barrier_t *b)
{
    if(!b)
    {
        if(verbose)
            printf("barrier_destroy: b is null\n");

        return -1;
    }

    atom_lock = 1;

    task_t  *it, *aux;

    while(b->barrier_tasks)
    {
        it = (task_t*)b->barrier_tasks;

        aux = (task_t*)queue_remove((queue_t**)(&(b->barrier_tasks)),\
                                    (queue_t*)it);

        aux->status = READY;
        aux->exit_code = -1;

        queue_append((queue_t**)(&(aux->owner->task_list)), (queue_t*)aux);
    }

    b->total_size = 0;

    fflush(stdin);

    atom_lock = 0;

    task_yield();

    return 0;
}

int mqueue_create (mqueue_t *queue, int max, int size)
{
    if(!queue)
        queue = (mqueue_t*)malloc(sizeof(mqueue_t));

    atom_lock = 1;

    queue->msg_content = (void*)malloc(size*max);
    queue->msg_size = size;
    queue->msg_counter = 0;

    sem_create(&(queue->mutex), 1);
    sem_create(&(queue->msg_seats), max);
    sem_create(&(queue->read), 0);

    atom_lock = 0;


    return 0;
}

int mqueue_send (mqueue_t *queue, void *msg)
{
    if(!queue)
    {
        if(verbose)
            printf("mqueue_send: invalid queue\n");

        return -1;
    }

    int flag = 0;

    while(!flag && queue && queue->mutex.sem_tasks != destroyed)
    {
        atom_lock = 1;
        if(queue->mutex.value > 0 && queue->msg_seats.value > 0)
        {
            if(verbose)
                printf("mqueue_send: (queue->mutex.value > 0 && queue->msg_seats.value > 0) == true\n");

            sem_down(&(queue->mutex));
            sem_down(&(queue->msg_seats));
            flag = 1;
        }

        if(!flag)
        {
            if(verbose)
                printf("Não foi possível alocar recursos\n");

            atom_lock = 0;
            task_sleep(1);
        }
    }

    if(queue->mutex.sem_tasks == destroyed)
    {
        if(verbose)
            printf("mqueue_send: destroyed queue\n");

        return -1;
    }

    atom_lock = 1;

    fflush(stdin);

    memcpy(queue->msg_content + (queue->msg_size * queue->msg_counter), msg, queue->msg_size);

    queue->msg_counter = queue->msg_counter + 1;

    atom_lock = 0;

    sem_up(&(queue->read));
    sem_up(&(queue->mutex));

    return 0;
}

int mqueue_recv (mqueue_t *queue, void *msg)
{
    if(!queue)
    {
        if(verbose)
            printf("mqueue_send: invalid queue\n");

        return -1;
    }

    int flag = 0;

    atom_lock = 1;

    while(!flag && queue && queue->mutex.sem_tasks != destroyed)
    {
        atom_lock = 1;
        if(queue->mutex.value > 0 && queue->read.value > 0)
        {
            if(verbose)
                printf("mqueue_send: (queue->mutex.value > 0 && queue->msg_seats.value > 0) == true\n");

            sem_down(&(queue->mutex));
            sem_down(&(queue->read));
            flag = 1;
        }

        if(!flag)
        {
            if(verbose)
            {
                printf("Não foi possível alocar recursos\n");
            }
            atom_lock = 0;
            task_sleep(1);
        }
    }

    if(queue->mutex.sem_tasks == destroyed)
    {
        if(verbose)
            printf("mqueue_send: destroyed queue\n");

        return -1;
    }

    fflush(stdin);

    memcpy(msg, queue->msg_content, queue->msg_size);

    queue->msg_counter -= 1;

    int i;

    void* aux = queue->msg_content;

    for(i = 0; i < queue->msg_counter; i++)
    {
        memcpy(aux, aux + queue->msg_size, queue->msg_size);
        aux += queue->msg_size;
    }

    sem_up(&(queue->msg_seats));
    sem_up(&(queue->mutex));

    atom_lock = 0;

    return 0;
}

int mqueue_destroy (mqueue_t *queue)
{
    if(!queue)
        return -1;

    sem_destroy(&(queue->mutex));
    sem_destroy(&(queue->msg_seats));
    sem_destroy(&(queue->read));

    atom_lock = 1;

    free(queue->msg_content);
    queue->msg_counter = queue->msg_size = 0;

    atom_lock = 0;

    return 0;
}

int mqueue_msgs (mqueue_t *queue)
{
    if(!queue)
        return -1;

    return queue->msg_counter;
}
