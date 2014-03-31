#include "Memory.h"
#include "MemoryMap.h"

// TODO FIXE THE EMULATOR ON READ FOR SUPPORT CORRECTLY gIsDebugAccess

extern "C"
{
    #include "shared.h"
}

extern int gIsDebugAccess;

class StartDebugAccess
{
public:
    StartDebugAccess() { gIsDebugAccess++; }
    ~StartDebugAccess() { --gIsDebugAccess; }
};

const char gMemoryTypeNames[kMemory_Count][32] =
{
    "Z80",
    "68000",
    "68000 Sega CD",
    "VRAM",
    "PAL",
    "ROM",
};

uint32 gSegaCDMemoryTypes[] =
{
    kMemory_M68000,
    kMemory_S68000,
    kMemory_Z80,
    kMemory_VRAM,
    kMemory_PAL,
    kMemory_ROM,
};

uint32 gMegadriveTypes[] = 
{
    kMemory_M68000,
    kMemory_Z80,
    kMemory_VRAM,
    kMemory_PAL,
    kMemory_ROM,
};

uint32 gMasterSystemTypes[] = 
{
    kMemory_Z80,
    kMemory_VRAM,
    kMemory_PAL,
    kMemory_ROM,
};

uint32 GetMemoryCount(EmulatorHandle _hData)
{
    StartDebugAccess debug;

    if (system_hw == SYSTEM_MCD)
    {
        return sizeof(gSegaCDMemoryTypes) / sizeof(gSegaCDMemoryTypes[0]);
    }
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
    {
        return sizeof(gMegadriveTypes) / sizeof(gMegadriveTypes[0]);
    }

    return sizeof(gMasterSystemTypes) / sizeof(gMasterSystemTypes[0]);
}

MemoryHandle GetMemory(EmulatorHandle _hData, uint32 _uiMemIndex)
{
    StartDebugAccess debug;

    if (system_hw == SYSTEM_MCD)
        return (MemoryHandle)(gSegaCDMemoryTypes[_uiMemIndex] + 1);
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
        return (MemoryHandle)(gMegadriveTypes[_uiMemIndex] + 1);

    return (MemoryHandle)(gMasterSystemTypes[_uiMemIndex] + 1);
}

static const char szEmpty[] = "";

const char* GetName(MemoryHandle _Mem)
{
    StartDebugAccess debug;

    if (_Mem != NULL)
        return gMemoryTypeNames[(uint32)_Mem - 1];

    return szEmpty;
}

int GetByte(MemoryHandle _Mem, unsigned _uiPosition)
{
    StartDebugAccess debug;

    if (_Mem != NULL)
    {
        MemoryTypes_e  mem = (MemoryTypes_e)((uint32)_Mem - 1);

        switch (mem)
        {
        case kMemory_M68000:
            return READ_BYTE(m68k.memory_map[(_uiPosition >> 16) & 0xff].base, _uiPosition & 0xFFFF);
        case kMemory_S68000:
            return READ_BYTE(s68k.memory_map[(_uiPosition >> 16) & 0xff].base, _uiPosition & 0xFFFF);
        case kMemory_Z80:
            return z80_readmap[(_uiPosition & 0xffff) >> 10][(_uiPosition & 0xffff) & 0x03FF];
        case kMemory_ROM:
            {
                if (system_hw == SYSTEM_MCD || (system_hw & SYSTEM_PBC) == SYSTEM_MD)
                {
                    bool bIsPair = (_uiPosition & 0x1) == 0;
                    return cart.rom[(_uiPosition % cart.romsize & 0xfffffffe) | (bIsPair ? 1 : 0)];
                }

                return cart.rom[_uiPosition % cart.romsize];
            }            

            return cart.rom[_uiPosition % cart.romsize];
        case kMemory_VRAM:
            {
                if (system_hw == SYSTEM_MCD)
                    return vram[_uiPosition & 0xffff];
                else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
                    return vram[_uiPosition & 0xffff];

                return vram[_uiPosition & 0x3fff];
            }
        case kMemory_PAL:
            {
                if (system_hw == SYSTEM_MCD)
                    return cram[_uiPosition & 0x7f];
                else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
                    return cram[_uiPosition & 0x7f];

                return cram[_uiPosition & 0x1f];
            }            
        }
    }

    return 0;
}

bool SetByte(MemoryHandle _Mem, unsigned _uiPosition, int _Value)
{
    StartDebugAccess debug;

    return false;
}

unsigned GetSize(MemoryHandle _Mem)
{
    StartDebugAccess debug;

    MemoryTypes_e  mem = (MemoryTypes_e)((uint32)_Mem - 1);

    switch (mem)
    {
    case kMemory_M68000:
    case kMemory_S68000:
        return 0x1000000;
    case kMemory_Z80:
        return 0x10000;
    case kMemory_ROM:
        return cart.romsize;
    case kMemory_VRAM:
        if (system_hw == SYSTEM_MCD)
            return 0x10000;
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            return 0x10000;
        
        return 0x4000;
    case kMemory_PAL:
        if (system_hw == SYSTEM_MCD)
            return 0x80;
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            return 0x80;

        return 0x20;
    }

    return 0;
}

MemoryMapHandle GetMap(MemoryHandle _Mem)
{
    MemoryTypes_e  mem = (MemoryTypes_e)((uint32)_Mem - 1);

    switch (mem)
    {
    case kMemory_Z80:
        return GetZ80MemMap();
    case kMemory_M68000:
        return GetM68000MemMap();
    case kMemory_S68000:
        return GetS68000MemMap();
    }

    return NULL;
}

