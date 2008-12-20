#include <types.h>
#include <console.h>

#include <arch/cpu.h>
#include <arch/interrupt.h>

static void dump_registers( registers_t* regs ) {
    kprintf( "Error code: %d\n", regs->error_code );
    kprintf( "EAX=%x EBX=%x ECX=%x EDX=%x\n", regs->eax, regs->ebx, regs->ecx, regs->edx );
    kprintf( "ESI=%x EDI=%x\n", regs->esi, regs->edi );
    kprintf( "CS:EIP=%x:%x\n", regs->cs, regs->eip );
    kprintf( "SS:ESP=%x:%x\n", regs->ss, regs->esp );
}

void handle_division_by_zero( registers_t* regs ) {
    kprintf( "Division by zero!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}

void handle_invalid_opcode( registers_t* regs ) {
    kprintf( "Invalid opcode!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}

void handle_device_not_available( registers_t* regs ) {
    kprintf( "Device not available!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}

void handle_general_protection_fault( registers_t* regs ) {
    kprintf( "General protection fault!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}

void handle_fpu_exception( registers_t* regs ) {
    kprintf( "FPU exception!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}

void handle_sse_exception( registers_t* regs ) {
    kprintf( "SSE exception!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}
