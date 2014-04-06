#include "Disasm.h"
#include "MemoryMap.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

extern "C"
{
    #include "m68k.h"
    #include "z80.h"
};

// Based on Gens disassembler
// Globaly it is really a ugly code, but that do the job.
namespace Disasm68000
{
    extern "C"
    {
        extern m68ki_cpu_core m68k;
        extern m68ki_cpu_core s68k;
    }

    m68ki_cpu_core* gCurrentCore = &m68k;

    class Diasm68k
    {
    public:

        static char*  GetOpcodeText(uint32 _uiPosition);
        static char*  GetOpcodeSpecialText(uint32 _uiPosition);
        static uint32 GetOpcodeSize(uint16 _uiOpcode);
        static uint32 GetAdressingDataSize(uint32 _uiSize, uint32 _uiEANum, uint32 _uiRegNum);

    private:
        enum AdressingModes_e
        {
            kEA_DRegister,
            kEA_ARegister,
            kEA_ARegisterIndirect,
            kEA_ARegisterIndirectPostInc,
            kEA_ARegisterIndirectPostDec,
            kEA_ARegisterIndirectDisplacement,
            kEA_ARegisterIndirectIndexDisplacement,
            kEA_AbsoluteWord,
            kEA_AbsoluteLong,
            kEA_Relative,
            kEA_RelativePlusIndex,
            kEA_Immediate,

            kEA_Count,
        };


        enum SizeTypes_e
        {
            kSize_Byte,
            kSize_Word,
            kSize_Long,

            kSize_Count,
        };

        enum ConditionTypes_e
        {
            kCond_Tr,
            kCond_Fa,
            kCond_HI,
            kCond_LS,
            kCond_CC,
            kCond_CS,
            kCond_NE,
            kCond_EQ,
            kCond_VC,
            kCond_VS,
            kCond_PL,
            kCond_MI,
            kCond_GE,
            kCond_LT,
            kCond_GT,
            kCond_LE,

            kCond_Count,
        };

        static uint32 sm_bIsLowerCase;

        static char sm_szLastOpcode[64];

        static char sm_szSizeTypes[kSize_Count][3];
        static char sm_szConditionTypes[kCond_Count][3];
    };

    uint32 Diasm68k::sm_bIsLowerCase = true;

    char Diasm68k::sm_szLastOpcode[64];


    char Diasm68k::sm_szSizeTypes[kSize_Count][3] =
    {
        "B",
        "W",
        "L",
    };

    char Diasm68k::sm_szConditionTypes[kCond_Count][3] =
    {
        "TR",
        "FA",
        "HI",
        "LS",
        "CC",
        "CS",
        "NE",
        "EQ",
        "VC",
        "VS",
        "PL",
        "MI",
        "GE",
        "LT",
        "GT",
        "LE",
    };

    static uint32 guiCurrentFlow;

    static uint16 NextWord()
    {
        uint16 uiValue = READ_BYTE(gCurrentCore->memory_map[(guiCurrentFlow >> 16) & 0xff].base, guiCurrentFlow & 0xffff);
        ++guiCurrentFlow;
        uiValue <<= 8;
        uiValue |= READ_BYTE(gCurrentCore->memory_map[(guiCurrentFlow >> 16) & 0xff].base, guiCurrentFlow & 0xffff);
        ++guiCurrentFlow;

        return uiValue;
    }

    static uint32 NextLong()
    {
        uint32 uiValue = NextWord();

        uiValue <<= 16;
        uiValue |= NextWord();

        return uiValue;
    }


    // Really ugly conversion :\ I will need to think to all this.
    uint32 Diasm68k::GetAdressingDataSize(uint32 _uiSize, uint32 _uiEANum, uint32 _uiRegNum)
    {
        if (_uiEANum == 0x6 || _uiEANum == 0x5)
        {
            return 2;
        }
        else if (_uiEANum == 0x7)
        {
            switch(_uiRegNum)
            {
            case 0: case 2: case 3:
                return 2;
            case 1:
                return 4;
            case 4:
                return (_uiSize == 0x2 ? 4 : 2);
            }
        }

        return 0;

    }

