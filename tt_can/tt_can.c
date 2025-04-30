/*! ***************************************************************************
 *
 * \brief     Time Triggered CAN (TTCAN) scheduler
 * \file      tt_can.c
 * \author    Hugo Arends
 * \date      April 2025
 *
 * \see       NEN-ISO 11898-4:2014. Road vehicles — Controller area network
 *            (CAN) — Part 4: Time-triggered communication.
 *
 * \see       NXP. (2024). MCX A153, A152, A143, A142 Reference Manual. Rev. 4,
 *            01/2024. From:
 *            https://www.nxp.com/docs/en/reference-manual/MCXAP64M96FS3RM.pdf
 *
 * \copyright 2025 HAN University of Applied Sciences. All Rights Reserved.
 *            \n\n
 *            Permission is hereby granted, free of charge, to any person
 *            obtaining a copy of this software and associated documentation
 *            files (the "Software"), to deal in the Software without
 *            restriction, including without limitation the rights to use,
 *            copy, modify, merge, publish, distribute, sublicense, and/or sell
 *            copies of the Software, and to permit persons to whom the
 *            Software is furnished to do so, subject to the following
 *            conditions:
 *            \n\n
 *            The above copyright notice and this permission notice shall be
 *            included in all copies or substantial portions of the Software.
 *            \n\n
 *            THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *            EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *            OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *            NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *            HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *            WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *            FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *            OTHER DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/
#include "tt_can.h"
#include "gpio_debug.h"

// -----------------------------------------------------------------------------
// Local type definitions
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Local function prototypes
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Local variables
// -----------------------------------------------------------------------------

uint8_t tt_cycle_count = 0;
uint8_t tt_column_count = 0;

// Schedule synchronisation state machine (Figure 8)
// TS0
tt_master_state_t tt_master_state =
{
    .error_level = S0,
    .master_slave_mode = CURRENT_MASTER,
    .sync_mode = SYNC_OFF,
};

// The following strings are used for debugging purposes
const char * tt_sync_mode_str[4] =
{
    "SYNC_OFF     ",
    "SYNCHRONISING",
    "IN_GAP       ",
    "IN_SCHEDULE  ",
};

const char * tt_master_slave_mode_str[4] =
{
    "MASTER_OFF    ",
    "SLAVE         ",
    "BACKUP_MASTER ",
    "CURRENT_MASTER",
};

const char * tt_error_level_str[4] =
{
    "S0",
    "S1",
    "S2",
    "S3",
};

const char * tt_master_state_str[3] =
{
    "ERROR_LEVEL",
    "SYNC_MODE",
    "MASTER_SLAVE_MODE",
};

// Scheduler callback functions
static tt_scheduler_callbacks_t tt_scheduler_callbacks = {NULL, NULL, NULL};

// Table of function pointers for the TTCAN scheduler. The table is indexed by
// the cycle count and the column count. The function pointers are set in the
// main application.
extern void (*tt_functions[TT_BASIC_CYCLES][TT_TRANSMISSION_COLUMNS])(void);

// -----------------------------------------------------------------------------
// Function implementations
// -----------------------------------------------------------------------------

/*******************************************************************************
 * \brief     Initialise the TTCAN scheduler
 *
 * This function initialises the TTCAN scheduler. It sets the master_slave mode
 * and the callbacks for the timer initialisation, start and stop. The timer
 * initialisation is called to set up the timer for the scheduler. The function
 * also sets the synchronisation state machine to the initial state. The timer
 * must be configured to generate an interrupt every TT_TIME_WINDOW_US
 * microseconds.
 *
 * \param[in] master_slave_mode  The master_slave mode to set
 *                               (tt_master_slave_mode_t)
 * \param[in] callbacks          Callback functions for timer initialisation,
 *                               start and stop
 * \return    None
 *****************************************************************************/
void tt_scheduler_init(tt_master_slave_mode_t master_slave_mode,
    tt_scheduler_callbacks_t *callbacks)
{
    // Check callbacks
    if((NULL == callbacks->tt_scheduler_timer_init) ||
        (NULL == callbacks->tt_scheduler_timer_start) ||
        (NULL == callbacks->tt_scheduler_timer_stop))
    {
        // Error
        while(1)
        {}
    }

    // Copy callbacks
    tt_scheduler_callbacks = *callbacks;

    // Initialise the timer
    tt_scheduler_callbacks.tt_scheduler_timer_init(TT_TIME_WINDOW_US);

    // Schedule synchronisation state machine (Figure 8)
    // TS0

    // Set master slave mode
    tt_master_state.master_slave_mode = master_slave_mode;

    // Start scheduler in master mode
    if(CURRENT_MASTER == master_slave_mode)
    {
        tt_scheduler_start(0);
        tt_master_state.sync_mode = IN_SCHEDULE;
    }
    else
    {
        // Schedule synchronisation state machine (Figure 8)
        // TS1
        tt_master_state.sync_mode = SYNCHRONISING;
    }

    // Reset error level
    tt_master_state.error_level = S0;
}

