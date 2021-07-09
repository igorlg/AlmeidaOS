/*
 * interrupt.c
 *
 *  Created on: Jul 7, 2021
 *      Author: Paulo Almeida
 */

#include "kernel/arch/interrupt.h"
#include "kernel/asm/generic.h"
#include "kernel/lib/printk.h"
#include "kernel/lib/bit.h"
#include "kernel/debug/coredump.h"

/*
 * Notes to myself:
 *
 * Next I will have to implement interrupts. My main goal is to grasp the concept properly this time but as an PoC, I
 * would like to get the timer interrupt working so I can add that to printk for funzzies
 *
 * Relevant notes:
 *  -> Interrupt Description Table:
 *      -> The long-mode interrupt-descriptor table (IDT) must contain 64-bit mode interrupt-gate or trap-gate
 *          descriptors for all interrupts or exceptions that can occur while the processor is running in long mode.
 *          Task gates cannot be used in the long-mode IDT, because control transfers through task gates are not
 *          supported in long mode. In long mode, the IDT index is formed by scaling the interrupt vector by 16.
 *
 *      -> An interrupt handler is privileged software designed to identify and respond to the cause of an
 *          interrupt or exception, and return control back to the interrupted software
 *
 *      -> IDT entries are selected using the interrupt vector number rather than a selector value.
 *
 *      -> Vectors 0 through 8, 10 through 14, and 16 through 19 are the predefined interrupts and exceptions;
 *         vectors 32 through 255 are for software-defined interrupts, which are for either software interrupts or
 *         maskable hardware interrupts.
 *
 *  TODO: Add default x86-64 vectors (up to 31) -> Watch out for the reserved ones
 *  TODO: study about PIC and APIC -> So we can get the timer working
 */

typedef struct idt_entry {
    uint16_t low_offset;            // offset bits 0..15
    uint16_t target_selector;       // a code segment selector in GDT or LDT
    uint8_t ist_ign_res;            // IST, Reserver, IGN
    uint8_t attr;                   // attributes ( P=1, DPL=00b, S=0, type=1110b )
    uint16_t mid_offset;            // offset bits 16..31
    uint32_t high_offset;           // offset bits 32..63
    uint32_t reserved;              //reserved / unused
} __packed idt_entry_t;

typedef struct idt_pointer {
    uint16_t limit;
    uintptr_t addr;
} __packed idt_pointer_t;

__aligned(0x10)
static idt_entry_t idt64_table[256];
static idt_pointer_t idt64_table_pointer;

// extern functions
extern void divide_by_zero_isr(void);

__force_inline static void load_idt(idt_pointer_t *idt_ptr) {
    asm volatile (
            "lidt %0 \n"
            :
            : "m" (*idt_ptr)
            : "memory"
    );
}

static void config_idt_vector(uint8_t vector_id, uintptr_t fn) {
//  @formatter:off
    idt_entry_t entry = {
          .low_offset = extract_bit_chunk(0, 15, fn),
          .target_selector = 8  ,                         // 0000100 -> GDT Code Segment -> 0x08
          .ist_ign_res = 0,
          .attr = 0x8e ,                                // attributes ( P=1, DPL=00b, S=0, type=1110b )
          .mid_offset = extract_bit_chunk(16, 31, fn),
          .high_offset = extract_bit_chunk(32, 63, fn),
          .reserved = 0
    };

//  @formatter:on
    idt64_table[vector_id] = entry;
}

void idt_init(void) {
    idt64_table_pointer.addr = (uintptr_t) &idt64_table;
    idt64_table_pointer.limit = ARR_SIZE(idt64_table) - 1;

    config_idt_vector(0, (uintptr_t) &divide_by_zero_isr);
    printk("Loading IDT");
    load_idt(&idt64_table_pointer);
    printk("Enabling interruptions");
}

void interrupt_handler(registers_64_t *regs) {
    printk("Really? Divide by zero bruh?");
    coredump(regs, 10);
    halt();
}

