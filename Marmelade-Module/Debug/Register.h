#ifndef Register_h
#define Register_h

#include "API\APIMarmelade.h"

RegistersHandle GetZ80Register();
RegistersHandle GetM68000Register();
RegistersHandle GetS68000Register();

int GetRegisterCount(RegistersHandle _Register);
const char* GetRegisterName(RegistersHandle _Register, int _iRegisterIndex);

const char* GetRegisterValue(RegistersHandle  _Register, int _iRegisterIndex);
void SetRegisterValue(RegistersHandle  _Register, int _iIndex, const char* _szValue);

uint32 GetRunningAddr(RegistersHandle  _Register);
uint32 GetStackAddr(RegistersHandle  _Register);

#endif
