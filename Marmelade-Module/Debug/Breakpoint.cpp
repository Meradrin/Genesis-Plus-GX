#include "Breakpoint.h"

int gIsDebugAccess = 0;

void SpyM68kPreExec()
{

}

void SpyS68kPreExec()
{

}

void SpyZ80PreExec()
{

}

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
