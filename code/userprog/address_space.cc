/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "exception_type.hh"
#include "threads/system.hh"
#include "lib/utility.hh"
#include "filesys/directory_entry.hh"
#include "executable.hh"
#include <stdio.h>
#include <string.h>

int
AddressSpace::addPage(unsigned vpn){
  int frame = pages->Find(); 
  DEBUG('a', "Frame selected for page %u is %d\n", vpn, frame);
  #ifdef SWAP
  if(frame == -1) { // No hay suficiente espacio
    int victim = pages->PickVictim(machine->GetNumPhysicalPages());
    DEBUG('a', "Freeing up frame %d for page %u\n", victim, vpn);

    unsigned oldVpn = pages->GetVPN(victim);
    ASSERT(oldVpn != (unsigned) -1);
    Thread *oldThread = pages->GetThread(victim);
    ASSERT(oldThread != nullptr);
    TranslationEntry *oldEntry = oldThread->space->GetEntry(oldVpn);

    // Invalidates tlb and page table entries.
    for (unsigned i = 0; i < TLB_SIZE; i++)
        if (machine->GetMMU()->tlb[i].valid && machine->GetMMU()->tlb[i].physicalPage == (unsigned) victim){ //Actualizo los bits
          oldEntry->use = machine->GetMMU()->tlb[i].use;
          oldEntry->dirty = machine->GetMMU()->tlb[i].dirty;
          machine->GetMMU()->tlb[i].valid = false;
        }

    
    // If it's a dirty page, swap it on disk.
    //if (oldEntry->dirty || !oldThread->space->swapMap->Test(oldVpn))
        oldThread->space->SwapPage(oldVpn);

    pages->RemoveEntry(victim);

    frame = pages->Find();
    ASSERT(frame != -1);
    pages->AddEntry(frame, vpn, currentThread);
  }
  else{
    pages->AddEntry(frame, vpn, currentThread);
  }
  #endif
  
  return frame;
}

#ifdef SWAP
void
AddressSpace::SwapPage(unsigned vpn) {
  DEBUG('a', "Writing virtual page %u on disk.\n", vpn);

  unsigned physicalAddress = pageTable[vpn].physicalPage * PAGE_SIZE;
  char* mainMemory = machine->mainMemory;
  swapFile->WriteAt(&mainMemory[physicalAddress], PAGE_SIZE, vpn * PAGE_SIZE);

  pageTable[vpn].physicalPage = -1;
  swapMap->Mark(vpn);
  stats->numSwappedPages++;
}

#endif

/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file, Thread* thread)
{
    ASSERT(executable_file != nullptr);
    exe_file = executable_file;
    Executable exe = (executable_file);
    ASSERT(exe.CheckMagic());

    // How big is address space?

    unsigned size = exe.GetSize() + USER_STACK_SIZE;
      // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

    #ifndef SWAP
      // Check we are not trying to run anything too big
      ASSERT(numPages <= machine->GetNumPhysicalPages());
      ASSERT(numPages <= pages->CountClear());
    #endif

    #ifdef SWAP
      // Initialize swap file.
      DEBUG('a', "Initialize swap file\n");
      char swapFileName[FILE_NAME_MAX_LEN];
      snprintf(swapFileName, FILE_NAME_MAX_LEN, "SWAP.%u", thread->pid);
      fileSystem->Create(swapFileName, size);
      swapFile = fileSystem->Open(swapFileName);
      ASSERT(swapFile);
      threadPid = thread->pid;
      swapMap = new Bitmap(numPages);
    #endif

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.
    pageTable = new TranslationEntry[numPages];
    #ifndef DEMAND_LOADING
      char *mainMemory = machine->mainMemory; 
    #endif 
    
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
          // For now, virtual page number = physical page number.

        pageTable[i].valid        = true;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
        #ifndef DEMAND_LOADING
          pageTable[i].physicalPage = addPage(i); //pageTable[i].physicalPage = pages->Find(); // = i;
          memset(&mainMemory[pageTable[i].physicalPage * PAGE_SIZE], 0, PAGE_SIZE);
        #else
          pageTable[i].physicalPage = -1;
        #endif 
    }

    // Zero out the entire address space, to zero the unitialized data
    // segment and the stack segment.
    // memset(mainMemory, 0, size);

    #ifndef DEMAND_LOADING
    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe.GetCodeSize();
    uint32_t initDataSize = exe.GetInitDataSize();
    
    if (codeSize > 0) {
        uint32_t virtualAddr = exe.GetCodeAddr(); // Direccion virtual del codigo en el exe y del codigo en el stack virtual

        unsigned offset = 0;
        uint32_t leftToRead = codeSize;

        while (leftToRead > 0) {
            uint32_t virtualPage = DivRoundDown(virtualAddr, PAGE_SIZE);
            uint32_t physicalAddr = pageTable[virtualPage].physicalPage * PAGE_SIZE;
            
            uint32_t bytesToRead = leftToRead > PAGE_SIZE ? PAGE_SIZE : leftToRead;

            uint32_t pageOffset = virtualAddr % PAGE_SIZE;      // Posicion en la pagina desde donde hay que leer
            physicalAddr += pageOffset;

            uint32_t pageRemaining = PAGE_SIZE - pageOffset;    // me dice cuantos bytes me faltan para terminar la pagina
            if(pageRemaining < bytesToRead){
                bytesToRead = pageRemaining;
            }

            DEBUG('a', "Initializing code segment, at virtual address 0x%X, physical address 0x%X size %u\n", virtualAddr, physicalAddr, bytesToRead);

            exe.ReadCodeBlock(&mainMemory[physicalAddr], bytesToRead, offset);
            offset += bytesToRead;
            leftToRead -= bytesToRead;
            virtualAddr += bytesToRead;

            if (leftToRead > 0 && pageOffset == 0) {
                pageTable[virtualPage].readOnly = true;
            }
        }
    }
    
    if (initDataSize > 0) {
        uint32_t virtualAddr = exe.GetInitDataAddr();

        unsigned offset = 0;
        uint32_t leftToRead = initDataSize;

        while (leftToRead > 0) {
            uint32_t virtualPage = DivRoundDown(virtualAddr, PAGE_SIZE);
            uint32_t physicalAddr = pageTable[virtualPage].physicalPage * PAGE_SIZE;
            
            uint32_t bytesToRead = leftToRead > PAGE_SIZE ? PAGE_SIZE : leftToRead;

            uint32_t pageOffset = virtualAddr % PAGE_SIZE;      // Posicion en la pagina desde donde hay que leer
            physicalAddr += pageOffset;
            uint32_t pageRemaining = PAGE_SIZE - pageOffset;    // me dice cuantos bytes me faltan para terminar la pagina
            if(pageRemaining < bytesToRead){
                bytesToRead = pageRemaining;
            }

            // Por si el segmento codigo comparte paguina con el segmento de datos
            pageTable[virtualPage].readOnly = false;

            DEBUG('a', "Initializing data segment, at virtual address 0x%X, physical address 0x%X size %u\n", virtualAddr, physicalAddr, bytesToRead);

            exe.ReadDataBlock(&mainMemory[physicalAddr], bytesToRead, offset);
            offset += bytesToRead;
            leftToRead -= bytesToRead;
            virtualAddr += bytesToRead;
        }
    }

    #endif
}

