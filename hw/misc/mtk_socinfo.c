#include "hw/misc/mtk_socinfo.h"
#include "hw/qdev-properties.h"
#include "qemu/log.h"

static const SocinfoData socinfo_data[] = {
    [SOC_MT6765] = {
        .soc = SOC_MT6765,
        .cells = {
            // We basically emulate physical efuses
            // Layout is little-endian
            {.reg = 0x4c, .value = 0x2},
            {.reg = 0x4d, .value = 0x0},
            {.reg = 0x4e, .value = 0x19},
            {.reg = 0x4f, .value = 0x0},

            {.reg = 0x60, .value = 0x2},
            {.reg = 0x61, .value = 0x0},
            {.reg = 0x62, .value = 0x0},
            {.reg = 0x63, .value = 0x0},
        },
    },
    // Add more socinfo types as necessary
};

static uint64_t mtk_socinfo_read(void *opaque, hwaddr offset, unsigned int size)
{
    MtkSocinfoState *s = MTK_SOCINFO(opaque);

    if (s->soc == -1) {
        return CELL_UNUSED;
    }

    SocinfoData data = socinfo_data[s->soc];

    for (int i = 0; i < MTK_SOCINFO_CELLS; i++) {
        SocinfoCell cell = data.cells[i];
        if (offset == cell.reg)
            return cell.value;
    }
    return CELL_UNUSED;
}

static void mtk_socinfo_write(void *opaque, hwaddr offset, uint64_t data, unsigned int size)
{
    qemu_log_mask(LOG_GUEST_ERROR, "MTK SoCinfo: illegal write!\n");
}

static const MemoryRegionOps mtk_socinfo_ops = {
    .read = mtk_socinfo_read,
    .write = mtk_socinfo_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void mtk_socinfo_init(Object *obj)
{
    MtkSocinfoState *s = MTK_SOCINFO(obj);

    memory_region_init_io(&s->iomem, obj, &mtk_socinfo_ops, s, TYPE_MTK_SOCINFO, 0x10000);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->iomem);
}

static Property mtk_socinfo_properties[] = {
    DEFINE_PROP_INT32("soc", MtkSocinfoState, soc, -1),
    DEFINE_PROP_END_OF_LIST(),
};

static void mtk_socinfo_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_props(dc, mtk_socinfo_properties);
}

static const TypeInfo mtk_socinfo_info = {
    .name          = TYPE_MTK_SOCINFO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(MtkSocinfoState),
    .instance_init = mtk_socinfo_init,
    .class_init    = mtk_socinfo_class_init,
};

static void mtk_socinfo_register(void)
{
    type_register_static(&mtk_socinfo_info);
}

type_init(mtk_socinfo_register)