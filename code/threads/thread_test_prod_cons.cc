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

int buffer[BUFFER_SIZE];
// Lock *m = new Lock("ProdConsLock");
// Condition *bufferNotEmpty = new Condition("ProdConsCond1", m); 
// Condition *bufferNotFull  = new Condition("ProdConsCond2", m);
int state = 0;
int consumed = 0;
int produced = 0;
int finish = 0;

Lock* lock = new Lock("production");
Condition* producerCond = new Condition("producer", lock);
Condition* consumerCond = new Condition("consumer", lock);

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
    
    Thread *producer = new Thread("producer");
    Thread *consumer = new Thread("consumer");

    producer->Fork(producerFun, NULL);
    consumer->Fork(consumerFun, NULL);

    while(!finish) currentThread->Yield();

    printf("Test finished\n");

}












// #define M 5
// #define N 5
// #define SZ 8


// /*
//  * El buffer guarda punteros a enteros, los
//  * productores los consiguen con malloc() y los
//  * consumidores los liberan con free() 
//  */

// int *buffer[SZ];
// int usados;


// /*
// Las variables de condicion son utiles cuando se necesita una comunicacion eficiente y segura entre hilos o procesos,
// 	 evitando el uso de esperas activas o bucles de espera.
// Ayudan a evitar condiciones de carrera y sincronizan la ejecucion de los hilos o procesos involucrados.
// */

// pthread_cond_t prod; // Se utiliza para bloquear a los productores cuando el buffer esta lleno.
// pthread_cond_t cons; // Se utiliza para bloquear a los consumidores cuando el buffer esta vacio.

// pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
// // Si el mutex se libera antes del signal se podria llegar a perder la señal, perdiendo asi un valor producido 

// void enviar(int *p)
// {
//     pthread_mutex_lock (&m);
//     while (usados == SZ)			/* bloquear si el buffer esta lleno*/
// 			pthread_cond_wait (&prod, &m);
    
// 	for(int i = 0; i < SZ; i++) 
//         if(buffer[i] == NULL){
//             buffer[i] = p;
//             break;
//         }
// 	usados++;

//     pthread_cond_signal (&cons); // Aviso que el buffer no esta vacio
// 	pthread_mutex_unlock (&m);
//     return;
// }

// int * recibir()
// {
// 	pthread_mutex_lock (&m);
//     while (usados == 0)			/* bloquear si el buffer esta vacio */
// 			pthread_cond_wait (&cons, &m);
//     int *aux = NULL;
// 	int i = 0;

// 	for(int i = SZ-1; i >= 0; i--) 
//         if(buffer[i] != NULL){
//             aux = buffer[i];
//             buffer[i] = NULL;
//             break;
//         }
// 	usados--;

//     pthread_cond_signal (&prod); // Aviso que el buffer no esta lleno

// 	// Si el mutex se libera antes del signal se podria llegar a perder la señal, perdiendo asi un valor producido 
// 	pthread_mutex_unlock (&m); 

//     return aux;
// }

// void * prod_f(void *arg)
// {
// 	int id = arg - (void*)0;
// 	while (1) {
// 		sleep(random() % 3);

// 		int *p = malloc(sizeof *p);
// 		*p = random() % 100;
// 		printf("Productor %d: produje %p->%d\n", id, p, *p);
// 		enviar(p);
// 	}
// 	return NULL;
// }

// void * cons_f(void *arg)
// {
// 	int id = arg - (void*)0;
// 	while (1) {
// 		sleep(random() % 3);

// 		int *p = recibir();
// 		printf("Consumidor %d: obtuve %p->%d\n", id, p, *p);
// 		free(p);
// 	}
// 	return NULL;
// }

// int main()
// {
// 	pthread_t productores[M], consumidores[N];
// 	int i;

//     for(i = 0; i<SZ; i++) buffer[i] = NULL;

//     pthread_cond_init(&prod, NULL);
//     pthread_cond_init(&cons, NULL);

// 	for (i = 0; i < M; i++)
// 		pthread_create(&productores[i], NULL, prod_f, i + (void*)0);

// 	for (i = 0; i < N; i++)
// 		pthread_create(&consumidores[i], NULL, cons_f, i + (void*)0);

// 	pthread_join(productores[0], NULL); /* Espera para siempre */


// 	return 0;
// }


// /*
// Las variables de condición son útiles cuando se necesita una comunicación eficiente y segura entre hilos o procesos, evitando el uso de esperas activas o bucles de espera. Ayudan a evitar condiciones de carrera y sincronizan la ejecución de los hilos o procesos involucrados.
// */