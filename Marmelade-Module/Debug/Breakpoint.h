#ifndef Breakpoint_h
#define Breakpoint_h

#include "API\APIMarmelade.h"
#include "Debug\Memory.h"

#include <vector>

struct BreakData
{
    std::vector<Breakpoint> mDef;
    std::vector<uint8> mMap;
};

extern BreakData gBreakInfo[kMemory_Count];

BreakpointMgrHandle GetBreakHandle();

void SetBreakpoints(BreakpointMgrHandle _BreakMgr, Breakpoint* _pBreak, uint32 _uiBreakCount);

#endif