/*******************************************************************************
 * \brief     Start the TTCAN scheduler
 *
 * This function starts the TTCAN scheduler by invoking the timer start callback
 * with the specified offset. The offset determines the start value of the timer
 * in microseconds.
 *
 * \param[in] offset_us  The start value of the counter in microseconds.
 * \return    None
 *****************************************************************************/
void tt_scheduler_start(const uint32_t offset_us)
{
    tt_scheduler_callbacks.tt_scheduler_timer_start(offset_us);
}

/*******************************************************************************
 * \brief     Stop the TTCAN scheduler
 *
 * This function stops the TTCAN scheduler by invoking the timer stop callback.
 *
 * \return    None
 *****************************************************************************/
void tt_scheduler_stop(void)
{
    tt_scheduler_callbacks.tt_scheduler_timer_stop();
}

/*******************************************************************************
 * \brief     Set the master state error level
 *
 * This function sets the error level of the master state. The error level is
 * used to indicate the current state of the system.
 *
 * \param[in] error_level  The error level to set (see tt_error_level_t)
 * \return    None
 *****************************************************************************/
void tt_set_master_state_master_slave_mode(tt_master_slave_mode_t master_slave_mode)
{
    tt_master_state.master_slave_mode = master_slave_mode;
}

/*******************************************************************************
 * \brief     Get the master state error level
 *
 * This function gets the error level of the master state. The error level is
 * used to indicate the current state of the system.
 *
 * \param     None
 * \return    The error level (see tt_error_level_t)
 *****************************************************************************/
tt_master_slave_mode_t tt_get_master_state_master_slave_mode(void)
{
    return tt_master_state.master_slave_mode;
}

/*******************************************************************************
 * \brief     Set the master state synchronisation mode
 *
 * This function sets the synchronisation mode of the master state. The
 * synchronisation mode is used to indicate the current state of the system.
 *
 * \param[in] sync_mode  The synchronisation mode to set (see tt_sync_mode_t)
 * \return    None
 *****************************************************************************/
void tt_set_master_state_sync_mode(tt_sync_mode_t sync_mode)
{
    tt_master_state.sync_mode = sync_mode;
}

/*******************************************************************************
 * \brief     Get the master state synchronisation mode
 *
 * This function gets the synchronisation mode of the master state. The
 * synchronisation mode is used to indicate the current state of the system.
 *
 * \param     None
 * \return    The synchronisation mode (see tt_sync_mode_t)
 *****************************************************************************/
tt_sync_mode_t tt_get_master_state_sync_mode(void)
{
    return tt_master_state.sync_mode;
}

/*******************************************************************************
 * \brief     Set the master state error level
 *
 * This function sets the error level of the master state. The error level is
 * used to indicate the current state of the system.
 *
 * \param[in] error_level  The error level to set (see tt_error_level_t)
 * \return    None
 *****************************************************************************/
void tt_set_master_state_error_level(tt_error_level_t error_level)
{
    tt_master_state.error_level = error_level;
}

/*******************************************************************************
 * \brief     Get the master state error level
 *
 * This function gets the error level of the master state. The error level is
 * used to indicate the current state of the system.
 *
 * \param     None
 * \return    The error level (see tt_error_level_t)
 *****************************************************************************/
tt_error_level_t tt_get_master_state_error_level(void)
{
    return tt_master_state.error_level;
}

/*******************************************************************************
 * \brief     TTCAN scheduler
 *
 * This function must be called as often as possible in the main loop. If the
 * current cycle and/or column count was updated, it calls the corresponding
 * function from the tt_functions table. The function is only called if the
 * column count has changed since the last call.
 *
 * \param     None
 * \return    None
 *****************************************************************************/
void tt_scheduler(void)
{
    static uint8_t column_count_prev = 0xFF;

    if(column_count_prev != tt_column_count)
    {
        column_count_prev = tt_column_count;

        // Set next Cycle_Count
        if(0 == tt_column_count)
        {
            tt_cycle_count = (tt_cycle_count + 1) % TT_BASIC_CYCLES;
        }

        // Call the function for the current cycle and column
        if(NULL != tt_functions[tt_cycle_count][tt_column_count])
        {
            tt_functions[tt_cycle_count][tt_column_count]();
        }
    }
}

/*******************************************************************************
 * \brief     TTCAN scheduler timer callback
 *
 * This function is called by the timer interrupt every TT_TIME_WINDOW_US
 * microseconds. It updates the column count for the next cycle.
 *
 * \param     None
 * \return    None
 *****************************************************************************/
void tt_scheduler_timer_callback(void)
{
    tt_column_count = (tt_column_count + 1) % TT_TRANSMISSION_COLUMNS;
}
