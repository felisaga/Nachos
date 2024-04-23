/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_channel.hh"
#include "system.hh"
#include "channel.hh"

#include <stdio.h>

#define THREADS_CANT 1000
static bool done[THREADS_CANT];

static Channel* channel = new Channel("canal");

void
senderFun(void *id_)
{
    unsigned *id = (unsigned *) id_;
    channel->Send(*id);
}

void
receiverFun(void *id_)
{
    unsigned *id = (unsigned *) id_;
 
    int buf;
    channel->Receive(&buf);

    printf("Recieved %d by %u\n", buf, *id);

    done[*id] = true;
}

void
ThreadTestChannel()
{
    unsigned *values = new unsigned[THREADS_CANT];
    Thread *senders[THREADS_CANT], *receivers[THREADS_CANT];

    for (unsigned i = 0; i < THREADS_CANT; i++){
        values[i] = i;
        senders[values[i]] = new Thread("sender", 1);
        senders[values[i]]->Fork(senderFun, (void *) &(values[i]));
        receivers[values[i]] = new Thread("receiver", 1);
        receivers[values[i]]->Fork(receiverFun, (void *) &(values[i]));
    }

    // for (unsigned i = 0; i < THREADS_CANT; i++){
    //     values[i] = i;
    //     senders[values[i]]->Join();
    //     receivers[values[i]]->Join();
    // }

    for (unsigned i = 0; i < THREADS_CANT; i++) {
        while (!done[i]) {
            currentThread->Yield();
        }
    }
    
    delete channel;

    printf("Test finished\n");
}