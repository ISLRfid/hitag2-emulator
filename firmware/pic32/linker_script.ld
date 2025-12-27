/* Hi-Tag 2 Emulator - PIC32 Linker Script
 * for PIC32MX795F512L
 */

OUTPUT_FORMAT("elf32-littlemips")
ENTRY(_start)

/* Memory layout for PIC32MX795F512L */
MEMORY {
    /* Flash memory (512 KB) */
    kseg0_program_mem : ORIGIN = 0x9D000000, LENGTH = 0x80000
    
    /* Boot flash (12 KB) */
    kseg1_boot_mem : ORIGIN = 0xBFC00000, LENGTH = 0x3000
    
    /* Data memory (128 KB) */
    kseg1_data_mem : ORIGIN = 0xA0000000, LENGTH = 0x20000
    
    /* SFR (special function registers) */
    sfr : ORIGIN = 0xBF800000, LENGTH = 0x100000
}

/* Stack size */
_stack_size = 2K;

/* Heap size (can be overridden by linker flag) */
_min_heap_size = 1K;

SECTIONS
{
    /* .reset section - boot vector */
    .reset :
    {
        KEEP(*(.reset))
    } > kseg1_boot_mem

    /* .text section - code */
    .text :
    {
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
    } > kseg0_program_mem

    /* .rodata section - read-only data */
    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
    } > kseg0_program_mem

    /* .data section - initialized data */
    .data :
    {
        _sidata = .;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        _edata = .;
    } > kseg1_data_mem AT> kseg0_program_mem

    /* .sdata section - small data */
    .sdata :
    {
        *(.sdata)
        *(.sdata.*)
        *(.gnu.linkonce.s.*)
    } > kseg1_data_mem AT> kseg0_program_mem

    /* .bss section - uninitialized data */
    .bss :
    {
        _sbss = .;
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        _ebss = .;
    } > kseg1_data_mem AT> kseg0_program_mem

    /* .sbss section - small uninitialized data */
    .sbss :
    {
        *(.sbss)
        *(.sbss.*)
        *(.gnu.linkonce.sb.*)
    } > kseg1_data_mem AT> kseg0_program_mem

    /* Heap */
    .heap :
    {
        . = ALIGN(4);
        _heap_start = .;
        . += _min_heap_size;
        _heap_end = .;
    } > kseg1_data_mem

    /* Stack */
    .stack :
    {
        . = ALIGN(4);
        _stack_end = .;
        . += _stack_size;
        _stack_start = .;
    } > kseg1_data_mem

    /* Debug info */
    .debug_info     0 : { *(.debug_info) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
}
