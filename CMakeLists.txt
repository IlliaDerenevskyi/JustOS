cmake_minimum_required(VERSION 3.20)
project(JustOS LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Any warning is an error (Rule 18). NDEBUG is undefined so that assert()
# in tests is never compiled out.
add_compile_options(
	-Wall -Wextra -Wpedantic -Werror
	-Wstrict-prototypes -Wmissing-prototypes
	-Wshadow -Wconversion -Wundef
	-UNDEBUG
)

enable_testing()

# Host unit tests (level U). Only files that exist may be listed here.
# jp_add_unit_test(<name> <source>) builds one executable and registers it.
function(jp_add_unit_test name source)
	add_executable(${name} ${source})
	target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/include)
	add_test(NAME ${name} COMMAND ${name})
endfunction()

jp_add_unit_test(test_boot_types tests/unit/boot/test_boot_types.c)

# Test support (tests only, never linked into production code).
add_library(jp_test_support STATIC
	tests/support/elf_builder.c
	tests/support/fake_uefi.c
	tests/support/fake_bios.c
)
target_include_directories(jp_test_support PUBLIC
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/tests/support
)

jp_add_unit_test(test_support tests/unit/support/test_elf_builder.c)
target_link_libraries(test_support PRIVATE jp_test_support)

# Tests of production modules. A module is linked in only when its source file
# exists. Until then the link fails with "undefined reference", which CI
# reports as "Build incomplete" (missing code of the same milestone).
function(jp_link_module_if_exists target source)
	if(EXISTS ${CMAKE_SOURCE_DIR}/${source})
		target_sources(${target} PRIVATE ${source})
	endif()
endfunction()

jp_add_unit_test(test_boot_environment tests/unit/boot/test_boot_environment.c)
target_link_libraries(test_boot_environment PRIVATE jp_test_support)
jp_link_module_if_exists(test_boot_environment src/boot/boot_environment.c)

# Fakes of production layers. They live in separate libraries so that a test
# cannot silently link against a fake instead of a missing production module.
add_library(jp_fake_environment STATIC tests/support/fake_environment.c)
target_include_directories(jp_fake_environment PUBLIC
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/tests/support
)
add_library(jp_fake_architecture STATIC tests/support/fake_architecture.c)
target_include_directories(jp_fake_architecture PUBLIC
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/tests/support
)

jp_add_unit_test(test_hardware tests/unit/boot/test_hardware.c)
target_link_libraries(test_hardware PRIVATE jp_fake_architecture)
jp_link_module_if_exists(test_hardware src/boot/hardware.c)

jp_add_unit_test(test_memory_map tests/unit/boot/test_memory_map.c)
target_link_libraries(test_memory_map PRIVATE jp_fake_environment jp_test_support)
jp_link_module_if_exists(test_memory_map src/boot/memory_map.c)

jp_add_unit_test(test_framebuffer tests/unit/boot/test_framebuffer.c)
target_link_libraries(test_framebuffer PRIVATE jp_fake_environment jp_test_support)
jp_link_module_if_exists(test_framebuffer src/boot/framebuffer.c)

jp_add_unit_test(test_kernel_image tests/unit/boot/test_kernel_image.c)
target_link_libraries(test_kernel_image PRIVATE jp_fake_environment jp_test_support)
jp_link_module_if_exists(test_kernel_image src/boot/kernel_image.c)

jp_add_unit_test(test_kernel_validate tests/unit/boot/test_kernel_validate.c)
target_link_libraries(test_kernel_validate PRIVATE jp_fake_architecture jp_fake_environment jp_test_support)
jp_link_module_if_exists(test_kernel_validate src/boot/kernel_loader.c)

jp_add_unit_test(test_kernel_load tests/unit/boot/test_kernel_load.c)
target_link_libraries(test_kernel_load PRIVATE jp_fake_architecture jp_fake_environment jp_test_support)
jp_link_module_if_exists(test_kernel_load src/boot/kernel_loader.c)

