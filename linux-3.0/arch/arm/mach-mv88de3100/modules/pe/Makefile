include $(srctree)/arch/arm/mach-mv88de3100/mv88de3100.mk

HEADER_PATH :=  \
	-I$(srctree)/arch/arm/mach-mv88de3100/modules/shm \
	-I$(srctree)/arch/arm/mach-mv88de3100/modules/cc  \
	-I$(srctree)/arch/arm/mach-mv88de3100/modules/pe/gsinc \
	-I$(srctree)/arch/arm/mach-mv88de3100/modules/pe/gsinc/$(FIRMWARE)

ccflags-y += $(MV_DEFINE) $(HEADER_PATH) -DBERLIN_SINGLE_CPU

obj-$(CONFIG_MV88DE3100_PE_MODULE) += galois_pe.o
galois_pe-objs := pe_driver.o avio_dhub_drv.o pe_agent_driver.o

