#include "Breakpoint.h"
#include "Memory.h"
#include "Spy.h"

BreakData gBreakInfo[kMemory_Count];

BreakpointMgrHandle GetBreakHandle()
{
    return (BreakpointMgrHandle)&gBreakInfo;
}

void UpdateBreakMap()
{
    for (uint32 uiMemIndex = 0; uiMemIndex < kMemory_Count; ++uiMemIndex)
    {
        gBreakInfo[uiMemIndex].mMap.clear();
        gBreakInfo[uiMemIndex].mMap.resize(GetSize((MemoryMapHandle)(uiMemIndex + 1)));

        uint32 uiDefCount = gBreakInfo[uiMemIndex].mDef.size();

        for (uint32 i = 0; i < uiDefCount; ++i)
        {
            if (!gBreakInfo[uiMemIndex].mDef[i].Enabled || gBreakInfo[uiMemIndex].mDef[i].Zone == 0)
                continue;

            uint32 uiBreakFlag = (gBreakInfo[uiMemIndex].mDef[i].Exec ? BREAK_FLAG_EXEC : 0) | (gBreakInfo[uiMemIndex].mDef[i].Read ? BREAK_FLAG_READ : 0) | (gBreakInfo[uiMemIndex].mDef[i].Write ? BREAK_FLAG_WRITE : 0);

            for (uint32 j = gBreakInfo[uiMemIndex].mDef[i].Start; j <= gBreakInfo[uiMemIndex].mDef[i].End; ++j)
                gBreakInfo[uiMemIndex].mMap[j] |= uiBreakFlag;
        }
    }
}

void SetBreakpoints(BreakpointMgrHandle _BreakMgr, Breakpoint* _pBreak, uint32 _uiBreakCount)
{
    for (uint32 uiMemIndex = 0; uiMemIndex < kMemory_Count; ++uiMemIndex)
    {
        gBreakInfo[uiMemIndex].mDef.reserve(_uiBreakCount);
        gBreakInfo[uiMemIndex].mDef.clear();
    }

    for (uint32 i = 0; i < _uiBreakCount; ++i)
    {
        if (_pBreak[i].Zone == 0)
            continue;
        
        uint32 uiMemIndex = (uint32)_pBreak[i].Zone - 1;
        gBreakInfo[uiMemIndex].mDef.push_back(_pBreak[i]);
    }

    UpdateBreakMap();
}


unsigned IsNeedBreak(unsigned _uiMemType, unsigned _uiPos, unsigned _uiBreakMask)
{
    return (gBreakInfo[_uiMemType].mMap[_uiPos & 0xffffff] & _uiBreakMask) != 0;
}

void InitBreakpoint()
{
    for (uint32 uiMemIndex = 0; uiMemIndex < kMemory_Count; ++uiMemIndex)
        gBreakInfo[uiMemIndex].mMap.resize(GetSize((MemoryMapHandle)(uiMemIndex + 1)));
}
