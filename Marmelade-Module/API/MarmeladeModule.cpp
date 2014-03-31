#include "API\APIMarmelade.h"
#include "Debug\Memory.h"
#include "Debug\DebugCPU.h"
#include "Debug\Register.h"
#include "Debug\Disasm.h"
#include "Debug\MemoryMap.h"
#include "Debug\DebugInfo.h"
#include "Debug\Breakpoint.h"

extern "C"
{
    #include "shared.h"
}

enum System_e
{
    kSystem_SG1000,
    kSystem_MasterSystem,
    kSystem_Megadrive,
    kSystem_MegaCD,
    kSystem_Count,
};

struct TargetInfo
{
    System_e WantedSystem;
    PixelColor RenderBuffer[320 * 480];
} gInfo;

VideoRenderCallback gRenderFnc = NULL;
AudioSampleCallback gAudioFnc = NULL;
InputCallback gInputFnc = NULL;

t_config config;

char GG_ROM[256];
char AR_ROM[256];
char SK_ROM[256];
char SK_UPMEM[256];
char GG_BIOS[256];
char CD_BIOS_EU[256];
char CD_BIOS_US[256];
char CD_BIOS_JP[256];
char MS_BIOS_US[256];
char MS_BIOS_EU[256];
char MS_BIOS_JP[256];

int16 soundbuffer[3068];

extern "C" void error(char* _szStr, ...)
{

}

#define CHUNKSIZE   (0x10000)

int load_archive(char *filename, unsigned char *buffer, int maxsize, char *extension)
{
    int size = 0;
    char in[CHUNKSIZE];
    char msg[64] = "Unable to open file";

    /* Open file */
    FILE *fd = fopen(filename, "rb");

    /* Master System & Game Gear BIOS are optional files */
    if (!strcmp(filename,MS_BIOS_US) || !strcmp(filename,MS_BIOS_EU) || !strcmp(filename,MS_BIOS_JP) || !strcmp(filename,GG_BIOS))
    {
        /* disable all messages */
    }

    /* Mega CD BIOS are required files */
    if (!strcmp(filename,CD_BIOS_US) || !strcmp(filename,CD_BIOS_EU) || !strcmp(filename,CD_BIOS_JP)) 
    {
        sprintf(msg, "Unable to open CD BIOS: %s", filename);
    }

    if (!fd)
    {
        fprintf(stderr, "ERROR - %s.\n", msg);
        return 0;
    }

    /* Read first chunk */
    fread(in, CHUNKSIZE, 1, fd);

    {
        int left;
        /* Get file size */
        fseek(fd, 0, SEEK_END);
        size = ftell(fd);
        fseek(fd, 0, SEEK_SET);

        /* size limit */
        if(size > maxsize)
        {
            fclose(fd);
            fprintf(stderr, "ERROR - File is too large.\n");
            return 0;
        }

        sprintf((char *)msg,"Loading %d bytes ...", size);
        fprintf(stderr, "INFORMATION - %s\n", msg);

        /* filename extension */
        if (extension)
        {
            memcpy(extension, &filename[strlen(filename) - 3], 3);
            extension[3] = 0;
        }

        /* Read into buffer */
        left = size;
        while (left > CHUNKSIZE)
        {
            fread(buffer, CHUNKSIZE, 1, fd);
            buffer += CHUNKSIZE;
            left -= CHUNKSIZE;
        }

        /* Read remaining bytes */
        fread(buffer, left, 1, fd);
    }

    /* Close file */
    fclose(fd);

    /* Return loaded ROM size */
    return size;
}

void SetRenderFnc(VideoRenderCallback _RenderFnc)
{
    gRenderFnc = _RenderFnc;
}

void SetAudioFnc(AudioSampleCallback _AudioFnc)
{
    gAudioFnc = _AudioFnc;
}

void SetInputFnc(InputCallback _InputFnc)
{
    gInputFnc = _InputFnc;
}

#define CONFIG_VERSION "GENPLUS-GX 1.6.1"

