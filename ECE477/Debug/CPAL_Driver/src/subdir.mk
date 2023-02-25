################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CPAL_Driver/src/stm32f0xx_i2c_cpal.c \
../CPAL_Driver/src/stm32f0xx_i2c_cpal_hal.c \
../CPAL_Driver/src/stm32f0xx_i2c_cpal_usercallback_template.c 

OBJS += \
./CPAL_Driver/src/stm32f0xx_i2c_cpal.o \
./CPAL_Driver/src/stm32f0xx_i2c_cpal_hal.o \
./CPAL_Driver/src/stm32f0xx_i2c_cpal_usercallback_template.o 

C_DEPS += \
./CPAL_Driver/src/stm32f0xx_i2c_cpal.d \
./CPAL_Driver/src/stm32f0xx_i2c_cpal_hal.d \
./CPAL_Driver/src/stm32f0xx_i2c_cpal_usercallback_template.d 


# Each subdirectory must supply rules for building sources it contributes
CPAL_Driver/src/%.o: ../CPAL_Driver/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -mfloat-abi=soft -DSTM32 -DSTM32F0 -DSTM32F091RCTx -DDEBUG -DSTM32F091 -DUSE_STDPERIPH_DRIVER -I"C:/Users/Ryan/Documents/GitHub/ece477/ECE477/CPAL_Driver/inc" -I"C:/Users/Ryan/Documents/GitHub/ece477/ECE477/StdPeriph_Driver/inc" -I"C:/Users/Ryan/Documents/GitHub/ece477/ECE477/inc" -I"C:/Users/Ryan/Documents/GitHub/ece477/ECE477/CMSIS/device" -I"C:/Users/Ryan/Documents/GitHub/ece477/ECE477/CMSIS/core" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


