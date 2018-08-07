/* ULP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   This file contains assembly code which runs on the ULP.
*/

/* ULP assembly files are passed through C preprocessor first, so include directives
   and C macros may be used in these files 
 */


#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "stack.S"

/* BH1750 */
.set BH1750_ADDR_W,            0x46 
.set BH1750_ADDR_R,            0x47 
.set BH1750_POWER_ON,          0x01
.set BH1750_DATA_RESET,        0x07
.set BH1750_ONETIME_4LX_RES,   0x23
.set BH1750_CONTINUE_4LX_RES,  0x13

/* HTS221 */
.set HTS221_ADDR_W,            0xbe
.set HTS221_ADDR_R,            0xbf

.set HTS221_WHO_AM_I_REG,      0x0f

.set HTS221_CTRL_REG1,         0x20
.set HTS221_CTRL_REG1_VALUE,   0x81  /* active mode; ODR=1Hz; BDU disable*/

.set HTS221_AV_CONF_REG,       0x10
.set HTS221_AV_CONF_REG_VALUE, 0x1b  /* Humidity average:32 samples; Temperature average:16 samples */

.set HTS221_HR_OUT_L_REG,      0x28  /* Humidity data (LSB) */
.set HTS221_HR_OUT_H_REG,      0x29  /* Humidity data (MSB) */

.set HTS221_TEMP_OUT_L_REG,    0x2a  /* Temperature data (LSB) */
.set HTS221_TEMP_OUT_H_REG,    0x2b  /* Temperature data (MSB) */

/* Calibration registers */
.set HTS221_H0_RH_X2,          0x30
.set HTS221_H1_RH_X2,          0x31
.set HTS221_T0_DEGC_X8,        0x32
.set HTS221_T1_DEGC_X8,        0x33
.set HTS221_T0_T1_DEGC_H2,     0x35
.set HTS221_H0_T0_OUT_L,       0x36
.set HTS221_H0_T0_OUT_H,       0x37
.set HTS221_H1_T0_OUT_L,       0x3A
.set HTS221_H1_T0_OUT_H,       0x3B
.set HTS221_T0_OUT_L,          0x3C
.set HTS221_T0_OUT_H,          0x3D
.set HTS221_T1_OUT_L,          0x3E
.set HTS221_T1_OUT_H,          0x3F

.set ZERO, 0x00
    
.bss
    .global debug_value1
debug_value1:
    .long 0

    .global debug_value2
debug_value2:
    .long 0
    
    .global sample_counter
sample_counter:
    .long 0

    .global temp_min_value
temp_min_value:
    .long 0
   
    .global temp_max_value
temp_max_value:
    .long 0

    .global temp_step
temp_step:
    .long 0
    
    .global hum_min_value
hum_min_value:
    .long 0
   
    .global hum_max_value
hum_max_value:
    .long 0

    .global hum_step
hum_step:
    .long 0
    
    .global lum_min_value
lum_min_value:
    .long 0
   
    .global lum_max_value
lum_max_value:
    .long 0
    
    .global lum_step
lum_step:
    .long 0
    
    .global raw_hum_lsb
raw_hum_lsb:
    .long 0
   
    .global raw_hum_msb
raw_hum_msb:
    .long 0
   
    .global raw_temp_lsb
raw_temp_lsb:
    .long 0
   
    .global raw_temp_msb
raw_temp_msb:
    .long 0

    .global raw_lum_value
raw_lum_value:
    .long 0

    .global last_raw_temp_value
last_raw_temp_value:
    .long 0
    
    .global last_raw_hum_value
last_raw_hum_value:
    .long 0
    
    .global last_raw_lum_value
last_raw_lum_value:
    .long 0
   
    .global stack
stack:
    .skip 100
   
    .global stackEnd
stackEnd:
    .long 0

.text
    .global entry
