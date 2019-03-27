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

.bss
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
.global waitMs

    .global entry
entry:
    move r3,stackEnd
    
    move r2, 10                  // delay 10 ms
    psr
    jump waitMs
    
    psr
    jump Task_HTS221

    move r2, 50                  // delay 10 ms
    psr
    jump waitMs

    psr
    jump Task_BH1750

    move r1, sample_counter    /* Read sample counter */
    ld r0, r1, 0
    add r0, r0, 1              /* Increment */
    st r0, r1, 0               /* Save counter in memory */

    //move r1, raw_temp_msb
    //ld r0, r1, 0
    //lsh r0, r0, 8
    //move r1, raw_temp_lsb
    //ld r2, r1, 0
    //add r0, r0, r2

    //move r2, temp_min_value
    //ld r2, r2, 0
    //sub r2, r0, r2
    //jump clear, ov           /* temperature is less than minimum value */

    //move r2, temp_max_value
    //ld r2, r2, 0
    //sub r2, r2, r0
    //jump clear, ov           /* temperature is greater than maximum value */

    //move r2, last_raw_temp_value
    //ld r2, r2, 0
    //sub r2, r0, r2
    //jump temp_neg_diff_value, ov
    
    //move r1, temp_step
    //ld r1, r1, 0
    //sub r2, r1, r2
    //jump clear, ov
    
    //move r1, raw_hum_msb
    //ld r0, r1, 0
    //lsh r0, r0, 8
    //move r1, raw_hum_lsb
    //ld r2, r1, 0
    //add r0, r0, r2

    //move r2, hum_min_value
    //ld r2, r2, 0
    //sub r2, r2, r0
    //jump clear, ov             /* humidity is less than minimum value */

    //move r2, hum_max_value
    //ld r2, r2, 0
    //sub r2, r0, r2
    //jump clear, ov           /* humidity is greater than maximum value */

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
    jump clear, ov              /* luminance is less than minimum value */
    
    move r2, lum_max_value
    ld r2, r2, 0
    sub r2, r2, r0
    jump clear, ov              /* luminance is greater than maximum value */

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
   
    /* Wait for r2 milliseconds */
    .global waitMs
waitMs:
    wait 8000
    sub r2,r2,1
    jump doneWaitMs,eq
    jump waitMs
doneWaitMs:
    ret
