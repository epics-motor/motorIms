TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# The following are used for debugging messages.
#!USR_CXXFLAGS += -DDEBUG

#=============================
# Build the IOC application

PROD_IOC = ims
# ims.dbd will be created and installed
DBD += ims.dbd

# ims.dbd will be made up from these files:
ims_DBD += base.dbd

# Include dbd files from all support applications:
#ifdef ASYN
ims_DBD += asyn.dbd
ims_DBD += drvAsynSerialPort.dbd
ims_DBD += drvAsynIPPort.dbd
#endif
ims_DBD += motorSupport.dbd
ims_DBD += devImsMotor.dbd

# Add all the support libraries needed by this IOC
ims_LIBS += Ims
ims_LIBS += motor
#ifdef ASYN
ims_LIBS += asyn
#endif
#ifdef SNCSEQ
ims_LIBS += seq pv
#endif

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

