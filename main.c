/******************************************************************************
* File Name:   main.c
*
Description: This is the source code for Code Flash Sector Data Updating Example
*              for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
 * (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

#include "cybsp.h"
#include "cy_pdl.h"
#include "cy_retarget_io.h"
#include "mtb_hal.h"

/*******************************************************************************
* Macros
*******************************************************************************/

/* Select flash address to be tested.  */
#define TEST_TARGET_ADDR    (CY_FLASH_SM_SBM_END - CY_CODE_SES_SIZE_IN_BYTE)
#define TEST_TARGET_SIZE    CY_FLASH_SIZEOF_ROW

#define TEST_EMPTY_ADDR     (CY_FLASH_SM_SBM_END - CY_CODE_SES_SIZE_IN_BYTE*2)
#define TEST_EMPTY_SIZE     CY_FLASH_SIZEOF_ROW

/*******************************************************************************
* Function Prototypes
*******************************************************************************/


/*******************************************************************************
* Global Variables
*******************************************************************************/
/* For the Retarget -IO (Debug UART) usage */
static cy_stc_scb_uart_context_t    UART_context;           /** UART context */
static mtb_hal_uart_t               UART_hal_obj;           /** Debug UART HAL object  */

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* Main function
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    uint32_t index;
    cy_rslt_t     result;
    uint8_t write_buff[CY_FLASH_SIZEOF_ROW];
    uint8_t read_buff[CY_FLASH_SIZEOF_ROW];

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

#if defined (CY_IP_M7CPUSS)
    SCB_DisableDCache();
