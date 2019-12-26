// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef __DEBUG_RECV_H__
#define __DEBUG_RECV_H__

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief   Print log record
 *
 * @param   addr  Destination address
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
void log_record_print(uint8_t *addr);

/**
 * @brief   Create esp-now receive task to listen esp-now data from other device
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t debug_recv_init();

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __DEBUG_RECV_H__ */
