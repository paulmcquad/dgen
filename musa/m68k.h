/* ======================================================================== */
/* ========================= LICENSING & COPYRIGHT ======================== */
/* ======================================================================== */
/*
 *                                  MUSASHI
 *                                Version 4.10
 *
 * A portable Motorola M680x0 processor emulation engine.
 * Copyright Karl Stenerud.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef M68K__HEADER
#define M68K__HEADER

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(x)         (sizeof(x) / sizeof(x[0]))
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

/* ======================================================================== */
/* ============================= CONFIGURATION ============================ */
/* ======================================================================== */

/* Import the configuration for this build */
#ifdef MUSASHI_CNF
#include MUSASHI_CNF
#else
#include "m68kconf.h"
#endif

/* ======================================================================== */
/* ============================ GENERAL DEFINES =========================== */

/* ======================================================================== */

/* There are 7 levels of interrupt to the 68K.
 * A transition from < 7 to 7 will cause a non-maskable interrupt (NMI).
 */
#define M68K_IRQ_NONE 0
#define M68K_IRQ_1    1
#define M68K_IRQ_2    2
#define M68K_IRQ_3    3
#define M68K_IRQ_4    4
#define M68K_IRQ_5    5
#define M68K_IRQ_6    6
#define M68K_IRQ_7    7


/* Special interrupt acknowledge values.
 * Use these as special returns from the interrupt acknowledge callback
 * (specified later in this header).
 */

/* Causes an interrupt autovector (0x18 + interrupt level) to be taken.
 * This happens in a real 68K if VPA or AVEC is asserted during an interrupt
 * acknowledge cycle instead of DTACK.
 */
#define M68K_INT_ACK_AUTOVECTOR    0xffffffff

/* Causes the spurious interrupt vector (0x18) to be taken
 * This happens in a real 68K if BERR is asserted during the interrupt
 * acknowledge cycle (i.e. no devices responded to the acknowledge).
 */
#define M68K_INT_ACK_SPURIOUS      0xfffffffe


/* CPU types for use in m68k_set_cpu_type() */
enum
{
	M68K_CPU_TYPE_INVALID,
	M68K_CPU_TYPE_68000,
	M68K_CPU_TYPE_68008,
	M68K_CPU_TYPE_68010,
	M68K_CPU_TYPE_68EC020,
	M68K_CPU_TYPE_68020,
	M68K_CPU_TYPE_68030,	/* Supported by disassembler ONLY */
	M68K_CPU_TYPE_68040		/* Supported by disassembler ONLY */
};

/* Registers used by m68k_get_reg() and m68k_set_reg() */
typedef enum
{
	/* Real registers */
	M68K_REG_D0,		/* Data registers */
	M68K_REG_D1,
	M68K_REG_D2,
	M68K_REG_D3,
	M68K_REG_D4,
	M68K_REG_D5,
	M68K_REG_D6,
	M68K_REG_D7,
	M68K_REG_A0,		/* Address registers */
	M68K_REG_A1,
	M68K_REG_A2,
	M68K_REG_A3,
	M68K_REG_A4,
	M68K_REG_A5,
	M68K_REG_A6,
	M68K_REG_A7,
	M68K_REG_PC,		/* Program Counter */
	M68K_REG_SR,		/* Status Register */
	M68K_REG_SP,		/* The current Stack Pointer (located in A7) */
	M68K_REG_USP,		/* User Stack Pointer */
	M68K_REG_ISP,		/* Interrupt Stack Pointer */
	M68K_REG_MSP,		/* Master Stack Pointer */
	M68K_REG_SFC,		/* Source Function Code */
	M68K_REG_DFC,		/* Destination Function Code */
	M68K_REG_VBR,		/* Vector Base Register */
	M68K_REG_CACR,		/* Cache Control Register */
	M68K_REG_CAAR,		/* Cache Address Register */

	/* Assumed registers */
	/* These are cheat registers which emulate the 1-longword prefetch
	 * present in the 68000 and 68010.
	 */
	M68K_REG_PREF_ADDR,	/* Last prefetch address */
	M68K_REG_PREF_DATA,	/* Last prefetch data */

	/* Convenience registers */
	M68K_REG_PPC,		/* Previous value in the program counter */
	M68K_REG_IR,		/* Instruction register */
	M68K_REG_CPU_TYPE	/* Type of CPU being run */
} m68k_register_t;

