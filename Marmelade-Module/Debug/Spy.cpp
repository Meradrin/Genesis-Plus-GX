#include "Spy.h"
#include "API\APIMarmelade.h"
#include "Memory.h"
#include "Breakpoint.h"
#include "DebugCPU.h"

extern "C"
{
    #include "shared.h"

    extern m68ki_cpu_core m68k;
    extern m68ki_cpu_core s68k;
};

int gIsDebugAccess = 0;
unsigned gTmpValue = 0;

extern ExecutionBreak gExecutionBreak;
extern uint8 system_hw;

int SpyM68kBusPreRead(unsigned _uiAddr, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return 0;

    if (IsNeedBreak(kMemory_M68000, _uiAddr, BREAK_FLAG_READ) || (_uiSize >= 2 && IsNeedBreak(kMemory_M68000, _uiAddr + 1, BREAK_FLAG_READ))
        || (_uiSize >= 3 && IsNeedBreak(kMemory_M68000, _uiAddr + 2, BREAK_FLAG_READ)) || (_uiSize >= 4 && IsNeedBreak(kMemory_M68000, _uiAddr + 3, BREAK_FLAG_READ)))
    {
        gbNeedBreak[1] = true;
        guiStepCount[1] = -1;
        gExecutionBreak(1);
    }

    return 0;
}

int SpyM68kBusPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return 0;

    if (IsNeedBreak(kMemory_M68000, _uiAddr, BREAK_FLAG_WRITE) || (_uiSize >= 2 && IsNeedBreak(kMemory_M68000, _uiAddr + 1, BREAK_FLAG_WRITE))
        || (_uiSize >= 3 && IsNeedBreak(kMemory_M68000, _uiAddr + 2, BREAK_FLAG_WRITE)) || (_uiSize >= 4 && IsNeedBreak(kMemory_M68000, _uiAddr + 3, BREAK_FLAG_WRITE)))
    {
        gbNeedBreak[1] = true;
        guiStepCount[1] = -1;
        gExecutionBreak(1);
    }

    return 0;
}

int SpyS68kBusPreRead(unsigned _uiAddr, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return 0;

    if (IsNeedBreak(kMemory_S68000, _uiAddr, BREAK_FLAG_READ) || (_uiSize >= 2 && IsNeedBreak(kMemory_S68000, _uiAddr + 1, BREAK_FLAG_READ))
        || (_uiSize >= 3 && IsNeedBreak(kMemory_S68000, _uiAddr + 2, BREAK_FLAG_READ)) || (_uiSize >= 4 && IsNeedBreak(kMemory_S68000, _uiAddr + 3, BREAK_FLAG_READ)))
    {
        gbNeedBreak[2] = true;
        guiStepCount[2] = -1;
        gExecutionBreak(2);
    }

    return 0;
}

int SpyS68kBusPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return 0;

    if (IsNeedBreak(kMemory_S68000, _uiAddr, BREAK_FLAG_WRITE) || (_uiSize >= 2 && IsNeedBreak(kMemory_S68000, _uiAddr + 1, BREAK_FLAG_WRITE))
        || (_uiSize >= 3 && IsNeedBreak(kMemory_S68000, _uiAddr + 2, BREAK_FLAG_WRITE)) || (_uiSize >= 4 && IsNeedBreak(kMemory_S68000, _uiAddr + 3, BREAK_FLAG_WRITE)))
    {
        gbNeedBreak[2] = true;
        guiStepCount[2] = -1;
        gExecutionBreak(2);
    }

    return 0;
}

int SpyZ80BusPreRead(unsigned _uiAddr, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return _uiAddr;

    if (IsNeedBreak(kMemory_Z80, _uiAddr, BREAK_FLAG_READ) || (_uiSize == 2 && IsNeedBreak(kMemory_Z80, _uiAddr + 1, BREAK_FLAG_READ)))
    {
        gbNeedBreak[0] = true;
        guiStepCount[0] = -1;
        gExecutionBreak(0);
    }

    return _uiAddr;
}

int SpyZ80BusPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return _uiAddr;

    if (IsNeedBreak(kMemory_Z80, _uiAddr, BREAK_FLAG_WRITE) || (_uiSize == 2 && IsNeedBreak(kMemory_Z80, _uiAddr + 1, BREAK_FLAG_WRITE)))
    {
        gbNeedBreak[0] = true;
        guiStepCount[0] = -1;
        gExecutionBreak(0x0);
    }

    return _uiAddr;
}

