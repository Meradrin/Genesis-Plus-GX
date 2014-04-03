#ifndef Memory_h
#define Memory_h

#include "API\APIMarmelade.h"

enum MemoryTypes_e
{
    kMemory_Z80,
    kMemory_M68000,
    kMemory_S68000,
    kMemory_VRAM,
    kMemory_PAL,
    kMemory_VSRAM,
    kMemory_ROM,
    kMemory_Count,
};

uint32        GetMemoryCount(EmulatorHandle _hData);
MemoryHandle  GetMemory(EmulatorHandle _hData, uint32 _uiMemIndex);

const char*   GetName(MemoryHandle _Mem);

int  GetByte(MemoryHandle _Mem, unsigned _uiPosition);
bool SetByte(MemoryHandle _Mem, unsigned _uiPosition, int _Value);

unsigned        GetSize(MemoryHandle _Mem);
MemoryMapHandle GetMap(MemoryHandle _Mem);

#endif