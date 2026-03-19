/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed_assert.h"
#include "can_api.h"

#include "cmsis.h"
#include "pinmap.h"
#include "fsl_flexcan.h"
#include "PeripheralPins.h"
#include "clock_config.h"

#include <math.h>
#include <string.h>

#include <stdio.h>

#define CAN_NUM    3

extern uint32_t can_get_clock(void);

status_t returnstatus;

/* Array of CAN peripheral base address. */
static CAN_Type *const can_addrs[] = CAN_BASE_PTRS;

#define RX_MESSAGE_BUFFER_NUM (1)
#define TX_MESSAGE_BUFFER_NUM (15)

static uintptr_t can_irq_contexts[CAN_NUM] = {0};
static can_irq_handler irq_handler;

static uint32_t can_disable(can_t *obj) {
    return 0;
}

static inline void can_enable(can_t *obj) {
}

static void CAN_EnterFreezeMode(CAN_Type *base)
{
    /* Set Freeze, Halt bits. */
    base->MCR |= CAN_MCR_FRZ_MASK;
    base->MCR |= CAN_MCR_HALT_MASK;
    while (0U == (base->MCR & CAN_MCR_FRZACK_MASK))
    {
    }
}

static void CAN_ExitFreezeMode(CAN_Type *base)
{
    /* Clear Freeze, Halt bits. */
    base->MCR &= ~CAN_MCR_HALT_MASK;
    base->MCR &= ~CAN_MCR_FRZ_MASK;

    /* Wait until the FlexCAN Module exit freeze mode. */
    while (0U != (base->MCR & CAN_MCR_FRZACK_MASK))
    {
    }
}

//static FLEXCAN_CALLBACK(flexcan_callback)
static void flexcan_callback(CAN_Type * base, flexcan_handle_t * handle, status_t status, uint32_t result, void *userData)
{
	returnstatus = status;
	return;
}

int can_mode(can_t *obj, CanMode mode) {
    int success = 0;
	CAN_Type *base = can_addrs[obj->index];
	uint32_t ctrl1;
	uint32_t mcr;
	
    ctrl1 = base->CTRL1;
	mcr = base->MCR;
	
    switch (mode) {
        case MODE_NORMAL:
            // Clear all special modes
			// Disable loopback and self-reception
			ctrl1 &= ~(CAN_CTRL1_LPB_MASK);
			mcr |= CAN_MCR_SRXDIS_MASK;
			// Disable listen-only mode 
			ctrl1 &= ~(CAN_CTRL1_LOM_MASK);
            success = 1;
            break;
        case MODE_SILENT:
            // Set listen-only mode
			ctrl1 |= CAN_CTRL1_LOM_MASK;
			// Disable loopback and self-reception
			ctrl1 &= ~(CAN_CTRL1_LPB_MASK);
			mcr |= CAN_MCR_SRXDIS_MASK;			
            success = 1;
            break;
        case MODE_TEST_LOCAL:
            // Set self-test mode and clear listen-only mode
			ctrl1 |= CAN_CTRL1_LPB_MASK;
			mcr &= ~(CAN_MCR_SRXDIS_MASK);
			// Disable listen-only mode
			ctrl1 &= ~(CAN_CTRL1_LOM_MASK);
            success = 1;
            break;
        case MODE_RESET:
        case MODE_TEST_SILENT:
        case MODE_TEST_GLOBAL:
        default:
            success = 0;
            break;
    }

	if(success == 1) {
		CAN_EnterFreezeMode(base);
		base->CTRL1 = ctrl1;
		base->MCR = mcr;
		CAN_ExitFreezeMode(base);
	}

    return success;
}

int can_filter(can_t *obj, uint32_t id, uint32_t mask, CANFormat format, int32_t handle) {
    return 0; // not implemented
}

static inline void can_irq(uint32_t icr, uint32_t index) {

}

// Have to check that the CAN block is active before reading the Interrupt
// Control Register, or the mbed hangs
void can_irq_n() {
}

// Register CAN object's irq handler
void can_irq_init(can_t *obj, can_irq_handler handler, uintptr_t context) {
}

// Unregister CAN object's irq handler
void can_irq_free(can_t *obj) {
}

// Clear or set a irq
void can_irq_set(can_t *obj, CanIrqType type, uint32_t enable) {

}

static unsigned int can_speed(unsigned int sclk, unsigned int pclk, unsigned int cclk, unsigned char psjw) {

  return 0;
}

