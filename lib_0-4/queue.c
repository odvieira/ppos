#include "queue.h"

void queue_append (queue_t **queue, queue_t *elem)
{
    if(elem == NULL)
    {
        printf("Erro [queue_append() #1]: Elemento nulo\n");
        return;
    }
    if(elem->prev != NULL || elem->next != NULL)
    {
        printf("Erro [queue_append() #2]: Elemento já pertence a uma fila\n");
		return;
    }

    if(*queue == NULL)  //Fila vazia
    {
        *queue = elem;
        (*queue)->next = *queue;
        (*queue)->prev = *queue;
    }
    else
    {
        elem->prev = (*queue)->prev; //Novo elemento aponta para ultimo da lista
        (*queue)->prev = elem; //Primeiro elemento aponta para o novo ultimo
        elem->prev->next = elem; //Penúltimo aponta para o último
        elem->next = *queue; //Novo último aponta para o primeiro
    }

	return;
}

queue_t *queue_remove (queue_t **queue, queue_t *elem)
{
    if(*queue == NULL)
    {
        printf("Erro [queue_remove() #1]: Fila vazia\n");
        return NULL;
    }
    if(elem == NULL)
    {
        printf("Erro [queue_remove() #2]: Elemento inválido\n");
        return NULL;
    }

    queue_t *aux;

    // Caso elem seja a cabeça
    if ((*queue)->prev == elem->prev && *queue == elem && (*queue)->next == elem->next)
    {
        if(queue_size(*queue) == 1)   //caso a fila tenha apenas um elemento
        {
            aux = *queue;
            aux->prev = NULL;
            aux->next = NULL;
            *queue = NULL;
            return aux;
        }

        aux = *queue;
        aux->prev->next = aux->next;
        aux->next->prev = aux->prev;

        *queue = (*queue)->next;

        aux->next = NULL;
        aux->prev = NULL;

        return aux;
    }

    // CASO ELEM ESTEJA NO CORPO
    for(aux = *queue; aux->next != *queue; aux = aux->next)
        if(aux->prev == elem->prev && aux == elem && aux->next == elem->next)
        {
            aux->prev->next = aux->next;
            aux->next->prev = aux->prev;
            aux->next = NULL;
            aux->prev = NULL;
            return aux;
        }

    // CASO ELEM SEJA O ULTIMO ELEMENTO
    if(aux->prev == elem->prev && aux == elem && aux->next == elem->next)
    {
        aux->prev->next = aux->next;
        aux->next->prev = aux->prev;
        aux->next = NULL;
        aux->prev = NULL;
        return aux;
    }

    printf("Erro [queue_remove() #3]: Elemento não pertence a lista\n");

    return NULL;
}

int queue_size (queue_t *queue)
{
    if(queue == NULL)
        return 0;

    int cont = 1;
    queue_t *aux;

    for(aux = queue; aux->next != queue; aux = aux->next)
        cont++;

    return cont;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*))
{
    printf("%s[", name);

    if(queue == NULL)  //Conjunto vazio
    {
        printf("]\n");
        return;
    }

    queue_t *aux;

    for(aux = queue; aux->next != queue; aux = aux->next)
    {
        print_elem(aux);
        printf(" ");
    }

    print_elem(aux);
    printf("]\n");
}
