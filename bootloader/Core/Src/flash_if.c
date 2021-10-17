/**
 ******************************************************************************
 * @file    IAP_Main/Src/flash_if.c
 * @author  MCD Application Team
 * @version 1.0.0
 * @date    8-April-2015
 * @brief   This file provides all the memory related operation functions.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/** @addtogroup STM32F4xx_IAP
 * @{
 */

#include "flash_if.h"

static uint32_t GetSector(uint32_t Address);

/* Clear flags */
void FLASH_If_Init(void)
{
	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
			               FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
	HAL_FLASH_Lock();
}

/* Erase flash memory */
uint32_t FLASH_If_Erase(uint32_t start)
{
	uint32_t FirstSector, NbOfSectors, SectorError;
	FLASH_EraseInitTypeDef pEraseInit;
	HAL_StatusTypeDef status = HAL_OK;

	HAL_FLASH_Unlock();

	/* Erase from SECTOR 2~5 */
	FirstSector = GetSector(USER_START_ADDRESS);
	NbOfSectors = GetSector(USER_END_ADDRESS) - FirstSector;

	pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
	pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	pEraseInit.Sector = FirstSector;
	pEraseInit.NbSectors = NbOfSectors;
	status = HAL_FLASHEx_Erase(&pEraseInit, &SectorError);

	HAL_FLASH_Lock();

	if (status != HAL_OK)
	{
		return FLASHIF_ERASEKO;
	}

	return FLASHIF_OK;
}

/* Write flash memory */
uint32_t FLASH_If_Write(uint32_t destination, uint32_t *p_source, uint32_t length)
{
	uint32_t i = 0;

	HAL_FLASH_Unlock();

	for (i = 0; (i < length) && (destination <= (USER_END_ADDRESS-4)); i++)
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, destination, *(uint32_t*)(p_source+i)) == HAL_OK)
		{
			/* Validate the written value */
			if (*(uint32_t*)destination != *(uint32_t*)(p_source+i))
			{
				HAL_FLASH_Lock();
				return (FLASHIF_WRITINGCTRL_ERROR);
			}

			/* Increase WORD length */
			destination += 4;
		}
		else
		{
			HAL_FLASH_Lock();
			return (FLASHIF_WRITING_ERROR);
		}
	}

	HAL_FLASH_Lock();
	return (FLASHIF_OK);
}

/* Check write protection */
uint32_t FLASH_If_GetWriteProtectionStatus(void)
{
	uint32_t ProtectedSector = FLASHIF_PROTECTION_NONE;
	FLASH_OBProgramInitTypeDef OptionsBytesStruct;

	HAL_FLASH_Unlock();
	HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);
	HAL_FLASH_Lock();

	/* If sectors are protected, WRPSector bits are zero, so it needs to be inverted */
	ProtectedSector = ~(OptionsBytesStruct.WRPSector) & USER_WRP_SECTORS;

	if(ProtectedSector != 0)
	{
		return FLASHIF_PROTECTION_WRPENABLED;
	}

	return FLASHIF_PROTECTION_NONE;
}

/* Configure write protection */
uint32_t FLASH_If_WriteProtectionConfig(uint32_t protectionstate)
{
	FLASH_OBProgramInitTypeDef OBInit;
	HAL_StatusTypeDef status = HAL_OK;

	HAL_FLASH_OB_Unlock();
	HAL_FLASH_Unlock();

	/* Configure sector write protection */
	OBInit.OptionType = OPTIONBYTE_WRP;
	OBInit.Banks = FLASH_BANK_1;
	OBInit.WRPState = (protectionstate == FLASHIF_WRP_ENABLE ? OB_WRPSTATE_ENABLE : OB_WRPSTATE_DISABLE);
	OBInit.WRPSector = USER_WRP_SECTORS;

	HAL_FLASHEx_OBProgram(&OBInit);
	status = HAL_FLASH_OB_Launch();

	HAL_FLASH_OB_Lock();
	HAL_FLASH_Lock();

	if(status != HAL_OK)
	{
		return FLASHIF_PROTECTION_ERRROR;
	}
	else
	{
		return FLASHIF_OK;
	}
}

/* Get sector number by address */
static uint32_t GetSector(uint32_t Address)
{
	uint32_t sector = 0;

	if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
	{
		sector = FLASH_SECTOR_0;
	}
	else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
	{
		sector = FLASH_SECTOR_1;
	}
	else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
	{
		sector = FLASH_SECTOR_2;
	}
	else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
	{
		sector = FLASH_SECTOR_3;
	}
	else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
	{
		sector = FLASH_SECTOR_4;
	}
	else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
	{
		sector = FLASH_SECTOR_5;
	}
	else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
	{
		sector = FLASH_SECTOR_6;
	}
	else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
	{
		sector = FLASH_SECTOR_7;
	}
	else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
	{
		sector = FLASH_SECTOR_8;
	}
	else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
	{
		sector = FLASH_SECTOR_9;
	}
	else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
	{
		sector = FLASH_SECTOR_10;
	}
	else
	{
		sector = FLASH_SECTOR_11;
	}

	return sector;
}
/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
