#include "DebugInfo.h"
#include <stdarg.h>
#include <stdio.h>

extern "C"
{
    #include "shared.h"
}

// TODO: Create all group and init all stuff

// Sega Master System
enum GroupTypesSMS_e
{
    kGroupSMS_VDP,
};

const uint32 GroupHandleSMS[] =
{
    kGroupSMS_VDP,
};

const char* GroupTextSMS[] =
{
    "VDP Registers",
};

#define SMS_GROUP_COUNT     (sizeof(GroupHandleSMS) / sizeof(uint32))

// Sega Mega Drive
enum GroupTypesMD_e
{
    kGroupMD_VDP,
    kGroupMD_DMA,
    kGroupMD_Cartridge,
};

const uint32 GroupHandleMD[] =
{
    kGroupMD_VDP,
    kGroupMD_DMA,
    kGroupMD_Cartridge,
};

const char* GroupTextMD[] =
{
    "VDP",
    "DMA",
    "Cartridge",
};

#define MD_GROUP_COUNT     (sizeof(GroupHandleMD) / sizeof(uint32))

// Sega Mega CD
enum GroupTypesMCD_e
{
    kGroupMCD_VDP,
    kGroupMCD_DMA,
    kGroupMCD_Cartridge,
};

const uint32 GroupHandleMCD[] =
{
    kGroupMCD_VDP,
    kGroupMCD_DMA,
    kGroupMCD_Cartridge,
};

const char* GroupTextMCD[] =
{
    "VDP",
    "DMA",
    "Cartridge",
};

#define MCD_GROUP_COUNT     (sizeof(GroupHandleMCD) / sizeof(uint32))

uint32 GetGroupPropertiesCount(EmulatorHandle _hData)
{
    if (system_hw == SYSTEM_MCD)
    {
        return MCD_GROUP_COUNT;
    }
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
    {
        return MD_GROUP_COUNT;
    }

    return SMS_GROUP_COUNT;
}

GroupPropertiesHandle GetGroupProperties(EmulatorHandle _hData, uint32 _uiIndex)
{
    if (system_hw == SYSTEM_MCD)
    {
        if (_uiIndex < GetGroupPropertiesCount(_hData))
            return (GroupPropertiesHandle)&GroupHandleMCD[_uiIndex];
    }
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
    {
        if (_uiIndex < GetGroupPropertiesCount(_hData))
            return (GroupPropertiesHandle)&GroupHandleMD[_uiIndex];
    }

    if (_uiIndex < GetGroupPropertiesCount(_hData))
        return (GroupPropertiesHandle)&GroupHandleSMS[_uiIndex];

    return NULL;
}

const char* GetTitles(GroupPropertiesHandle _Group)
{
    if (_Group == NULL)
        return 0;

    if (system_hw == SYSTEM_MCD)
    {
        return GroupTextMCD[*((uint32*)_Group)];
    }
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
    {
        return GroupTextMD[*((uint32*)_Group)];
    }

    return GroupTextSMS[*((uint32*)_Group)];
}

bool IsNeedRefresh(GroupPropertiesHandle _Group)
{
    return true;
}

TextProperty* pDeptGroup[256] = {0};
uint32 uiDeptGroup = 0;

char* pOutputStr = NULL;
uint32 uiOutputSize = 0;

void ToStr(const char* _szStr, ...)
{
    va_list args;

    va_start(args, _szStr);
    vsnprintf(pOutputStr, uiOutputSize, _szStr, args);
    va_end(args);
}

#define CREATE_ROOT_GROUP()

#define BEGIN_PROPERTY_GROUP(TEXT_NAME, TEXT_VALUE)           \
{ \
    static TextProperty Property; \
    static char szName[512]; \
    static char szValue[512]; \
    \
    Property.Name = szName; \
    Property.Value = szValue; \
    \
    pOutputStr = szName; \
    uiOutputSize = 512; \
    ToStr TEXT_NAME; \
    \
    pOutputStr = szValue; \
    uiOutputSize = 512; \
    ToStr TEXT_VALUE; \
    \
    if (uiDeptGroup > 0 && pDeptGroup[uiDeptGroup - 1]->pChilds == NULL) \
    pDeptGroup[uiDeptGroup - 1]->pChilds = &Property; \
    \
    if (pDeptGroup[uiDeptGroup] != NULL) \
    pDeptGroup[uiDeptGroup]->pSibling = &Property; \
    \
    pDeptGroup[uiDeptGroup] =  &Property; \
    ++uiDeptGroup;


#define END_PROPERTY_GROUP()     \
    pDeptGroup[uiDeptGroup] = NULL; \
    --uiDeptGroup; \
}

#define PROPERTY(NAME, VALUE) \
    BEGIN_PROPERTY_GROUP(NAME, VALUE) \
    END_PROPERTY_GROUP()