jp_add_unit_test(test_handoff tests/unit/boot/test_handoff.c)
target_link_libraries(test_handoff PRIVATE jp_fake_architecture jp_fake_environment)
jp_link_module_if_exists(test_handoff src/boot/handoff.c)
jp_link_module_if_exists(test_handoff src/boot/memory_map.c)

jp_add_unit_test(test_pipeline tests/unit/boot/test_pipeline.c)
jp_link_module_if_exists(test_pipeline src/boot/boot_pipeline.c)

# Data list of the Milestone 0 test kernel (formatting logic, tested on the host).
jp_add_unit_test(test_report tests/unit/qemu/test_report.c)
target_sources(test_report PRIVATE tests/qemu/test_kernel/report.c)
target_include_directories(test_report PRIVATE ${CMAKE_SOURCE_DIR}/tests/qemu/test_kernel)

jp_add_unit_test(test_font tests/unit/qemu/test_font.c)
target_sources(test_font PRIVATE tests/qemu/test_kernel/font8x16.c)
target_include_directories(test_font PRIVATE ${CMAKE_SOURCE_DIR}/tests/qemu/test_kernel)

jp_add_unit_test(test_wait tests/unit/qemu/test_wait.c)
target_sources(test_wait PRIVATE tests/qemu/test_kernel/wait_common.c)
target_include_directories(test_wait PRIVATE ${CMAKE_SOURCE_DIR}/tests/qemu/test_kernel)

jp_add_unit_test(test_screen tests/unit/qemu/test_screen.c)
target_sources(test_screen PRIVATE
	tests/qemu/test_kernel/screen.c
	tests/qemu/test_kernel/report.c
	tests/qemu/test_kernel/font8x16.c
)
target_include_directories(test_screen PRIVATE ${CMAKE_SOURCE_DIR}/tests/qemu/test_kernel)

# Self-test of the serial-log checker (host only, needs no QEMU).
add_test(NAME test_check_serial COMMAND ${CMAKE_SOURCE_DIR}/tests/qemu/test_check_serial.sh)

