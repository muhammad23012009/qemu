#ifndef WDT_MTK_H
#define WDT_MTK_H

#include "qom/object.h"
#include "hw/ptimer.h"
#include "hw/sysbus.h"

#define WDT_MTK_MAX_REGS    6
#define WDT_MTK_MAX_TIMEOUT 31
#define WDT_MTK_MIN_TIMEOUT 2

#define WDT_MTK_MMIO_SIZE   0x100

/* Registers */
#define WDT_MTK_MODE        0
#define WDT_MTK_TIMEOUT     1
#define WDT_MTK_RST         2
#define WDT_MTK_SWRST       3
#define WDT_MTK_SWSYSRST    4
#define WDT_MTK_SWSYSRST_EN 5

/* Values */
#define WDT_MTK_MODE_KEY            0x22000000
#define WDT_MTK_MODE_ENABLE         (1 << 0)
#define WDT_MTK_MODE_IRQ_ENABLE     (1 << 3)
#define WDT_MTK_MODE_DUAL_ENABLE    (1 << 6)
#define WDT_MTK_MODE_CNT_SEL        (1 << 8)
#define WDT_MTK_TIMEOUT_KEY         0x8
#define WDT_MTK_RST_RELOAD          0x1971
#define WDT_MTK_SWRST_KEY           0x1209

static const int wdt_regmap[] = {
    [0x00] = WDT_MTK_MODE,
    [0x04] = WDT_MTK_TIMEOUT,
    [0x08] = WDT_MTK_RST,
    [0x14] = WDT_MTK_SWRST,
    [0x18] = WDT_MTK_SWSYSRST,
    [0xfc] = WDT_MTK_SWSYSRST_EN
};

#define TYPE_MTK_WDT "wdt-mtk"
OBJECT_DECLARE_SIMPLE_TYPE(MtkWdtState, MTK_WDT)

struct MtkWdtState {
    SysBusDevice parent_obj;
    QEMUTimer *timer;
    QEMUTimer *ptimer;

    MemoryRegion iomem;
    uint32_t regs[WDT_MTK_MAX_REGS];
    qemu_irq irq;

    uint32_t pretimeout;
};

#endif // WDT_MTK_H