
drvAsynSerialPortConfigure("serial1", "/dev/ttyS0", 0, 0, 0)

dbLoadTemplate("IM483.substitutions")

# IMS IM483 driver setup parameters:
#     (1) maximum number of controllers in system
#     (2) motor task polling rate (min=1Hz,max=60Hz)
#  SM - single mode     PL - party mode
IM483SMSetup(1, 10)
#!IM483PLSetup(1, 10)

# IMS IM483 configuration parameters:
#     (1) controller# (chain#) being configured,
#     (2) ASYN port name
#  SM - single mode     PL - party mode
IM483SMConfig(0, "serial1")
#!var drvIM483SMdebug 4
#!IM483PLConfig(0, "L0")
#!var drvIM483PLdebug 4
