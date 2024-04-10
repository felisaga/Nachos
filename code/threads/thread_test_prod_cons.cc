/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons.hh"
#include "system.hh"
#include "condition.hh"

#include <stdio.h>

#define BUFFER_SIZE 3
#define TOTAL_ITEMS 1000

static int buffer[BUFFER_SIZE];
static int state = 0;
static int consumed = 0;
static int produced = 0;
static int finish = 0;

static Lock* lock = new Lock("production");

static Condition* producerCond = new Condition("producer", lock);
static Condition* consumerCond = new Condition("consumer", lock);

void
producerFun(void *name_)
{
    while(produced < TOTAL_ITEMS-1) {
        lock->Acquire();

        if (state == BUFFER_SIZE-1) { /* bloquear si el buffer esta lleno*/
            printf("Productor esperando (buffer lleno)\n");

            /* Aseguramos que el buffer esta en la condicion necesaria */
            while (state == BUFFER_SIZE-1)
		        producerCond->Wait();
        }			
        
        printf("Productor produce: %d en %d\n", produced+1, state);
        buffer[state] = produced+1;
        state++;
        produced++;
        consumerCond->Signal();

        lock->Release();
    }
    printf("Producidos: %d\n", produced+1);
}

void
consumerFun(void *name_)
{
    while(consumed < TOTAL_ITEMS-1) {
        lock->Acquire();

        if (state == 0) { /* bloquear si el buffer esta vacio*/
            printf("Consumidor esperando (buffer vacio)\n");
            /* Aseguramos que el buffer esta en la condicion necesaria */
            while (state == 0)
		        consumerCond->Wait();
        }			
        
        printf("Consumidor consume: %d en %d\n", buffer[state-1], state);
        buffer[state-1] = 0;
        state--;
        consumed++;
        producerCond->Signal();

        lock->Release();
    }
    printf("Consumidos: %d\n", consumed+1);
    finish = 1;
}

void
ThreadTestProdCons()
{
    printf("lock %s = %p\n", "production", lock);   
    Thread *producer = new Thread("producer");
    Thread *consumer = new Thread("consumer");

    producer->Fork(producerFun, NULL);
    consumer->Fork(consumerFun, NULL);

    while(!finish) currentThread->Yield();

    delete lock;
    delete producerCond;
    delete consumerCond;

    printf("Test finished\n");
}
