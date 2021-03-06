#include "cpu/exec/template-start.h"

#define instr je

static void do_execute()
{
    DATA_TYPE_S tmp = op_src->val;
    print_asm("je %x\n", cpu.eip + tmp + DATA_BYTE + 1);
    if(cpu.EFLAGS.ZF == 1)
    cpu.eip += tmp;
}

make_instr_helper(i);

#include "cpu/exec/template-end.h"