TextProperty* GetSMSProperties(GroupPropertiesHandle _Group)
{
    static TextProperty mRoot;

    {
        for (uint32 i = 0; i < sizeof(pDeptGroup) / sizeof(pDeptGroup[0]); ++i)
            pDeptGroup[i] = NULL;

        pDeptGroup[0] = &mRoot;
        uiDeptGroup = 0;
    }

    if (_Group == &GroupHandleSMS[kGroupSMS_VDP])
    {
        BEGIN_PROPERTY_GROUP(("Register $00 - Mode Control No. 1"), ("0x%02x", reg[0x00]));
            PROPERTY(("Bit 0 : Synch enable"), ("%s", (reg[0x00] >> 0) & 0x1 ? "Enable" : "Disable"));
            PROPERTY(("Bit 1 : Extra height enable/TMS9918 mode select"), ("%s", (reg[0x00] >> 1) & 0x1 ? "Enable" : "Disable"));
            PROPERTY(("Bit 2 : Mode 4 enable"), ("%s", (reg[0x00] >> 2) & 0x1 ? "Enable" : "Disable"));
            PROPERTY(("Bit 3 : Shift sprites left 8 pixels"), ("%s", (reg[0x00] >> 3) & 0x1 ? "Enable" : "Disable"));
            PROPERTY(("Bit 4 : Enable line interrupts"), ("%s", (reg[0x00] >> 4) & 0x1 ? "Enable" : "Disable"));
            PROPERTY(("Bit 5 : Hide leftmost 8 pixels"), ("%s", (reg[0x00] >> 5) & 0x1 ? "Enable" : "Disable"));
            PROPERTY(("Bit 6 : Horizontal scroll lock"), ("%s", (reg[0x00] >> 6) & 0x1 ? "Enable" : "Disable"));
            PROPERTY(("Bit 7 : Vertical scroll lock"), ("%s", (reg[0x00] >> 7) & 0x1 ? "Enable" : "Disable"));
        END_PROPERTY_GROUP();

        BEGIN_PROPERTY_GROUP(("Register $01 - Mode Control No. 2"), ("0x%02x", reg[0x01]));
            PROPERTY(("Bit 0 : Doubled (stretched) sprites"), ("%s", (reg[0x01] >> 0) & 0x1 ? "Enable" : "Disable"));
            PROPERTY(("Bit 1 : Large (tiled) sprites"), ("%s", (reg[0x01] >> 1) & 0x1 ? "Enable" : "Disable"));
            PROPERTY(("Bit 3 : 240-line mode/TMS9918 mode select"), ("%s", (reg[0x01] >> 3) & 0x1 ? "Enable" : "Disable"));
            PROPERTY(("Bit 4 : 224-line mode/TMS9918 mode select"), ("%s", (reg[0x01] >> 4) & 0x1 ? "Enable" : "Disable"));
            PROPERTY(("Bit 5 : Enable frame interrupts"), ("%s", (reg[0x01] >> 5) & 0x1 ? "Enable" : "Disable"));
            PROPERTY(("Bit 6 : Enable display"), ("%s", (reg[0x01] >> 6) & 0x1 ? "Enable" : "Disable"));
        END_PROPERTY_GROUP();

        BEGIN_PROPERTY_GROUP(("Register $02 - Name Table Base Address"), ("0x%02x", reg[0x02]));
        END_PROPERTY_GROUP();

        BEGIN_PROPERTY_GROUP(("Register $03 - Color Table Base Address"), ("0x%02x", reg[0x03]));
        END_PROPERTY_GROUP();

        BEGIN_PROPERTY_GROUP(("Register $04 - Pattern Generator Table Base Address"), ("0x%02x", reg[0x04]));
        END_PROPERTY_GROUP();

        BEGIN_PROPERTY_GROUP(("Register $05 - Sprite Attribute Table Base Address"), ("0x%02x", reg[0x05]));
        END_PROPERTY_GROUP();

        BEGIN_PROPERTY_GROUP(("Register $06 - Sprite Pattern Generator Table Base Address"), ("0x%02x", reg[0x06]));
        END_PROPERTY_GROUP();

        BEGIN_PROPERTY_GROUP(("Register $07 - Overscan/Backdrop Color"), ("0x%02x", reg[0x07]));
        END_PROPERTY_GROUP();

        BEGIN_PROPERTY_GROUP(("Register $08 - Background X Scroll"), ("0x%02x", reg[0x08]));
            PROPERTY(("Bit 0-7 : Horizontal scroll value"), ("%d", reg[0x08]));
        END_PROPERTY_GROUP();

        BEGIN_PROPERTY_GROUP(("Register $09 - Background Y Scroll"), ("0x%02x", reg[0x09]));
            PROPERTY(("Bit 0-7 : Vertical scroll value"), ("%d", reg[0x09]));
        END_PROPERTY_GROUP();

        BEGIN_PROPERTY_GROUP(("Register $0A - Line counter"), ("0x%02x", reg[0x0a]));
        END_PROPERTY_GROUP();
    }


    if (mRoot.pSibling != NULL)
        return mRoot.pSibling;

    return NULL;
}

TextProperty* GetMDProperties(GroupPropertiesHandle _Group)
{
    static TextProperty mRoot;

    {
        for (uint32 i = 0; i < sizeof(pDeptGroup) / sizeof(pDeptGroup[0]); ++i)
            pDeptGroup[i] = NULL;

        pDeptGroup[0] = &mRoot;
        uiDeptGroup = 0;
    }

    // TODO ADD STUFF

    if (mRoot.pSibling != NULL)
        return mRoot.pSibling;

    return NULL;
}

TextProperty* GetMCDProperties(GroupPropertiesHandle _Group)
{
    static TextProperty mRoot;

    {
        for (uint32 i = 0; i < sizeof(pDeptGroup) / sizeof(pDeptGroup[0]); ++i)
            pDeptGroup[i] = NULL;

        pDeptGroup[0] = &mRoot;
        uiDeptGroup = 0;
    }

    // TODO ADD STUFF

    if (mRoot.pSibling != NULL)
        return mRoot.pSibling;

    return NULL;
}

TextProperty* GetProperties(GroupPropertiesHandle _Group)
{
    if (system_hw == SYSTEM_MCD)
        return GetMCDProperties(_Group);
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
        return GetMDProperties(_Group);

    return GetSMSProperties(_Group);
}
