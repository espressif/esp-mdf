#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Attenuation range: 0.5 dB LSB steps to 31.5 dB
 *
*/
#define MAX_ATT_SUPPORTED       31
#define DEFAULT_ATT_POWER_ON    0

/**
 * @breif Attenuator init
 */
void attenuator_init(void);

/**
 * @breif Attenuation data seting
 *      Attenuation range: 0.5 dB LSB steps to 31.5 dB
 * In a bit stream to serial data input, the first eight bits (D[7:0])are designated as attenuation data,
 *  and the last eight bits (A[7:0]) are designated as address data.
 */
void attenuator_set(uint8_t att_val);

#ifdef __cplusplus
}
#endif
