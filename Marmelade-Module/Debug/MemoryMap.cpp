#include "MemoryMap.h"
#include "Memory.h"
#include "Register.h"
#include "Disasm.h"

extern "C"
{
    #include "shared.h"

    extern m68ki_cpu_core m68k;
    extern m68ki_cpu_core s68k;
};

#include <map>
#include <string>
#include <vector>
#include <stdio.h>

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
    if (system_hw == SYSTEM_MCD)
    {
        if (_Map == GetZ80MemMap())
        {

        }
        else if (_Map == GetM68000MemMap())
        {

        }
        else if (_Map == GetS68000MemMap())
        {

        }
    }
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
    {
        if (_Map == GetZ80MemMap())
        {
            if (_uiPosition < 0x2000)
                return "RAM";

            if (_uiPosition < 0x4000)
                return "RAM-M";

            if (_uiPosition < 0x6000)
                return "YM2612";

            if (_uiPosition < 0x6100)
                return "BankReg";

            if (_uiPosition < 0x7f00)
                return "Unused";

            if (_uiPosition < 0x8000)
                return "VDP";

            static char szBank[10] = "BANK";

            sprintf(szBank, "BANK_%X", zbank >> 15);

            return szBank;
        }
        else if (_Map == GetM68000MemMap())
        {
            if (_uiPosition < 0x400000)
                return "ROM";

            if (_uiPosition < 0x800000)
                return "Unused(1)";

            if (_uiPosition < 0xA00000)
                return "Unused(2)";

            if (_uiPosition < 0xA10000)
                return "Z80";

            if (_uiPosition < 0xA10020)
                return "I/O";

            if (_uiPosition < 0xC00000)
                return "Exp";

            if (_uiPosition < 0xE00000)
                return "VDP";

            return "RAM";
        }
    }
    
    if (_Map == GetZ80MemMap())
    {

    }

    return "???";
}

void SetFixString(MemoryMapHandle _Map, uint32 _uiPosition, uint32 _uiSize)
{
    SetMapType(_Map, _uiPosition, MAP_DATA_ASCII);

    for (uint32 i = 1; i < _uiSize; ++i)
        SetMapType(_Map, _uiPosition + i, MAP_SUBCODE);
}

void SetSmartData(MemoryMapHandle _Map, uint32 _uiPosition, uint32 _uiDataSize, uint32 _uiDataCount)
{
    for (uint32 i = 0; i < _uiDataCount; ++i)
    {
        uint32 uiPos = i * _uiDataSize + _uiPosition;

        switch (_uiDataSize)
        {
        case 1:
            SetMapType(_Map, uiPos, MAP_DATA_8);
            break;
        case 2:
            SetMapType(_Map, uiPos, MAP_DATA_16);
            SetMapType(_Map, uiPos + 1, MAP_SUBCODE);
            break;
        case 3:
            SetMapType(_Map, uiPos, MAP_DATA_24);
            SetMapType(_Map, uiPos + 1, MAP_SUBCODE);
            SetMapType(_Map, uiPos + 2, MAP_SUBCODE);
            break;
        case 4:
            SetMapType(_Map, uiPos, MAP_DATA_32);
            SetMapType(_Map, uiPos + 1, MAP_SUBCODE);
            SetMapType(_Map, uiPos + 2, MAP_SUBCODE);
            SetMapType(_Map, uiPos + 3, MAP_SUBCODE);
            break;
        }
    }
}

void ClearMap(MemoryMapHandle _Map)
{
    if (_Map == NULL)
        return;

    MapData* mapData = (MapData*)_Map;

    if (mapData == &gMapZ80)
    {
        mapData->mMap.clear();
        mapData->mMap.resize(0x10000);
        mapData->mComments.clear();
        mapData->mLabel.clear();
    }

    if (mapData == &gMapM68000)
    {
        mapData->mMap.clear();
        mapData->mMap.resize(0x1000000);
        mapData->mComments.clear();
        mapData->mLabel.clear();
    }

    if (mapData == &gMapS68000)
    {
        mapData->mMap.clear();
        mapData->mMap.resize(0x1000000);
        mapData->mComments.clear();
        mapData->mLabel.clear();
    }
}

