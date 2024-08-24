/*---------------Includes--------------------*/
#include"bootloader.h"

/*--------------Static Function Declarations-----------------*/

static void Bootloader_Get_Version(uint8_t *Host_Buffer);
static void Bootloader_Get_Help(uint8_t *Host_Buffer);
static void Bootloader_Get_Chip_Identification_Number(uint8_t *Host_Buffer);
static void Bootloader_Read_Protection_Level(uint8_t *Host_Buffer);
static void Bootloader_Jump_To_Address(uint8_t *Host_Buffer);
static void Bootloader_Erase_Flash(uint8_t *Host_Buffer);
static void Bootloader_Memory_Write(uint8_t *Host_Buffer);
static void Bootloader_Change_Read_Protection_Level(uint8_t *Host_Buffer);

static uint8_t CRC_Verification(uint8_t *P_data,uint8_t DataLenght,uint32_t Host_CRC32);
static void BL_Send_ACK(uint16_t Reply_Lenght);
static void BL_Send_NACK(void);
static void Bootloader_Send_Data_To_Host(uint8_t *Host_Buffer, uint32_t Data_Len);
static uint8_t Host_Address_Verification(uint32_t Jump_Address);
static uint8_t Bootloader_Perform_Flash_Erase(uint8_t Sector_Num,uint8_t Number_Of_Sectors);
static uint8_t Flash_Memory_Write_Payload(uint8_t *Host_Payload,uint32_t Payload_start_address,uint16_t Payload_Len);		
static uint32_t CBL_READ_PROTECTION_LEVEL(uint8_t *Host_Buffer);
/*--------------Global Variablaes Definitions-----------------*/
static uint8_t BL_HOST_BUFFER[BL_HOST_BUFFER_RX_LENGHT];

static uint8_t Bootloader_Supported_CMDs[8] = {
    CBL_GET_VER_CMD,
    CBL_GET_HELP_CMD,
    CBL_GET_CID_CMD,
    CBL_GET_RDP_STATUS_CMD,
    CBL_GO_TO_ADDR_CMD,
    CBL_FLASH_ERASE_CMD,
    CBL_MEM_WRITE_CMD,
    CBL_CHANGE_ROP_Level_CMD
};
/*-------------Software Interfaces Definitions-----------------*/	
void BL_print_Message(char *format,...){
char Message[100] = {0};
va_list args;
va_start(args,format);
vsprintf(Message,format,args);
#if (BL_ENABLE_DEBUG_UART_MESSAGE == BL_DEBUG_METHOD)
/* Transmit the formatted data through Uart*/
 HAL_UART_Transmit(BL_DEBUG_UART,Message ,15, HAL_MAX_DELAY);
/* Transmit the formatted data through SPI*/
#elif (BL_ENABLE_DEBUG_SPI_MESSAGE == BL_DEBUG_METHOD)
 HAL_UART_Transmit(BL_DEBUG_SPI,Message ,15, HAL_MAX_DELAY);
/* Transmit the formatted data through I2C*/
#elif (BL_ENABLE_DEBUG_I2C_MESSAGE == BL_DEBUG_METHOD)
 HAL_UART_Transmit(BL_DEBUG_I2C,Message ,15, HAL_MAX_DELAY);
#endif
va_end(args);
  
}