void InitConfig()
{
    switch (gInfo.WantedSystem)
    {
    case kSystem_SG1000:
        config.system = SYSTEM_SG;
        break;
    case kSystem_MasterSystem:
        config.system = SYSTEM_SMS;
        break;
    case kSystem_Megadrive:
        config.system = SYSTEM_MD;
        break;
    case kSystem_MegaCD:
        config.system = SYSTEM_MCD;
        break;
    }

    /* version TAG */
    strncpy(config.version,CONFIG_VERSION,16);

    /* sound options */
    config.psg_preamp     = 150;
    config.fm_preamp      = 100;
    config.hq_fm          = 1; /* high-quality resampling */
    config.psgBoostNoise  = 1;
    config.filter         = 0; /* no filter */
    config.lp_range       = 0x9999; /* 0.6 in 16.16 fixed point */
    config.low_freq       = 880;
    config.high_freq      = 5000;
    config.lg             = 1.0;
    config.mg             = 1.0;
    config.hg             = 1.0;
    config.dac_bits 	     = 14; /* MAX DEPTH */ 
    config.ym2413         = 2; /* AUTO */
    config.mono           = 0; /* STEREO output */

    /* system options */
    config.system         = 0; /* AUTO */
    config.region_detect  = 0; /* AUTO */
    config.vdp_mode       = 0; /* AUTO */
    config.master_clock   = 0; /* AUTO */
    config.force_dtack    = 0;
    config.addr_error     = 1;
    config.bios           = 0;
    config.lock_on        = 0;

    /* video options */
    config.overscan = 0;
    config.gg_extra = 0;
    config.ntsc     = 0;
    config.render   = 0;

    YM2612Config(config.dac_bits);
}

uint32 GetTargetCount()
{
    return kSystem_Count;
}
static const char szEmpty[] = "";

const char* GetConsoleName(uint32 _uiTargetIndex)
{
    static char szConsoles[kSystem_Count][80] =
    {
        "SG-1000",
        "Master System",
        "Megadrive",
        "Mega CD",
    };

    if (_uiTargetIndex < kSystem_Count)
        return szConsoles[_uiTargetIndex];

    return szEmpty;
}

const char* GetEmulatorName(uint32 _uiTargetIndex)
{
    static char szEmulator[] = "Genesis Plus-GX";
    return szEmulator;
}

const char* GetEmulatorVersion(uint32 _uiTargetIndex)
{
    static char szVersion[] = "1.7.3";
    return szVersion;
}

const char* GetFilters(uint32 _uiTargetIndex)
{
    static char szFilters[kSystem_Count][256] =
    {
        "SG-1000 files (*.sg)|*.sg|All Files (*.*)|*.*",
        "Master System files (*.sms;*.gg)|*.sms;*.gg|All Files (*.*)|*.*",
        "Megadrive files (*.md)|*.md|All Files (*.*)|*.*",
        "Sega CD files (*.cue, *.bin, *.iso)|*.cue;*.bin;*.iso|All Files (*.*)|*.*",
    };

    if (_uiTargetIndex < kSystem_Count)
        return szFilters[_uiTargetIndex];

    return szEmpty;
}

EmulatorHandle Init(uint32 _uiTargetIndex)
{
    gInfo.WantedSystem = (System_e)_uiTargetIndex;

    bitmap.data = (uint8*)gInfo.RenderBuffer;
    bitmap.width = 320;
    bitmap.height = 480;
    bitmap.pitch = 320 * sizeof(PixelColor);

    InitConfig();
     
    return &gInfo;
}

void Terminate(EmulatorHandle _hData)
{
}

void RenderFrame(EmulatorHandle _hData)
{
    if (system_hw == SYSTEM_MCD)
        system_frame_scd(0);
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
        system_frame_gen(0);
    else
        system_frame_sms(0);

    if (gRenderFnc != NULL)
    {
        if (system_hw == SYSTEM_GG)
            gRenderFnc((PixelColor*)bitmap.data, 160, 144, bitmap.pitch); // Why Game Gear viewport need to be hardcoded ?
        else
            gRenderFnc((PixelColor*)bitmap.data, bitmap.viewport.w, bitmap.viewport.h, bitmap.pitch);
    }

    int aud = audio_update(soundbuffer) << 1;

    if (gAudioFnc != NULL)
    {
        for (int i = 0; i < aud; i += 2)
            gAudioFnc(soundbuffer[i], soundbuffer[i + 1]);
    }
}

