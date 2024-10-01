ARCH ?= i686
CC = $(ARCH)-elf-gcc
LD = $(ARCH)-elf-ld
AS = nasm -f elf32
AR = ar rcs
CPP = cpp
CFLAGS = -ffreestanding -g -nostdlib
CWARNS = -Wall -Wextra -fno-exceptions

B_DIR = boot
K_DIR = kernel
L_DIR = lib
M_DIR = melon

BOOT = melonos-bootloader
KERNEL = melonos-kernel
LIBUTILS = libutils.a
LIBMELON = libmelon.a
MELONOS = melonos.img

QEMU = qemu-system-i386

.PHONY: boot kernel all

default: all
all: $(MELONOS)
boot: $(BOOT)
kernel: $(KERNEL)

$(MELONOS): $(BOOT) $(KERNEL)
	dd if=/dev/zero of=$(MELONOS) count=10000
	dd if=$(BOOT) of=$(MELONOS) conv=notrunc
	dd if=$(KERNEL) of=$(MELONOS) seek=20 conv=notrunc


.PHONY: clean qemu-debug copy echo
clean:
	find $(K_DIR) \( -name "*.o" -o -name "*.pp.*" \) -exec rm {} \;
	find $(B_DIR) \( -name "*.o" -o -name "*.pp.*" \) -exec rm {} \;
	find $(L_DIR) \( -name "*.o" -o -name "*.pp.*" \) -exec rm {} \;
	rm -rf *.o *.pp.* $(MELONOS) $(BOOT) $(KERNEL) $(LIBUTILS) $(LIBMELON)

echo:
	@echo 'CC $(CC)'
	@echo 'AS $(AS)'
	@echo 'MELONOS $(MELONOS)'
	@echo 'BOOT	$(BOOT)'
	@echo 'KERNEL $(KERNEL)'
	@echo 'K_OBJS $(K_OBJS)'
	@echo 'K_ASMFILES $(K_ASMFILES)'
	@echo 'K_LINKER $(K_LINKER)'
	@echo 'B_OBJS $(B_OBJS)'
	@echo 'B_ASMFILES $(B_ASMFILES)'
	@echo 'CFLAGS $(CFLAGS)'
	@echo 'CWARNS $(CWARNS)'

qemu-boot:
	$(QEMU) -drive format=raw,file=$(BOOT)

qemu:
	$(QEMU) -drive format=raw,file=$(MELONOS) -device virtio-vga,xres=640,yres=320

qemu-log:
	$(QEMU) -drive format=raw,file=$(MELONOS) -d 'int,cpu_reset,guest_errors,in_asm,exec' \
		-no-reboot -D .qemu.log \
		-serial file:.uart.log \
		-monitor stdio

qemu-debug:
	$(QEMU) -drive format=raw,file=$(MELONOS) -d 'int,cpu_reset,guest_errors,in_asm,exec' \
		-s -S \
		-no-reboot -D .qemu.log \
		-serial file:.uart.log

elf-headers:
	readelf -headers $(KERNEL)

d:
	objdump -d $(KERNEL)

hex:
	hexdump -C $(MELONOS)

watch:
	tail -f -n 1 .uart.log

cc:
	bear -- make $(MELONOS)

# subfolder makefiles
include boot/Makefile
include kernel/Makefile
include lib/Makefile

-include .local.mk