entry:
    move r3,stackEnd
    psr
    jump Task_HTS221

    move r2, 100                  // delay
    psr
    jump waitMs

    psr
    jump Task_BH1750

    move r1, sample_counter    /* Read sample counter */
    ld r0, r1, 0
    add r0, r0, 1              /* Increment */
    st r0, r1, 0               /* Save counter in memory */

    // move r1, raw_temp_msb
    // ld r0, r1, 0
    // lsh r0, r0, 8
    // move r1, raw_temp_lsb
    // ld r2, r1, 0
    // add r0, r0, r2

    // move r2, temp_min_value
    // ld r2, r2, 0
    // sub r2, r0, r2
    // jump clear, ov           /* temperature is less than minimum value */

    // move r2, temp_max_value
    // ld r2, r2, 0
    // sub r2, r2, r0
    // jump clear, ov           /* temperature is greater than maximum value */

    //move r2, last_raw_temp_value
    //ld r2, r2, 0
    //sub r2, r0, r2
    //jump temp_neg_diff_value, ov
    
    //move r1, temp_step
    //ld r1, r1, 0
    //sub r2, r1, r2
    //jump clear, ov
    
    // move r1, raw_hum_msb
    // ld r0, r1, 0
    // lsh r0, r0, 8
    // move r1, raw_hum_lsb
    // ld r2, r1, 0
    // add r0, r0, r2

    // move r2, hum_min_value
    // ld r2, r2, 0
    // sub r2, r2, r0
    // jump clear, ov           /* humidity is less than minimum value */

    // move r2, hum_max_value
    // ld r2, r2, 0
    // sub r2, r0, r2
    // jump clear, ov           /* humidity is greater than maximum value */

    //move r2, last_raw_hum_value
    //ld r2, r2, 0
    //sub r2, r2, r0
    //jump hum_neg_diff_value, ov
    
    //move r1, hum_step
    //ld r1, r1, 0
    //sub r2, r1, r2
    //jump clear, ov

    move r0, raw_lum_value
    ld r0, r0, 0
    
    move r2, lum_min_value
    ld r2, r2, 0
    sub r2, r0, r2
    jump clear, ov           /* luminance is less than minimum value */
    
    move r2, lum_max_value
    ld r2, r2, 0
    sub r2, r2, r0
    jump clear, ov           /* luminance is greater than maximum value */

    //move r2, last_raw_lum_value
    //ld r2, r2, 0
    //sub r2, r0, r2
    //jump lum_neg_diff_value, ov
    
    //move r1, lum_step
    //ld r1, r1, 0
    //sub r2, r1, r2
    //jump clear, ov

    jump exit

    .global temp_neg_diff_value
temp_neg_diff_value:
    move r2, last_raw_temp_value
    ld r2, r2, 0
    sub r2, r2, r0

    move r1, temp_step
    ld r1, r1, 0
    sub r2, r1, r2
    
    jump clear, ov
    jump exit

    .global hum_neg_diff_value
hum_neg_diff_value:
    move r2, last_raw_hum_value
    ld r2, r2, 0
    sub r2, r0, r2

    move r1, hum_step
    ld r1, r1, 0
    sub r2, r1, r2
    
    jump clear, ov
    jump exit

    .global lum_neg_diff_value
lum_neg_diff_value:
    move r2, last_raw_lum_value
    ld r2, r2, 0
    sub r2, r2, r0

    move r1, lum_step
    ld r1, r1, 0
    sub r2, r1, r2
    
    jump clear, ov
    jump exit
    
    .global clear
clear:
    move r1, sample_counter
    ld r0, r1, 0
    .set zero, 0x00
    move r0, zero
    st r0, r1, 0
    jump wake_up
   
    /* value within range, end the program */
    .global exit
exit:
    halt

    .global wake_up
wake_up:
    /* Check if the system can be woken up */
    READ_RTC_REG(RTC_CNTL_DIAG0_REG, 19, 1)
    and r0, r0, 1
    jump exit, eq
    /* Wake up the SoC, end program */
    wake
    WRITE_RTC_FIELD(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0)
    halt

    /* read one byte from hts221, aquired value stored in r0 */
    .global Read_onebyte_HTS221
Read_onebyte_HTS221:
    /* write one hts221 register address */
    psr 
    jump i2c_start_cond           // i2c Start

    ld r2, r3, 16                 // Address+Write
    psr
    jump i2c_write_byte
    //jumpr popfail,1,ge

    ld r2, r3, 8                  // Register
    psr
    jump i2c_write_byte
    //jumpr popfail, 1, ge 

    psr
    jump i2c_stop_cond            // i2c Stop

    /* read one byte from one hts221 register */
    psr 
    jump i2c_start_cond          // i2c Start

    ld r2, r3, 12                 // Address+Read
    psr
    jump i2c_write_byte
    //jumpr popfail, 1, ge

    move r2,0
    psr
    jump i2c_read_byte
    push r0
    psr
    jump i2c_send_nack
    psr
    jump i2c_stop_cond

    pop r0
    ret

    .global Cmd_Write_HTS221