# QEMU tests (level Q). Off by default: they need clang, lld, QEMU, UEFI
# firmware, and mtools (see requirements.txt). KVM variants (label kvm) exist for
# the x86_64 platforms. Turn on with
#   cmake -DJP_ENABLE_QEMU_TESTS=ON ...   and run with   ctest -L qemu
# Each test builds the test kernel, then runs tests/qemu/run_test.sh, which
# also needs the boot application <build>/qemu/<platform>/boot.efi. While the
# boot code does not exist, these tests fail with "not built yet".
option(JP_ENABLE_QEMU_TESTS "Register the QEMU (level Q) tests" OFF)
if(JP_ENABLE_QEMU_TESTS)
	find_program(JP_CLANG clang REQUIRED)
	find_program(JP_LLD ld.lld REQUIRED)

	set(JP_TEST_KERNEL_DIR ${CMAKE_SOURCE_DIR}/tests/qemu/test_kernel)
	set(JP_TEST_KERNEL_SOURCES
		${JP_TEST_KERNEL_DIR}/kernel_main.c
		${JP_TEST_KERNEL_DIR}/report.c
		${JP_TEST_KERNEL_DIR}/screen.c
		${JP_TEST_KERNEL_DIR}/font8x16.c
		${JP_TEST_KERNEL_DIR}/wait_common.c
	)

	# Stage 2 (src/kernel): the test kernel starts through the Stage 2 entry stub, so the QEMU test runs the
	# real Stage 2. Only the stand-in for the Stage 4 continuation is test code.
	set(JP_KERNEL_SRC ${CMAKE_SOURCE_DIR}/src/kernel)
	set(JP_KERNEL_COMMON_SOURCES
		${JP_KERNEL_SRC}/cpu.c ${JP_KERNEL_SRC}/memory.c ${JP_KERNEL_SRC}/paging.c ${JP_KERNEL_SRC}/stack.c
		${JP_KERNEL_SRC}/heap.c ${JP_KERNEL_SRC}/elf.c ${JP_KERNEL_SRC}/init.c
		${JP_TEST_KERNEL_DIR}/fake_stage2_complete.c
	)
	set(JP_KERNEL_ONLY_MACRO_x86_64 JP_KERNEL_ONLY_X86_64)
	set(JP_KERNEL_ONLY_MACRO_aarch64 JP_KERNEL_ONLY_AARCH64)

	# The test kernel must not use SSE on x86_64: the BIOS path leaves the SSE bits of CR0 and CR4 off.
	set(JP_TEST_KERNEL_ARCH_FLAGS_x86_64 -mgeneral-regs-only -mno-red-zone)
	set(JP_TEST_KERNEL_ARCH_FLAGS_aarch64 "")

	# jp_add_qemu_test(<platform> <clang target> <architecture>)
	function(jp_add_qemu_test platform target arch)
		set(out_dir ${CMAKE_BINARY_DIR}/qemu/${platform})
		set(kernel ${out_dir}/test_kernel.elf)
		add_custom_command(
			OUTPUT ${kernel}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
			COMMAND ${JP_CLANG} --target=${target} -std=c17 -ffreestanding
				-fno-pic -fno-stack-protector ${JP_TEST_KERNEL_ARCH_FLAGS_${arch}} -Wall -Wextra -Werror -DJP_TEST_KERNEL_NO_WAIT=1
				-fuse-ld=lld -nostdlib -Wl,-z,max-page-size=4096
				-Wl,-T,${JP_TEST_KERNEL_DIR}/kernel_${arch}.ld
				-I${CMAKE_SOURCE_DIR}/include -I${JP_TEST_KERNEL_DIR}
				${JP_TEST_KERNEL_SOURCES}
				${JP_TEST_KERNEL_DIR}/serial_${arch}.c
				${JP_TEST_KERNEL_DIR}/wait_${arch}.c
				${JP_KERNEL_COMMON_SOURCES}
				${JP_KERNEL_SRC}/cpu_regs_${arch}.c ${JP_KERNEL_SRC}/paging_hw_${arch}.c
				${JP_KERNEL_SRC}/stack_hw_${arch}.c ${JP_KERNEL_SRC}/halt_hw_${arch}.c
				${JP_KERNEL_SRC}/entry_${arch}.S
				-D${JP_KERNEL_ONLY_MACRO_${arch}}=1 -fno-builtin -I${JP_KERNEL_SRC}
				-o ${kernel}
			DEPENDS ${JP_TEST_KERNEL_SOURCES}
				${JP_KERNEL_COMMON_SOURCES}
				${JP_KERNEL_SRC}/cpu_regs_${arch}.c ${JP_KERNEL_SRC}/paging_hw_${arch}.c
				${JP_KERNEL_SRC}/stack_hw_${arch}.c ${JP_KERNEL_SRC}/halt_hw_${arch}.c
				${JP_KERNEL_SRC}/entry_${arch}.S
				${JP_TEST_KERNEL_DIR}/serial_${arch}.c
				${JP_TEST_KERNEL_DIR}/wait_${arch}.c
				${JP_TEST_KERNEL_DIR}/screen.h
				${JP_TEST_KERNEL_DIR}/wait.h
				${JP_TEST_KERNEL_DIR}/font.h
				${JP_TEST_KERNEL_DIR}/kernel_${arch}.ld
				${JP_TEST_KERNEL_DIR}/platform.h
				${JP_TEST_KERNEL_DIR}/report.h
			VERBATIM
		)
		add_custom_target(test_kernel_${platform} ALL DEPENDS ${kernel})
		add_test(NAME qemu_${platform}
			COMMAND ${CMAKE_SOURCE_DIR}/tests/qemu/run_test.sh ${platform} ${CMAKE_BINARY_DIR})
		set_tests_properties(qemu_${platform} PROPERTIES LABELS qemu TIMEOUT 120)
		# The same test with KVM acceleration (x86_64 only; ctest -L kvm).
		if(arch STREQUAL "x86_64")
			add_test(NAME qemu_kvm_${platform}
				COMMAND ${CMAKE_COMMAND} -E env JP_QEMU_KVM=1
					${CMAKE_SOURCE_DIR}/tests/qemu/run_test.sh ${platform} ${CMAKE_BINARY_DIR})
			set_tests_properties(qemu_kvm_${platform} PROPERTIES LABELS "qemu;kvm" TIMEOUT 120)
		endif()
	endfunction()

	# The UEFI boot application: a PE/COFF file made by clang and lld. It holds only the UEFI
	# provider and the architecture it is built for (see arch_dispatch.h and boot_environment.c).
	set(JP_BOOT_SRC ${CMAKE_SOURCE_DIR}/src/boot)
	set(JP_BOOT_APP_COMMON_SOURCES
		${JP_BOOT_SRC}/boot_pipeline.c ${JP_BOOT_SRC}/boot_environment.c ${JP_BOOT_SRC}/hardware.c
		${JP_BOOT_SRC}/memory_map.c ${JP_BOOT_SRC}/framebuffer.c ${JP_BOOT_SRC}/kernel_image.c
		${JP_BOOT_SRC}/kernel_loader.c ${JP_BOOT_SRC}/handoff.c
		${JP_BOOT_SRC}/platform/uefi.c ${JP_BOOT_SRC}/platform/uefi_entry.c
	)

	# jp_add_boot_app(<platform> <clang target> <architecture, for the macro and the file names> <macro>)
	function(jp_add_boot_app platform target arch macro)
		set(app_dir ${CMAKE_BINARY_DIR}/qemu/${platform})
		set(sources ${JP_BOOT_APP_COMMON_SOURCES}
			${JP_BOOT_SRC}/architecture/${arch}.c ${JP_BOOT_SRC}/architecture/${arch}_enter.S)
		add_custom_command(
			OUTPUT ${app_dir}/boot.efi
			COMMAND ${CMAKE_COMMAND} -E make_directory ${app_dir}
			COMMAND ${JP_CLANG} --target=${target} -std=c17 -ffreestanding
				-fno-stack-protector -fshort-wchar -O2 -Wall -Wextra -Wpedantic -Werror
				-Wconversion -Wshadow -Wmissing-prototypes -Wstrict-prototypes
				-DJP_BOOT_WITHOUT_BIOS=1 -D${macro}=1
				-I${CMAKE_SOURCE_DIR}/include -I${JP_BOOT_SRC} -I${JP_BOOT_SRC}/platform -I${JP_BOOT_SRC}/architecture
				-fuse-ld=lld -nostdlib -Wl,/subsystem:efi_application -Wl,/entry:efi_main
				${sources} -o ${app_dir}/boot.efi
			DEPENDS ${sources}
			VERBATIM
		)
		add_custom_target(boot_app_${platform} ALL DEPENDS ${app_dir}/boot.efi)
	endfunction()

	# The BIOS boot chain: the boot sector (512 bytes) and the loader (one flat binary: the 16-bit
	# part, then all the 64-bit code, linked for 0x8000). Both are made by clang and lld.
	function(jp_add_bios_boot_chain)
		set(app_dir ${CMAKE_BINARY_DIR}/qemu/x86_64-bios)
		set(plat ${JP_BOOT_SRC}/platform)
		set(common_flags --target=x86_64-unknown-none-elf -ffreestanding -fno-pic -fno-pie
			-fuse-ld=lld -nostdlib -static)
		set(loader_sources
			${plat}/bios_loader.S ${plat}/bios_entry.c ${plat}/bios.c
			${JP_BOOT_SRC}/boot_pipeline.c ${JP_BOOT_SRC}/boot_environment.c ${JP_BOOT_SRC}/hardware.c
			${JP_BOOT_SRC}/memory_map.c ${JP_BOOT_SRC}/framebuffer.c ${JP_BOOT_SRC}/kernel_image.c
			${JP_BOOT_SRC}/kernel_loader.c ${JP_BOOT_SRC}/handoff.c
			${JP_BOOT_SRC}/architecture/x86_64.c ${JP_BOOT_SRC}/architecture/x86_64_enter.S)
		add_custom_command(
			OUTPUT ${app_dir}/boot_sector.bin
			COMMAND ${CMAKE_COMMAND} -E make_directory ${app_dir}
			COMMAND ${JP_CLANG} ${common_flags} -Wl,-T,${plat}/bios_boot_sector.ld -Wl,--oformat=binary
				${plat}/bios_boot_sector.S -o ${app_dir}/boot_sector.bin
			DEPENDS ${plat}/bios_boot_sector.S ${plat}/bios_boot_sector.ld
			VERBATIM
		)
		add_custom_command(
			OUTPUT ${app_dir}/loader.bin
			COMMAND ${CMAKE_COMMAND} -E make_directory ${app_dir}
			COMMAND ${JP_CLANG} ${common_flags} -std=c17 -mno-red-zone -mgeneral-regs-only -fno-stack-protector
				-fno-asynchronous-unwind-tables -O2 -Wall -Wextra -Wpedantic -Werror -Wconversion
				-Wshadow -Wmissing-prototypes -Wstrict-prototypes
				-DJP_BOOT_ONLY_X86_64=1 -DJP_BOOT_WITHOUT_UEFI=1
				-I${CMAKE_SOURCE_DIR}/include -I${JP_BOOT_SRC} -I${plat} -I${JP_BOOT_SRC}/architecture
				-Wl,-T,${plat}/bios_loader.ld -Wl,--oformat=binary
				${loader_sources} -o ${app_dir}/loader.bin
			DEPENDS ${loader_sources} ${plat}/bios_loader.ld ${plat}/bios_io.h
			VERBATIM
		)
		add_custom_target(boot_chain_x86_64-bios ALL DEPENDS ${app_dir}/boot_sector.bin ${app_dir}/loader.bin)
	endfunction()

	jp_add_bios_boot_chain()
	jp_add_boot_app(x86_64-uefi x86_64-pc-windows-msvc x86_64 JP_BOOT_ONLY_X86_64)
	jp_add_boot_app(aarch64-uefi aarch64-pc-windows-msvc aarch64 JP_BOOT_ONLY_AARCH64)

	jp_add_qemu_test(x86_64-uefi x86_64-unknown-none-elf x86_64)
	jp_add_qemu_test(x86_64-bios x86_64-unknown-none-elf x86_64)
	jp_add_qemu_test(aarch64-uefi aarch64-unknown-none-elf aarch64)
