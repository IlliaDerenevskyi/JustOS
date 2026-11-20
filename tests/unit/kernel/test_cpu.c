/*
 * Unit test (level U) of src/kernel/cpu.c. The register functions of
 * cpu_regs.h are replaced by a fake processor: a set of register values that
 * the test script sets, and writes that the fake applies (or ignores, when the
 * test wants to prove that cpu.c reads the registers back).
 */

#include <assert.h>
#include <string.h>

#include "cpu_regs.h"
#include "justos/kernel/cpu.h"

/* ---- The fake processor ---- */

typedef struct FakeCpu {
	JPKernelX86Registers x86;
	bool nx_supported;
	JPKernelAArch64Registers arm;
	bool ignore_cr0_write;
	bool ignore_efer_write;
	unsigned int reads;
	unsigned int cr0_writes;
	unsigned int efer_writes;
	uint64_t last_cr0;
	uint64_t last_efer;
} FakeCpu;

static FakeCpu cpu;

void jp_kernel_x86_64_read_registers(JPKernelX86Registers *registers)
{
	cpu.reads++;
	*registers = cpu.x86;
}

bool jp_kernel_x86_64_nx_supported(void)
{
	return cpu.nx_supported;
}

void jp_kernel_x86_64_write_cr0(uint64_t value)
{
	cpu.cr0_writes++;
	cpu.last_cr0 = value;
	if (!cpu.ignore_cr0_write) {
		cpu.x86.cr0 = value;
	}
}

void jp_kernel_x86_64_write_efer(uint64_t value)
{
	cpu.efer_writes++;
	cpu.last_efer = value;
	if (!cpu.ignore_efer_write) {
		cpu.x86.efer = value;
	}
}

void jp_kernel_aarch64_read_registers(JPKernelAArch64Registers *registers)
{
	cpu.reads++;
	*registers = cpu.arm;
}

uint32_t jp_kernel_cpu_physical_address_bits(void)
{
	return 48;
}

/* ---- Setup ---- */

#define OTHER_CR0_BITS 0x0000000000040032ULL   /* MP, ET, NE, AM: bits that cpu.c must leave alone */
#define OTHER_EFER_BITS 0x0000000000000001ULL  /* SCE */

static JPBootContext context;

static void setup_x86(void)
{
	memset(&cpu, 0, sizeof cpu);
	memset(&context, 0, sizeof context);
	context.handoff.architecture = JP_ARCH_X86_64;
	cpu.x86.cr0 = JP_X86_64_CR0_PE | JP_X86_64_CR0_PG | OTHER_CR0_BITS;
	cpu.x86.cr4 = JP_X86_64_CR4_PAE | 0x600;      /* OSFXSR/OSXMMEXCPT may be set or not: left alone */
	cpu.x86.efer = JP_X86_64_EFER_LME | JP_X86_64_EFER_LMA | OTHER_EFER_BITS;
	cpu.x86.rflags = 0x2;                          /* bit 1 is always 1; IF is 0 */
	cpu.x86.cs = 0x08;
	cpu.nx_supported = true;
}

static void setup_arm(void)
{
	memset(&cpu, 0, sizeof cpu);
	memset(&context, 0, sizeof context);
	context.handoff.architecture = JP_ARCH_AARCH64;
	cpu.arm.current_el = JP_AARCH64_EL1 << JP_AARCH64_CURRENT_EL_SHIFT;
	cpu.arm.daif = JP_AARCH64_DAIF_MASK << JP_AARCH64_DAIF_SHIFT;
	cpu.arm.sctlr_el1 = JP_AARCH64_SCTLR_M | 0x30D00800ULL;
	cpu.arm.tcr_el1 = JP_AARCH64_TCR_SIZE_48_BITS |
		(JP_AARCH64_TCR_SIZE_48_BITS << JP_AARCH64_TCR_T1SZ_SHIFT) |
		(JP_AARCH64_TCR_TG1_4K << JP_AARCH64_TCR_TG1_SHIFT) | (5ULL << 32) | 0x3500ULL;
	cpu.arm.mair_el1 = JP_AARCH64_MAIR_STAGE1;
}

