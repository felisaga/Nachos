#include "lock.hh"
#include "system.hh"
#include "channel.hh"

#include <cstring>
#include <stdio.h>

Channel::Channel(const char *debugName) {
  name = debugName;

  char *lockName = new char[strlen(debugName) + 14];
  sprintf(lockName, "ChannelLock-%s", debugName);
  lock = new Lock(lockName);

  char *semSendName = new char[strlen(debugName) + 10];
  sprintf(semSendName, "SemSend-%s", debugName);
  semSend = new Semaphore(semSendName, 0);

  char *semReceiveName = new char[strlen(debugName) + 10];
  sprintf(semReceiveName, "SemSend-%s", debugName);
  semReceive = new Semaphore(semReceiveName, 0);

  DEBUG('s', "Channel %s created by %p\n", name, currentThread);
  
  data = 0;
}

Channel::~Channel() {
  delete lock;
  delete semReceive;
  delete semSend;
}

void Channel::Send(int message) {
  lock->Acquire();

  data = message;
  semReceive->V();  
  semSend->P();

  lock->Release(); 
}

void Channel::Receive(int *message) {
  semReceive->P(); 
  *message = data;
  semSend->V();
}