bool IsNeedBreak(EmulatorHandle _hData)
{
    bool bIsNeedBreak = false;

    for (int i = 0; i < kCPU_Count; ++i)
        bIsNeedBreak |= gbNeedBreak[i];

    return bIsNeedBreak;
}

void Play(EmulatorHandle _hData)
{
    for (int i = 0; i < kCPU_Count; ++i)
        gbNeedBreak[i] = false;
}

void Break(EmulatorHandle _hData)
{
    if (system_hw == SYSTEM_MCD || (system_hw & SYSTEM_PBC) == SYSTEM_MD)
        gbNeedBreak[1] = true;

    gbNeedBreak[0] = true;
}

bool OpenROM(EmulatorHandle _hData, const char* _szROMFile)
{
    if (!load_rom((char*)_szROMFile))
        return false;

    switch (system_hw)
    {
    case SYSTEM_MD:
    case SYSTEM_MCD:
        for(uint32 i = 0; i < MAX_INPUTS; i++)
        {
            config.input[i].padtype = DEVICE_PAD6B;
        }	
        input.system[0] = SYSTEM_MD_GAMEPAD;
        input.system[1] = SYSTEM_MD_GAMEPAD;
        break;
    case SYSTEM_GG:
    case SYSTEM_SMS:
        input.system[0] = SYSTEM_MS_GAMEPAD;
        input.system[1] = SYSTEM_MS_GAMEPAD;
        break;
    default:
        break;
    }

    static const double pal_fps = 53203424.0 / (3420.0 * 313.0);
    static const double ntsc_fps = 53693175.0 / (3420.0 * 262.0);

    audio_init(48000, vdp_pal ? pal_fps : ntsc_fps);

    system_init();
    system_reset();

    return true;
}

void Reset(EmulatorHandle _hData)
{
    system_reset();
}

InputMgrHandle GetInputMgr(uint32 _uiTargetIndex)
{
    return &input;
}

int GetPortCount(InputMgrHandle _hData)
{
    if (_hData == &input)
        return 2;

    return 0;
}

const char* GetPortName(InputMgrHandle _hData, int _iPortIndex)
{
    static char szPortNames[2][64] = 
    {
        "Port 1",
        "Port 2",
    };

    if (_hData == &input && _iPortIndex < 2)
        return szPortNames[_iPortIndex];

    return szEmpty;
}

int GetDeviceTypeCount(InputMgrHandle _hData)
{
    if (_hData == &input)
        return 12;

    return 0;
}

enum DeviceTypes_e
{
    kDevice_None,
    kDevice_GamePad3B,
    kDevice_GamePad6B,
    kDevice_GamePad2B,    
    kDevice_Mouse,
    kDevice_Lightgun,
    kDevice_Paddle,
    kDevice_SportSpad,
    kDevice_PICO,
    kDevice_Terebi,
    kDevice_XE_A1P,
    kDevice_Activator,
    kDevice_Count,
};

const char* GetDeviceTypeName(InputMgrHandle _hData, int _iDeviceTypeIndex)
{
    static char szDeviceNames[kDevice_Count][128] = 
    {
        "None",
        "3-buttons Control Pad",
        "6-buttons Control Pad",
        "2-buttons Control Pad",
        "Sega Mouse",
        "Sega Light Phaser, Menacer, or Konami Justifiers",
        "Sega Paddle Control",
        "Sega Sports Pad",
        "PICO tablet",
        "Terebi Oekaki tablet",
        "XE-A1P analog controller",
        "Activator",
    };

    if (_hData == &input && _iDeviceTypeIndex < kDevice_Count)
        return szDeviceNames[_iDeviceTypeIndex];

    return szEmpty;
}

static int giDeviceInputCount[kDevice_Count] =
{
    0,
    8,
    12,
    7,
};

int GetDeviceInputCount(InputMgrHandle _hData, int _iDeviceTypeIndex)
{
    if (_hData == &input && _iDeviceTypeIndex < kDevice_Count)
        return giDeviceInputCount[_iDeviceTypeIndex];

    return 0;
}

