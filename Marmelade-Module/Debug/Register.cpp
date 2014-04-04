#include "Register.h"
#include <stdio.h>

extern "C"
{
    #include "m68k.h"
    #include "z80.h"

    extern m68ki_cpu_core m68k;
    extern m68ki_cpu_core s68k;

    unsigned int m68k_get_reg(m68k_register_t regnum);
    void m68k_set_reg(m68k_register_t regnum, unsigned int value);

    unsigned int s68k_get_reg(m68k_register_t regnum);
    void s68k_set_reg(m68k_register_t regnum, unsigned int value);
};

enum RegisterTypes_e
{
    kRegister_Z80 = 1,
    kRegister_M68000,
    kRegister_S68000,
};

const char* gszRegisterZ80[] =
{
   "PC",
   "AF",
   "BC",
   "DE",
   "HL",
   "SP",
   "IX",
   "IY",
   "WZ",
   "AF'",
   "BC'",
   "DE'",
   "HL'",
};

const char* gszRegisterFormatZ80[] =
{
    "%04x",
    "%04x",
    "%04x",
    "%04x",
    "%04x",
    "%04x",
    "%04x",
    "%04x",
    "%04x",
    "%04x",
    "%04x",
    "%04x",
    "%04x",
};

PAIR* gRegisterZ80[] =
{
    &(Z80.pc),
    &(Z80.af),
    &(Z80.bc),
    &(Z80.de),
    &(Z80.hl),
    &(Z80.sp),
    &(Z80.ix),
    &(Z80.iy),
    &(Z80.wz),
    &(Z80.af2),
    &(Z80.bc2),
    &(Z80.de2),
    &(Z80.hl2),
};

const char* gszRegister68000[] =
{
    "PC",
    "D0",
    "D1",
    "D2",
    "D3",
    "D4",
    "D5",
    "D6",
    "D7",
    "A0",
    "A1",
    "A2",
    "A3",
    "A4",
    "A5",
    "A6",
    "A7",
    "SR",
};

const char* gszRegisterFormat68000[] =
{
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%08x",
    "%04x",
};

m68k_register_t guiConv68000DebugToEmu[] = 
{
    M68K_REG_PC,
    M68K_REG_D0,
    M68K_REG_D1,
    M68K_REG_D2,
    M68K_REG_D3,
    M68K_REG_D4,
    M68K_REG_D5,
    M68K_REG_D6,
    M68K_REG_D7,
    M68K_REG_A0,
    M68K_REG_A1,
    M68K_REG_A2,
    M68K_REG_A3,
    M68K_REG_A4,
    M68K_REG_A5,
    M68K_REG_A6,
    M68K_REG_A7,
    M68K_REG_SR,
};

RegistersHandle GetZ80Register()
{
    return (RegistersHandle)kRegister_Z80;
}

RegistersHandle GetM68000Register()
{
    return (RegistersHandle)kRegister_M68000;
}

RegistersHandle GetS68000Register()
{
    return (RegistersHandle)kRegister_S68000;
}

int GetRegisterCount(RegistersHandle _Register)
{
    switch ((uint32)_Register)
    {
    case kRegister_Z80:
        return sizeof(gszRegisterZ80) / sizeof(const char*);
    case kRegister_M68000:
        return sizeof(gszRegister68000) / sizeof(const char*);
    case kRegister_S68000:
        return sizeof(gszRegister68000) / sizeof(const char*);
    default:
        return 0;
    }
}

const char* GetRegisterName(RegistersHandle _Register, int _iRegisterIndex)
{
    switch ((uint32)_Register)
    {
    case kRegister_Z80:
        return gszRegisterZ80[_iRegisterIndex];
    case kRegister_M68000:
        return gszRegister68000[_iRegisterIndex];
    case kRegister_S68000:
        return gszRegister68000[_iRegisterIndex];
    default:
        return NULL;
    }
}

const char* GetRegisterValue(RegistersHandle _Register, int _iRegisterIndex)
{
    static char szRegisters[128];

    szRegisters[0] = '\0';

    switch ((uint32)_Register)
    {
    case kRegister_Z80:
        sprintf(szRegisters, gszRegisterFormatZ80[_iRegisterIndex], gRegisterZ80[_iRegisterIndex]->d);
        break;
    case kRegister_M68000:
        sprintf(szRegisters, gszRegisterFormat68000[_iRegisterIndex], m68k_get_reg(guiConv68000DebugToEmu[_iRegisterIndex]));
        break;
    case kRegister_S68000:
        sprintf(szRegisters, gszRegisterFormat68000[_iRegisterIndex], s68k_get_reg(guiConv68000DebugToEmu[_iRegisterIndex]));
        break;
    }

    return szRegisters;
}

void SetRegisterValue(RegistersHandle _Register, int _iIndex, const char* _szValue)
{
    switch ((uint32)_Register)
    {
    case kRegister_Z80:
        {
            uint32 uiRegister = 0;
            sscanf(_szValue, gszRegisterFormatZ80[_iIndex], &uiRegister);
            gRegisterZ80[_iIndex]->d = uiRegister;
            break;
        }
    case kRegister_M68000:
        {
            uint32 uiRegister = 0;
            sscanf(_szValue, gszRegisterFormat68000[_iIndex], &uiRegister);
            m68k_set_reg(guiConv68000DebugToEmu[_iIndex], uiRegister);
            break;
        }
    case kRegister_S68000:
        {
            uint32 uiRegister = 0;
            sscanf(_szValue, gszRegisterFormat68000[_iIndex], &uiRegister);
            s68k_set_reg(guiConv68000DebugToEmu[_iIndex], uiRegister);
            break;
        }
    }
}

uint32 GetRunningAddr(RegistersHandle _Register)
{
    switch ((uint32)_Register)
    {
    case kRegister_Z80:
        return gRegisterZ80[0]->d;
    case kRegister_M68000:
        return m68k_get_reg(M68K_REG_PC);
    case kRegister_S68000:
        return m68k_get_reg(M68K_REG_PC);
    default:
        return 0;
    }
}

uint32 GetStackAddr(RegistersHandle _Register)
{
    switch ((uint32)_Register)
    {
    case kRegister_Z80:
        return gRegisterZ80[5]->d;
    case kRegister_M68000:
        return m68k_get_reg(M68K_REG_SP);
    case kRegister_S68000:
        return m68k_get_reg(M68K_REG_SP);
    default:
        return 0;
    }
}
