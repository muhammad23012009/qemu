
#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qom/object.h"

#include "hw/misc/mt6765_clk.h"

/* TODO: 
 * migrate to full clock tree later. (use QEMU Clock objects)
 * Add different clock types (apmixed, topckgen, pericfg, infracfg)
 */

static uint64_t mt6765_clk_read(void *opaque, hwaddr offset,
                                    unsigned size)
{
    qemu_log_mask(LOG_GUEST_ERROR, "%s: bad read offset 0x%04x\n",
                  __func__, (uint32_t)offset);
    return 0;
}

static void mt6765_clk_write(void *opaque, hwaddr offset,
                                 uint64_t val, unsigned size)
{
    qemu_log_mask(LOG_GUEST_ERROR, "%s: bad write offset 0x%04x\n",
                  __func__, (uint32_t)offset);
}

static const MemoryRegionOps mt6765_clk_ops = {
    .read = mt6765_clk_read,
    .write = mt6765_clk_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
        .unaligned = false
    }
};

static void mt6765_clk_reset(DeviceState *dev)
{
    // do nothing
}

static void mt6765_clk_init(Object *obj)
{
    Mt6765ClkState *s = MT6765_CLK(obj);
    SysBusDevice *dev = SYS_BUS_DEVICE(obj);

    /* memory mapping */
    memory_region_init_io(&s->iomem, obj, &mt6765_clk_ops, s,
                          TYPE_MT6765_CLK, 0x1000);
    sysbus_init_mmio(dev, &s->iomem);
}

static const VMStateDescription mt6765_clk_vmstate = {
    .name = TYPE_MT6765_CLK,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static void mt6765_clk_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_legacy_reset(dc, mt6765_clk_reset);
    dc->vmsd = &mt6765_clk_vmstate;
}

static const TypeInfo mt6765_clk_info = {
    .name          = TYPE_MT6765_CLK,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Mt6765ClkState),
    .instance_init = mt6765_clk_init,
    .class_init    = mt6765_clk_class_init,
};

static void mt6765_clk_register(void)
{
    type_register_static(&mt6765_clk_info);
}

type_init(mt6765_clk_register)
