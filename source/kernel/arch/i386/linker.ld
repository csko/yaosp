OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)

ENTRY(_start)

SECTIONS {
    . = 0x100000;

    .text : {
        *(.multiboot_header);
        *(.text);
        __smp_trampoline_start = .;
        *(.smp_trampoline);
        __smp_trampoline_end = .;
    }

    . = ALIGN(0x1000);
    __text_end = .;

    .data : {
        *(.data);
    }

    .bss : {
        . = ALIGN(0x1000);
        *(.bss);
    }

    . = ALIGN(0x1000);
    __kernel_end = .;
}
