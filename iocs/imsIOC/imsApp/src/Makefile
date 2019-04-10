TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = ims
# ims.dbd will be created and installed
DBD += ims.dbd

# ims.dbd will be made up from these files:
ims_DBD += base.dbd

# Include dbd files from all support applications:
#ims_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#ims_LIBS += xxx

# ims_registerRecordDeviceDriver.cpp derives from ims.dbd
ims_SRCS += ims_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
ims_SRCS_DEFAULT += imsMain.cpp
ims_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#ims_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
ims_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

