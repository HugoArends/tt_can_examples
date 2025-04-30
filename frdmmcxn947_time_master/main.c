/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "fsl_flexcan.h"
#include "fsl_ctimer.h"
#include "board.h"
#include "app.h"
#include "tt_can.h"
#include "gpio_debug.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Fix MISRA_C-2012 Rule 17.7. */
#define LOG_INFO (void)PRINTF
#define DLC (8)

#ifdef DEBUG
#define TARGETSTR "Debug"
#else
#define TARGETSTR "Release"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

void tt_tx_reference(void);
void tt_tx_msgd(void);
void tt_rx_msgr(void);

void ctimer4_init(const uint32_t match_value_us);
void ctimer4_start(const uint32_t offset_us);
void ctimer4_stop(void);
void ctimer4_match0_callback(uint32_t flags);

/*******************************************************************************
 * Variables
 ******************************************************************************/
flexcan_handle_t flexcanHandle;
volatile bool txReferenceMessageComplete = false;
volatile bool rxReferenceMessageComplete = false;
volatile bool rxMessageRComplete = false;
volatile bool wakenUp = false;
flexcan_mb_transfer_t txReferenceMessageXfer, rxReferenceMessageXfer, rxMessageRXfer;
flexcan_frame_t frame;
uint32_t txReferenceMessageId;
uint32_t rxReferenceMessageId;
uint32_t rxMessageRId;
flexcan_config_t flexcanConfig;
flexcan_rx_mb_config_t mbReferenceMessageConfig, mbMessageRConfig;

static ctimer_match_config_t matchConfig0;
ctimer_callback_t ctimer_callback = ctimer4_match0_callback;

void (*tt_functions[TT_BASIC_CYCLES][TT_TRANSMISSION_COLUMNS])(void) =
{
    {tt_tx_reference, NULL, NULL,       NULL, NULL, tt_tx_msgd, NULL,},
    {tt_tx_reference, NULL, NULL,       NULL, NULL, NULL,       NULL,},
    {tt_tx_reference, NULL, tt_rx_msgr, NULL, NULL, NULL,       NULL,},
    {tt_tx_reference, NULL, NULL,       NULL, NULL, NULL,       NULL,},
};

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief FlexCAN Call Back function
 */
static FLEXCAN_CALLBACK(flexcan_callback)
{
    switch (status)
    {
        case kStatus_FLEXCAN_RxIdle:
            if(RX_MESSAGE_BUFFER_NUM == result)
            {
                rxReferenceMessageComplete = true;
            }

            if(RX_MESSAGE_BUFFER_NUM2 == result)
            {
                rxMessageRComplete = true;
            }
            break;

        case kStatus_FLEXCAN_TxIdle:
            if(TX_MESSAGE_BUFFER_NUM == result)
            {
                txReferenceMessageComplete = true;
            }
            break;

        case kStatus_FLEXCAN_WakeUp:
            wakenUp = true;
            break;

        default:
            break;
    }
}

/*!
 * @brief Main function
 */