endif()

# The UEFI provider (private definitions live next to it, so the test adds that directory).
jp_add_unit_test(test_uefi tests/unit/boot/test_uefi.c)
target_include_directories(test_uefi PRIVATE ${CMAKE_SOURCE_DIR}/src/boot/platform)
jp_link_module_if_exists(test_uefi src/boot/platform/uefi.c)

# The BIOS provider. The port read (the PS/2 controller) is faked by the test.
jp_add_unit_test(test_cpu tests/unit/kernel/test_cpu.c)
target_include_directories(test_cpu PRIVATE ${CMAKE_SOURCE_DIR}/src/kernel)
jp_link_module_if_exists(test_cpu src/kernel/cpu.c)

jp_add_unit_test(test_paging tests/unit/kernel/test_paging.c)
target_include_directories(test_paging PRIVATE ${CMAKE_SOURCE_DIR}/src/kernel)
jp_link_module_if_exists(test_paging src/kernel/paging.c)
jp_link_module_if_exists(test_paging src/kernel/memory.c)

jp_add_unit_test(test_stack tests/unit/kernel/test_stack.c)
target_include_directories(test_stack PRIVATE ${CMAKE_SOURCE_DIR}/src/kernel)
jp_link_module_if_exists(test_stack src/kernel/stack.c)
jp_link_module_if_exists(test_stack src/kernel/memory.c)

