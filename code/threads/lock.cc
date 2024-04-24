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
#include "lock.hh"
#include "system.hh"
#include "scheduler.hh"
#include <stdio.h>

/// Dummy functions -- so we can compile our later assignments.

Lock::Lock(const char *debugName)
{
    sem = new Semaphore(debugName, 1);
    name = debugName;
    lockOwner = NULL;
    DEBUG('s', "Lock %s created by %p\n", name, currentThread);
}

Lock::~Lock()
{
    sem->~Semaphore();
    lockOwner = NULL;
    DEBUG('s', "Lock %s destroyed by %p\n", name, currentThread);
}

const char *
Lock::GetName() const
{
    return name;
}

unsigned Lock::getMaxPriority()
{
    int i; 
    for(i = MAX_PRIORITY; i > 0; i--){
        if(priorities[i]) break;
    }
    return i;
}

/*
    El problema no se podría resolver con semáforos ya que con los mismos
    cualquier hilo puede liberar el semáforo. En cambio con los locks, al 
    solo el dueño poder levantarlo, se puede solucionar el problema.

*/

void
Lock::Acquire()
{      
    ASSERT(!IsHeldByCurrentThread());
    /// Me fijo si tengo una prioridad mas alta que la actual del lock y si la tengo cambio la del lock y la del lock owner
    unsigned priority = currentThread->GetOriginalPriority();

    if(lockOwner != nullptr &&lockOwner->GetPriority() < priority)
        scheduler->ChangePriority(lockOwner, priority);

    
    priorities[priority]++;

    sem->P();

    /// Me fijo si el lock tiene una prioridad mas alta que yo si la tiene cambio mi prioridad para tenerla
    if(getMaxPriority() > priority)
        scheduler->ChangePriority(currentThread, getMaxPriority());

    lockOwner = currentThread;
    DEBUG('s', "Lock %s acquired by %p\n", name, currentThread);
}

void
Lock::Release()
{
    DEBUG('s', "Lock %s try to be released by %p, lockowner %p\n", name, currentThread, lockOwner);
    ASSERT(IsHeldByCurrentThread());

    lockOwner = NULL;
    
    priorities[currentThread->GetOriginalPriority()] --;
    
    sem->V();
    
    /// Vuelvo mi prioridad a la original
    currentThread->SetPriority(currentThread->GetOriginalPriority());

    DEBUG('s', "Lock %s released by %p\n", name, currentThread);
}

bool
Lock::IsHeldByCurrentThread() const
{
    return currentThread == lockOwner;
}
