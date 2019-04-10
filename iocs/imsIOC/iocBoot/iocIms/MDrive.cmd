
drvAsynSerialPortConfigure("serial2", "/dev/ttyS1", 0, 0, 0)

dbLoadTemplate("MDrive.substitutions")

# IMS MDrive driver setup parameters: 
#     (1) maximum number of controllers in system
#     (2) motor task polling rate (min=1Hz,max=60Hz)
MDriveSetup(1, 10)

# IMS MDrive driver configuration parameters: 
#     (1) controller# being configured,
#     (2) ASYN port name
MDriveConfig(0, "serial2")
#!var drvMDrivedebug 4
