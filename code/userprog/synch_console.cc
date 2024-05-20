#include "synch_console.hh"
#include <stdio.h>

void
ReadAvailAux(void *args)
{
    ASSERT(args != nullptr);
    ((SynchConsole *)args)->ReadAvail();
}

void
WriteDoneAux(void *args)
{
    ASSERT(args != nullptr);
    ((SynchConsole *)args)->WriteDone();
}

/// Initialize the synchronous console
SynchConsole::SynchConsole()
{
    readAvail = new Semaphore("synch console readAvail", 0);

    writeDone = new Semaphore("synch console writeDone", 0);

    writeLock = new Lock("SynchConsoleWriteLock");

    readLock = new Lock("SynchConsoleReadLock");
    
    console = new Console(nullptr, nullptr, ReadAvailAux, WriteDoneAux, this);
}

/// De-allocate data structures needed for the synchronous disk abstraction.
SynchConsole::~SynchConsole()
{
    delete console;
    delete readAvail;
    delete writeDone;
    delete writeLock;
    delete readLock;
}


/// Console interrupt handlers.
/// Wake up the thread that requested the I/O.

void
SynchConsole::ReadAvail()
{
    readAvail->V();
}

void
SynchConsole::WriteDone()
{
    writeDone->V();
}

char SynchConsole::GetChar()
{
    readLock->Acquire();

    readAvail->P();
    char output = console->GetChar();
    
    readLock->Release();

    return output;
}

void SynchConsole::PutChar(char c)
{
    writeLock->Acquire();

    console->PutChar(c);
    writeDone->P();
    
    writeLock->Release();
}
