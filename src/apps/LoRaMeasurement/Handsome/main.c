#include <string.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "serialio.h"
#include "board.h"
#include "gpio.h"
#include "delay.h"
#include "timer.h"
#include "radio.h"

/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_MAX_SIZE                           242

/*!
 * User application data
 */
static uint8_t AppData[LORAWAN_APP_DATA_MAX_SIZE];

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

static void OnRadioTxDone( void )
{
    Radio.Sleep( );
}

static void OnRadioRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Radio.Sleep( );
}

static void OnRadioRxError( void )
{
    Radio.Sleep( );
}

static void OnRadioTxTimeout( void )
{
    Radio.Sleep( );
}

static void OnRadioRxTimeout( void )
{
    Radio.Sleep( );
}

/**
 * Main application entry point.
 */
int main( void )
{
    BoardInitMcu( );
    BoardInitPeriph( );

	// Initialize Radio driver
    RadioEvents.TxDone = OnRadioTxDone;
    RadioEvents.RxDone = OnRadioRxDone;
    RadioEvents.RxError = OnRadioRxError;
    RadioEvents.TxTimeout = OnRadioTxTimeout;
    RadioEvents.RxTimeout = OnRadioRxTimeout;
    Radio.Init( &RadioEvents );

    // Random seed initialization
    srand1( Radio.Random( ) );

    bool PublicNetwork = true;
    Radio.SetPublicNetwork( PublicNetwork );
    Radio.Sleep( );

    // scanf does not support int8_t or uint8_t
    uint32_t node_id = 1;
    int16_t power = 20; // [20, 16, 14, 12, 10, 7, 5, 2] dBm, int8_t
    uint32_t datarate = 9; // Spreading Factor
    uint32_t bandwidth = 0; // 0: 125 kHz, 1: 250 kHz, 2: 500 kHz
    uint16_t coderate = 1; // 1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8, uint8_t
    uint32_t tx_dutycycle = 1000; // ms
    uint16_t data_size = 32; // App Data Size, 32 bytes
    uint32_t freq = 471500000; // Hz

    log_info("scanf NODE_ID, POWER, SF, BW, CR, TX_DUTYCYCLE, APPDATASIZE, FREQUENCY\n");
    scanf("%ld", &node_id);
    scanf("%hd", &power);
    scanf("%ld", &datarate);
    scanf("%ld", &bandwidth);
    scanf("%hd", &coderate);
    scanf("%ld", &tx_dutycycle);
    scanf("%hd", &data_size);
    scanf("%ld", &freq);

    memset( AppData, 0, sizeof(AppData) );
    memcpy( AppData, &node_id, 4 ); // store node id in AppData
    memcpy( AppData+4, &tx_dutycycle, 4 ); // store dutycycle in AppData
    memcpy( AppData+8, &data_size, 4 ); // store data size in AppData
    printf("AppData: ");
    for( int i = 0; i < data_size; i++ )
        printf("%x", AppData[i]);
    printf("\n");

    while( 1 )
    {
        static uint32_t sentCnt = 0;

        Radio.SetChannel( freq );

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
                                                            uint8_t HopPeriod, bool iqInverted, uint32_t timeout );*/

        Radio.SetTxConfig( MODEM_LORA, power, 0,
                        bandwidth, datarate,
                        coderate, 8,
                        false, true, 0,
                        0, false, 3000 );

        /*!
         * \brief Sets the maximum payload length.
         *
         * \param [IN] modem      Radio modem to be used [0: FSK, 1: LoRa]
         * \param [IN] max        Maximum payload length in bytes
         *
        void SX1276SetMaxPayloadLength( RadioModems_t modem, uint8_t max );*/
        // Setup maximum payload lenght of the radio driver
        Radio.SetMaxPayloadLength( MODEM_LORA, data_size );

        printf("sentCnt:%ld, power:%d, bw:%ld, dr:%ld, cr:%d, freq:%ld\n",
            sentCnt++, power, bandwidth, datarate, coderate, freq);

        // Send now
        Radio.Send( AppData, data_size );

        memcpy( AppData+12, &sentCnt, 4 ); // store count-number in AppData

        DelayMs( tx_dutycycle ); // have a rest~~
    }
}
