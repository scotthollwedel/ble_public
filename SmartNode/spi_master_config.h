/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
#ifndef SPI_MASTER_CONFIG_H
#define SPI_MASTER_CONFIG_H

#define SPI_OPERATING_FREQUENCY  ( 0x02000000UL << (uint32_t)Freq_8Mbps )  /*!< Slave clock frequency. */

/*  SPI0 */
#define VBAT_SW_EN                0   /*!< GPIO pin for powering up CC3000 */
#define SPI_PSELSCK0              1   /*!< GPIO pin number for SPI clock (note that setting this to 31 will only work for loopback purposes as it not connected to a pin) */
#define SPI_PSELMOSI0             2   /*!< GPIO pin number for Master Out Slave In    */
#define SPI_PSELIRQ               3   /*!< GPIO pin number for the CC3000 IRQ */
#define SPI_PSELMISO0             4   /*!< GPIO pin number for Master In Slave Out    */
#define SPI_PSELSS0               5   /*!< GPIO pin number for Slave Select           */
#define TIMEOUT_COUNTER          0x3000UL  /*!< timeout for getting rx bytes from slave */

#endif /* SPI_MASTER_CONFIG_H */
