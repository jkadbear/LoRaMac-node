/*!
 * \file      main.c
 *
 * \brief     Pure LoRa Measurement
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *               _______ _____ _____ _   _  _____ _    _ _    _
 *              |__   __/ ____|_   _| \ | |/ ____| |  | | |  | |  /\
 *                 | | | (___   | | |  \| | |  __| |__| | |  | | /  \
 *                 | |  \___ \  | | | . ` | | |_ |  __  | |  | |/ /\ \
 *                 | |  ____) |_| |_| |\  | |__| | |  | | |__| / ____ \
 *                 |_| |_____/|_____|_| \_|\_____|_|  |_|\____/_/    \_\
 *              (C)2017-2018 Tsinghua
 *
 * \endcode
 *
 * \author    jkadbear( Tsinghua )
 */
#include "board.h"
#include "delay.h"
#include "gpio.h"
#include "radio.h"
#include "serialio.h"
#include "timer.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_MAX_SIZE 242

/*!
 * User application data
 */
static uint8_t AppData[LORAWAN_APP_DATA_MAX_SIZE];

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

static uint8_t is_on = 0;
static uint8_t rec_node_id = 0;
static uint8_t loc_node_id = 1;
static uint8_t rec_datarate = 0;
static uint32_t sent_cnt = 0;

static void OnRadioTxDone(void)
{
    Radio.SetChannel(507500000); // Hz
    Radio.SetRxConfig(MODEM_LORA, 0, 12, 1, 0, 8, 5000, false, 0, false, 0, 0, false, true);
    Radio.Rx(0);
    //printf("OnRadioTxDone\n");
}

static void OnRadioRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
    // printf("OnRadioRxDone\n");
    // for (int i = 0; i < size; i++)
    // {
    //     printf("%x ", payload[i]);
    // }
    // printf("\n");

    if (size == 3)
    {
        for (uint8_t i = 0; i < size; i++)
        {
            if (i == 0)
            {
                rec_node_id = *payload;
            }
            else if (i == 1)
            {
                is_on = (loc_node_id == rec_node_id) ? (*payload) : 0;
            }

            else if (i == 2 && loc_node_id == rec_node_id)
            {
                rec_datarate = *payload;
            }
            payload++;
        }
        sent_cnt = 0;
    }
}

static void OnRadioRxError(void)
{
    // Radio.Sleep();
}

static void OnRadioTxTimeout(void)
{
    // Radio.Sleep();
}

static void OnRadioRxTimeout(void)
{
    // Radio.Sleep();
}

/**
 * Main application entry point.
 */