int main(void)
{
    // Initialize board hardware.
    BOARD_InitHardware();

    gpio_debug_init();

    LED_RED_INIT(LOGIC_LED_OFF);
    LED_GREEN_INIT(LOGIC_LED_OFF);
    LED_BLUE_INIT(LOGIC_LED_OFF);

    LOG_INFO("\r\n");
    LOG_INFO("Time-triggered: Time master\r\n");
    LOG_INFO("%s build %s %s\r\n", TARGETSTR, __DATE__, __TIME__);
    LOG_INFO("TT_TIME_WINDOW_US      : %d\r\n", TT_TIME_WINDOW_US);
    LOG_INFO("TT_NTU_US              : %d\r\n", TT_NTU_US);
    LOG_INFO("TT_BASIC_CYCLES        : %d\r\n", TT_BASIC_CYCLES);
    LOG_INFO("TT_TRANSMISSION_COLUMNS: %d\r\n", TT_TRANSMISSION_COLUMNS);

    txReferenceMessageId = 0x0;
    rxReferenceMessageId = 0x0;
    rxMessageRId = 0x123;

    /* Get FlexCAN module default Configuration.
     *
     * flexcanConfig.clkSrc                 = kFLEXCAN_ClkSrc0;
     * flexcanConfig.bitRate                = 1000000U;
     * flexcanConfig.bitRateFD              = 2000000U;
     * flexcanConfig.maxMbNum               = 16;
     * flexcanConfig.enableLoopBack         = false;
     * flexcanConfig.enableTimerSync        = true;
     * flexcanConfig.enableSelfWakeup       = false;
     * flexcanConfig.enableIndividMask      = false;
     * flexcanConfig.disableSelfReception   = false;
     * flexcanConfig.enableListenOnlyMode   = false;
     * flexcanConfig.enableDoze             = false;
     */
    FLEXCAN_GetDefaultConfig(&flexcanConfig);

    // In each FSE, local time shall be implemented by a cyclic incremented
    // counter. In level 1, the counter shall contain 16 bits.
    flexcanConfig.enableTimerSync = false;

    flexcan_timing_config_t timing_config;
    memset(&timing_config, 0, sizeof(flexcan_timing_config_t));

    if (FLEXCAN_CalculateImprovedTimingValues(EXAMPLE_CAN, flexcanConfig.bitRate, EXAMPLE_CAN_CLK_FREQ, &timing_config))
    {
        // Update the improved timing configuration
        memcpy(&(flexcanConfig.timingConfig), &timing_config, sizeof(flexcan_timing_config_t));
    }
    else
    {
        LOG_INFO("No found Improved Timing Configuration. Use default configuration\r\n");
    }

    FLEXCAN_Init(EXAMPLE_CAN, &flexcanConfig, EXAMPLE_CAN_CLK_FREQ);

    // Create FlexCAN handle structure and set call back function.
    FLEXCAN_TransferCreateHandle(EXAMPLE_CAN, &flexcanHandle, flexcan_callback, NULL);

    // Set Rx Masking mechanism.
    FLEXCAN_SetRxMbGlobalMask(EXAMPLE_CAN, FLEXCAN_RX_MB_STD_MASK(rxReferenceMessageId | rxMessageRId, 0, 0));

    // Setup Rx Message Buffer.
    mbReferenceMessageConfig.format = kFLEXCAN_FrameFormatStandard;
    mbReferenceMessageConfig.type   = kFLEXCAN_FrameTypeData;
    mbReferenceMessageConfig.id     = FLEXCAN_ID_STD(rxReferenceMessageId);
    FLEXCAN_SetRxMbConfig(EXAMPLE_CAN, RX_MESSAGE_BUFFER_NUM, &mbReferenceMessageConfig, true);

    mbMessageRConfig.format = kFLEXCAN_FrameFormatStandard;
    mbMessageRConfig.type   = kFLEXCAN_FrameTypeData;
    mbMessageRConfig.id     = FLEXCAN_ID_STD(rxMessageRId);
    FLEXCAN_SetRxMbConfig(EXAMPLE_CAN, RX_MESSAGE_BUFFER_NUM2, &mbMessageRConfig, true);

    // Setup Tx Message Buffer.
    FLEXCAN_SetTxMbConfig(EXAMPLE_CAN, TX_MESSAGE_BUFFER_NUM, true);

    // CTIMER3 is used as a free running timer togenerate accurate delays.
    // Configure CTIMER3 with default configuration settings.
    // The timer runs at 12MHz (see hardware_init.c).
    ctimer_config_t config;
    CTIMER_GetDefaultConfig(&config);
    CTIMER_Init(CTIMER3, &config);
    CTIMER_StartTimer(CTIMER3);

    tt_scheduler_callbacks_t tt_scheduler_callbacks =
    {
        ctimer4_init,
        ctimer4_start,
        ctimer4_stop,
    };

    tt_scheduler_init(CURRENT_MASTER, &tt_scheduler_callbacks);

    while(true)
    {
        tt_scheduler();

        // Set to synchronising mode in case of error level other than S0
        if(S0 < tt_get_master_state_error_level())
        {
            // Restore error level
            tt_set_master_state_error_level(S0);

            // Set to synchronising mode
            tt_set_master_state_sync_mode(SYNCHRONISING);
        }
    }
}

/**
 * @brief Initializes CTIMER4 with the specified match value in microseconds.
 *
 * This function configures CTIMER4 with default settings and sets up a match
 * channel to generate an interrupt every match_value_us microseconds. The
 * timer runs at a frequency of 12 MHz, and the match value is calculated
 * accordingly. A single callback function is registered for handling the
 * interrupt. In the interrupt handler, call the function
 * tt_scheduler_timer_callback().
 *
 * @param match_value_us The match value in microseconds. This determines the
 *                       duration after which the timer interrupt is triggered.
 */
