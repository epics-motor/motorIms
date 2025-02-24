//! @File : ImsMDrivePlusController.cpp
//!         Motor record driver level support for Intelligent Motion Systems, Inc.
//!         MDrivePlus series; M17, M23, M34.
//!	      Simple implementation using "model 3" asynMotorController and asynMotorAxis base classes (derived from asynPortDriver)
//!
//!  Original Author : Nia Fong
//!  Date : 11-21-2011
//!  Current Author : Mitch D'Ewart (SLAC)
//!
//!  Assumptions :
//!    1) Like all controllers, the MDrivePlus must be powered-on when EPICS is first booted up.
//!    2) Assume single controller-card-axis relationship; 1 controller = 1 axis.  
//!    3) Append Line Feed (ctrl-J) to end of command string for party mode support
//!    4) If not using device name to address device, config ImsMDrivePlusCreateController() with empty string "" for deviceName
//
//  Revision History
//  ----------------
//  11-21-2011  NF Initial version


#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <exception>
#include <epicsThread.h>
#include <iocsh.h>
#include <asynOctetSyncIO.h>
#include <asynOctet.h>

#include "asynMotorController.h"
#include "asynMotorAxis.h"
#include "ImsMDrivePlusMotorAxis.h"

#include <epicsExport.h>
#include "ImsMDrivePlusMotorController.h"

////////////////////////////////////////////////////////
//! @ImsMDrivePlusMotorController()
//! Constructor
//! Driver assumes only 1 axis configured per controller for now...
//!
//! @param[in] motorPortName     Name assigned to the port created to communicate with the motor
//! @param[in] IOPortName        Name assigned to the asyn IO port, name that was assigned in drvAsynIPPortConfigure()
//! @param[in] devName           Name of device (DN) assigned to motor axis in MCode, the device name is prepended to the MCode command to support Party Mode (PY) multidrop communication setup
//!                              set to empty string "" if no device name needed/not using Party Mode
//! @param[in] movingPollPeriod  Moving polling period in milliseconds
//! @param[in] idlePollPeriod    Idle polling period in milliseconds
////////////////////////////////////////////////////////
ImsMDrivePlusMotorController::ImsMDrivePlusMotorController(const char *motorPortName, const char *IOPortName, const char *devName, double movingPollPeriod, double idlePollPeriod)
    : asynMotorController(motorPortName, NUM_AXES, NUM_IMS_PARAMS,
						  asynInt32Mask | asynFloat64Mask | asynUInt32DigitalMask,
						  asynInt32Mask | asynFloat64Mask | asynUInt32DigitalMask,
						  ASYN_CANBLOCK | ASYN_MULTIDEVICE,
						  1, // autoconnect
						  0, 0),  // Default priority and stack size
    pAsynUserIMS(0)
{
	static const char *functionName = "ImsMDrivePlusMotorController()";
	asynStatus status;
	// asynMotorController constructor calloc's memory for array of axis pointers
	pAxes_ = (ImsMDrivePlusMotorAxis **)(asynMotorController::pAxes_);

	// copy names
	strncpy(motorName, motorPortName, (MAX_NAME_LEN - 1));

	// setup communication
	status = pasynOctetSyncIO->connect(IOPortName, 0, &pAsynUserIMS, NULL);
	if (status != asynSuccess) {
		printf("\n\n%s:%s: ERROR connecting to Controller's IO port=%s\n\n", DRIVER_NAME, functionName, IOPortName);
		// TODO would be good to implement exceptions
		// TODO THROW_(SmarActMCSException(MCSConnectionError, "SmarActMCSController: unable to connect serial channel"));
	}

	// write version, cannot use asynPrint() in constructor since controller (motorPortName) hasn't been created yet
	printf("%s:%s: motorPortName=%s, IOPortName=%s, devName=%s \n", DRIVER_NAME, functionName, motorPortName, IOPortName, devName);


	// init
	pasynOctetSyncIO->setInputEos(pAsynUserIMS, "\n", 1);
	pasynOctetSyncIO->setOutputEos(pAsynUserIMS, "\r\n", 2); 

	// Create controller-specific parameters
	createParam(ImsMDrivePlusSaveToNVMControlString, asynParamInt32, &ImsMDrivePlusSaveToNVM_);
	createParam(ImsMDrivePlusLoadMCodeControlString, asynParamOctet, &this->ImsMDrivePlusLoadMCode_);
	createParam(ImsMDrivePlusClearMCodeControlString, asynParamOctet, &this->ImsMDrivePlusClearMCode_);

	// Check the validity of the arguments and init controller object
	initController(devName, movingPollPeriod, idlePollPeriod);

	// Create axis
	// Assuming single axis per controller the way drvAsynIPPortConfigure( "M06", "ts-b34-nw08:2101", 0, 0 0 ) is called in st.cmd script
	new ImsMDrivePlusMotorAxis(this, 0);

	// read home and limit config from S1-S4
	readHomeAndLimitConfig();

	startPoller(movingPollPeriod, idlePollPeriod, 2);
}

