#ifndef Memory_h
#define Memory_h

#include "API\APIMarmelade.h"

uint32        GetMemoryCount(EmulatorHandle _hData);
MemoryHandle  GetMemory(EmulatorHandle _hData, uint32 _uiMemIndex);

const char*   GetName(MemoryHandle _Mem);

int  GetByte(MemoryHandle _Mem, unsigned _uiPosition);
bool SetByte(MemoryHandle _Mem, unsigned _uiPosition, int _Value);

unsigned        GetSize(MemoryHandle _Mem);
MemoryMapHandle GetMap(MemoryHandle _Mem);

#endif