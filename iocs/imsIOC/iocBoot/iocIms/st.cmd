#!../../bin/linux-x86_64/ims

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/ims.dbd"
ims_registerRecordDeviceDriver pdbbase

cd "${TOP}/iocBoot/${IOC}"

## motorUtil (allstop & alldone)
dbLoadRecords("$(MOTOR)/db/motorUtil.db", "P=ims:")

##
< IM483.cmd
< MDrive.cmd

iocInit

## motorUtil (allstop & alldone)
motorUtilInit("ims:")

# Boot complete
