/****************************************************************************
 * arch/arm/src/rp23xx/rp23xx_timerisr.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <time.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/timers/arch_timer.h>
#include <arch/board/board.h>

#include "nvic.h"
#include "clock/clock.h"
#include "arm_internal.h"
#include "chip.h"
#include "systick.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The SysTick is driven by watchdog tock clock. */

#define SYSTICK_CLOCK BOARD_SYS_FREQ

/* The desired timer interrupt frequency is provided by the definition
 * CLK_TCK (see include/time.h).  CLK_TCK defines the desired number of
 * system clock ticks per second.  That value is a user configurable setting
 * that defaults to 100 (100 ticks per second = 10 MS interval).
 */

#define SYSTICK_RELOAD ((SYSTICK_CLOCK / CLK_TCK) - 1)

/* The size of the reload field is 24 bits.  Verify that the reload value
 * will fit in the reload register.
 */

#if SYSTICK_RELOAD > 0x00ffffff
#  error SYSTICK_RELOAD exceeds the range of the RELOAD register
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Function:  rp23xx_timerisr
 *
 * Description:
 *   The timer ISR will perform a variety of services for various portions
 *   of the systems.
 *
 ****************************************************************************/
#ifndef CONFIG_RP23XX_SYSTIMER_SYSTICK
static int rp23xx_timerisr(int irq, uint32_t *regs, void *arg)
{
  /* Process timer interrupt */

  nxsched_process_timer();
  return 0;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function:  up_timer_initialize
 *
 * Description:
 *   This function is called during start-up to initialize
 *   the timer interrupt.
 *
 ****************************************************************************/

void up_timer_initialize(void)
{
  uint32_t regval;

  /* Set the SysTick interrupt to the default priority */

  regval = getreg32(NVIC_SYSH12_15_PRIORITY);
  regval &= ~NVIC_SYSH_PRIORITY_PR15_MASK;
  regval |= (NVIC_SYSH_PRIORITY_DEFAULT << NVIC_SYSH_PRIORITY_PR15_SHIFT);
  putreg32(regval, NVIC_SYSH12_15_PRIORITY);

#ifdef CONFIG_RP23XX_SYSTIMER_SYSTICK
  up_timer_set_lowerhalf(systick_initialize(true, SYSTICK_CLOCK, -1));
#else

  /* Configure SysTick to interrupt at the requested rate */

  putreg32(SYSTICK_RELOAD, NVIC_SYSTICK_RELOAD);
  putreg32(0, NVIC_SYSTICK_CURRENT);

  /* Attach the timer interrupt vector */

  irq_attach(RP23XX_IRQ_SYSTICK, (xcpt_t)rp23xx_timerisr, NULL);

  /* Enable SysTick interrupts. */

  putreg32((NVIC_SYSTICK_CTRL_CLKSOURCE |
            NVIC_SYSTICK_CTRL_TICKINT |
            NVIC_SYSTICK_CTRL_ENABLE),
            NVIC_SYSTICK_CTRL);

  /* And enable the timer interrupt */

  up_enable_irq(RP23XX_IRQ_SYSTICK);
#endif
}