struct InputInfo
{
    const char szName[64];
    uint16 uiInput;
};

InputInfo gGamePad2B[] = 
{
    {"Up", INPUT_UP},
    {"Down", INPUT_DOWN},
    {"Left", INPUT_LEFT},
    {"Right", INPUT_RIGHT},
    {"Start", INPUT_START},
    {"1", INPUT_BUTTON1},
    {"2", INPUT_BUTTON2},
};

InputInfo gGamePad3B[] = 
{
    {"Up", INPUT_UP},
    {"Down", INPUT_DOWN},
    {"Left", INPUT_LEFT},
    {"Right", INPUT_RIGHT},
    {"Start", INPUT_START},
    {"A", INPUT_A},   
    {"B", INPUT_C},     
    {"C", INPUT_B},
};

InputInfo gGamePad6B[] = 
{
    {"Up", INPUT_UP},
    {"Down", INPUT_DOWN},
    {"Left", INPUT_LEFT},
    {"Right", INPUT_RIGHT},
    {"Start", INPUT_START},
    {"Mode", INPUT_MODE},
    {"A", INPUT_A},   
    {"B", INPUT_C},     
    {"C", INPUT_B},
    {"X", INPUT_X},  
    {"Y", INPUT_Y},
    {"Z", INPUT_Z},
};


const char* GetDeviceInputName(InputMgrHandle _hData, int _iDeviceTypeIndex, int _iPortInputIndex)
{
    if (_hData == &input && _iDeviceTypeIndex < kDevice_Count && _iPortInputIndex < giDeviceInputCount[_iDeviceTypeIndex])
    {
        switch (_iDeviceTypeIndex)
        {
        case kDevice_GamePad2B:
            return gGamePad2B[_iPortInputIndex].szName;
        case kDevice_GamePad3B:
            return gGamePad3B[_iPortInputIndex].szName;
        case kDevice_GamePad6B:
            return gGamePad6B[_iPortInputIndex].szName;
        }
    }

    return szEmpty;
}

int giDeviceValue[kDevice_Count] =
{
    NO_DEVICE,
    DEVICE_PAD3B,
    DEVICE_PAD6B,
    DEVICE_PAD2B,
    DEVICE_MOUSE,
    DEVICE_LIGHTGUN,
    DEVICE_PADDLE,
    DEVICE_SPORTSPAD,
    DEVICE_PICO,
    DEVICE_TEREBI,
    DEVICE_XE_A1P,
    DEVICE_ACTIVATOR,
};

int GetDeviceOnPort(InputMgrHandle _hData, int _iPortIndex)
{
    if (_hData == &input && _iPortIndex < 2)
    {
        int iValue = (_iPortIndex == 0 ? input.dev[0] : input.dev[4]);

        for (uint32 i = 0; i < kDevice_Count; ++i)
        {
            if (iValue == giDeviceValue[i])
                return i;
        }
    }

    return 0;
}

void SetDeviceOnPort(InputMgrHandle _hData, int _iPortIndex, int _iDeviceTypeIndex)
{
    if (_hData == &input && _iPortIndex < 2 && _iDeviceTypeIndex < kDevice_Count)
        (_iPortIndex == 0 ? input.dev[0] : input.dev[4]) = giDeviceValue[_iDeviceTypeIndex];
}

