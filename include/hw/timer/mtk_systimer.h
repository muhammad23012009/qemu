#ifndef MTK_SYSTIMER_H
#define MTK_SYSTIMER_H

#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/irq.h"
#include "qemu/timer.h"

#define SYST_CLOCK          0x40
#define SYST_TICKS          (SYST_CLOCK + 0x4)

/* Values */
#define CLOCK_ENABLE        BIT(0)
#define CLOCK_IRQ_ENABLE    BIT(1)
#define CLOCK_IRQ_CLEAR     BIT(4)

#define TYPE_MTK_SYSTIMER "mtk_systimer"
OBJECT_DECLARE_SIMPLE_TYPE(MtkSystimerState, MTK_SYSTIMER)

struct MtkSystimerState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    QEMUTimer *timer;
    qemu_irq irq;

    bool enabled;
};

#endif // MTK_SYSTIMER_H