BL_Status BL_Uart_Featch_Host_Command(void)
{
BL_Status Status = BL_OK;
HAL_StatusTypeDef Ret_Status = HAL_ERROR;
	uint8_t Data_Lenght = 0;
memset(BL_HOST_BUFFER ,0,BL_HOST_BUFFER_RX_LENGHT);
	/* Receive the First byte which is the BL Command */
	Ret_Status = HAL_UART_Receive(BL_HOST_COMMUNICATION_UART,BL_HOST_BUFFER ,1,HAL_MAX_DELAY);
	if(Ret_Status != HAL_OK){
		/* There is an error */
		Status = BL_NOT_OK;
	}
	else
	{
	Data_Lenght = BL_HOST_BUFFER[0];
	/* Receive the Data after the First byte Command */
	Ret_Status = HAL_UART_Receive(BL_HOST_COMMUNICATION_UART,&BL_HOST_BUFFER[1] ,Data_Lenght ,HAL_MAX_DELAY);
		if(Ret_Status != HAL_OK){
		/* There is an error */
		Status = BL_NOT_OK;
	}
	else
	{
		switch(BL_HOST_BUFFER[1]){
			case CBL_GET_VER_CMD:
					Bootloader_Get_Version(BL_HOST_BUFFER);
					Status = BL_OK;
					break;
				case CBL_GET_HELP_CMD:
					Bootloader_Get_Help(BL_HOST_BUFFER);
					Status = BL_OK;
					break;
				case CBL_GET_CID_CMD:
					Bootloader_Get_Chip_Identification_Number(BL_HOST_BUFFER);
					Status = BL_OK;
					break;
				case CBL_GET_RDP_STATUS_CMD:
					Bootloader_Read_Protection_Level(BL_HOST_BUFFER);
					Status = BL_OK;
					break;
				case CBL_GO_TO_ADDR_CMD:
					Bootloader_Jump_To_Address(BL_HOST_BUFFER);
					Status = BL_OK;
					break;
				case CBL_FLASH_ERASE_CMD:
					Bootloader_Erase_Flash(BL_HOST_BUFFER);
					Status = BL_OK;
					break;
				case CBL_MEM_WRITE_CMD:
					Bootloader_Memory_Write(BL_HOST_BUFFER);
					Status = BL_OK;
					break;
				case CBL_CHANGE_ROP_Level_CMD:
					Bootloader_Change_Read_Protection_Level(BL_HOST_BUFFER);
					Status = BL_OK;
					break;
				default:
					BL_print_Message("Invalid command code received from host !! \r\n");
					break;
		}
			
	}
	
}
	
	
	return Status;
}


/*-------------Static Function Definitions-----------------*/	
static void bootloader_jump_to_user_app(void){
	/* Value of the main stack pointer of our main application */
	uint32_t MSP_Value = *((volatile uint32_t *)FLASH_SECTOR2_BASE_ADDRESS);
	
	/* Reset Handler definition function of our main application */
	uint32_t MainAppAddr = *((volatile uint32_t *)(FLASH_SECTOR2_BASE_ADDRESS + 4));
	
	/* Fetch the reset handler address of the user application */
	pMainApp ResetHandler_Address = (pMainApp)MainAppAddr;
	
	/* Set Main Stack Pointer */
	__set_MSP(MSP_Value);
	
	/* DeInitialize / Disable of modules */
	HAL_RCC_DeInit(); /* DeInitialize the RCC clock configuration to the default reset state. */
	                  /* Disable Maskable Interrupt */
	
	
	/* Jump to Application Reset Handler */
	ResetHandler_Address();
}


static uint8_t CRC_Verification(uint8_t *P_data,uint8_t DataLenght,uint32_t Host_CRC32){
uint8_t CRC_Status = CRC_VERIFICATION_FAILED;
uint32_t MCU_CRC_Calculated = 0;
	uint8_t Data_Counter = 0;
	uint32_t Data_Buffer = 0;

	              /*Calculate the CRC32*/
	for(Data_Counter = 0;Data_Counter < DataLenght;Data_Counter++){
		Data_Buffer = (uint32_t) P_data[Data_Counter];
MCU_CRC_Calculated	= HAL_CRC_Accumulate(CRC_ENGINE_OBJECT ,&Data_Buffer ,DataLenght);
	}
	/* Reset the CRC Engine */
	__HAL_CRC_DR_RESET(CRC_ENGINE_OBJECT);
	
              /* Compare the Host CRC and the BL CRC */
	if(MCU_CRC_Calculated == Host_CRC32)		
	{
		CRC_Status = CRC_VERIFICATION_PASSED;

	}
else
{
CRC_Status = CRC_VERIFICATION_FAILED;

}
return CRC_Status;
}



