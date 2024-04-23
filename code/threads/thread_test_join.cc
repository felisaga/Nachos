/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_channel.hh"
#include "system.hh"
#include "channel.hh"
#include "thread.hh"

#include <stdio.h>


void
sleeper(void *name_)
{
    for(unsigned i = 0; i < 1<<30; i++);
}

void
nothing(void *name_)
{
    return;
}

void
ThreadTestJoin()
{
    Thread *t_sleeper, *t_nothing;

    t_nothing = new Thread("nothing", 1);
    t_nothing->Fork(nothing, NULL);
    
    t_sleeper = new Thread("sleeper", 1);
    t_sleeper->Fork(sleeper, NULL);
    

    t_sleeper->Join();
    currentThread->Yield();
    for(unsigned i = 0; i < 1<<30;  i++);
    
    t_nothing->Join();
    currentThread->Yield();

    printf("Test finished\n");
}