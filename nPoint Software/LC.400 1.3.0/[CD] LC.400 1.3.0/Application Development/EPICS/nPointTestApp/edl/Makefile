TOP=../..
# include $(TOP)/configure/CONFIG

# DATA += nPointTop.edl Wavedata.edl DigitalIO.edl

ARCHTYPE = $(shell echo ${EPICS_HOST_ARCH} | cut -c1-5)

install:
      ifeq ($(ARCHTYPE), linux)
	mkdir -p $(TOP)/data
	cp -f $(TOP)/nPointTestApp/edl/nPointTop.edl $(TOP)/data
	cp -f $(TOP)/nPointTestApp/edl/Wavedata.edl $(TOP)/data
	cp -f $(TOP)/nPointTestApp/edl/DigitalIO.edl $(TOP)/data
      endif
clean::
	rm -rf $(TOP)/data

# include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

