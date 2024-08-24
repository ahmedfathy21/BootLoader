
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

/*---------------Includes--------------------*/
#include "usart.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "crc.h"
/*--------------Macro Definition-----------------*/
#define BL_DEBUG_UART                   &huart1
#define BL_HOST_COMMUNICATION_UART      &huart2
#define CRC_ENGINE_OBJECT               &hcrc

#define DEBUG_INFO_DISABLE               0
#define DEBUG_INFO_ENABLE                1
#define BL_DEBUG_ENABLE            DEBUG_INFO_ENABLE

#define BL_ENABLE_DEBUG_UART_MESSAGE 0x00
#define BL_ENABLE_DEBUG_SPI_MESSAGE  0x01
#define BL_ENABLE_DEBUG_I2C_MESSAGE  0x02

#define BL_DEBUG_METHOD    (BL_ENABLE_DEBUG_UART_MESSAGE)

#define BL_HOST_BUFFER_RX_LENGHT            200


#define CBL_GET_VER_CMD              0x10
#define CBL_GET_HELP_CMD             0x11
#define CBL_GET_CID_CMD              0x12
/* Get Read Protection Status */
#define CBL_GET_RDP_STATUS_CMD       0x13
#define CBL_GO_TO_ADDR_CMD           0x14
#define CBL_FLASH_ERASE_CMD          0x15
#define CBL_MEM_WRITE_CMD            0x16
/* Enable/Disable Write Protection */
#define CBL_ED_W_PROTECT_CMD         0x17
#define CBL_MEM_READ_CMD             0x18
/* Get Sector Read/Write Protection Status */
#define CBL_READ_SECTOR_STATUS_CMD   0x19
#define CBL_OTP_READ_CMD             0x20
/* Change Read Out Protection Level */
#define CBL_CHANGE_ROP_Level_CMD     0x21

#define BL_VEDNOR_ID									7
#define BL_SW_MAJOR_VERSION						1
#define BL_SW_MINOR_VERSION						0
#define BL_SW_PATCH_VERSION						0
       /*  CRC Verification     */
#define CRC_SIZE_BYTE                      4

#define CRC_VERIFICATION_FAILED                    0
#define CRC_VERIFICATION_PASSED                    1

#define CBL_SEND_NACK                0xAB
#define CBL_SEND_ACK                 0xCD

/* Start address of sector 2 */
#define FLASH_SECTOR2_BASE_ADDRESS   0x08008000U

#define ADDRESS_IS_INVALID           0x00
#define ADDRESS_IS_VALID             0x01

#define STM32F401_SRAM1_SIZE              (64 * 1024)
#define STM32F401_FLASH_SIZE              (256 * 1024)
#define STM32F401_SRAM1_END              (SRAM1_BASE + STM32F401_SRAM1_SIZE)
#define STM32F401_FLASH_END              (FLASH_BASE + STM32F401_FLASH_SIZE)

/* Erase Commands Defines */
#define CBL_FLASH_SECTOR_MAX_NUM                 8
#define CBL_FLASH_MASS_ERASE                  0xFF
#define ERASE_FAILED                             0
#define ERASE_SUCCESS                            1

#define HAL_SUCCESSFUL_ERASE             0xFFFFFFFFU

/* CBL_MEM_WRITE_CMD */
#define FLASH_PAYLOAD_WRITE_FAILED       0x00
#define FLASH_PAYLOAD_WRITE_PASSED       0x01

#define FLASH_LOCK_WRITE_FAILED          0x00
#define FLASH_LOCK_WRITE_PASSED          0x01

/* CBL_CHANGE_ROP_Level_CMD */
#define ROP_LEVEL_CHANGE_INVALID         0x00
#define ROP_LEVEL_CHANGE_VALID           0X01

#define CBL_ROP_LEVEL_0                  0x00
#define CBL_ROP_LEVEL_1                  0x01
#define CBL_ROP_LEVEL_2                  0x02
/*--------------Macro Functions-----------------*/


/*--------------Data types-----------------*/
typedef enum{
BL_NOT_OK = 0,
BL_OK =1,

}BL_Status;

typedef void (*pMainApp)(void);
typedef void (*Jump_Ptr)(void);
/*--------------Software Interfaces-----------------*/
void BL_print_Message(char *format,...);
BL_Status BL_Uart_Featch_Host_Command(void);
#endif /* BOOTLOADER_H*/