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

#ifndef __ALIYUN_SUBDEVICE_H__
#define __ALIYUN_SUBDEVICE_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief  device get login status. OK means the root node is successfully connected to the
 * external network and the device is added to the root node
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_get_login_status(void);

/**
 * @brief device get online status
 *
 * @return
 *  - true: device is online
 *  - false: device is offline
 */
bool aliyun_subdevice_is_online(void);

#ifdef CONFIG_ALIYUN_PLATFORM_MDF
/**
 * @brief Set subdevice that current mesh network root is loss.
 *
 * @attention This function must called in MDF_EVENT_MWIFI_NETWORK_STATE event and pass is_rootless param to it.
 *
 * @param Rootless mesh network is rootless or not.
 */
void aliyun_subdevice_set_rootless(bool rootless);

/**
 * @brief Update toDS state for subdevice
 *
 * @attention This function must called in MDF_EVENT_TODS_STATE event and pass toDS_state param to it.
 *
 * @param state Current toDS state.
 *
 */
void aliyun_subdevice_update_tods(mesh_event_toDS_state_t state);

/**
 * @brief Tell subdevcie that current parent is disconnected.
 *
 * @attention This function must called by node in MDF_EVENT_MWIFI_PARENT_DISCONNECTED
 *
 */
void aliyun_subdevice_parent_disconnected();

/**
 * @brief Tell subdevice that current param is connected.
 *
 * @attention This function must called by node in MDF_EVENT_MWIFI_PARENT_CONNECTED
 */
void aliyun_subdevice_parent_connected();
#endif

