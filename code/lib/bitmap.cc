/// Routines to manage a bitmap -- an array of bits each of which can be
/// either on or off.  Represented as an array of integers.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "bitmap.hh"
#include <stdlib.h>
#include <stdio.h>

/// Initialize a bitmap with `nitems` bits, so that every bit is clear.  It
/// can be added somewhere on a list.
///
/// * `nitems` is the number of bits in the bitmap.
Bitmap::Bitmap(unsigned nitems)
{
    ASSERT(nitems > 0);

    numBits  = nitems;
    numWords = DivRoundUp(numBits, BITS_IN_WORD);
    map      = new unsigned [numWords];
    for (unsigned i = 0; i < numBits; i++) {
        Clear(i);
    }

    #ifdef SWAP
    //struct coreEntry coreMap[nitems];
    coreMap = (coreEntry*) malloc(sizeof(coreEntry)*nitems);
    #endif
}

/// De-allocate a bitmap.
Bitmap::~Bitmap()
{
    delete [] map;
}

/// Set the “nth” bit in a bitmap.
///
/// * `which` is the number of the bit to be set.
void
Bitmap::Mark(unsigned which)
{
    ASSERT(which < numBits);
    map[which / BITS_IN_WORD] |= 1 << which % BITS_IN_WORD;
}

/// Clear the “nth” bit in a bitmap.
///
/// * `which` is the number of the bit to be cleared.
void
Bitmap::Clear(unsigned which)
{
    ASSERT(which < numBits);
    map[which / BITS_IN_WORD] &= ~(1 << which % BITS_IN_WORD);
}

/// Return true if the “nth” bit is set.
///
/// * `which` is the number of the bit to be tested.
bool
Bitmap::Test(unsigned which) const
{
    ASSERT(which < numBits);
    return map[which / BITS_IN_WORD] & 1 << which % BITS_IN_WORD;
}

/// Return the number of the first bit which is clear.  As a side effect, set
/// the bit (mark it as in use).  (In other words, find and allocate a bit.)
///
/// If no bits are clear, return -1.
int
Bitmap::Find()
{
    for (unsigned i = 0; i < numBits; i++) {
        if (!Test(i)) {
            Mark(i);
            return i;
        }
    }
    return -1;
}

/// Return the number of clear bits in the bitmap.  (In other words, how many
/// bits are unallocated?)
unsigned
Bitmap::CountClear() const
{
    unsigned count = 0;

    for (unsigned i = 0; i < numBits; i++) {
        if (!Test(i)) {
            count++;
        }
    }
    return count;
}

#ifdef SWAP
unsigned
Bitmap::GetVPN(int frame)
{
    return coreMap[frame].vpn;
}

Thread*
Bitmap::GetThread(int frame)
{
    return coreMap[frame].thread;
}

void
Bitmap::AddEntry(int frame, unsigned vpn, Thread *current_thread)
{
    coreMap[frame].thread = current_thread;
    coreMap[frame].vpn = vpn;
}

void
Bitmap::RemoveEntry(int frame)
{
    coreMap[frame].thread = nullptr;
    coreMap[frame].vpn = (unsigned) -1;
    Clear(frame);
}

int
Bitmap::PickVictim(unsigned num_physical_pages)
{   
    static unsigned victim = -1;

    #ifdef PRPOLICY_FIFO
    victim++;
    #elif PRPOLICY_CLOCK
    static unsigned clock = 0;
    AddressSpace* space  = pages->coreMap[clock].thread->space;
    unsigned virtualPage = pages->coreMap[clock].virtualPage;

    if (!space->pageTable[virtualPage].use) {
        victim = clock;
        clock++;
        clock %= num_physical_pages;
    } else {
        space->pageTable[virtualPage].use = false;
        clock++;
        clock %= num_physical_pages;
        victim = PickVictim();
    }
    #else
    victim = SystemDep::Random();
    #endif

    return victim % num_physical_pages;
}

#endif

/// Print the contents of the bitmap, for debugging.
///
/// Could be done in a number of ways, but we just print the indexes of all
/// the bits that are set in the bitmap.
void
Bitmap::Print() const
{
    printf("Bitmap bits set:\n");
    for (unsigned i = 0; i < numBits; i++) {
        if (Test(i)) {
            printf("%u ", i);
        }
    }
    printf("\n");
}

/// Initialize the contents of a bitmap from a Nachos file.
///
/// Note: this is not needed until the *FILESYS* assignment.
///
/// * `file` is the place to read the bitmap from.
void
Bitmap::FetchFrom(OpenFile *file)
{
    ASSERT(file != nullptr);
    file->ReadAt((char *) map, numWords * sizeof (unsigned), 0);
}

/// Store the contents of a bitmap to a Nachos file.
///
/// Note: this is not needed until the *FILESYS* assignment.
///
/// * `file` is the place to write the bitmap to.
void
Bitmap::WriteBack(OpenFile *file) const
{
    ASSERT(file != nullptr);
    file->WriteAt((char *) map, numWords * sizeof (unsigned), 0);
}