TranslationEntry*
AddressSpace::LoadPage(unsigned vpn) {
  //DEBUG('e', "Load requested for page %u\n", vpn);
  if(pageTable[vpn].physicalPage != (unsigned) -1) // La pagina ya esta cargada
    return &pageTable[vpn];

  char *mainMemory = machine->mainMemory;
  pageTable[vpn].physicalPage = addPage(vpn);
  uint32_t physicalAddr = pageTable[vpn].physicalPage * PAGE_SIZE;
  memset(&mainMemory[physicalAddr], 0, PAGE_SIZE);

  #ifdef SWAP
  if(swapMap->Test(vpn)){ // Ya esta cargada en swap
    DEBUG('a', "La pagina %d esta en swap\n",vpn);
    //swapMap->Clear(vpn);
    swapFile->ReadAt(&(mainMemory[physicalAddr]), PAGE_SIZE, vpn * PAGE_SIZE);
    return &pageTable[vpn];
  }
  #endif


  Executable exe = (exe_file);
  uint32_t codeSize = exe.GetCodeSize();
  uint32_t initDataSize = exe.GetInitDataSize();
  unsigned exePos = (vpn * PAGE_SIZE); // Nuestra posicion en el exe

  // Stack
  if (exePos > codeSize + initDataSize) {
    DEBUG('a', "Initializing stack page %u, physical address 0x%X\n", vpn, physicalAddr);
    pageTable[vpn].readOnly = false;
  }

  else{
    // Es de codigo
    if(exePos < codeSize){
      DEBUG('a', "Initializing code page %u, physical address 0x%X\n", vpn, physicalAddr);
      uint32_t bytesToRead = PAGE_SIZE;//codeSize - exePos > PAGE_SIZE ? PAGE_SIZE : (codeSize - exePos);
      exe.ReadCodeBlock(&mainMemory[physicalAddr], bytesToRead, exePos);
      //pageTable[vpn].readOnly = true;
    }
    // Es de data
    else {
      DEBUG('a', "Initializing data page %u, physical address 0x%X\n", vpn, physicalAddr);
      uint32_t bytesToRead =PAGE_SIZE; // (initDataSize + codeSize) - exePos > PAGE_SIZE ? PAGE_SIZE : (initDataSize + codeSize) - exePos ;
      exe.ReadDataBlock(&mainMemory[physicalAddr], bytesToRead, (initDataSize + codeSize) - exePos);
      pageTable[vpn].readOnly = false;
    }
  }
 
  return &pageTable[vpn];
}


/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    for (unsigned i = 0; i < numPages; i++) {
      if(pageTable[i].physicalPage != (unsigned) -1)
        pages->Clear(pageTable[i].physicalPage); // = i; 
    }
    delete [] pageTable;

    #ifdef SWAP
      char swapFileName[FILE_NAME_MAX_LEN];
      snprintf(swapFileName, FILE_NAME_MAX_LEN, "SWAP.%u", threadPid);
      fileSystem->Remove(swapFileName);
    #endif
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++) {
        machine->WriteRegister(i, 0);
    }

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
  #ifdef  USE_TLB
    for(unsigned i = 0; i < TLB_SIZE; i++) {
      // Guardar bit de referencia y de modificacion en la pagina de tablas correspondiente
      //unsigned oldVpn = machine->GetMMU()->tlb[i].virtualPage;
      //TranslationEntry *oldEntry = currentThread->space->GetEntry(oldVpn);
      //if(oldEntry != nullptr){
      //  oldEntry->use = machine->GetMMU()->tlb[i].use;
      //  oldEntry->dirty = machine->GetMMU()->tlb[i].dirty;
      //}

      machine->GetMMU()->tlb[i].valid = false;
    }
  #endif

  #ifndef  USE_TLB
    machine->GetMMU()->pageTable     = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
  #endif
}

TranslationEntry*
AddressSpace::GetEntry(unsigned vpn){
  return &pageTable[vpn];
}
