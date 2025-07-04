// ARM Assembly startup code for SAMD51 defines reset vector table and reset_handler where execution starts.

.syntax unified
.cpu cortex-m4
.thumb

// External symbols from linker script
.extern __data_start__
.extern __data_end__
.extern __bss_start__
.extern __bss_end__
.extern __etext
.extern __StackTop
.extern main

// Interrupt vector table placed in .isr_vector section
.section .isr_vector,"a",%progbits
.type vectors, %object
.size vectors, .-vectors

vectors:
    .word __StackTop                // 0 - Initial Stack Pointer Value
    .word irq_handler_reset         // 1 - Reset
    .word irq_handler_nmi           // 2 - NMI
    .word irq_handler_hard_fault    // 3 - Hard Fault
    .word irq_handler_mm_fault      // 4 - MM Fault
    .word irq_handler_bus_fault     // 5 - Bus Fault
    .word irq_handler_usage_fault   // 6 - Usage Fault
    .word 0                         // 7 - Reserved
    .word 0                         // 8 - Reserved
    .word 0                         // 9 - Reserved
    .word 0                         // 10 - Reserved
    .word irq_handler_sv_call       // 11 - SVCall
    .word irq_handler_debug_mon     // 12 - Debug
    .word 0                         // 13 - Reserved
    .word irq_handler_pend_sv       // 14 - PendSV
    .word irq_handler_sys_tick      // 15 - SysTick
    
    // Peripheral handlers (16-152)
    .word irq_handler_pm            // 16 - Power Manager
    .word irq_handler_mclk          // 17 - Main Clock
    .word irq_handler_oscctrl_0     // 18 - Oscillators Control 0
    .word irq_handler_oscctrl_1     // 19 - Oscillators Control 1
    .word irq_handler_oscctrl_2     // 20 - Oscillators Control 2
    .word irq_handler_oscctrl_3     // 21 - Oscillators Control 3
    .word irq_handler_oscctrl_4     // 22 - Oscillators Control 4
    .word irq_handler_osc32kctrl    // 23 - 32kHz Oscillators Control
    .word irq_handler_supc_0        // 24 - Supply Controller 0
    .word irq_handler_supc_1        // 25 - Supply Controller 1
    .word irq_handler_wdt           // 26 - Watchdog Timer
    .word irq_handler_rtc           // 27 - Real-Time Counter
    .word irq_handler_eic_0         // 28 - External Interrupt Controller 0
    .word irq_handler_eic_1         // 29 - External Interrupt Controller 1
    .word irq_handler_eic_2         // 30 - External Interrupt Controller 2
    .word irq_handler_eic_3         // 31 - External Interrupt Controller 3
    .word irq_handler_eic_4         // 32 - External Interrupt Controller 4
    .word irq_handler_eic_5         // 33 - External Interrupt Controller 5
    .word irq_handler_eic_6         // 34 - External Interrupt Controller 6
    .word irq_handler_eic_7         // 35 - External Interrupt Controller 7
    .word irq_handler_eic_8         // 36 - External Interrupt Controller 8
    .word irq_handler_eic_9         // 37 - External Interrupt Controller 9
    .word irq_handler_eic_10        // 38 - External Interrupt Controller 10
    .word irq_handler_eic_11        // 39 - External Interrupt Controller 11
    .word irq_handler_eic_12        // 40 - External Interrupt Controller 12
    .word irq_handler_eic_13        // 41 - External Interrupt Controller 13
    .word irq_handler_eic_14        // 42 - External Interrupt Controller 14
    .word irq_handler_eic_15        // 43 - External Interrupt Controller 15
    .word irq_handler_freqm         // 44 - Frequency Meter
    .word irq_handler_nvmctrl_0     // 45 - Non-Volatile Memory Controller 0
    .word irq_handler_nvmctrl_1     // 46 - Non-Volatile Memory Controller 1
    .word irq_handler_dmac_0        // 47 - Direct Memory Access Controller 0
    .word irq_handler_dmac_1        // 48 - Direct Memory Access Controller 1
    .word irq_handler_dmac_2        // 49 - Direct Memory Access Controller 2
    .word irq_handler_dmac_3        // 50 - Direct Memory Access Controller 3
    .word irq_handler_dmac_4        // 51 - Direct Memory Access Controller 4
    .word irq_handler_evsys_0       // 52 - Event System Interface 0
    .word irq_handler_evsys_1       // 53 - Event System Interface 1
    .word irq_handler_evsys_2       // 54 - Event System Interface 2
    .word irq_handler_evsys_3       // 55 - Event System Interface 3
    .word irq_handler_evsys_4       // 56 - Event System Interface 4
    .word irq_handler_pac           // 57 - Peripheral Access Controller
    .word 0                         // 58 - Reserved
    .word 0                         // 59 - Reserved
    .word 0                         // 60 - Reserved
    .word irq_handler_ramecc        // 61 - RAM ECC
    .word irq_handler_sercom0_0     // 62 - Serial Communication Interface 0.0
    .word irq_handler_sercom0_1     // 63 - Serial Communication Interface 0.1
    .word irq_handler_sercom0_2     // 64 - Serial Communication Interface 0.2
    .word irq_handler_sercom0_3     // 65 - Serial Communication Interface 0.3
    .word irq_handler_sercom1_0     // 66 - Serial Communication Interface 1.0
    .word irq_handler_sercom1_1     // 67 - Serial Communication Interface 1.1
    .word irq_handler_sercom1_2     // 68 - Serial Communication Interface 1.2
    .word irq_handler_sercom1_3     // 69 - Serial Communication Interface 1.3
    .word irq_handler_sercom2_0     // 70 - Serial Communication Interface 2.0
    .word irq_handler_sercom2_1     // 71 - Serial Communication Interface 2.1
    .word irq_handler_sercom2_2     // 72 - Serial Communication Interface 2.2
    .word irq_handler_sercom2_3     // 73 - Serial Communication Interface 2.3
    .word irq_handler_sercom3_0     // 74 - Serial Communication Interface 3.0
    .word irq_handler_sercom3_1     // 75 - Serial Communication Interface 3.1
    .word irq_handler_sercom3_2     // 76 - Serial Communication Interface 3.2
    .word irq_handler_sercom3_3     // 77 - Serial Communication Interface 3.3
    .word irq_handler_sercom4_0     // 78 - Serial Communication Interface 4.0
    .word irq_handler_sercom4_1     // 79 - Serial Communication Interface 4.1
    .word irq_handler_sercom4_2     // 80 - Serial Communication Interface 4.2
    .word irq_handler_sercom4_3     // 81 - Serial Communication Interface 4.3
    .word irq_handler_sercom5_0     // 82 - Serial Communication Interface 5.0
    .word irq_handler_sercom5_1     // 83 - Serial Communication Interface 5.1
    .word irq_handler_sercom5_2     // 84 - Serial Communication Interface 5.2
    .word irq_handler_sercom5_3     // 85 - Serial Communication Interface 5.3
    .word irq_handler_sercom6_0     // 86 - Serial Communication Interface 6.0
    .word irq_handler_sercom6_1     // 87 - Serial Communication Interface 6.1
    .word irq_handler_sercom6_2     // 88 - Serial Communication Interface 6.2
    .word irq_handler_sercom6_3     // 89 - Serial Communication Interface 6.3
    .word irq_handler_sercom7_0     // 90 - Serial Communication Interface 7.0
    .word irq_handler_sercom7_1     // 91 - Serial Communication Interface 7.1
    .word irq_handler_sercom7_2     // 92 - Serial Communication Interface 7.2
    .word irq_handler_sercom7_3     // 93 - Serial Communication Interface 7.3
    .word irq_handler_can0          // 94 - Control Area Network 0
    .word irq_handler_can1          // 95 - Control Area Network 1
    .word irq_handler_usb_0         // 96 - Universal Serial Bus 0
    .word irq_handler_usb_1         // 97 - Universal Serial Bus 1
    .word irq_handler_usb_2         // 98 - Universal Serial Bus 2
    .word irq_handler_usb_3         // 99 - Universal Serial Bus 3
    .word irq_handler_gmac          // 100 - Ethernet MAC
    .word irq_handler_tcc0_0        // 101 - Timer Counter Control 0.0
    .word irq_handler_tcc0_1        // 102 - Timer Counter Control 0.1
    .word irq_handler_tcc0_2        // 103 - Timer Counter Control 0.2
    .word irq_handler_tcc0_3        // 104 - Timer Counter Control 0.3
    .word irq_handler_tcc0_4        // 105 - Timer Counter Control 0.4
    .word irq_handler_tcc0_5        // 106 - Timer Counter Control 0.5
    .word irq_handler_tcc0_6        // 107 - Timer Counter Control 0.6
    .word irq_handler_tcc1_0        // 108 - Timer Counter Control 1.0
    .word irq_handler_tcc1_1        // 109 - Timer Counter Control 1.1
    .word irq_handler_tcc1_2        // 110 - Timer Counter Control 1.2
    .word irq_handler_tcc1_3        // 111 - Timer Counter Control 1.3
    .word irq_handler_tcc1_4        // 112 - Timer Counter Control 1.4
    .word irq_handler_tcc2_0        // 113 - Timer Counter Control 2.0
    .word irq_handler_tcc2_1        // 114 - Timer Counter Control 2.1
    .word irq_handler_tcc2_2        // 115 - Timer Counter Control 2.2
    .word irq_handler_tcc2_3        // 116 - Timer Counter Control 2.3
    .word irq_handler_tcc3_0        // 117 - Timer Counter Control 3.0
    .word irq_handler_tcc3_1        // 118 - Timer Counter Control 3.1
    .word irq_handler_tcc3_2        // 119 - Timer Counter Control 3.2
    .word irq_handler_tcc4_0        // 120 - Timer Counter Control 4.0
    .word irq_handler_tcc4_1        // 121 - Timer Counter Control 4.1
    .word irq_handler_tcc4_2        // 122 - Timer Counter Control 4.2
    .word irq_handler_tc0           // 123 - Basic Timer Counter 0
    .word irq_handler_tc1           // 124 - Basic Timer Counter 1
    .word irq_handler_tc2           // 125 - Basic Timer Counter 2
    .word irq_handler_tc3           // 126 - Basic Timer Counter 3
    .word irq_handler_tc4           // 127 - Basic Timer Counter 4
    .word irq_handler_tc5           // 128 - Basic Timer Counter 5
    .word irq_handler_tc6           // 129 - Basic Timer Counter 6
    .word irq_handler_tc7           // 130 - Basic Timer Counter 7
    .word irq_handler_pdec_0        // 131 - Quadrature Decoder 0
    .word irq_handler_pdec_1        // 132 - Quadrature Decoder 1
    .word irq_handler_pdec_2        // 133 - Quadrature Decoder 2
    .word irq_handler_adc0_0        // 134 - Analog to Digital Converter 0.0
    .word irq_handler_adc0_1        // 135 - Analog to Digital Converter 0.1
    .word irq_handler_adc1_0        // 136 - Analog to Digital Converter 1.0
    .word irq_handler_adc1_1        // 137 - Analog to Digital Converter 1.1
    .word irq_handler_ac            // 138 - Analog Comparators
    .word irq_handler_dac_0         // 139 - Digital to Analog Converter 0
    .word irq_handler_dac_1         // 140 - Digital to Analog Converter 1
    .word irq_handler_dac_2         // 141 - Digital to Analog Converter 2
    .word irq_handler_dac_3         // 142 - Digital to Analog Converter 3
    .word irq_handler_dac_4         // 143 - Digital to Analog Converter 4
    .word irq_handler_i2s           // 144 - Inter-IC Sound Interface
    .word irq_handler_pcc           // 145 - Parallel Capture Controller
    .word irq_handler_aes           // 146 - Advanced Encryption Standard
    .word irq_handler_trng          // 147 - True Random Generator
    .word irq_handler_icm           // 148 - Integrity Check Monitor
    .word irq_handler_pukcc         // 149 - PUblic-Key Cryptography Controller
    .word irq_handler_qspi          // 150 - Quad SPI interface
    .word irq_handler_sdhc0         // 151 - SD/MMC Host Controller 0
    .word irq_handler_sdhc1         // 152 - SD/MMC Host Controller 1

