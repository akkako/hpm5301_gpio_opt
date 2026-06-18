#include "JTAG_DP.h"
#include "setting.h"

void PORT_JTAG_SETUP(void)
{
    IO_PORT_JTAG_SETUP();
}

void JTAG_Sequence(uint32_t info, const uint8_t *tdi, uint8_t *tdo)
{
    IO_JTAG_Sequence(info, tdi, tdo);
}

uint32_t JTAG_ReadIDCode(void)
{
    return IO_JTAG_ReadIDCode();
}

void JTAG_WriteAbort(uint32_t data)
{
    IO_JTAG_WriteAbort(data);
}

void JTAG_IR(uint32_t ir)
{
    IO_JTAG_IR(ir);
}

uint8_t JTAG_Transfer(uint32_t request, uint32_t *data)
{
    return IO_JTAG_Transfer(request, data);
}