Cmd_Write_HTS221:
    psr 
    jump i2c_start_cond           // i2c Start

    ld r2, r3, 16                 // Address+Write
    psr
    jump i2c_write_byte
    //jumpr popfail,1,ge

    ld r2, r3, 12                  // Register
    psr
    jump i2c_write_byte
    //jumpr popfail, 1, ge

    ld r2, r3, 8                  // Value
    psr
    jump i2c_write_byte
    //jumpr popfail, 1, ge   

    psr
    jump i2c_stop_cond            // i2c Stop
    ret

    .global Start_HTS221
Start_HTS221:
    move r1, HTS221_ADDR_W
    push r1

    /* set active mode, odr, and bdu */
    move r1, HTS221_CTRL_REG1
    push r1
    move r1, HTS221_CTRL_REG1_VALUE
    push r1

    psr 
    jump Cmd_Write_HTS221
    pop r1

    /* set Humidity average and Temperature average */
    pop r1
    move r1, HTS221_AV_CONF_REG
    push r1
    move r1, HTS221_AV_CONF_REG_VALUE
    push r1

    psr 
    jump Cmd_Write_HTS221
    pop r1
    pop r1
    pop r1

    move r2, 100                  // delay
    psr
    jump waitMs

    move r1, HTS221_ADDR_W
    push r1

    move r1, HTS221_ADDR_R
    push r1

    /* read humidity */
    move r1, HTS221_HR_OUT_L_REG
    push r1
    psr 
    jump Read_onebyte_HTS221
    move r2, raw_hum_lsb
    st r0, r2, 0

    pop r1
    move r1, HTS221_HR_OUT_H_REG
    push r1
    psr 
    jump Read_onebyte_HTS221
    move r2, raw_hum_msb
    st r0, r2, 0

    /* read temperature */
    pop r1
    move r1, HTS221_TEMP_OUT_L_REG
    push r1
    psr
    jump Read_onebyte_HTS221
    move r2, raw_temp_lsb
    st r0, r2, 0

    pop r1
    move r1, HTS221_TEMP_OUT_H_REG
    push r1
    psr 
    jump Read_onebyte_HTS221
    move r2, raw_temp_msb
    st r0, r2, 0

    pop r1
    pop r1
    pop r1

    ret

    .global Task_HTS221
Task_HTS221:
    //WRITE_RTC_REG(RTC_GPIO_ENABLE_W1TS_REG, RTC_GPIO_ENABLE_W1TS_S + 17, 1, 1)  // power on sensor

    move r2, 100                  // delay
    psr
    jump waitMs

    psr
    jump Start_HTS221
    ret

    .global Read_BH1750
Read_BH1750:
	move r1, BH1750_ADDR_R
	push r1
	psr 
	jump i2c_start_cond          // i2c Start
	ld r2, r3, 4                 // Address+Read
	psr
	jump i2c_write_byte
	//jumpr popfail, 1, ge
	pop r1
	move r2,0
	psr
	jump i2c_read_byte
    push r0

	psr
	jump i2c_send_ack
	
	move r2,1 // last byte
	psr
	jump i2c_read_byte
	push r0

	psr
	jump i2c_send_nack
	
	psr
	jump i2c_stop_cond
	pop r0 // Low-byte
	pop r2 // Hight-byte
	lsh r2,r2,8
	or r2,r2,r0
	move r0,r2
	move r1, raw_lum_value
	st r0, r1, 0
	move r2,0 // OK
	ret

    .global Cmd_Write_BH1750
Cmd_Write_BH1750:
	psr 
	jump i2c_start_cond           // i2c Start
	ld r2, r3, 12                 // Address+Write
	psr
	jump i2c_write_byte
	//jumpr popfail,1,ge
	ld r2, r3, 8                  // Command
	psr
	jump i2c_write_byte
	//jumpr popfail, 1, ge
	psr
	jump i2c_stop_cond            // i2c Stop
	ret

    .global Start_BH1750
Start_BH1750:
	move r1, BH1750_ADDR_W
	push r1
	move r1, BH1750_POWER_ON
	push r1
	psr 
	jump Cmd_Write_BH1750         // power on
	pop r1
	move r1, BH1750_CONTINUE_4LX_RES
	push r1
	psr 
	jump Cmd_Write_BH1750         // continue low resolution
	pop r1
	pop r1
	ret

    .global Task_BH1750
Task_BH1750:
	psr
	jump Start_BH1750
	move r2, 100                  // delay
	psr
	jump waitMs
	psr
	jump Read_BH1750
	ret

popfail:
    pop r1                        // pop caller return address
    move r2,1
    ret
   
    /* Wait for r2 milliseconds */
    .global waitMs
waitMs:
    wait 8000
    sub r2,r2,1
    jump doneWaitMs,eq
    jump waitMs
doneWaitMs:
    ret
