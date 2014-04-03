#include "Spy.h"
#include "API\APIMarmelade.h"
#include "Memory.h"
#include "Breakpoint.h"

extern "C"
{
    #include "shared.h"
};

int gIsDebugAccess = 0;

extern ExecutionBreak gExecutionBreak;
extern uint8 system_hw;

void SpyM68kBusPreRead(unsigned _uiAddr, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return;

    if (IsNeedBreak(kMemory_M68000, _uiAddr, BREAK_FLAG_READ) || (_uiSize == 2 && IsNeedBreak(kMemory_M68000, _uiAddr + 1, BREAK_FLAG_READ)))
    {
        gExecutionBreak(0x1);
    }
}

void SpyM68kBusPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return;

    if (IsNeedBreak(kMemory_M68000, _uiAddr, BREAK_FLAG_WRITE) || (_uiSize == 2 && IsNeedBreak(kMemory_M68000, _uiAddr + 1, BREAK_FLAG_WRITE)))
    {
        gExecutionBreak(0x1);
    }
}

void SpyS68kBusPreRead(unsigned _uiAddr, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return;

    if (IsNeedBreak(kMemory_S68000, _uiAddr, BREAK_FLAG_READ) || (_uiSize == 2 && IsNeedBreak(kMemory_S68000, _uiAddr + 1, BREAK_FLAG_READ)))
    {
        gExecutionBreak(0x2);
    }
}

void SpyS68kBusPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return;

    if (IsNeedBreak(kMemory_S68000, _uiAddr, BREAK_FLAG_WRITE) || (_uiSize == 2 && IsNeedBreak(kMemory_S68000, _uiAddr + 1, BREAK_FLAG_WRITE)))
    {
        gExecutionBreak(0x2);
    }
}

void SpyZ80kBusPreRead(unsigned _uiAddr, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return;

    if (IsNeedBreak(kMemory_Z80, _uiAddr, BREAK_FLAG_READ) || (_uiSize == 2 && IsNeedBreak(kMemory_Z80, _uiAddr + 1, BREAK_FLAG_READ)))
    {
        gExecutionBreak(0x0);
    }
}

void SpyZ80BusPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return;

    if (IsNeedBreak(kMemory_Z80, _uiAddr, BREAK_FLAG_WRITE) || (_uiSize == 2 && IsNeedBreak(kMemory_Z80, _uiAddr + 1, BREAK_FLAG_WRITE)))
    {
        gExecutionBreak(0x0);
    }
}

void SpyVDPVRamPreRead(unsigned _uiAddr, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return;

    if (IsNeedBreak(kMemory_VRAM, _uiAddr, BREAK_FLAG_READ) || (_uiSize == 2 && IsNeedBreak(kMemory_VRAM, _uiAddr + 1, BREAK_FLAG_READ)))
    {
        if (system_hw == SYSTEM_MCD)
            gExecutionBreak(0x1);
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            gExecutionBreak(0x1);
        else
            gExecutionBreak(0x0);
    }
}

void SpyVDPVRamPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return;

    if (IsNeedBreak(kMemory_VRAM, _uiAddr, BREAK_FLAG_WRITE) || (_uiSize == 2 && IsNeedBreak(kMemory_VRAM, _uiAddr + 1, BREAK_FLAG_WRITE)))
    {
        if (system_hw == SYSTEM_MCD)
            gExecutionBreak(0x1);
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            gExecutionBreak(0x1);
        else
            gExecutionBreak(0x0);
    }
}

void SpyVDPCRamPreRead(unsigned _uiAddr, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return;

    if (IsNeedBreak(kMemory_PAL, _uiAddr, BREAK_FLAG_READ) || (_uiSize == 2 && IsNeedBreak(kMemory_PAL, _uiAddr + 1, BREAK_FLAG_READ)))
    {
        if (system_hw == SYSTEM_MCD)
            gExecutionBreak(0x1);
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            gExecutionBreak(0x1);
        else
            gExecutionBreak(0x0);
    }
}

void SpyVDPCRamPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return;

    if (IsNeedBreak(kMemory_PAL, _uiAddr, BREAK_FLAG_WRITE) || (_uiSize == 2 && IsNeedBreak(kMemory_PAL, _uiAddr + 1, BREAK_FLAG_WRITE)))
    {
        if (system_hw == SYSTEM_MCD)
            gExecutionBreak(0x1);
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            gExecutionBreak(0x1);
        else
            gExecutionBreak(0x0);
    }
}

void SpyVDPVSRamPreRead(unsigned _uiAddr, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return;

    if (IsNeedBreak(kMemory_VSRAM, _uiAddr, BREAK_FLAG_READ) || (_uiSize == 2 && IsNeedBreak(kMemory_VSRAM, _uiAddr + 1, BREAK_FLAG_READ)))
    {
        if (system_hw == SYSTEM_MCD)
            gExecutionBreak(0x1);
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            gExecutionBreak(0x1);
        else
            gExecutionBreak(0x0);
    }
}

void SpyVDPVSRamWPrerite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{
    if (gIsDebugAccess)
        return;

    if (IsNeedBreak(kMemory_VSRAM, _uiAddr, BREAK_FLAG_WRITE) || (_uiSize == 2 && IsNeedBreak(kMemory_VSRAM, _uiAddr + 1, BREAK_FLAG_WRITE)))
    {
        if (system_hw == SYSTEM_MCD)
            gExecutionBreak(0x1);
        else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
            gExecutionBreak(0x1);
        else
            gExecutionBreak(0x0);
    }
}
