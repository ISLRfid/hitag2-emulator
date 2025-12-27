; Hi-Tag 2 Emulator - PIC32 Startup Assembly
; Entry point and vector table

#include <xc32.inc>

.section .reset, code
.global _start

_start:
    ; Disable interrupts
    di
    
    ; Initialize stack pointer
    la      $sp, _stack_start
    
    ; Initialize GP
    move    $gp, _gp
    
    ; Zero BSS section
    la      $t0, _sbss
    la      $t1, _ebss
bss_loop:
    beq     $t0, $t1, bss_done
    sw      $zero, 0($t0)
    addiu   $t0, $t0, 4
    j       bss_loop
bss_done:
    
    ; Copy initialized data from flash to RAM
    la      $t0, _sidata
    la      $t1, _sdata
    la      $t2, _edata
data_loop:
    bgeu    $t1, $t2, data_done
    lw      $t3, 0($t0)
    sw      $t3, 0($t1)
    addiu   $t0, $t0, 4
    addiu   $t1, $t1, 4
    j       data_loop
data_done:
    
    ; Call constructors
    la      $t0, __libc_fini_array
    beqz    $t0, no_fini
    addiu   $t0, $t0, -4
fini_loop:
    lw      $t1, 4($t0)
    beqz    $t1, fini_done
    addiu   $t0, $t0, 4
    move    $t9, $t1
    jalr    $t9
    j       fini_loop
fini_done:
no_fini:
    
    ; Call main application
    jal     main
    
    ; If main returns, loop forever
loop_forever:
    j       loop_forever

.global __libc_fini_array
.global __libc_init_array

.section .data
.global __libc_fini_array
__libc_fini_array:
    .word 0

__libc_init_array:
    .word 0
