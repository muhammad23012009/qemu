#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qapi/qmp/qlist.h"
#include "qemu/error-report.h"
#include "qemu/module.h"
#include "qemu/units.h"
#include "exec/address-spaces.h"
#include "hw/boards.h"
#include "hw/char/serial-mm.h"
#include "hw/qdev-properties.h"
#include "hw/arm/mt6765.h"
#include "hw/arm/boot.h"
#include "target/arm/gtimer.h"
#include "sysemu/sysemu.h"

// SoC specific code

#define SBSA_GTIMER_HZ 26000000

const hwaddr mt6765_memmap[] = {
    [MT6765_GIC_DIST] = 0x0c000000,
    [MT6765_GIC_REDIST] = 0x0c100000,
    [MT6765_WDT] = 0x10007000,
    [MT6765_UART0] = 0x11002000,
    [MT6765_UART1] = 0x11003000,
    [MT6765_SDRAM] = 0x40000000
};

static void mt6765_init(Object *obj)
{
    MT6765State *s = MT6765(obj);

    s->memmap = mt6765_memmap;

    for (int i = 0; i < MT6765_NCPUS; i++) {
        object_initialize_child(obj, "cpu[*]", &s->cpus[i], ARM_CPU_TYPE_NAME("cortex-a53"));
    }

    object_initialize_child(obj, "gic", &s->gic, gicv3_class_name());

    object_initialize_child(obj, "wdt", &s->wdt, TYPE_MTK_WDT);
}

static void mt6765_realize(DeviceState *dev, Error **err)
{
    MT6765State *s = MT6765(dev);
    QList *redist_region_count;

    /* CPUs */
    for (int i = 0; i < MT6765_NCPUS; i++) {
        object_property_set_int(OBJECT(&s->cpus[i]), "cntfrq", SBSA_GTIMER_HZ, &error_abort);

        qdev_realize(DEVICE(&s->cpus[i]), NULL, &error_fatal);
    }

    /* GIC */
    qdev_prop_set_uint32(DEVICE(&s->gic), "num-irq", GIC_IRQ_NUM + GIC_INTERNAL);
    qdev_prop_set_uint32(DEVICE(&s->gic), "revision", 3);
    qdev_prop_set_uint32(DEVICE(&s->gic), "num-cpu", MT6765_NCPUS);
    redist_region_count = qlist_new();
    qlist_append_int(redist_region_count, MT6765_NCPUS);
    qdev_prop_set_array(DEVICE(&s->gic), "redist-region-count", redist_region_count);
    sysbus_realize(SYS_BUS_DEVICE(&s->gic), &error_fatal);

    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gic), 0, s->memmap[MT6765_GIC_DIST]);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gic), 1, s->memmap[MT6765_GIC_REDIST]);

    for (int i = 0; i < MT6765_NCPUS; i++) {
        DeviceState *cpu = DEVICE(&s->cpus[i]);
        int ppibase = GIC_IRQ_NUM + i * GIC_INTERNAL + GIC_NR_SGIS;

        const int timer_irq[] = {
            [GTIMER_PHYS] = 14,
            [GTIMER_VIRT] = 11,
            [GTIMER_HYP]  = 10,
            [GTIMER_SEC]  = 13,
        };

        for (int j = 0; j < ARRAY_SIZE(timer_irq); j++) {
            qdev_connect_gpio_out(cpu, j, qdev_get_gpio_in(DEVICE(&s->gic), ppibase + timer_irq[j]));
        }

        qemu_irq irq = qdev_get_gpio_in(DEVICE(&s->gic),
                                        ppibase + ARCH_GIC_MAINT_IRQ);
        qdev_connect_gpio_out_named(cpu, "gicv3-maintenance-interrupt",
                                    0, irq);
        qdev_connect_gpio_out_named(cpu, "pmu-interrupt", 0,
                qdev_get_gpio_in(DEVICE(&s->gic), ppibase + VIRTUAL_PMU_IRQ));

        sysbus_connect_irq(SYS_BUS_DEVICE(&s->gic), i, qdev_get_gpio_in(cpu, ARM_CPU_IRQ));

        // TODO: Not sure if these are correct
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->gic), i + MT6765_NCPUS,
                           qdev_get_gpio_in(cpu, ARM_CPU_FIQ));

        sysbus_connect_irq(SYS_BUS_DEVICE(&s->gic), i + 2 * MT6765_NCPUS,
                           qdev_get_gpio_in(cpu, ARM_CPU_VIRQ));

        sysbus_connect_irq(SYS_BUS_DEVICE(&s->gic), i + 3 * MT6765_NCPUS,
                           qdev_get_gpio_in(cpu, ARM_CPU_VFIQ));
    }

    /* Timer */
    // TODO: Find some implementation for armv8-timer

    /* UART */
    // TODO: Doesn't seem to work, do I need to implement serial_mtk?
    serial_mm_init(get_system_memory(), s->memmap[MT6765_UART0], 2,
                   qdev_get_gpio_in(DEVICE(&s->gic), MT6765_GIC_SPI_UART0),
                   115200, serial_hd(0), DEVICE_NATIVE_ENDIAN);

    serial_mm_init(get_system_memory(), s->memmap[MT6765_UART1], 2,
                   qdev_get_gpio_in(DEVICE(&s->gic), MT6765_GIC_SPI_UART1),
                   115200, serial_hd(1), DEVICE_NATIVE_ENDIAN);

    /* WDT */
    sysbus_realize(SYS_BUS_DEVICE(&s->wdt), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->wdt), 0, s->memmap[MT6765_WDT]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->wdt), 0, qdev_get_gpio_in(DEVICE(&s->gic), MT6765_GIC_SPI_WDT));
}

static void mt6765_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = mt6765_realize;
    dc->user_creatable = false;
}

static const TypeInfo mt6765_type_info = {
    .name = TYPE_MT6765,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(MT6765State),
    .instance_init = mt6765_init,
    .class_init = mt6765_class_init,
};

static void mt6765_register_types(void)
{
    type_register_static(&mt6765_type_info);
}

type_init(mt6765_register_types)


// Machine specific code

static struct arm_boot_info mt6765_boot_info;

static void mt6765_mach_init(MachineState *machine)
{
    MT6765State *state;

    state = MT6765(object_new(TYPE_MT6765));
    object_property_add_child(OBJECT(machine), "soc", OBJECT(state));
    object_unref(OBJECT(state));

    qdev_realize(DEVICE(state), NULL, &error_abort);

    /* SDRAM */
    memory_region_add_subregion(get_system_memory(), state->memmap[MT6765_SDRAM],
                                machine->ram);
    
    mt6765_boot_info.loader_start = state->memmap[MT6765_SDRAM];
    mt6765_boot_info.ram_size = machine->ram_size;
    mt6765_boot_info.psci_conduit = QEMU_PSCI_CONDUIT_SMC;
    arm_load_kernel(&state->cpus[0], machine, &mt6765_boot_info);
}

static void mt6765_machine_init(MachineClass *mc)
{
    static const char * const valid_cpu_types[] = {
        ARM_CPU_TYPE_NAME("cortex-a53"),
        NULL
    };

    mc->desc = "MT6765 test platform";
    mc->init = mt6765_mach_init;
    mc->min_cpus = MT6765_NCPUS;
    mc->max_cpus = MT6765_NCPUS;
    mc->default_cpus = MT6765_NCPUS;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-a53");
    mc->valid_cpu_types = valid_cpu_types;
    mc->default_ram_size = 2 * GiB;
    mc->default_ram_id = "ram";
}

DEFINE_MACHINE("mt6765", mt6765_machine_init)