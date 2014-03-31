#ifndef DebugCPU_h
#define DebugCPU_h

#include "API\APIMarmelade.h"

enum CPUTypes_e
{
    kCPU_Z80,
    kCPU_M68000,
    kCPU_S68000,
    kCPU_Count,
};

extern bool gbNeedBreak[kCPU_Count];

void SetExecutionBreak(ExecutionBreak _ExecBreakFnc);

uint32     GetCPUCount(EmulatorHandle _hData);
CPUHandle  GetCPU(EmulatorHandle _hData, uint32 _uiCPUIndex);

const char* GetCPUName(CPUHandle _CPU);

MemoryHandle      GetBusMem(CPUHandle _CPU);
RegistersHandle   GetRegisters(CPUHandle _CPU);
DiassemblerHandle GetDiasm(CPUHandle _CPU);
CallstackHandle   GetCallstack(CPUHandle _CPU);

unsigned GetValidStep(CPUHandle _CPU);
void StepIn(CPUHandle _CPU);
void StepOut(CPUHandle _CPU);
void StepOver(CPUHandle _CPU); 

int GetRegisterCount(RegistersHandle _Register);
const char* GetRegisterName(RegistersHandle _Register, int _iRegisterIndex);

const char* GetRegisterValue(RegistersHandle  _Register, int _iRegisterIndex);
void SetRegisterValue(RegistersHandle  _Register, int _iIndex, const char* _szValue);

uint32 GetRunningAddr(RegistersHandle  _Register);
uint32 GetStackAddr(RegistersHandle  _Register);

void InitCallstack();
void UpdateCallstack();

CallstackHandle GetARM7Callstack();

uint32 GetCallCount(CallstackHandle _Callstack);
CallData* GetCallInfo(CallstackHandle _Callstack, uint32 _uiIndex);

void PushCall(uint32 _uiRootPos, uint32 _uiCallPos, uint32 _uiStackPos);
void PopCall();

#endif