static void BL_Send_ACK(uint16_t Reply_Lenght ){

uint8_t ACK_Value[2] ={0};
ACK_Value[0] = CBL_SEND_ACK;
ACK_Value[1] = Reply_Lenght;
	HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART ,ACK_Value , 2, HAL_MAX_DELAY);
}

static void BL_Send_NACK(void){

uint8_t ACK_Value =CBL_SEND_NACK;
	HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART ,&ACK_Value , 1, HAL_MAX_DELAY);
}


static void Bootloader_Send_Data_To_Host(uint8_t *Host_Buffer, uint32_t Data_Len){
HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART ,Host_Buffer , Data_Len, HAL_MAX_DELAY);

}
static void Bootloader_Get_Version(uint8_t *Host_Buffer){
uint8_t BL_Version[4] ={BL_VEDNOR_ID,BL_SW_MAJOR_VERSION,BL_SW_MINOR_VERSION,BL_SW_PATCH_VERSION};
uint16_t Host_Packet_Lenght = 0;
uint32_t Host_CRC32 = 0;
uint8_t CRC_RET_STATUS = CRC_VERIFICATION_FAILED;
#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("Read the BL version from the mcu \n ");
#endif
/* Extract the Lenght of the Host packet lenght && Host CRC */
Host_Packet_Lenght = BL_HOST_BUFFER[0] +1;
Host_CRC32 = *((uint32_t *)((Host_Buffer+Host_Packet_Lenght)-CRC_SIZE_BYTE));

               /* CRC Verification */
 CRC_RET_STATUS = CRC_Verification((uint8_t *)&Host_Buffer[0],Host_Packet_Lenght - 4 ,Host_CRC32);
if(CRC_RET_STATUS == CRC_VERIFICATION_PASSED){
BL_Send_ACK(4);
	Bootloader_Send_Data_To_Host((uint8_t *)BL_Version ,4 );
#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Passed \n  ");
#endif
}
else
{

#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Failed \n  ");
#endif
	BL_Send_NACK();
	
}
}
static void Bootloader_Get_Help(uint8_t *Host_Buffer){
uint16_t Host_Packet_Lenght = 0;
uint32_t Host_CRC32 = 0;
uint8_t CRC_RET_STATUS = CRC_VERIFICATION_FAILED;
	          /* Extract the Lenght of the Host packet lenght && Host CRC */
Host_Packet_Lenght = BL_HOST_BUFFER[0] +1;
Host_CRC32 = *((uint32_t *)((Host_Buffer+Host_Packet_Lenght)-CRC_SIZE_BYTE));
	               /* CRC Verification */
 CRC_RET_STATUS = CRC_Verification((uint8_t *)&Host_Buffer[0],Host_Packet_Lenght - 4 ,Host_CRC32);
	if(CRC_RET_STATUS == CRC_VERIFICATION_PASSED){
	#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Passed \n  ");
#endif
	BL_Send_ACK(12);
		 Bootloader_Send_Data_To_Host(Bootloader_Supported_CMDs , 12);
	}
	else
	{
	#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Failed \n  ");
#endif
	BL_Send_NACK();
	}
}
static void Bootloader_Get_Chip_Identification_Number(uint8_t *Host_Buffer){
uint16_t Host_Packet_Lenght = 0;
uint32_t Host_CRC32 = 0;
uint16_t MCU_ID_Number = 0;
uint8_t CRC_RET_STATUS = CRC_VERIFICATION_FAILED;
#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("Read the Bootloader_Get_Chip_Identification_Number \n ");
#endif
		          /* Extract the Lenght of the Host packet lenght && Host CRC */
Host_Packet_Lenght = BL_HOST_BUFFER[0] +1;
Host_CRC32 = *((uint32_t *)((Host_Buffer+Host_Packet_Lenght)-CRC_SIZE_BYTE));
	
		               /* CRC Verification */
 CRC_RET_STATUS = CRC_Verification((uint8_t *)&Host_Buffer[0],Host_Packet_Lenght - 4 ,Host_CRC32);
	
		if(CRC_RET_STATUS == CRC_VERIFICATION_PASSED){
	#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Passed \n  ");
#endif
              /*  Get_Chip_Identification_Number  */
			MCU_ID_Number = (uint16_t)((DBGMCU->IDCODE) & 0x00000FFF);
			/*Report MCU ID NUM TO THE HOST */
			BL_Send_ACK(2);
		 Bootloader_Send_Data_To_Host((uint8_t *)MCU_ID_Number , 2);
	}
	else
	{
	#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Failed \n  ");
#endif
	BL_Send_NACK();
	}
}
static void Bootloader_Read_Protection_Level(uint8_t *Host_Buffer){


}
static uint8_t BL_HOST_JUMP_ADDRESS_Verifiication(uint32_t Host_Jump_Address){
	uint8_t Address_Ver_Status = ADDRESS_IS_INVALID;
	
	if( (Host_Jump_Address >= SRAM1_BASE) && ( Host_Jump_Address <= STM32F401_SRAM1_END)){
	
	Address_Ver_Status = ADDRESS_IS_VALID;
	}
   else if( (Host_Jump_Address >= FLASH_BASE) && ( Host_Jump_Address <= STM32F401_FLASH_END)){
	
	Address_Ver_Status = ADDRESS_IS_VALID;
	}
	 else
	 {
	 Address_Ver_Status = ADDRESS_IS_INVALID;
	 }
return Address_Ver_Status;
}
static void Bootloader_Jump_To_Address(uint8_t *Host_Buffer){
uint16_t Host_Packet_Lenght = 0;
uint32_t Host_CRC32 = 0;
uint32_t Host_Jump_Address = 0;
uint8_t Address_Verifications = ADDRESS_IS_INVALID;
uint8_t CRC_RET_STATUS = CRC_VERIFICATION_FAILED;
	#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("Read the Bootloader_Jump_To_Address \n ");
#endif
	          /* Extract the Lenght of the Host packet lenght && Host CRC */
Host_Packet_Lenght = BL_HOST_BUFFER[0] +1;
Host_CRC32 = *((uint32_t *)((Host_Buffer+Host_Packet_Lenght)-CRC_SIZE_BYTE));
	
	
		               /* CRC Verification */
 CRC_RET_STATUS = CRC_Verification((uint8_t *)&Host_Buffer[0],Host_Packet_Lenght - 4 ,Host_CRC32);
	
		if(CRC_RET_STATUS == CRC_VERIFICATION_PASSED){
	#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Passed \n  ");
#endif
			/* Extract the address From the Host Packet */
					BL_Send_ACK(1);
		 
			Host_Jump_Address = *((uint32_t *)(&Host_Buffer[2]));
			/* Verify the Address if it is valid or not Valid  */
			Address_Verifications = BL_HOST_JUMP_ADDRESS_Verifiication(Host_Jump_Address);
			if(Address_Verifications == ADDRESS_IS_VALID ){
			/*  Valid Adress  & Jump to the Address Recevied From the Host */
				
			Bootloader_Send_Data_To_Host((uint8_t *)Address_Verifications , 1);
			Jump_Ptr Jump_Address = ((Jump_Ptr)Host_Jump_Address + 1);
				Jump_Address();
					#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
				BL_print_Message("Jump to : 0x%X \n  ",Jump_Address);
#endif
				
			} 
			else
			{
				/*  InValid Adress */
					Bootloader_Send_Data_To_Host((uint8_t *)Address_Verifications , 1);
			
				
			}
			/* Report Chip Identification num to Host*/
		
			
		}
		else
		{
		#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Failed \n  ");
#endif
	BL_Send_NACK();
		}
}
static uint8_t Bootloader_Perform_Flash_Erase(uint8_t Sector_Num,uint8_t Number_Of_Sectors){
uint8_t EraseStatus = ERASE_SUCCESS;
FLASH_EraseInitTypeDef 	pEraseInit;
uint8_t Remaining_Sectors = 0;
uint32_t Sector_Error = 0;
HAL_StatusTypeDef Hal_Status =  HAL_ERROR;
if(Number_Of_Sectors > CBL_FLASH_SECTOR_MAX_NUM ){
EraseStatus = ERASE_FAILED;

}
else{
if(Sector_Num <= (CBL_FLASH_SECTOR_MAX_NUM - 1) || (Sector_Num == CBL_FLASH_MASS_ERASE)){

	if(Sector_Num == CBL_FLASH_MASS_ERASE ){
	/* Flash Mass Erase Activation*/
		pEraseInit.TypeErase = FLASH_TYPEERASE_MASSERASE;/* Erase all Sectors */
#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
				BL_print_Message("Flash Mass Erase Activation");
#endif
	
	}
	else
	{
		#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
				BL_print_Message("Check if the entered number is Valid");
#endif
		/* Check if user enter Number of sectors Valid */
	Remaining_Sectors = CBL_FLASH_SECTOR_MAX_NUM - Sector_Num;
	if(Remaining_Sectors < Number_Of_Sectors )
	{
	Number_Of_Sectors = Remaining_Sectors;
	}
	/* Sector Erase Activation */
		pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;/* Erase Sectors Only */
		pEraseInit.NbSectors = Number_Of_Sectors;/* Number of Sectors */
		pEraseInit.Sector    = Sector_Num;/* The Sector Number which is should be erased*/
		
	}
	pEraseInit.Banks = FLASH_BANK_1;
  pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	/* Unlock the Flash Control Register */
Hal_Status = 	HAL_FLASH_OB_Unlock();	
	/*Perform a mass erase or a specified Flash memory sectors */
	Hal_Status = HAL_FLASHEx_Erase(&pEraseInit ,&Sector_Error );
	if(Sector_Error == HAL_SUCCESSFUL_ERASE )
	{
	   EraseStatus = ERASE_SUCCESS;
	
	}
	else
	{
	EraseStatus = ERASE_FAILED;
	
	}
	/* lock the Flash Control Register */
	Hal_Status = HAL_FLASH_OB_Lock();
}
else{
EraseStatus = ERASE_FAILED;
}
}

return EraseStatus;
}
static void Bootloader_Erase_Flash(uint8_t *Host_Buffer){
uint16_t Host_Packet_Lenght = 0;
uint32_t Host_CRC32 = 0;
uint8_t Erase_Status = ERASE_FAILED;
uint8_t CRC_RET_STATUS = CRC_VERIFICATION_FAILED;
	#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("BL Read the Erase Flash Command From the Host \n ");
#endif
	          /* Extract the Lenght of the Host packet lenght && Host CRC */
Host_Packet_Lenght = BL_HOST_BUFFER[0] +1;
Host_CRC32 = *((uint32_t *)((Host_Buffer+Host_Packet_Lenght)-CRC_SIZE_BYTE));
	
	
		               /* CRC Verification */
 CRC_RET_STATUS = CRC_Verification((uint8_t *)&Host_Buffer[0],Host_Packet_Lenght - 4 ,Host_CRC32);
		if(CRC_RET_STATUS == CRC_VERIFICATION_PASSED){
	#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Passed \n  ");
#endif
			BL_Send_ACK(1);
		Erase_Status =	Bootloader_Perform_Flash_Erase(Host_Buffer[2],Host_Buffer[3]);
			if(Erase_Status == ERASE_SUCCESS ){
			
				/*  Report Erase Status Passed */
					Bootloader_Send_Data_To_Host((uint8_t *)Erase_Status , 1);
			}
			else
			{
				/*  Report Erase Status Failed */
					Bootloader_Send_Data_To_Host((uint8_t *)Erase_Status , 1);
			}
		}
		else
		{
		#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Failed \n  ");
#endif
					
	BL_Send_NACK();
}
		}