#endif

    /* Debug UART init */
    result = (cy_rslt_t)Cy_SCB_UART_Init(UART_HW, &UART_config, &UART_context);

    /* UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    Cy_SCB_UART_Enable(UART_HW);

    /* Setup the HAL UART */
    result = mtb_hal_uart_setup(&UART_hal_obj, &UART_hal_config, &UART_context, NULL);

    /* HAL UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    result = cy_retarget_io_init(&UART_hal_obj);

    /* HAL retarget_io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    printf("\r\n__________________________________________________________________________\r\n*\t\t"\
            "PDL: Code Flash Sector Data Updating\r\n*\r\n*\tThis code example"\
            "shows how to update data for code flash sector\r\n*\t"\
            "UART Terminal Settings: Baud Rate - 115200 bps, 8N1\r\n*"\
            "\r\n__________________________________________________________________________\r\n\n");

    /* 1.Initialize code flash. */
    /* Initialize the data in RAM that will be written into target flash sector*/
    for(index = 0; index < TEST_TARGET_SIZE; index++)
    {
        write_buff[index] = (uint8_t)index;
    }

    /* Initialization */
    Cy_Flash_Init();
    Cy_Flashc_WorkWriteEnable();
    /*Enable code flash write function */
    Cy_Flashc_MainWriteEnable();
    printf("[1] Code flash Init success! \r\n\n");

    /* 2.Erase Target sector and Program source data. */
    printf("\r__________________________________________________________________________\r\n\n");
    printf("[2] Program source data into Target sector \r\n");

    printf("Target sector erase start.. \r\n");
    /* Erase code flash target sector */
    Cy_Flash_EraseSector(TEST_TARGET_ADDR);
    for(uint32_t PageNum = 0; PageNum < (CY_CODE_SES_SIZE_IN_BYTE/CY_FLASH_SIZEOF_ROW); PageNum++)
    {
        memset(read_buff, 0, sizeof(read_buff));
        memcpy(read_buff, (void *)(TEST_TARGET_ADDR + (PageNum * CY_FLASH_SIZEOF_ROW)), TEST_TARGET_SIZE);
        /* Verify erase state */
        for(uint32_t i = 0; i < CY_FLASH_SIZEOF_ROW; i++)
        {
           if(read_buff[i] != 0xFF)
           {
               CY_ASSERT(0);
           }
        }
    }
    printf("Target sector erase finished! \r\n\n");

    printf("Target sector program start.. \r\n");
    /* Programming code flash target sector */
    result = Cy_Flash_ProgramRow(TEST_TARGET_ADDR,(const uint32_t*)write_buff);
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
   /* Verify code flash write success */
    memset(read_buff, 0, sizeof(read_buff));
    memcpy(read_buff, (void *)TEST_TARGET_ADDR, TEST_TARGET_SIZE);
    for(uint32_t i = 0; i < TEST_TARGET_SIZE; i++)
    {
        if(write_buff[i] != read_buff[i])
        {
            CY_ASSERT(0);
        }
    }
    printf("Target sector program finished! \r\n\n");

    /* 3.Read entire sector data from target sector. the sector size is 32k or 8k,
        the data size smaller than the sector size, it's could any size. and program the data into empty sector. */
    printf("\r__________________________________________________________________________\r\n\n");
    printf("[3] Read out target sector data into RAM, and program to empty sector.\r\n");

    printf("Empty sector erase start.. \r\n");
    /* Erase empty sector */
    Cy_Flash_EraseSector(TEST_EMPTY_ADDR);
    /* Verify erase state */
    for(uint32_t PageNum = 0; PageNum < (CY_CODE_SES_SIZE_IN_BYTE/CY_FLASH_SIZEOF_ROW); PageNum++)
    {
        memset(read_buff, 0, sizeof(read_buff));
        memcpy(read_buff, (void *)(TEST_EMPTY_ADDR + (PageNum * CY_FLASH_SIZEOF_ROW)), TEST_TARGET_SIZE);
        /* Verify erase state */
        for(uint32_t i = 0; i < CY_FLASH_SIZEOF_ROW; i++)
        {
           if(read_buff[i] != 0xFF)
           {
               CY_ASSERT(0);
           }
        }
    }
    printf("Empty sector erase finished! \r\n\n");
    
    printf("Empty sector program start.. \r\n");
    /* Programming code flash target sector */
    for(uint32_t PageNum = 0; PageNum < (CY_CODE_SES_SIZE_IN_BYTE/CY_FLASH_SIZEOF_ROW); PageNum++)
    {
        memset(write_buff, 0, sizeof(write_buff));
        memcpy(write_buff, (void *)(TEST_TARGET_ADDR + (PageNum * CY_FLASH_SIZEOF_ROW)), TEST_TARGET_SIZE);

        result = Cy_Flash_ProgramRow((TEST_EMPTY_ADDR + (PageNum * CY_FLASH_SIZEOF_ROW)),(const uint32_t*)write_buff);
        if (result != CY_RSLT_SUCCESS)
        {
            CY_ASSERT(0);
        }
        /* Verify erase state */
        memset(read_buff, 0, sizeof(read_buff));
        memcpy(read_buff, (void *)(TEST_EMPTY_ADDR + (PageNum * CY_FLASH_SIZEOF_ROW)), TEST_TARGET_SIZE);
        for(uint32_t i = 0; i < CY_FLASH_SIZEOF_ROW; i++)
        {
           if(read_buff[i] != write_buff[i])
           {
               CY_ASSERT(0);
           }
        }
    }

    printf("Empty sector program finished! \r\n\n");
    
    /* 4.Data process, the new 512bytes data with readback data, it's could replace, insert, add, and so on. */
    printf("\r__________________________________________________________________________\r\n\n");

    printf("[4] Data process, and program new data into target sector\r\n");
    printf("Target sector erase start.. \r\n\n");
    /* Erase target sector */
    Cy_Flash_EraseSector(TEST_TARGET_ADDR);
    /* Verify */
    for(uint32_t PageNum = 0; PageNum < (CY_CODE_SES_SIZE_IN_BYTE/CY_FLASH_SIZEOF_ROW); PageNum++)
    {
        memset(read_buff, 0, sizeof(read_buff));
        memcpy(read_buff, (void *)(TEST_TARGET_ADDR + (PageNum * CY_FLASH_SIZEOF_ROW)), TEST_TARGET_SIZE);
        /* Verify erase state */
        for(uint32_t i = 0; i < CY_FLASH_SIZEOF_ROW; i++)
        {
           if(read_buff[i] != 0xFF)
           {
               CY_ASSERT(0);
           }
        }
    }
    printf("Target sector erase finished! \r\n\n");

    printf("Update target data and program.. \r\n");
    printf("Modify [0] and [10] data value \r\n");
    /* Programming */
    for(uint32_t PageNum = 0; PageNum < (CY_CODE_SES_SIZE_IN_BYTE/CY_FLASH_SIZEOF_ROW); PageNum++)
    {
        memset(write_buff, 0, sizeof(write_buff));
        memcpy(write_buff, (void *)(TEST_EMPTY_ADDR + (PageNum * CY_FLASH_SIZEOF_ROW)), TEST_TARGET_SIZE);
        write_buff[0] =  0x55;
        write_buff[10] = 0xaa;

        result = Cy_Flash_ProgramRow((TEST_TARGET_ADDR + (PageNum * CY_FLASH_SIZEOF_ROW)),(const uint32_t*)write_buff);
        if (result != CY_RSLT_SUCCESS)
        {
            CY_ASSERT(0);
        }
        /* Verify erase state */
        memset(read_buff, 0, sizeof(read_buff));
        memcpy(read_buff, (void *)(TEST_TARGET_ADDR + (PageNum * CY_FLASH_SIZEOF_ROW)), TEST_TARGET_SIZE);
       for(uint32_t i = 0; i < CY_FLASH_SIZEOF_ROW; i++)
        {
           if(read_buff[i] != write_buff[i])
           {
               CY_ASSERT(0);
           }
        }
    }

    printf("Program finished! \r\n\n");
    printf("\r__________________________________________________________________________\r\n");
    
    for(;;)
    {

    }
}


/* [] END OF FILE */
