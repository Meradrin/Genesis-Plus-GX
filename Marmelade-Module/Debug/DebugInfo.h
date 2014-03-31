#ifndef DebugGBAIORegisters_h
#define DebugGBAIORegisters_h

#include "API\APIMarmelade.h"

uint32 GetGroupPropertiesCount(EmulatorHandle _hData);
GroupPropertiesHandle GetGroupProperties(EmulatorHandle _hData, uint32 _uiIndex);

const char* GetTitles(GroupPropertiesHandle _Group);

bool IsNeedRefresh(GroupPropertiesHandle _Group);
TextProperty* GetProperties(GroupPropertiesHandle _Group);

#endif
