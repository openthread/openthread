/*
 * stimer.h
 *
 *  Created on: May 21, 2020
 *      Author: Telink
 */

#ifndef STIMER_H_
#define STIMER_H_

typedef enum
{
    STIMER_IRQ_MASK         = BIT(0),
    STIMER_32K_CAL_IRQ_MASK = BIT(1),
} stimer_irq_mask_e;

typedef enum
{
    STIMER_IRQ_CLR         = BIT(0),
    STIMER_32K_CAL_IRQ_CLR = BIT(1),
} stimer_irq_clr_e;

static inline void stimer_set_irq_mask(stimer_irq_mask_e mask)
{
    reg_system_irq_mask |= mask;
}

static inline void stimer_clr_irq_mask(stimer_irq_clr_e mask)
{
    reg_system_cal_irq = mask;
}

static inline void stimer_clr_irq_status(void)
{
    reg_system_cal_irq = STIMER_IRQ_CLR;
}

static inline void stimer_set_irq_capture(unsigned int tick)
{
    reg_system_irq_level = tick;
}

static inline void stimer_set_tick(unsigned int tick)
{
    reg_system_tick = tick;
}

static inline void stimer_enable(void)
{
    reg_system_ctrl |= FLD_SYSTEM_TIMER_EN;
}

static inline void stimer_disable(void)
{
    reg_system_ctrl &= ~(FLD_SYSTEM_TIMER_EN);
}

#endif /* STIMER_H_ */