void ctimer4_init(const uint32_t match_value_us)
{
    ctimer_config_t config;

    // Configure CTIMER4 with default configuration settings.
    // The timer runs at 12MHz (see hardware_init.c).
    CTIMER_GetDefaultConfig(&config);

    CTIMER_Init(CTIMER4, &config);

    // Configuration match channel 0
    matchConfig0.enableCounterReset = true;
    matchConfig0.enableCounterStop  = false;
    matchConfig0.matchValue         = (12 * match_value_us) - 1;
    matchConfig0.outControl         = kCTIMER_Output_NoAction;
    matchConfig0.outPinInitState    = false;
    matchConfig0.enableInterrupt    = true;

    // Set callback function. Single callback is sufficient, because a single
    // channel is used.
    CTIMER_RegisterCallBack(CTIMER4, &ctimer_callback, kCTIMER_SingleCallback);

    // Configure match.
    CTIMER_SetupMatch(CTIMER4, kCTIMER_Match_0, &matchConfig0);
}

/**
 * @brief Callback function for CTIMER4 Match 0 interrupt.
 *
 * This function is triggered when a match event occurs on Match 0 of CTIMER4.
 * It should be executed every TT_TIME_WINDOW_US microseconds (if the scheduler
 * is enabled). It calls the scheduler timer callback function to handle the
 * event.
 *
 * @param flags Flags indicating the match event details (unused in this function).
 */
void ctimer4_match0_callback(uint32_t flags)
{
    tt_scheduler_timer_callback();
}

/**
 * @brief Starts the CTIMER4 timer with a specified offset.
 *
 * This function is called by the TTCAN scheduler to (re)start generation
 * of interrupts.
 *
 * This function (re)starts the CTIMER4 timer with a given offset
 * in microseconds. The timer runs at a frequency of 12 MHz, and the offset
 * is used to set the initial timer count value.
 *
 * @param offset_us The offset in microseconds to set the initial timer count.
 *                  This value is multiplied by 12 to account for the 12 MHz
 *                  timer frequency.
 */
void ctimer4_start(const uint32_t offset_us)
{
    // Note: CTIMER4 runs at 12MHz
    CTIMER4->TC = CTIMER_TC_TCVAL(12 * offset_us);
    CTIMER_StartTimer(CTIMER4);
}

/**
 * @brief Stops the operation of CTIMER4.
 *
 * This function is called by the TTCAN scheduler to stop generation of
 * interrupts.
 *
 * This function halts the timer associated with CTIMER4 by calling the
 * CTIMER_StopTimer function. It is typically used to stop the timer
 * when it is no longer needed or to pause its operation.
 */
void ctimer4_stop(void)
{
    CTIMER_StopTimer(CTIMER4);
}