bool SaveMap(MemoryMapHandle _Map, const char* _szMapFile)
{
    if (_Map == NULL)
        return false;

    MapData* pMapData = (MapData*)_Map;

    FILE* pMapFile = fopen(_szMapFile, "wb");

    if (pMapFile != NULL)
    {
        static uint8 uiHeader[] = {'G','E','N','P','L','U','S','G','X','1'};

        fwrite(uiHeader, sizeof(uiHeader), 1, pMapFile);

        uint32 uiSize = pMapData->mMap.size();

        fwrite(&uiSize, sizeof(uint32), 1, pMapFile);
    
        if (uiSize)
            fwrite(&pMapData->mMap[0], pMapData->mMap.size(), 1, pMapFile);

        uint32 uiCommentCount = pMapData->mComments.size();
        fwrite(&uiCommentCount, sizeof(uiCommentCount), 1, pMapFile);

        for (std::map<uint32, std::string>::const_iterator i = pMapData->mComments.begin(); i != pMapData->mComments.end(); ++i)
        {
            fwrite(&(i->first), sizeof(uint32), 1, pMapFile);
            uint32 uiStrSize = i->second.size();
            fwrite(&uiStrSize, sizeof(uint32), 1, pMapFile);
            fwrite(i->second.c_str(), uiStrSize, 1, pMapFile);
        }

        uint32 uiLabelCount = pMapData->mLabel.size();
        fwrite(&uiLabelCount, sizeof(uiLabelCount), 1, pMapFile);

        for (std::map<uint32, std::string>::const_iterator i = pMapData->mLabel.begin(); i != pMapData->mLabel.end(); ++i)
        {
            fwrite(&(i->first), sizeof(uint32), 1, pMapFile);
            uint32 uiStrSize = i->second.size();
            fwrite(&uiStrSize, sizeof(uint32), 1, pMapFile);
            fwrite(i->second.c_str(), uiStrSize, 1, pMapFile);
        }

        fclose(pMapFile);

        return true;
    }

    return false;
}

bool LoadMap(MemoryMapHandle _Map, const char* _szMapFile, bool _bIgnoreSrcFile)
{
    if (_Map == NULL)
        return false;

    MapData* pMapData = (MapData*)_Map;

    FILE* pMapFile = fopen(_szMapFile, "rb");

    if (pMapFile != NULL)
    {
        ClearMap(_Map);

        static uint8 uiHeader[] = {'G','E','N','P','L','U','S','G','X','1'};
        uint8 uiCheckHeader[sizeof(uiHeader)];

        fread(uiCheckHeader, sizeof(uiCheckHeader), 1, pMapFile);

        if (memcmp(uiCheckHeader, uiHeader, sizeof(uiHeader)) != 0)
        {
            fclose(pMapFile);
            return false;
        }

        uint32 uiMapSize;

        fread(&uiMapSize, sizeof(uint32), 1, pMapFile);

        pMapData->mMap.resize(uiMapSize);
        if (uiMapSize)
            fread(&pMapData->mMap[0], pMapData->mMap.size(), 1, pMapFile);

        uint32 uiCommentCount = 0;
        fread(&uiCommentCount, sizeof(uiCommentCount), 1, pMapFile);

        for (uint32 i = 0; i < uiCommentCount; ++i)
        {
            uint32 uiPos = 0;
            fread(&uiPos, sizeof(uint32), 1, pMapFile);
            uint32 uiStrSize = 0;
            fread(&uiStrSize, sizeof(uint32), 1, pMapFile);
            char* szStr = new char[uiStrSize + 1];
            szStr[uiStrSize] = '\0';
            fread(szStr, uiStrSize, 1, pMapFile);

            pMapData->mComments[uiPos] = szStr;
        }

        uint32 uiLabelCount = 0;
        fread(&uiLabelCount, sizeof(uiLabelCount), 1, pMapFile);

        for (uint32 i = 0; i < uiLabelCount; ++i)
        {
            uint32 uiPos = 0;
            fread(&uiPos, sizeof(uint32), 1, pMapFile);
            uint32 uiStrSize = 0;
            fread(&uiStrSize, sizeof(uint32), 1, pMapFile);
            char* szStr = new char[uiStrSize + 1];
            szStr[uiStrSize] = '\0';
            fread(szStr, uiStrSize, 1, pMapFile);
            pMapData->mLabel[uiPos] = szStr;
        }

        fclose(pMapFile);
        return true;
    }

    return false;
}

uint32 GetMapType(MemoryMapHandle _Map, uint32 _uiPosition)
{
    if (_Map == NULL)
        return MAP_UNKNOW;

    MapData* pMapData = (MapData*)_Map;

    if (_uiPosition >= pMapData->mMap.size())
        return MAP_UNKNOW;

    return pMapData->mMap[_uiPosition] & 0x0000ffff;
}