void osd_input_update()
{
    float fInputs[256];
    uint8 uiLastFrameDev[2] = {input.dev[0], input.dev[4]};
    bool bNeedReset = false;

    for (uint32 i = 0; i < 2; ++i)
    {
        uint32 uiInputCount = 0;
        uint32 uiDeviceType = gInputFnc(i, &uiInputCount, fInputs);

        InputInfo* pInputInfo = NULL;
        uint8 uiDevID = NO_DEVICE;
        uint8 uiSystemID = NO_SYSTEM;

        switch (uiDeviceType)
        {
        case kDevice_GamePad2B:
            pInputInfo = gGamePad2B;
            uiDevID = DEVICE_PAD2B;
            uiSystemID = SYSTEM_MS_GAMEPAD;
            break;
        case kDevice_GamePad3B:
            pInputInfo = gGamePad3B;
            uiDevID = DEVICE_PAD3B;
            uiSystemID = SYSTEM_MD_GAMEPAD;
            break;
        case kDevice_GamePad6B:
            pInputInfo = gGamePad6B;
            uiDevID = DEVICE_PAD6B;
            uiSystemID = SYSTEM_MD_GAMEPAD;
            break;
        }

        uint32 uiInput = 0;

        for (uint32 j = 0; j < uiInputCount; ++j)
        {
            if (fInputs[j] != 0.0f)
                uiInput |= pInputInfo[j].uiInput;
        }

        input.system[i] = uiSystemID;
//         (i == 0 ? input.dev[0] : input.dev[4]) = uiDevID;
        (i == 0 ? input.pad[0] : input.pad[4]) = uiInput;

//         if (uiLastFrameDev[i] != uiDevID)
//             bNeedReset = true;
    }

    if (bNeedReset)
    {
        io_init();
        input_reset();
    }
}

BreakpointMgrHandle GetBreakpointMgr(EmulatorHandle _hData)
{
    return GetBreakHandle();
}

Status ModuleIsSupportedVersion(uint32 _uiVersion)
{
    if (_uiVersion == CURRENT_EMU_MODULE_API_VERSION)
        return STATUS_VALID;

    return STATUS_INVALID;
}

