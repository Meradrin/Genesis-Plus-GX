#ifndef DebugMacro_h
#define DebugMacro_h

#ifdef MARMELADE_MODULE

#include "Spy.h"

extern void error(char* _szStr, ...);

#define SET_TEMP_VALUE(value)                             (gTmpValue = (value))
#define GET_TEMP_VALUE(value)                             gTmpValue

#define IS_DEBUG_READ       gIsDebugAcess
#define IS_DEBUG_WRITE      gIsDebugAcess

#define SPY_M68K_PRE_EXEC   SpyM68kPreExec()
#define SPY_S68K_PRE_EXEC   SpyS68kPreExec()
#define SPY_Z80_PRE_EXEC    SpyZ80PreExec()

#define SPY_BUS_68K_PRE_READ(cpu, addr, size)       Spy68kBusPreRead(cpu, addr, size)
#define SPY_BUS_M68K_PRE_READ(addr, size)           SpyM68kBusPreRead(addr, size)
#define SPY_BUS_S68K_PRE_READ(addr, size)           SpyS68kBusPreRead(addr, size)
#define SPY_BUS_Z80_PRE_READ(addr, size)            SpyZ80BusPreRead(addr, size)
#define SPY_VDP_VRAM_PRE_READ(addr, size)           SpyVDPVRamPreRead(addr, size)
#define SPY_VDP_CRAM_PRE_READ(addr, size)           SpyVDPCRamPreRead(addr, size)
#define SPY_VDP_VSRAM_PRE_READ(addr, size)          SpyVDPVSRamPreRead(addr, size)

#define SPY_BUS_68K_PRE_WRITE(cpu, addr, data, size)        Spy68kBusPreWrite(cpu, addr, data, size)
#define SPY_BUS_M68K_PRE_WRITE(addr, data, size)            SpyM68kBusPreWrite(addr, data, size)
#define SPY_BUS_S68K_PRE_WRITE(addr, data, size)            SpyM68kBusPreWrite(addr, data, size)
#define SPY_BUS_Z80_PRE_WRITE(addr, data, size)             SpyZ80BusPreWrite(addr, data, size)
#define SPY_VDP_VRAM_PRE_WRITE(addr, data, size)            SpyVDPVRamPreWrite(addr, data,size)
#define SPY_VDP_CRAM_PRE_WRITE(addr, data, size)            SpyVDPCRamPreWrite(addr, data, size)
#define SPY_VDP_VSRAM_PRE_WRITE(addr, data, size)           SpyVDPVSRamWPrerite(addr, data, size)

#else

#define SET_TEMP_VALUE(value)                             0
#define GET_TEMP_VALUE(value)                             (value)

#define IS_DEBUG_READ
#define IS_DEBUG_WRITE

#define SPY_M68K_PRE_EXEC
#define SPY_S68K_PRE_EXEC
#define SPY_Z80_PRE_EXEC

#define SPY_BUS_68K_PRE_READ(cpu, addr, size)             0
#define SPY_BUS_M68K_PRE_READ(addr, size)                 0
#define SPY_BUS_S68K_PRE_READ(addr, size)                 0
#define SPY_BUS_Z80_PRE_READ(addr, size)                  (addr)
#define SPY_VDP_VRAM_PRE_READ(addr, size)                 0
#define SPY_VDP_CRAM_PRE_READ(addr, size)                 0
#define SPY_VDP_VSRAM_PRE_READ(addr, size)                0
                                                          
#define SPY_BUS_68K_PRE_WRITE(cpu, addr, data, size)      0
#define SPY_BUS_M68K_PRE_WRITE(addr, data, size)          0
#define SPY_BUS_S68K_PRE_WRITE(addr, data, size)          0
#define SPY_BUS_Z80_PRE_WRITE(addr, data, size)           (addr)
#define SPY_VDP_VRAM_PRE_WRITE(addr, data, size)          0
#define SPY_VDP_CRAM_PRE_WRITE(addr, data, size)          0
#define SPY_VDP_VSRAM_PRE_WRITE(addr, data, size)         0

#endif

#endif /* debug_macro_h */