void tt_tx_reference(void)
{
    DBG0_SET();

    if(IN_GAP == tt_get_master_state_sync_mode())
    {
        // Stop scheduler if it was running
        tt_scheduler_stop();

        // Generate a gap of twenty time windows before transmitting the next
        // reference message. Max allowd value is TT_CYCLE_COUNT_MAX.
        CTIMER3->TC = 0;
        const uint32_t timeout_us_max = 12 * (TT_TIME_WINDOW_US * 20); // CTIMER3 runs at 12 MHz
        while(CTIMER3->TC < timeout_us_max)
        {}

        tt_set_master_state_sync_mode(IN_SCHEDULE);
    }

    uint8_t tt_next_is_gap = 0;

    // *********** //
    // FOR TESTING //
    // *********** //
    // START
    #if 1

    // Generate Is_Gap every 100th time the reference message is sent
    static uint32_t gap_test_cnt = 0;
    const uint32_t gap_test_max = 100;
    if(gap_test_cnt++ > gap_test_max)
    {
        gap_test_cnt = 0;

        tt_set_master_state_sync_mode(IN_GAP);
        tt_next_is_gap = 0x80;
    }

    #endif
    // END
    // *********** //
    // FOR TESTING //
    // *********** //

    // Transmit reference message
    frame.id     = FLEXCAN_ID_STD(txReferenceMessageId);
    frame.format = (uint8_t)kFLEXCAN_FrameFormatStandard;
    frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
    frame.length = (uint8_t)DLC;
    frame.dataByte0 = tt_next_is_gap | tt_cycle_count;
    txReferenceMessageXfer.mbIdx = (uint8_t)TX_MESSAGE_BUFFER_NUM;
    txReferenceMessageXfer.frame = &frame;
    FLEXCAN_TransferSendNonBlocking(EXAMPLE_CAN, &flexcanHandle, &txReferenceMessageXfer);

    // Wait until Tx transmit complete or timeout
    CTIMER3->TC = 0;
    const uint32_t timeout_us_max = 12 * TT_RX_TRIGGER; // CTIMER3 runs at 12 MHz
    while(!txReferenceMessageComplete && (CTIMER3->TC < timeout_us_max))
    {}

    // Timeout?
    if(CTIMER3->TC >= timeout_us_max)
    {
        // Watch_Trigger reached

        // Abort transmission
        FLEXCAN_TransferAbortSend(EXAMPLE_CAN, &flexcanHandle, txReferenceMessageXfer.mbIdx);

        // Set error level
        tt_set_master_state_error_level(S3);

        LOG_INFO("[%02d][%02d][tt_tx_reference] %s %s %s\r\n",  tt_cycle_count, tt_column_count,
            tt_error_level_str[tt_get_master_state_error_level()],
            tt_sync_mode_str[tt_get_master_state_sync_mode()],
            tt_master_slave_mode_str[tt_get_master_state_master_slave_mode()]);

        // Unable to synchronise, let application decide what to do
        tt_set_master_state_sync_mode(SYNC_OFF);

        DBG0_CLR();

        return;
    }

    // Clear the flag
    txReferenceMessageComplete = false;

    // Receive reference message that was just transmitted
    rxReferenceMessageXfer.mbIdx = (uint8_t)RX_MESSAGE_BUFFER_NUM;
    rxReferenceMessageXfer.frame = &frame;
    FLEXCAN_TransferReceiveNonBlocking(EXAMPLE_CAN, &flexcanHandle, &rxReferenceMessageXfer);

    // TODO
    // For now, assume that if the reference message is sent, the Rx Message
    // Buffer is will also receive the reference message. This is not always true, but
    // for now, it is sufficient. In the future, we may need to check the Rx Message Buffer
    while(!rxReferenceMessageComplete)
    {}

    // Clear the flag
    rxReferenceMessageComplete = false;

    if(SYNCHRONISING == tt_get_master_state_sync_mode())
    {
        // Synchronisation successful
        tt_set_master_state_sync_mode(IN_SCHEDULE);
    }

    tt_set_master_state_error_level(S0);

    // Note: frame.timestamp = Ref_Mark

    // Calculate offset with respect to SOF timestamp
    uint32_t tt_cycle_time = 0;
    uint32_t current_timestamp = CAN0->TIMER;
    if(current_timestamp > frame.timestamp)
    {
        tt_cycle_time = current_timestamp - frame.timestamp;
    }
    else
    {
        tt_cycle_time = (0xFFFF - frame.timestamp) + current_timestamp;
    }

    // Compensate scheduler timer with this offset
    tt_scheduler_start(tt_cycle_time);

    LOG_INFO("[%02d][%02d][tt_tx_reference] %s %s %s\r\n", tt_cycle_count, tt_column_count,
        tt_error_level_str[tt_get_master_state_error_level()],
        tt_sync_mode_str[tt_get_master_state_sync_mode()],
        tt_master_slave_mode_str[tt_get_master_state_master_slave_mode()]);

    DBG0_CLR();
}

void tt_tx_msgd(void)
{
    DBG1_SET();

    LED_GREEN_ON();

    // For now, do nothing in this timeslot, except for blinking the green LED and
    // printing a log info.

    LOG_INFO("[%02d][%02d][tt_rx_msgd]\r\n", tt_cycle_count, tt_column_count);

    LED_GREEN_OFF();

    DBG1_CLR();
}

void tt_rx_msgr(void)
{
    DBG1_SET();

    LED_RED_ON();

    // Receive message R
    frame.id     = FLEXCAN_ID_STD(rxMessageRId);
    frame.format = (uint8_t)kFLEXCAN_FrameFormatStandard;
    frame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
    frame.length = (uint8_t)DLC;
    rxMessageRXfer.mbIdx = (uint8_t)RX_MESSAGE_BUFFER_NUM2;
    rxMessageRXfer.frame = &frame;

    FLEXCAN_TransferReceiveNonBlocking(EXAMPLE_CAN, &flexcanHandle, &rxMessageRXfer);

    // Wait until Rx receive full or timeout
    CTIMER3->TC = 0;
    const uint32_t timeout_us_max = 12 * TT_RX_TRIGGER; // CTIMER3 runs at 12 MHz
    while(!rxMessageRComplete && (CTIMER3->TC < timeout_us_max))
    {}

    // Timeout?
    if(CTIMER3->TC >= timeout_us_max)
    {
        // Abort reception of the message R
        FLEXCAN_TransferAbortReceive(EXAMPLE_CAN, &flexcanHandle, rxMessageRXfer.mbIdx);

        LOG_INFO("[%02d][%02d][tt_rx_msgr] Timeout\r\n", tt_cycle_count, tt_column_count);
    }
    else
    {
        LOG_INFO("[%02d][%02d][tt_tx_msgr] IN %d\r\n", tt_cycle_count, tt_column_count, frame.dataByte0);
    }

    rxMessageRComplete = false;

    LED_RED_OFF();

    DBG1_CLR();
}