////////////////////////////////////////
//! initController()
//! config controller variables - axis, etc.
//! only support single axis per controller
//
//! @param[in] devName Name of device (DN) used to identify axis within controller for Party Mode
//! @param[in] movingPollPeriod  Moving polling period in milliseconds
//! @param[in] idlePollPeriod    Idle polling period in milliseconds
////////////////////////////////////////
void ImsMDrivePlusMotorController::initController(const char *devName, double movingPollPeriod, double idlePollPeriod)
{
	strncpy(this->deviceName, devName, (MAX_NAME_LEN - 1));

	// initialize asynMotorController variables
	this->numAxes_ = NUM_AXES;  // only support single axis
	this->movingPollPeriod_ = movingPollPeriod;
	this->idlePollPeriod_ = idlePollPeriod;

	// initialize switch inputs
	this->homeSwitchInput=-1;
	this->posLimitSwitchInput=-1;
	this->negLimitSwitchInput=-1;

	// flush io buffer
	pasynOctetSyncIO->flush(pAsynUserIMS);
}

void ImsMDrivePlusMotorController::set_switch_vars(int type, int setto)
{
	switch (type) {
	case 0: // general purpose input
		break;
	case 1: // home switch input
		homeSwitchInput = setto;
		printf( "Setup type %d found: HOME limit switch input line is set to %d\n", type, homeSwitchInput );
		break;
	case 2: // positive limit switch input
		posLimitSwitchInput = setto;
		printf( "Setup type %d found: POSITIVE limit switch input line is set to %d\n", type, posLimitSwitchInput );
		break;
	case 3: // negative limit switch input
		negLimitSwitchInput = setto;
		printf( "Setup type %d found: NEGATIVE limit switch input line is set to %d\n", type, negLimitSwitchInput );
		break;
	default:
		break;
	}
}


