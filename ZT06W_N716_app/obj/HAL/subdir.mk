################################################################################
# MRS Version: 1.9.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../HAL/KEY.c \
../HAL/LED.c \
../HAL/MCU.c \
../HAL/RTC.c \
../HAL/SLEEP.c 

OBJS += \
./HAL/KEY.o \
./HAL/LED.o \
./HAL/MCU.o \
./HAL/RTC.o \
./HAL/SLEEP.o 

C_DEPS += \
./HAL/KEY.d \
./HAL/LED.d \
./HAL/MCU.d \
./HAL/RTC.d \
./HAL/SLEEP.d 


# Each subdirectory must supply rules for building sources it contributes
HAL/%.o: ../HAL/%.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -DDEBUG=2 -DCLK_OSC32K=0x82 -I"../StdPeriphDriver/inc" -I"R:\CODE\CHcode\CH582\ZT06W_N716\ZT06W_N716_app\Task\inc" -I"R:\CODE\CHcode\CH582\ZT06W_N716\ZT06W_N716_app\APP\include" -I"R:\CODE\CHcode\CH582\ZT06W_N716\ZT06W_N716_app\LIB" -I"R:\CODE\CHcode\CH582\ZT06W_N716\ZT06W_N716_app\HAL\include" -I"R:\CODE\CHcode\CH582\ZT06W_N716\ZT06W_N716_app\APP" -I"../RVMSIS" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

