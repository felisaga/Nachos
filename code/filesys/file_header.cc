/// Routines for managing the disk file header (in UNIX, this would be called
/// the i-node).
///
/// The file header is used to locate where on disk the file's data is
/// stored.  We implement this as a fixed size table of pointers -- each
/// entry in the table points to the disk sector containing that portion of
/// the file data (in other words, there are no indirect or doubly indirect
/// blocks). The table size is chosen so that the file header will be just
/// big enough to fit in one disk sector,
///
/// Unlike in a real system, we do not keep track of file permissions,
/// ownership, last modification date, etc., in the file header.
///
/// A file header can be initialized in two ways:
///
/// * for a new file, by modifying the in-memory data structure to point to
///   the newly allocated data blocks;
/// * for a file already on disk, by reading the file header from disk.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "file_header.hh"
#include "threads/system.hh"

#include <ctype.h>
#include <stdio.h>


/// Initialize a fresh file header for a newly created file.  Allocate data
/// blocks for the file out of the map of free disk blocks.  Return false if
/// there are not enough free blocks to accomodate the new file.
///
/// * `freeMap` is the bit map of free disk sectors.
/// * `fileSize` is the bit map of free disk sectors.
bool
FileHeader::Allocate(Bitmap *freeMap, unsigned fileSize)
{
    ASSERT(freeMap != nullptr);

    raw.numBytes = fileSize;
    raw.numSectors = DivRoundUp(fileSize, SECTOR_SIZE);
    if (freeMap->CountClear() < raw.numSectors) {
        return false;  // Not enough space.
    }

    if (fileSize <= MAX_FILE_SIZE) {
        DEBUG('f', "Creando file header simple\n");
        for (unsigned i = 0; i < raw.numSectors; i++)
            raw.dataSectors[i] = freeMap->Find();
    }
    else {
        unsigned numIndirect = DivRoundUp(raw.numSectors, NUM_DIRECT);
        DEBUG('f', "Creando file header con indireccion\n");
        for (unsigned i = 0; i < numIndirect; i++) {
            int sector = freeMap->Find();
            raw.dataSectors[i] = sector;

            unsigned toWrite = fileSize > MAX_FILE_SIZE ? MAX_FILE_SIZE : fileSize;
            FileHeader *h = new FileHeader;
            bool success = h->Allocate(freeMap, toWrite);
            
            if(success) h->WriteBack(sector);
            else return false;

            if(fileSize > MAX_FILE_SIZE) fileSize -= MAX_FILE_SIZE;
            if(fileSize == 0) break; // Por las dudas
            delete h;
        }
    }
    return true;
}


/// De-allocate all the space allocated for data blocks for this file.
///
/// * `freeMap` is the bit map of free disk sectors.
void
FileHeader::Deallocate(Bitmap *freeMap)
{
    ASSERT(freeMap != nullptr);

    if (raw.numBytes <= MAX_FILE_SIZE) {
        for (unsigned i = 0; i < raw.numSectors; i++){
            ASSERT(freeMap->Test(raw.dataSectors[i]));  // ought to be marked!
            freeMap->Clear(raw.dataSectors[i]);
        }
    }
    else {
        unsigned numIndirect = DivRoundUp(raw.numSectors, NUM_DIRECT);
        for (unsigned i = 0; i < numIndirect; i++) {
            int sector = raw.dataSectors[i];
            ASSERT(freeMap->Test(raw.dataSectors[i]));  // ought to be marked!

            FileHeader *h = new FileHeader;
            h->FetchFrom(sector);
            h->Deallocate(freeMap);
            freeMap->Clear(sector);
            delete h;
        }
    }

}

void 
FileHeader::ModifySector(Bitmap *freeMap, unsigned id, unsigned sectorNumber, bool clearOld){
    if(clearOld) freeMap->Clear(raw.dataSectors[id]);
    raw.dataSectors[id] = sectorNumber;
}

