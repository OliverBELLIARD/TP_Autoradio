################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/drivers/MCP23S17.c \
../Core/drivers/SGTL5000.c 

OBJS += \
./Core/drivers/MCP23S17.o \
./Core/drivers/SGTL5000.o 

C_DEPS += \
./Core/drivers/MCP23S17.d \
./Core/drivers/SGTL5000.d 


# Each subdirectory must supply rules for building sources it contributes
Core/drivers/%.o Core/drivers/%.su Core/drivers/%.cyclo: ../Core/drivers/%.c Core/drivers/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L476xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-drivers

clean-Core-2f-drivers:
	-$(RM) ./Core/drivers/MCP23S17.cyclo ./Core/drivers/MCP23S17.d ./Core/drivers/MCP23S17.o ./Core/drivers/MCP23S17.su ./Core/drivers/SGTL5000.cyclo ./Core/drivers/SGTL5000.d ./Core/drivers/SGTL5000.o ./Core/drivers/SGTL5000.su

.PHONY: clean-Core-2f-drivers

