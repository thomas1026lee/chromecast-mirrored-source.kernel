include $(src)/mv88de3100.mk

# TODO(kolla): Remove this hack once Marvell resolves the
# optimization issue with kernel modules. b/8081260
# Marvell modules crash the CPU when -O2 is enabled, so
# override -O2 for these modules with -Os.
subdir-ccflags-y := -Os

obj-y := core.o irq.o time.o platform_devs.o clock.o mv88de31xx_android.o

ifndef CONFIG_MV88DE3100_GPIO
ifdef CONFIG_MV88DE3100_SDHC
mv88de3100-y += modules/gpio/gpio.o
endif
endif

ifdef CONFIG_MV88DE3100_PXA3XX_NFC
mv88de3100-y += hal_dhub.o
else
ifdef CONFIG_MV88DE3100_PE_MODULE
mv88de3100-y += hal_dhub.o
endif
endif
ifdef mv88de3100-y
obj-y += mv88de3100.o
endif

obj-$(CONFIG_MV88DE3100_PINMUX) += pinmux.o
obj-$(CONFIG_MV88DE3100_ETHERNET) += geth.o
obj-$(CONFIG_SMP) += platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU) += hotplug.o
obj-$(CONFIG_CPU_FREQ) += cpufreq.o
obj-$(CONFIG_LOCAL_TIMERS) += localtimer.o
obj-$(CONFIG_MV88DE3100_SM) += sm.o
obj-$(CONFIG_MV88DE3100_PXA3XX_NFC) += pxa3xx_nand.o
pxa3xx_nand-y := pxa3xx_nand_debu.o
pxa3xx_nand-$(CONFIG_MV88DE3100_NAND_RANDOMIZER) += prbs15.o nand_randomizer.o
obj-$(CONFIG_SUSPEND) += pm.o sleep.o
obj-$(CONFIG_MV88DE3100_USB_EHCI_HCD) += plat_usb.o
obj-$(CONFIG_MV88DE3100_SDHC) += plat_mmc.o
obj-$(CONFIG_MV88DE3100_IR) += modules/ir/
obj-$(CONFIG_MV88DE3100_SPI) += modules/spi/
obj-$(CONFIG_MV88DE3100_I2C) += modules/i2c/
obj-$(CONFIG_MV88DE3100_SDIO_WLAN_8782) += modules/wlan_sd8782/
obj-$(CONFIG_MV88DE3100_SDIO_WLAN_8787) += modules/wlan_sd8787/
obj-$(CONFIG_MV88DE3100_SDIO_BT_8787) += modules/bt_sd8787/
obj-$(CONFIG_MV88DE3100_SDIO_WLAN_8786) += modules/wlan_sd8786/
obj-$(CONFIG_MV88DE3100_SDIO_WLAN_8797) += modules/wlan_sd8797/
obj-$(CONFIG_MV88DE3100_USB_WLAN_8797) += modules/wlan_usb8797/
obj-$(CONFIG_MV88DE3100_USB_WLAN_8782) += modules/wlan_usb8782/
obj-$(CONFIG_MV88DE3100_SHM) += modules/shm/
obj-$(CONFIG_MV88DE3100_CC) += modules/cc/
obj-$(CONFIG_MV88DE3100_PE_MODULE) += modules/pe/
obj-$(CONFIG_MV88DE3100_GPU) += modules/gpu/
obj-$(CONFIG_MV88DE3100_GPU3D) += modules/gpu3D/
obj-$(CONFIG_MV88DE3100_PWM) += modules/pwm/
obj-$(CONFIG_MV88DE3100_GPIO) += modules/gpio/
obj-$(CONFIG_MV88DE3100_FUSION) += modules/fusion/
obj-$(CONFIG_MV88DE3100_FASTLOGO) += modules/fastlogo/

CFLAGS_plat_usb.o += -I$(srctree)/arch/arm/mach-mv88de3100/include/platform_setting/$(PLATFORM)
CFLAGS_plat_mmc.o += -I$(srctree)/arch/arm/mach-mv88de3100/include/platform_setting/$(PLATFORM)