////////////////////////////////////////
//! readHomeAndLimitConfig
//! read home, positive limit, and neg limit switch configuration from MCode S1-S4 settings or Lexium IS
//! S1-S4 or IS must be set up beforehand
//! I1-I4 are used to read the status of S1-S4
//  Use logic from existing drvMDrive.cc
////////////////////////////////////////
int ImsMDrivePlusMotorController::readHomeAndLimitConfig()
{
	asynStatus status = asynError;
	char cmd[MAX_CMD_LEN];
	char resp[1024];
	size_t nread, nwrite;
	int type, eomReason;
	asynInterface *pasynInterface;
	asynOctet *pasynOctet;

	resp[0] = 0;
	sprintf(cmd, "PR VR"); // get version
	// ignoring status of writeReadController here, since old MForce 1
	// responds with error for some reason, whereas MForce 2 / LMM / LMD responds correctly.
	this->writeReadController(cmd, resp, sizeof(resp), &nread, IMS_TIMEOUT);

	if (strlen(resp) > 0)
		printf("Controller version %s\n", resp);

	printf( "Setup config:\n" );

	if (strstr(resp, "lmm") || strstr(resp, "LMM"))
	{
		// LMM controller
		printf("Lexium Motor Module detected\n" );
		printf("-------------------------------------------\n");
	}
	else if (strstr(resp, "lmd") || strstr(resp, "LMD"))
	{
		// LMD motor - different terminating characters
		printf("Lexium MDrive detected\n");
		printf("-------------------------------------------\n");
		if (this->deviceName[0]=='\0')
		{
			printf("Have no device name, line ending CR.\n");
			pasynOctetSyncIO->setOutputEos(pAsynUserIMS, "\r", 1);
		}
		else
		{
			printf("Have device name going party modus, line ending LF.\n");
			pasynOctetSyncIO->setOutputEos(pAsynUserIMS, "\n", 1);
		}
	}
	else
	{
		// MForce 1 (IMS) controller
		printf("MForce 1 driver detected\n");
		printf("-------------------------------------------\n");
		printf("Checking setup for home, pos and neg limit switches:\n");
		// iterate through S1-S4 and parse each configuration to see if home, pos, and neg limits are set
		for (int i=1; i<=4; i++) {
			sprintf(cmd, "PR S%d", i); // query S1-S4 setting
			status = this->writeReadController(cmd, resp, sizeof(resp), &nread, IMS_TIMEOUT);
			printf("%s\n", resp);
			sscanf(resp, "%4d[^,]", &type);
			set_switch_vars(type, i);
		}
		goto end;
	}

	// Any Lexium from here on: fetch home/limit switch configuration

	// fetch unlocked base octet interface and do low level I/O with different EOS
	// see <https://epics.anl.gov/tech-talk/2024/msg01188.php> for reason of this
	pasynInterface = pasynManager->findInterface(pAsynUserIMS,asynOctetType,1);
	pasynOctet = (asynOctet *)pasynInterface->pinterface;
	pAsynUserIMS->timeout = IMS_TIMEOUT;

	pasynManager->lockPort(pAsynUserIMS);
	pasynOctet->setInputEos(pasynInterface->drvPvt, pAsynUserIMS, "\0", 1);
	epicsSnprintf(cmd, sizeof(cmd), "%sPR IS", deviceName);
	cmd[sizeof(cmd) - 1] = '\0';
	resp[0] = 0;
	nread = nwrite = 0;
	status = pasynOctet->write(pasynInterface->drvPvt, pAsynUserIMS, cmd, strlen(cmd), &nwrite);
	eomReason = 0;
	status = pasynOctet->read(pasynInterface->drvPvt, pAsynUserIMS, resp, sizeof(resp), &nread, &eomReason);
	pasynOctet->setInputEos(pasynInterface->drvPvt, pAsynUserIMS, "\n", 1);
	pasynManager->unlockPort(pAsynUserIMS);
	printf("%s\n", resp);

	// quick and dirty solution:
	// kill all nondigit chars, break into separate strings on LF, scan 3 params
	if (nread > 0) {
		char *start = resp;
		for (size_t i = 0; i < nread; i++)
			if (!isdigit(resp[i])) {
				if (resp[i] == '\n') {
					int inputno, fn, act;
					resp[i] = 0;
					sscanf(start, "%d %d %d", &inputno, &fn, &act);
					//printf("got %d %d %d\n", inputno, fn, act);
					set_switch_vars(fn, inputno);
					start = resp + i + 1;
				}
				else
					resp[i] = ' ';
			}
	}

end:
	printf( "    HOME limit switch input line: %d\n", homeSwitchInput );
	printf( "POSITIVE limit switch input line: %d\n", posLimitSwitchInput );
	printf( "NEGATIVE limit switch input line: %d\n", negLimitSwitchInput );
	printf( "-------------------------------------------\n" );

	return status;
}

////////////////////////////////////////
//! getAxis()
//! Override asynMotorController function to return pointer to IMS axis object
//
//! Returns a pointer to an ImsMDrivePlusAxis object.
//! Returns NULL if the axis number encoded in pasynUser is invalid
//
//! param[in] pasynUser asynUser structure that encodes the axis index number
////////////////////////////////////////
ImsMDrivePlusMotorAxis* ImsMDrivePlusMotorController::getAxis(asynUser *pasynUser)
{
	int axisNo;

	getAddress(pasynUser, &axisNo);
	return getAxis(axisNo);
}

