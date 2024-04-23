/// Routines for synchronizing threads.
///
/// The implementation for this primitive does not come with base Nachos.
/// It is left to the student.
///
/// When implementing this module, keep in mind that any implementation of a
/// synchronization routine needs some primitive atomic operation.  The
/// semaphore implementation, for example, disables interrupts in order to
/// achieve this; another way could be leveraging an already existing
/// primitive.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "condition.hh"
#include "system.hh"
#include <cstring>
#include <iostream>
/// Dummy functions -- so we can compile our later assignments.
///

Condition::Condition(const char *debugName, Lock *conditionLock)
{
    char newname[100];
    strcpy(newname, debugName);
    strcat(newname, "sem");
    lock = conditionLock;
    name = debugName;
    lock_waits = new Lock("lockWaits");
    sem = new Semaphore(newname, 0);
    DEBUG('s', "Condition variable %s created by %p\n", name, currentThread);
}

Condition::~Condition()
{
    delete lock_waits;
    delete sem;
    DEBUG('s', "Condition variable %s destroyed by %p\n", name, currentThread);
}

const char *
Condition::GetName() const
{
    return name;
}

void
Condition::Wait()
{
    ASSERT(lock->IsHeldByCurrentThread());
    lock_waits->Acquire();
    waits++;
    lock_waits->Release();

    DEBUG('s', "Thread %p waiting condicion variable %s\n", currentThread, name);
    
    DEBUG('s', "Release condition 56\n");
    lock->Release();

    sem->P();

    lock->Acquire();
}

void
Condition::Signal()
{
    ASSERT(lock->IsHeldByCurrentThread());
    lock_waits->Acquire();
    if (waits > 0) {
        sem->V();
        waits--;
    }
    lock_waits->Release();
    DEBUG('s', "Condition variable %s signaled by %p\n", name, currentThread);
}

void
Condition::Broadcast()
{
    ASSERT(lock->IsHeldByCurrentThread());
    lock_waits->Acquire();
    while (waits > 0) {
        sem->V();
        waits--;
    }
    lock_waits->Release();
    DEBUG('s', "Condition variable %s broadcasted by %p\n", name, currentThread);
}