/**
 * @brief  device add itself to the gateway
 *
 * @param  meta       Pointer to device info meta.
 * @param  timeout_ms Timeout for waiting for a reply.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_add_to_gateway(const aliyun_device_meta_t *meta);

/**
 * @brief  device delete itself from the gateway
 *
 * @param  meta       Pointer to device info meta.
 * @param  timeout_ms Timeout for waiting for a reply.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_delete_from_gateway(const aliyun_device_meta_t *meta);

/**
 * @brief  device send enroll token to cloud
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_match_enrollee(void);

/**
 * @brief gateway device send reset message to subdevice.
 *
 * @note only the gateway call is valid
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_reset(void);

/**
 * @brief  device send get cipher request to cloud
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_get_cipher(void);

/**
 * @brief  The device post properties to cloud
 *
 * @param  params pointer to message.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_post_property(const char *params);

/**
 * @brief  The device replies to the cloud's setup properties message
 *
 * @param  id       Message ID
 * @param  code     Return the result, 200 represents success.
 * @param  data     The result of the return when the request is successful.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_reply_property_set(uint32_t id, int code, const char *data);

/**
 * @brief  device post event to cloud
 *
 * @note You should make sure that the time is accurate before call this funcion, other wise
 * cloud may refuse event post
 *
 * @param  identifier  event identifier.
 * @param  value       Specific event information.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_post_event(const char *identifier, const char *value);

/**
 * @brief  device replies to cloud's invoke service message
 *
 * @param  id       Message ID
 * @param  code     Return the result, 200 represents success.
 * @param  data     The result of the return when the request is successful.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_reply_service_invoke(uint32_t id, int code, const char *identifier, const char *data);

/**
 * @brief  device post property in batches
 *
 * @param  params   Pointer to a sends json message.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_pack_post_property(const char *params);

/**
 * @brief  Device update device's information(tag)
 *
 * @param  params   Pointer to a sends json message.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_update_deviceinfo(const char *params);

/**
 * @brief  Device delete device's information(tag)
 *
 * @param  params   Pointer to a sends json message.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_delete_deviceinfo(const char *params);

/**
 * @brief  device send get configuration reqeust to cloud
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_get_config(void);

/**
 * @brief  device replies to cloud's push configuration message
 *
 * @param  id       Message ID
 * @param  code     Return the result, 200 represents success.
 * @param  data     The result of the return when the request is successful.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_reply_config_push(int id, int code, const char *data);

/**
 * @brief  device inform device's firmware version to cloud
 *
 * @param  version  Firmware version of the device.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_inform_version(const char *version);

/**
 * @brief  device report ota progress to cloud
 *
 * @param  step  Firmware upgrade progress information.
 * @param  desc  Description of the current step.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_post_ota_progress(int step, const char *desc);

/**
 * @brief  device request ota information from cloud
 *
 * @param  version  Firmware version of the device.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_request_ota_device(const char *version);

/**
 * @brief device Send NTP request message to cloud
 *
 * @note When device receive response ntp message, it will calibrate time automatically.
 * The callback will be invoked if it was set by aliyun_subdevice_set_callback()
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_request_ntp(void);

/**
 * @brief  device request TSL template from cloud
 *
 * @note The size of TSL template is huge(more than 2k bytes)
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_get_dslttemplate(void);

/**
 * @brief  device get desired property
 *
 * @param  params   Pointer to a sends json message.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_get_desired_property(const char *params);

/**
 * @brief  device delete desired property
 *
 * @param  params  Pointer to a sends array message.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_delete_desired_property(const char *params);

/**
 * @brief  device post log to cloud
 *
 * @param  params   Pointer to a sends char message.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_post_log(const char *params);

/**
 * @brief  aliyun subdevice coap bind notify
 *
 * @param  cyclic_num   Number of rounds sent.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_coap_bind_notify(uint32_t cyclic_num);

/**
 * @brief  aliyun subdevice coap connectapp notify
 *
 * @param  cyclic_num   Number of rounds sent.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_coap_connectapp_notify(uint32_t cyclic_num);

/**
 * @brief  init subdevice
 *
 * @param  cyclic_num   Number of rounds sent.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_init(void);

/**
 * @brief  deinit subdevice
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_deinit(void);

/**
 * @brief  device set callback
 *
 * @param[in]  type maliyun linkkit subscribe callback function type.
 * @param[in]  *func Pointer to the callback function.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_subdevice_set_callback(aliyun_msg_type_t type, const void *func);

typedef mdf_err_t (*aliyun_subdevice_model_up_raw_reply_cb_t)(uint32_t msg_id, int code, const char *data);

typedef mdf_err_t (*aliyun_subdevice_model_down_raw_cb_t)(const uint8_t *params, size_t params_len);

typedef mdf_err_t (*aliyun_subdevice_post_property_reply_cb_t)(uint32_t msg_id, int code, const char *data);

typedef mdf_err_t (*aliyun_subdevice_property_set_cb_t)(uint32_t msg_id, const char *params, size_t params_len);

typedef mdf_err_t (*aliyun_subdevice_post_event_reply_cb_t)(uint32_t msg_id, int code, const char *data);

typedef mdf_err_t (*aliyun_subdevice_service_invoke_cb_t)(uint32_t msg_id, const char *identifier, const char *params, size_t params_len);

typedef mdf_err_t (*aliyun_subdevice_pack_post_property_reply_cb_t)(uint32_t msg_id, int code, const char *data);

typedef mdf_err_t (*aliyun_subdevice_update_deviceinfo_reply_cb_t)(uint32_t msg_id, int code, const char *data);

typedef mdf_err_t (*aliyun_subdevice_delete_deviceinfo_reply_cb_t)(uint32_t msg_id, int code, const char *data);

typedef mdf_err_t (*aliyun_subdevice_get_config_reply_cb_t)(uint32_t msg_id, int code, const char *data);

typedef mdf_err_t (*aliyun_subdevice_config_push_cb_t)(uint32_t msg_id, const char *data);

typedef mdf_err_t (*aliyun_subdevice_ruquest_ntp_cb_t)(uint64_t utc_ms);

typedef mdf_err_t (*aliyun_subdevice_get_dsltemplate_reply_cb_t)(uint32_t msg_id, int code, const char *data);

typedef mdf_err_t (*aliyun_subdevice_get_desired_property_reply_cb_t)(uint32_t msg_id, int code, const char *data);

typedef mdf_err_t (*aliyun_subdevice_delete_desired_property_reply_cb_t)(uint32_t msg_id, int code, const char *data);

typedef mdf_err_t (*aliyun_subdevice_add_reply_cb_t)(aliyun_device_reply_t *reply);

typedef mdf_err_t (*aliyun_subdevice_delete_reply_cb_t)(aliyun_device_reply_t *reply);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_SUBDEVICE_H__ */
