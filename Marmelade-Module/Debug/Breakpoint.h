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

#define BREAK_FLAG_EXEC  1
#define BREAK_FLAG_READ  2
#define BREAK_FLAG_WRITE 4

extern BreakData gBreakInfo[kMemory_Count];



BreakpointMgrHandle GetBreakHandle();

void InitBreakpoint();

void SetBreakpoints(BreakpointMgrHandle _BreakMgr, Breakpoint* _pBreak, uint32 _uiBreakCount);
unsigned IsNeedBreak(unsigned _uiMemType, unsigned _uiPos, unsigned _uiBreakMask);

#endif