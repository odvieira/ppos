#include <stdio.h>
#include <stdlib.h>
#include "lib10_1/pingpong.h"

#define CONSUMIDORES_QTD 2
#define PRODUTORES_QTD 3

task_t produtores[PRODUTORES_QTD],
       consumidores[CONSUMIDORES_QTD];

semaphore_t s_buffer,
            s_item,
            s_vaga;

int buffer[5],
    posicao_insercao = 0,
    posicao_remocao
     = 0;


boolean_t verbose_pc = false;

void produtor_task(void *arg)
{
    int item;
    int id = *(int*)arg-1;

    while(true)
    {
        task_sleep (1);
        item = random() % 100;

        sem_down(&s_vaga);
        sem_down(&s_buffer);

        buffer[posicao_insercao] = item;
        if(posicao_insercao==4)
            posicao_insercao=0;
        else
            posicao_insercao++;

        printf("item produzido p%d: %d\n", id, item);

        sem_up(&s_buffer);
        sem_up(&s_item);

    }


    task_exit(0);
}

void consumidor_task(void *arg)
{
    int item;
    int id = *(int*)arg - 1 - PRODUTORES_QTD;

    while(true)
    {
        sem_down(&s_item);
        sem_down(&s_buffer);

        item = buffer[posicao_remocao];

        if(posicao_remocao==4)
            posicao_remocao=0;
        else
            posicao_remocao++;

        printf("\t\t\titem consumido c%d %d\n", id, item);


        sem_up(&s_buffer);
        sem_up(&s_vaga);

        task_sleep(1);
    }
    task_exit(0);
}

int main(int argc, char *argv[])
{
    pingpong_init();

    sem_create(&s_buffer, 5);
    sem_create(&s_item, 0);
    sem_create(&s_vaga, 5);

    for(int i = 0; i<PRODUTORES_QTD; i++)
    {
        task_create(&(produtores[i]), produtor_task, &(produtores[i].id));
    }

    for(int i = 0; i<CONSUMIDORES_QTD; i++)
    {
        task_create(&(consumidores[i]), consumidor_task, &(consumidores[i].id));
    }

    task_exit(0);
    exit(0);

}