void CheckLabel68000(MemoryHandle _Mem, uint32 _uiPosition)
{
    _uiPosition &= 0xffffff;

    m68ki_cpu_core* pCpu = &(_Mem == GetM68000MemMap() ? m68k : s68k);

    uint32 uiOpcode = (GetByte(_Mem, _uiPosition) << 8) | GetByte(_Mem, _uiPosition + 1);
    uint32 uiData1 = (GetByte(_Mem, _uiPosition + 2) << 8) | GetByte(_Mem, _uiPosition + 3);
    uint32 uiData2 = (GetByte(_Mem, _uiPosition + 4) << 8) | GetByte(_Mem, _uiPosition + 5);

    uint32 uiLabelPos = 0x1000000;
    char szLabelName[80] = "";

    if (uiOpcode >= 0x4e90 && uiOpcode <= 0x4e97) // JSR (An)
    {
        uiLabelPos = pCpu->dar[8 + (uiOpcode & 0x7)];
        sprintf(szLabelName, "Sbr_%06x", uiLabelPos);
    }
    else if (uiOpcode >= 0x4ea8  && uiOpcode <= 0x4eaf) // JSR d16(An)
    {
        uiLabelPos = pCpu->dar[8 + (uiOpcode & 0x7)] + uiData1;
        sprintf(szLabelName, "Sbr_%06x", uiLabelPos);
    }
    else
    {
        switch (uiOpcode)
        {
        case 0x4ef8: // JMP.W
            uiLabelPos = ((uiData1 & 0xffff0000) | uiData1);
            sprintf(szLabelName, "Lbl_%06x", uiLabelPos);
            break;
        case 0x4ef9: // JMP.L
            uiLabelPos = (uiData1 << 16 | uiData2);
            sprintf(szLabelName, "Lbl_%06x", uiLabelPos);
            break;
        case 0x4eb8: // JSR.W
            uiLabelPos = ((_uiPosition & 0xffff0000) | uiData1);
            sprintf(szLabelName, "Sbr_%06x", uiLabelPos);
            break;	
        case 0x4eb9: // JSR.L
            uiLabelPos = (uiData1 << 16 | uiData2);
            sprintf(szLabelName, "Sbr_%06x", uiLabelPos);
            break;
        case 0x4eba: // JSR $0000(PC)
            uiLabelPos = _uiPosition + 2 + (int16)uiData1;
            sprintf(szLabelName, "Sbr_%06x", uiLabelPos);
            break;
        case 0x4ebb:
            uiLabelPos = _uiPosition + 2 + (int16)uiData1 + (int16)pCpu->dar[0];
            sprintf(szLabelName, "Sbr_%06x", uiLabelPos);
            break;
        case 0x4efa: // JMP $0000(PC)
            uiLabelPos = _uiPosition + 2 + (int16)uiData1;
            sprintf(szLabelName, "Lbl_%06x", uiLabelPos);
            break;
        }

        if ((uiOpcode & 0xf000) == 0x6000)
        {
            if (uiOpcode & 0xff) // BSR.B and Bcc.B
            {
                uiLabelPos = (uint32)((int32)_uiPosition + 2 + ((int8)(uiOpcode & 0xff)));

                if ((uiOpcode & 0xF00) == 0x100)
                    sprintf(szLabelName, "Sbr_%06x", uiLabelPos);
                else
                    sprintf(szLabelName, "Lbl_%06x", uiLabelPos);
            }
            else // BSR.W and Bcc.W
            {
                uiLabelPos = (uint32)((int32)_uiPosition + 2 + ((int16)uiData1));

                if ((uiOpcode & 0xf00) == 0x100)
                    sprintf(szLabelName, "Sbr_%06x", uiLabelPos);
                else
                    sprintf(szLabelName, "Lbl_%06x", uiLabelPos);
            }
        }
        else if ((uiOpcode & 0xf0f8) == 0x50C8) // DBcc
        {
            uiLabelPos = (uint32)((int32)_uiPosition + 2 + ((int16)uiData1));
            sprintf(szLabelName, "Lbl_%06x", uiLabelPos);
        }
    }

    if (szLabelName[0] != '\0')
        SetLabel(GetMap(_Mem), uiLabelPos, szLabelName);
}

void SetMapType(MemoryMapHandle _Map, uint32 _uiPosition, uint32 _uiDataType)
{
    if (_Map == NULL)
        return;

    MapData* pMapData = (MapData*)_Map;

    if (_uiPosition >= pMapData->mMap.size())
        return;

    uint32 uiMapData = pMapData->mMap[_uiPosition];

    if (_uiPosition < 0x800000 && (uiMapData & 0xffff) == (_uiDataType & 0xffff))
        return;

    pMapData->mMap[_uiPosition] = (_uiDataType & 0xffff) | (uiMapData & 0xffff0000);

    if (_uiDataType == MAP_CODE)
    {
        uint32 uiOpcodeSize;

        if (pMapData == &gMapZ80)
        {
            uiOpcodeSize = GetOpcodeSize((DiassemblerHandle)1, 0, _uiPosition, 0);
        }
        else if (pMapData == &gMapM68000)
        {
            CheckLabel68000((MemoryHandle)(kMemory_M68000 + 1), _uiPosition);
            uiOpcodeSize = GetOpcodeSize((DiassemblerHandle)2, 0, _uiPosition, 0);
        }
        else if (pMapData == &gMapS68000)
        {
            CheckLabel68000((MemoryHandle)(kMemory_S68000 + 1), _uiPosition);
            uiOpcodeSize = GetOpcodeSize((DiassemblerHandle)3, 0, _uiPosition, 0);
        }

        if (uiOpcodeSize > 0)
        {
            --uiOpcodeSize;
            
            for (uint32 i = 0; i < uiOpcodeSize; ++i)
                SetMapType(_Map, ++_uiPosition, MAP_SUBCODE);
        }
    }
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
    if (_Map == NULL)
        return 0;

    return ((MapData*)(_Map))->mComments.size();
}