    uint32 Diasm68k::GetOpcodeSize(uint16 _uiOpcode)
    {
        uint32 OPC = _uiOpcode;

        switch(OPC >> 12)
        {
        case 0:

            if (OPC & 0x100)
            {
                if ((OPC & 0x038) == 0x8)
                {
                    if (OPC & 0x080)
                        //MOVEP.z Ds,d16(Ad)
                        return 4;//sprintf(Dbg_Str, "MOVEP%-3sD%.1d,#$%.4X(A%.1d)%c", Make_Dbg_Size_Str_2((OPC & 0x40) >> 6), (OPC & 0xE00) >> 9, Next_Word(), OPC & 0x7, 0);
                    else
                        //MOVEP.z d16(As),Dd
                        return 4;//sprintf(Dbg_Str, "MOVEP%-3s#$%.4X(A%.1d),D%.1d%c", Make_Dbg_Size_Str_2((OPC & 0x40) >> 6), Next_Word(), OPC & 0x7, (OPC & 0xE00) >> 9, 0);
                }
                else
                {
                    switch((OPC >> 6) & 0x3)
                    {
                    case 0:
                        //BTST  Ds,a
                        return 2 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 7);
                        //sprintf(Dbg_Str, "BTST    D%.1d,%s%c", (OPC & 0xE00) >> 9, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 7), 0);
                        //break;

                    case 1:
                        //BCHG  Ds,a
                        return 2 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 7);
                        //sprintf(Dbg_Str, "BCHG    D%.1d,%s%c", (OPC & 0xE00) >> 9, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 7), 0);
                        //break;

                    case 2:
                        //BCLR  Ds,a
                        return 2 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 7);
                        //sprintf(Dbg_Str, "BCLR    D%.1d,%s%c", (OPC & 0xE00) >> 9, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 7), 0);
                        //break;

                    case 3:
                        //BSET  Ds,a
                        return 2 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 7);
                        //sprintf(Dbg_Str, "BSET    D%.1d,%s%c", (OPC & 0xE00) >> 9, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 7), 0);
                        //break;
                    }
                }
            }
            else
            {
                switch((OPC >> 6) & 0x3F)
                {
                case 0:
                    if ((OPC & 0xff) == 0x3c)
                        //ORI.B  #k,CCR
                        return 4;
                    else
                        //ORI.B  #k,a
                        return 4 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFF;
                    //sprintf(Dbg_Str, "ORI.B   #$%.2X,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 1:
                    if ((OPC & 0xff) == 0x7c)
                        //ORI.W  #k,SR
                        return 4;
                    else
                        //ORI.W  #k,a
                        return 4 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFFFF;
                    //sprintf(Dbg_Str, "ORI.W   #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 2:
                    //ORI.L  #k,a
                    return 6 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Long();
                    //sprintf(Dbg_Str, "ORI.L   #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 8:
                    //ANDI.B  #k,a
                    if ((OPC & 0xff) == 0x3c)
                        return 4;
                    return 4 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFF;
                    //sprintf(Dbg_Str, "ANDI.B  #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 9:
                    //ANDI.W  #k,a
                    if ((OPC & 0xff) == 0x7c)
                        return 4;

                    return 4 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFFFF;
                    //sprintf(Dbg_Str, "ANDI.W  #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 10:
                    //ANDI.L  #k,a
                    return 6 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Long();
                    //sprintf(Dbg_Str, "ANDI.L  #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 16:
                    //SUBI.B  #k,a
                    return 4 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFF;
                    //sprintf(Dbg_Str, "SUBI.B  #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 17:
                    //SUBI.W  #k,a
                    return 4 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFFFF;
                    //sprintf(Dbg_Str, "SUBI.W  #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 18:
                    //SUBI.L  #k,a
                    return 6 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Long();
                    //sprintf(Dbg_Str, "SUBI.L  #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 24:
                    //ADDI.B  #k,a
                    return 4 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFF;
                    //sprintf(Dbg_Str, "ADDI.B  #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 25:
                    //ADDI.W  #k,a
                    return 4 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFFFF;
                    //sprintf(Dbg_Str, "ADDI.W  #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 26:
                    //ADDI.L  #k,a
                    return 6 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Long();
                    //sprintf(Dbg_Str, "ADDI.L  #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 32:
                    //BTST  #n,a
                    return 4 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFF;
                    //sprintf(Dbg_Str, "BTST    #%d,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 33:
                    //BCHG  #n,a
                    return 4 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFF;
                    //sprintf(Dbg_Str, "BCHG    #%d,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 34:
                    //BCLR  #n,a
                    return 4 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFF;
                    //sprintf(Dbg_Str, "BCLR    #%d,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 35:
                    //BSET  #n,a
                    return 4 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFF;
                    //sprintf(Dbg_Str, "BSET    #%d,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 40:
                    //EORI.B  #k,a
                    return 4 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFF;
                    //sprintf(Dbg_Str, "EORI.B  #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 41:
                    //EORI.W  #k,a
                    return 4 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFFFF;
                    //sprintf(Dbg_Str, "EORI.W  #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 42:
                    //EORI.L  #k,a
                    return 6 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Long();
                    //sprintf(Dbg_Str, "EORI.L  #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 48:
                    //CMPI.B  #k,a
                    return 4 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFF;
                    //sprintf(Dbg_Str, "CMPI.B  #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 49:
                    //CMPI.W  #k,a
                    return 4 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Word() & 0xFFFF;
                    //sprintf(Dbg_Str, "CMPI.W  #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 50:
                    //CMPI.L  #k,a
                    return 6 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 0x7);
                    //i = Next_Long();
                    //sprintf(Dbg_Str, "CMPI.L  #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;
                }
            }
            break;

        case 1:
            //MOVE.b  as,ad
            return 2 + GetAdressingDataSize(0, (OPC >> 3) & 0x7, OPC & 0x7) + GetAdressingDataSize(0, (OPC >> 6) & 0x7, (OPC >> 9) & 0x7);
            //sprintf(Tmp_Str, "%s%c", Make_Dbg_EA_Str(0, (OPC >> 3) & 0x7, OPC & 0x7), 0);
            //sprintf(Dbg_Str, "MOVE.b  %s,%s%c", Tmp_Str, Make_Dbg_EA_Str(0, (OPC >> 6) & 0x7, (OPC >> 9) & 0x7), 0);
            //break;

        case 2:
            //MOVE.l  as,ad
            return 2 + GetAdressingDataSize(2, (OPC >> 3) & 0x7, OPC & 0x7) + GetAdressingDataSize(2, (OPC >> 6) & 0x7, (OPC >> 9) & 0x7);
            //sprintf(Tmp_Str, "%s%c", Make_Dbg_EA_Str(2, (OPC >> 3) & 0x7, OPC & 0x7), 0);
            //sprintf(Dbg_Str, "MOVE.l  %s,%s%c", Tmp_Str, Make_Dbg_EA_Str(2, (OPC >> 6) & 0x7, (OPC >> 9) & 0x7), 0);
            //break;

        case 3:
            //MOVE.w  as,ad
            return 2 + GetAdressingDataSize(1, (OPC >> 3) & 0x7, OPC & 0x7) + GetAdressingDataSize(1, (OPC >> 6) & 0x7, (OPC >> 9) & 0x7);
            //sprintf(Tmp_Str, "%s%c", Make_Dbg_EA_Str(1, (OPC >> 3) & 0x7, OPC & 0x7), 0);
            //sprintf(Dbg_Str, "MOVE.w  %s,%s%c", Tmp_Str, Make_Dbg_EA_Str(1, (OPC >> 6) & 0x7, (OPC >> 9) & 0x7), 0);
            //break;

        case 4:
            //SPECIALS ...

            if (OPC & 0x100)
            {
                if (OPC & 0x40)
                    //LEA  a,Ad
                    return 2 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "LEA     %s,A%.1d%c", Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                else
                    //CHK.W  a,Dd
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "CHK.W   %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
            }
            else
            {
                switch((OPC >> 6) & 0x3F)
                {
                case 0:	case 1: case 2:
                    //NEGX.z  a
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "NEGX%-4s%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 3:
                    //MOVE  SR,a
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "MOVE    SR,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 8: case 9: case 10:
                    //CLR.z  a
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "CLR%-5s%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 16: case 17: case 18:
                    //NEG.z  a
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "NEG%-5s%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 19:
                    //MOVE  a,CCR
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "MOVE    %s,CCR%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 24: case 25: case 26:
                    //NOT.z  a
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "NOT%-4s%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 27:
                    //MOVE  a,SR
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "MOVE    %s,SR%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 32:
                    //NBCD  a
                    return 2 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "NBCD    %s%c", Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 33:

                    if (OPC & 0x38)
                        //PEA  a
                        return 2 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "PEA     %s%c", Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    else
                        //SWAP.w  Dd
                        return 2;
                    //sprintf(Dbg_Str, "SWAP.w  D%d%c", OPC & 0x7, 0);

                    break;

                case 34: case 35:

                    if (OPC & 0x38)
                    {
                        //MOVEM.z Reg-List,a
                        return 4 + GetAdressingDataSize((OPC >> 6) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7);
                        //sprintf(Dbg_Str, "MOVEM%-3sReg-List,%s%c", Make_Dbg_Size_Str_2((OPC >> 6) & 1), Make_Dbg_EA_Str((OPC >> 6) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        //Next_Word();
                    }
                    else
                        //EXT.z  Dd
                        return 2 + GetAdressingDataSize((OPC >> 6) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "EXT%-5s%s%c", Make_Dbg_Size_Str_2((OPC >> 6) & 1), Make_Dbg_EA_Str((OPC >> 6) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), 0);

                    break;

                case 40: case 41: case 42:
                    //TST.z a
                    return 2 + GetAdressingDataSize((OPC >> 6) & 0x3, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "TST%-5s%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 0x3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 43:
                    //TAS.b a
                    return 2 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "TAS.B %s%c", Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 48: case 49:
                    //Bad Opcode
                    return 0;
                    //sprintf(Dbg_Str, "Bad Opcode%c", 0);
                    //break;

                case 50: case 51:
                    //MOVEM.z a,Reg-List
                    return 4 + GetAdressingDataSize((OPC >> 6) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "MOVEM%-3s%s,Reg-List%c", Make_Dbg_Size_Str_2((OPC >> 6) & 1), Make_Dbg_EA_Str((OPC >> 6) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //Next_Word();
                    //break;

                case 57:

                    switch((OPC >> 3) & 0x7)
                    {
                    case 0: case 1:
                        //TRAP  #vector
                        return 2;
                        //sprintf(Dbg_Str, "TRAP    #$%.1X%c", OPC & 0xF, 0);
                        //break;

                    case 2:
                        //LINK As,#k16
                        return 4;
                        //sprintf(Dbg_Str, "LINK    A%.1d,#$%.4X%c", OPC & 0x7, Next_Word(), 0);
                        //break;

                    case 3:
                        //ULNK Ad
                        return 2;
                        //sprintf(Dbg_Str, "ULNK    A%.1d%c", OPC & 0x7, 0);
                        //break;

                    case 4:
                        //MOVE As,USP
                        return 2;
                        //sprintf(Dbg_Str, "MOVE    A%.1d,USP%c",OPC & 0x7, 0);
                        //break;

                    case 5:
                        //MOVE USP,Ad
                        return 2;
                        //sprintf(Dbg_Str, "MOVE    USP,A%.1d%c",OPC & 0x7, 0);
                        //break;

                    case 6:

                        switch(OPC & 0x7)
                        {
                        case 0:
                            //RESET
                            return 2;
                            //sprintf(Dbg_Str, "RESET%c", 0);
                            //break;

                        case 1:
                            //NOP
                            return 2;
                            //sprintf(Dbg_Str, "NOP%c", 0);
                            //break;

                        case 2:
                            //STOP #k16
                            return 4;
                            //sprintf(Dbg_Str, "STOP    #$%.4X%c", Next_Word(), 0);
                            //break;

                        case 3:
                            //RTE
                            return 2;
                            //sprintf(Dbg_Str, "RTE%c", 0);
                            //break;

                        case 4:
                            //Bad Opcode
                            return 0;
                            //sprintf(Dbg_Str, "Bad Opcode%c", 0);
                            //break;

                        case 5:
                            //RTS
                            return 2;
                            //sprintf(Dbg_Str, "RTS%c", 0);
                            //break;

                        case 6:
                            //TRAPV
                            return 2;
                            //sprintf(Dbg_Str, "TRAPV%c", 0);
                            //break;

                        case 7:
                            //RTR
                            return 2;
                            //sprintf(Dbg_Str, "RTR%c", 0);
                            //break;
                        }
                        break;
                    }
                    break;

                case 58:
                    //JSR  a
                    return 2 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "JSR     %s%c", Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 59:
                    //JMP  a
                    return 2 + GetAdressingDataSize(2, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "JMP     %s%c", Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;
                }
            }
            break;

        case 5:

            if ((OPC & 0xC0) == 0xC0)
            {
                if ((OPC & 0x38) == 0x08)
                    //DBCC  Ds,label
                    return 4;
                //sprintf(Dbg_Str, "DB%-6sD%.1d,#$%.4X%c", Make_Dbg_Cond_Str((OPC >> 8) & 0xF), OPC & 0x7, Next_Word(), 0);
                else
                    //STCC.b  a
                    return 2 + GetAdressingDataSize(0, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "ST%-6s%s%c", Make_Dbg_Cond_Str((OPC >> 8) & 0xF), Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                //break;
            }
            else
            {
                if (OPC & 0x100)
                    //SUBQ.z  #k3,a
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "SUBQ%-4s#%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                else
                    //ADDQ.z  #k3,a
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "ADDQ%-4s#%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                //break;
            }
            break;

        case 6:

            if (OPC & 0xFF)
            {
                if ((OPC & 0xF00) == 0x100)
                {
                    //BSR  label
                    return 2;
                    //sprintf(Dbg_Str, "BSR     #$%.2X%c", OPC & 0xFF, 0);
                    //break;
                }

                if (!(OPC & 0xF00))
                {
                    //BRA  label
                    return 2;
                    //sprintf(Dbg_Str, "BRA     #$%.2X%c", OPC & 0xFF, 0);
                    //break;
                }

                //BCC  label
                //sprintf(Dbg_Str, "B%-7s#$%.2X%c", Make_Dbg_Cond_Str((OPC >> 8) & 0xF), OPC & 0xFF, 0);
                return 2;
            }
            else
            {
                if ((OPC & 0xF00) == 0x100)
                {
                    //BSR  label
                    return 4;
                    //sprintf(Dbg_Str, "BSR     #$%.4X%c", Next_Word(), 0);
                    //break;
                }

                if (!(OPC & 0xF00))
                {
                    //BRA  label
                    return 4;
                    //sprintf(Dbg_Str, "BRA     #$%.4X%c", Next_Word(), 0);
                    //break;
                }

                //BCC  label
                //sprintf(Dbg_Str, "B%-7s#$%.4X%c", Make_Dbg_Cond_Str((OPC >> 8 ) & 0xF), Next_Word(), 0);

                return 4;
            }
            //break;

        case 7:
            //MOVEQ  #k8,Dd
            return 2;
            //sprintf(Dbg_Str, "MOVEQ   #$%.2X,D%.1d%c", OPC & 0xFF, (OPC >> 9) & 0x7, 0);
            //break;

        case 8:

            if (OPC & 0x100)
            {
                if (!(OPC & 0xF8))
                {
                    //SBCD  Ds,Dd
                    return 2;
                    //sprintf(Dbg_Str, "SBCD D%.1d,D%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                    //break;
                }

                if ((OPC & 0xF8) == 0x8)
                {
                    //SBCD  -(As),-(Ad)
                    return 2;
                    //sprintf(Dbg_Str, "SBCD -(A%.1d),-(A%.1d)%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                    //break;
                }

                if ((OPC & 0xC0) == 0xC0)
                    //DIVS.w  a,Dd
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "DIVS.W  %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                else
                    //OR.z  Ds,a
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "OR%-6sD%.1d;%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
            }
            else
            {
                if ((OPC & 0xC0) == 0xC0)
                    //DIVU.w  a,Dd
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "DIVU.W  %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                else
                    //OR.z  a,Dd
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "OR%-6s%s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
            }
            break;

        case 9:

            if ((OPC & 0xC0) == 0xC0)
                //SUBA.z  a,Ad
                return 2 + GetAdressingDataSize((OPC >> 8) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7);
            //sprintf(Dbg_Str, "SUBA%-4s%s,A%.1d%c", Make_Dbg_Size_Str_2((OPC >> 8) & 1), Make_Dbg_EA_Str((OPC >> 8) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
            else
            {
                if (OPC & 0x100)
                {
                    if (!(OPC & 0x38))
                    {
                        //SUBX.z  Ds,Dd
                        return 2;
                        //sprintf(Dbg_Str, "SUBX%-4sD%.1d,D%.1d%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                        //break;
                    }

                    if ((OPC & 0x38) == 0x8)
                    {
                        //SUBX.z  -(As),-(Ad)
                        return 2;
                        //sprintf(Dbg_Str, "SUBX%-4s-(A%.1d),-(A%.1d)%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                        //break;
                    }

                    //SUB.z  Ds,a
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "SUB%-5sD%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                }
                else
                    //SUB.z  a,Dd
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "SUB%-5s%s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
            }
            break;

        case 11:

            if ((OPC & 0xC0) == 0xC0)
                //CMPA.z  a,Ad
                return 2 + GetAdressingDataSize((OPC >> 7) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7);
            //sprintf(Dbg_Str, "CMPA%-4s%s,A%.1d%c", Make_Dbg_Size_Str_2((OPC >> 8) & 1), Make_Dbg_EA_Str((OPC >> 7) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
            else
            {
                if (OPC & 0x100)
                {
                    if ((OPC & 0x38) == 0x8)
                    {
                        //CMPM.z  (As)+,(Ad)+
                        return 2;
                        //sprintf(Dbg_Str, "CMPM%-4s(A%.1d)+,(A%.1d)+%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                        //break;
                    }

                    //EOR.z  Ds,a
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "EOR%-5sD%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                }
                else
                    //CMP.z  a,Dd
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "CMP%-5s%s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
            }
            break;

        case 12:

            if ((OPC & 0X1F8) == 0x100)
            {
                //ABCD Ds,Dd
                return 2;
                //sprintf(Dbg_Str, "ABCD    D%.1d,D%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                //break;
            }

            if ((OPC & 0X1F8) == 0x140)
            {
                //EXG.l Ds,Dd
                return 2;
                //sprintf(Dbg_Str, "EXG.L   D%.1d,D%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                //break;
            }

            if ((OPC & 0X1F8) == 0x108)
            {
                //ABCD -(As),-(Ad)
                return 2;
                //sprintf(Dbg_Str, "ABCD    -(A%.1d),-(A%.1d)%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                //break;
            }

            if ((OPC & 0X1F8) == 0x148)
            {
                //EXG.l As,Ad
                return 2;
                //sprintf(Dbg_Str, "EXG.L   A%.1d,A%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                //break;
            }

            if ((OPC & 0X1F8) == 0x188)
            {
                //EXG.l As,Dd
                return 2;
                //sprintf(Dbg_Str, "EXG.L   A%.1d,D%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                //break;
            }

            switch((OPC	>> 6) & 0x7)
            {
            case 0: case 1: case 2:
                //AND.z  a,Dd
                return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "AND%-5s%s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                //break;

            case 3:
                //MULU.w  a,Dd
                return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "MULU.W  %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                //break;

            case 4: case 5: case 6:
                //AND.z  Ds,a
                return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "AND%-5sD%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                //break;

            case 7:
                //MULS.w  a,Dd
                return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "MULS.W  %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                //break;
            }
            break;

        case 13:

            if ((OPC & 0xC0) == 0xC0)
                //ADDA.z  a,Ad
                return 2 + GetAdressingDataSize((OPC >> 8) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7);
            //sprintf(Dbg_Str, "ADDA%-4s%s,A%.1d%c", Make_Dbg_Size_Str_2((OPC >> 8) & 1), Make_Dbg_EA_Str((OPC >> 8) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
            else
            {
                if (OPC & 0x100)
                {
                    if (!(OPC & 0x38))
                    {
                        //ADDX.z  Ds,Dd
                        return 2;
                        //sprintf(Dbg_Str, "ADDX%-4sD%.1d,D%.1d%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                        //break;
                    }

                    if ((OPC & 0x38) == 0x8)
                    {
                        //ADDX.z  -(As),-(Ad)
                        return 2;
                        //sprintf(Dbg_Str, "ADDX%-4s-(A%.1d),-(A%.1d)%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                        //break;
                    }

                    //ADD.z  Ds,a
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "ADD%-5sD%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                }
                else
                    //ADD.z  a,Dd
                    return 2 + GetAdressingDataSize((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7);
                //sprintf(Dbg_Str, "ADD%-5s%s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
            }
            break;

        case 14:

            if ((OPC & 0xC0) == 0xC0)
            {
                switch ((OPC >> 8) & 0x7)
                {
                case 0:
                    //ASR.w  #1,a
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "ASR.W   #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 1:
                    //ASL.w  #1,a
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "ASL.W   #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 2:
                    //LSR.w  #1,a
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "LSR.W   #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 3:
                    //LSL.w  #1,a
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "LSL.W   #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 4:
                    //ROXR.w  #1,a
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "ROXR.W  #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 5:
                    //ROXL.w  #1,a
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "ROXL.W  #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 6:
                    //ROR.w  #1,a
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "ROR.W   #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                case 7:
                    //ROL.w  #1,a
                    return 2 + GetAdressingDataSize(1, (OPC & 0x38) >> 3, OPC & 0x7);
                    //sprintf(Dbg_Str, "ROL.W   #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    //break;

                }
            }
            else
            {
                switch ((OPC >> 3) & 0x3F)
                {
                case 0: case 8: case 16:
                    //ASR.z  #k,Dd
                    return 2;
                    //sprintf(Dbg_Str, "ASR%-5s#%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 1: case 9: case 17:
                    //LSR.z  #k,Dd
                    return 2;
                    //sprintf(Dbg_Str, "LSR%-5s#%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 2: case 10: case 18:
                    //ROXR.z  #k,Dd
                    return 2;
                    //sprintf(Dbg_Str, "ROXR%-4s#%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 3: case 11: case 19:
                    //ROR.z  #k,Dd
                    return 2;
                    //sprintf(Dbg_Str, "ROR%-5s#%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 4: case 12: case 20:
                    //ASR.z  Ds,Dd
                    return 2;
                    //sprintf(Dbg_Str, "ASR%-5sD%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 5: case 13: case 21:
                    //LSR.z  Ds,Dd
                    return 2;
                    //sprintf(Dbg_Str, "LSR%-5sD%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 6: case 14: case 22:
                    //ROXR.z  Ds,Dd
                    return 2;
                    //sprintf(Dbg_Str, "ROXR%-4sD%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 7: case 15: case 23:
                    //ROR.z  Ds,Dd
                    return 2;
                    //sprintf(Dbg_Str, "ROR%-5sD%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 32: case 40: case 48:
                    //ASL.z  #k,Dd
                    return 2;
                    //sprintf(Dbg_Str, "ASL%-5s#%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 33: case 41: case 49:
                    //LSL.z  #k,Dd
                    return 2;
                    //sprintf(Dbg_Str, "LSL%-5s#%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 34: case 42: case 50:
                    //ROXL.z  #k,Dd
                    return 2;
                    //sprintf(Dbg_Str, "ROXL%-4s#%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 35: case 43: case 51:
                    //ROL.z  #k,Dd
                    return 2;
                    //sprintf(Dbg_Str, "ROL%-5s#%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 36: case 44: case 52:
                    //ASL.z  Ds,Dd
                    return 2;
                    //sprintf(Dbg_Str, "ASL%-5sD%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 37: case 45: case 53:
                    //LSL.z  Ds,Dd
                    return 2;
                    //sprintf(Dbg_Str, "LSL%-5sD%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 38: case 46: case 54:
                    //ROXL.z  Ds,Dd
                    return 2;
                    //sprintf(Dbg_Str, "ROXL%-4sD%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                case 39: case 47: case 55:
                    //ROL.z  Ds,Dd
                    return 2;
                    //sprintf(Dbg_Str, "ROL%-5sD%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                    //break;

                }
            }
            //break;
        }

        return 0;
    }

    namespace Normal
    {
        static char Dbg_Str[256];
        static char Dbg_EA_Str[16];
        static char Dbg_Size_Str[3];
        static char Dbg_Cond_Str[3];
        static char Dbg_RegList_Str[49];

        static uint32 Position;

        static unsigned short (*Next_Word)();
        static unsigned int (*Next_Long)();

        char *Make_Dbg_RegList_Str(int RegList, int  Flip)
        {
            int Bit;
            int Index = 0;
            int Empty = 1;

            for (Bit = 0; Bit < 8; ++Bit)
            {
                if (RegList & 0x1)
                {
                    if (Empty)
                        Empty = 0;
                    else
                        Dbg_RegList_Str[Index++] = '/';

                    Dbg_RegList_Str[Index++] = Flip ? 'D' : 'A';
                    Dbg_RegList_Str[Index++] = Flip ? '0' + Bit : '0' + (7 - Bit);
                }

                RegList >>= 1;
            }

            for (Bit = 0; Bit < 8; ++Bit)
            {
                if (RegList & 0x1)
                {
                    if (Empty)
                        Empty = 0;
                    else
                        Dbg_RegList_Str[Index++] = '/';

                    Dbg_RegList_Str[Index++] = Flip ? 'A' : 'D';
                    Dbg_RegList_Str[Index++] = Flip ? '0' + Bit : '0' + (7 - Bit);
                }

                RegList >>= 1;
            }

            Dbg_RegList_Str[Index++] = '\0';

            return Dbg_RegList_Str;
        }

        char *Make_Dbg_EA_Str(int Size, int EA_Num, int Reg_Num)
        {
            int i;
            Dbg_EA_Str[15] = 0;

            switch(EA_Num)
            {
            case 0:
                // 000 rrr  Dr
                sprintf(Dbg_EA_Str, "D%.1d%c", Reg_Num, 0);
                break;

            case 1:
                // 001 rrr  Ar
                sprintf(Dbg_EA_Str, "A%.1d%c", Reg_Num, 0);
                break;

            case 2:
                // 010 rrr  (Ar)
                sprintf(Dbg_EA_Str, "(A%.1d)%c", Reg_Num, 0);
                break;

            case 3:
                // 011 rrr  (Ar)+
                sprintf(Dbg_EA_Str, "(A%.1d)+%c", Reg_Num, 0);
                break;

            case 4:
                // 100 rrr  -(Ar)
                sprintf(Dbg_EA_Str, "-(A%.1d)%c", Reg_Num, 0);
                break;

            case 5:
                // 101 rrr  d16(Ar)     dddddddd dddddddd
                sprintf(Dbg_EA_Str, "$%.4X(A%.1d)%c", Next_Word(), Reg_Num, 0);
                break;

            case 6:
                // 110 rrr  d8(Ar,ix)   aiiizcc0 dddddddd
                i = Next_Word() & 0xFFFF;
                if (i & 0x8000)
                    sprintf(Dbg_EA_Str, "$%.2X(A%.1d,A%.1d)%c", i & 0xFF, Reg_Num, (i >> 12) & 0x7, 0);
                else
                    sprintf(Dbg_EA_Str, "$%.2X(A%.1d,D%.1d)%c", i & 0xFF, Reg_Num, (i >> 12) & 0x7, 0);
                break;

            case 7:
                switch(Reg_Num)
                {
                case 0:
                    // 111 000  addr16      dddddddd dddddddd
                    {
                        uint32 uiPos = (Position & 0xffff0000) | Next_Word();
                        const char* szLabel = GetLabel((gCurrentCore == &m68k ? GetM68000MemMap() : GetS68000MemMap()), uiPos);

                        if (szLabel)
                            sprintf(Dbg_EA_Str, "(~~%08X~~).w", uiPos);
                        else
                            sprintf(Dbg_EA_Str, "($%04X).w", uiPos & 0xffff);
                        break;
                    }
                case 1:
                    // 111 001  addr32      dddddddd dddddddd ddddddddd dddddddd
                    {
                        uint32 uiPos = Next_Long();
                        const char* szLabel = GetLabel((gCurrentCore == &m68k ? GetM68000MemMap() : GetS68000MemMap()), uiPos);

                        if (szLabel)
                            sprintf(Dbg_EA_Str, "(~~%08X~~).l", uiPos);
                        else
                            sprintf(Dbg_EA_Str, "($%08X).l", uiPos);
                        break;
                    }
                case 2:
                    // 111 010  d16(PC)     dddddddd dddddddd
                    sprintf(Dbg_EA_Str, "$%.4X(PC)%c", Next_Word(), 0);
                    break;

                case 3:
                    // 111 011  d8(PC,ix)   aiiiz000 dddddddd
                    i = Next_Word() & 0xFFFF;
                    if (i & 0x8000)
                        sprintf(Dbg_EA_Str, "$%.2X(PC,A%.1d)%c", i & 0xFF, (i >> 12) & 0x7, 0);
                    else
                        sprintf(Dbg_EA_Str, "$%.2X(PC,D%.1d)%c", i & 0xFF, (i >> 12) & 0x7, 0);
                    break;

                case 4:
                    // 111 100  imm/implied
                    switch(Size)
                    {
                    case 0:
                        sprintf(Dbg_EA_Str, "#$%.2X%c", Next_Word() & 0xFF, 0);
                        break;

                    case 1:
                        sprintf(Dbg_EA_Str, "#$%.4X%c", Next_Word(), 0);
                        break;

                    case 2:
                        sprintf(Dbg_EA_Str, "#$%.8X%c", Next_Long(), 0);
                        break;
                    }
                    break;
                }
                break;
            }

            return(Dbg_EA_Str);
        }


        char *Make_Dbg_Size_Str(int Size)
        {
            Dbg_Size_Str[2] = 0;
            sprintf(Dbg_Size_Str, ".?");

            switch(Size)
            {
            case 0:
                sprintf(Dbg_Size_Str, ".B");
                break;

            case 1:
                sprintf(Dbg_Size_Str, ".W");
                break;

            case 2:
                sprintf(Dbg_Size_Str, ".L");
                break;
            }

            return(Dbg_Size_Str);
        }


        char *Make_Dbg_Size_Str_2(int Size)
        {
            Dbg_Size_Str[2] = 0;
            sprintf(Dbg_Size_Str, ".?");

            switch(Size)
            {
            case 0:
                sprintf(Dbg_Size_Str, ".W");
                break;

            case 1:
                sprintf(Dbg_Size_Str, ".L");
                break;
            }

            return(Dbg_Size_Str);
        }

        char *Make_Dbg_Cond_Str(int Cond)
        {
            Dbg_Cond_Str[2] = 0;
            sprintf(Dbg_Cond_Str, "??");

            switch(Cond)
            {
            case 0:
                sprintf(Dbg_Cond_Str, "Tr");
                break;

            case 1:
                sprintf(Dbg_Cond_Str, "Fa");
                break;

            case 2:
                sprintf(Dbg_Cond_Str, "HI");
                break;

            case 3:
                sprintf(Dbg_Cond_Str, "LS");
                break;

            case 4:
                sprintf(Dbg_Cond_Str, "CC");
                break;

            case 5:
                sprintf(Dbg_Cond_Str, "CS");
                break;

            case 6:
                sprintf(Dbg_Cond_Str, "NE");
                break;

            case 7:
                sprintf(Dbg_Cond_Str, "EQ");
                break;

            case 8:
                sprintf(Dbg_Cond_Str, "VC");
                break;

            case 9:
                sprintf(Dbg_Cond_Str, "VS");
                break;

            case 10:
                sprintf(Dbg_Cond_Str, "PL");
                break;

            case 11:
                sprintf(Dbg_Cond_Str, "MI");
                break;

            case 12:
                sprintf(Dbg_Cond_Str, "GE");
                break;

            case 13:
                sprintf(Dbg_Cond_Str, "LT");
                break;

            case 14:
                sprintf(Dbg_Cond_Str, "GT");
                break;

            case 15:
                sprintf(Dbg_Cond_Str, "LE");
                break;
            }

            return(Dbg_Cond_Str);
        }

        char *M68KDisasm(unsigned short (*NW)(), unsigned int (*NL)(), unsigned int _uiPosition)
        {
            int i;
            unsigned short OPC;
            char Tmp_Str[128] = {0};

            Position =_uiPosition;
            Next_Word = NW;
            Next_Long = NL;

            OPC = Next_Word();

            sprintf(Dbg_Str, "Unknow Opcode%c", 0);

            switch(OPC >> 12)
            {
            case 0:

                if (OPC & 0x100)
                {
                    if ((OPC & 0x038) == 0x8)
                    {
                        if (OPC & 0x080)
                            //MOVEP.z Ds,d16(Ad)
                            sprintf(Dbg_Str, "MOVEP%s D%.1d,#$%.4X(A%.1d)%c", Make_Dbg_Size_Str_2((OPC & 0x40) >> 6), (OPC & 0xE00) >> 9, Next_Word(), OPC & 0x7, 0);
                        else
                            //MOVEP.z d16(As),Dd
                            sprintf(Dbg_Str, "MOVEP%s #$%.4X(A%.1d),D%.1d%c", Make_Dbg_Size_Str_2((OPC & 0x40) >> 6), Next_Word(), OPC & 0x7, (OPC & 0xE00) >> 9, 0);
                    }
                    else
                    {
                        switch((OPC >> 6) & 0x3)
                        {
                        case 0:
                            //BTST  Ds,a
                            sprintf(Dbg_Str, "BTST D%.1d,%s%c", (OPC & 0xE00) >> 9, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 7), 0);
                            break;

                        case 1:
                            //BCHG  Ds,a
                            sprintf(Dbg_Str, "BCHG D%.1d,%s%c", (OPC & 0xE00) >> 9, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 7), 0);
                            break;

                        case 2:
                            //BCLR  Ds,a
                            sprintf(Dbg_Str, "BCLR D%.1d,%s%c", (OPC & 0xE00) >> 9, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 7), 0);
                            break;

                        case 3:
                            //BSET  Ds,a
                            sprintf(Dbg_Str, "BSET D%.1d,%s%c", (OPC & 0xE00) >> 9, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 7), 0);
                            break;
                        }
                    }
                }
                else
                {
                    switch((OPC >> 6) & 0x3F)
                    {
                    case 0:
                        if ((OPC & 0xff) == 0x3c)
                        {
                            i = Next_Word() & 0xFF;
                            sprintf(Dbg_Str, "ORI.B #$%.2X,CCR", i);
                        }
                        else
                        {
                            i = Next_Word() & 0xFF;
                            sprintf(Dbg_Str, "ORI.B #$%.2X,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        }
                        break;

                    case 1:
                        //ORI.W  #k,a
                        if ((OPC & 0xff) == 0x7c)
                        {
                            i = Next_Word() & 0xFFFF;
                            sprintf(Dbg_Str, "ORI.B #$%.2X,SR", i);
                        }
                        else
                        {
                            i = Next_Word() & 0xFFFF;
                            sprintf(Dbg_Str, "ORI.W #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        }
                        break;

                    case 2:
                        //ORI.L  #k,a
                        i = Next_Long();
                        sprintf(Dbg_Str, "ORI.L #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 8:
                        //ANDI.B  #k,a
                        if ((OPC & 0xff) == 0x3c)
                        {
                            i = Next_Word() & 0xFF;
                            sprintf(Dbg_Str, "ANDI.B #$%.2X,CCR", i);

                        }
                        else
                        {
                            i = Next_Word() & 0xFF;
                            sprintf(Dbg_Str, "ANDI.B #$%.2X,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        }
                        break;

                    case 9:
                        //ANDI.W  #k,a
                        if ((OPC & 0xff) == 0x7c)
                        {
                            i = Next_Word() & 0xFFFF;
                            sprintf(Dbg_Str, "ANDI.W #$%.4X,SR", i);
                        }
                        else
                        {
                            i = Next_Word() & 0xFFFF;
                            sprintf(Dbg_Str, "ANDI.W #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        }
                        break;

                    case 10:
                        //ANDI.L  #k,a
                        i = Next_Long();
                        sprintf(Dbg_Str, "ANDI.L #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 16:
                        //SUBI.B  #k,a
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "SUBI.B #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 17:
                        //SUBI.W  #k,a
                        i = Next_Word() & 0xFFFF;
                        sprintf(Dbg_Str, "SUBI.W #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 18:
                        //SUBI.L  #k,a
                        i = Next_Long();
                        sprintf(Dbg_Str, "SUBI.L #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 24:
                        //ADDI.B  #k,a
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "ADDI.B #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 25:
                        //ADDI.W  #k,a
                        i = Next_Word() & 0xFFFF;
                        sprintf(Dbg_Str, "ADDI.W #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 26:
                        //ADDI.L  #k,a
                        i = Next_Long();
                        sprintf(Dbg_Str, "ADDI.L #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 32:
                        //BTST  #n,a
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "BTST #%d,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 33:
                        //BCHG  #n,a
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "BCHG #%d,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 34:
                        //BCLR  #n,a
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "BCLR #%d,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 35:
                        //BSET  #n,a
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "BSET #%d,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 40:
                        //EORI.B  #k,a
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "EORI.B #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 41:
                        //EORI.W  #k,a
                        i = Next_Word() & 0xFFFF;
                        sprintf(Dbg_Str, "EORI.W #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 42:
                        //EORI.L  #k,a
                        i = Next_Long();
                        sprintf(Dbg_Str, "EORI.L #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 48:
                        //CMPI.B  #k,a
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "CMPI.B #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 49:
                        //CMPI.W  #k,a
                        i = Next_Word() & 0xFFFF;
                        sprintf(Dbg_Str, "CMPI.W #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 50:
                        //CMPI.L  #k,a
                        i = Next_Long();
                        sprintf(Dbg_Str, "CMPI.L #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;
                    }
                }
                break;

            case 1:
                //MOVE.b  as,ad
                sprintf(Tmp_Str, "%s%c", Make_Dbg_EA_Str(0, (OPC >> 3) & 0x7, OPC & 0x7), 0);
                sprintf(Dbg_Str, "MOVE.b %s,%s%c", Tmp_Str, Make_Dbg_EA_Str(0, (OPC >> 6) & 0x7, (OPC >> 9) & 0x7), 0);
                break;

            case 2:
                //MOVE.l  as,ad
                sprintf(Tmp_Str, "%s%c", Make_Dbg_EA_Str(2, (OPC >> 3) & 0x7, OPC & 0x7), 0);
                sprintf(Dbg_Str, "MOVE.l %s,%s%c", Tmp_Str, Make_Dbg_EA_Str(2, (OPC >> 6) & 0x7, (OPC >> 9) & 0x7), 0);
                break;

            case 3:
                //MOVE.w  as,ad
                sprintf(Tmp_Str, "%s%c", Make_Dbg_EA_Str(1, (OPC >> 3) & 0x7, OPC & 0x7), 0);
                sprintf(Dbg_Str, "MOVE.w %s,%s%c", Tmp_Str, Make_Dbg_EA_Str(1, (OPC >> 6) & 0x7, (OPC >> 9) & 0x7), 0);
                break;

            case 4:
                //SPECIALS ...

                if (OPC & 0x100)
                {
                    if (OPC & 0x40)
                        //LEA  a,Ad
                        sprintf(Dbg_Str, "LEA %s,A%.1d%c", Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                    else
                        //CHK.W  a,Dd
                        sprintf(Dbg_Str, "CHK.W %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                }
                else
                {
                    switch((OPC >> 6) & 0x3F)
                    {
                    case 0:	case 1: case 2:
                        //NEGX.z  a
                        sprintf(Dbg_Str, "NEGX%s %s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 3:
                        //MOVE  SR,a
                        sprintf(Dbg_Str, "MOVE SR,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 8: case 9: case 10:
                        //CLR.z  a
                        sprintf(Dbg_Str, "CLR%s %s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 16: case 17: case 18:
                        //NEG.z  a
                        sprintf(Dbg_Str, "NEG%s %s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 19:
                        //MOVE  a,CCR
                        sprintf(Dbg_Str, "MOVE %s,CCR%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 24: case 25: case 26:
                        //NOT.z  a
                        sprintf(Dbg_Str, "NOT%s %s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 27:
                        //MOVE  a,SR
                        sprintf(Dbg_Str, "MOVE %s,SR%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 32:
                        //NBCD  a
                        sprintf(Dbg_Str, "NBCD %s%c", Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 33:

                        if (OPC & 0x38)
                            //PEA  a
                            sprintf(Dbg_Str, "PEA %s%c", Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        else
                            //SWAP.w  Dd
                            sprintf(Dbg_Str, "SWAP.w  D%d%c", OPC & 0x7, 0);

                        break;

                    case 34: case 35:

                        if (OPC & 0x38)
                        {
                            //MOVEM.z Reg-List,a
                            sprintf(Dbg_Str, "MOVEM%s %s,%s%c", Make_Dbg_Size_Str_2((OPC >> 6) & 1), Make_Dbg_RegList_Str(Next_Word(), 0), Make_Dbg_EA_Str((OPC >> 6) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        }
                        else
                        {
                            //EXT.z  Dd
                            sprintf(Dbg_Str, "EXT%s %s%c", Make_Dbg_Size_Str_2((OPC >> 6) & 1), Make_Dbg_EA_Str((OPC >> 6) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        }

                        break;

                    case 40: case 41: case 42:
                        //TST.z a
                        sprintf(Dbg_Str, "TST%s %s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 0x3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 43:
                        //TAS.b a
                        sprintf(Dbg_Str, "TAS.B %s%c", Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 48: case 49:
                        //Bad Opcode
                        sprintf(Dbg_Str, "BadOpcode", 0);
                        break;

                    case 50: case 51:
                        //MOVEM.z a,Reg-List
                        sprintf(Dbg_Str, "MOVEM%s %s,%s%c", Make_Dbg_Size_Str_2((OPC >> 6) & 1), Make_Dbg_EA_Str((OPC >> 6) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), Make_Dbg_RegList_Str(Next_Word(), 1), 0);
                        break;
                    case 57:

                        switch((OPC >> 3) & 0x7)
                        {
                        case 0: case 1:
                            //TRAP  #vector
                            sprintf(Dbg_Str, "TRAP #$%.1X%c", OPC & 0xF, 0);
                            break;

                        case 2:
                            //LINK As,#k16
                            sprintf(Dbg_Str, "LINK A%.1d,#$%.4X%c", OPC & 0x7, Next_Word(), 0);
                            break;

                        case 3:
                            //ULNK Ad
                            sprintf(Dbg_Str, "ULNK A%.1d%c", OPC & 0x7, 0);
                            break;

                        case 4:
                            //MOVE As,USP
                            sprintf(Dbg_Str, "MOVE A%.1d,USP%c",OPC & 0x7, 0);
                            break;

                        case 5:
                            //MOVE USP,Ad
                            sprintf(Dbg_Str, "MOVE USP,A%.1d%c",OPC & 0x7, 0);
                            break;

                        case 6:

                            switch(OPC & 0x7)
                            {
                            case 0:
                                //RESET
                                sprintf(Dbg_Str, "RESET", 0);
                                break;

                            case 1:
                                //NOP
                                sprintf(Dbg_Str, "NOP", 0);
                                break;

                            case 2:
                                //STOP #k16
                                sprintf(Dbg_Str, "STOP #$%.4X%c", Next_Word(), 0);
                                break;

                            case 3:
                                //RTE
                                sprintf(Dbg_Str, "RTE", 0);
                                break;

                            case 4:
                                //Bad Opcode
                                sprintf(Dbg_Str, "BadOpcode", 0);
                                break;

                            case 5:
                                //RTS
                                sprintf(Dbg_Str, "RTS", 0);
                                break;

                            case 6:
                                //TRAPV
                                sprintf(Dbg_Str, "TRAPV", 0);
                                break;

                            case 7:
                                //RTR
                                sprintf(Dbg_Str, "RTR", 0);
                                break;
                            }
                            break;
                        }
                        break;

                    case 58:
                        //JSR  a
                        sprintf(Dbg_Str, "JSR %s%c", Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 59:
                        //JMP  a
                        sprintf(Dbg_Str, "JMP %s%c", Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;
                    }
                }
                break;

            case 5:

                if ((OPC & 0xC0) == 0xC0)
                {
                    if ((OPC & 0x38) == 0x08)
                    {
                        //DBCC  Ds,label
                        uint32 uiLabelPos = _uiPosition + 2 + ((signed short)Next_Word());
                        const char* szLabel = GetLabel((gCurrentCore == &m68k ? GetM68000MemMap() : GetS68000MemMap()), uiLabelPos);

                        if (szLabel)
                            sprintf(Dbg_Str, "DB%s D%.1d,(~~%08X~~).w", Make_Dbg_Cond_Str((OPC >> 8) & 0xF), OPC & 0x7, uiLabelPos);
                        else
                            sprintf(Dbg_Str, "DB%s D%.1d,($%08X).w", Make_Dbg_Cond_Str((OPC >> 8) & 0xF), OPC & 0x7, uiLabelPos);
                    }
                    else
                    {
                        //STCC.b  a
                        sprintf(Dbg_Str, "ST%s %s%c", Make_Dbg_Cond_Str((OPC >> 8) & 0xF), Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    }
                    break;
                }
                else
                {
                    if (OPC & 0x100)
                        //SUBQ.z  #k3,a
                        sprintf(Dbg_Str, "SUBQ%s #%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    else
                        //ADDQ.z  #k3,a
                        sprintf(Dbg_Str, "ADDQ%s #%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    break;
                }
                break;

            case 6:

                if (OPC & 0xFF)
                {
                    uint32 uiLabelPos = _uiPosition + 2 + ((signed char)(OPC & 0xFF));
                    const char* szLabel = GetLabel((gCurrentCore == &m68k ? GetM68000MemMap() : GetS68000MemMap()), uiLabelPos);

                    if ((OPC & 0xF00) == 0x100)
                    {
                        //BSR  label
                        if (szLabel)
                            sprintf(Dbg_Str, "BSR (~~%08X~~).b", uiLabelPos);
                        else
                            sprintf(Dbg_Str, "BSR ($%06X).b", uiLabelPos);
                        break;
                    }

                    if (!(OPC & 0xF00))
                    {
                        //BRA  label
                        if (szLabel)
                            sprintf(Dbg_Str, "BRA (~~%08X~~).b", uiLabelPos);
                        else
                            sprintf(Dbg_Str, "BRA ($%08X).b", uiLabelPos);
                        break;
                    }

                    //BCC  label
                    if (szLabel)
                        sprintf(Dbg_Str, "B%s (~~%08X~~).b", Make_Dbg_Cond_Str((OPC >> 8) & 0xF), uiLabelPos);
                    else
                        sprintf(Dbg_Str, "B%s ($%06X).b", Make_Dbg_Cond_Str((OPC >> 8) & 0xF), uiLabelPos);
                }
                else
                {
                    uint32 uiLabelPos = _uiPosition + 2 + ((signed short)(Next_Word()));
                    const char* szLabel = GetLabel((gCurrentCore == &m68k ? GetM68000MemMap() : GetS68000MemMap()), uiLabelPos);

                    if ((OPC & 0xF00) == 0x100)
                    {
                        //BSR  label
                        if (szLabel)
                            sprintf(Dbg_Str, "BSR (~~%08X~~).w", uiLabelPos);
                        else
                            sprintf(Dbg_Str, "BSR ($%06X).w", uiLabelPos);
                        break;
                    }

                    if (!(OPC & 0xF00))
                    {
                        //BRA  label
                        if (szLabel)
                            sprintf(Dbg_Str, "BRA (~~%08X~~).w", uiLabelPos);
                        else
                            sprintf(Dbg_Str, "BRA ($%06X).w", uiLabelPos);
                        break;
                    }

                    //BCC  label
                    if (szLabel)
                        sprintf(Dbg_Str, "B%s (~~%08X~~).w", Make_Dbg_Cond_Str((OPC >> 8 ) & 0xF), uiLabelPos);
                    else
                        sprintf(Dbg_Str, "B%s ($%06X).w", Make_Dbg_Cond_Str((OPC >> 8 ) & 0xF), uiLabelPos);
                }
                break;

            case 7:
                //MOVEQ  #k8,Dd
                sprintf(Dbg_Str, "MOVEQ #$%.2X,D%.1d%c", OPC & 0xFF, (OPC >> 9) & 0x7, 0);
                break;

            case 8:

                if (OPC & 0x100)
                {
                    if (!(OPC & 0xF8))
                    {
                        //SBCD  Ds,Dd
                        sprintf(Dbg_Str, "SBCD D%.1d,D%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                        break;
                    }

                    if ((OPC & 0xF8) == 0x8)
                    {
                        //SBCD  -(As),-(Ad)
                        sprintf(Dbg_Str, "SBCD -(A%.1d),-(A%.1d)%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                        break;
                    }

                    if ((OPC & 0xC0) == 0xC0)
                        //DIVS.w  a,Dd
                        sprintf(Dbg_Str, "DIVS.W %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                    else
                        //OR.z  Ds,a
                        sprintf(Dbg_Str, "OR%s D%.1d;%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                }
                else
                {
                    if ((OPC & 0xC0) == 0xC0)
                        //DIVU.w  a,Dd
                        sprintf(Dbg_Str, "DIVU.W %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                    else
                        //OR.z  a,Dd
                        sprintf(Dbg_Str, "OR%s %s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                }
                break;

            case 9:

                if ((OPC & 0xC0) == 0xC0)
                    //SUBA.z  a,Ad
                    sprintf(Dbg_Str, "SUBA%s %s,A%.1d%c", Make_Dbg_Size_Str_2((OPC >> 8) & 1), Make_Dbg_EA_Str((OPC >> 8) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                else
                {
                    if (OPC & 0x100)
                    {
                        if (!(OPC & 0x38))
                        {
                            //SUBX.z  Ds,Dd
                            sprintf(Dbg_Str, "SUBX%s D%.1d,D%.1d%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                            break;
                        }

                        if ((OPC & 0x38) == 0x8)
                        {
                            //SUBX.z  -(As),-(Ad)
                            sprintf(Dbg_Str, "SUBX%s -(A%.1d),-(A%.1d)%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                            break;
                        }

                        //SUB.z  Ds,a
                        sprintf(Dbg_Str, "SUB%s D%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    }
                    else
                        //SUB.z  a,Dd
                        sprintf(Dbg_Str, "SUB%s %s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                }
                break;

            case 10:
                //Bad Opcode
                sprintf(Dbg_Str, "BadOpcode", 0);
                break;

            case 11:

                if ((OPC & 0xC0) == 0xC0)
                    //CMPA.z  a,Ad
                    sprintf(Dbg_Str, "CMPA%s %s,A%.1d%c", Make_Dbg_Size_Str_2((OPC >> 8) & 1), Make_Dbg_EA_Str((OPC >> 7) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                else
                {
                    if (OPC & 0x100)
                    {
                        if ((OPC & 0x38) == 0x8)
                        {
                            //CMPM.z  (As)+,(Ad)+
                            sprintf(Dbg_Str, "CMPM%s (A%.1d)+,(A%.1d)+%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                            break;
                        }

                        //EOR.z  Ds,a
                        sprintf(Dbg_Str, "EOR%s D%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    }
                    else
                        //CMP.z  a,Dd
                        sprintf(Dbg_Str, "CMP%s %s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                }
                break;

            case 12:

                if ((OPC & 0X1F8) == 0x100)
                {
                    //ABCD Ds,Dd
                    sprintf(Dbg_Str, "ABCD D%.1d,D%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                    break;
                }

                if ((OPC & 0X1F8) == 0x140)
                {
                    //EXG.l Ds,Dd
                    sprintf(Dbg_Str, "EXG.L D%.1d,D%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                    break;
                }

                if ((OPC & 0X1F8) == 0x108)
                {
                    //ABCD -(As),-(Ad)
                    sprintf(Dbg_Str, "ABCD -(A%.1d),-(A%.1d)%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                    break;
                }

                if ((OPC & 0X1F8) == 0x148)
                {
                    //EXG.l As,Ad
                    sprintf(Dbg_Str, "EXG.L A%.1d,A%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                    break;
                }

                if ((OPC & 0X1F8) == 0x188)
                {
                    //EXG.l As,Dd
                    sprintf(Dbg_Str, "EXG.L A%.1d,D%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                    break;
                }

                switch((OPC	>> 6) & 0x7)
                {
                case 0: case 1: case 2:
                    //AND.z  a,Dd
                    sprintf(Dbg_Str, "AND%s %s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                    break;

                case 3:
                    //MULU.w  a,Dd
                    sprintf(Dbg_Str, "MULU.W %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                    break;

                case 4: case 5: case 6:
                    //AND.z  Ds,a
                    sprintf(Dbg_Str, "AND%s D%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    break;

                case 7:
                    //MULS.w  a,Dd
                    sprintf(Dbg_Str, "MULS.W %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                    break;
                }
                break;

            case 13:

                if ((OPC & 0xC0) == 0xC0)
                    //ADDA.z  a,Ad
                    sprintf(Dbg_Str, "ADDA%s %s,A%.1d%c", Make_Dbg_Size_Str_2((OPC >> 8) & 1), Make_Dbg_EA_Str((OPC >> 8) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                else
                {
                    if (OPC & 0x100)
                    {
                        if (!(OPC & 0x38))
                        {
                            //ADDX.z  Ds,Dd
                            sprintf(Dbg_Str, "ADDX%s D%.1d,D%.1d%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                            break;
                        }

                        if ((OPC & 0x38) == 0x8)
                        {
                            //ADDX.z  -(As),-(Ad)
                            sprintf(Dbg_Str, "ADDX%s -(A%.1d),-(A%.1d)%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                            break;
                        }

                        //ADD.z  Ds,a
                        sprintf(Dbg_Str, "ADD%s D%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    }
                    else
                        //ADD.z  a,Dd
                        sprintf(Dbg_Str, "ADD%s %s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                }
                break;

            case 14:

                if ((OPC & 0xC0) == 0xC0)
                {
                    switch ((OPC >> 8) & 0x7)
                    {
                    case 0:
                        //ASR.w  #1,a
                        sprintf(Dbg_Str, "ASR.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 1:
                        //ASL.w  #1,a
                        sprintf(Dbg_Str, "ASL.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 2:
                        //LSR.w  #1,a
                        sprintf(Dbg_Str, "LSR.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 3:
                        //LSL.w  #1,a
                        sprintf(Dbg_Str, "LSL.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 4:
                        //ROXR.w  #1,a
                        sprintf(Dbg_Str, "ROXR.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 5:
                        //ROXL.w  #1,a
                        sprintf(Dbg_Str, "ROXL.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 6:
                        //ROR.w  #1,a
                        sprintf(Dbg_Str, "ROR.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 7:
                        //ROL.w  #1,a
                        sprintf(Dbg_Str, "ROL.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    }
                }
                else
                {
                    switch ((OPC >> 3) & 0x3F)
                    {
                    case 0: case 8: case 16:
                        //ASR.z  #k,Dd
                        sprintf(Dbg_Str, "ASR%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 1: case 9: case 17:
                        //LSR.z  #k,Dd
                        sprintf(Dbg_Str, "LSR%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 2: case 10: case 18:
                        //ROXR.z  #k,Dd
                        sprintf(Dbg_Str, "ROXR%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 3: case 11: case 19:
                        //ROR.z  #k,Dd
                        sprintf(Dbg_Str, "ROR%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 4: case 12: case 20:
                        //ASR.z  Ds,Dd
                        sprintf(Dbg_Str, "ASR%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 5: case 13: case 21:
                        //LSR.z  Ds,Dd
                        sprintf(Dbg_Str, "LSR%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 6: case 14: case 22:
                        //ROXR.z  Ds,Dd
                        sprintf(Dbg_Str, "ROXR%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 7: case 15: case 23:
                        //ROR.z  Ds,Dd
                        sprintf(Dbg_Str, "ROR%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 32: case 40: case 48:
                        //ASL.z  #k,Dd
                        sprintf(Dbg_Str, "ASL%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 33: case 41: case 49:
                        //LSL.z  #k,Dd
                        sprintf(Dbg_Str, "LSL%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 34: case 42: case 50:
                        //ROXL.z  #k,Dd
                        sprintf(Dbg_Str, "ROXL%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 35: case 43: case 51:
                        //ROL.z  #k,Dd
                        sprintf(Dbg_Str, "ROL%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 36: case 44: case 52:
                        //ASL.z  Ds,Dd
                        sprintf(Dbg_Str, "ASL%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 37: case 45: case 53:
                        //LSL.z  Ds,Dd
                        sprintf(Dbg_Str, "LSL%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 38: case 46: case 54:
                        //ROXL.z  Ds,Dd
                        sprintf(Dbg_Str, "ROXL%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 39: case 47: case 55:
                        //ROL.z  Ds,Dd
                        sprintf(Dbg_Str, "ROL%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    }
                }
                break;

            case 15:
                //Bad Opcode
                sprintf(Dbg_Str, "BadOpcode");
                break;
            }

            return(Dbg_Str);
        }
    }

    namespace Special
    {
        static char Dbg_Str[512];
        static char Dbg_EA_Str[512];
        static char Dbg_Size_Str[3];
        static char Dbg_Cond_Str[3];
        static char Dbg_RegList_Str[512];

        static uint32 Position;
        static uint32 SizeMode = 1;
        static uint32 TotalSizeMode = 1;

        static unsigned short (*Next_Word)();
        static unsigned int (*Next_Long)();

        char *Make_Dbg_RegList_Str(int RegList, int  Flip)
        {
            int Bit;
            int Index = 0;
            int Empty = 1;

            TotalSizeMode = 0;

            for (Bit = 0; Bit < 8; ++Bit)
            {
                if (RegList & 0x1)
                {
                    if (Empty)
                        Empty = 0;
                    else
                        Dbg_RegList_Str[Index++] = '/';

                    Dbg_RegList_Str[Index++] = '?';
                    Dbg_RegList_Str[Index++] = '!';
                    Dbg_RegList_Str[Index++] = '?';
                    Dbg_RegList_Str[Index++] = 'R';
                    Dbg_RegList_Str[Index++] = 'E';
                    Dbg_RegList_Str[Index++] = 'G';
                    Dbg_RegList_Str[Index++] = '{';
                    Dbg_RegList_Str[Index++] = '^';
                    Dbg_RegList_Str[Index++] = '}';

                    Dbg_RegList_Str[Index++] = Flip ? 'D' : 'A';
                    Dbg_RegList_Str[Index++] = Flip ? '0' + Bit : '0' + (7 - Bit);

                    Dbg_RegList_Str[Index++] = '{';
                    Dbg_RegList_Str[Index++] = '^';
                    Dbg_RegList_Str[Index++] = '}';

                    int iRegIndex = Flip ? 1 + Bit : 9 + (7 - Bit);

                    if (iRegIndex >= 10)
                        Dbg_RegList_Str[Index++] = '0' + (iRegIndex / 10);

                    Dbg_RegList_Str[Index++] = '0' + (iRegIndex % 10);

                    Dbg_RegList_Str[Index++] = '?';
                    Dbg_RegList_Str[Index++] = '!';
                    Dbg_RegList_Str[Index++] = '?';

                    TotalSizeMode += SizeMode;
                }

                RegList >>= 1;
            }

            for (Bit = 0; Bit < 8; ++Bit)
            {
                if (RegList & 0x1)
                {
                    if (Empty)
                        Empty = 0;
                    else
                        Dbg_RegList_Str[Index++] = '/';

                    Dbg_RegList_Str[Index++] = '?';
                    Dbg_RegList_Str[Index++] = '!';
                    Dbg_RegList_Str[Index++] = '?';
                    Dbg_RegList_Str[Index++] = 'R';
                    Dbg_RegList_Str[Index++] = 'E';
                    Dbg_RegList_Str[Index++] = 'G';
                    Dbg_RegList_Str[Index++] = '{';
                    Dbg_RegList_Str[Index++] = '^';
                    Dbg_RegList_Str[Index++] = '}';

                    Dbg_RegList_Str[Index++] = Flip ? 'A' : 'D';
                    Dbg_RegList_Str[Index++] = Flip ? '0' + Bit : '0' + (7 - Bit);

                    Dbg_RegList_Str[Index++] = '{';
                    Dbg_RegList_Str[Index++] = '^';
                    Dbg_RegList_Str[Index++] = '}';

                    int iRegIndex = Flip ? 9 + Bit : 1 + (7 - Bit);

                    if (iRegIndex >= 10)
                        Dbg_RegList_Str[Index++] = '0' + (iRegIndex / 10);

                    Dbg_RegList_Str[Index++] = '0' + (iRegIndex % 10);

                    Dbg_RegList_Str[Index++] = '?';
                    Dbg_RegList_Str[Index++] = '!';
                    Dbg_RegList_Str[Index++] = '?';

                    TotalSizeMode += SizeMode;
                }

                RegList >>= 1;
            }

            Dbg_RegList_Str[Index++] = '\0';

            return Dbg_RegList_Str;
        }

        //?!?REG{^}text{^}reg_index?!?
        //?!?MEM{^}text{^}addr{^}size?!?

        char *Make_Dbg_EA_Str(int Size, int EA_Num, int Reg_Num)
        {
            Dbg_EA_Str[0] = '\0';

            switch(EA_Num)
            {
            case 0:
                // 000 rrr  Dr
                sprintf(Dbg_EA_Str, "?!?REG{^}D%.1d{^}%d?!?", Reg_Num, 1 + Reg_Num);
                break;

            case 1:
                // 001 rrr  Ar
                sprintf(Dbg_EA_Str, "?!?REG{^}A%.1d{^}%d?!?", Reg_Num, 9 + Reg_Num);
                break;

            case 2:
                // 010 rrr  (Ar)
                sprintf(Dbg_EA_Str, "?!?MEM{^}({^}%d{^}%d?!??!?REG{^}A%.1d{^}%d?!??!?MEM{^}){^}%d{^}%d?!?", gCurrentCore->dar[Reg_Num + 8], SizeMode, Reg_Num, 9 + Reg_Num, gCurrentCore->dar[Reg_Num + 8], SizeMode);
                break;

            case 3:
                // 011 rrr  (Ar)+
                sprintf(Dbg_EA_Str, "?!?MEM{^}({^}%d{^}%d?!??!?REG{^}A%.1d{^}%d?!??!?MEM{^})+{^}%d{^}%d?!?", gCurrentCore->dar[Reg_Num + 8], SizeMode, Reg_Num, 9 + Reg_Num, gCurrentCore->dar[Reg_Num + 8], SizeMode);
                break;

            case 4:
                // 100 rrr  -(Ar)
                sprintf(Dbg_EA_Str, "?!?MEM{^}-({^}%d{^}%d?!??!?REG{^}A%.1d{^}%d?!??!?MEM{^}){^}%d{^}%d?!?", gCurrentCore->dar[Reg_Num + 8] - TotalSizeMode, SizeMode, Reg_Num, 9 + Reg_Num, gCurrentCore->dar[Reg_Num + 8] - TotalSizeMode, SizeMode);
                break;

            case 5:
                // 101 rrr  d16(Ar)     dddddddd dddddddd
                {
                    uint32 uiWord = (uint32)(int32)(int16)Next_Word();
                    sprintf(Dbg_EA_Str, "?!?MEM{^}$%.4X({^}%d{^}%d?!??!?REG{^}A%.1d{^}%d?!??!?MEM{^}){^}%d{^}%d?!?", uiWord & 0xffff, uiWord + gCurrentCore->dar[Reg_Num + 8], SizeMode, Reg_Num, 9 + Reg_Num, uiWord + gCurrentCore->dar[Reg_Num + 8], SizeMode);
                }
                break;

            case 6:
                // 110 rrr  d8(Ar,ix)   aiiizcc0 dddddddd
                {
                    uint32 uiWord = Next_Word() & 0xFFFF;
                    uint32 uiRegIndex = (uiWord >> 12) & 0x7;
                    uint32 uiDisp = (uint32)(int32)(int8)(uiWord & 0xff);

                    if (uiWord & 0x8000)
                    {
                        uint32 uiIndex = (uiWord & 0x0800 ? gCurrentCore->dar[uiRegIndex + 8] : (uint32)(int32)(int16)gCurrentCore->dar[uiRegIndex + 8]);
                        uint32 uiAddr = gCurrentCore->dar[Reg_Num + 8] + uiDisp + uiIndex;
                        sprintf(Dbg_EA_Str, "?!?MEM{^}$%.2X({^}%d{^}%d?!??!?REG{^}A%.1d{^}%d?!?,?!?REG{^}A%.1d.%c{^}%d?!??!?MEM{^}){^}%d{^}%d?!?", uiWord & 0xFF, uiAddr, SizeMode, Reg_Num, Reg_Num + 9, uiRegIndex, (uiWord & 0x0800 ? 'l' : 'w' ), uiRegIndex + 9, uiAddr, SizeMode);
                    }
                    else
                    {
                        uint32 uiIndex = (uiWord & 0x0800 ? gCurrentCore->dar[uiRegIndex] : (uint32)(int32)(int16)gCurrentCore->dar[uiRegIndex]);
                        uint32 uiAddr = gCurrentCore->dar[Reg_Num + 8] + uiDisp + uiIndex;
                        sprintf(Dbg_EA_Str, "?!?MEM{^}$%.2X({^}%d{^}%d?!??!?REG{^}A%.1d{^}%d?!?,?!?REG{^}D%.1d.%c{^}%d?!??!?MEM{^}){^}%d{^}%d?!?", uiWord & 0xFF, uiAddr, SizeMode, Reg_Num, Reg_Num + 9, uiRegIndex, (uiWord & 0x0800 ? 'l' : 'w' ), uiRegIndex + 1, uiAddr, SizeMode);
                    }
                    break;
                }
            case 7:
                //?!?LINK{^}VIEW{^}LINK_TO_POS?!?
                switch(Reg_Num)
                {
                case 0:
                    // 111 000  addr16      dddddddd dddddddd
                    {
                        uint32 uiPos = (Position & 0xffff0000) | Next_Word();
                        const char* szLabel = GetLabel((gCurrentCore == &m68k ? GetM68000MemMap() : GetS68000MemMap()), uiPos);

                        if (szLabel)
                            sprintf(Dbg_EA_Str, "(?!?LINK{^}$%04X{^}%d?!?).w", uiPos & 0xffff, uiPos);
                        else
                            sprintf(Dbg_EA_Str, "?!?MEM{^}($%04X).w{^}%d{^}%d?!?", uiPos & 0xffff, uiPos, SizeMode);
                        break;
                    }
                case 1:
                    // 111 001  addr32      dddddddd dddddddd ddddddddd dddddddd
                    {
                        uint32 uiPos = Next_Long();
                        const char* szLabel = GetLabel((gCurrentCore == &m68k ? GetM68000MemMap() : GetS68000MemMap()), uiPos);

                        if (szLabel)
                            sprintf(Dbg_EA_Str, "(?!?LINK{^}$%08X{^}%d?!?).l", uiPos, uiPos);
                        else
                            sprintf(Dbg_EA_Str, "?!?MEM{^}($%08X).l{^}%d{^}%d?!?", uiPos, uiPos, SizeMode);
                        break;
                    }
                case 2:
                    // 111 010  d16(PC)     dddddddd dddddddd
                    {
                        uint32 uiOffset = Next_Word();
                        uint32 uiAddr = ((uint32)(int32)(int16)uiOffset) + Position + 2;
                        sprintf(Dbg_EA_Str, "?!?MEM{^}$%.4X({^}%d{^}%d?!??!?REG{^}PC{^}0?!??!?MEM{^}){^}%d{^}%d?!?", uiOffset, uiAddr, SizeMode, uiAddr, SizeMode);
                    }
                    break;

                case 3:
                    {
                        uint32 uiWord = Next_Word() & 0xFFFF;
                        uint32 uiRegIndex = (uiWord >> 12) & 0x7;
                        uint32 uiDisp = (uint32)(int32)(int8)(uiWord & 0xff);

                        if (uiWord & 0x8000)
                        {
                            uint32 uiIndex = (uiWord & 0x0800 ? gCurrentCore->dar[uiRegIndex + 8] : (uint32)(int32)(int16)gCurrentCore->dar[uiRegIndex + 8]);
                            uint32 uiAddr = (Position + 2) + uiDisp + uiIndex;
                            sprintf(Dbg_EA_Str, "?!?MEM{^}$%.2X({^}%d{^}%d?!??!?REG{^}PC{^}0?!?,?!?REG{^}A%.1d.%c{^}%d?!??!?MEM{^}){^}%d{^}%d?!?", uiWord & 0xFF, uiAddr, SizeMode, uiRegIndex, (uiWord & 0x0800 ? 'l' : 'w' ), uiRegIndex + 9, uiAddr, SizeMode);
                        }
                        else
                        {
                            uint32 uiIndex = (uiWord & 0x0800 ? gCurrentCore->dar[uiRegIndex] : (uint32)(int32)(int16)gCurrentCore->dar[uiRegIndex]);
                            uint32 uiAddr = (Position + 2) + uiDisp + uiIndex;
                            sprintf(Dbg_EA_Str, "?!?MEM{^}$%.2X({^}%d{^}%d?!??!?REG{^}PC{^}0?!?,?!?REG{^}D%.1d.%c{^}%d?!??!?MEM{^}){^}%d{^}%d?!?", uiWord & 0xFF, uiAddr, SizeMode, uiRegIndex, (uiWord & 0x0800 ? 'l' : 'w' ), uiRegIndex + 1, uiAddr, SizeMode);
                        }
                    }
                    break;
                case 4:
                    // 111 100  imm/implied
                    switch(Size)
                    {
                    case 0:
                        sprintf(Dbg_EA_Str, "#$%.2X", Next_Word() & 0xFF);
                        break;

                    case 1:
                        sprintf(Dbg_EA_Str, "#$%.4X", Next_Word());
                        break;

                    case 2:
                        sprintf(Dbg_EA_Str, "#$%.8X", Next_Long());
                        break;
                    }
                    break;
                }
                break;
            }

            return(Dbg_EA_Str);
        }


        char *Make_Dbg_Size_Str(int Size)
        {
            Dbg_Size_Str[2] = 0;
            sprintf(Dbg_Size_Str, ".?");
            SizeMode = 1;

            switch(Size)
            {
            case 0:
                SizeMode = 1;
                sprintf(Dbg_Size_Str, ".B");
                break;

            case 1:
                SizeMode = 2;
                sprintf(Dbg_Size_Str, ".W");
                break;

            case 2:
                SizeMode = 4;
                sprintf(Dbg_Size_Str, ".L");
                break;
            }

            TotalSizeMode = SizeMode;

            return(Dbg_Size_Str);
        }


        char *Make_Dbg_Size_Str_2(int Size)
        {
            Dbg_Size_Str[2] = 0;
            sprintf(Dbg_Size_Str, ".?");
            SizeMode = 1;

            switch(Size)
            {
            case 0:
                SizeMode = 2;
                sprintf(Dbg_Size_Str, ".W");
                break;

            case 1:
                SizeMode = 4;
                sprintf(Dbg_Size_Str, ".L");
                break;
            }

            TotalSizeMode = SizeMode;

            return(Dbg_Size_Str);
        }

        char *Make_Dbg_Cond_Str(int Cond)
        {
            Dbg_Cond_Str[2] = 0;
            sprintf(Dbg_Cond_Str, "??");

            switch(Cond)
            {
            case 0:
                sprintf(Dbg_Cond_Str, "Tr");
                break;

            case 1:
                sprintf(Dbg_Cond_Str, "Fa");
                break;

            case 2:
                sprintf(Dbg_Cond_Str, "HI");
                break;

            case 3:
                sprintf(Dbg_Cond_Str, "LS");
                break;

            case 4:
                sprintf(Dbg_Cond_Str, "CC");
                break;

            case 5:
                sprintf(Dbg_Cond_Str, "CS");
                break;

            case 6:
                sprintf(Dbg_Cond_Str, "NE");
                break;

            case 7:
                sprintf(Dbg_Cond_Str, "EQ");
                break;

            case 8:
                sprintf(Dbg_Cond_Str, "VC");
                break;

            case 9:
                sprintf(Dbg_Cond_Str, "VS");
                break;

            case 10:
                sprintf(Dbg_Cond_Str, "PL");
                break;

            case 11:
                sprintf(Dbg_Cond_Str, "MI");
                break;

            case 12:
                sprintf(Dbg_Cond_Str, "GE");
                break;

            case 13:
                sprintf(Dbg_Cond_Str, "LT");
                break;

            case 14:
                sprintf(Dbg_Cond_Str, "GT");
                break;

            case 15:
                sprintf(Dbg_Cond_Str, "LE");
                break;
            }

            return(Dbg_Cond_Str);
        }

        void FixResult(char* _szText)
        {
            char* szOut = _szText;
            char szTemp[512];
            int iAdd = 0;

            while (*_szText != '\0')
            {
                if (_szText[0] == '(' && _szText[1] == 'D' && _szText[2] >= '0' && _szText[2] <= '7' && _szText[3] == ')')
                {
                    int Reg_Num = (_szText[2] - '0');
                    iAdd += sprintf(szTemp + iAdd, "?!?MEM{^}({^}%d{^}%d?!??!?REG{^}D%.1d{^}%d?!??!?MEM{^}){^}%d{^}%d?!?", gCurrentCore->dar[Reg_Num], SizeMode, Reg_Num, 1 + Reg_Num, gCurrentCore->dar[Reg_Num], SizeMode);
                    _szText += 4;
                }
                else if (_szText[0] == '(' && _szText[1] == 'A' && _szText[2] >= '0' && _szText[2] <= '7' && _szText[3] == ')')
                {
                    int Reg_Num = (_szText[2] - '0');
                    iAdd += sprintf(szTemp + iAdd, "?!?MEM{^}({^}%d{^}%d?!??!?REG{^}A%.1d{^}%d?!??!?MEM{^}){^}%d{^}%d?!?", gCurrentCore->dar[Reg_Num + 8], SizeMode, Reg_Num, 9 + Reg_Num, gCurrentCore->dar[Reg_Num + 8], SizeMode);
                    _szText += 4;}
                else if (_szText[0] == '-' && _szText[1] == '(' && _szText[2] == 'A' && _szText[3] >= '0' && _szText[3] <= '7' && _szText[4] == ')')
                {
                    int Reg_Num = (_szText[3] - '0');
                    iAdd += sprintf(szTemp + iAdd, "?!?MEM{^}-({^}%d{^}%d?!??!?REG{^}A%.1d{^}%d?!??!?MEM{^}){^}%d{^}%d?!?", gCurrentCore->dar[Reg_Num + 8] - TotalSizeMode, SizeMode, Reg_Num, 9 + Reg_Num, gCurrentCore->dar[Reg_Num + 8] - TotalSizeMode, SizeMode);
                    _szText += 5;
                }
                else if ((_szText[0] == ' ' || _szText[0] == ',') && _szText[1] == 'A' && _szText[2] >= '0' && _szText[2] <= '7')
                {
                    iAdd += sprintf(szTemp + iAdd, "%c?!?REG{^}A%c{^}%d?!?", _szText[0], _szText[2], (_szText[2] - '0') + 9);
                    _szText += 3;
                }
                else if ((_szText[0] == ' ' || _szText[0] == ',') && _szText[1] == 'D' && _szText[2] >= '0' && _szText[2] <= '7')
                {
                    iAdd += sprintf(szTemp + iAdd, "%c?!?REG{^}D%c{^}%d?!?", _szText[0], _szText[2], (_szText[2] - '0') + 1);
                    _szText += 3;
                }
                else
                {
                    szTemp[iAdd++] = *_szText++;
                }
            }

            szTemp[iAdd] = '\0';

            strcpy(szOut, szTemp);
        }

        char *M68KDisasmSpecial(unsigned short (*NW)(), unsigned int (*NL)(), unsigned int _uiPosition)
        {
            int i;
            unsigned short OPC;
            char Tmp_Str[512] = {0};

            Position =_uiPosition;
            Next_Word = NW;
            Next_Long = NL;

            OPC = Next_Word();

            sprintf(Dbg_Str, "Unknow Opcode%c", 0);

            switch(OPC >> 12)
            {
            case 0:

                if (OPC & 0x100)
                {
                    if ((OPC & 0x038) == 0x8)
                    {
                        if (OPC & 0x080)
                            //MOVEP.z Ds,d16(Ad) FIXE ME
                            sprintf(Dbg_Str, "MOVEP%s ?!?REG{^}D%.1d{^}%d?!?,#$%.4X(A%.1d)", Make_Dbg_Size_Str_2((OPC & 0x40) >> 6), (OPC & 0xE00) >> 9, ((OPC & 0xE00) >> 9) + 1, Next_Word(), OPC & 0x7);
                        else
                            //MOVEP.z d16(As),Dd FIXE ME
                            sprintf(Dbg_Str, "MOVEP%s #$%.4X(A%.1d),?!?REG{^}D%.1d{^}%d?!?", Make_Dbg_Size_Str_2((OPC & 0x40) >> 6), Next_Word(), OPC & 0x7, (OPC & 0xE00) >> 9, ((OPC & 0xE00) >> 9) + 1);
                    }
                    else
                    {
                        TotalSizeMode = SizeMode = 1;
                        switch((OPC >> 6) & 0x3)
                        {
                        case 0:
                            //BTST  Ds,a
                            sprintf(Dbg_Str, "BTST ?!?REG{^}D%.1d{^}%d?!?,%s", (OPC & 0xE00) >> 9, ((OPC & 0xE00) >> 9) + 1, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 7));
                            break;

                        case 1:
                            //BCHG  Ds,a
                            sprintf(Dbg_Str, "BCHG ?!?REG{^}D%.1d{^}%d?!?,%s", (OPC & 0xE00) >> 9, ((OPC & 0xE00) >> 9) + 1, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 7));
                            break;

                        case 2:
                            //BCLR  Ds,a
                            sprintf(Dbg_Str, "BCLR ?!?REG{^}D%.1d{^}%d?!?,%s", (OPC & 0xE00) >> 9, ((OPC & 0xE00) >> 9) + 1, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 7));
                            break;

                        case 3:
                            //BSET  Ds,a
                            sprintf(Dbg_Str, "BSET ?!?REG{^}D%.1d{^}%d?!?,%s", (OPC & 0xE00) >> 9, ((OPC & 0xE00) >> 9) + 1, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 7));
                            break;
                        }
                    }
                }
                else
                {
                    switch((OPC >> 6) & 0x3F)
                    {
                    case 0:
                        TotalSizeMode = SizeMode = 1;
                        if ((OPC & 0xff) == 0x3c)
                        {
                            i = Next_Word() & 0xFF;
                            sprintf(Dbg_Str, "ORI.B #$%.2X,?!?REG{^}CCR{^}17?!?", i);
                        }
                        else
                        {
                            i = Next_Word() & 0xFF;
                            sprintf(Dbg_Str, "ORI.B #$%.2X,%s", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7));
                        }
                        break;

                    case 1:
                        //ORI.W  #k,a
                        TotalSizeMode = SizeMode = 2;
                        if ((OPC & 0xff) == 0x7c)
                        {
                            i = Next_Word() & 0xFFFF;
                            sprintf(Dbg_Str, "ORI.W #$%.4X,?!?REG{^}CCR{^}17?!?", i);
                        }
                        else
                        {
                            i = Next_Word() & 0xFFFF;
                            sprintf(Dbg_Str, "ORI.W #$%.4X,%s", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7));
                        }
                        break;

                    case 2:
                        //ORI.L  #k,a
                        TotalSizeMode = SizeMode = 4;
                        i = Next_Long();
                        sprintf(Dbg_Str, "ORI.L #$%.8X,%s", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7));
                        break;

                    case 8:
                        //ANDI.B  #k,a
                        TotalSizeMode = SizeMode = 1;
                        if ((OPC & 0xff) == 0x3c)
                        {
                            i = Next_Word() & 0xFF;
                            sprintf(Dbg_Str, "ANDI.B #$%.2X,?!?REG{^}CCR{^}17?!?", i);

                        }
                        else
                        {
                            i = Next_Word() & 0xFF;
                            sprintf(Dbg_Str, "ANDI.B #$%.2X,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        }
                        break;

                    case 9:
                        //ANDI.W  #k,a
                        TotalSizeMode = SizeMode = 2;
                        if ((OPC & 0xff) == 0x7c)
                        {
                            i = Next_Word() & 0xFFFF;
                            sprintf(Dbg_Str, "ANDI.W #$%.4X,?!?REG{^}CCR{^}17?!?", i);
                        }
                        else
                        {
                            i = Next_Word() & 0xFFFF;
                            sprintf(Dbg_Str, "ANDI.W #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        }
                        break;

                    case 10:
                        //ANDI.L  #k,a
                        TotalSizeMode = SizeMode = 4;
                        i = Next_Long();
                        sprintf(Dbg_Str, "ANDI.L #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 16:
                        //SUBI.B  #k,a
                        TotalSizeMode = SizeMode = 1;
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "SUBI.B #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 17:
                        //SUBI.W  #k,a
                        TotalSizeMode = SizeMode = 2;
                        i = Next_Word() & 0xFFFF;
                        sprintf(Dbg_Str, "SUBI.W #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 18:
                        //SUBI.L  #k,a
                        TotalSizeMode = SizeMode = 4;
                        i = Next_Long();
                        sprintf(Dbg_Str, "SUBI.L #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 24:
                        //ADDI.B  #k,a
                        TotalSizeMode = SizeMode = 1;
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "ADDI.B #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 25:
                        //ADDI.W  #k,a
                        TotalSizeMode = SizeMode = 2;
                        i = Next_Word() & 0xFFFF;
                        sprintf(Dbg_Str, "ADDI.W #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 26:
                        //ADDI.L  #k,a
                        TotalSizeMode = SizeMode = 4;
                        i = Next_Long();
                        sprintf(Dbg_Str, "ADDI.L #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 32:
                        //BTST  #n,a
                        TotalSizeMode = SizeMode = 1;
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "BTST #%d,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 33:
                        //BCHG  #n,a
                        TotalSizeMode = SizeMode = 1;
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "BCHG #%d,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 34:
                        //BCLR  #n,a
                        TotalSizeMode = SizeMode = 1;
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "BCLR #%d,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 35:
                        //BSET  #n,a
                        TotalSizeMode = SizeMode = 1;
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "BSET #%d,%s%c", i, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 40:
                        //EORI.B  #k,a
                        TotalSizeMode = SizeMode = 1;
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "EORI.B #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 41:
                        //EORI.W  #k,a
                        TotalSizeMode = SizeMode = 2;
                        i = Next_Word() & 0xFFFF;
                        sprintf(Dbg_Str, "EORI.W #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 42:
                        //EORI.L  #k,a
                        TotalSizeMode = SizeMode = 4;
                        i = Next_Long();
                        sprintf(Dbg_Str, "EORI.L #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 48:
                        //CMPI.B  #k,a
                        TotalSizeMode = SizeMode = 1;
                        i = Next_Word() & 0xFF;
                        sprintf(Dbg_Str, "CMPI.B #$%.2X,%s%c", i & 0xFF, Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 49:
                        //CMPI.W  #k,a
                        TotalSizeMode = SizeMode = 2;
                        i = Next_Word() & 0xFFFF;
                        sprintf(Dbg_Str, "CMPI.W #$%.4X,%s%c", i, Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 50:
                        //CMPI.L  #k,a
                        TotalSizeMode = SizeMode = 4;
                        i = Next_Long();
                        sprintf(Dbg_Str, "CMPI.L #$%.8X,%s%c", i, Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;
                    }
                }
                break;

            case 1:
                //MOVE.b  as,ad
                TotalSizeMode = SizeMode = 1;
                sprintf(Tmp_Str, "%s%c", Make_Dbg_EA_Str(0, (OPC >> 3) & 0x7, OPC & 0x7), 0);
                sprintf(Dbg_Str, "MOVE.b %s,%s%c", Tmp_Str, Make_Dbg_EA_Str(0, (OPC >> 6) & 0x7, (OPC >> 9) & 0x7), 0);
                break;

            case 2:
                //MOVE.l  as,ad
                TotalSizeMode = SizeMode = 4;
                sprintf(Tmp_Str, "%s%c", Make_Dbg_EA_Str(2, (OPC >> 3) & 0x7, OPC & 0x7), 0);
                sprintf(Dbg_Str, "MOVE.l %s,%s%c", Tmp_Str, Make_Dbg_EA_Str(2, (OPC >> 6) & 0x7, (OPC >> 9) & 0x7), 0);
                break;

            case 3:
                //MOVE.w  as,ad
                TotalSizeMode = SizeMode = 2;
                sprintf(Tmp_Str, "%s%c", Make_Dbg_EA_Str(1, (OPC >> 3) & 0x7, OPC & 0x7), 0);
                sprintf(Dbg_Str, "MOVE.w %s,%s%c", Tmp_Str, Make_Dbg_EA_Str(1, (OPC >> 6) & 0x7, (OPC >> 9) & 0x7), 0);
                break;

            case 4:
                //SPECIALS ...

                if (OPC & 0x100)
                {
                    if (OPC & 0x40)
                    {
                        TotalSizeMode = SizeMode = 4;
                        //LEA  a,Ad
                        sprintf(Dbg_Str, "LEA %s,?!?REG{^}A%.1d{^}%d?!?", Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, ((OPC >> 9) & 0x7) + 9);
                    }
                    else
                    {
                        TotalSizeMode = SizeMode = 2;
                        //CHK.W  a,Dd
                        sprintf(Dbg_Str, "CHK.W %s,?!?REG{^}D%.1d{^}%d?!?", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, ((OPC >> 9) & 0x7) + 1);
                    }
                }
                else
                {
                    switch((OPC >> 6) & 0x3F)
                    {
                    case 0:	case 1: case 2:
                        //NEGX.z  a
                        sprintf(Dbg_Str, "NEGX%s %s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 3:
                        //MOVE  SR,a
                        TotalSizeMode = SizeMode = 2;
                        sprintf(Dbg_Str, "MOVE ?!?REG{^}CCR{^}17?!?,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 8: case 9: case 10:
                        //CLR.z  a
                        sprintf(Dbg_Str, "CLR%s %s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 16: case 17: case 18:
                        //NEG.z  a
                        sprintf(Dbg_Str, "NEG%s %s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 19:
                        //MOVE  a,CCR
                        TotalSizeMode = SizeMode = 1;
                        sprintf(Dbg_Str, "MOVE %s,?!?REG{^}CCR{^}17?!?%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 24: case 25: case 26:
                        //NOT.z  a
                        sprintf(Dbg_Str, "NOT%s %s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 27:
                        //MOVE  a,SR
                        TotalSizeMode = SizeMode = 2;
                        sprintf(Dbg_Str, "MOVE %s,?!?REG{^}CCR{^}17?!?", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7));
                        break;

                    case 32:
                        //NBCD  a
                        TotalSizeMode = SizeMode = 1;
                        sprintf(Dbg_Str, "NBCD %s%c", Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 33:

                        if (OPC & 0x38)
                            //PEA  a
                            sprintf(Dbg_Str, "PEA %s%c", Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        else
                            //SWAP.w  Dd
                            sprintf(Dbg_Str, "SWAP.w  ?!?REG{^}D%d{^}%d?!?", OPC & 0x7, (OPC & 0x7) + 1);

                        break;

                    case 34: case 35:

                        if (OPC & 0x38)
                        {
                            //MOVEM.z Reg-List,a
                            sprintf(Dbg_Str, "MOVEM%s %s,%s%c", Make_Dbg_Size_Str_2((OPC >> 6) & 1), Make_Dbg_RegList_Str(Next_Word(), 0), Make_Dbg_EA_Str((OPC >> 6) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        }
                        else
                        {
                            //EXT.z  Dd
                            sprintf(Dbg_Str, "EXT%s %s%c", Make_Dbg_Size_Str_2((OPC >> 6) & 1), Make_Dbg_EA_Str((OPC >> 6) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        }

                        break;

                    case 40: case 41: case 42:
                        //TST.z a
                        sprintf(Dbg_Str, "TST%s %s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 0x3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 43:
                        //TAS.b a
                        TotalSizeMode = SizeMode = 1;
                        sprintf(Dbg_Str, "TAS.B %s%c", Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 48: case 49:
                        //Bad Opcode
                        sprintf(Dbg_Str, "BadOpcode", 0);
                        break;

                    case 50: case 51:
                        //MOVEM.z a,Reg-List
                        sprintf(Dbg_Str, "MOVEM%s %s,%s%c", Make_Dbg_Size_Str_2((OPC >> 6) & 1), Make_Dbg_EA_Str((OPC >> 6) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), Make_Dbg_RegList_Str(Next_Word(), 1), 0);
                        break;
                    case 57:

                        switch((OPC >> 3) & 0x7)
                        {
                        case 0: case 1:
                            //TRAP  #vector
                            sprintf(Dbg_Str, "TRAP #$%.1X", OPC & 0xF);
                            break;

                        case 2:
                            //LINK As,#k16
                            sprintf(Dbg_Str, "LINK ?!?REG{^}A%.1d{^}%d?!?,#$%.4X", OPC & 0x7, (OPC & 0x7) + 9, Next_Word());
                            break;

                        case 3:
                            //ULNK Ad
                            sprintf(Dbg_Str, "ULNK ?!?REG{^}A%.1d{^}%d?!?", OPC & 0x7, (OPC & 0x7) + 9);
                            break;

                        case 4:
                            //MOVE As,USP
                            sprintf(Dbg_Str, "MOVE ?!?REG{^}A%.1d{^}%d?!?,?!?REG{^}USP{^}17?!?",OPC & 0x7, (OPC & 0x7) + 9);
                            break;

                        case 5:
                            //MOVE USP,Ad
                            sprintf(Dbg_Str, "MOVE ?!?REG{^}USP{^}17?!?,?!?REG{^}A%.1d{^}%d?!?",OPC & 0x7, (OPC & 0x7) + 9);
                            break;

                        case 6:

                            switch(OPC & 0x7)
                            {
                            case 0:
                                //RESET
                                sprintf(Dbg_Str, "RESET", 0);
                                break;

                            case 1:
                                //NOP
                                sprintf(Dbg_Str, "NOP", 0);
                                break;

                            case 2:
                                //STOP #k16
                                sprintf(Dbg_Str, "STOP #$%.4X%c", Next_Word(), 0);
                                break;

                            case 3:
                                //RTE
                                sprintf(Dbg_Str, "RTE", 0);
                                break;

                            case 4:
                                //Bad Opcode
                                sprintf(Dbg_Str, "BadOpcode", 0);
                                break;

                            case 5:
                                //RTS
                                sprintf(Dbg_Str, "RTS", 0);
                                break;

                            case 6:
                                //TRAPV
                                sprintf(Dbg_Str, "TRAPV", 0);
                                break;

                            case 7:
                                //RTR
                                sprintf(Dbg_Str, "RTR", 0);
                                break;
                            }
                            break;
                        }
                        break;

                    case 58:
                        //JSR  a
                        TotalSizeMode = SizeMode = 4;
                        sprintf(Dbg_Str, "JSR %s%c", Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 59:
                        //JMP  a
                        TotalSizeMode = SizeMode = 4;
                        sprintf(Dbg_Str, "JMP %s%c", Make_Dbg_EA_Str(2, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;
                    }
                }
                break;

            case 5:

                if ((OPC & 0xC0) == 0xC0)
                {
                    if ((OPC & 0x38) == 0x08)
                    {
                        TotalSizeMode = SizeMode = 4;
                        //DBCC  Ds,label
                        uint32 uiLabelPos = _uiPosition + 2 + ((signed short)Next_Word());

                        sprintf(Dbg_Str, "DB%s ?!?REG{^}D%.1d{^}%d?!?,(?!?LINK{^}$%08X{^}%d?!?).w", Make_Dbg_Cond_Str((OPC >> 8) & 0xF), OPC & 0x7, (OPC & 0x7) + 1, uiLabelPos, uiLabelPos);
                    }
                    else
                    {
                        TotalSizeMode = SizeMode = 1;
                        //STCC.b  a
                        sprintf(Dbg_Str, "ST%s %s%c", Make_Dbg_Cond_Str((OPC >> 8) & 0xF), Make_Dbg_EA_Str(0, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    }
                    break;
                }
                else
                {
                    if (OPC & 0x100)
                        //SUBQ.z  #k3,a
                        sprintf(Dbg_Str, "SUBQ%s #%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    else
                        //ADDQ.z  #k3,a
                        sprintf(Dbg_Str, "ADDQ%s #%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    break;
                }
                break;

            case 6:

                if (OPC & 0xFF)
                {
                    TotalSizeMode = SizeMode = 2;
                    uint32 uiLabelPos = _uiPosition + 2 + ((signed char)(OPC & 0xFF));
                    const char* szLabel = GetLabel((gCurrentCore == &m68k ? GetM68000MemMap() : GetS68000MemMap()), uiLabelPos);

                    if ((OPC & 0xF00) == 0x100)
                    {
                        //BSR  label
                        sprintf(Dbg_Str, "BSR (?!?LINK{^}$%08X{^}%d?!?).b", uiLabelPos, uiLabelPos);
                        break;
                    }

                    if (!(OPC & 0xF00))
                    {
                        //BRA  label
                        sprintf(Dbg_Str, "BRA (?!?LINK{^}$%08X{^}%d?!?).b", uiLabelPos, uiLabelPos);
                        break;
                    }

                    //BCC  label
                    sprintf(Dbg_Str, "B%s (?!?LINK{^}$%08X{^}%d?!?).b", Make_Dbg_Cond_Str((OPC >> 8) & 0xF), uiLabelPos, uiLabelPos);
                }
                else
                {
                    uint32 uiLabelPos = _uiPosition + 2 + ((signed short)(Next_Word()));
                    const char* szLabel = GetLabel((gCurrentCore == &m68k ? GetM68000MemMap() : GetS68000MemMap()), uiLabelPos);

                    if ((OPC & 0xF00) == 0x100)
                    {
                        //BRS label
                        sprintf(Dbg_Str, "BSR (?!?LINK{^}$%08X{^}%d?!?).w", uiLabelPos, uiLabelPos);
                        break;
                    }

                    if (!(OPC & 0xF00))
                    {
                        //BRA  label
                        sprintf(Dbg_Str, "BRA (?!?LINK{^}$%08X{^}%d?!?).w", uiLabelPos, uiLabelPos);
                        break;
                    }

                    //BCC  label
                    sprintf(Dbg_Str, "B%s (?!?LINK{^}$%08X{^}%d?!?).w", Make_Dbg_Cond_Str((OPC >> 8 ) & 0xF), uiLabelPos, uiLabelPos);
                }
                break;

            case 7:
                //MOVEQ  #k8,Dd
                sprintf(Dbg_Str, "MOVEQ #$%.2X,D%.1d%c", OPC & 0xFF, (OPC >> 9) & 0x7, 0);
                break;

            case 8:

                if (OPC & 0x100)
                {
                    if (!(OPC & 0xF8))
                    {
                        //SBCD  Ds,Dd
                        sprintf(Dbg_Str, "SBCD D%.1d,D%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                        break;
                    }

                    if ((OPC & 0xF8) == 0x8)
                    {
                        //SBCD  -(As),-(Ad)
                        sprintf(Dbg_Str, "SBCD -(A%.1d),-(A%.1d)%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                        break;
                    }

                    if ((OPC & 0xC0) == 0xC0)
                        //DIVS.w  a,Dd
                        sprintf(Dbg_Str, "DIVS.W %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                    else
                        //OR.z  Ds,a
                        sprintf(Dbg_Str, "OR%s D%.1d;%s%c", Make_Dbg_Size_Str((OPC >> 6) & 3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                }
                else
                {
                    if ((OPC & 0xC0) == 0xC0)
                        //DIVU.w  a,Dd
                        sprintf(Dbg_Str, "DIVU.W %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                    else
                        //OR.z  a,Dd
                        sprintf(Dbg_Str, "OR%s %s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                }
                break;

            case 9:

                if ((OPC & 0xC0) == 0xC0)
                    //SUBA.z  a,Ad
                    sprintf(Dbg_Str, "SUBA%s %s,A%.1d%c", Make_Dbg_Size_Str_2((OPC >> 8) & 1), Make_Dbg_EA_Str((OPC >> 8) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                else
                {
                    if (OPC & 0x100)
                    {
                        if (!(OPC & 0x38))
                        {
                            //SUBX.z  Ds,Dd
                            sprintf(Dbg_Str, "SUBX%s D%.1d,D%.1d%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                            break;
                        }

                        if ((OPC & 0x38) == 0x8)
                        {
                            //SUBX.z  -(As),-(Ad)
                            sprintf(Dbg_Str, "SUBX%s -(A%.1d),-(A%.1d)%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                            break;
                        }

                        //SUB.z  Ds,a
                        sprintf(Dbg_Str, "SUB%s D%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    }
                    else
                        //SUB.z  a,Dd
                        sprintf(Dbg_Str, "SUB%s %s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                }
                break;

            case 10:
                //Bad Opcode
                sprintf(Dbg_Str, "BadOpcode", 0);
                break;

            case 11:

                if ((OPC & 0xC0) == 0xC0)
                    //CMPA.z  a,Ad
                    sprintf(Dbg_Str, "CMPA%s %s,A%.1d%c", Make_Dbg_Size_Str_2((OPC >> 8) & 1), Make_Dbg_EA_Str((OPC >> 7) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                else
                {
                    if (OPC & 0x100)
                    {
                        if ((OPC & 0x38) == 0x8)
                        {
                            //CMPM.z  (As)+,(Ad)+
                            sprintf(Dbg_Str, "CMPM%s (A%.1d)+,(A%.1d)+%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                            break;
                        }

                        //EOR.z  Ds,a
                        sprintf(Dbg_Str, "EOR%s D%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    }
                    else
                        //CMP.z  a,Dd
                        sprintf(Dbg_Str, "CMP%s %s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                }
                break;

            case 12:

                if ((OPC & 0X1F8) == 0x100)
                {
                    //ABCD Ds,Dd
                    sprintf(Dbg_Str, "ABCD D%.1d,D%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                    break;
                }

                if ((OPC & 0X1F8) == 0x140)
                {
                    //EXG.l Ds,Dd
                    TotalSizeMode = SizeMode = 4;
                    sprintf(Dbg_Str, "EXG.L D%.1d,D%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                    break;
                }

                if ((OPC & 0X1F8) == 0x108)
                {
                    //ABCD -(As),-(Ad)
                    sprintf(Dbg_Str, "ABCD -(A%.1d),-(A%.1d)%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                    break;
                }

                if ((OPC & 0X1F8) == 0x148)
                {
                    //EXG.l As,Ad
                    TotalSizeMode = SizeMode = 4;
                    sprintf(Dbg_Str, "EXG.L A%.1d,A%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                    break;
                }

                if ((OPC & 0X1F8) == 0x188)
                {
                    //EXG.l As,Dd
                    TotalSizeMode = SizeMode = 4;
                    sprintf(Dbg_Str, "EXG.L A%.1d,D%.1d%c", OPC & 0x7, (OPC >> 9) & 0x7, 0);
                    break;
                }

                switch((OPC	>> 6) & 0x7)
                {
                case 0: case 1: case 2:
                    //AND.z  a,Dd
                    sprintf(Dbg_Str, "AND%s %s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                    break;

                case 3:
                    //MULU.w  a,Dd
                    TotalSizeMode = SizeMode = 2;
                    sprintf(Dbg_Str, "MULU.W %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                    break;

                case 4: case 5: case 6:
                    //AND.z  Ds,a
                    sprintf(Dbg_Str, "AND%s D%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    break;

                case 7:
                    //MULS.w  a,Dd
                    TotalSizeMode = SizeMode = 2;
                    sprintf(Dbg_Str, "MULS.W %s,D%.1d%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                    break;
                }
                break;

            case 13:

                if ((OPC & 0xC0) == 0xC0)
                    //ADDA.z  a,Ad
                    sprintf(Dbg_Str, "ADDA%s %s,A%.1d%c", Make_Dbg_Size_Str_2((OPC >> 8) & 1), Make_Dbg_EA_Str((OPC >> 8) & 1 + 1, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                else
                {
                    if (OPC & 0x100)
                    {
                        if (!(OPC & 0x38))
                        {
                            //ADDX.z  Ds,Dd
                            sprintf(Dbg_Str, "ADDX%s D%.1d,D%.1d%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                            break;
                        }

                        if ((OPC & 0x38) == 0x8)
                        {
                            //ADDX.z  -(As),-(Ad)
                            sprintf(Dbg_Str, "ADDX%s -(A%.1d),-(A%.1d)%c",	Make_Dbg_Size_Str((OPC >> 6) & 0x3), OPC & 0x7, (OPC >> 9) & 0x7, 0);
                            break;
                        }

                        //ADD.z  Ds,a
                        sprintf(Dbg_Str, "ADD%s D%.1d,%s%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                    }
                    else
                        //ADD.z  a,Dd
                        sprintf(Dbg_Str, "ADD%s %s,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), Make_Dbg_EA_Str((OPC >> 6) & 3, (OPC & 0x38) >> 3, OPC & 0x7), (OPC >> 9) & 0x7, 0);
                }
                break;

            case 14:

                if ((OPC & 0xC0) == 0xC0)
                {
                    switch ((OPC >> 8) & 0x7)
                    {
                    case 0:
                        //ASR.w  #1,a
                        TotalSizeMode = SizeMode = 2;
                        sprintf(Dbg_Str, "ASR.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 1:
                        //ASL.w  #1,a
                        TotalSizeMode = SizeMode = 2;
                        sprintf(Dbg_Str, "ASL.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 2:
                        //LSR.w  #1,a
                        TotalSizeMode = SizeMode = 2;
                        sprintf(Dbg_Str, "LSR.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 3:
                        //LSL.w  #1,a
                        TotalSizeMode = SizeMode = 2;
                        sprintf(Dbg_Str, "LSL.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 4:
                        //ROXR.w  #1,a
                        TotalSizeMode = SizeMode = 2;
                        sprintf(Dbg_Str, "ROXR.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 5:
                        //ROXL.w  #1,a
                        TotalSizeMode = SizeMode = 2;
                        sprintf(Dbg_Str, "ROXL.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 6:
                        //ROR.w  #1,a
                        TotalSizeMode = SizeMode = 2;
                        sprintf(Dbg_Str, "ROR.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    case 7:
                        //ROL.w  #1,a
                        TotalSizeMode = SizeMode = 2;
                        sprintf(Dbg_Str, "ROL.W #1,%s%c", Make_Dbg_EA_Str(1, (OPC & 0x38) >> 3, OPC & 0x7), 0);
                        break;

                    }
                }
                else
                {
                    switch ((OPC >> 3) & 0x3F)
                    {
                    case 0: case 8: case 16:
                        //ASR.z  #k,Dd
                        sprintf(Dbg_Str, "ASR%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 1: case 9: case 17:
                        //LSR.z  #k,Dd
                        sprintf(Dbg_Str, "LSR%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 2: case 10: case 18:
                        //ROXR.z  #k,Dd
                        sprintf(Dbg_Str, "ROXR%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 3: case 11: case 19:
                        //ROR.z  #k,Dd
                        sprintf(Dbg_Str, "ROR%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 4: case 12: case 20:
                        //ASR.z  Ds,Dd
                        sprintf(Dbg_Str, "ASR%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 5: case 13: case 21:
                        //LSR.z  Ds,Dd
                        sprintf(Dbg_Str, "LSR%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 6: case 14: case 22:
                        //ROXR.z  Ds,Dd
                        sprintf(Dbg_Str, "ROXR%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 7: case 15: case 23:
                        //ROR.z  Ds,Dd
                        sprintf(Dbg_Str, "ROR%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 32: case 40: case 48:
                        //ASL.z  #k,Dd
                        sprintf(Dbg_Str, "ASL%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 33: case 41: case 49:
                        //LSL.z  #k,Dd
                        sprintf(Dbg_Str, "LSL%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 34: case 42: case 50:
                        //ROXL.z  #k,Dd
                        sprintf(Dbg_Str, "ROXL%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 35: case 43: case 51:
                        //ROL.z  #k,Dd
                        sprintf(Dbg_Str, "ROL%s #%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 36: case 44: case 52:
                        //ASL.z  Ds,Dd
                        sprintf(Dbg_Str, "ASL%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 37: case 45: case 53:
                        //LSL.z  Ds,Dd
                        sprintf(Dbg_Str, "LSL%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 38: case 46: case 54:
                        //ROXL.z  Ds,Dd
                        sprintf(Dbg_Str, "ROXL%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    case 39: case 47: case 55:
                        //ROL.z  Ds,Dd
                        sprintf(Dbg_Str, "ROL%s D%.1d,D%.1d%c", Make_Dbg_Size_Str((OPC >> 6) & 0x3), (OPC >> 9) & 0x7, OPC & 0x7, 0);
                        break;

                    }
                }
                break;

            case 15:
                //Bad Opcode
                sprintf(Dbg_Str, "BadOpcode");
                break;
            }

            FixResult(Dbg_Str);

            return(Dbg_Str);
        }
    }

    char* Diasm68k::GetOpcodeText(uint32 _uiPosition)
    {
        guiCurrentFlow = _uiPosition;
        char* szAsm = Normal::M68KDisasm(NextWord, NextLong, _uiPosition);

        for (uint32 i = 0; szAsm[i] != '\0'; ++i)
        {
            if (sm_bIsLowerCase)
                szAsm[i] = tolower(szAsm[i]);
            else
                szAsm[i] = toupper(szAsm[i]);
        }

        return szAsm;
    }

    char* Diasm68k::GetOpcodeSpecialText(uint32 _uiPosition)
    {
        guiCurrentFlow = _uiPosition;
        char* szAsm = Special::M68KDisasmSpecial(NextWord, NextLong, _uiPosition);

        for (uint32 i = 0; szAsm[i] != '\0'; ++i)
        {
            if (sm_bIsLowerCase)
                szAsm[i] = tolower(szAsm[i]);
            else
                szAsm[i] = toupper(szAsm[i]);
        }

        return szAsm;
    }
}

namespace DisasmZ80
{
    extern "C"
    {
        extern Z80_Regs Z80;

        extern unsigned char *z80_readmap[64];
        extern unsigned char *z80_writemap[64];
    }

    char Mnemonics[256][16] =
    {
        /* 00 */ "NOP","LD BC,#","LD (BC),A","INC BC","INC B","DEC B","LD B,*","RLCA",
        /* 08 */ "EX AF,AF'","ADD HL,BC","LD A,(BC)","DEC BC","INC C","DEC C","LD C,*","RRCA",
        /* 10 */ "DJNZ *","LD DE,#","LD (DE),A","INC DE","INC D","DEC D","LD D,*","RLA",
        /* 18 */ "JR *","ADD HL,DE","LD A,(DE)","DEC DE","INC E","DEC E","LD E,*","RRA",
        /* 20 */ "JR NZ,*","LD HL,#","LD (#),HL","INC HL","INC H","DEC H","LD H,*","DAA",
        /* 28 */ "JR Z,*","ADD HL,HL","LD HL,(#)","DEC HL","INC L","DEC L","LD L,*","CPL",
        /* 30 */ "JR NC,*","LD SP,#","LD (#),A","INC SP","INC (HL)","DEC (HL)","LD (HL),*","SCF",
        /* 38 */ "JR C,*","ADD HL,SP","LD A,(#)","DEC SP","INC A","DEC A","LD A,*","CCF",
        /* 40 */ "LD B,B","LD B,C","LD B,D","LD B,E","LD B,H","LD B,L","LD B,(HL)","LD B,A",
        /* 48 */ "LD C,B","LD C,C","LD C,D","LD C,E","LD C,H","LD C,L","LD C,(HL)","LD C,A",
        /* 50 */ "LD D,B","LD D,C","LD D,D","LD D,E","LD D,H","LD D,L","LD D,(HL)","LD D,A",
        /* 58 */ "LD E,B","LD E,C","LD E,D","LD E,E","LD E,H","LD E,L","LD E,(HL)","LD E,A",
        /* 60 */ "LD H,B","LD H,C","LD H,D","LD H,E","LD H,H","LD H,L","LD H,(HL)","LD H,A",
        /* 68 */ "LD L,B","LD L,C","LD L,D","LD L,E","LD L,H","LD L,L","LD L,(HL)","LD L,A",
        /* 70 */ "LD (HL),B","LD (HL),C","LD (HL),D","LD (HL),E","LD (HL),H","LD (HL),L","HALT","LD (HL),A",
        /* 78 */ "LD A,B","LD A,C","LD A,D","LD A,E","LD A,H","LD A,L","LD A,(HL)","LD A,A",
        /* 80 */ "ADD B","ADD C","ADD D","ADD E","ADD H","ADD L","ADD (HL)","ADD A",
        /* 88 */ "ADC B","ADC C","ADC D","ADC E","ADC H","ADC L","ADC (HL)","ADC,A",
        /* 90 */ "SUB B","SUB C","SUB D","SUB E","SUB H","SUB L","SUB (HL)","SUB A",
        /* 98 */ "SBC B","SBC C","SBC D","SBC E","SBC H","SBC L","SBC (HL)","SBC A",
        /* A0 */ "AND B","AND C","AND D","AND E","AND H","AND L","AND (HL)","AND A",
        /* A8 */ "XOR B","XOR C","XOR D","XOR E","XOR H","XOR L","XOR (HL)","XOR A",
        /* B0 */ "OR B","OR C","OR D","OR E","OR H","OR L","OR (HL)","OR A",
        /* B8 */ "CP B","CP C","CP D","CP E","CP H","CP L","CP (HL)","CP A",
        /* C0 */ "RET NZ","POP BC","JP NZ,#","JP #","CALL NZ,#","PUSH BC","ADD *","RST 00h",
        /* C8 */ "RET Z","RET","JP Z,#","PFX_CB","CALL Z,#","CALL #","ADC *","RST 08h",
        /* D0 */ "RET NC","POP DE","JP NC,#","OUTA (*)","CALL NC,#","PUSH DE","SUB *","RST 10h",
        /* D8 */ "RET C","EXX","JP C,#","INA (*)","CALL C,#","PFX_DD","SBC *","RST 18h",
        /* E0 */ "RET PO","POP HL","JP PO,#","EX HL,(SP)","CALL PO,#","PUSH HL","AND *","RST 20h",
        /* E8 */ "RET PE","LD PC,HL","JP PE,#","EX DE,HL","CALL PE,#","PFX_ED","XOR *","RST 28h",
        /* F0 */ "RET P","POP AF","JP P,#","DI","CALL P,#","PUSH AF","OR *","RST 30h",
        /* F8 */ "RET M","LD SP,HL","JP M,#","EI","CALL M,#","PFX_FD","CP *","RST 38h"
    };


    char MnemonicsCB[256][16] =
    {
        /* 00 */ "RLC B","RLC C","RLC D","RLC E","RLC H","RLC L","RLC xHL","RLC A",
        /* 08 */ "RRC B","RRC C","RRC D","RRC E","RRC H","RRC L","RRC xHL","RRC A",
        /* 10 */ "RL B","RL C","RL D","RL E","RL H","RL L","RL xHL","RL A",
        /* 18 */ "RR B","RR C","RR D","RR E","RR H","RR L","RR xHL","RR A",
        /* 20 */ "SLA B","SLA C","SLA D","SLA E","SLA H","SLA L","SLA xHL","SLA A",
        /* 28 */ "SRA B","SRA C","SRA D","SRA E","SRA H","SRA L","SRA xHL","SRA A",
        /* 30 */ "SLL B","SLL C","SLL D","SLL E","SLL H","SLL L","SLL xHL","SLL A",
        /* 38 */ "SRL B","SRL C","SRL D","SRL E","SRL H","SRL L","SRL xHL","SRL A",
        /* 40 */ "BIT 0,B","BIT 0,C","BIT 0,D","BIT 0,E","BIT 0,H","BIT 0,L","BIT 0,(HL)","BIT 0,A",
        /* 48 */ "BIT 1,B","BIT 1,C","BIT 1,D","BIT 1,E","BIT 1,H","BIT 1,L","BIT 1,(HL)","BIT 1,A",
        /* 50 */ "BIT 2,B","BIT 2,C","BIT 2,D","BIT 2,E","BIT 2,H","BIT 2,L","BIT 2,(HL)","BIT 2,A",
        /* 58 */ "BIT 3,B","BIT 3,C","BIT 3,D","BIT 3,E","BIT 3,H","BIT 3,L","BIT 3,(HL)","BIT 3,A",
        /* 60 */ "BIT 4,B","BIT 4,C","BIT 4,D","BIT 4,E","BIT 4,H","BIT 4,L","BIT 4,(HL)","BIT 4,A",
        /* 68 */ "BIT 5,B","BIT 5,C","BIT 5,D","BIT 5,E","BIT 5,H","BIT 5,L","BIT 5,(HL)","BIT 5,A",
        /* 70 */ "BIT 6,B","BIT 6,C","BIT 6,D","BIT 6,E","BIT 6,H","BIT 6,L","BIT 6,(HL)","BIT 6,A",
        /* 78 */ "BIT 7,B","BIT 7,C","BIT 7,D","BIT 7,E","BIT 7,H","BIT 7,L","BIT 7,(HL)","BIT 7,A",
        /* 80 */ "RES 0,B","RES 0,C","RES 0,D","RES 0,E","RES 0,H","RES 0,L","RES 0,(HL)","RES 0,A",
        /* 88 */ "RES 1,B","RES 1,C","RES 1,D","RES 1,E","RES 1,H","RES 1,L","RES 1,(HL)","RES 1,A",
        /* 90 */ "RES 2,B","RES 2,C","RES 2,D","RES 2,E","RES 2,H","RES 2,L","RES 2,(HL)","RES 2,A",
        /* 98 */ "RES 3,B","RES 3,C","RES 3,D","RES 3,E","RES 3,H","RES 3,L","RES 3,(HL)","RES 3,A",
        /* A0 */ "RES 4,B","RES 4,C","RES 4,D","RES 4,E","RES 4,H","RES 4,L","RES 4,(HL)","RES 4,A",
        /* A8 */ "RES 5,B","RES 5,C","RES 5,D","RES 5,E","RES 5,H","RES 5,L","RES 5,(HL)","RES 5,A",
        /* B0 */ "RES 6,B","RES 6,C","RES 6,D","RES 6,E","RES 6,H","RES 6,L","RES 6,(HL)","RES 6,A",
        /* B8 */ "RES 7,B","RES 7,C","RES 7,D","RES 7,E","RES 7,H","RES 7,L","RES 7,(HL)","RES 7,A",
        /* C0 */ "SET 0,B","SET 0,C","SET 0,D","SET 0,E","SET 0,H","SET 0,L","SET 0,(HL)","SET 0,A",
        /* C8 */ "SET 1,B","SET 1,C","SET 1,D","SET 1,E","SET 1,H","SET 1,L","SET 1,(HL)","SET 1,A",
        /* D0 */ "SET 2,B","SET 2,C","SET 2,D","SET 2,E","SET 2,H","SET 2,L","SET 2,(HL)","SET 2,A",
        /* D8 */ "SET 3,B","SET 3,C","SET 3,D","SET 3,E","SET 3,H","SET 3,L","SET 3,(HL)","SET 3,A",
        /* E0 */ "SET 4,B","SET 4,C","SET 4,D","SET 4,E","SET 4,H","SET 4,L","SET 4,(HL)","SET 4,A",
        /* E8 */ "SET 5,B","SET 5,C","SET 5,D","SET 5,E","SET 5,H","SET 5,L","SET 5,(HL)","SET 5,A",
        /* F0 */ "SET 6,B","SET 6,C","SET 6,D","SET 6,E","SET 6,H","SET 6,L","SET 6,(HL)","SET 6,A",
        /* F8 */ "SET 7,B","SET 7,C","SET 7,D","SET 7,E","SET 7,H","SET 7,L","SET 7,(HL)","SET 7,A"
    };


    char MnemonicsED[256][16] =
    {
        /* 00 */ "FUCK00","FUCK01","FUCK02","FUCK03","FUCK04","FUCK05","FUCK06","FUCK07",
        /* 08 */ "FUCK08","FUCK09","FUCK0A","FUCK0B","FUCK0C","FUCK0D","FUCK0E","FUCK0F",
        /* 10 */ "FUCK10","FUCK11","FUCK12","FUCK13","FUCK14","FUCK15","FUCK16","FUCK17",
        /* 18 */ "FUCK18","FUCK19","FUCK1A","FUCK1B","FUCK1C","FUCK1D","FUCK1E","FUCK1F",
        /* 20 */ "FUCK20","FUCK21","FUCK22","FUCK23","FUCK24","FUCK25","FUCK26","FUCK27",
        /* 28 */ "FUCK28","FUCK29","FUCK2A","FUCK2B","FUCK2C","FUCK2D","FUCK2E","FUCK2F",
        /* 30 */ "FUCK30","FUCK31","FUCK32","FUCK33","FUCK34","FUCK35","FUCK36","FUCK37",
        /* 38 */ "FUCK38","FUCK39","FUCK3A","FUCK3B","FUCK3C","FUCK3D","FUCK3E","FUCK3F",
        /* 40 */ "IN B,(C)","OUT (C),B","SBC HL,BC","FUCK43","FUCK44","RETN","IM 0","LD I,A",
        /* 48 */ "IN C,(C)","OUT (C),C","ADC HL,BC","FUCK4B","FUCK4C","RETI","FUCK","LD R,A",
        /* 50 */ "IN D,(C)","OUT (C),D","SBC HL,DE","FUCK53","FUCK54","FUCK55","IM 1","LD A,I",
        /* 58 */ "IN E,(C)","OUT (C),E","ADC HL,DE","FUCK5B","FUCK5C","FUCK5D","IM 2","LD A,R",
        /* 60 */ "IN H,(C)","OUT (C),H","SBC HL,HL","FUCK63","FUCK64","FUCK65","FUCK66","RRD",
        /* 68 */ "IN L,(C)","OUT (C),L","ADC HL,HL","FUCK6B","FUCK6C","FUCK6D","FUCK6E","RLD",
        /* 70 */ "IN F,(C)","FUCK","SBC HL,SP","FUCK73","FUCK74","FUCK75","FUCK76","FUCK77",
        /* 78 */ "IN A,(C)","OUT (C),A","ADC HL,SP","FUCK7B","FUCK7C","FUCK7D","FUCK7E","FUCK7F",
        /* 80 */ "FUCK80","FUCK81","FUCK82","FUCK83","FUCK84","FUCK85","FUCK86","FUCK87",
        /* 88 */ "FUCK88","FUCK89","FUCK8A","FUCK8B","FUCK8C","FUCK8D","FUCK8E","FUCK8F",
        /* 90 */ "FUCK90","FUCK91","FUCK92","FUCK93","FUCK94","FUCK95","FUCK96","FUCK97",
        /* 98 */ "FUCK98","FUCK99","FUCK9A","FUCK9B","FUCK9C","FUCK9D","FUCK9E","FUCK9F",
        /* A0 */ "LDI","CPI","INI","OUTI","FUCKA4","FUCKA5","FUCKA6","FUCKA7",
        /* A8 */ "LDD","CPD","IND","OUTD","FUCKAC","FUCKAD","FUCKAE","FUCKAF",
        /* B0 */ "LDIR","CPIR","INIR","OTIR","FUCKB4","FUCKB5","FUCKB6","FUCKB7",
        /* B8 */ "LDDR","CPDR","INDR","OTDR","FUCKBC","FUCKBD","FUCKBE","FUCKBF",
        /* C0 */ "FUCKC0","FUCKC1","FUCKC2","FUCKC3","FUCKC4","FUCKC5","FUCKC6","FUCKC7",
        /* C8 */ "FUCKC8","FUCKC9","FUCKCA","FUCKCB","FUCKCC","FUCKCD","FUCKCE","FUCKCF",
        /* D0 */ "FUCKD0","FUCKD1","FUCKD2","FUCKD3","FUCKD4","FUCKD5","FUCKD6","FUCKD7",
        /* D8 */ "FUCKD8","FUCKD9","FUCKDA","FUCKDB","FUCKDC","FUCKDD","FUCKDE","FUCKDF",
        /* E0 */ "FUCKE0","FUCKE1","FUCKE2","FUCKE3","FUCKE4","FUCKE5","FUCKE6","FUCKE7",
        /* E8 */ "FUCKE8","FUCKE9","FUCKEA","FUCKEB","FUCKEC","FUCKED","FUCKEE","FUCKEF",
        /* F0 */ "FUCKF0","FUCKF1","FUCKF2","FUCKF3","FUCKF4","FUCKF5","FUCKF6","FUCKF7",
        /* F8 */ "FUCKF8","FUCKF9","FUCKFA","FUCKFB","FUCKFC","FUCKFD","FUCKFE","FUCKFF"
    };


    char MnemonicsXX[256][16] =
    {
        /* 00 */ "NOP","LD BC,#","LD (BC),A","INC BC","INC B","DEC B","LD B,*","RLCA",
        /* 08 */ "EX AF,AF'","ADD I%,BC","LD A,(BC)","DEC BC","INC C","DEC C","LD C,*","RRCA",
        /* 10 */ "DJNZ *","LD DE,#","LD (DE),A","INC DE","INC D","DEC D","LD D,*","RLA",
        /* 18 */ "JR *","ADD I%,DE","LD A,(DE)","DEC DE","INC E","DEC E","LD E,*","RRA",
        /* 20 */ "JR NZ,*","LD I%,#","LD (#),I%","INC I%","INC I%h","DEC I%h","LD I%Xh,*","DAA",
        /* 28 */ "JR Z,*","ADD I%,I%","LD I%,(#)","DEC I%","INC I%l","DEC I%l","LD I%l,*","CPL",
        /* 30 */ "JR NC,*","LD SP,#","LD (#),A","INC SP","INC (I%+*)","DEC (I%+*)","LD (I%+*),*","SCF",
        /* 38 */ "JR C,*","ADD I%,SP","LD A,(#)","DEC SP","INC A","DEC A","LD A,*","CCF",
        /* 40 */ "LD B,B","LD B,C","LD B,D","LD B,E","LD B,I%h","LD B,I%l","LD B,(I%+*)","LD B,A",
        /* 48 */ "LD C,B","LD C,C","LD C,D","LD C,E","LD C,I%h","LD C,I%l","LD C,(I%+*)","LD C,A",
        /* 50 */ "LD D,B","LD D,C","LD D,D","LD D,E","LD D,I%h","LD D,I%l","LD D,(I%+*)","LD D,A",
        /* 58 */ "LD E,B","LD E,C","LD E,D","LD E,E","LD E,I%h","LD E,I%l","LD E,(I%+*)","LD E,A",
        /* 60 */ "LD I%h,B","LD I%h,C","LD I%h,D","LD I%h,E","LD I%h,I%h","LD I%h,I%l","LD H,(I%+*)","LD I%h,A",
        /* 68 */ "LD I%l,B","LD I%l,C","LD I%l,D","LD I%l,E","LD I%l,I%h","LD I%l,I%l","LD L,(I%+*)","LD I%l,A",
        /* 70 */ "LD (I%+*),B","LD (I%+*),C","LD (I%+*),D","LD (I%+*),E","LD (I%+*),H","LD (I%+*),L","HALT","LD (I%+*),A",
        /* 78 */ "LD A,B","LD A,C","LD A,D","LD A,E","LD A,I%h","LD A,L","LD A,(I%+*)","LD A,A",
        /* 80 */ "ADD B","ADD C","ADD D","ADD E","ADD I%h","ADD I%l","ADD (I%+*)","ADD A",
        /* 88 */ "ADC B","ADC C","ADC D","ADC E","ADC I%h","ADC I%l","ADC (I%+*)","ADC,A",
        /* 90 */ "SUB B","SUB C","SUB D","SUB E","SUB I%h","SUB I%l","SUB (I%+*)","SUB A",
        /* 98 */ "SBC B","SBC C","SBC D","SBC E","SBC I%h","SBC I%l","SBC (I%+*)","SBC A",
        /* A0 */ "AND B","AND C","AND D","AND E","AND I%h","AND I%l","AND (I%+*)","AND A",
        /* A8 */ "XOR B","XOR C","XOR D","XOR E","XOR I%h","XOR I%l","XOR (I%+*)","XOR A",
        /* B0 */ "OR B","OR C","OR D","OR E","OR I%h","OR I%l","OR (I%+*)","OR A",
        /* B8 */ "CP B","CP C","CP D","CP E","CP I%h","CP I%l","CP (I%+*)","CP A",
        /* C0 */ "RET NZ","POP BC","JP NZ,#","JP #","CALL NZ,#","PUSH BC","ADD *","RST 00h",
        /* C8 */ "RET Z","RET","JP Z,#","PFX_CB","CALL Z,#","CALL #","ADC *","RST 08h",
        /* D0 */ "RET NC","POP DE","JP NC,#","OUTA (*)","CALL NC,#","PUSH DE","SUB *","RST 10h",
        /* D8 */ "RET C","EXX","JP C,#","INA (*)","CALL C,#","PFX_DD","SBC *","RST 18h",
        /* E0 */ "RET PO","POP I%","JP PO,#","EX I%,(SP)","CALL PO,#","PUSH I%","AND *","RST 20h",
        /* E8 */ "RET PE","LD PC,I%","JP PE,#","EX DE,I%","CALL PE,#","PFX_ED","XOR *","RST 28h",
        /* F0 */ "RET P","POP AF","JP P,#","DI","CALL P,#","PUSH AF","OR *","RST 30h",
        /* F8 */ "RET M","LD SP,I%","JP M,#","EI","CALL M,#","PFX_FD","CP *","RST 38h"
    };



    int LabelFlag[256] =
    {
      //0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
        1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, // 1
        1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, // 2
        1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, // 3
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 7
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
        0, 0, 1, 1, 2, 0, 0, 0, 0, 0, 1, 0, 2, 2, 0, 0, // C
        0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, // D
        0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, // E
        0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, // F
    };

    int z80dis(unsigned char *buf, int *Counter, char str[128])
    {
        char S[80],T[80],U[80],*P,*R;
        int I, J;

        if((I=buf[*Counter])<0) return(0);

        memset(S,0,80);
        memset(T,0,80);
        memset(U,0,80);
        memset(str,0,128);

        sprintf(str, "%.4X: %.2X",(*Counter)++,I);

        switch(I)
        {
        case 0xCB: if((I=buf[*Counter])<0) return(0);
            sprintf(U, "%.2X",I);
            strcpy(S,MnemonicsCB[I]);
            (*Counter)++;break;
        case 0xED: if((I=buf[*Counter])<0) return(0);
            sprintf(U, "%.2X",I);
            strcpy(S,MnemonicsED[I]);
            (*Counter)++;break;
        case 0xFD: if((I=buf[*Counter])<0) return(0);
            sprintf(U, "%.2X",I);
            if(I==0xCB){
                (*Counter)++;
                if((I=buf[*Counter])<0) return(0);
                (*Counter)++;
                if((J=buf[*Counter])<0) return(0);
                sprintf(U, "%s%.2X%.2X",U,I,J);
                sprintf(S,"%s, (IY+%.2X)", MnemonicsCB[J], I);
            }else{
                strcpy(S,MnemonicsXX[I]);
                if(P=strchr(S,'%')) *P='Y';}
            (*Counter)++;break;
        case 0xDD: if((I=buf[*Counter])<0) return(0);
            sprintf(U, "%.2X",I);
            if(I==0xCB){
                (*Counter)++;
                if((I=buf[*Counter])<0) return(0);
                (*Counter)++;
                if((J=buf[*Counter])<0) return(0);
                sprintf(U, "%s%.2X%.2X",U,I,J);
                sprintf(S,"%s, (IX+%.2X)", MnemonicsCB[J], I);
            }else{
                strcpy(S,MnemonicsXX[I]);
                if(P=strchr(S,'%')) *P='X';}
            (*Counter)++;break;
        default:   strcpy(S,Mnemonics[I]);
        }

        if(P=strchr(S,'*'))
        {
            if((I=buf[*Counter])<0) return(0);
            sprintf(U, "%s%.2X",U, I);
            *P++='\0';(*Counter)++;
            sprintf(T,"%s%hX",S,I);
            if(R=strchr(P,'*'))
            {
                if((I=buf[*Counter])<0) return(0);
                sprintf(U, "%s%.2X",U, I);
                *R++='\0';(*Counter)++;
                sprintf(strchr(T,'\0'),"%s%hX%s",P,I,R);
            }
            else strcat(T,P);  
        }
        else if(P=strchr(S,'#'))
        {
            if((I=buf[*Counter])<0) return(0);
            (*Counter)++;
            if((J=buf[*Counter])<0) return(0);
            sprintf(U, "%s%.2X%.2X",U, I, J);
            *P++='\0';
            (*Counter)++;
            sprintf(T,"%s%hX%s",S,256*J+I,P);
        }
        else strcpy(T,S);

        strcat(str, U);
        while(strlen(str) < 18) strcat(str, " ");
        strcat(str, T);
        strcat(str, "\n");

        return 1;
    }

    int z80dis2(int addr, char str[128])
    {
        char S[80],T[80],U[80],*P,*R;
        int I, J;
        int iStartAddr = addr;

        I = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];

        memset(S,0,80);
        memset(T,0,80);
        memset(U,0,80);
        memset(str,0,128);

        addr++;

        switch(I)
        {
        case 0xCB:
            I = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];
            sprintf(U, "%.2X",I);
            strcpy(S,MnemonicsCB[I]);
            addr++;
            break;
        case 0xED:
            I = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];
            sprintf(U, "%.2X",I);
            strcpy(S,MnemonicsED[I]);
            addr++;
            break;
        case 0xFD:
            I = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];
            sprintf(U, "%.2X",I);
            if(I==0xCB)
            {
                addr++;
                I = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];
                addr++;
                J = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];
                sprintf(U, "%s%.2X%.2X",U,I,J);
                sprintf(S,"%s, (IY+%.2X)", MnemonicsCB[J], I);
            }
            else
            {
                strcpy(S,MnemonicsXX[I]);
                if(P=strchr(S,'%')) *P='Y';
            }
            addr++;
            break;
        case 0xDD:
            I = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];
            sprintf(U, "%.2X",I);
            if(I==0xCB)
            {
                addr++;
                I = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];
                addr++;
                J = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];
                sprintf(U, "%s%.2X%.2X",U,I,J);
                sprintf(S,"%s, (IX+%.2X)", MnemonicsCB[J], I);
            }
            else
            {
                strcpy(S,MnemonicsXX[I]);
                if(P=strchr(S,'%')) *P = 'X';
            }
            addr++;
            break;
        default:
            strcpy(S,Mnemonics[I]);
            break;
        }

        if(P=strchr(S,'*'))
        {
            I = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];
            sprintf(U, "%s%.2X",U, I);
            *P++='\0';
            addr++;
            sprintf(T,"%s$%hX",S,I);
            if(R=strchr(P,'*'))
            {
                I = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];
                sprintf(U, "%s%.2X",U, I);
                *R++='\0';
                addr++;
                sprintf(strchr(T,'\0'),"%s$%02X%s",P,I,R);
            }
            else
            {
                strcat(T,P);  
            }
        }
        else if(P=strchr(S,'#'))
        {
            I = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];
            addr++;
            J = z80_readmap[(addr & 0xffff) >> 10][(addr & 0xffff) & 0x03FF];
            sprintf(U, "%s%.02X%.02X",U, I, J);
            *P++='\0';
            addr++;
            sprintf(T,"%s$%04X%s",S,(J << 8 | I),P);
        }
        else
            strcpy(T,S);

        //strcat(str, U);
        strcat(str, T);

        for (uint32 i = 0; str[i] != '\0'; ++i)
            str[i] = tolower(str[i]);

        return addr - iStartAddr;
    }

    uint32 SizeMode = 1;

    void FixZ80Result(char* _szText)
    {
        char* szOut = _szText;
        char szTemp[512];
        int iAdd = 0;

        while (*_szText != ' ' && *_szText != '\0')
            szTemp[iAdd++] = *_szText++;

        while (*_szText != '\0')
        {
            if (_szText[0] == '(' && _szText[1] == 'A' && _szText[2] == 'F' && _szText[3] == ')')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?MEM{^}({^}%d{^}%d?!??!?REG{^}AF{^}1?!??!?MEM{^}){^}%d{^}%d?!?", Z80.af, SizeMode, Z80.af, SizeMode);
                _szText += 4;
            }
            else if (_szText[0] == '(' && _szText[1] == 'B' && _szText[2] == 'C' && _szText[3] == ')')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?MEM{^}({^}%d{^}%d?!??!?REG{^}BC{^}2?!??!?MEM{^}){^}%d{^}%d?!?", Z80.bc, SizeMode, Z80.bc, SizeMode);
                _szText += 4;
            }

            else if (_szText[0] == '(' && _szText[1] == 'D' && _szText[2] == 'E' && _szText[3] == ')')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?MEM{^}({^}%d{^}%d?!??!?REG{^}DE{^}3?!??!?MEM{^}){^}%d{^}%d?!?", Z80.de, SizeMode, Z80.de, SizeMode);
                _szText += 4;
            }
            else if (_szText[0] == '(' && _szText[1] == 'H' && _szText[2] == 'L' && _szText[3] == ')')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?MEM{^}({^}%d{^}%d?!??!?REG{^}HL{^}4?!??!?MEM{^}){^}%d{^}%d?!?", Z80.hl, SizeMode, Z80.hl, SizeMode);
                _szText += 4;
            }
            else if (_szText[0] == 'A' && _szText[1] == 'F' && _szText[2] == '\'')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}AF'{^}8?!?");
                _szText += 3;
            }
            else if (_szText[0] == 'B' && _szText[1] == 'C' && _szText[2] == '\'')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}BC'{^}9?!?");
                _szText += 3;
            }
            else if (_szText[0] == 'D' && _szText[1] == 'E' && _szText[2] == '\'')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}DE'{^}10?!?");
                _szText += 3;
            }
            else if (_szText[0] == 'H' && _szText[1] == 'L' && _szText[2] == '\'')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}HL'{^}11?!?");
                _szText += 3;
            }
            else if (_szText[0] == 'A' && _szText[1] == 'F')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}AF{^}1?!?");
                _szText += 2;
            }
            else if (_szText[0] == 'B' && _szText[1] == 'C')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}BC{^}2?!?");
                _szText += 2;
            }
            else if (_szText[0] == 'D' && _szText[1] == 'E')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}DE{^}3?!?");
                _szText += 2;
            }
            else if (_szText[0] == 'H' && _szText[1] == 'L')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}HL{^}4?!?");
                _szText += 2;
            }
            else if (_szText[0] == 'S' && _szText[1] == 'P')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}SP{^}5?!?");
                _szText += 2;
            }
            else if (_szText[0] == 'I' && _szText[1] == 'X')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}IX{^}6?!?");
                _szText += 2;
            }
            else if (_szText[0] == 'I' && _szText[1] == 'Y')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}IY{^}7?!?");
                _szText += 2;
            }
            else if (_szText[0] == 'A')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}A{^}1?!?");
                _szText += 1;
            }
            else if (_szText[0] == 'F')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}F{^}1?!?");
                _szText += 1;
            }
            else if (_szText[0] == 'B')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}B{^}2?!?");
                _szText += 1;
            }
            else if (_szText[0] == 'C')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}C{^}2?!?");
                _szText += 1;
            }
            else if (_szText[0] == 'D')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}D{^}3?!?");
                _szText += 1;
            }
            else if (_szText[0] == 'E')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}E{^}3?!?");
                _szText += 1;
            }
            else if (_szText[0] == 'H')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}H{^}4?!?");
                _szText += 1;
            }
            else if (_szText[0] == 'L')
            {
                iAdd += sprintf(szTemp + iAdd, "?!?REG{^}L{^}4?!?");
                _szText += 1;
            }
            else
            {
                szTemp[iAdd++] = *_szText++;
            }
        }

        szTemp[iAdd] = '\0';

        strcpy(szOut, szTemp);
    }

    int Z80DisFormated(int32 _iAddr, char _szStr[512])
    {
        char szOpcode[512] = "";
        char* pFind;
        int32 iByte = 0;
        int32 iStartAddr = _iAddr;

        iByte = z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff];
        _iAddr++;

        bool bUseFlag = true;

        switch(iByte)
        {
        case 0xcb:
            bUseFlag = false;
            iByte = z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff];
            strcpy(szOpcode, MnemonicsCB[iByte]);
            _iAddr++;
            break;
        case 0xed:
            bUseFlag = false;
            iByte = z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff];
            strcpy(szOpcode, MnemonicsED[iByte]);
            _iAddr++;
            break;
        case 0xfd:
            iByte = z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff];

            if(iByte == 0xcb)
            {
                bUseFlag = false;
                _iAddr++;
                iByte = z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff];
                _iAddr++;
                sprintf(szOpcode,"%s, (IY+%.2X)", MnemonicsCB[z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff]], iByte);
            }
            else
            {
                strcpy(szOpcode, MnemonicsXX[iByte]);

                if(pFind = strchr(szOpcode,'%'))
                    *pFind = 'Y';
            }
            _iAddr++;
            break;
        case 0xDD:
            iByte = z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff];

            if(iByte == 0xcb)
            {
                bUseFlag = false;
                iByte++;
                iByte = z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff];
                iByte++;
                sprintf(szOpcode,"%s, (IX+%.2X)", MnemonicsCB[z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff]], iByte);
            }
            else
            {
                strcpy(szOpcode, MnemonicsXX[iByte]);
                
                if(pFind = strchr(szOpcode,'%'))
                    *pFind = 'X';
            }
            _iAddr++;
            break;

        default:
            strcpy(szOpcode, Mnemonics[iByte]);
            break;
        }

        if (!bUseFlag || (bUseFlag && !LabelFlag[iByte]))
            FixZ80Result(szOpcode);

        if(pFind = strchr(szOpcode, '*'))
        {
            *pFind++ = '\0';

            if (bUseFlag && LabelFlag[iByte])
            {
                iByte = z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff];
                _iAddr++;

                int8 iOffset = (int8)iByte;
                uint32 uiPos = iStartAddr + 2 + iOffset;
                sprintf(_szStr, "%s?!?LINK{^}%s{^}%d?!?", szOpcode, GetLabel(GetZ80MemMap(), uiPos), uiPos);
            }
            else
            {
                iByte = z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff];
                _iAddr++;

                sprintf(_szStr,"%s$%02X", szOpcode, iByte);
            }

            char* pFindNext;

            if(pFindNext = strchr(pFind, '*'))
            {
                iByte = z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff];
                *pFindNext++ = '\0';
                sprintf(strchr(_szStr, '\0'), "%s$%02X%s", pFind, iByte, pFindNext);
            }
            else
            {
                strcat(_szStr, pFind);  
            }
        }
        else if(pFind = strchr(szOpcode, '#'))
        {
            *pFind++ = '\0';

            if (bUseFlag && LabelFlag[iByte])
            {
                iByte = z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff];
                _iAddr++;
                uint32 uiPos = (z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff] << 8) | iByte;
                sprintf(_szStr, "%s?!?LINK{^}%s{^}%d?!?%s", szOpcode, GetLabel(GetZ80MemMap(), uiPos), uiPos, pFind);
            }
            else
            {
                iByte = z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff];
                _iAddr++;
                sprintf(_szStr, "%s$%04X%s", szOpcode, (z80_readmap[(_iAddr & 0xffff) >> 10][(_iAddr & 0xffff) & 0x03ff] << 8 | iByte), pFind);
            }
        }
        else
        {
            strcpy(_szStr, szOpcode);
        }

        for (uint32 i = 0; _szStr[i] != '\0'; ++i)
            _szStr[i] = tolower(_szStr[i]);

        return _iAddr - iStartAddr;
    }

    int z80opsize(int addr)
    {
        char szEmpty[128];
        return z80dis2(addr, szEmpty);
    }
}

DiassemblerHandle GetZ80Disasm()
{
    return (DiassemblerHandle)1;
}

DiassemblerHandle GetM68000Disasm()
{
    return (DiassemblerHandle)2;
}

DiassemblerHandle GetS68000Disasm()
{
    return (DiassemblerHandle)3;
}

void GetOpcodeFormatedText(DiassemblerHandle _Disasm, MemoryHandle _Mem, uint32 _uiPosition, uint32 _uiFlag, char* _szOutput, uint32 _uiOutputSize)
{
    _szOutput[0] = '?';
    _szOutput[1] = '?';
    _szOutput[2] = '?';
    _szOutput[3] = '\0';

    if (_Disasm == GetM68000Disasm())
    {
        Disasm68000::gCurrentCore = &m68k;
        strcpy(_szOutput, Disasm68000::Diasm68k::GetOpcodeSpecialText(_uiPosition));
    }

    if (_Disasm == GetS68000Disasm())
    {
        Disasm68000::gCurrentCore = &s68k;
        strcpy(_szOutput, Disasm68000::Diasm68k::GetOpcodeSpecialText(_uiPosition));
    }

    if (_Disasm == GetZ80Disasm())
    {
        DisasmZ80::Z80DisFormated(_uiPosition, _szOutput);
    }
}

void GetOpcodeText(DiassemblerHandle _Disasm, MemoryHandle _Mem, uint32 _uiPosition, uint32 _uiFlag, char* _szOutput, uint32 _uiOutputSize)
{
    _szOutput[0] = '?';
    _szOutput[1] = '?';
    _szOutput[2] = '?';
    _szOutput[3] = '\0';

    if (_Disasm == GetM68000Disasm())
    {
        Disasm68000::gCurrentCore = &m68k;
        strcpy(_szOutput, Disasm68000::Diasm68k::GetOpcodeText(_uiPosition));
    }

    if (_Disasm == GetS68000Disasm())
    {
        Disasm68000::gCurrentCore = &s68k;
        strcpy(_szOutput, Disasm68000::Diasm68k::GetOpcodeText(_uiPosition));
    }

    if (_Disasm == GetZ80Disasm())
    {
        DisasmZ80::z80dis2(_uiPosition, _szOutput);
    }
}

uint32 GetOpcodeSize(DiassemblerHandle _Disasm, MemoryHandle _Mem, uint32 _uiPosition, uint32 _uiFlag)
{
    if (_Disasm == GetM68000Disasm())
    {
        Disasm68000::gCurrentCore = &m68k;
        Disasm68000::guiCurrentFlow = _uiPosition;
        return Disasm68000::Diasm68k::GetOpcodeSize(Disasm68000::NextWord());
    }

    if (_Disasm == GetS68000Disasm())
    {
        Disasm68000::gCurrentCore = &s68k;
        Disasm68000::guiCurrentFlow = _uiPosition;
        return Disasm68000::Diasm68k::GetOpcodeSize(Disasm68000::NextWord());
    }

    if (_Disasm == GetZ80Disasm())
    {
        return (uint32)DisasmZ80::z80opsize(_uiPosition);
    }

    return 0;
}

bool GetResultOperand(DiassemblerHandle _Disasm, MemoryHandle _Mem, uint32 _uiPosition, uint32 _uiFlag, uint32 _iOperandIndex, uint32* _puiValue)
{
    return false;
}

uint32 GetEndiannessType(DiassemblerHandle _Disasm)
{
    if (_Disasm == GetM68000Disasm()  || _Disasm == GetS68000Disasm())
        return ENDIANNESS_BIG;

    return ENDIANNESS_LITTLE;
}

uint32 GetWordSize(DiassemblerHandle _Disasm)
{
    if (_Disasm == GetM68000Disasm() || _Disasm == GetS68000Disasm())
        return 2;
    
    return 1;
}

uint32 GetLongSize(DiassemblerHandle _Disasm)
{
    if (_Disasm == GetM68000Disasm() || _Disasm == GetS68000Disasm())
        return 4;

    return 2;
}

uint32 GetDumpFormatCount(DiassemblerHandle _Disasm)
{
    return 0;
}

const char* GetDumpFormatName(DiassemblerHandle _Disasm, uint32 _uiIndex)
{
    return NULL;
}

void DumpFormat(DiassemblerHandle _Disasm, uint32 _uiIndex, MemoryHandle _Mem, uint32 _uiStartPosition, uint32 _uiEndPosition, const char* _szFile)
{

}

