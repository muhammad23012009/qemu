#ifndef MTK_SOCINFO_H
#define MTK_SOCINFO_H

#include "qemu/osdep.h"
#include "exec/memory.h"
#include "hw/sysbus.h"
#include "qom/object.h"

#define MTK_SOCINFO_CELLS   (2 * 4)
#define CELL_UNUSED         0xFFFFFFFF

enum {
    SOC_MT6765,
};

#define TYPE_MTK_SOCINFO "mtk-socinfo"
OBJECT_DECLARE_SIMPLE_TYPE(MtkSocinfoState, MTK_SOCINFO)

typedef struct SocinfoCell {
    uint8_t reg;
    uint8_t value;
} SocinfoCell;

typedef struct SocinfoData {
    int soc;
    SocinfoCell cells[MTK_SOCINFO_CELLS];
} SocinfoData;

struct MtkSocinfoState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;

    int soc;
};

#endif // MTK_SOCINFO_H