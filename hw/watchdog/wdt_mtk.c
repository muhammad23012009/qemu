#include "qemu/osdep.h"

#include "qapi/error.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "sysemu/watchdog.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/sysbus.h"
#include "hw/watchdog/wdt_mtk.h"
#include "migration/vmstate.h"

static bool mtk_wdt_enabled(MtkWdtState *s)
{
    return s->regs[WDT_MTK_MODE] & WDT_MTK_MODE_ENABLE;
}

static void mtk_wdt_reload(MtkWdtState *s)
{
    uint64_t reload = s->regs[WDT_MTK_TIMEOUT];

    reload = ((reload >> 6) >> 5);
    printf("MTK WDT: Reloading timer %ld\n", reload);

    /* In dual mode, IRQ occurs at timeout / 2 (which is the value we get),
       and the real timeout occurs at timeout.
    */
    if (s->regs[WDT_MTK_MODE] & (WDT_MTK_MODE_IRQ_ENABLE | WDT_MTK_MODE_DUAL_ENABLE))
        reload *= 2;

    if (mtk_wdt_enabled(s)) {
        timer_mod(s->timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + (reload * NANOSECONDS_PER_SECOND));
        timer_mod(s->ptimer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + (s->pretimeout * NANOSECONDS_PER_SECOND));
    }
}

static uint64_t mtk_wdt_read(void *opaque, hwaddr offset, unsigned size)
{
    MtkWdtState *s = MTK_WDT(opaque);

    int value = wdt_regmap[offset];

    switch (value) {
        case WDT_MTK_MODE:
            return s->regs[WDT_MTK_MODE];
        default:
            printf("Tried to read unimplemented register at offset 0x%" HWADDR_PRIx "\n", offset);
            break;
    }
    return 0;
}

static void mtk_wdt_write(void *opaque, hwaddr offset, uint64_t data, unsigned size)
{
    MtkWdtState *s = MTK_WDT(opaque);

    int value = wdt_regmap[offset];

    switch (value) {
        case WDT_MTK_MODE: {
            if (data & WDT_MTK_MODE_KEY) {
                int enabled = data & WDT_MTK_MODE_ENABLE;
                s->regs[WDT_MTK_MODE] = enabled;

                if (enabled && !mtk_wdt_enabled(s)) {
                    mtk_wdt_reload(s);
                } else if (!enabled && mtk_wdt_enabled(s)) {
                    timer_del(s->timer);
                    timer_del(s->ptimer);
                }

                printf("is IRQ enabled? %ld\n", data & WDT_MTK_MODE_IRQ_ENABLE);
                printf("is dual mode enabled? %ld\n", data & WDT_MTK_MODE_DUAL_ENABLE);
            }
            break;
        }
        
        case WDT_MTK_TIMEOUT: {
            if (data & WDT_MTK_TIMEOUT_KEY) {
                s->regs[WDT_MTK_TIMEOUT] = data;

                if (s->regs[WDT_MTK_MODE] & (WDT_MTK_MODE_IRQ_ENABLE | WDT_MTK_MODE_DUAL_ENABLE)) {
                    s->pretimeout = ((data >> 6) >> 5);
                } else {
                    s->pretimeout = 0;
                }
            }
            break;
        }

        case WDT_MTK_RST: {
            if ((data & 0xFFFF) == WDT_MTK_RST_RELOAD) {
                mtk_wdt_reload(s);
            }
            break;
        }
        
        default:
            printf("Tried to write to unimplemented register at offset 0x%" HWADDR_PRIx " with value %ld\n", offset, data);
            break;
    }
}

static void mtk_wdt_reset(DeviceState *dev)
{
    MtkWdtState *s = MTK_WDT(dev);

    s->regs[WDT_MTK_MODE] = ~(WDT_MTK_MODE_ENABLE);
    s->regs[WDT_MTK_TIMEOUT] = WDT_MTK_MAX_TIMEOUT;
    s->regs[WDT_MTK_RST] = 0;
    s->regs[WDT_MTK_SWRST] = 0;
    s->regs[WDT_MTK_SWSYSRST] = 0;
    s->regs[WDT_MTK_SWSYSRST_EN] = 0;
    s->pretimeout = WDT_MTK_MAX_TIMEOUT / 2;

    if (s->timer)
        timer_del(s->timer);
    
    if (s->ptimer)
        timer_del(s->ptimer);
}

static void mtk_wdt_timer_expired(void *dev)
{
    MtkWdtState *s = MTK_WDT(dev);

    printf("Watchdog timer %" HWADDR_PRIx " expired.\n",
                  s->iomem.addr);

    watchdog_perform_action();
    timer_del(s->timer);
    timer_del(s->ptimer);
}

static void mtk_wdt_timer_interrupt(void *dev)
{
    MtkWdtState *s = MTK_WDT(dev);

    printf("Sending interrupt on watchdog timer %" HWADDR_PRIx ".\n", s->iomem.addr);

    qemu_set_irq(s->irq, 1);
}

static const MemoryRegionOps mtk_wdt_ops = {
    .read = mtk_wdt_read,
    .write = mtk_wdt_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
    .valid.unaligned = false,
};

static void mtk_wdt_realize(DeviceState *dev, Error **err)
{
    MtkWdtState *s = MTK_WDT(dev);

    mtk_wdt_reset(dev);
    s->timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, mtk_wdt_timer_expired, dev);
    s->ptimer = timer_new_ns(QEMU_CLOCK_VIRTUAL, mtk_wdt_timer_interrupt, dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &mtk_wdt_ops, s,
                          TYPE_MTK_WDT, WDT_MTK_MMIO_SIZE);
    
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);
    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq);
}

static const VMStateDescription vmstate_mtk_wdt = {
    .name = "vmstate_mtk_wdt",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (const VMStateField[]) {
        VMSTATE_TIMER_PTR(timer, MtkWdtState),
        VMSTATE_TIMER_PTR(ptimer, MtkWdtState),
        VMSTATE_UINT32_ARRAY(regs, MtkWdtState, WDT_MTK_MAX_REGS),
        VMSTATE_UINT32(pretimeout, MtkWdtState),
        VMSTATE_END_OF_LIST()
    }
};

static void mtk_wdt_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "MTK Watchdog controller";
    dc->realize = mtk_wdt_realize;
    device_class_set_legacy_reset(dc, mtk_wdt_reset);
    set_bit(DEVICE_CATEGORY_WATCHDOG, dc->categories);
    dc->vmsd = &vmstate_mtk_wdt;
}

static const TypeInfo mtk_wdt_info = {
    .parent = TYPE_SYS_BUS_DEVICE,
    .name  = TYPE_MTK_WDT,
    .instance_size  = sizeof(MtkWdtState),
    .class_init = mtk_wdt_class_init,
};

static void wdt_mtk_register_types(void)
{
    type_register_static(&mtk_wdt_info);
}

type_init(wdt_mtk_register_types)