/* ---- x86_64 ---- */

static void test_x86_success(void)
{
	setup_x86();
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);

	/* the two bits are set, every other bit is as it was */
	assert((cpu.x86.efer & JP_X86_64_EFER_NXE) != 0);
	assert((cpu.x86.cr0 & JP_X86_64_CR0_WP) != 0);
	assert(cpu.x86.efer == (JP_X86_64_EFER_LME | JP_X86_64_EFER_LMA | OTHER_EFER_BITS | JP_X86_64_EFER_NXE));
	assert(cpu.x86.cr0 == (JP_X86_64_CR0_PE | JP_X86_64_CR0_PG | OTHER_CR0_BITS | JP_X86_64_CR0_WP));
	assert(cpu.x86.cr4 == (JP_X86_64_CR4_PAE | 0x600));    /* CR4 is never written */
	assert(cpu.x86.rflags == 0x2);                          /* interrupts stay masked */
	assert(cpu.efer_writes == 1 && cpu.cr0_writes == 1);
}

static void test_x86_bits_already_set(void)
{
	setup_x86();
	cpu.x86.efer |= JP_X86_64_EFER_NXE;
	cpu.x86.cr0 |= JP_X86_64_CR0_WP;
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);
	assert(cpu.efer_writes == 0 && cpu.cr0_writes == 0);    /* a bit that is already set is not written again */

	/* only one of them missing: only that one is written */
	setup_x86();
	cpu.x86.efer |= JP_X86_64_EFER_NXE;
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);
	assert(cpu.efer_writes == 0 && cpu.cr0_writes == 1);
	setup_x86();
	cpu.x86.cr0 |= JP_X86_64_CR0_WP;
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);
	assert(cpu.efer_writes == 1 && cpu.cr0_writes == 0);
}

static void test_x86_twice(void)
{
	setup_x86();
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);
	assert(cpu.efer_writes == 1 && cpu.cr0_writes == 1);
}

#define EXPECT_X86_FAILS(statement) \
	do { \
		setup_x86(); \
		statement; \
		assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_FAILED); \
		assert(cpu.efer_writes == 0 && cpu.cr0_writes == 0);   /* a wrong state is not repaired */ \
	} while (0)

static void test_x86_wrong_state(void)
{
	EXPECT_X86_FAILS(cpu.x86.cr0 &= ~JP_X86_64_CR0_PE);
	EXPECT_X86_FAILS(cpu.x86.cr0 &= ~JP_X86_64_CR0_PG);
	EXPECT_X86_FAILS(cpu.x86.cr4 &= ~JP_X86_64_CR4_PAE);
	EXPECT_X86_FAILS(cpu.x86.efer &= ~JP_X86_64_EFER_LME);
	EXPECT_X86_FAILS(cpu.x86.efer &= ~JP_X86_64_EFER_LMA);
	EXPECT_X86_FAILS(cpu.x86.cs = 0x0B);                    /* privilege level 3 */
	EXPECT_X86_FAILS(cpu.x86.cs = 0x09);                    /* level 1 */
	EXPECT_X86_FAILS(cpu.x86.cs = 0x0A);                    /* level 2 */
	EXPECT_X86_FAILS(cpu.x86.rflags |= JP_X86_64_RFLAGS_IF);
	EXPECT_X86_FAILS(cpu.nx_supported = false);
	EXPECT_X86_FAILS(memset(&cpu.x86, 0, sizeof cpu.x86));  /* real mode */
}

static void test_x86_cs_selector_value_does_not_matter(void)
{
	setup_x86();
	cpu.x86.cs = 0x18;                                      /* another selector, level 0 */
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);
	setup_x86();
	cpu.x86.cs = 0x14;                                      /* a table-indicator bit set, level 0 */
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);
}

static void test_x86_write_without_effect(void)
{
	/* The write instruction ran, but the bit did not appear: success must not be reported. */
	setup_x86();
	cpu.ignore_efer_write = true;
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_FAILED);
	assert(cpu.efer_writes == 1);

	setup_x86();
	cpu.ignore_cr0_write = true;
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_FAILED);
	assert(cpu.cr0_writes == 1);
}

