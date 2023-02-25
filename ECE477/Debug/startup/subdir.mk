################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../startup/startup_stm32.s 

OBJS += \
./startup/startup_stm32.o 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m0 -mthumb -mfloat-abi=soft -I"C:/Users/Ryan/Documents/GitHub/ece477/ECE477/CPAL_Driver/inc" -I"C:/Users/Ryan/Documents/GitHub/ece477/ECE477/StdPeriph_Driver/inc" -I"C:/Users/Ryan/Documents/GitHub/ece477/ECE477/inc" -I"C:/Users/Ryan/Documents/GitHub/ece477/ECE477/CMSIS/device" -I"C:/Users/Ryan/Documents/GitHub/ece477/ECE477/CMSIS/core" -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


