#ifndef Spy_h
#define Spy_h

#ifdef __cplusplus
extern "C"
{
#endif

void SpyM68kPreExec();
void SpyS68kPreExec();
void SpyZ80PreExec();

int Spy68kBusPreRead(void* _pCPU, unsigned _uiAddr, unsigned _uiSize);
int Spy68kBusPreWrite(void* _pCPU, unsigned _uiAddr, unsigned _uiData, unsigned _uiSize);
int SpyM68kBusPreRead(unsigned _uiAddr, unsigned _uiSize);
int SpyM68kBusPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize);
int SpyS68kBusPreRead(unsigned _uiAddr, unsigned _uiSize);
int SpyM68kBusPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize);
int SpyZ80BusPreRead(unsigned _uiAddr, unsigned _uiSize);
int SpyZ80BusPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize);
int SpyVDPVRamPreRead(unsigned _uiAddr, unsigned _uiSize);
int SpyVDPVRamPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize);
int SpyVDPCRamPreRead(unsigned _uiAddr, unsigned _uiSize);
int SpyVDPCRamPreWrite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize);
int SpyVDPVSRamPreRead(unsigned _uiAddr, unsigned _uiSize);
int SpyVDPVSRamWPrerite(unsigned _uiAddr, unsigned _uiData, unsigned _uiSize);

extern int gIsDebugAcess;
extern unsigned gTmpValue;

#ifdef __cplusplus
}
#endif

#endif /* spy_h */