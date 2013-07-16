# Add your debugging flag (or not) to CFLAGS
ifeq ($(CONFIG_MV88DE3100_CC_DEBUG),y)
  DEBFLAGS = -O -g -DENABLE_DEBUG -DENABLE_DEBUG_OSAL # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

ccflags-y += $(DEBFLAGS)


obj-$(CONFIG_MV88DE3100_CC) += cc.o

cc-objs := cc_driver.o cc_std_map.o cc_list.o cc_cbuf_body.o cc_cbuf_server.o \
                    cc_dss.o cc_udp.o cc_msgq.o