static void test_x86_writes_keep_other_bits(void)
{
	setup_x86();
	cpu.x86.efer |= 0x4000;           /* a bit that cpu.c does not know */
	cpu.x86.cr0 |= 0x20000000;        /* the same for CR0 */
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);
	assert(cpu.last_efer == (JP_X86_64_EFER_LME | JP_X86_64_EFER_LMA | OTHER_EFER_BITS | 0x4000 | JP_X86_64_EFER_NXE));
	assert(cpu.last_cr0 == (JP_X86_64_CR0_PE | JP_X86_64_CR0_PG | OTHER_CR0_BITS | 0x20000000 | JP_X86_64_CR0_WP));
}

/* ---- AArch64 ---- */

static void test_arm_success(void)
{
	JPKernelAArch64Registers before;

	setup_arm();
	before = cpu.arm;
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);
	assert(memcmp(&before, &cpu.arm, sizeof before) == 0);   /* nothing is written */
}

static void test_arm_wxn_either_way(void)
{
	setup_arm();
	cpu.arm.sctlr_el1 &= ~(1ULL << 19);
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);
	setup_arm();
	cpu.arm.sctlr_el1 |= 1ULL << 19;
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);
}

#define EXPECT_ARM_FAILS(statement) \
	do { \
		setup_arm(); \
		statement; \
		assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_FAILED); \
	} while (0)

static void test_arm_wrong_state(void)
{
	unsigned int bit;

	EXPECT_ARM_FAILS(cpu.arm.current_el = 0 << JP_AARCH64_CURRENT_EL_SHIFT);
	EXPECT_ARM_FAILS(cpu.arm.current_el = 2 << JP_AARCH64_CURRENT_EL_SHIFT);
	EXPECT_ARM_FAILS(cpu.arm.current_el = 3 << JP_AARCH64_CURRENT_EL_SHIFT);
	/* each of D, A, I, F alone unmasked */
	for (bit = 0; bit < 4; bit++) {
		EXPECT_ARM_FAILS(cpu.arm.daif &= ~(1ULL << (JP_AARCH64_DAIF_SHIFT + bit)));
	}
	EXPECT_ARM_FAILS(cpu.arm.daif = 0);
	EXPECT_ARM_FAILS(cpu.arm.sctlr_el1 &= ~JP_AARCH64_SCTLR_M);

	EXPECT_ARM_FAILS(cpu.arm.tcr_el1 = (cpu.arm.tcr_el1 & ~JP_AARCH64_TCR_T0SZ_MASK) | 17);
	EXPECT_ARM_FAILS(cpu.arm.tcr_el1 = (cpu.arm.tcr_el1 & ~JP_AARCH64_TCR_T0SZ_MASK) | 15);
	EXPECT_ARM_FAILS(cpu.arm.tcr_el1 = (cpu.arm.tcr_el1 & ~(JP_AARCH64_TCR_T1SZ_MASK << JP_AARCH64_TCR_T1SZ_SHIFT)) |
		(17ULL << JP_AARCH64_TCR_T1SZ_SHIFT));
	EXPECT_ARM_FAILS(cpu.arm.tcr_el1 = (cpu.arm.tcr_el1 & ~(JP_AARCH64_TCR_T1SZ_MASK << JP_AARCH64_TCR_T1SZ_SHIFT)) |
		(15ULL << JP_AARCH64_TCR_T1SZ_SHIFT));
	EXPECT_ARM_FAILS(cpu.arm.tcr_el1 |= 1ULL << JP_AARCH64_TCR_TG0_SHIFT);      /* 64 KiB for the low half */
	EXPECT_ARM_FAILS(cpu.arm.tcr_el1 |= 2ULL << JP_AARCH64_TCR_TG0_SHIFT);
	EXPECT_ARM_FAILS(cpu.arm.tcr_el1 = (cpu.arm.tcr_el1 & ~(JP_AARCH64_TCR_TG1_MASK << JP_AARCH64_TCR_TG1_SHIFT)) |
		(1ULL << JP_AARCH64_TCR_TG1_SHIFT));
	EXPECT_ARM_FAILS(cpu.arm.tcr_el1 = (cpu.arm.tcr_el1 & ~(JP_AARCH64_TCR_TG1_MASK << JP_AARCH64_TCR_TG1_SHIFT)) |
		(3ULL << JP_AARCH64_TCR_TG1_SHIFT));
	EXPECT_ARM_FAILS(cpu.arm.tcr_el1 = cpu.arm.tcr_el1 & ~(JP_AARCH64_TCR_TG1_MASK << JP_AARCH64_TCR_TG1_SHIFT));
	EXPECT_ARM_FAILS(cpu.arm.tcr_el1 |= JP_AARCH64_TCR_EPD0);
	EXPECT_ARM_FAILS(cpu.arm.tcr_el1 |= JP_AARCH64_TCR_EPD1);
	EXPECT_ARM_FAILS(cpu.arm.mair_el1 = 0);
	EXPECT_ARM_FAILS(cpu.arm.mair_el1 = JP_AARCH64_MAIR_STAGE1 | 0x400000);
	EXPECT_ARM_FAILS(cpu.arm.mair_el1 = 0xFF);
}

