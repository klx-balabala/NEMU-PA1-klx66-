#include "cpu/exec/template-start.h"

#define instr push

static void do_execute()
{
    current_sreg = R_SS;
    if(DATA_BYTE == 2) {
        cpu.eip -= 2;
        print_asm_template1();
        MEM_W(cpu.esp, op_src->val);
    }
    else {
        if(DATA_BYTE == 1)
            op_src->val = (int8_t)op_src->val;
        reg_l(R_ESP) -= 4;
        swaddr_write(reg_l(R_ESP), 4, op_src->val);
        print_asm("push 0x%x\n", op_src->val);
    }
    
}

make_instr_helper(i)
#if DATA_BYTE == 2 || DATA_BYTE == 4
make_instr_helper(r)
make_instr_helper(rm)
#endif

#include "cpu/exec/template-end.h"