typedef struct
{
	/* If any of the R/W/X bits is disabled, call the associated
	 * m68k_(read|write)_*() function instead of accessing it
	 * directly.
	 */
	unsigned int r:1;    /* Read */
	unsigned int w:1;    /* Write */
	unsigned int x:1;    /* Execute (PC) */
	unsigned int swab:1; /* Swap bytes during access */
	unsigned int addr;   /* First address in M68K memory */
	unsigned int size;   /* Size of this region in M68K memory */
	unsigned int mask;   /* Mask addresses with mask before accessing mem */
	void *mem;           /* First address in program memory */
} m68k_mem_t;

/* ======================================================================== */
/* ====================== FUNCTIONS CALLED BY THE CPU ===================== */
/* ======================================================================== */

/* You will have to implement these functions */

/* read/write functions called by the CPU to access memory.
 * while values used are 32 bits, only the appropriate number
 * of bits are relevant (i.e. in write_memory_8, only the lower 8 bits
 * of value should be written to memory).
 *
 * NOTE: I have separated the immediate and PC-relative memory fetches
 *       from the other memory fetches because some systems require
 *       differentiation between PROGRAM and DATA fetches (usually
 *       for security setups such as encryption).
 *       This separation can either be achieved by setting
 *       M68K_SEPARATE_READS in m68kconf.h and defining
 *       the read functions, or by setting M68K_EMULATE_FC and
 *       making a function code callback function.
 *       Using the callback offers better emulation coverage
 *       because you can also monitor whether the CPU is in SYSTEM or
 *       USER mode, but it is also slower.
 */

/* Read from anywhere */
unsigned int  m68k_read_memory_8(unsigned int address);
unsigned int  m68k_read_memory_16(unsigned int address);
unsigned int  m68k_read_memory_32(unsigned int address);

/* Read data immediately following the PC */
unsigned int  m68k_read_immediate_16(unsigned int address);
unsigned int  m68k_read_immediate_32(unsigned int address);

/* Read data relative to the PC */
unsigned int  m68k_read_pcrelative_8(unsigned int address);
unsigned int  m68k_read_pcrelative_16(unsigned int address);
unsigned int  m68k_read_pcrelative_32(unsigned int address);

/* Memory access for the disassembler */
unsigned int m68k_read_disassembler_8  (unsigned int address);
unsigned int m68k_read_disassembler_16 (unsigned int address);
unsigned int m68k_read_disassembler_32 (unsigned int address);

/* Write to anywhere */
void m68k_write_memory_8(unsigned int address, unsigned int value);
void m68k_write_memory_16(unsigned int address, unsigned int value);
void m68k_write_memory_32(unsigned int address, unsigned int value);

/* Special call to simulate undocumented 68k behavior when move.l with a
 * predecrement destination mode is executed.
 * To simulate real 68k behavior, first write the high word to
 * [address+2], and then write the low word to [address].
 *
 * Enable this functionality with M68K_SIMULATE_PD_WRITES in m68kconf.h.
 */
void m68k_write_memory_32_pd(unsigned int address, unsigned int value);

/* Register an array of memory regions directly accessible from Musashi
 * for faster direct access without having to use the above functions.
 * See m68k_mem_t definition.
 *
 * Enable this functionality with M68K_REGISTER_MEMORY in m68kconf.h.
 */
void m68k_register_memory(m68k_mem_t memory[], unsigned int len);

/* ======================================================================== */
/* ============================== CALLBACKS =============================== */
/* ======================================================================== */