static uint8_t Flash_Memory_Write_Payload(uint8_t *Host_Payload,uint32_t Payload_start_address,uint16_t Payload_Len)		{
		HAL_StatusTypeDef HAL_Status = HAL_ERROR;
	uint8_t Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
	  uint16_t PayLoad_Counter = 0;
	/* Unlock the flash control register */
HAL_Status = HAL_FLASH_Unlock();
	if(HAL_Status == HAL_ERROR){
	Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
	}
	
	else
	{
	for(PayLoad_Counter = 0; PayLoad_Counter < Payload_Len; PayLoad_Counter++){
	 HAL_Status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,(Payload_start_address + PayLoad_Counter),Host_Payload[PayLoad_Counter]);
		if(HAL_Status !=  HAL_OK)
			{
			Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
		break;		
		}
			else
			{
				Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_PASSED;
			}
	}
}		
	if(Flash_Payload_Write_Status == FLASH_PAYLOAD_WRITE_PASSED && HAL_Status == HAL_OK)
	{
	     /* Flash_Memory_Write_Payload Done Successfully && Lock the Flash Memory Access  */
	   HAL_Status = HAL_FLASH_Lock();
		if(HAL_Status == HAL_OK)
		{
		  Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_PASSED; 
		}
		else
		{
		Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED; 
		}
	}
	
		
		return Flash_Payload_Write_Status;
}
static void Bootloader_Memory_Write(uint8_t *Host_Buffer){
uint16_t Host_Packet_Lenght = 0;
uint32_t Host_CRC32   = 0;
uint32_t Host_Address = 0;
uint8_t  Payload_Len  = 0;
uint8_t Address_Veri = ADDRESS_IS_INVALID;
uint8_t CRC_RET_STATUS = CRC_VERIFICATION_FAILED;
uint8_t Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
	#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("BL Write Data in Memory \n ");
#endif
	          /* Extract the Lenght of the Host packet lenght && Host CRC */
Host_Packet_Lenght = BL_HOST_BUFFER[0] +1;
Host_CRC32 = *((uint32_t *)((Host_Buffer+Host_Packet_Lenght)-CRC_SIZE_BYTE));
	
	
		               /* CRC Verification */
 CRC_RET_STATUS = CRC_Verification((uint8_t *)&Host_Buffer[0],Host_Packet_Lenght - 4 ,Host_CRC32);
		if(CRC_RET_STATUS == CRC_VERIFICATION_PASSED){
	#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Passed \n  ");
#endif
			/* Send the ACK to the Host*/
			BL_Send_ACK(1);
			/* Load the Address which will be written */
			Host_Address = *((uint32_t *)(&Host_Buffer[2]));
			/* Load the data written in the Address */
			Payload_Len = Host_Buffer[6];
			Address_Veri =  BL_HOST_JUMP_ADDRESS_Verifiication(Host_Address);
			if( Address_Veri == ADDRESS_IS_VALID )
   {
   Bootloader_Send_Data_To_Host(((uint8_t *)Address_Veri) , 1);
   Flash_Payload_Write_Status = Flash_Memory_Write_Payload(((uint8_t *)&Host_Buffer[7]),Host_Address,Payload_Len );
		 if( Flash_Payload_Write_Status == FLASH_PAYLOAD_WRITE_PASSED)
		 {
			 /* Report Write in Memory Successfully */
		     Bootloader_Send_Data_To_Host(((uint8_t *)Flash_Payload_Write_Status) , 1);
		 }
		 else
		 {
		 /* Report Write in Memory Failed  */
		     Bootloader_Send_Data_To_Host(((uint8_t *)Flash_Payload_Write_Status) , 1);
		 }
   }
	 else
			{

					
			}
			
			
}
			else
			{
				#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Failed \n  ");
#endif
			BL_Send_NACK();
			}

		}
	
