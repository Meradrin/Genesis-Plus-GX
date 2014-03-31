#ifndef MemoryMap_h
#define MemoryMap_h

#include "API\APIMarmelade.h"

MemoryMapHandle GetZ80MemMap();
MemoryMapHandle GetM68000MemMap();
MemoryMapHandle GetS68000MemMap();

void UpdateZ80Map();
void UpdateM68000Map();
void UpdateS68000Map();

const char* GetMapName(MemoryMapHandle _Map, uint32 _uiPosition);

void ClearMap(MemoryMapHandle _Map);
bool SaveMap(MemoryMapHandle _Map, const char* _szMapFile);
bool LoadMap(MemoryMapHandle _Map, const char* _szMapFile, bool _bIgnoreSrcFile);

uint32 GetMapType(MemoryMapHandle _Map, uint32 _uiPosition);
void SetMapType(MemoryMapHandle _Map, uint32 _uiPosition, uint32 _uiDataType);

uint32 GetFlagCount(MemoryMapHandle _Map);
const char* GetFlagName(MemoryMapHandle _Map, uint32 _uiFlagIndex);

uint32 GetFlags(MemoryMapHandle _Map, uint32 _uiPosition);
void SetFlags(MemoryMapHandle _Map, uint32 _uiPosition, uint32 _uiFlags);

uint32 GetCommentCount(MemoryMapHandle _Map);
void FillCommentPosition(MemoryMapHandle _Map, uint32* _pCommentsPos);

const char* GetComment(MemoryMapHandle _Map, uint32 _uiPosition);
void SetComment(MemoryMapHandle _Map, uint32 _uiPosition, const char* _sComment);

uint32 GetLabelCount(MemoryMapHandle _Map);
void FillLabelPosition(MemoryMapHandle _Map, uint32* _pLabelsPos);

uint32 FindLabel(MemoryMapHandle _Map, const char* _sLabelName);
const char* GetLabel(MemoryMapHandle _Map, uint32 _uiPosition);
void SetLabel(MemoryMapHandle _Map, uint32 _uiPosition, const char* _sLabelName);

#endif