Status ModuleGetAPI(uint32 _uiVersion, EmulatorModuleAPI* _pAPI)
{
    if (_uiVersion != CURRENT_EMU_MODULE_API_VERSION)
        return STATUS_INVALID;
    _pAPI->Emu.SetVideoRenderCallback = SetRenderFnc;
    _pAPI->Emu.SetAudioSampleCallback = SetAudioFnc;
    _pAPI->Emu.SetInputCallback = SetInputFnc;
    _pAPI->Emu.GetTargetCount = GetTargetCount;
    _pAPI->Emu.GetConsoleName = GetConsoleName;
    _pAPI->Emu.GetEmulatorName = GetEmulatorName;
    _pAPI->Emu.GetEmulatorVersion = GetEmulatorVersion;
    _pAPI->Emu.GetROMFilters = GetFilters;
    _pAPI->Emu.Init = Init;
    _pAPI->Emu.Terminate = Terminate;
    _pAPI->Emu.DoFrame = RenderFrame;
    _pAPI->Emu.OpenROM = OpenROM;
    _pAPI->Emu.Reset = Reset;
    _pAPI->Emu.Play = Play;
    _pAPI->Emu.Break = Break;
    _pAPI->Emu.IsNeedBreak = IsNeedBreak;
    _pAPI->Emu.GetInputMgr = GetInputMgr;
    _pAPI->Emu.GetMemoryCount = GetMemoryCount;
    _pAPI->Emu.GetMemory = GetMemory;
//    _pAPI->Emu.GetTileInfo = GetTileInfo;
    _pAPI->Emu.GetGroupPropertiesCount = GetGroupPropertiesCount;
    _pAPI->Emu.GetGroupProperties = GetGroupProperties;
    _pAPI->Emu.GetCPUCount = GetCPUCount;
    _pAPI->Emu.GetCPU = GetCPU;
    _pAPI->Emu.SetExecutionBreak = SetExecutionBreak;
    _pAPI->Emu.GetBreakpointMgr = GetBreakpointMgr;
//    _pAPI->Emu.GetSaveStateMode = GetSaveStateMode;
//    _pAPI->Emu.GetSaveState = GetSaveState;
//    _pAPI->Emu.SetSaveState = SetSaveState;
//    _pAPI->Emu.DeleteSaveState = DeleteSaveState;
//    _pAPI->Emu.GetSaveStateExt = GetSaveStateExt;

    // Input
    _pAPI->Inputs.GetPortCount = GetPortCount;
    _pAPI->Inputs.GetPortName = GetPortName;
    _pAPI->Inputs.GetDeviceTypeCount = GetDeviceTypeCount;
    _pAPI->Inputs.GetDeviceTypeName = GetDeviceTypeName;
    _pAPI->Inputs.GetDeviceInputCount = GetDeviceInputCount;
    _pAPI->Inputs.GetDeviceInputName = GetDeviceInputName;
    _pAPI->Inputs.GetDeviceOnPort = GetDeviceOnPort;
    _pAPI->Inputs.SetDeviceOnPort = SetDeviceOnPort;

    // Group Properties
    _pAPI->GroupProperties.GetTitles = GetTitles;
    _pAPI->GroupProperties.IsNeedRefresh = IsNeedRefresh;
    _pAPI->GroupProperties.GetProperties = GetProperties;

    // CPU
    _pAPI->CPU.GetBusMem = GetBusMem;
    _pAPI->CPU.GetCallstack = GetCallstack;
    _pAPI->CPU.GetDiasm = GetDiasm; 
    _pAPI->CPU.GetName = GetCPUName;
    _pAPI->CPU.GetRegisters = GetRegisters;
    _pAPI->CPU.GetValidStep = GetValidStep;
    _pAPI->CPU.StepIn = StepIn;
    _pAPI->CPU.StepOut = StepOut;
    _pAPI->CPU.StepOver = StepOver;

    _pAPI->Register.GetRegisterCount = GetRegisterCount;
    _pAPI->Register.GetRegisterName = GetRegisterName;
    _pAPI->Register.GetRegisterValue = GetRegisterValue;
    _pAPI->Register.SetRegisterValue = SetRegisterValue;
    _pAPI->Register.GetRunningAddr = GetRunningAddr;
    _pAPI->Register.GetStackAddr = GetStackAddr;

    _pAPI->Disasm.DumpFormat = DumpFormat;
    _pAPI->Disasm.GetDumpFormatCount = GetDumpFormatCount;
    _pAPI->Disasm.GetDumpFormatName = GetDumpFormatName;
    _pAPI->Disasm.GetEndiannessType = GetEndiannessType;
    _pAPI->Disasm.GetLongSize = GetLongSize;
    _pAPI->Disasm.GetWordSize = GetWordSize;
    _pAPI->Disasm.GetOpcodeSize = GetOpcodeSize;
    _pAPI->Disasm.GetOpcodeText = GetOpcodeText;
    _pAPI->Disasm.GetOpcodeFormatedText = GetOpcodeFormatedText;
    _pAPI->Disasm.GetResultOperand = GetResultOperand;

    // Memory
    _pAPI->Memory.GetName = GetName;
    _pAPI->Memory.GetByte = GetByte;
    _pAPI->Memory.SetByte = SetByte;
    _pAPI->Memory.GetSize = GetSize;
    _pAPI->Memory.GetMap = GetMap;

    // Memory Map
    _pAPI->MemoryMap.Clear = ClearMap;
    _pAPI->MemoryMap.Load = LoadMap;
    _pAPI->MemoryMap.Save = SaveMap;
    _pAPI->MemoryMap.FillLabelPosition = FillLabelPosition;
    _pAPI->MemoryMap.FindLabel = FindLabel;
    _pAPI->MemoryMap.GetComment = GetComment;
    _pAPI->MemoryMap.GetFlagCount = GetFlagCount;
    _pAPI->MemoryMap.GetFlagName = GetFlagName;
    _pAPI->MemoryMap.GetFlags = GetFlags;
    _pAPI->MemoryMap.GetLabel = GetLabel;
    _pAPI->MemoryMap.GetLabelCount = GetLabelCount;
    _pAPI->MemoryMap.GetMapName = GetMapName;
    _pAPI->MemoryMap.GetMapType = GetMapType;
    _pAPI->MemoryMap.SetComment = SetComment;
    _pAPI->MemoryMap.SetFlags = SetFlags;
    _pAPI->MemoryMap.SetLabel = SetLabel;
    _pAPI->MemoryMap.SetMapType = SetMapType;
    _pAPI->MemoryMap.GetCommentCount = GetCommentCount;
    _pAPI->MemoryMap.FillCommentPosition = FillCommentPosition;

    // Breakpoints
    _pAPI->Breakpoint.SetBreakpoints = SetBreakpoints;

    return STATUS_VALID;
}
