#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/sysbus.h"
#include "hw/timer/mtk_systimer.h"

static void mtk_systimer_cb(void *opaque)
{
    MtkSystimerState *s = MTK_SYSTIMER(opaque);

    printf("MTK Systimer: Timer expired!");
    qemu_irq_raise(s->irq);
}

static uint64_t mtk_systimer_read(void *opaque, hwaddr offset,
                                  unsigned int size)
{
    printf("Tried to read offset %lx\n", offset);
    return 0;
}

static void mtk_systimer_write(void *opaque, hwaddr offset,
                               uint64_t value, unsigned int size)
{
    MtkSystimerState *s = MTK_SYSTIMER(opaque);

    printf("Writing %lx with %ld\n", offset, value);
    switch (offset) {
        case SYST_CLOCK: {
            bool enabled = value & CLOCK_ENABLE;
            if (enabled && !s->enabled) {
                // Timer will only start countdown once kernelspace writes ticks
                s->enabled = enabled;
                printf("MTK Systimer: Requested to enable timer (waiting for ticks)\n");
            } else if (!enabled && s->enabled) {
                // Stop timer and clear everything
                if (s->timer)
                    timer_del(s->timer);
                
                s->enabled = false;

                printf("MTK Systimer: Disabling timer\n");
            }

            if (value & CLOCK_IRQ_CLEAR) {
                printf("MTK Systimer: Clearing IRQ\n");
                qemu_irq_lower(s->irq);
            }
            break;
        }

        case SYST_TICKS: {
            if (s->enabled) {
                printf("MTK Systimer: Requested to start timer with %ld ticks\n", value);
                timer_mod(s->timer, value);
            }
            break;
        }
    }
}

static const MemoryRegionOps mtk_systimer_ops = {
    .read = mtk_systimer_read,
    .write = mtk_systimer_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void mtk_systimer_init(Object *obj)
{
    MtkSystimerState *s = MTK_SYSTIMER(obj);

    s->timer = timer_new(QEMU_CLOCK_VIRTUAL, NANOSECONDS_PER_SECOND / 13000000, mtk_systimer_cb, s);

    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->iomem, obj, &mtk_systimer_ops, s,
                          TYPE_MTK_SYSTIMER, 0x1000);
    
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
}

static void mtk_systimer_realize(DeviceState *dev, Error **errp)
{
    MtkSystimerState *s = MTK_SYSTIMER(dev);

    qdev_init_gpio_out(dev, &s->irq, 1);
}


static void mtk_systimer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    //device_class_set_legacy_reset(dc, ibex_timer_reset);
    //dc->vmsd = &vmstate_ibex_timer;
    dc->realize = mtk_systimer_realize;
}

static const TypeInfo mtk_systimer_info = {
    .name          = TYPE_MTK_SYSTIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(MtkSystimerState),
    .instance_init = mtk_systimer_init,
    .class_init    = mtk_systimer_class_init,
};

static void mtk_systimer_register_types(void)
{
    type_register_static(&mtk_systimer_info);
}

type_init(mtk_systimer_register_types)