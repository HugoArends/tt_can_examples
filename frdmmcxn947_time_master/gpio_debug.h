/*! ***************************************************************************
 *
 * \brief     General purpose input/output - output
 * \file      gpio_debug.h
 * \author    Hugo Arends
 * \date      March 2025
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
#ifndef GPIO_DEBUG_H
#define GPIO_DEBUG_H

#include <MCXN947_cm33_core0.h>

// This driver asumes that all debug pins are on PORT1

#define DBG0_PIN      (14)
#define DBG1_PIN      (15)
#define DBG2_PIN      (16)
#define DBG3_PIN      (17)

#ifdef DEBUG

#define DBG0_SET()    (GPIO1->PSOR = (1<<DBG0_PIN))
#define DBG0_CLR()    (GPIO1->PCOR = (1<<DBG0_PIN))
#define DBG0_TGL()    (GPIO1->PTOR = (1<<DBG0_PIN))

#define DBG1_SET()    (GPIO1->PSOR = (1<<DBG1_PIN))
#define DBG1_CLR()    (GPIO1->PCOR = (1<<DBG1_PIN))
#define DBG1_TGL()    (GPIO1->PTOR = (1<<DBG1_PIN))

#define DBG2_SET()    (GPIO1->PSOR = (1<<DBG2_PIN))
#define DBG2_CLR()    (GPIO1->PCOR = (1<<DBG2_PIN))
#define DBG2_TGL()    (GPIO1->PTOR = (1<<DBG2_PIN))

#define DBG3_SET()    (GPIO1->PSOR = (1<<DBG3_PIN))
#define DBG3_CLR()    (GPIO1->PCOR = (1<<DBG3_PIN))
#define DBG3_TGL()    (GPIO1->PTOR = (1<<DBG3_PIN))

#define DBGALL_SET()  (GPIO1->PSOR = (1<<DBG0_PIN) | (1<<DBG1_PIN) | (1<<DBG2_PIN) | (1<<DBG3_PIN))
#define DBGALL_CLR()  (GPIO1->PCOR = (1<<DBG0_PIN) | (1<<DBG1_PIN) | (1<<DBG2_PIN) | (1<<DBG3_PIN))
#define DBGALL_TGL()  (GPIO1->PTOR = (1<<DBG0_PIN) | (1<<DBG1_PIN) | (1<<DBG2_PIN) | (1<<DBG3_PIN))

#else

#define DBG0_SET()
#define DBG0_CLR()
#define DBG0_TGL()

#define DBG1_SET()
#define DBG1_CLR()
#define DBG1_TGL()

#define DBG2_SET()
#define DBG2_CLR()
#define DBG2_TGL()

#define DBG3_SET()
#define DBG3_CLR()
#define DBG3_TGL()

#define DBGALL_SET()
#define DBGALL_CLR()
#define DBGALL_TGL()

#endif


void gpio_debug_init(void);

#endif // GPIO_DEBUG_H
