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

#define RX_MESSAGE_BUFFER_NUM (10)
#define TX_MESSAGE_BUFFER_NUM (9)

static uintptr_t can_irq_contexts[CAN_NUM] = {0};
static can_irq_handler irq_handler;

static uint32_t can_disable(can_t *obj) {
    return 0;
}

static inline void can_enable(can_t *obj) {
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
		base->CTRL1 = ctrl1;
		base->MCR = mcr;
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

    // Map pins
    pin_function(pinmap->rd_pin, pinmap->rd_function);
    pin_mode(pinmap->rd_pin, PullNone);
    pin_function(pinmap->td_pin, pinmap->td_function);
    pin_mode(pinmap->td_pin, PullNone);
	obj->index = pinmap->peripheral;
    MBED_ASSERT((int)obj->index != NC);
    
    printf("Init can %u %d\n", obj->index, hz);
  
    FLEXCAN_GetDefaultConfig(&obj->flexcanConfig);

    obj->flexcanConfig.baudRate = hz;
#if 0
    memset(&obj->timing_config, 0, sizeof(flexcan_timing_config_t));
    
    if (FLEXCAN_CalculateImprovedTimingValues(obj->flexcanConfig.baudRate, can_get_clock(), &obj->timing_config))
    {
        /* Update the improved timing configuration*/
        memcpy(&(obj->flexcanConfig.timingConfig), &obj->timing_config, sizeof(flexcan_timing_config_t));
    }
    else
    {
        printf("No found Improved Timing Configuration. Just used default configuration\n\n");
    }
#endif
    FLEXCAN_Init(can_addrs[obj->index], &obj->flexcanConfig, can_get_clock());
    
	FLEXCAN_TransferCreateHandle(can_addrs[obj->index], &obj->flexcanHandle, flexcan_callback, NULL);
	   
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

	static flexcan_frame_t frame;
	flexcan_mb_transfer_t txXfer;
	uint8_t i;
	
	status_t ret;
	
	printf("return status %d\n", returnstatus);

	
	if(msg.format == CANStandard) {
		frame.id     = FLEXCAN_ID_STD(msg.id);
		frame.format = (uint8_t)kFLEXCAN_FrameFormatStandard;
	}
	else {
		frame.id     = FLEXCAN_ID_EXT(msg.id);
		frame.format = (uint8_t)kFLEXCAN_FrameFormatExtend;	
	}
	
	if(msg.type == CANData) {
		frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
		frame.dataWord0 = 0;
		frame.dataWord1 = 0;
		for(i=0; i<msg.len; i++){
			switch(i) {
				case 0:
					frame.dataByte0 = msg.data[0];
					break;
				case 1:
					frame.dataByte1 = msg.data[1];
					break;
				case 2:
					frame.dataByte2 = msg.data[2];
					break;
				case 3:
					frame.dataByte3 = msg.data[3];
					break;
				case 4:
					frame.dataByte4 = msg.data[4];
					break;
				case 5:
					frame.dataByte5 = msg.data[5];
					break;
				case 6:
					frame.dataByte6 = msg.data[6];
					break;
				case 7:
					frame.dataByte7 = msg.data[7];
					break;
				default:
					break;
			}
		}
	}
	else{
		frame.type   = (uint8_t)kFLEXCAN_FrameTypeRemote;
	}
	
	frame.length = (uint8_t)msg.len;
		
	txXfer.mbIdx = (uint8_t)(TX_MESSAGE_BUFFER_NUM + obj->index);
	txXfer.frame = &frame;
	
	FLEXCAN_SetTxMbConfig(can_addrs[obj->index], txXfer.mbIdx, true);
	
	ret = FLEXCAN_TransferSendNonBlocking(can_addrs[obj->index], &obj->flexcanHandle, &txXfer);

	printf("Can write ret %d\n", ret);

	if(ret == kStatus_Success)
		return 1;
		
	return 0;
}

int can_read(can_t *obj, CAN_Message *msg, int handle) {


    return 0;
}

void can_reset(can_t *obj) {
	CAN_Type *base = can_addrs[obj->index];
	uint32_t u32TimeoutCount = 0U;

    /* Assert Soft Reset Signal. */
    base->MCR |= CAN_MCR_SOFTRST_MASK;
    /* Wait until FlexCAN reset completes. */
	u32TimeoutCount = (uint32_t)FLEXCAN_WAIT_TIMEOUT * 20U;
	while ((CAN_MCR_SOFTRST_MASK == (base->MCR & CAN_MCR_SOFTRST_MASK)) && (u32TimeoutCount > 0U))
	{
		u32TimeoutCount--;
	}
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

}

const PinMap *can_rd_pinmap()
{
    return PinMap_CAN_TD;
}

const PinMap *can_td_pinmap()
{
    return PinMap_CAN_RD;
}
