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

void
Lock::Acquire()
{      
    ASSERT(!IsHeldByCurrentThread());
    sem->P();
    lockOwner = currentThread;
    DEBUG('s', "Lock %s acquired by %p\n", name, currentThread);
}

void
Lock::Release()
{
    DEBUG('s', "Lock %s try to be released by %p, lockowner %p\n", name, currentThread, lockOwner);
    ASSERT(IsHeldByCurrentThread());

    lockOwner = NULL;
    sem->V();
    DEBUG('s', "Lock %s released by %p\n", name, currentThread);
}

bool
Lock::IsHeldByCurrentThread() const
{
    return currentThread == lockOwner;
}
