/*
 * Linker script for SAMD51J19A with UF2 bootloader
 */
MEMORY
{
  FLASH (rx) : ORIGIN = 0x00004000, LENGTH = 0x00080000 - 0x4000 /* 512KB - 16KB bootloader */
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 0x00030000            /* 192KB */
}

/* Define the stack size */
__stack_size__ = 0x2000;

ENTRY(irq_handler_reset)

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        KEEP(*(.vectors))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        __etext = .;
    } > FLASH

    .data : AT (__etext)
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM

    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM

    /* Stack section */
    .stack (NOLOAD):
    {
        . = ALIGN(8);
        __stack_start__ = .;
        . = . + __stack_size__;
        . = ALIGN(8);
        __StackTop = .;
    } > RAM

    /* End of memory */
    _end = .;
}