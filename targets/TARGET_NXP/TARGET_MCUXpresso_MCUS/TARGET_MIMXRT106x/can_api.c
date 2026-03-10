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


/* Array of CAN peripheral base address. */
static CAN_Type *const can_addrs[] = CAN_BASE_PTRS;

/* Acceptance filter mode in AFMR register */
#define ACCF_OFF                0x01
#define ACCF_BYPASS             0x02
#define ACCF_ON                 0x00
#define ACCF_FULLCAN            0x04

/* There are several bit timing calculators on the internet.
http://www.port.de/engl/canprod/sv_req_form.html
http://www.kvaser.com/can/index.htm
*/

// Type definition to hold a CAN message
struct CANMsg {
    unsigned int  reserved1 : 16;
    unsigned int  dlc       :  4; // Bits 16..19: DLC - Data Length Counter
    unsigned int  reserved0 : 10;
    unsigned int  rtr       :  1; // Bit 30: Set if this is a RTR message
    unsigned int  type      :  1; // Bit 31: Set if this is a 29-bit ID message
    unsigned int  id;             // CAN Message ID (11-bit or 29-bit)
    unsigned char data[8];        // CAN Message Data Bytes 0-7
};
typedef struct CANMsg CANMsg;

static uintptr_t can_irq_contexts[CAN_NUM] = {0};
static can_irq_handler irq_handler;

static uint32_t can_disable(can_t *obj) {
    return 0;
}

static inline void can_enable(can_t *obj) {
}

int can_mode(can_t *obj, CanMode mode) {
    int success = 0;

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
    
    can_reset(obj);

    flexcan_config_t flexcanConfig;
    flexcan_rx_mb_config_t mbConfig;
    flexcan_timing_config_t timing_config;

    uint8_t node_type;

    FLEXCAN_GetDefaultConfig(&flexcanConfig);

    flexcanConfig.baudRate = hz;

    memset(&timing_config, 0, sizeof(flexcan_timing_config_t));

    if (FLEXCAN_CalculateImprovedTimingValues(flexcanConfig.baudRate, can_get_clock(), &timing_config))
    {
        /* Update the improved timing configuration*/
        memcpy(&(flexcanConfig.timingConfig), &timing_config, sizeof(flexcan_timing_config_t));
    }
    else
    {
        printf("No found Improved Timing Configuration. Just used default configuration\n\n");
    }


    FLEXCAN_Init(can_addrs[obj->index], &flexcanConfig, can_get_clock());
    
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


    return 0;
}

int can_read(can_t *obj, CAN_Message *msg, int handle) {


    return 0;
}

void can_reset(can_t *obj) {
	return;
}

unsigned char can_rderror(can_t *obj) {
  return 0;
}

unsigned char can_tderror(can_t *obj) {
  return 0;
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
