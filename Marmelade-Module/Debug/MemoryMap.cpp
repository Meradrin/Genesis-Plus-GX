#include "MemoryMap.h"

#include <map>
#include <string>
#include <vector>

enum MemMaps_e
{
    kMemMap_Z80 = 1,
    kMemMap_M68000,
    kMemMap_S68000,
};


#define MASTERSYSTEM        0x100;

struct MapData
{
    std::vector<uint32> mMap;
    std::map<uint32, std::string> mComments;
    std::map<uint32, std::string> mLabel;
};

MapData gMapZ80;
MapData gMapM68000;
MapData gMapS68000;

MemoryMapHandle GetZ80MemMap()
{
    return (MemoryMapHandle)&gMapZ80;
}

MemoryMapHandle GetM68000MemMap()
{
    return (MemoryMapHandle)&gMapM68000;
}

MemoryMapHandle GetS68000MemMap()
{
    return (MemoryMapHandle)&gMapS68000;
}

const char* GetMapName(MemoryMapHandle _Map, uint32 _uiPosition)
{
    return "";
}

void ClearMap(MemoryMapHandle _Map)
{

}

bool SaveMap(MemoryMapHandle _Map, const char* _szMapFile)
{
    return false;
}

bool LoadMap(MemoryMapHandle _Map, const char* _szMapFile, bool _bIgnoreSrcFile)
{
    return false;
}

uint32 GetMapType(MemoryMapHandle _Map, uint32 _uiPosition)
{
    return 0;
}

void SetMapType(MemoryMapHandle _Map, uint32 _uiPosition, uint32 _uiDataType)
{

}

uint32 GetFlagCount(MemoryMapHandle _Map)
{
    return 0;
}

const char* GetFlagName(MemoryMapHandle _Map, uint32 _uiFlagIndex)
{
    return "";
}

uint32 GetFlags(MemoryMapHandle _Map, uint32 _uiPosition)
{
    return 0;
}

void SetFlags(MemoryMapHandle _Map, uint32 _uiPosition, uint32 _uiFlags)
{

}

uint32 GetCommentCount(MemoryMapHandle _Map)
{
    return 0;
}

void FillCommentPosition(MemoryMapHandle _Map, uint32* _pCommentsPos)
{

}

const char* GetComment(MemoryMapHandle _Map, uint32 _uiPosition)
{
    return "";
}

void SetComment(MemoryMapHandle _Map, uint32 _uiPosition, const char* _sComment)
{

}

uint32 GetLabelCount(MemoryMapHandle _Map)
{
    return 0;
}

void FillLabelPosition(MemoryMapHandle _Map, uint32* _pLabelsPos)
{

}

uint32 FindLabel(MemoryMapHandle _Map, const char* _sLabelName)
{
    return 0;
}

const char* GetLabel(MemoryMapHandle _Map, uint32 _uiPosition)
{
    return "";
}

void SetLabel(MemoryMapHandle _Map, uint32 _uiPosition, const char* _sLabelName)
{

}

void UpdateZ80Map()
{

}

void UpdateM68000Map()
{

}

void UpdateS68000Map()
{

}
