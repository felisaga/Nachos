/// Data structures to export a synchronous interface to the raw disk device.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_MACHINE_SYNCHCONSOLE__HH
#define NACHOS_MACHINE_SYNCHCONSOLE__HH

#include "console.hh"
#include "threads/lock.hh"
#include "threads/semaphore.hh"
#include "lib/assert.hh"

class SynchConsole {
public:

    /// Initialize a synchronous console
    SynchConsole();

    /// De-allocate the synch console data.
    ~SynchConsole();

    void ReadAvail();

    void WriteDone();

    char GetChar();

    void PutChar(char c);
private:

    Console *console;

    Lock *writeLock;

    Lock *readLock;

    Semaphore *readAvail; // Disponibles para leer

    Semaphore *writeDone; // Escritos
};


#endif