jp_add_unit_test(test_heap tests/unit/kernel/test_heap.c)
target_include_directories(test_heap PRIVATE ${CMAKE_SOURCE_DIR}/src/kernel)
jp_link_module_if_exists(test_heap src/kernel/heap.c)
jp_link_module_if_exists(test_heap src/kernel/memory.c)

jp_add_unit_test(test_elf tests/unit/kernel/test_elf.c)
target_include_directories(test_elf PRIVATE ${CMAKE_SOURCE_DIR}/src/kernel)
jp_link_module_if_exists(test_elf src/kernel/elf.c)
jp_link_module_if_exists(test_elf src/kernel/memory.c)

jp_add_unit_test(test_init tests/unit/kernel/test_init.c)
target_include_directories(test_init PRIVATE ${CMAKE_SOURCE_DIR}/src/kernel)
jp_link_module_if_exists(test_init src/kernel/init.c)

jp_add_unit_test(test_memory tests/unit/kernel/test_memory.c)
target_include_directories(test_memory PRIVATE ${CMAKE_SOURCE_DIR}/src/kernel)
jp_link_module_if_exists(test_memory src/kernel/memory.c)

jp_add_unit_test(test_bios tests/unit/boot/test_bios.c)
target_include_directories(test_bios PRIVATE ${CMAKE_SOURCE_DIR}/src/boot/platform)
jp_link_module_if_exists(test_bios src/boot/platform/bios.c)

