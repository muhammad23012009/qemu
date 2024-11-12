#ifndef MT6765_H
#define MT6765_H

#include "qom/object.h"
#include "hw/intc/arm_gicv3.h"
#include "hw/watchdog/wdt_mtk.h"
#include "hw/misc/mt6765_clk.h"
#include "hw/timer/mtk_systimer.h"
#include "hw/char/serial-mtk.h"
#include "hw/misc/mtk_socinfo.h"
#include "hw/misc/unimp.h"
#include "target/arm/cpu.h"

#define MT6765_NCPUS    (8)
#define MT6765_NUART    (2)
#define TYPE_MT6765     "mt6765"

enum {
    MT6765_CLK_TOPCKGEN,
    MT6765_CLK_PERICFG,
    MT6765_GIC_DIST,
    MT6765_GIC_REDIST,
    MT6765_SDRAM,
    MT6765_SOCINFO,
    MT6765_SYSTIMER,
    MT6765_UART0,
    MT6765_UART1,
    MT6765_WDT
};

// GIC
#define GIC_IRQ_NUM (288)
#define ARCH_GIC_MAINT_IRQ (9)
#define VIRTUAL_PMU_IRQ (7)

enum {
    MT6765_GIC_SPI_UART0    = 91,
    MT6765_GIC_SPI_UART1    = 92,
    MT6765_GIC_SPI_WDT      = 139,
    MT6765_GIC_SPI_SYSTIMER = 194
};

OBJECT_DECLARE_SIMPLE_TYPE(MT6765State, MT6765)

struct MT6765State {
    DeviceState parent_obj;

    ARMCPU cpus[MT6765_NCPUS];
    const hwaddr *memmap;
    GICv3State gic;
    MtkWdtState wdt;
    Mt6765ClkState topckgen;
    MtkSystimerState systimer;

    MtkSocinfoState socinfo;
};

#endif // MT6765_H