// Code section
.section .text
.thumb_func
.global irq_handler_reset
.type irq_handler_reset, %function

irq_handler_reset:
    // Copy .data section from flash to RAM
    ldr r0, =__etext       // Source address (end of .text in flash)
    ldr r1, =__data_start__ // Destination start
    ldr r2, =__data_end__   // Destination end
    
copy_data_loop:
    cmp r1, r2              // Check if we've copied all data
    bge copy_data_done      // Branch if r1 >= r2
    ldr r3, [r0], #4        // Load word from source, post-increment
    str r3, [r1], #4        // Store word to dest, post-increment
    b copy_data_loop

copy_data_done:
    // Zero .bss section
    ldr r1, =__bss_start__  // Start of .bss
    ldr r2, =__bss_end__    // End of .bss
    mov r3, #0              // Zero value

zero_bss_loop:
    cmp r1, r2              // Check if we've zeroed all bss
    bge zero_bss_done       // Branch if r1 >= r2
    str r3, [r1], #4        // Store zero, post-increment
    b zero_bss_loop

zero_bss_done:
    // Set vector table offset register
    ldr r0, =vectors
    ldr r1, =0xE000ED08     // SCB->VTOR address
    str r0, [r1]

    // Enable FPU (Cortex-M4 with FPU)
    ldr r0, =0xE000ED88     // SCB->CPACR address
    ldr r1, [r0]
    orr r1, r1, #(0xF << 20) // Enable CP10 and CP11 (FPU)
    str r1, [r0]
    dsb                     // Data synchronization barrier
    isb                     // Instruction synchronization barrier

    // Call main function
    bl main

    // If main returns, loop forever
infinite_loop:
    b infinite_loop

.size irq_handler_reset, .-irq_handler_reset

.end
