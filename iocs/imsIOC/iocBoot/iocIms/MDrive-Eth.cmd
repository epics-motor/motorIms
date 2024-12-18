
drvAsynIPPortConfigure("P1", "192.168.1.51:503", 0, 0, 0)
ImsMDrivePlusCreateController("M1","P1","",100,1000)

dbLoadRecords("../../db/motor.db")
dbLoadRecords("../../db/IMS_extra.db","P=Test-CT,R={Slt:1-Ax:T},PORT=M1,ADDR=0,TIMEOUT=1.0")
dbLoadRecords("../../db/asynRecord.db","P=Test-CT,R={MC:1}Asyn,PORT=P1,ADDR=80,OMAX=100,IMAX=100")