void can_init_freq_direct(can_t *obj, const can_pinmap_t *pinmap, int hz) {
	CAN_Type *base;
	flexcan_rx_fifo_config_t rxFifoConfig;
    flexcan_rx_mb_config_t mbConfig;

    // Map pins
    pin_function(pinmap->rd_pin, pinmap->rd_function);
    pin_mode(pinmap->rd_pin, PullNone);
    pin_function(pinmap->td_pin, pinmap->td_function);
    pin_mode(pinmap->td_pin, PullNone);
	obj->index = pinmap->peripheral;
    MBED_ASSERT((int)obj->index != NC);
    
    printf("Init can %u %d\n", obj->index, hz);
    
    base = can_addrs[obj->index];
  
    FLEXCAN_GetDefaultConfig(&(obj->flexcanConfig));

    obj->flexcanConfig.baudRate = hz;
    // Set individual mask mode
	obj->flexcanConfig.enableIndividMask = true;

#if 0 // I think that FLEXCAN_CalculateImprovedTimingValues is broken in this SDK
    memset(&(obj->timingConfig), 0, sizeof(flexcan_timing_config_t));
    
    if(FLEXCAN_CalculateImprovedTimingValues(obj->flexcanConfig.baudRate, can_get_clock(), &(obj->timingConfig)))
    {
        /* Update the improved timing configuration*/
		printf("Timing %d %d %d %d %d\n", obj->flexcanConfig.timingConfig.preDivider, obj->flexcanConfig.timingConfig.rJumpwidth, obj->flexcanConfig.timingConfig.phaseSeg1, obj->flexcanConfig.timingConfig.phaseSeg2, obj->flexcanConfig.timingConfig.propSeg);
            
        memcpy(&(obj->flexcanConfig.timingConfig), &(obj->timingConfig), sizeof(flexcan_timing_config_t));

		printf("Timing %d %d %d %d %d\n", obj->flexcanConfig.timingConfig.preDivider, obj->flexcanConfig.timingConfig.rJumpwidth, obj->flexcanConfig.timingConfig.phaseSeg1, obj->flexcanConfig.timingConfig.phaseSeg2, obj->flexcanConfig.timingConfig.propSeg);

    }
    else
    {
        printf("No found Improved Timing Configuration. Just used default configuration\n\n");
    }
#endif

    FLEXCAN_Init(base, &(obj->flexcanConfig), can_get_clock());
    
	FLEXCAN_TransferCreateHandle(base, &(obj->flexcanHandle), flexcan_callback, NULL);

	can_reset(obj);

	CAN_EnterFreezeMode(base);

	// Enable auto-recovery from bus-off
	base->CTRL1 &= ~(CAN_CTRL1_BOFFREC_MASK);
	// Disable self reception
	base->MCR |= CAN_MCR_SRXDIS_MASK;

	CAN_ExitFreezeMode(base);

	// Setup default mailbox
    mbConfig.format = kFLEXCAN_FrameFormatStandard;
    mbConfig.type   = kFLEXCAN_FrameTypeData;
    mbConfig.id     = FLEXCAN_ID_STD(1);
    FLEXCAN_SetRxMbConfig(base, RX_MESSAGE_BUFFER_NUM, &mbConfig, true);
    // Set the mask to 0 to allow all messages
	FLEXCAN_SetRxIndividualMask(base, RX_MESSAGE_BUFFER_NUM, 0);

    printf("Init can init done\n");

}

void can_init_freq(can_t *obj, PinName rd, PinName td, int hz) {
    can_pinmap_t pinmap;
    pinmap.rd_pin = rd;
    pinmap.td_pin = td;

    // Determine peripheral associated with these pins
    CANName can_rd = (CANName)pinmap_peripheral(rd, PinMap_CAN_RD);
    CANName can_td = (CANName)pinmap_peripheral(td, PinMap_CAN_TD);
    pinmap.peripheral = pinmap_merge(can_rd, can_td);
    MBED_ASSERT((int)pinmap.peripheral != NC);

    // Get pin functions
    pinmap.rd_function = pinmap_find_function(rd, PinMap_CAN_RD);
    pinmap.td_function = pinmap_find_function(td, PinMap_CAN_TD);

    can_init_freq_direct(obj, &pinmap, hz);
}

void can_init_direct(can_t *obj, const can_pinmap_t *pinmap) {
    can_init_freq_direct(obj, pinmap, 100000);
}

void can_init(can_t *obj, PinName rd, PinName td) {
    can_init_freq(obj, rd, td, 100000);
}

void can_free(can_t *obj) {
	FLEXCAN_Deinit(can_addrs[obj->index]);
}

int can_frequency(can_t *obj, int f) {
	return 0;
}