bool 
FileHeader::Extend(Bitmap *freeMap, unsigned newFilesize)
{
    ASSERT(freeMap != nullptr);
    if(newFilesize <= raw.numBytes) return true; // Nothing to do
    
    unsigned bytesToAdd = newFilesize - raw.numBytes;
    unsigned sectorsToAdd = DivRoundUp(bytesToAdd, SECTOR_SIZE);

    if (freeMap->CountClear() < sectorsToAdd) {
        return false;  // Not enough space.
    }

    if(raw.numBytes <= MAX_FILE_SIZE){ // No indirection already in place
        if(newFilesize <= MAX_FILE_SIZE){ // It fits with no change needed 
            for(unsigned i = raw.numSectors; i < sectorsToAdd; i++)
                raw.dataSectors[i] = freeMap->Find();
            raw.numBytes = newFilesize;
            raw.numSectors = DivRoundUp(newFilesize, SECTOR_SIZE);
            return true;
        }
        else{ // Indirection needed
            FileHeader *newHdr = new FileHeader; // New header with indirection
            bool success = newHdr->Allocate(freeMap, newFilesize); // Populate it (lo creamos)
            if(!success) return false;
            const RawFileHeader *newRaw = newHdr->GetRaw();
            FileHeader *ToBeCopiedInto = new FileHeader; // Header of the first second level header of newHdr
            ToBeCopiedInto->FetchFrom(newHdr->GetRaw()->dataSectors[0]); 

            for(unsigned i = 0; i < raw.numSectors; i++)
               ToBeCopiedInto->ModifySector(freeMap, i, raw.dataSectors[i], true); //We change the first raw.numSectors sectors to point to the old sectors

            ToBeCopiedInto->WriteBack(newHdr->GetRaw()->dataSectors[0]); // Save changes

            for(unsigned i = 0; i < DivRoundUp(newRaw->numSectors, NUM_DIRECT); i++) // Swap old header for new
                raw.dataSectors[i] = newRaw->dataSectors[i];
            raw.numBytes = newRaw->numBytes;
            raw.numSectors = newRaw->numSectors;

            delete newHdr;
            delete ToBeCopiedInto;
            return true;
        }
    }

    // Indirection already in place but needs to be extended
    unsigned fstLevelId = DivRoundDown(raw.numSectors, NUM_DIRECT);
    unsigned sndLevelId = raw.numSectors % NUM_DIRECT;
    FileHeader *sndLevel = new FileHeader;
    sndLevel->FetchFrom(raw.dataSectors[fstLevelId]);

    while(sectorsToAdd > 0 && sndLevelId >= NUM_DIRECT){ // Fill snd level header 
        sndLevel->ModifySector(freeMap, sndLevelId++, freeMap->Find(), false);
        sectorsToAdd--;
        bytesToAdd -= SECTOR_SIZE;
    }
    sndLevel->WriteBack(raw.dataSectors[fstLevelId++]);

    while(sectorsToAdd > 0 && fstLevelId >= NUM_DIRECT){
        if(sectorsToAdd > (MAX_FILE_SIZE/SECTOR_SIZE)) {
            bool success = sndLevel->Allocate(freeMap, MAX_FILE_SIZE);
            if(!success) return false;
            sectorsToAdd -= (MAX_FILE_SIZE/SECTOR_SIZE);
            bytesToAdd -= MAX_FILE_SIZE;
        }
        else{
            bool success = sndLevel->Allocate(freeMap, bytesToAdd);
            if(!success) return false;
            sectorsToAdd = 0;
            bytesToAdd = 0;
        }
        sndLevel->WriteBack(raw.dataSectors[fstLevelId++]);
    }
    delete sndLevel;
    /*
    unsigned fstLevelId = DivRoundDown(raw.numSectors, NUM_DIRECT);
    unsigned sndLevelId = raw.numSectors % NUM_DIRECT;
    FileHeader *sndLevel = new FileHeader;
    sndLevel->FetchFrom(raw.dataSectors[fstLevelId]);

    for(unsigned i = 0; i < sectorsToAdd; i++){
        sndLevel->ModifySector(freeMap, sndLevelId++, freeMap->Find(), false);
        
        if(sndLevelId >= NUM_DIRECT){
            sndLevel->WriteBack(raw.dataSectors[fstLevelId]);
            fstLevelId++;
            sndLevel->FetchFrom(raw.dataSectors[fstLevelId]);
            sndLevelId = 0;
        }
    }*/
    return true;

}


/// Fetch contents of file header from disk.
///
/// * `sector` is the disk sector containing the file header.
void
FileHeader::FetchFrom(unsigned sector)
{
    synchDisk->ReadSector(sector, (char *) &raw);
}

/// Write the modified contents of the file header back to disk.
///
/// * `sector` is the disk sector to contain the file header.
void
FileHeader::WriteBack(unsigned sector)
{
    synchDisk->WriteSector(sector, (char *) &raw);
}

/// Return which disk sector is storing a particular byte within the file.
/// This is essentially a translation from a virtual address (the offset in
/// the file) to a physical address (the sector where the data at the offset
/// is stored).
///
/// * `offset` is the location within the file of the byte in question.
unsigned
FileHeader::ByteToSector(unsigned offset)
{
    if(raw.numBytes <= MAX_FILE_SIZE){ //Esta en este nivel
        DEBUG('f',"aaaaaaaaaaaaaaaaa\n");
        return raw.dataSectors[offset / SECTOR_SIZE];
    }
    else{
        int sector = raw.dataSectors[offset / MAX_FILE_SIZE];
        FileHeader *hdr = new FileHeader; 
        hdr->FetchFrom(sector);
        unsigned res = hdr->ByteToSector(offset % MAX_FILE_SIZE);
        DEBUG('f',"Translate pos: %u a %u, sector %d\n",offset, res, sector);
        delete hdr;
        return res;
    }
}

/// Return the number of bytes in the file.
unsigned
FileHeader::FileLength() const
{
    return raw.numBytes;
}

/// Print the contents of the file header, and the contents of all the data
/// blocks pointed to by the file header.
void
FileHeader::Print(const char *title)
{
    char *data = new char [SECTOR_SIZE];

    if (title == nullptr) {
        printf("File header:\n");
    } else {
        printf("%s file header:\n", title);
    }

    printf("    size: %u bytes\n"
           "    block indexes: ",
           raw.numBytes);

    for (unsigned i = 0; i < raw.numSectors; i++) {
        printf("%u ", raw.dataSectors[i]);
    }
    printf("\n");

    for (unsigned i = 0, k = 0; i < raw.numSectors; i++) {
        printf("    contents of block %u:\n", raw.dataSectors[i]);
        synchDisk->ReadSector(raw.dataSectors[i], data);
        for (unsigned j = 0; j < SECTOR_SIZE && k < raw.numBytes; j++, k++) {
            if (isprint(data[j])) {
                printf("%c", data[j]);
            } else {
                printf("\\%X", (unsigned char) data[j]);
            }
        }
        printf("\n");
    }
    delete [] data;
}

const RawFileHeader *
FileHeader::GetRaw() const
{
    return &raw;
}