/* These functions allow you to set callbacks to the host when specific events
 * occur.  Note that you must enable the corresponding value in m68kconf.h
 * in order for these to do anything useful.
 * Note: I have defined default callbacks which are used if you have enabled
 * the corresponding #define in m68kconf.h but either haven't assigned a
 * callback or have assigned a callback of NULL.
 */

/* Set the callback for an interrupt acknowledge.
 * You must enable M68K_EMULATE_INT_ACK in m68kconf.h.
 * The CPU will call the callback with the interrupt level being acknowledged.
 * The host program must return either a vector from 0x02-0xff, or one of the
 * special interrupt acknowledge values specified earlier in this header.
 * If this is not implemented, the CPU will always assume an autovectored
 * interrupt, and will automatically clear the interrupt request when it
 * services the interrupt.
 * Default behavior: return M68K_INT_ACK_AUTOVECTOR.
 */
void m68k_set_int_ack_callback(int  (*callback)(int int_level));


/* Set the callback for a breakpoint acknowledge (68010+).
 * You must enable M68K_EMULATE_BKPT_ACK in m68kconf.h.
 * The CPU will call the callback with whatever was in the data field of the
 * BKPT instruction for 68020+, or 0 for 68010.
 * Default behavior: do nothing.
 */
void m68k_set_bkpt_ack_callback(void (*callback)(unsigned int data));


/* Set the callback for the RESET instruction.
 * You must enable M68K_EMULATE_RESET in m68kconf.h.
 * The CPU calls this callback every time it encounters a RESET instruction.
 * Default behavior: do nothing.
 */
void m68k_set_reset_instr_callback(void  (*callback)(void));

/* Set the callback for the CMPI.L #v, Dn instruction.
 * You must enable M68K_CMPILD_HAS_CALLBACK in m68kconf.h.
 * The CPU calls this callback every time it encounters a CMPI.L #v, Dn instruction.
 * Default behavior: do nothing.
 */
void m68k_set_cmpild_instr_callback(void  (*callback)(unsigned int val, int reg));


/* Set the callback for the RTE instruction.
 * You must enable M68K_RTE_HAS_CALLBACK in m68kconf.h.
 * The CPU calls this callback every time it encounters a RTE instruction.
 * Default behavior: do nothing.
 */
void m68k_set_rte_instr_callback(void  (*callback)(void));


/* Set the callback for the TAS instruction.
 * You must enable M68K_TAS_HAS_CALLBACK in m68kconf.h.
 * The CPU calls this callback every time it encounters a TAS instruction.
 * Default behavior: return 1, allow writeback.
 */
void m68k_set_tas_instr_callback(int  (*callback)(void));

/* Set the callback for illegal instructions.
 * You must enable M68K_ILLG_HAS_CALLBACK in m68kconf.h.
 * The CPU calls this callback every time it encounters an illegal instruction
 * which must return 1 if it handles the instruction normally or 0 if it's really an illegal instruction.
 * Default behavior: return 0, exception will occur.
 */
void m68k_set_illg_instr_callback(int  (*callback)(int));

/* Set the callback for informing of a large PC change.
 * You must enable M68K_MONITOR_PC in m68kconf.h.
 * The CPU calls this callback with the new PC value every time the PC changes
 * by a large value (currently set for changes by longwords).
 * Default behavior: do nothing.
 */
void m68k_set_pc_changed_callback(void  (*callback)(unsigned int new_pc));

/* Set the callback for CPU function code changes.
 * You must enable M68K_EMULATE_FC in m68kconf.h.
 * The CPU calls this callback with the function code before every memory
 * access to set the CPU's function code according to what kind of memory
 * access it is (supervisor/user, program/data and such).
 * Default behavior: do nothing.
 */
void m68k_set_fc_callback(void  (*callback)(unsigned int new_fc));


/* Set a callback for the instruction cycle of the CPU.
 * You must enable M68K_INSTRUCTION_HOOK in m68kconf.h.
 * The CPU calls this callback just before fetching the opcode in the
 * instruction cycle.
 * If this callback returns a nonzero value, the instruction isn't processed
 * and m68k_execute() returns.
 * Default behavior: do nothing.
 */