int SpyVDPVRamPreRead(unsigned _uiAddr, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return 0;

    if (IsNeedBreak(kMemory_VRAM, _uiAddr, BREAK_FLAG_READ) || (_uiSize == 2 && IsNeedBreak(kMemory_VRAM, _uiAddr + 1, BREAK_FLAG_READ)))
    {
        gbNeedBreak[0] = true;
        guiStepCount[0] = -1;

        if (system_hw == SYSTEM_MCD)
            gExecutionBreak(0x1);
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            gExecutionBreak(0x1);
        else
            gExecutionBreak(0x0);
    }

    return 0;
}

int SpyVDPVRamPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return 0;

    if (IsNeedBreak(kMemory_VRAM, _uiAddr, BREAK_FLAG_WRITE) || (_uiSize == 2 && IsNeedBreak(kMemory_VRAM, _uiAddr + 1, BREAK_FLAG_WRITE)))
    {
        gbNeedBreak[0] = true;
        guiStepCount[0] = -1;

        if (system_hw == SYSTEM_MCD)
            gExecutionBreak(0x1);
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            gExecutionBreak(0x1);
        else
            gExecutionBreak(0x0);
    }

    return 0;
}

int SpyVDPCRamPreRead(unsigned _uiAddr, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return 0;

    if (IsNeedBreak(kMemory_PAL, _uiAddr, BREAK_FLAG_READ) || (_uiSize == 2 && IsNeedBreak(kMemory_PAL, _uiAddr + 1, BREAK_FLAG_READ)))
    {
        gbNeedBreak[0] = true;
        guiStepCount[0] = -1;

        if (system_hw == SYSTEM_MCD)
            gExecutionBreak(0x1);
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            gExecutionBreak(0x1);
        else
            gExecutionBreak(0x0);
    }

    return 0;
}

int SpyVDPCRamPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return 0;

    if (IsNeedBreak(kMemory_PAL, _uiAddr, BREAK_FLAG_WRITE) || (_uiSize == 2 && IsNeedBreak(kMemory_PAL, _uiAddr + 1, BREAK_FLAG_WRITE)))
    {
        gbNeedBreak[0] = true;
        guiStepCount[0] = -1;

        if (system_hw == SYSTEM_MCD)
            gExecutionBreak(0x1);
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            gExecutionBreak(0x1);
        else
            gExecutionBreak(0x0);
    }

    return 0;
}

int SpyVDPVSRamPreRead(unsigned _uiAddr, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return 0;

    if (IsNeedBreak(kMemory_VSRAM, _uiAddr, BREAK_FLAG_READ) || (_uiSize == 2 && IsNeedBreak(kMemory_VSRAM, _uiAddr + 1, BREAK_FLAG_READ)))
    {
        gbNeedBreak[0] = true;
        guiStepCount[0] = -1;

        if (system_hw == SYSTEM_MCD)
            gExecutionBreak(0x1);
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            gExecutionBreak(0x1);
        else
            gExecutionBreak(0x0);
    }

    return 0;
}

int SpyVDPVSRamWPrerite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return 0;

    if (IsNeedBreak(kMemory_VSRAM, _uiAddr, BREAK_FLAG_WRITE) || (_uiSize == 2 && IsNeedBreak(kMemory_VSRAM, _uiAddr + 1, BREAK_FLAG_WRITE)))
    {
        gbNeedBreak[0] = true;
        guiStepCount[0] = -1;

        if (system_hw == SYSTEM_MCD)
            gExecutionBreak(0x1);
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            gExecutionBreak(0x1);
        else
            gExecutionBreak(0x0);
    }

    return 0;
}

int Spy68kBusPreRead(void* _pCPU, unsigned _uiAddr, unsigned _uiSize)
{
    if (&m68k == _pCPU)
        return SpyM68kBusPreRead(_uiAddr, _uiSize);

    if (&s68k == _pCPU)
        return SpyS68kBusPreRead(_uiAddr, _uiSize);

    return 0;
}

int Spy68kBusPreWrite(void* _pCPU, unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (&m68k == _pCPU)
        return SpyM68kBusPreWrite(_uiAddr, _uiData, _uiSize);
    
    if (&s68k == _pCPU)
        return SpyS68kBusPreWrite(_uiAddr, _uiData, _uiSize);

    return 0;
}

