#ifndef Disasm_h
#define Disasm_h

#include "API\APIMarmelade.h"

DiassemblerHandle GetZ80Disasm();
DiassemblerHandle GetM68000Disasm();
DiassemblerHandle GetS68000Disasm();

void GetOpcodeFormatedText(DiassemblerHandle _Disasm, MemoryHandle _Mem, uint32 _uiPosition, uint32 _uiFlag, char* _szOutput, uint32 _uiOutputSize);
void GetOpcodeText(DiassemblerHandle _Disasm, MemoryHandle _Mem, uint32 _uiPosition, uint32 _uiFlag, char* _szOutput, uint32 _uiOutputSize);
uint32  GetOpcodeSize(DiassemblerHandle _Disasm, MemoryHandle _Mem, uint32 _uiPosition, uint32 _uiFlag);
bool GetResultOperand(DiassemblerHandle _Disasm, MemoryHandle _Mem, uint32 _uiPosition, uint32 _uiFlag, uint32 _iOperandIndex, uint32* _puiValue);

uint32 GetEndiannessType(DiassemblerHandle _Disasm);
uint32 GetWordSize(DiassemblerHandle _Disasm);
uint32 GetLongSize(DiassemblerHandle _Disasm);

uint32 GetDumpFormatCount(DiassemblerHandle _Disasm);
const char* GetDumpFormatName(DiassemblerHandle _Disasm, uint32 _uiIndex);
void DumpFormat(DiassemblerHandle _Disasm, uint32 _uiIndex, MemoryHandle _Mem, uint32 _uiStartPosition, uint32 _uiEndPosition, const char* _szFile);

#endif