////////////////////////////////////////
//! getAxis()
//! Override asynMotorController function to return pointer to IMS axis object
//
//! Returns a pointer to an ImsMDrivePlusAxis object.
//! Returns NULL if the axis number is invalid.
//
//! param[in] axisNo Axis index number
////////////////////////////////////////
ImsMDrivePlusMotorAxis* ImsMDrivePlusMotorController::getAxis(int axisNo)
{
	if ((axisNo < 0) || (axisNo >= numAxes_)) return NULL;
	return pAxes_[axisNo];
}

////////////////////////////////////////
//! writeInt32()
//! Override asynMotorController function to add hooks to IMS records
// Based on XPSController.cpp
//
//! param[in] pointer to asynUser object
//! param[in] value to pass to function
////////////////////////////////////////
asynStatus ImsMDrivePlusMotorController::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	int reason = pasynUser->reason;
	int status = asynSuccess;
	ImsMDrivePlusMotorAxis *pAxis;
	static const char *functionName = "writeInt32";

	pAxis = this->getAxis(pasynUser);
	if (!pAxis) return asynError;

	asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s:%s: val=%d\n", DRIVER_NAME, functionName, value);

	// Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
	// status at the end, but that's OK
	status = pAxis->setIntegerParam(reason, value);

	if (reason == ImsMDrivePlusSaveToNVM_) {
		if (value == 1) { // save current user parameters to NVM
			status = pAxis->saveToNVM();
			if (status) asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: ERROR saving to NVM\n", DRIVER_NAME, functionName);
			else asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "%s:%s: Successfully saved to NVM\n", DRIVER_NAME, functionName);
		} else {
			asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: ERROR, value of 1 to save to NVM\n", DRIVER_NAME, functionName);
		}
	} else { // call base class method to continue handling
			status = asynMotorController::writeInt32(pasynUser, value);
	}

	callParamCallbacks(pAxis->axisNo_);
	return (asynStatus)status;
}

////////////////////////////////////////
//! writeController()
//! reference ACRMotorDriver
//
//! Writes a string to the IMS controller.
//! Prepends deviceName to command string, if party mode not enabled, set device name to ""
//! @param[in] output the string to be written.
//! @param[in] timeout Timeout before returning an error.
////////////////////////////////////////
asynStatus ImsMDrivePlusMotorController::writeController(const char *output, double timeout)
{
	size_t nwrite;
	asynStatus status;
	char outbuff[MAX_BUFF_LEN];
	static const char *functionName = "writeController()";

	// in party-mode Line Feed must follow command string
	sprintf(outbuff, "%s%s", deviceName, output);
	asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s:%s: deviceName=%s, command=%s\n", DRIVER_NAME, functionName, deviceName, outbuff);
	status = pasynOctetSyncIO->write(pAsynUserIMS, outbuff, strlen(outbuff), timeout, &nwrite);
	if (status) { // update comm flag
		setIntegerParam(this->motorStatusCommsError_, 1);
	}
	return status ;
}

////////////////////////////////////////
//! writeReadController()
//! reference ACRMotorDriver
//
//! Writes a string to the IMS controller and reads a response.
//! Prepends deviceName to command string, if party mode not enabled, set device name to ""
//! param[in] output Pointer to the output string.
//! param[out] input Pointer to the input string location.
//! param[in] maxChars Size of the input buffer.
//! param[out] nread Number of characters read.
//! param[out] timeout Timeout before returning an error.*/
////////////////////////////////////////
asynStatus ImsMDrivePlusMotorController::writeReadController(const char *output, char *input, size_t maxChars, size_t *nread, double timeout)
{
	size_t nwrite;
	asynStatus status;
	int eomReason;
	char outbuff[MAX_BUFF_LEN];
	static const char *functionName = "writeReadController()";

	// in party-mode Line Feed must follow command string
	sprintf(outbuff, "%s%s", deviceName, output);
	status = pasynOctetSyncIO->writeRead(pAsynUserIMS, outbuff, strlen(outbuff), input, maxChars, timeout, &nwrite, nread, &eomReason);
	if (status) { // update comm flag
		setIntegerParam(this->motorStatusCommsError_, 1);
	}
	asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s:%s: deviceName=%s, command=%s, response=%s\n", DRIVER_NAME, functionName, deviceName, outbuff, input);
	return status;
}

