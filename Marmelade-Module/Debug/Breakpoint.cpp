#include "Breakpoint.h"
#include "API\APIMarmelade.h"

int gIsDebugAccess = 0;

void SpyM68kBusPreRead(unsigned _uiAddr, unsigned _uiSize)
{

}

void SpyM68kBusPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{

}

void SpyS68kBusPreRead(unsigned _uiAddr, unsigned _uiSize)
{

}

void SpyS68kBusPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{

}

void SpyZ80kBusPreRead(unsigned _uiAddr, unsigned _uiSize)
{

}

void SpyZ80BusPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{

}

void SpyVDPVRamPreRead(unsigned _uiAddr, unsigned _uiSize)
{

}

void SpyVDPVRamPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{

}

void SpyVDPCRamPreRead(unsigned _uiAddr, unsigned _uiSize)
{

}

void SpyVDPCRamPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{

}

void SpyVDPVSRamPreRead(unsigned _uiAddr, unsigned _uiSize)
{

}

void SpyVDPVSRamWPrerite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize)
{

}

void SetBreakpoint(uint32 _uiMemType, uint32 _uiPos, uint32 _uiBreakValue)
{

}

unsigned GetBreakpoint(unsigned _uiMemType, unsigned _uiPos)
{
    return 0;
}

unsigned IsNeedBreak(unsigned _uiMemType, unsigned _uiPos, unsigned _uiBreakMask)
{
    return false;
}
