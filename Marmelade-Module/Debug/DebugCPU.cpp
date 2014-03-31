#include "DebugCPU.h"
#include "Memory.h"
#include "MemoryMap.h"
#include "Register.h"
#include "Disasm.h"
#include "Breakpoint.h"

extern "C"
{
#include "shared.h"
}

ExecutionBreak gExecutionBreak = NULL;

void SetExecutionBreak(ExecutionBreak _ExecBreakFnc)
{
    gExecutionBreak = _ExecBreakFnc;
}

const char gCPUTypeNames[kCPU_Count][32] =
{
    "Z80",
    "Main 68000",
    "Sub 68000",
};

uint32 gCPUTypes[kCPU_Count] =
{
    kCPU_Z80,
    kCPU_M68000,
    kCPU_S68000,
};

uint32 gCPUBusMemory[kCPU_Count] =
{
    1,
    2,
    3,
};

RegistersHandle gCPURegister[kCPU_Count] =
{
    GetZ80Register(),
    GetM68000Register(),
    GetS68000Register(),
};

DiassemblerHandle gCPUDisasm[kCPU_Count] =
{
    GetZ80Disasm(),
    GetM68000Disasm(),
    GetS68000Disasm(),
};

uint32 GetCPUCount(EmulatorHandle _hData)
{
    if (system_hw == SYSTEM_MCD)
        return 3;
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
        return 2;

    return 1;
}

CPUHandle GetCPU(EmulatorHandle _hData, uint32 _uiCPUIndex)
{
    if (_uiCPUIndex < kCPU_Count)
        return (CPUHandle)(gCPUTypes[_uiCPUIndex] + 1);

    return NULL;
}

const char* GetCPUName(CPUHandle _CPU)
{
    static const char szEmpty[] = "";

    if (_CPU != NULL)
    {
        uint32 uiCPUIndex = (uint32)_CPU - 1;

        if (uiCPUIndex < kCPU_Count)
            return gCPUTypeNames[uiCPUIndex];
    }

    return szEmpty;
}

MemoryHandle GetBusMem(CPUHandle _CPU)
{
    if (_CPU == NULL)
        return NULL;

    if (system_hw == SYSTEM_MCD)
    {
        switch ((uint32)_CPU)
        {
        case kCPU_Z80:
            return GetMemory(NULL, 1);
        case kCPU_M68000:
            return GetMemory(NULL, 0);
        case kCPU_S68000:
            return GetMemory(NULL, 2);

        }
    }
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
    {
        switch ((uint32)_CPU)
        {
        case kCPU_Z80:
            return GetMemory(NULL, 1);
        case kCPU_M68000:
            return GetMemory(NULL, 0);
        }
    }

    return GetMemory(NULL, 0);
}

RegistersHandle GetRegisters(CPUHandle _CPU)
{
    if (_CPU != NULL)
        return (RegistersHandle)gCPURegister[(uint32)_CPU - 1];

    return NULL;
}

DiassemblerHandle GetDiasm(CPUHandle _CPU)
{
    if (_CPU != NULL)
        return (DiassemblerHandle)gCPUDisasm[(uint32)_CPU - 1];

    return NULL;
}

CallstackHandle GetCallstack(CPUHandle _CPU)
{
//     if (_CPU != NULL)
//         return Get68000Callstack();

    return NULL;
}

unsigned GetValidStep(CPUHandle _CPU)
{
    return VALID_STEP_IN;
}

bool gbNeedBreak[kCPU_Count] = {false, false, false};
int32 guiStepCount[kCPU_Count] = {-1, -1, -1};

void StepIn(CPUHandle _CPU)
{
    if (_CPU == NULL)
        return;

    gbNeedBreak[(uint32)_CPU - 1] = false;
    guiStepCount[(uint32)_CPU - 1] = 1;
}

void StepOut(CPUHandle _CPU)
{
    StepIn(_CPU);
}

void StepOver(CPUHandle _CPU)
{
    StepIn(_CPU);
}

extern "C" void SpyM68kPreExec()
{

}

extern "C" void SpyS68kPreExec()
{

}

extern "C" void SpyZ80PreExec()
{
    if (guiStepCount[0] > 0)
        --guiStepCount[0];

    if (guiStepCount[0] == 0)
    {
        gbNeedBreak[0] = true;
        guiStepCount[0] = -1;
    }

    uint32 uiPC = GetRunningAddr(GetZ80Register());

    if (IsNeedBreak(kMemory_Z80, uiPC, BREAK_FLAG_EXEC))
    {
        gbNeedBreak[0] = true;
        guiStepCount[0] = -1;
    }

    UpdateZ80Map();

    if (gbNeedBreak[0] && gExecutionBreak != NULL)
    {
        //UpdateCallstack();
        gExecutionBreak(0x0);
    }
}