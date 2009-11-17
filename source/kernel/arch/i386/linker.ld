OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)

ENTRY(_start)

SECTIONS {
    . = 0x100000 + SIZEOF_HEADERS;

    .text : {
        *(.multiboot_header)

        *(.text)
        *(.text.*)

        __smp_trampoline_start = .;
        *(.smp_trampoline)
        __smp_trampoline_end = .;
    }

    . = ALIGN(0x1000);

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    . = ALIGN(0x1000);
    __ro_end = .;
    __data_start = .;

    .data : {
        *(.data)
        *(.data.*)
    }

    . = ALIGN(0x1000);

    .bss : {
        *(.bss)
        *(.bss.*)
    }

    . = ALIGN(0x1000);
    __k_init_start = .;

    .kinit : {
        *(.kernel_init)
    }

    . = ALIGN(0x1000);
    __kernel_end = .;
}