void m68k_set_instr_hook_callback(int  (*callback)(void));


/* ======================================================================== */
/* ====================== FUNCTIONS TO ACCESS THE CPU ===================== */
/* ======================================================================== */

/* Use this function to set the CPU type you want to emulate.
 * Currently supported types are: M68K_CPU_TYPE_68000, M68K_CPU_TYPE_68010,
 * M68K_CPU_TYPE_EC020, and M68K_CPU_TYPE_68020.
 */
void m68k_set_cpu_type(unsigned int cpu_type);

/* Do whatever initialisations the core requires.  Should be called
 * at least once at init time.
 */
void m68k_init(void);

/* Pulse the RESET pin on the CPU.
 * You *MUST* reset the CPU at least once to initialize the emulation
 * Note: If you didn't call m68k_set_cpu_type() before resetting
 *       the CPU for the first time, the CPU will be set to
 *       M68K_CPU_TYPE_68000.
 */
void m68k_pulse_reset(void);

/* execute num_cycles worth of instructions.  returns number of cycles used */
int m68k_execute(int num_cycles);

/* These functions let you read/write/modify the number of cycles left to run
 * while m68k_execute() is running.
 * These are useful if the 68k accesses a memory-mapped port on another device
 * that requires immediate processing by another CPU.
 */
int m68k_cycles_run(void);              /* Number of cycles run so far */
int m68k_cycles_remaining(void);        /* Number of cycles left */
void m68k_modify_timeslice(int cycles); /* Modify cycles left */
void m68k_end_timeslice(void);          /* End timeslice now */

/* Set the IPL0-IPL2 pins on the CPU (IRQ).
 * A transition from < 7 to 7 will cause a non-maskable interrupt (NMI).
 * Setting IRQ to 0 will clear an interrupt request.
 */
void m68k_set_irq(unsigned int int_level);

/* Set the virtual irq lines, where the highest level
 * active line is automatically selected.  If you use this function,
 * do not use m68k_set_irq.
 */
void m68k_set_virq(unsigned int level, unsigned int active);
unsigned int m68k_get_virq(unsigned int level);

/* Halt the CPU as if you pulsed the HALT pin. */
void m68k_pulse_halt(void);


/* Trigger a bus error exception */
void m68k_pulse_bus_error(void);


/* Context switching to allow multiple CPUs */

/* Get the size of the cpu context in bytes */
unsigned int m68k_context_size(void);

/* Get a cpu context */
unsigned int m68k_get_context(void* dst);

/* set the current cpu context */
void m68k_set_context(void* dst);

/* Register the CPU state information */
void m68k_state_register(const char *type, int index);


/* Peek at the internals of a CPU context.  This can either be a context
 * retrieved using m68k_get_context() or the currently running context.
 * If context is NULL, the currently running CPU context will be used.
 */
unsigned int m68k_get_reg(void* context, m68k_register_t reg);

/* Poke values into the internals of the currently running CPU context */
void m68k_set_reg(m68k_register_t reg, unsigned int value);

/* Check if an instruction is valid for the specified CPU type */
unsigned int m68k_is_valid_instruction(unsigned int instruction, unsigned int cpu_type);

/* Disassemble 1 instruction using the epecified CPU type at pc.  Stores
 * disassembly in str_buff and returns the size of the instruction in bytes.
 */
unsigned int m68k_disassemble(char* str_buff, unsigned int pc, unsigned int cpu_type);

/* Same as above but accepts raw opcode data directly rather than fetching
 * via the read/write interfaces.
 */
unsigned int m68k_disassemble_raw(char* str_buff, unsigned int pc, const unsigned char* opdata, const unsigned char* argdata, unsigned int cpu_type);


/* ======================================================================== */
/* ============================== MAME STUFF ============================== */
/* ======================================================================== */

#if M68K_COMPILE_FOR_MAME == OPT_ON
#include "m68kmame.h"
#endif /* M68K_COMPILE_FOR_MAME */


/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#ifdef __cplusplus
}
#endif


#endif /* M68K__HEADER */
