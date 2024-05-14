/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "thread_test_priority.hh"
#include "system.hh"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lock.hh"

static Lock *l = new Lock("lockTestScheduler");

static const int threadsAmount = 2;

void PrioritySchedulerThread(void *name_)
{

  l->Acquire();
  printf("*** Thread `%s` W/ priority %d begun execution\n", currentThread->GetName(), currentThread->GetPriority());
  for (unsigned num = 0; num < 5; num++)
  {

    printf("*** Thread `%s` W/ priority %d is running: sch iteration %u\n", currentThread->GetName(), currentThread->GetPriority(), num);

    currentThread->Yield();
  }
  l->Release();

  printf("!!! Thread `%s` has finished SimpleThread\n", currentThread->GetName());
}

void ThreadTestPriority()
{
    l->Acquire();

    Thread *t1 = new Thread("0", true, 5);
    t1->Fork(PrioritySchedulerThread, NULL);

    currentThread->Yield();
    Thread *t2 = new Thread("1", true, 6);
    t2->Fork(PrioritySchedulerThread, NULL);

    currentThread->Yield();
    Thread *t3 = new Thread("2", true, 7);
    t3->Fork(PrioritySchedulerThread, NULL);

    currentThread->Yield();
    Thread *t4 = new Thread("3", true, 8);
    t4->Fork(PrioritySchedulerThread, NULL);

    l->Release();
    currentThread->Yield();

    // Wait for all threads to finish if needed

    t1->Join();
    t2->Join();
    t3->Join();
    t4->Join();


  printf("Test finished\n");
}