////////////////////////////////////////////////////////
// Start code for iocsh Registration :
// Available Functions :
//   ImsMDrivePlusCreateController()
////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
//! ImsMDrivePlusCreateController()
//! IOCSH function
//! Creates a new IMSMDrivePlusController object
//
//! Configuration command, called directly or from iocsh
//! @param[in] motorPortName     User-specific name of motor port
//! @param[in] IOPortName        User-specific name of port that was configured by drvAsynIPPortConfigure()
//! @param[in] deviceName        Name of device, used to address motor by MCODE in party mode
//                              If not using party mode, config ImsMDrivePlusCreateController() with empty string "" for deviceName
//! @param[in] movingPollPeriod  time in ms between polls when any axis is moving
//! @param[in] idlePollPeriod    time in ms between polls when no axis is moving
////////////////////////////////////////////////////////
extern "C" int ImsMDrivePlusCreateController(const char *motorPortName, const char *IOPortName, char *devName, double movingPollPeriod, double idlePollPeriod)
{
	new ImsMDrivePlusMotorController(motorPortName, IOPortName, devName, movingPollPeriod/1000., idlePollPeriod/1000.);
	return(asynSuccess);
}

////////////////////////////////////////////////////////
// ImsMDrivePlus IOCSH Registration
// Copied from ACRMotorDriver.cpp
//
// Motor port name    : user-specified name of port
// IO port name       : user-specific name of port that was initialized with drvAsynIPPortConfigure()
// Device name        : name of device, used to address motor by MCODE in party mode
//                    : if not using party mode, config ImsMDrivePlusCreateController() with empty string "" for deviceName
// Moving poll period : time in ms between polls when any axis is moving
// Idle poll period   : time in ms between polls when no axis is moving
////////////////////////////////////////////////////////
static const iocshArg ImsMDrivePlusCreateControllerArg0 = {"Motor port name", iocshArgString};
static const iocshArg ImsMDrivePlusCreateControllerArg1 = {"IO port name", iocshArgString};
static const iocshArg ImsMDrivePlusCreateControllerArg2 = {"Device name", iocshArgString};
static const iocshArg ImsMDrivePlusCreateControllerArg3 = {"Moving poll period (ms)", iocshArgDouble};
static const iocshArg ImsMDrivePlusCreateControllerArg4 = {"Idle poll period (ms)", iocshArgDouble};
static const iocshArg * const ImsMDrivePlusCreateControllerArgs[] = {&ImsMDrivePlusCreateControllerArg0,
                                                                     &ImsMDrivePlusCreateControllerArg1,
                                                                     &ImsMDrivePlusCreateControllerArg2,
                                                                     &ImsMDrivePlusCreateControllerArg3,
                                                                     &ImsMDrivePlusCreateControllerArg4};
static const iocshFuncDef ImsMDrivePlusCreateControllerDef = {"ImsMDrivePlusCreateController", 5, ImsMDrivePlusCreateControllerArgs};
static void ImsMDrivePlusCreateControllerCallFunc(const iocshArgBuf *args)
{
	ImsMDrivePlusCreateController(args[0].sval, args[1].sval, args[2].sval, args[3].dval, args[4].dval);
}

static void ImsMDrivePlusMotorRegister(void)
{
	iocshRegister(&ImsMDrivePlusCreateControllerDef, ImsMDrivePlusCreateControllerCallFunc);
}

extern "C" {
	epicsExportRegistrar(ImsMDrivePlusMotorRegister);
}
