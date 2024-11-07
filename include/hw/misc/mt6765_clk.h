#ifndef MT6765_CLK_H
#define MT6765_CLK_H

#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/irq.h"

/* TODO: implement a full clock tree later */
#define TYPE_MT6765_CLK             "mt6765_clk"
OBJECT_DECLARE_SIMPLE_TYPE(Mt6765ClkState, MT6765_CLK)

struct Mt6765ClkState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
};

#endif // MT6765_CLK_H