# The x86_64 architecture module. It uses the CPUID instruction, so the host test
# is built only on an x86_64 host; the privileged instructions are faked.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
	jp_add_unit_test(test_x86_64 tests/unit/boot/test_x86_64.c)
	target_link_libraries(test_x86_64 PRIVATE jp_fake_environment jp_test_support)
	target_include_directories(test_x86_64 PRIVATE ${CMAKE_SOURCE_DIR}/src/boot/architecture)
	target_compile_definitions(test_x86_64 PRIVATE JP_X86_64_TESTING=1)
	jp_link_module_if_exists(test_x86_64 src/boot/architecture/x86_64.c)
endif()

# boot_environment.c built without the BIOS provider (the UEFI boot application).
jp_add_unit_test(test_boot_environment_no_bios tests/unit/boot/test_boot_environment_no_bios.c)
target_link_libraries(test_boot_environment_no_bios PRIVATE jp_test_support)
target_compile_definitions(test_boot_environment_no_bios PRIVATE JP_BOOT_WITHOUT_BIOS=1)
jp_link_module_if_exists(test_boot_environment_no_bios src/boot/boot_environment.c)

# boot_environment.c built without the UEFI provider (the BIOS loader).
jp_add_unit_test(test_boot_environment_no_uefi tests/unit/boot/test_boot_environment_no_uefi.c)
target_link_libraries(test_boot_environment_no_uefi PRIVATE jp_test_support)
target_compile_definitions(test_boot_environment_no_uefi PRIVATE JP_BOOT_WITHOUT_UEFI=1)
jp_link_module_if_exists(test_boot_environment_no_uefi src/boot/boot_environment.c)

# The AArch64 architecture module is plain C (the processor instructions are faked),
# so its host test runs on any host.
jp_add_unit_test(test_aarch64 tests/unit/boot/test_aarch64.c)
target_link_libraries(test_aarch64 PRIVATE jp_fake_environment jp_test_support)
target_include_directories(test_aarch64 PRIVATE ${CMAKE_SOURCE_DIR}/src/boot/architecture)
target_compile_definitions(test_aarch64 PRIVATE JP_AARCH64_TESTING=1)
jp_link_module_if_exists(test_aarch64 src/boot/architecture/aarch64.c)
