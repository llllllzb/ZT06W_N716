################################################################################
# MRS Version: 1.9.1
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include StdPeriphDriver/subdir.mk
-include Startup/subdir.mk
-include RVMSIS/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
ZT06W_boot.hex \

SECONDARY_LIST += \
ZT06W_boot.lst \

SECONDARY_SIZE += \
ZT06W_boot.siz \


# All Target
all: ZT06W_boot.elf secondary-outputs

# Tool invocations
ZT06W_boot.elf: $(OBJS) $(USER_OBJS)
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -T "../Ld/Link.ld" -nostartfiles -Xlinker --gc-sections -L"../" -L"../StdPeriphDriver" -Xlinker --print-memory-usage -Wl,-Map,"ZT06W_boot.map" --specs=nano.specs -u _printf_float --specs=nosys.specs -o "ZT06W_boot.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@	@
ZT06W_boot.hex: ZT06W_boot.elf
	@	riscv-none-embed-objcopy -O ihex "ZT06W_boot.elf"  "ZT06W_boot.hex"
	@	@
ZT06W_boot.lst: ZT06W_boot.elf
	@	riscv-none-embed-objdump --all-headers --demangle --disassemble "ZT06W_boot.elf" > "ZT06W_boot.lst"
	@	@
ZT06W_boot.siz: ZT06W_boot.elf
	@	riscv-none-embed-size --format=berkeley "ZT06W_boot.elf"
	@	@
# Other Targets
clean:
	-$(RM) $(ASM_UPPER_DEPS)$(OBJS)$(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE)$(ASM_DEPS)$(S_DEPS)$(S_UPPER_DEPS)$(C_DEPS) ZT06W_boot.elf
	-@
secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets
