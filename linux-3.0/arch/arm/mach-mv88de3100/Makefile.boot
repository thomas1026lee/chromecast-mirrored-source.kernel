__ZRELADDR	:= $(shell /bin/bash -c 'printf "0x%08x" \
		     $$[$(CONFIG_MV88DE3100_CPU0MEM_START) + 0x8000]')
zreladdr-y	:= $(__ZRELADDR)

params_phys-y	:= 0x00000100
initrd_phys-y	:= 0x00800000
