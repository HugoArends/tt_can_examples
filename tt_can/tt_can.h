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
#ifndef TT_CAN_H
#define TT_CAN_H

#include <stdint.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Shared type definitions
// -----------------------------------------------------------------------------

// Duration of a time window in microseconds
#define TT_TIME_WINDOW_US        (50000)

// Network Time Unit in microseconds
#define TT_NTU_US                (1)

// Number of basic cycles
#define TT_BASIC_CYCLES          (4)

// Number of transmission columns
#define TT_TRANSMISSION_COLUMNS  (7)

// Maximum cycle count
#define TT_CYCLE_COUNT_MAX       (TT_BASIC_CYCLES * TT_TRANSMISSION_COLUMNS)

// The RX_TRIGGER gives a point in time after which the reception of the
// corresponding message is to be verified.
#define TT_RX_TRIGGER            (TT_TIME_WINDOW_US / 4)

typedef enum tt_sync_mode
{
    SYNC_OFF = 0,
    SYNCHRONISING,
    IN_GAP,
    IN_SCHEDULE,
}
tt_sync_mode_t;

typedef enum tt_master_slave_mode
{
    MASTER_OFF = 0,
    SLAVE,
    BACKUP_MASTER,
    CURRENT_MASTER,
}
tt_master_slave_mode_t;

typedef enum tt_errorlevel
{
    S0 = 0,
    S1,
    S2,
    S3,
}
tt_error_level_t;

typedef struct tt_master_state
{
    tt_error_level_t error_level;
    tt_sync_mode_t sync_mode;
    tt_master_slave_mode_t master_slave_mode;
}
tt_master_state_t;

extern const char * tt_sync_mode_str[4];
extern const char * tt_master_slave_mode_str[4];
extern const char * tt_error_level_str[4];
extern const char * tt_master_state_str[3];

typedef void (*tt_scheduler_timer_init_t)(const uint32_t match_value_us);
typedef void (*tt_scheduler_timer_start_t)(const uint32_t offset_us);
typedef void (*tt_scheduler_timer_stop_t)(void);

typedef struct tt_scheduler_callbacks
{
    tt_scheduler_timer_init_t tt_scheduler_timer_init;
    tt_scheduler_timer_start_t tt_scheduler_timer_start;
    tt_scheduler_timer_stop_t tt_scheduler_timer_stop;
}tt_scheduler_callbacks_t;

// Currently not used, because these are configured in the tt_functions pointer
// table.
//
// typedef struct tt_tx_trigger
// {
//     int32_t time_mark;
//     //window_type_t window_type; // TODO
//     int32_t cycle_offset;
//     int32_t repeat_factor;
// }
// tt_tx_trigger_t;
//
// typedef struct tt_tx_ref_trigger
// {
//     tt_tx_trigger_t tx_trigger;
//     int32_t ref_trigger_offset_ms;
// }
// tt_tx_ref_trigger_t;

// -----------------------------------------------------------------------------
// Shared function prototypes
// -----------------------------------------------------------------------------

void tt_scheduler_init(tt_master_slave_mode_t master_slave_mode,
    tt_scheduler_callbacks_t *callbacks);
void tt_scheduler_start(const uint32_t offset_us);
void tt_scheduler_stop(void);
void tt_scheduler_timer_callback(void);
void tt_scheduler(void);

void tt_set_master_state_master_slave_mode(tt_master_slave_mode_t master_slave_mode);
tt_master_slave_mode_t tt_get_master_state_master_slave_mode(void);

void tt_set_master_state_sync_mode(tt_sync_mode_t sync_mode);
tt_sync_mode_t tt_get_master_state_sync_mode(void);

void tt_set_master_state_error_level(tt_error_level_t error_level);
tt_error_level_t tt_get_master_state_error_level(void);

// -----------------------------------------------------------------------------
// Shared variables
// -----------------------------------------------------------------------------
extern uint8_t tt_cycle_count;
extern uint8_t tt_column_count;

// -----------------------------------------------------------------------------
// Shared function prototypes
// -----------------------------------------------------------------------------

#endif // TT_CAN_H
