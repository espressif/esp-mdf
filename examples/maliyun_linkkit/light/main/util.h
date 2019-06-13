
#ifndef __UTIL_H__
#define __UTIL_H__

#define MALIYUN_LINKKIT_RESTART_COUNT_RESET     (5)

/**
 * @brief Initialize wifi.
 *
 * @return
 *     - NULL
 */
mdf_err_t wifi_init();

/**
 * @brief Periodically print system information.
 *
 * @param timer pointer of timer
 *
 * @return
 *     - NULL
 */
void show_system_info_timercb(void *timer);

/**
 * @brief restart count get
 *
 * @return int restart count
 *
 */
int restart_count_get(void);

/**
 * @brief restart is exception
 *
 * @return
 *    - true
 *    - false
 */
bool restart_is_exception(void);

#endif /**< __LIGHT_H__ */