int main(void)
{
    BoardInitMcu();
    BoardInitPeriph();

    // Initialize Radio driver
    RadioEvents.TxDone = OnRadioTxDone;
    RadioEvents.RxDone = OnRadioRxDone;
    RadioEvents.RxError = OnRadioRxError;
    RadioEvents.TxTimeout = OnRadioTxTimeout;
    RadioEvents.RxTimeout = OnRadioRxTimeout;
    Radio.Init(&RadioEvents);

    // Random seed initialization
    srand1(Radio.Random());

    bool PublicNetwork = true;
    Radio.SetPublicNetwork(PublicNetwork);
    // Radio.Sleep( );

    uint32_t tx_dutycycle = 3000; // ms
    int8_t power = 20;         // [20, 16, 14, 12, 10, 7, 5, 2] dBm
    uint32_t freq = 486300000; // Hz
    uint32_t datarate = 7;     // Spreading Factor
    uint32_t bandwidth = 0;    // 0: 125 kHz, 1: 250 kHz, 2: 500 kHz
    uint8_t coderate = 1;      // 1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8
    uint16_t data_size = 16;   // App Data Size, 32 bytes

    // log_info("scanf NODE_ID, POWER, SF, BW, CR, TX_DUTYCYCLE, APPDATASIZE, FREQUENCY\n");
    // scanf("%d", &loc_node_id);
    // scanf("%d", (uint32_t*)&power);
    // scanf("%d", &datarate);
    // scanf("%d", &bandwidth);
    // scanf("%d", (uint32_t*)&coderate);
    // scanf("%d", &tx_dutycycle);
    // scanf("%d", (uint32_t*)&data_size);
    // scanf("%d", &freq);

    memcpy(AppData, &loc_node_id, 1);      // store node id in AppData
    memcpy(AppData + 4, &tx_dutycycle, 4); // store dutycycle in AppData
    memcpy(AppData + 8, &data_size, 2);    // store data size in AppData
    // log_info("AppData: ");
    // for (int i = 0; i < data_size; i++)
    // {
    //     printf("%x", AppData[i]);
    // }
    // printf("\n");

    OnRadioTxDone();

    while (1)
    {
        if (!is_on)
        {
            DelayMs(tx_dutycycle); // have a rest~~
            continue;
        }

        /*!
        * \brief Sets the transmission parameters
        *
        * \param [IN] modem        Radio modem to be used [0: FSK, 1: LoRa]
        * \param [IN] power        Sets the output power [dBm] [0: 20, 16, 14, 12, 10, 7, 5, 2]
        * \param [IN] fdev         Sets the frequency deviation (FSK only)
        *                          FSK : [Hz]
        *                          LoRa: 0
        * \param [IN] bandwidth    Sets the bandwidth (LoRa only)
        *                          FSK : 0
        *                          LoRa: [0: 125 kHz, 1: 250 kHz,
        *                                 2: 500 kHz, 3: Reserved]
        * \param [IN] datarate     Sets the Datarate
        *                          FSK : 600..300000 bits/s
        *                          LoRa: [6: 64, 7: 128, 8: 256, 9: 512,
        *                                10: 1024, 11: 2048, 12: 4096  chips]
        * \param [IN] coderate     Sets the coding rate (LoRa only)
        *                          FSK : N/A ( set to 0 )
        *                          LoRa: [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
        * \param [IN] preambleLen  Sets the preamble length
        *                          FSK : Number of bytes
        *                          LoRa: Length in symbols (the hardware adds 4 more symbols)
        * \param [IN] fixLen       Fixed length packets [0: variable, 1: fixed]
        * \param [IN] crcOn        Enables disables the CRC [0: OFF, 1: ON]
        * \param [IN] FreqHopOn    Enables disables the intra-packet frequency hopping
        *                          FSK : N/A ( set to 0 )
        *                          LoRa: [0: OFF, 1: ON]
        * \param [IN] HopPeriod    Number of symbols bewteen each hop
        *                          FSK : N/A ( set to 0 )
        *                          LoRa: Number of symbols
        * \param [IN] iqInverted   Inverts IQ signals (LoRa only)
        *                          FSK : N/A ( set to 0 )
        *                          LoRa: [0: not inverted, 1: inverted]
        * \param [IN] timeout      Transmission timeout [ms]
        *
        void    ( *SetTxConfig )( RadioModems_t modem, int8_t power, uint32_t fdev,
                            uint32_t bandwidth, uint32_t datarate,
                            uint8_t coderate, uint16_t preambleLen,
                            bool fixLen, bool crcOn, bool FreqHopOn,
                            uint8_t HopPeriod, bool iqInverted, uint32_t timeout
        );*/
        Radio.Standby();
        Radio.SetChannel(freq);
        if (rec_datarate != 0)
        {
            Radio.SetTxConfig(MODEM_LORA, power, 0, bandwidth, rec_datarate, coderate,
                              8, false, true, 0, 0, false, 3000);
        }
        else
        {
            Radio.SetTxConfig(MODEM_LORA, power, 0, bandwidth, datarate, coderate, 8,
                              false, true, 0, 0, false, 3000);
        }
        /*!
        * \brief Sets the maximum payload length.
        *
        * \param [IN] modem      Radio modem to be used [0: FSK, 1: LoRa]
        * \param [IN] max        Maximum payload length in bytes
        *
        void SX1276SetMaxPayloadLength( RadioModems_t modem, uint8_t max );*/
        // Setup maximum payload lenght of the radio driver
        Radio.SetMaxPayloadLength(MODEM_LORA, data_size);

        memcpy(AppData + 12, &sent_cnt, 4); // store count-number in AppData

        // Send now
        Radio.Send(AppData, data_size);

        // log_debug("sent_cnt:%d, power:%d, bw:%d, dr:%d, cr:%d, freq:%d\n", sent_cnt++,
        //           power, bandwidth, datarate, coderate, freq);
        sent_cnt++;

        DelayMs(tx_dutycycle); // have a rest~~
    }
}
