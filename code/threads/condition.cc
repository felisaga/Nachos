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


/// Dummy functions -- so we can compile our later assignments.
///

Condition::Condition(const char *debugName, Lock *conditionLock)
{
    name = debugName;
    lock = conditionLock;
    lock_waits = new Lock(debugName);
    signal = new Semaphore(debugName, 0);
    DEBUG('s', "Condition variable %s created by %p\n", name, currentThread);
}

Condition::~Condition()
{
    lock_waits->~Lock();
    signal->~Semaphore();
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
    lock_waits->Acquire();
    waits++;
    lock_waits->Release();

    DEBUG('s', "Thread %p waiting condicion variable %s\n", currentThread, name);
    
    lock->Release();

    signal->P();

    lock->Acquire();
}

void
Condition::Signal()
{
    lock_waits->Acquire();
    if (waits > 0) {
        signal->V();
        waits--;
    }
    lock_waits->Release();
    DEBUG('s', "Condition variable %s signaled by %p\n", name, currentThread);

}

void
Condition::Broadcast()
{
    lock_waits->Acquire();
    while (waits > 0) {
        signal->V();
        waits--;
    }
    lock_waits->Release();
    DEBUG('s', "Condition variable %s broadcasted by %p\n", name, currentThread);
}