int can_write(can_t *obj, CAN_Message msg) {
	CAN_Type *base = can_addrs[obj->index];
	flexcan_mb_transfer_t txXfer;
	uint8_t i;
	
	status_t ret;
	
	if((base->ESR1 & 0x30) >= 0x10)
	{
		printf("Bus Off\n");
		return 0;
	}

	
	if(msg.format == CANStandard) {
		obj->txFrame.id     = FLEXCAN_ID_STD(msg.id);
		obj->txFrame.format = (uint8_t)kFLEXCAN_FrameFormatStandard;
	}
	else {
		obj->txFrame.id     = FLEXCAN_ID_EXT(msg.id);
		obj->txFrame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;	
	}
	
	if(msg.type == CANData) {
		obj->txFrame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
		obj->txFrame.dataWord0 = 0;
		obj->txFrame.dataWord1 = 0;
		// I can't think of a nice way to do this with variable data lengths!
		for(i=0; i<msg.len; i++){
			switch(i) {
				case 0:
					obj->txFrame.dataByte0 = msg.data[0];
					break;
				case 1:
					obj->txFrame.dataByte1 = msg.data[1];
					break;
				case 2:
					obj->txFrame.dataByte2 = msg.data[2];
					break;
				case 3:
					obj->txFrame.dataByte3 = msg.data[3];
					break;
				case 4:
					obj->txFrame.dataByte4 = msg.data[4];
					break;
				case 5:
					obj->txFrame.dataByte5 = msg.data[5];
					break;
				case 6:
					obj->txFrame.dataByte6 = msg.data[6];
					break;
				case 7:
					obj->txFrame.dataByte7 = msg.data[7];
					break;
				default:
					break;
			}
		}
	}
	else{
		obj->txFrame.type   = (uint8_t)kFLEXCAN_FrameTypeRemote;
	}
	
	obj->txFrame.length = (uint8_t)msg.len;
		
	txXfer.mbIdx = (uint8_t)(TX_MESSAGE_BUFFER_NUM);
	txXfer.frame = &obj->txFrame;
	
	FLEXCAN_SetTxMbConfig(base, txXfer.mbIdx, true);
	
	ret = FLEXCAN_TransferSendNonBlocking(base, &obj->flexcanHandle, &txXfer);

	if(ret == kStatus_Success)
		return 1;
		
	return 0;
}

int can_read(can_t *obj, CAN_Message *msg, int handle) {
	CAN_Type *base = can_addrs[obj->index];
	status_t ret;
	flexcan_frame_t rxFrame;
	uint32_t u32flag = 1;
		   
	if(FLEXCAN_GetMbStatusFlags(base, u32flag << (handle + 1)) != 0)
	{
		ret = FLEXCAN_ReadRxMb(base, (handle + 1), &rxFrame);
	
		printf("Can read ret %d\n", ret);
		
		FLEXCAN_ClearMbStatusFlags(base, u32flag << (handle + 1));
	
		if(ret == kStatus_Success)
		{

			if(rxFrame.format == kFLEXCAN_FrameFormatStandard) {
				msg->id = rxFrame.id >> CAN_ID_STD_SHIFT;
				msg->format = CANStandard;
			}
			else {
				msg->id = rxFrame.id >> CAN_ID_EXT_SHIFT;
				msg->format = CANExtended;
			}
			
			msg->len = rxFrame.length;

			if(rxFrame.type == kFLEXCAN_FrameTypeData) {
				msg->type = CANData;
						
				msg->data[0] = rxFrame.dataByte0;
				msg->data[1] = rxFrame.dataByte1;
				msg->data[2] = rxFrame.dataByte2;
				msg->data[3] = rxFrame.dataByte3;
				msg->data[4] = rxFrame.dataByte4;
				msg->data[5] = rxFrame.dataByte5;
				msg->data[6] = rxFrame.dataByte6;
				msg->data[7] = rxFrame.dataByte7;
			}
			else
				msg->type = CANRemote;

			return 1;
		}
	}

    return 0;
}

void can_reset(can_t *obj) {
	CAN_Type *base = can_addrs[obj->index];
	
	if((base->ESR1 & 0x30) == 0x00)
		printf("Error Active\n");
	if((base->ESR1 & 0x30) == 0x10)
		printf("Error Passive\n");
	if((base->ESR1 & 0x30) >= 0x20)
		printf("Bus Off\n");

	
	CAN_EnterFreezeMode(base);
	base->ECR &= ~(CAN_ECR_TXERRCNT_MASK | CAN_ECR_RXERRCNT_MASK);
	CAN_ExitFreezeMode(base);
}

unsigned char can_rderror(can_t *obj) {
	unsigned char errCount;
	
	FLEXCAN_GetBusErrCount(can_addrs[obj->index], NULL, &errCount);
	
	return errCount;
}

unsigned char can_tderror(can_t *obj) {
	unsigned char errCount;
	
	FLEXCAN_GetBusErrCount(can_addrs[obj->index], &errCount, NULL);
	
	return errCount;
}

void can_monitor(can_t *obj, int silent) {
	if(silent == 1)
		can_mode(obj, MODE_SILENT);
	else
		can_mode(obj, MODE_NORMAL);
}

const PinMap *can_rd_pinmap()
{
    return PinMap_CAN_TD;
}

const PinMap *can_td_pinmap()
{
    return PinMap_CAN_RD;
}
