#ifndef NACHOS_THREADS_CHANNEL__HH
#define NACHOS_THREADS_CHANNEL__HH

#include "lock.hh"

class Channel {
public:

  /// Constructor
  Channel(const char *debugName);

  ~Channel();

  const char *GetName() const;
  
  void Send(int message);

  void Receive(int *message);



private:

  const char *name;

  int data;

  Semaphore* semSend;
  Semaphore* semReceive;
  Lock* lock;
};

#endif