void FillCommentPosition(MemoryMapHandle _Map, uint32* _pCommentsPos)
{
    if (_Map == NULL)
        return;

    uint32 uiAddIndex = 0;

    MapData* pMapData = (MapData*)_Map;

    for (std::map<uint32, std::string>::const_iterator i = pMapData->mComments.begin(); i != pMapData->mComments.end(); ++i)
        _pCommentsPos[uiAddIndex++] = i->first;
}

const char* GetComment(MemoryMapHandle _Map, uint32 _uiPosition)
{
    if (_Map == NULL)
        return NULL;

    MapData* pMapData = (MapData*)_Map;

    std::map<uint32, std::string>::iterator itr = pMapData->mComments.find(_uiPosition);

    if (itr != pMapData->mComments.end())
        return itr->second.c_str();

    return NULL;
}

void SetComment(MemoryMapHandle _Map, uint32 _uiPosition, const char* _sComment)
{
    if (_Map == NULL)
        return;

    MapData* pMapData = (MapData*)_Map;

    if (_sComment != NULL && _sComment[0] != '\0')
    {
        pMapData->mComments[_uiPosition] = _sComment;
    }
    else
    {
        std::map<uint32, std::string>::iterator itr = pMapData->mComments.find(_uiPosition);

        if (itr != pMapData->mComments.end())
            pMapData->mComments.erase(itr);
    }
}

uint32 GetLabelCount(MemoryMapHandle _Map)
{
    if (_Map == NULL)
        return 0;

    MapData* pMapData = (MapData*)_Map;

    return pMapData->mLabel.size();
}

void FillLabelPosition(MemoryMapHandle _Map, uint32* _pLabelsPos)
{
    if (_Map == NULL)
        return;

    MapData* pMapData = (MapData*)_Map;

    uint32 uiAddIndex = 0;

    for (std::map<uint32, std::string>::const_iterator i = pMapData->mLabel.begin(); i != pMapData->mLabel.end(); ++i)
        _pLabelsPos[uiAddIndex++] = i->first;
}

uint32 FindLabel(MemoryMapHandle _Map, const char* _sLabelName)
{
    MapData* pMapData = (MapData*)_Map;

    uint32 uiStrLength = strlen(_sLabelName);

    for (std::map<uint32, std::string>::const_iterator i = pMapData->mLabel.begin(); i != pMapData->mLabel.end(); ++i)
    {
        if (_strnicmp(i->second.c_str(), _sLabelName, uiStrLength) == 0)
            return i->first;
    }

    return 0xffffffff;
}

const char* GetLabel(MemoryMapHandle _Map, uint32 _uiPosition)
{
    if (_Map == NULL)
        return NULL;

    MapData* pMapData = (MapData*)_Map;

    std::map<uint32, std::string>::iterator itr = pMapData->mLabel.find(_uiPosition);

    if (itr != pMapData->mLabel.end())
        return itr->second.c_str();

    return NULL;
}

void SetLabel(MemoryMapHandle _Map, uint32 _uiPosition, const char* _sLabelName)
{
    if (_Map == NULL)
        return;

    MapData* pMapData = (MapData*)_Map;

    if (_sLabelName != NULL && _sLabelName[0] != '\0')
    {
        pMapData->mLabel[_uiPosition] = _sLabelName;
    }
    else
    {
        std::map<uint32, std::string>::iterator itr = pMapData->mLabel.find(_uiPosition);

        if (itr != pMapData->mLabel.end())
            pMapData->mLabel.erase(itr);
    }
}

void UpdateZ80Map()
{
    SetMapType(GetZ80MemMap(), GetRunningAddr(GetZ80Register()), MAP_CODE);
}

void UpdateM68000Map()
{
    SetMapType(GetM68000MemMap(), GetRunningAddr(GetM68000Register()), MAP_CODE);
}

void UpdateS68000Map()
{
    SetMapType(GetS68000MemMap(), GetRunningAddr(GetS68000Register()), MAP_CODE);
}
