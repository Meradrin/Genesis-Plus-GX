#ifndef APIMarmelade_h
#define APIMarmelade_h

#ifdef __cplusplus
extern "C" {
#endif

#define CURRENT_EMU_MODULE_API_VERSION  1

typedef signed char  int8;
typedef signed short int16;
typedef signed int   int32;

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;

typedef void* EmulatorHandle;
typedef void* InputMgrHandle;
typedef void* SettingData;

typedef void* CPUHandle;
typedef void* RegistersHandle;
typedef void* DiassemblerHandle;
typedef void* CallstackHandle;
typedef void* MemoryHandle;
typedef void* MemoryMapHandle;

typedef void* BreakpointMgrHandle;
typedef void* GroupPropertiesHandle;
typedef void* TileInfoHandle;
typedef void* TileMapHandle;
typedef void* SpriteMgrHandle;
typedef void* SourceMgrHandle;

typedef uint32 PixelColor;

typedef void (*VideoRenderCallback)(const PixelColor* _pPixel, uint32 _uiWidth, uint32 _uiHeight, uint32 _uiPitch);
typedef void (*AudioSampleCallback)(int16 _iLeft, int16 _iRight);
typedef uint32 (*InputCallback)(uint32 _uiPort, uint32* _puiInputCount, float* _puiInputValue);

typedef void (*ExecutionBreak)(uint32 _uiCPUToBreak);

#define SAVESTATEMODE_NO_SUPPORT            0
#define SAVESTATEMODE_SAVE_SUPPORT          1
#define SAVESTATEMODE_LOAD_SUPPORT          2
#define SAVESTATEMODE_REWINDER_SUPPORT      4

struct APIEmulator
{
    uint32 (*GetTargetCount)();

    const char* (*GetConsoleName)(uint32 _uiTargetIndex);
    const char* (*GetEmulatorName)(uint32 _uiTargetIndex);
    const char* (*GetEmulatorVersion)(uint32 _uiTargetIndex);
    const char* (*GetROMFilters)(uint32 _uiTargetIndex);

    void (*SetVideoRenderCallback)(VideoRenderCallback _RenderFnc);
    void (*SetAudioSampleCallback)(AudioSampleCallback _AudioFnc);
    void (*SetInputCallback)(InputCallback _InputFnc);

    EmulatorHandle (*Init)(uint32 _uiTargetIndex);
    void (*Terminate)(EmulatorHandle _hData);
    void (*Reset)(EmulatorHandle _hData);
    
    void (*DoFrame)(EmulatorHandle _hData);
    
    bool (*OpenROM)(EmulatorHandle _hData, const char* _szPathFile);
    void (*CloseROM)(EmulatorHandle _hData);

    InputMgrHandle (*GetInputMgr)(uint32 _uiTargetIndex);
    SettingData    (*GetSettings)(uint32 _uiTargetIndex);

    // Debug Stuff
    void (*SetExecutionBreak)(ExecutionBreak _ExecBreakFnc);

    uint32     (*GetCPUCount)(EmulatorHandle _hData);
    CPUHandle  (*GetCPU)(EmulatorHandle _hData, uint32 _uiCPUIndex);

    uint32        (*GetMemoryCount)(EmulatorHandle _hData);
    MemoryHandle  (*GetMemory)(EmulatorHandle _hData, uint32 _uiMemIndex);

    uint32                (*GetGroupPropertiesCount)(EmulatorHandle _hData);
    GroupPropertiesHandle (*GetGroupProperties)(EmulatorHandle _hData, uint32 _uiIndex);

    BreakpointMgrHandle (*GetBreakpointMgr)(EmulatorHandle _hData);

    // Tile Render System API
    TileInfoHandle      (*GetTileInfo)(EmulatorHandle _hData);
    TileMapHandle       (*GetTileMapMgr)(EmulatorHandle _hData);
    SpriteMgrHandle     (*GetSpriteMgr)(EmulatorHandle _hData);

    // Source Debugger
    SourceMgrHandle  (*GetSourceMgr)(EmulatorHandle _hData);

    bool (*IsNeedBreak)(EmulatorHandle _hData);
    void (*Play)(EmulatorHandle _hData);
    void (*Break)(EmulatorHandle _hData);

    //Save State System
    uint32 (*GetSaveStateMode)(EmulatorHandle _hData);
    uint8* (*GetSaveState)(EmulatorHandle _hData, uint32* _puiSize);
    bool (*SetSaveState)(EmulatorHandle _hData, uint8* _pData, uint32 _uiSize);
    void (*DeleteSaveState)(EmulatorHandle _hData, uint8* _pData);
    const char* (*GetSaveStateExt)(EmulatorHandle _hData);

};

#define SETTING_TYPE_STRING     1
#define SETTING_TYPE_FILE       2
#define SETTING_TYPE_DIRECTORY  3
#define SETTING_TYPE_ENUM       4
#define SETTING_TYPE_INT        5
#define SETTING_TYPE_FLOAT      6
#define SETTING_TYPE_BOOL       7

#define SETTING_FIELD_SIZE      128

struct SettingTypeString
{
    int MaxStringSize;
};

struct SettingTypeFile
{
    uint32 Count;
    char*  Extensions[SETTING_FIELD_SIZE];
};

struct SettingTypeEnum
{
    uint32 Count;
    char*  Fields[SETTING_FIELD_SIZE];
};

struct SettingTypeInt
{
    int32 Min;
    int32 Max;
};

struct SettingTypeFloat
{
    float Min;
    float Max;
};

struct SettingParamInfo
{
    char    Name[SETTING_FIELD_SIZE];
    uint32  Type;
    
    union
    {
        SettingTypeString    Str;
        SettingTypeFile      File;
        SettingTypeEnum      Enum;
        SettingTypeInt       Integer;
        SettingTypeFloat     Float;
    };
};

struct SettingPage
{
    char Name[SETTING_FIELD_SIZE];
    uint32 ParamCount;
    SettingParamInfo* Params;
};

struct APISetting
{
    uint32       (*GetPageCount)();
    SettingPage* (*GetPage)(uint32 _uiPageIndex);
    void         (*SetValue)(uint32 _uiPageIndex, uint32 _uiFieldIndex, void* _pValue);
    void         (*GetValue)(uint32 _uiPageIndex, uint32 _uiFieldIndex, void* _pValue);
};

struct APIInputMgr
{
    int (*GetPortCount)(InputMgrHandle _hData);
    const char* (*GetPortName)(InputMgrHandle _hData, int _iPortIndex);

    int (*GetDeviceTypeCount)(InputMgrHandle _hData);
    const char* (*GetDeviceTypeName)(InputMgrHandle _hData, int _iDeviceTypeIndex);

    int (*GetDeviceInputCount)(InputMgrHandle _hData, int _iDeviceTypeIndex);
    const char* (*GetDeviceInputName)(InputMgrHandle _hData, int _iDeviceTypeIndex, int _iPortInputIndex);

    int (*GetDeviceOnPort)(InputMgrHandle _hData, int _iPortIndex);
    void (*SetDeviceOnPort)(InputMgrHandle _hData, int _iPortIndex, int _iDeviceTypeIndex);
};

#define VALID_STEP_IN     0x1
#define VALID_STEP_OUT    0x2
#define VALID_STEP_OVER   0x4

struct APICPUMgr
{
    const char* (*GetName)(CPUHandle _CPU);

    MemoryHandle      (*GetBusMem)(CPUHandle _CPU);
    RegistersHandle   (*GetRegisters)(CPUHandle _CPU);
    DiassemblerHandle (*GetDiasm)(CPUHandle _CPU);
    CallstackHandle   (*GetCallstack)(CPUHandle _CPU);

    unsigned (*GetValidStep)(CPUHandle _CPU);
    void (*StepIn)(CPUHandle _CPU);
    void (*StepOut)(CPUHandle _CPU);
    void (*StepOver)(CPUHandle _CPU); 
};

struct APIRegister
{
    int (*GetRegisterCount)(RegistersHandle  _Register);
    const char* (*GetRegisterName)(RegistersHandle _Register, int _iRegisterIndex);

    const char* (*GetRegisterValue)(RegistersHandle  _Register, int _iRegisterIndex);
    void (*SetRegisterValue)(RegistersHandle  _Register, int _iIndex, const char* _szValue);

    uint32 (*GetRunningAddr)(RegistersHandle  _Register);
    uint32 (*GetStackAddr)(RegistersHandle  _Register);
};

#define ENDIANNESS_LITTLE       0
#define ENDIANNESS_BIG          1

//  ?!?REG,[VIEW],[REG_INDEX]?!?
//  ?!?MEM,VIEW,MEMORY_INDEX,MEMORY_OFFSET?!?
//  ?!?LINK,VIEW,LINK_TO_POS?!?

struct APIDisassembler
{
    void (*GetOpcodeFormatedText)(DiassemblerHandle _Disasm, MemoryHandle _Mem, uint32 _uiPosition, uint32 _uiFlag, char* _szOutput, uint32 _uiOutputSize);
    void (*GetOpcodeText)(DiassemblerHandle _Disasm, MemoryHandle _Mem, uint32 _uiPosition, uint32 _uiFlag, char* _szOutput, uint32 _uiOutputSize);
    uint32 (*GetOpcodeSize)(DiassemblerHandle _Disasm, MemoryHandle _Mem, uint32 _uiPosition, uint32 _uiFlag);
    bool (*GetResultOperand)(DiassemblerHandle _Disasm, MemoryHandle _Mem, uint32 _uiPosition, uint32 _uiFlag, uint32 _iOperandIndex, uint32* _puiValue);
     
    uint32 (*GetEndiannessType)(DiassemblerHandle _Disasm);
    uint32 (*GetWordSize)(DiassemblerHandle _Disasm);
    uint32 (*GetLongSize)(DiassemblerHandle _Disasm);

    uint32 (*GetDumpFormatCount)(DiassemblerHandle _Disasm);
    const char* (*GetDumpFormatName)(DiassemblerHandle _Disasm, uint32 _uiIndex);
    void (*DumpFormat)(DiassemblerHandle _Disasm, uint32 _uiIndex, MemoryHandle _Mem, uint32 _uiStartPosition, uint32 _uiEndPosition, const char* _szFile);
};

struct APIMemory
{
    const char* (*GetName)(MemoryHandle _Mem);

    int  (*GetByte)(MemoryHandle _Mem, unsigned _uiPosition);
    bool (*SetByte)(MemoryHandle _Mem, unsigned _uiPosition, int _Value);

    unsigned        (*GetSize)(MemoryHandle _Mem);
    MemoryMapHandle (*GetMap)(MemoryHandle _Mem);
};

#define MAP_UNKNOW      0
#define MAP_CODE        1
#define MAP_SUBCODE     2
#define MAP_DATA_8      3
#define MAP_DATA_16     4
#define MAP_DATA_24     5
#define MAP_DATA_32     6
#define MAP_DATA_ASCII  7

struct APIMemoryMap
{
    const char* (*GetMapName)(MemoryMapHandle _Map, uint32 _uiPosition);

    void (*Clear)(MemoryMapHandle _Map);
    bool (*Save)(MemoryMapHandle _Map, const char* _szMapFile);
    bool (*Load)(MemoryMapHandle _Map, const char* _szMapFile, bool _bIgnoreSrcFile);

    uint32 (*GetMapType)(MemoryMapHandle _Map, uint32 _uiPosition);
    void (*SetMapType)(MemoryMapHandle _Map, uint32 _uiPosition, uint32 _uiDataType);

    uint32 (*GetFlagCount)(MemoryMapHandle _Map);
    const char* (*GetFlagName)(MemoryMapHandle _Map, uint32 _uiFlagIndex);

    uint32 (*GetFlags)(MemoryMapHandle _Map, uint32 _uiPosition);
    void (*SetFlags)(MemoryMapHandle _Map, uint32 _uiPosition, uint32 _uiFlags);

    uint32 (*GetCommentCount)(MemoryMapHandle _Map);
    void (*FillCommentPosition)(MemoryMapHandle _Map, uint32* _pCommentsPos);
    
    const char* (*GetComment)(MemoryMapHandle _Map, uint32 _uiPosition);
    void (*SetComment)(MemoryMapHandle _Map, uint32 _uiPosition, const char* _sComment);
 
    uint32 (*GetLabelCount)(MemoryMapHandle _Map);
    void (*FillLabelPosition)(MemoryMapHandle _Map, uint32* _pLabelsPos);
 
    uint32 (*FindLabel)(MemoryMapHandle _Map, const char* _sLabelName);
    const char* (*GetLabel)(MemoryMapHandle _Map, uint32 _uiPosition);
    void (*SetLabel)(MemoryMapHandle _Map, uint32 _uiPosition, const char* _sLabelName);
};

struct Breakpoint
{
    bool           Enabled;
    MemoryHandle   Zone;
    uint32         Start;
    uint32         End;
    bool           Exec;
    bool           Read;
    bool           Write;
    bool           Change;
};

struct APIBreakpointMgr
{
    void (*SetBreakpoints)(BreakpointMgrHandle _BreakMgr, Breakpoint* _pBreak, uint32 _uiBreakCount);
};

struct CallData
{
    uint32 uiRootAddress;
    uint32 uiCallAddress;
    uint32 uiStackPos;
    char* szAddInfo;
};

struct APICallstack
{
public:
    uint32 (*GetCallCount)(CallstackHandle _Callstack);
    CallData* (*GetCallInfo)(CallstackHandle _Callstack, uint32 _uiIndex);
};

struct TextProperty
{
    char* Name;
    char* Value;

    int UpdateCount;
    TextProperty* pChilds;
    TextProperty* pSibling;
};

struct APIGroupProperties
{ 
    const char* (*GetTitles)(GroupPropertiesHandle _Group);

    bool (*IsNeedRefresh)(GroupPropertiesHandle _Group);
    TextProperty* (*GetProperties)(GroupPropertiesHandle _Group);
};

struct APITileInfo
{
    int (*GetContextCount)(TileInfoHandle _TileInfo);
    const char* (*GetContext)(TileInfoHandle _TileInfo, int _uiIndex);
    const char* (*GetContextName)(TileInfoHandle _TileInfo, int _uiIndex);

    void (*GetPaletteGridSize)(TileInfoHandle _TileInfo, uint32* _pSizeX, uint32* _pSizeY);
    uint32 (*GetPaletteColor)(TileInfoHandle _TileInfo, int _iPalette, int _iColorIndex);
};

struct APITileMapMgr
{
    int (*GetMapCount)(TileMapHandle _Map);
    const char* (*GeMaptName)(TileMapHandle _Map, int _iIndex);
    void (*GetMapSize)(TileMapHandle _Map, int _iIndex, uint32* _pSizeX, uint32* _pSizeY);
    uint32* (*GetVisualMap)(TileMapHandle _Map, int _iIndex);
    const char* (*GetMapInfo)(TileMapHandle _Map, int _iIndex);
    void (*GetTileSize)(TileMapHandle _Map, int _iIndex, uint32* _pSizeX, uint32* _pSizeY);
};

// struct Sprite
// {
// public:
//     virtual System::Drawing::Size GetSize() = 0;
//     virtual void RenderSprite(HWFrameBuffer^ _Buffer) = 0;
// 
//     virtual System::String^ GetPropertyValue(int _iIndex) = 0;
// };
// 
// struct APISpriteMgr
// {
//     virtual int GetSpriteCount() = 0;
//     virtual Sprite^ GetSprite(int _iIndex) = 0;
// 
//     virtual int GetPropertiesCount() = 0;
//     virtual System::String^ GetPropertyName(int _iIndex) = 0;
//     virtual bool IsPropertyVisibleOnList(int _iIndex) = 0;
// };
// 
// struct APISourceMgr
// {
// 
// };

struct EmulatorModuleAPI
{
    APIEmulator        Emu;
    APISetting         Setting;
    APIInputMgr        Inputs;
    APICPUMgr          CPU;
    APIRegister        Register;
    APIDisassembler    Disasm;
    APIMemory          Memory;
    APIMemoryMap       MemoryMap;
    APIBreakpointMgr   Breakpoint;
    APICallstack       Callstack;
    APITileInfo        TileInfo;
    APITileMapMgr      TileMap;
    APIGroupProperties GroupProperties;
};

typedef uint32 Status;

#define STATUS_INVALID      0
#define STATUS_VALID        1

#ifdef EMULATOR_MODULE_USE_API
    typedef Status (*ModuleIsSupportedVersion)(uint32 _uiVersion);
    typedef Status (*ModuleGetAPI)(uint32 _uiVersion, EmulatorModuleAPI* _pAPI);
#else
    Status __declspec(dllexport) ModuleIsSupportedVersion(uint32 _uiVersion);
    Status __declspec(dllexport) ModuleGetAPI(uint32 _uiVersion, EmulatorModuleAPI* _pAPI);
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef __cplusplus
    }
#endif


#endif