static uint8_t Change_ROP_Level(uint32_t ROP_Level){
 HAL_StatusTypeDef HAL_Status = HAL_ERROR;
	FLASH_OBProgramInitTypeDef FLASH_OBProgramInit;
	uint8_t ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
	
	/* Unlock the FLASH Option Control Registers access */
	HAL_Status = HAL_FLASH_OB_Unlock();
	if(HAL_Status != HAL_OK){
		ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("Failed -> Unlock the FLASH Option Control Registers access \r\n");
#endif
	}
	else{
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
		BL_print_Message("Passed -> Unlock the FLASH Option Control Registers access \r\n");
#endif
		FLASH_OBProgramInit.OptionType = OPTIONBYTE_RDP; /* RDP option byte configuration */
		FLASH_OBProgramInit.Banks = FLASH_BANK_1;
		FLASH_OBProgramInit.RDPLevel = ROP_Level;
		/* Program option bytes */
		HAL_Status = HAL_FLASHEx_OBProgram(&FLASH_OBProgramInit);
		if(HAL_Status != HAL_OK){
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
			BL_print_Message("Failed -> Program option bytes \r\n");
#endif
			HAL_Status = HAL_FLASH_OB_Lock();
			ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
		}
		else{
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
			BL_print_Message("Passed -> Program option bytes \r\n");
#endif
			/* Launch the option byte loading */
			HAL_Status = HAL_FLASH_OB_Launch();
			if(HAL_Status != HAL_OK){
				ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
			}
			else{
				/* Lock the FLASH Option Control Registers access */
				HAL_Status = HAL_FLASH_OB_Lock();
				if(HAL_Status != HAL_OK){
					ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
				}
				else{
					ROP_Level_Status = ROP_LEVEL_CHANGE_VALID;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
					BL_print_Message("Passed -> Program ROP to Level : 0x%X \r\n", ROP_Level);
#endif
				}
			}
		}
	}
	return ROP_Level_Status;
}
static void Bootloader_Change_Read_Protection_Level(uint8_t *Host_Buffer){
uint16_t Host_CMD_Packet_Len = 0;
  uint32_t Host_CRC32 = 0;
	uint8_t ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
	uint8_t Host_ROP_Level = 0;
	uint8_t CRC_RET_STATUS = CRC_VERIFICATION_FAILED;
	#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("Change read protection level of the user flash \r\n");
#endif
	 /* Extract the Lenght of the Host packet lenght && Host CRC */
Host_CMD_Packet_Len = BL_HOST_BUFFER[0] +1;
Host_CRC32 = *((uint32_t *)((Host_Buffer+Host_CMD_Packet_Len)-CRC_SIZE_BYTE));
		               /* CRC Verification */
 CRC_RET_STATUS = CRC_Verification((uint8_t *)&Host_Buffer[0],Host_CMD_Packet_Len - 4 ,Host_CRC32);
		if(CRC_RET_STATUS == CRC_VERIFICATION_PASSED){
	#if      (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_print_Message("CRC Verification Passed \n  ");
#endif
			/* Send the ACK to the Host*/
			BL_Send_ACK(1);
			/* Request change the Read Out Protection Level */
		Host_ROP_Level = Host_Buffer[2];
		/* Warning: When enabling read protection level 2, it s no more possible to go back to level 1 or 0 */
		if((CBL_ROP_LEVEL_2 == Host_ROP_Level) || (OB_RDP_LEVEL_2 == Host_ROP_Level)){
			ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
		}
		else{
			if(CBL_ROP_LEVEL_0 == Host_ROP_Level){ 
				Host_ROP_Level = 0xAA; 
			}
			else if(CBL_ROP_LEVEL_1 == Host_ROP_Level){ 
				Host_ROP_Level = 0x55; 
			}
			ROP_Level_Status = Change_ROP_Level(Host_ROP_Level);
		}
		Bootloader_Send_Data_To_Host((uint8_t *)&ROP_Level_Status, 1);
	}
		else{
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
		BL_print_Message("CRC Verification Failed \r\n");
#endif
		BL_Send_NACK();
	}
		
}
		