static void test_arm_other_tcr_bits_are_ignored(void)
{
	/* IPS, the shareability, the cache fields, TBI: not checked by cpu.c */
	setup_arm();
	cpu.arm.tcr_el1 |= (7ULL << 32) | (3ULL << 12) | (1ULL << 37);
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_SUCCESS);
}

/* ---- Common ---- */

static void test_common_refusals(void)
{
	setup_x86();
	assert(jp_kernel_cpu_initialize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(cpu.reads == 0);

	setup_x86();
	context.handoff.architecture = JP_ARCH_UNKNOWN;
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_FAILED);
	assert(cpu.reads == 0);                    /* no register is read for an unknown architecture */

	setup_x86();
	context.handoff.architecture = (JPArchitectureType)77;
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_FAILED);
	assert(cpu.reads == 0);
}

static void test_architecture_selects_the_registers(void)
{
	/* An x86_64 context with a good AArch64 state is not good, and the other way round. */
	setup_arm();
	context.handoff.architecture = JP_ARCH_X86_64;
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_FAILED);
	setup_x86();
	context.handoff.architecture = JP_ARCH_AARCH64;
	assert(jp_kernel_cpu_initialize(&context) == JP_BOOT_FAILED);
}

/* ---- The PARange decoder ---- */

static void test_parange(void)
{
	assert(jp_kernel_aarch64_parange_to_bits(0) == 32);
	assert(jp_kernel_aarch64_parange_to_bits(1) == 36);
	assert(jp_kernel_aarch64_parange_to_bits(2) == 40);
	assert(jp_kernel_aarch64_parange_to_bits(3) == 42);
	assert(jp_kernel_aarch64_parange_to_bits(4) == 44);
	assert(jp_kernel_aarch64_parange_to_bits(5) == 48);
	assert(jp_kernel_aarch64_parange_to_bits(6) == 52);
	assert(jp_kernel_aarch64_parange_to_bits(7) == 0);
	assert(jp_kernel_aarch64_parange_to_bits(0xF) == 0);
	/* only bits 3:0 count */
	assert(jp_kernel_aarch64_parange_to_bits(0xF0) == 32);
	assert(jp_kernel_aarch64_parange_to_bits(0x1234567890ABCDE5ULL) == 48);
}

int main(void)
{
	test_x86_success();
	test_x86_bits_already_set();
	test_x86_twice();
	test_x86_wrong_state();
	test_x86_cs_selector_value_does_not_matter();
	test_x86_write_without_effect();
	test_x86_writes_keep_other_bits();
	test_arm_success();
	test_arm_wxn_either_way();
	test_arm_wrong_state();
	test_arm_other_tcr_bits_are_ignored();
	test_common_refusals();
	test_architecture_selects_the_registers();
	test_parange();
	return 0;
}
