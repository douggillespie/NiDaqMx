// demolib.cpp : Defines the entry point for the DLL application.
//

/*** File I/O ***/
//	ofstream pktSums;
//	pktSums.open("pktsums2.txt",ofstream::out | ofstream::app);
//		for ( z=0; z<sampsPerChanRead; z++ ) {
//						//dataSum+= buffer[writeBufIndex][z];
//						pktSums <<  doubleArrPtr[z] << "\n";
//		}
//	}
//	pktSums.flush();
//	pktSums.close();



#ifdef NILINUX
typedef char byte;
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#else
#include <windows.h>
#endif


#include "nidaqdev_Nidaq.h"
#include <NIDAQmx.h>
#include "dataBuffer.h"
//#include <iostream.h>
//#include <fstream.h>

//#include <stdlib.h>
//#include <cstdlib>
// includes often in c:\java\include
//cl -Iinclude -Iinclude\win32
//    -MD -LD demolib.c -Femylib.dll




#define DAQmxErrChk(functionCall) if(DAQmxFailed(error=(functionCall)) ) goto Error; else
//static int32 GetTerminalNameWithDevPrefix(TaskHandle taskHandle, const char terminalName[], char triggerName[]);
static int32 GetTerminalNameWithDevPrefix(TaskHandle taskHandle, const char terminalName[], char triggerName[])
{
	int32	error=0;
	char	device[256];
	int32	productCategory;
	uInt32	numDevices,i=1;

	DAQmxErrChk (DAQmxGetTaskNumDevices(taskHandle,&numDevices));
	while( i<=numDevices ) {
		DAQmxErrChk (DAQmxGetNthTaskDevice(taskHandle,i++,device,256));
		DAQmxErrChk (DAQmxGetDevProductCategory(device,&productCategory));
		if( productCategory!=DAQmx_Val_CSeriesModule && productCategory!=DAQmx_Val_SCXIModule ) {
			*triggerName++ = '/';
			strcat(strcat(strcpy(triggerName,device),"/"),terminalName);
			break;
		}
	}

Error:
	return error;
}


#define MAX_NI_CHANS 64
#define MAX_NI_TASKS 8
#define NIDEVNAMELEN 100
#define BUFFERBLOCKSIZE		100
#define DAQ_BUFFER_MULTIPLE	 50	//cout << "DarwinDataToUdpThread: EnterCriticalSection"  << endl << flush;
#define NUM_CHANNELS 1

double getNIGainFactor(int NIGainCode);
void reportToJava(JNIEnv *env, char* string);
jint reportError(JNIEnv *env, jint errorCode, char* str = NULL);
void resetPamguard(JNIEnv *env, int32 errorCode);
jint stopDaq(JNIEnv *env);

// I use these two
int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData);
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);
int32 CVICALLBACK EveryNPlayCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData);
int32 CVICALLBACK DonePlayCallback(TaskHandle taskHandle, int32 status, void *callbackData);

/**
 * create an array of NI device names which can be used in
 * calls to getDeviceName;
 */
char** createDeviceNames(int* nameListLength) {
	/*
	 * NI call to get a load of device names. To maintain backward compatibility
	 * with the Dev%d numbering system which started at 1, the first name
	 * will be null, i.e. the list will be 1 indexed.
	 */
#define ALLNAMESLEN 1024
	char allNames[ALLNAMESLEN];
	int ret  = DAQmxGetSysDevNames(allNames, ALLNAMESLEN);
	// count the commas in allNames ...	int nCommas = 0;
	char* p = allNames;
	int nCommas = 0;
	while (p) {
		p = strchr(p+1, ',');
		nCommas++;
	}
	char** nameList = (char**) calloc(sizeof(char*), nCommas+1);
	*nameListLength = nCommas+1;
	char* bit;
	for (int i = 0; i < nCommas; i++) {
		if (i == 0) {
			bit = strtok(allNames, ", ");
		}
		else {
			bit = strtok(NULL, ",  ");
		}
		if (bit == NULL) {
			//			printf("NULL bit\n");
			break;
		}
		nameList[i+1] = (char*) calloc(sizeof(char), strlen(bit)+1);
		strcpy(nameList[i+1], bit);
		//		printf(nameList[i+1]);
		//		printf("\n");
	}
	return nameList;

}

static 	char** niDeviceNames = NULL;
static int nameListLength = 0;
/**
 * Get a device name from a device index. NB
 * device 0 is NULL to maintain compatibility with old Dev%d naming system
 * This new system is designed to handle any type of device name, even those
 * which don't start with "Dev", i.e. C series modules.
 */
char* getDeviceName(int iDevice) {
	if (niDeviceNames == NULL) {
		niDeviceNames = createDeviceNames(&nameListLength);
	}
	if (niDeviceNames == NULL) {
		return NULL;
	}
	if (iDevice < 0 || iDevice >= nameListLength) {
		return NULL;
	}
	return niDeviceNames[iDevice];
}


// Not used
JNIEXPORT void JNICALL
Java_nidaqdev_Nidaq_helloworld(JNIEnv *env, jobject obj)
{
	printf("Method: Java_nidaqdev_Nidaq_helloworld.\n");
	return;
}

// Not used
JNIEXPORT jstring JNICALL
Java_nidaqdev_HelloWorld_getLine(JNIEnv *env, jobject obj, jstring prompt)
{
	char buf[128];
	// const jbyte *str;
	const char *str;
	str = env->GetStringUTFChars(prompt, NULL);
	//str = env->GetStringUTFChars(env, prompt, NULL);

	if (str == NULL) {
		return NULL; /* OutOfMemoryError already thrown */
	}
	printf("%s", str);


	//    const char *str = env->GetStringUTFChars(s, 0);
	//    ...
	//    env->ReleaseStringUTFChars(s, str);

	env->ReleaseStringUTFChars(prompt, str);
	/* We assume here that the user does not type more than
	 * 127 characters */
	scanf("%s", buf);
	return env->NewStringUTF(buf);
}

// Not used
//***************************************************** daqmxCreateTask
JNIEXPORT jint JNICALL
Java_nidaqdev_Nidaq_daqmxCreateTask(JNIEnv *env, jobject obj,
		jobject daqmxCreateTaskParamsObj)
{
	/*** NI DAQ - Configure Create Task ***/
	TaskHandle  taskHandle = (TaskHandle) NULL;
	const char	taskName[] = "PamGuard TaskName"; // Name assigned to the task (Create Task function)
	//const char deviceName[] = "Dev1";					// The names of the physical channels to use to create virtual channels (Create Channel function)
	int daqmxResult=0;

	// Declare class references
	jclass daqmxCreateTaskParamsCls;

	// Declare Java Method IDs
	jmethodID setTaskHandleMID;

	// Initialise the class
	daqmxCreateTaskParamsCls = env->GetObjectClass(daqmxCreateTaskParamsObj);

	// Initialise the Methods
	setTaskHandleMID = env->GetMethodID(daqmxCreateTaskParamsCls, "setTaskHandle","(I)V");
	if(setTaskHandleMID == NULL)
		return(88);


	printf("\n\n\n\n\n\n\n\n\n\n");
	printf("\n======================================\n");
	printf("Java_nidaqdev_Nidaq_daqmxCreateTask\n");
	printf("======================================\n");
	printf("Parameters before initialisation.\n");
	printf("Java_nidaqdev_Nidaq_daqmxCreateTask: taskHandle: %d\n", taskHandle);

	daqmxResult= DAQmxCreateTask(taskName, &taskHandle);
	env->CallVoidMethod(daqmxCreateTaskParamsObj, setTaskHandleMID, taskHandle);

	printf("---------------------\n");
	printf("Task %s created \n", taskName);
	printf("Java_nidaqdev_Nidaq_daqmxCreateTask:taskHandle: %d\n", taskHandle);

	// Set a dummy error
	daqmxResult = -201076;
	return(daqmxResult);
}



// Not used
//***************************************************** daqmxStartTask
/* DAQmxStartTask(
 * TaskHandle	taskHandle,  // The task to start.
 */
JNIEXPORT jint JNICALL
Java_nidaqdev_Nidaq_daqmxStartTask(JNIEnv *env, jobject obj,
		jobject daqmxStartTaskParamsObj){

	TaskHandle  taskHandle = (TaskHandle) NULL;
	int daqmxResult=0;

	// Declare class references
	jclass daqmxStartTaskParamsCls;

	// Declare Java Method IDs
	jmethodID getTaskHandleMID;

	// Initialise the class
	daqmxStartTaskParamsCls = env->GetObjectClass(daqmxStartTaskParamsObj);

	// Initialise the Methods
	getTaskHandleMID = env->GetMethodID(daqmxStartTaskParamsCls, "getTaskHandle", "()I");
	if(getTaskHandleMID == NULL)
		return(89);

	printf("\n");
	printf("==================================\n");
	printf("Java_nidaqdev_Nidaq_daqmxStartTask\n");
	printf("\n");
	printf("Parameters before retrieving over jni.\n");
	printf("C++Nidaq:taskHandle: %d\n", taskHandle);

	// Retrieve parameters over JNI.
	taskHandle = (void*)  env->CallIntMethod(daqmxStartTaskParamsObj, getTaskHandleMID, "");

	printf("\n");
	printf("Parameters after retrieving over jni.\n");
	printf("C++Nidaq:taskHandle: %d\n", taskHandle);

	// *****************
	// THE CALL TO DAQmx
	daqmxResult = DAQmxStartTask(taskHandle);

	// Return the DAQmx return value
	return(daqmxResult);
}

// Not used
//***************************************************** daqmxStopTask
/* DAQmxStopTask(
 * TaskHandle	taskHandle,  // The task to stop.
 */
JNIEXPORT jint JNICALL
Java_nidaqdev_Nidaq_daqmxStopTask(JNIEnv *env, jobject obj,
		jobject daqmxStopTaskParamsObj){

	TaskHandle  taskHandle = (TaskHandle) NULL;
	int daqmxResult=0;

	// Declare class references
	jclass daqmxStopTaskParamsCls;

	// Declare Java Method IDs
	jmethodID getTaskHandleMID;

	// Initialise the class
	daqmxStopTaskParamsCls = env->GetObjectClass(daqmxStopTaskParamsObj);

	// Initialise the Methods
	getTaskHandleMID = env->GetMethodID(daqmxStopTaskParamsCls, "getTaskHandle", "()I");
	if(getTaskHandleMID == NULL)
		return(89);

	printf("\n");
	printf("==================================\n");
	printf("Java_nidaqdev_Nidaq_daqmxStopTask\n");
	printf("\n");
	printf("Parameters before retrieving over jni.\n");
	printf("C++Nidaq:taskHandle: %d\n", taskHandle);

	// Retrieve parameters over JNI.
	taskHandle = (void*)  env->CallIntMethod(daqmxStopTaskParamsObj, getTaskHandleMID, "");

	printf("\n");
	printf("Parameters after retrieving over jni.\n");
	printf("C++Nidaq:taskHandle: %d\n", taskHandle);

	// *****************
	// THE CALL TO DAQmx
	daqmxResult = DAQmxStopTask(taskHandle);

	// Return the DAQmx return value
	return(daqmxResult);
}

// Not used
//***************************************************** daqmxClearTask
/* DAQmxClearTask(
 * TaskHandle	taskHandle,  // The task to clear. (stops task first)
 *                           // Then frees all resources
 */
JNIEXPORT jint JNICALL
Java_nidaqdev_Nidaq_daqmxClearTask(JNIEnv *env, jobject obj,
		jobject daqmxClearTaskParamsObj){

	TaskHandle  taskHandle = (TaskHandle) NULL;
	int daqmxResult=0;

	// Declare class references
	jclass daqmxClearTaskParamsCls;

	// Declare Java Method IDs
	jmethodID getTaskHandleMID;

	// Initialise the class
	daqmxClearTaskParamsCls = env->GetObjectClass(daqmxClearTaskParamsObj);

	// Initialise the Methods
	getTaskHandleMID = env->GetMethodID(daqmxClearTaskParamsCls, "getTaskHandle", "()I");
	if(getTaskHandleMID == NULL)
		return(89);

	printf("\n");
	printf("==================================\n");
	printf("Java_nidaqdev_Nidaq_daqmxClearTask\n");
	printf("\n");
	printf("Parameters before retrieving over jni.\n");
	printf("C++Nidaq:taskHandle: %d\n", taskHandle);

	// Retrieve parameters over JNI.
	taskHandle = (TaskHandle) env->CallIntMethod(daqmxClearTaskParamsObj, getTaskHandleMID, "");

	printf("\n");
	printf("Parameters after retrieving over jni.\n");
	printf("C++Nidaq:taskHandle: %d\n", taskHandle);

	// *****************
	// THE CALL TO DAQmx
	daqmxResult = DAQmxClearTask(taskHandle);

	// Return the DAQmx return value
	return(daqmxResult);
}


// Not used
//***************************************************** daqmxGetDevIsSimulated

JNIEXPORT jint JNICALL
Java_nidaqdev_Nidaq_daqmxGetDevIsSimulated(JNIEnv *env, jobject obj,
		jobject daqmxGetDevIsSimulatedObj){
	int daqmxResult=0;
	bool32 dev_IsSimulated;		// Native data type
	jboolean deviceIsSimulated; // Java data type
	jstring deviceName; 		// Java data type
	char deviceNamebuf[128];	// Native data type


	/*	 {
     // assume the prompt string and user input has less
     // than 128 characters
     char outbuf[128], inbuf[128];
     int len = (*env)->GetStringLength(env, prompt);
     (*env)->GetStringUTFRegion(env, prompt, 0, len, outbuf);
     printf("%s", outbuf);
     scanf("%s", inbuf);
     return (*env)->NewStringUTF(env, inbuf);
 }
	 */

	// Declare class references
	jclass daqmxGetDevIsSimulatedCls;
	// Declare Java Method IDs
	jmethodID getDeviceNameMID;
	jmethodID setDeviceIsSimulatedMID;

	// Get the classes
	daqmxGetDevIsSimulatedCls = env->GetObjectClass(daqmxGetDevIsSimulatedObj);

	// Get the methods in daqmxCreateTaskParamsCls
	getDeviceNameMID = env->GetMethodID(daqmxGetDevIsSimulatedCls, "getDeviceName", "()Ljava/lang/String;");
	if(getDeviceNameMID == NULL)
		return(89);


	deviceName = (jstring) env->CallObjectMethod(daqmxGetDevIsSimulatedObj, getDeviceNameMID, "");
	int len = env->GetStringLength(deviceName);
	printf("Device Name is Length %d\n", len);

	env->GetStringUTFRegion(deviceName, 0, len, deviceNamebuf);

	printf("Device Name is: %s\n", deviceNamebuf);


	setDeviceIsSimulatedMID = env->GetMethodID(daqmxGetDevIsSimulatedCls, "setDeviceIsSimulated","(Z)V");
	if(setDeviceIsSimulatedMID == NULL)
		return(88);

	// *****************
	// THE CALL TO DAQmx
	daqmxResult = DAQmxGetDevIsSimulated(deviceNamebuf, &dev_IsSimulated);


	// ------------------
	if(dev_IsSimulated) {
		printf("device IS simulated.\n");
		deviceIsSimulated = true;
	}
	else {
		printf("device IS NOT simulated.\n");
		deviceIsSimulated = false;
	}

	env->CallVoidMethod(daqmxGetDevIsSimulatedObj, setDeviceIsSimulatedMID, deviceIsSimulated);
	return(daqmxResult);
}

// Not used
//***************************************************** daqmxCreateAIVoltageChan
JNIEXPORT jint JNICALL
Java_nidaqdev_Nidaq_daqmxCreateAIVoltageChan(
		JNIEnv *env,
		jobject obj,
		jobject daqmxCreateAIVoltageObj)
{
	/*** NI DAQ - Configure Voltage Channel ***/
	TaskHandle  taskHandle = (TaskHandle) NULL;
	//	const char physicalChannel[] = "Dev1/ai0";					// The names of the physical channels to use to create virtual channels (Create Channel function)
	//	const char	channelName[] = "";								// The name(s) to assign to the created virtual channel(s) (Create Channel function)
	float64		minVolVal = -1.0;								// The minimum value, in units, that you expect to measure (Create Channel function)
	float64		maxVolVal = 1.0;								// The maximum value, in units, that you expect to measure (Create Channel function)
	int32		terminalConfig = 0;
	int32		units;
	//
	const char	physicalChannel0[] = "Dev1/ai0";					// The names of the physical channels to use to create virtual channels (Create Channel function)
	const char	channelName0[] = "ai0";

	uInt32 daqmxResult = 0;

	// Declare class references
	jclass daqmxCreateAIVoltageCls;
	// Declare Java Method IDs
	jmethodID getTaskHandleMID;
	jmethodID getTerminalConfigMID;
	jmethodID getMinValMID;
	jmethodID getMaxValMID;
	jmethodID getUnitsMID;

	// Get the classes
	daqmxCreateAIVoltageCls = env->GetObjectClass(daqmxCreateAIVoltageObj);

	// Get the methods in daqmxCreateTaskParamsCls
	getTaskHandleMID = env->GetMethodID(daqmxCreateAIVoltageCls, "getTaskHandle", "()I");
	if(getTaskHandleMID == NULL)
		return(89);

	getTerminalConfigMID = env->GetMethodID(daqmxCreateAIVoltageCls, "getTerminalConfig", "()I");
	if(getTerminalConfigMID == NULL)
		return(89);

	getUnitsMID = env->GetMethodID(daqmxCreateAIVoltageCls, "getUnits", "()I");
	if(getUnitsMID == NULL)
		return(89);

	getMinValMID = env->GetMethodID(daqmxCreateAIVoltageCls, "getMinVal", "()D");
	if(getMinValMID == NULL)
		return(89);

	getMaxValMID = env->GetMethodID(daqmxCreateAIVoltageCls, "getMaxVal", "()D");
	if(getMaxValMID == NULL)
		return(89);

	printf("\n======================================\n");
	printf("Java_nidaqdev_Nidaq_daqmxCreateAIVoltageChan\n");
	printf("======================================\n");
	printf("Parameters before retrieving over jni.\n");

	printf("Analogue-In Voltage Channel Created \n");		/* Configure Channel */
	printf("	Physical Channel:0: %s\n", physicalChannel0);		/* Configure Channel */
	//printf("	Channel Name: %s\n", channelName);		/* Configure Channel */
	//printf("	DAQmx_Val_Cfg_Default: %d\n", DAQmx_Val_Cfg_Default);		/* Configure Channel */
	printf("	minVolVal: %f\n", minVolVal);
	printf("	maxVolVal: %f\n", maxVolVal);
	printf("	DAQmx_Val_Volts: %d\n", DAQmx_Val_Volts);


	// Retrieve parameters over JNI.
	taskHandle = (void*)  env->CallIntMethod(daqmxCreateAIVoltageObj, getTaskHandleMID, "");
	terminalConfig = env->CallIntMethod(daqmxCreateAIVoltageObj, getTerminalConfigMID, "");
	units = env->CallIntMethod(daqmxCreateAIVoltageObj, getUnitsMID, "");
	minVolVal = env->CallDoubleMethod(daqmxCreateAIVoltageObj, getMinValMID, "");
	maxVolVal = env->CallDoubleMethod(daqmxCreateAIVoltageObj, getMaxValMID, "");


	printf("\n======================================\n");
	printf("Parameters after retrieving over jni.\n");

	printf("Analogue-In Voltage Channel Created \n");					/* Configure Channel */
	printf("	Physical Channel:0: %s\n", physicalChannel0);			/* Configure Channel */
	//printf("	Channel Name: %s\n", channelName);						/* Configure Channel */
	//printf("	DAQmx_Val_Cfg_Default: %d\n", DAQmx_Val_Cfg_Default);	/* Configure Channel */
	printf("	minVolVal: %f\n", minVolVal);
	printf("	maxVolVal: %f\n", maxVolVal);
	printf("	units: %d\n", units);

	// *****************
	// THE CALL TO DAQmx
	daqmxResult = DAQmxCreateAIVoltageChan(taskHandle,
			physicalChannel0,
			channelName0,
			DAQmx_Val_Cfg_Default,
			minVolVal,
			maxVolVal,
			DAQmx_Val_Volts,
			NULL);

	return(daqmxResult);
}

// Not used
//******************************************** daqmxCfgSampClkTiming
JNIEXPORT jint JNICALL
Java_nidaqdev_Nidaq_daqmxCfgSampClkTiming(
		JNIEnv *env,
		jobject obj,
		jobject daqmxCfgSampClkTimingObj)
{
	/*** native daqmxCfgSampClkTiming parameters ***/
	TaskHandle  taskHandle = (TaskHandle) NULL;
	const char  source[] = "";	// The source terminal of the Sample Clock. NULL = onboard clk.
	float64		rate = 0.0;		// The sampling rate in samples per second per channel.
	int32		activeEdge  = 0;
	int32		sampleMode = 0 ;
	uInt64 		sampsPerChanToAcquire =0;

	// DAQmx method result value
	uInt32 daqmxResult = 0;

	// Declare class references
	jclass daqmxCfgSampClkTimingCls;
	// Declare Java Method IDs
	jmethodID getTaskHandleMID;
	//	jmethodID getSourceMID;
	jmethodID getRateMID;
	jmethodID getActiveEdgeMID;
	jmethodID getSampleModeMID;
	jmethodID getSampsPerChanToAcquireMID;

	// Get the classes
	daqmxCfgSampClkTimingCls = env->GetObjectClass(daqmxCfgSampClkTimingObj);


	// Get the methods in daqmxCfgSampClkTimingCls
	getTaskHandleMID = env->GetMethodID(daqmxCfgSampClkTimingCls, "getTaskHandle", "()I");
	if(getTaskHandleMID == NULL)
		return(89);

	//	getSourceMID = env->GetMethodID(daqmxCfgSampClkTimingCls, "getSource", "()I");
	//	if(getSourceMID == NULL)
	//		return(89);

	getRateMID = env->GetMethodID(daqmxCfgSampClkTimingCls, "getRate", "()D");
	if(getRateMID == NULL)
		return(89);

	getActiveEdgeMID = env->GetMethodID(daqmxCfgSampClkTimingCls, "getActiveEdge", "()I");
	if(getActiveEdgeMID == NULL)
		return(89);

	getSampleModeMID = env->GetMethodID(daqmxCfgSampClkTimingCls, "getSampleMode", "()I");
	if(getSampleModeMID == NULL)
		return(89);

	getSampsPerChanToAcquireMID = env->GetMethodID(daqmxCfgSampClkTimingCls, "getSampsPerChanToAcquire", "()J");
	if(getSampsPerChanToAcquireMID == NULL)
		return(89);

	printf("\n======================================\n");
	printf("Java_nidaqdev_Nidaq_daqmxCfgSampClkTiming\n");
	printf("======================================\n");
	printf("Parameters before retrieving over jni.\n");
	printf("C++Nidaq:taskHandle: %d\n", taskHandle);
	printf("C++Nidaq:source: %s\n", source);
	printf("C++Nidaq:rate: %f\n", rate);
	printf("C++Nidaq:activeEdge: %u\n", activeEdge);
	printf("C++Nidaq:sampleMode: %u\n", sampleMode);
	printf("C++Nidaq:sampsPerChanToAcquire: %lu\n", sampsPerChanToAcquire);

	// Retrieve parameters over JNI.
	taskHandle = (void*) env->CallIntMethod(daqmxCfgSampClkTimingObj, getTaskHandleMID, "");
	//	source = env->CallCharMethod(daqmxCfgSampClkTimingObj, getSourceMID, "");
	rate = env->CallDoubleMethod(daqmxCfgSampClkTimingObj, getRateMID, "");
	activeEdge = env->CallIntMethod(daqmxCfgSampClkTimingObj, getActiveEdgeMID, "");
	sampleMode = env->CallIntMethod(daqmxCfgSampClkTimingObj, getSampleModeMID, "");
	sampsPerChanToAcquire = env->CallLongMethod(daqmxCfgSampClkTimingObj, getSampsPerChanToAcquireMID, "");


	printf("--------------------------------------\n");
	printf("Parameters after retrieving over jni.\n");
	printf("C++Nidaq:taskHandle: %d\n", taskHandle);
	printf("C++Nidaq:source: %s\n", source);
	printf("C++Nidaq:rate: %f\n", rate);
	printf("C++Nidaq:activeEdge: %u\n", activeEdge);
	printf("C++Nidaq:sampleMode: %u\n", sampleMode);
	printf("C++Nidaq:sampsPerChanToAcquire: %lu\n", sampsPerChanToAcquire);


	daqmxResult = DAQmxCfgSampClkTiming(taskHandle,
			source,
			rate,
			activeEdge, 	//DAQmx_Val_Rising,
			sampleMode, 	//DAQmx_Val_ContSamps,
			sampsPerChanToAcquire);
	return(daqmxResult);
}


// Not used
//******************************************** daqmxGetErrorString
JNIEXPORT jstring JNICALL
Java_nidaqdev_Nidaq_daqmxGetErrorString(JNIEnv *env, jobject obj,
		jint errorCode)
{
	char errorString[2048];
	DAQmxGetExtendedErrorInfo(errorString,2048);
	printf("Java_nidaqdev_Nidaq_daqmxGetErrorString:\n%s", errorString);
	//
	return env->NewStringUTF("Blarg");
}



// Not used
//***************************************************** daqmxReadAnalogF64
JNIEXPORT jint JNICALL
Java_nidaqdev_Nidaq_daqmxReadAnalogF64(JNIEnv *env,
		jobject obj,
		jobject daqmxReadAnalogF64ParamsObj,
		jobject daqDataObj,
		jdoubleArray arr)
{
	TaskHandle  taskHandle;
	int daqmxResult=0;

	int bufferIncrementMultiple = DAQ_BUFFER_MULTIPLE;
	int samplesPerChan = BUFFERBLOCKSIZE * bufferIncrementMultiple;	// Number of samples to acquire per channel (sample clock timing function)
	int numChannels = NUM_CHANNELS;
	int totalSamps = samplesPerChan * numChannels;
	uInt32 arraySizeInSamples = (uInt32) totalSamps;

	/*** NI DAQ - Sample Data from Card ***/
	// read variables
	int32		sampsPerChanRead = 0;
	float64		timeout = 10.0;									// The amount of time, in seconds, to wait for the function to read the sample(s) (Read function)
	int32		fillMode = DAQmx_Val_GroupByChannel;			// Specifies whether or not the samples are interleaved (Create Channel function)



	jdouble 	*doubleArrPtr;
	jboolean 	isDataACopy;
	doubleArrPtr = env->GetDoubleArrayElements(arr,&isDataACopy);
	jsize len = env->GetArrayLength(arr);

	printf("Data is jsize:%d.\n",len);
	if(isDataACopy){
		printf("Data IS a copy.\n");

	} else
	{
		printf("Data IS not a copy, it directly referenced.\n");
	}


	// Declare class references
	jclass daqDataCls;
	jclass daqmxReadAnalogF64ParamsCls;
	// Declare Java Method IDs
	jmethodID getTaskHandleMID;
	jmethodID getSamplesPerChannelMID;
	jmethodID getTimeoutMID;
	jmethodID getFillModeMID;
	//	jmethodID getDataMID;
	jmethodID getArraySizeInSampsMID;
	jmethodID setSampsPerChanReadMID;

	// Get the classes
	daqmxReadAnalogF64ParamsCls = env->GetObjectClass(daqmxReadAnalogF64ParamsObj);
	daqDataCls = env->GetObjectClass(daqDataObj);

	//taskHandle = (uInt32) env->GetFieldID(daqConfCls, "taskHandle", "I");

	// Get the methods in daqmxReadAnalogF64ParamsCls
	getTaskHandleMID = env->GetMethodID(daqmxReadAnalogF64ParamsCls, "getTaskHandle", "()I");
	if(getTaskHandleMID == NULL)
		return(89);

	getSamplesPerChannelMID = env->GetMethodID(daqmxReadAnalogF64ParamsCls, "getSamplesPerChannel", "()I");
	if(getSamplesPerChannelMID == NULL)
		return(89);

	getTimeoutMID = env->GetMethodID(daqmxReadAnalogF64ParamsCls, "getTimeout", "()D");
	if(getTimeoutMID == NULL)
		return(89);

	getFillModeMID = env->GetMethodID(daqmxReadAnalogF64ParamsCls, "getFillMode", "()I");
	if(getFillModeMID == NULL)
		return(89);

	getArraySizeInSampsMID = env->GetMethodID(daqmxReadAnalogF64ParamsCls, "getArraySizeInSamps", "()I");
	if(getArraySizeInSampsMID == NULL)
		return(89);

	setSampsPerChanReadMID = env->GetMethodID(daqmxReadAnalogF64ParamsCls, "setSampsPerChanRead", "(I)V");
	if(setSampsPerChanReadMID == NULL)
		return(89);

	//getDataMID = env->GetMethodID(daqmxReadAnalogF64ParamsCls, "getData", "()D");
	//if(getDataMID == NULL)
	//	return(89);

	printf("\n======================================\n");
	printf("Java_nidaqdev_Nidaq_daqRead\n");

	printf("======================================\n");
	printf("Parameters before retrieving over jni.\n");
	printf("C++Nidaq:taskHandle: %d\n", taskHandle);
	printf("C++Nidaq:samplesPerChan: %d\n", samplesPerChan);
	printf("C++Nidaq:timeout: %f\n", timeout);
	printf("C++Nidaq:fillMode: %d\n", fillMode);
	printf("C++Nidaq:arraySizeInSamples: %d\n", arraySizeInSamples);

	// Retrieve parameters for DAQmxReadAnalogF64 call
	taskHandle = (void*) env->CallIntMethod(daqmxReadAnalogF64ParamsObj, getTaskHandleMID, "");
	samplesPerChan = env->CallIntMethod(daqmxReadAnalogF64ParamsObj, getSamplesPerChannelMID, "");
	timeout = env->CallDoubleMethod(daqmxReadAnalogF64ParamsObj, getTimeoutMID, "");
	fillMode = env->CallIntMethod(daqmxReadAnalogF64ParamsObj, getFillModeMID, "");
	arraySizeInSamples = (uInt32) env->CallIntMethod(daqmxReadAnalogF64ParamsObj, getArraySizeInSampsMID, "");



	printf("======================================\n");
	printf("Parameters after retrieving from jni.");
	printf("C++Nidaq:taskHandle: %d\n", taskHandle);
	printf("C++Nidaq:samplesPerChan: %d\n", samplesPerChan);
	printf("C++Nidaq:timeout: %f\n", timeout);
	printf("C++Nidaq:fillMode: %d\n", fillMode);
	printf("C++Nidaq:arraySizeInSamples: %d\n", arraySizeInSamples);
	printf("======================================\n");


	// THE CALL TO DAQmx
	daqmxResult  = DAQmxReadAnalogF64(
			taskHandle,
			samplesPerChan,
			timeout,
			fillMode,
			//buffer[writeBufIndex],
			doubleArrPtr,
			arraySizeInSamples,
			&sampsPerChanRead,
			NULL);



	// TODO: FIXME. BOMB out here if samples read does not match samples asked for.
	if(totalSamps != sampsPerChanRead*numChannels) {
		printf ("totalSamps: %d\n", totalSamps);;
		printf ("sampsPerChanRead: %d\n",sampsPerChanRead);
		printf ("read mismatch..\n");
		return(1);
	}
	printf ("totalSamps: %d\n", totalSamps);;
	printf ("sampsPerChanRead: %d\n",sampsPerChanRead);
	//setSampsPerChanReadMID(sampsPerChanRead);
	env->CallVoidMethod(daqmxReadAnalogF64ParamsObj, setSampsPerChanReadMID, sampsPerChanRead);


	// ReleaseDouble
	// Value 	Meaning
	// 0 	Copy the contents of the buffer back into array and free the buffer
	// JNI_ABORT 	Free the buffer without copying back any changes
	// JNI_COMMIT 	Copy the contents of the buffer back into array but do not free buffer
	env->ReleaseDoubleArrayElements(arr,doubleArrPtr,0);
	printf("DAQmxReadAnalogF64 Finishing\n");
	return(daqmxResult);
}

// used
#define BUFFLEN 80
/*
 * Class:     nidaqdev_Nidaq
 * Method:    javaDAQmxGetDeviceType
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_nidaqdev_Nidaq_javaDAQmxGetDeviceType
(JNIEnv *env, jobject, jint boardNumber) {
	char boardName[BUFFLEN];
	char* strDev = getDeviceName(boardNumber);
	if (strDev == NULL) {
		return NULL;
	}
	int32 NIStat = DAQmxGetDevProductType (strDev, boardName, BUFFLEN);

	if (NIStat == 0)
	{
		return env->NewStringUTF(boardName);
	}
	else {
		return NULL;
	}
}
/*
 * Class:     nidaqdev_Nidaq
 * Method:    javaDAQmxGetDeviceName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_nidaqdev_Nidaq_javaDAQmxGetDeviceName
(JNIEnv *env, jobject, jint boardNumber) {
	char boardName[BUFFLEN];
	char* strDev = getDeviceName(boardNumber);
	if (strDev == NULL) {
		return NULL;
	}

	return env->NewStringUTF(strDev);
}

/**
 * Get the number of devices configured on this machine.
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_javaDAQmxGetNumDevices
(JNIEnv *env, jobject) {
	getDeviceName(0); // initialise list
	return nameListLength-1; // list is 1 indexed so is 1 longer than needed.
}

/**
 * Get the NI major version number
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_javaDAQmxGetMajorVersion
(JNIEnv *env, jobject) {
	uInt32 version;
	int ret = DAQmxGetSysNIDAQMajorVersion(&version);
	if (ret == 0) {
		return (jint) version;
	}
	else {
		return 0;
	}
}

/**
 * Get the NI major version number
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_javaDAQmxGetMinorVersion
(JNIEnv *env, jobject) {
	uInt32 version;
	int ret = DAQmxGetSysNIDAQMinorVersion(&version);
	if (ret == 0) {
		return (jint) version;
	}
	else {
		return 0;
	}
}



// used
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_javaDAQmxGetSerialNum
(JNIEnv *env, jobject, jint boardNumber) {
	//	char strDev[BUFFLEN];
	//	sprintf(strDev, "Dev%d", boardNumber);
	char* strDev = getDeviceName(boardNumber);
	if (strDev == NULL) {
		return -1;
	}
	uInt32 serNum;

	int32 NIStat = DAQmxGetDevSerialNum(strDev, &serNum);

	if (NIStat == 0)
	{
		return serNum;
	}
	else {
		return -1;
	}
}

// used
JNIEXPORT jboolean JNICALL Java_nidaqdev_Nidaq_javaDAQmxIsSimulated
(JNIEnv *env, jobject, jint boardNumber) {
	//	char strDev[BUFFLEN];
	//	sprintf(strDev, "Dev%d", boardNumber);
	char* strDev = getDeviceName(boardNumber);
	if (strDev == NULL) {
		return false;
	}
	bool32 isSim;

	int32 NIStat = DAQmxGetDevIsSimulated (strDev, &isSim);

	if (NIStat == 0)
	{
		return isSim;
	}
	else {
		return 0;
	}
}
// used
JNIEXPORT jboolean JNICALL Java_nidaqdev_Nidaq_jniAISimultaneousSampling
(JNIEnv *env, jobject, jint boardNumber) {
	//	char strDev[BUFFLEN];
	//	sprintf(strDev, "Dev%d", boardNumber);
	char* strDev = getDeviceName(boardNumber);
	if (strDev == NULL) {
		return false;
	}
	bool32 isSim;

#ifdef NILINUX
	int32 NIStat=1;
#else
	int32 NIStat = DAQmxGetDevAISimultaneousSamplingSupported (strDev, &isSim);
#endif

	if (NIStat == 0)
	{
		return isSim;
	}
	else {
		return 1;
	}
}

// used
JNIEXPORT jdouble JNICALL Java_nidaqdev_Nidaq_jniGetMaxSingleChannelRate
(JNIEnv *env, jobject, jint boardNumber) {
	//	char strDev[BUFFLEN];
	//	sprintf(strDev, "Dev%d", boardNumber);
	char* strDev = getDeviceName(boardNumber);
	if (strDev == NULL) {
		return -1;
	}
	float64 maxRate;

#ifdef NILINUX
	maxRate = 1250000;
	int32 NIStat=0;
#else
	int32 NIStat = DAQmxGetDevAIMaxSingleChanRate(strDev, &maxRate);
#endif

	if (NIStat == 0)
	{
		return maxRate;
	}
	else {
		return -1;
	}
}

// used
JNIEXPORT jdouble JNICALL Java_nidaqdev_Nidaq_jniGetMaxMultiChannelRate
(JNIEnv *env, jobject, jint boardNumber) {
	//	char strDev[BUFFLEN];
	//	sprintf(strDev, "Dev%d", boardNumber);
	char* strDev = getDeviceName(boardNumber);
	if (strDev == NULL) {
		return -1;
	}
	float64 maxRate;

#ifdef NILINUX
	maxRate = 1000000;
	int32 NIStat=0;
#else
	int32 NIStat = DAQmxGetDevAIMaxMultiChanRate(strDev, &maxRate);
#endif

	if (NIStat == 0)
	{
		return maxRate;
	}
	else {
		return -1;
	}
}

// used
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniGetMaxInputChannels
(JNIEnv *env, jobject, jint boardNumber)
{
	//	char strDev[BUFFLEN];
	//	sprintf(strDev, "Dev%d", boardNumber);
	char* strDev = getDeviceName(boardNumber);
	if (strDev == NULL) {
		return -1;
	}

	char chans[1024];

	int32 NIStat = DAQmxGetDevAIPhysicalChans (strDev, chans, 1023);
	if (NIStat != 0) {
		return 0;
	}
	if (strlen(chans) == 0) {
		return 0;
	}
	// otherwise, count the commas.
	int nchan = 1;
	char* cPos = chans;
	while (cPos = strchr(cPos, ',')){
		nchan++;
		cPos++;
	}

	return nchan;
}

//double[] dummyRanges = {-1., +1., -.5, 0.5};

// used
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniGetMaxOutputChannels
(JNIEnv *env, jobject, jint boardNumber)
{
	//	char strDev[BUFFLEN];
	//	sprintf(strDev, "Dev%d", boardNumber);
	char* strDev = getDeviceName(boardNumber);
	if (strDev == NULL) {
		return -1;
	}

	char chans[1024];

	int32 NIStat = DAQmxGetDevAOPhysicalChans (strDev, chans, 1023);
	if (NIStat != 0) {
		return 0;
	}
	if (strlen(chans) == 0) {
		return 0;
	}
	// otherwise, count the commas.
	int nchan = 1;
	char* cPos = chans;
	while (cPos = strchr(cPos, ',')){
		nchan++;
		cPos++;
	}

	return nchan;
}

JNIEXPORT jdoubleArray Java_nidaqdev_Nidaq_javaGetAIVoltageRanges
(JNIEnv *env, jobject, jint boardNumber) {
#define MAXRANGES 128

	//	char strDev[BUFFLEN];
	//	sprintf(strDev, "Dev%d", boardNumber);
	char* strDev = getDeviceName(boardNumber);
	if (strDev == NULL) {
		return NULL;
	}

	float64 ranges[MAXRANGES];
	for (int i = 0; i < MAXRANGES; i++) {
		ranges[i] = 0;
	}
#ifdef NILINUX
	//		int32 NIStat = 0;
	ranges[0] = -0.1;
	ranges[1] = 0.1;
	ranges[2] = -0.2;
	ranges[3] = 0.2;
	ranges[4] = -0.5;
	ranges[5] = 0.5;
	ranges[6] = -1.0;
	ranges[7] = 1.0;
	ranges[8] = -2.0;
	ranges[9] = 2.0;
	ranges[10] = -5.0;
	ranges[11] = 5.0;
	ranges[12] = -10.0;
	ranges[13] = 10.0;
#else
	int32 NIStat = DAQmxGetDevAIVoltageRngs(strDev, ranges, MAXRANGES);
#endif
	int nR = 0;
	// work out how many ranges are non -zero.
	// these will be in pairs, check for either being non
	// zero to allow for ranges that start at 0;
	for (int i = 0; i < MAXRANGES; i+=2) {
		if (ranges[i] == 0 && ranges[i+1] == 0) {
			break;
		}
		nR+=2;
	}
	if (nR == 0) {
		return NULL;
	}
	jdoubleArray farray = env->NewDoubleArray(nR);

	env->SetDoubleArrayRegion(farray, 0, nR, ranges);

	return farray;
}

JNIEXPORT jdoubleArray Java_nidaqdev_Nidaq_javaGetAOVoltageRanges(JNIEnv *env, jobject, jint boardNumber) {
#define MAXRANGES 128

	//	char strDev[BUFFLEN];
	//	sprintf(strDev, "Dev%d", boardNumber);
	char* strDev = getDeviceName(boardNumber);
	if (strDev == NULL) {
		return NULL;
	}

	float64 ranges[MAXRANGES];
	for (int i = 0; i < MAXRANGES; i++) {
		ranges[i] = 0;
	}
#ifdef NILINUX
	//		int32 NIStat = 0;
	ranges[0] = -0.1;
	ranges[1] = 0.1;
	ranges[2] = -0.2;
	ranges[3] = 0.2;
	ranges[4] = -0.5;
	ranges[5] = 0.5;
	ranges[6] = -1.0;
	ranges[7] = 1.0;
	ranges[8] = -2.0;
	ranges[9] = 2.0;
	ranges[10] = -5.0;
	ranges[11] = 5.0;
	ranges[12] = -10.0;
	ranges[13] = 10.0;
#else
	int32 NIStat = DAQmxGetDevAOVoltageRngs(strDev, ranges, MAXRANGES);
#endif
	int nR = 0;
	// work out how many ranges are non -zero.
	// these will be in pairs, check for either being non
	// zero to allow for ranges that start at 0;
	for (int i = 0; i < MAXRANGES; i+=2) {
		if (ranges[i] == 0 && ranges[i+1] == 0) {
			break;
		}
		nR+=2;
	}
	if (nR == 0) {
		return NULL;
	}
	jdoubleArray farray = env->NewDoubleArray(nR);

	env->SetDoubleArrayRegion(farray, 0, nR, ranges);

	return farray;
}

JNIEXPORT jboolean Java_nidaqdev_Nidaq_jniSetAOVoltageChannel
(JNIEnv *env, jobject, int boardNumber, int channel,
		jdoubleArray range, double voltage) {
	/*
	 * Make a wee task and set the channel. Should be able to
	 * cancel the task afterwards. If this causes any problems
	 * we'll set a load of static tasks, one for each channel
	 * and reuse them.
	 */
	/*
	 * Unpack the range information
	 */

	int nRange = env->GetArrayLength(range);
	if (nRange != 2) {
		printf("jniSetAOVoltageChannel range must be a two element double array !");
		return false;
	}
	jboolean isCopy;
	jdouble* pRange = env->GetDoubleArrayElements(range, &isCopy);
	double aoRange[2];
	for (int i = 0; i < 2; i++) {
		aoRange[i] = pRange[i];
	}
	env->ReleaseDoubleArrayElements(range, pRange, 0);

	/*
	 * Make the task, then output the voltage
	 */
	int niStat;
	TaskHandle	taskHandle=0;
	niStat = DAQmxCreateTask("",&taskHandle);
	if (niStat) {
		printf("jniSetAOVoltageChannel Error creating task\n");
		fflush(stdout);
		return false;
	}
	char chanName[20];
	sprintf(chanName, "%s/ao%d", getDeviceName(boardNumber), channel);
	niStat = DAQmxCreateAOVoltageChan(taskHandle,chanName,"",aoRange[0], aoRange[1], DAQmx_Val_Volts,"");
	if (niStat) {
		printf("jniSetAOVoltageChannel Error creating voltage chan %s code %d\n", chanName, channel);
		fflush(stdout);
		return false;
	}

	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	niStat = DAQmxStartTask(taskHandle);
	if (niStat) {
		printf("jniSetAOVoltageChannel Error starting task\n");
		fflush(stdout);
		return false;
	}

	/*********************************************/
	// DAQmx Write Code
	/*********************************************/
	float64     data[1] = {voltage};
	niStat = DAQmxWriteAnalogF64(taskHandle,1,1,1,DAQmx_Val_GroupByChannel,data,NULL,NULL);
	if (niStat) {
		printf("jniSetAOVoltageChannel Error writing voltage %f\n", voltage);
		fflush(stdout);
		return false;
	}

	DAQmxStopTask(taskHandle);
	DAQmxClearTask(taskHandle);


	return true;
}

// used
// used - this is the main bit that does the work !
#define DEFAULT_RANGE 1
int channelList[MAX_NI_CHANS];
int deviceList[MAX_NI_CHANS];
int channelsPerTask[MAX_NI_TASKS];
int32 deviceCategory[MAX_NI_CHANS];
int nChannels;
int nTasks;
double rangeListLo[MAX_NI_CHANS];
double rangeListHi[MAX_NI_CHANS];
int nRanges;
int sampleRate;
int terminalConfig = DAQmx_Val_NRSE;
volatile bool keepRunning;
volatile bool isRunning;
volatile TaskHandle mxTasks[MAX_NI_TASKS];
//JNIEnv **javaEnv;
int dataSize; // size of each sample = nChannels * sizeof(float64);
int mxSamplesToRead; // samples to transfer over from NI to c on each read.
byte* buff; // main buffer for transferring data from NI card to C
int32 totalSamplesRead;
jclass niclass;
jmethodID sayStringId = NULL;
jmethodID javaFullBuffer;
jmethodID javaResetPamguard;
int callBacksPerSecond = 10;

jobject *javaObject = NULL;
JavaVM *jvm;
//jmethodID javaSendBuffer = NULL;
ChannelBuffer** channelBuffers = NULL;
#define NBUFFERS 2


// used
JNIEXPORT void JNICALL Java_nidaqdev_Nidaq_jniSetTerminalConfig
(JNIEnv *env, jobject obj, jint newTerminalConfig) {
	terminalConfig = newTerminalConfig;
}

JNIEXPORT void JNICALL Java_nidaqdev_Nidaq_jniSetCallBacksPerSecond
(JNIEnv *env, jobject obj, jint newCallBacksPerSecond) {
	callBacksPerSecond = newCallBacksPerSecond;
}

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniGetCallBacksPerSecond
(JNIEnv *env, jobject obj) {
	return  (jint) callBacksPerSecond;
}

// used
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniPrepareDAQ
(JNIEnv *env, jobject obj, jint boardNumber, jint jSampleRate,
		jint bufferSeconds, jintArray jInputChannelList,
		jdoubleArray jRangeListLo, jdoubleArray jRangeListHi,
		jintArray jDevicesList) {


	// synchType indicates what device family the devices you are synching belong to:
	// 0 : E series
	// 1 : M series (PCI)
	// 2 : M series (PXI)
	// 3 : DSA Sample Clock Timebase
	// 4 : DSA Reference Clock
	int synchType = 1;

	env->GetJavaVM(&jvm);
	niclass = env->GetObjectClass(obj);
	sayStringId = env->GetStaticMethodID(niclass, "sayString", "([C)V");
	javaResetPamguard = env->GetStaticMethodID(niclass, "resetPamguard", "(I[C)V");
	if (sayStringId == 0){
		printf("*********** Unable to locate sayString method in Java *************");
		return 0;
	}
	char str[256];

	// get the reference to the java side function
	//	jclass cls = env->GetObjectClass(*javaObject);
	javaFullBuffer = env->GetStaticMethodID(niclass, "fullBuffer", "(I[D)V");
	if (javaFullBuffer == 0){
		printf("*********** Unable to locate javaFullBuffer method in Java *************");
		//		printf(string);
		return 0;
	}


	jint niStat;
	sampleRate = (int) jSampleRate;

	// un pack the channel lists.
	nChannels = env->GetArrayLength(jInputChannelList);
	if (nChannels == 0) {
		sprintf(str, "No Channels");
		return  reportError(env, DAQmxErrorChanIndexInvalid, str);
	}
	if (nChannels >= MAX_NI_CHANS) nChannels = MAX_NI_CHANS;
	jboolean isCopy;
	jint* pChList = env->GetIntArrayElements(jInputChannelList, &isCopy);
	for (int i = 0; i < nChannels; i++) {
		channelList[i] = (int) pChList[i];
	}
	env->ReleaseIntArrayElements(jInputChannelList, pChList, 0);

	// sort out ranges.
	// start by setting all to one
	for (int i = 0; i < MAX_NI_CHANS; i++) {
		rangeListLo[i] = -DEFAULT_RANGE;
		rangeListHi[i] = DEFAULT_RANGE;
	}
	/*
	 * Get the gains, there should either be a single gain
	 * or the same number as there are channels.
	 */
	nRanges = env->GetArrayLength(jRangeListLo);
	jdouble* pRanges;
	if (nRanges >= 0) {
		pRanges = env->GetDoubleArrayElements(jRangeListLo, &isCopy);
		for (int i = 0; i < nRanges; i++) {
			rangeListLo[i] = pRanges[i];
		}
		if (nRanges == 1) {
			for (int i = 1; i < nChannels; i++) {
				rangeListLo[i] = rangeListLo[0];
			}
		}
		env->ReleaseDoubleArrayElements(jRangeListLo, pRanges, 0);
	}
	nRanges = env->GetArrayLength(jRangeListHi);
	if (nRanges >= 0) {
		pRanges = env->GetDoubleArrayElements(jRangeListHi, &isCopy);
		for (int i = 0; i < nRanges; i++) {
			rangeListHi[i] = pRanges[i];
		}
		if (nRanges == 1) {
			for (int i = 1; i < nChannels; i++) {
				rangeListHi[i] = rangeListHi[0];
			}
		}
		env->ReleaseDoubleArrayElements(jRangeListHi, pRanges, 0);
	}

	/*
	 * Start by setting all devices to master, then overwrite if
	 * other data exist
	 */
	for (int i = 0; i < MAX_NI_CHANS; i++) {
		deviceList[i] = boardNumber;
	}
	int nDevices;
	if (jDevicesList) {
		nDevices = env->GetArrayLength(jDevicesList);
		jint* pDevList = env->GetIntArrayElements(jDevicesList, &isCopy);
		for (int i = 0; i < nDevices; i++) {
			deviceList[i] = (int) pDevList[i];
		}
		env->ReleaseIntArrayElements(jDevicesList, pDevList, 0);
	}

	/**
	 * Work out a list of device categories
	 */
	char* devName;
	for (int i = 0; i < nChannels; i++) {
		//		sprintf(devName, "Dev%d", deviceList[i]);
		devName = getDeviceName(boardNumber);
		if (devName == NULL) {
			return -1;
		}
		DAQmxGetDevProductCategory(devName, deviceCategory+i );
		//		sprintf(str, "Channel %d device %s type %d", i, devName, deviceCategory[i]);
		//		reportToJava(env, str);
	}

	/*
	 * Do some tests to check that all channels are in devices with the
	 * same properties.
	 */
	bool multipleCategories = false;
	for (int i = 1; i < nChannels; i++) {
		if (deviceCategory[i] != deviceCategory[i-1]) {
			sprintf(str, "Attempt to use multiple NI device categories (%d and %d). Configuration may be invalid",
					deviceCategory[i-1], deviceCategory[i]);
			reportToJava(env, str);
			multipleCategories = true;
		}
	}
	bool isCSeries = false;
	bool isSSeries = false;
	if (multipleCategories == false) {
		//		sprintf(str, "Single NI device category in use: %d", deviceCategory[0]);
		reportToJava(env, str);
		if (deviceCategory[0] == DAQmx_Val_CSeriesModule) {
			isCSeries = true;
			//			reportToJava(env, "C series device(s) will run in a single NI task");
		}
		if (deviceCategory[0] == DAQmx_Val_SSeriesDAQ) {
			isSSeries = true;
			reportToJava(env, "S series device synchronisation");
		}
	}



	int bufferSamples = sampleRate / callBacksPerSecond;
	if (bufferSamples < 100 ) {
		bufferSamples = 100;
	}
	// create the channel buffers
	channelBuffers = (ChannelBuffer**) calloc(nChannels, sizeof(ChannelBuffer*));
	for (int i = 0; i < nChannels; i++) {
		channelBuffers[i] = new ChannelBuffer(channelList[i], NBUFFERS, bufferSamples);
	}

	// create a task
	TaskHandle* pTaskHandle;
	//	= (TaskHandle*) &mxTasks; // use temp pointer since problem of cast of volatile handle
	//	niStat = DAQmxCreateTask (TaskName, pTaskHandle);
	//	if (niStat) {
	//		return reportError(env, niStat, "DAQmxCreateTask");
	//	}


	int currentDevice = -1;
	int iTask;
	//	TaskHandle currentTask = NULL;
	// now set up the channels, if the device number changes, create a new task too.
	char chanName[20];
	char taskName[50];
	nTasks = 0;
	for (int i = 0; i < nChannels; i++)
	{
		if (currentDevice < 0 || (deviceList[i] != currentDevice & !isCSeries)) {
			//			sprintf(chanName, "Dev%d", deviceList[i]);
			devName = getDeviceName(deviceList[i]);
			niStat = DAQmxResetDevice(devName);
			if (niStat) {
				return reportError(env, niStat, "DAQmxResetDevice");
			}

			iTask = nTasks;
			sprintf(taskName, "PAMGUARD_NI_DAQ_%d", iTask);
			sprintf(str, "creating task %s", taskName);
			pTaskHandle = (TaskHandle*) &mxTasks[nTasks++]; // use temp pointer since problem of cast of volatile handle
			niStat = DAQmxCreateTask (taskName, pTaskHandle);
			if (niStat) {
				return reportError(env, niStat, "DAQmxCreateTask");
			}
			reportToJava(env, str);
			currentDevice = deviceList[i];
			channelsPerTask[iTask] = 0;
		}

		//if (PipeInfo.Sampling.BoardGain == 0) PipeInfo.Sampling.BoardGain = 1;
		sprintf(chanName, "%s/ai%d", getDeviceName(deviceList[i]), channelList[i]);
		sprintf(str, "Create channel %s", chanName);
		//		reportToJava(env, str);
		//		sprintf(chanName, "Dev1/ai%d", channelList[i]);

		niStat = DAQmxCreateAIVoltageChan(*pTaskHandle, chanName, "", terminalConfig,
				rangeListLo[i], rangeListHi[i], DAQmx_Val_Volts, NULL);
		if (niStat) {
			return reportError(env, niStat, chanName);
		}
		channelsPerTask[iTask] ++;

		niStat = DAQmxSetAIRngLow(*pTaskHandle, chanName, rangeListLo[i]);
		if (niStat) {
			return reportError(env, niStat, "DAQmxSetAIRngLow");
		}

		niStat = DAQmxSetAIRngHigh(*pTaskHandle, chanName, rangeListHi[i]);
		if (niStat) {
			return reportError(env, niStat, "DAQmxSetAIRngHigh");
		}

	}

	for (int t = 0; t < nTasks; t++) {
		// set the NI internal buffer to bufferSeconds;
		niStat = DAQmxCfgInputBuffer (mxTasks[t], (int) sampleRate * bufferSeconds);
		if (niStat) {
			return reportError(env, niStat, "DAQmxCfgInputBuffer");
		}
	}

	if (isSSeries) {
		char synchString[256];
		char trigName[256];
		float64     clkRate;

		for (int i = 0; i < nTasks; i++) {
			niStat = DAQmxCfgSampClkTiming(mxTasks[i], "", sampleRate,
					DAQmx_Val_Rising, DAQmx_Val_ContSamps, sampleRate);
			if (niStat) {
				return reportError(env, niStat, "Can't configure external DAQmxCfgSampClkTiming");
			}
//			niStat = DAQmxSetSampTimingType(mxTasks[0], "", sampleRate,
//					DAQmx_Val_Rising, DAQmx_Val_ContSamps, sampleRate);
//			if (niStat) {
//				return reportError(env, niStat, "Can't configure external DAQmxCfgSampClkTiming");
//			}
		}

//		niStat = DAQmxGetMasterTimebaseSrc(mxTasks[0],synchString,256);
//				if (niStat) {
//					return reportError(env, niStat, "DAQmxGetMasterTimebaseSrc");
//				}
//		niStat = DAQmxGetMasterTimebaseRate(mxTasks[0],&clkRate);
//				if (niStat) {
//					return reportError(env, niStat, "DAQmxGetMasterTimebaseRate");
//				}
//		niStat = DAQmxSetRefClkSrc(mxTasks[0], "PXI_Clk10");
//		if (niStat) {
//			return reportError(env, niStat, "DAQmxSetRefClkSrc");
//		}
//		niStat = GetTerminalNameWithDevPrefix(mxTasks[0],"SyncPulse",synchString);
//		if (niStat) {
//			return reportError(env, niStat, "GetTerminalNameWithDevPrefix");
//		}
		GetTerminalNameWithDevPrefix(mxTasks[0],"ai/StartTrigger",trigName);
		sprintf(str, "Got trigger from task 1: %s", trigName);
		reportToJava(env, str);
//		if (niStat) {
//			return reportError(env, niStat, "GetTerminalNameWithDevPrefix");
//		}

		for (int t = 1; t < nTasks; t++) {

//			niStat = DAQmxSetMasterTimebaseSrc(mxTasks[t],synchString);
//			if (niStat) {
//				return reportError(env, niStat, "DAQmxCfgDigEdgeStartTrig");
//			}
//			niStat = DAQmxSetMasterTimebaseRate(mxTasks[t],clkRate);
//			if (niStat) {
//				return reportError(env, niStat, "DAQmxCfgDigEdgeStartTrig");
//			}
			//			niStat = DAQmxSetSyncPulseSrc(mxTasks[t], synchString);
			//			if (niStat) {
//				return reportError(env, niStat, "DAQmxSetSyncPulseSrc");
//			}
//			niStat = DAQmxSetRefClkSrc(mxTasks[t], "PXI_Clk10");
//			if (niStat) {
//				return reportError(env, niStat, "DAQmxSetRefClkSrc");
//			}
			DAQmxCfgDigEdgeStartTrig(mxTasks[t],trigName,DAQmx_Val_Rising);
			if (niStat) {
				return reportError(env, niStat, "DAQmxCfgDigEdgeStartTrig");
			}
		}
	}
	else{
		/**
		 * Configure clock timing to be internal for first board. (param2 = null)
		 */
		niStat = DAQmxCfgSampClkTiming(mxTasks[0], "", sampleRate,
				DAQmx_Val_Rising, DAQmx_Val_ContSamps, sampleRate);
		if (niStat) {
			return reportError(env, niStat, "Can't configure external DAQmxCfgSampClkTiming");
		}
		/**
		 * Configure clock rates for all other boards to get it from PFI1
		 * (this clearly get's ignored for simulated boards, since they still run)
		 */
		for (int t = 1; t < nTasks; t++) {
			niStat = DAQmxCfgSampClkTiming(mxTasks[t], "PFI1", sampleRate,
					DAQmx_Val_Rising, DAQmx_Val_ContSamps, sampleRate);
			if (niStat) {
				return reportError(env, niStat, "Can't configure external DAQmxCfgSampClkTiming");
			}
		}
		/*
		 * for the first board, configure it to export the timing signal
		 */
		if (nTasks > 1) {
			/*
			 * Export the clock signal for the first device.
			 * m series devices should export ConvertClock since they are not multiplexed.
			 * In June 2012 we found that this fails for the non-multiplexed x series devices.
			 * Rather than waste time working out which type of device we have, will continue to
			 * try that, but if it fails, try the SampleClock which will work with X-series
			 * devices.
			 * An error message will be displayed if both of these fail.
			 */
			niStat = DAQmxExportSignal(mxTasks[0], DAQmx_Val_AIConvertClock, "PFI1");
			if (niStat) {
				niStat = DAQmxExportSignal(mxTasks[0], DAQmx_Val_SampleClock, "PFI1");
				if (niStat) {
					return reportError(env, niStat, "DAQmxExportSignal Can't configure DAQmx_Val_AIConvertClock or DAQmx_Val_SampleClock to PFI1");
				}
			}
		}
	}

	// only need to get callback from the master task (0)
	// since data from all tasks should be available at the same time
	niStat = DAQmxRegisterEveryNSamplesEvent(mxTasks[0],DAQmx_Val_Acquired_Into_Buffer,bufferSamples,0,
			EveryNCallback, NULL);
	if (niStat) {
		reportError(env, niStat, "DAQmxRegisterEveryNSamplesEvent");
	}

	niStat = DAQmxRegisterDoneEvent(mxTasks[0],0,DoneCallback,NULL);
	if (niStat) {
		reportError(env, niStat, "DAQmxRegisterDoneEvent");
	}

	dataSize = nChannels * sizeof(float64);
	mxSamplesToRead = channelBuffers[0]->getLengthSamples() * nChannels;
	buff = (byte*) calloc(1, mxSamplesToRead * dataSize);

	totalSamplesRead = 0;

	reportToJava(env, "NI DAQ Initialised OK");
	return 0;
}

// used
/*
 * Walk before run - get new references each time for now to anything
 * back in the JVM, then try to globalise later on
 */
int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData)
{
	JNIEnv *env;
	// need this call to get the thread specific link to the Java environment.
	jvm->AttachCurrentThread((void**)&env, NULL);
	//	reportToJava(env, "EveryNCallback called");
	int32 sampsRead, samplesPerChan;
	float64* rawPointer;

	//	double* fullBuffer;
	jboolean isCopy;
	//	static int callCount = 0;


	//	reportToJava(env, javaObject, "EveryNCallback read data");
	// read the data from the tasks all into the same buffer, since
	// this is grouped by channel, it should be easy to shift the ponter
	// along between each read.
	rawPointer = (float64*) buff;
	for (int iT = 0; iT < nTasks; iT++) {
		int32 niStat = DAQmxReadAnalogF64(mxTasks[iT], channelBuffers[0]->getLengthSamples(),
				0.1, DAQmx_Val_GroupByChannel, rawPointer, mxSamplesToRead,
				&sampsRead, NULL);
		//		if (++callCount >= 100) {
		//			niStat = -200279;
		//			callCount = 0;
		//		}
		if (niStat) {
			//			reportError(env, niStat, "DAQmxReadAnalogF64");
			resetPamguard(env, niStat);
			return niStat;
		}
		rawPointer += sampsRead * channelsPerTask[iT];
	}
	//	reportToJava(env, javaObject, "EveryNCallback Data read OK");
	totalSamplesRead += sampsRead;
	samplesPerChan = sampsRead ;/// nChannels;
	//	rawPointer = (float64*) buff;

	static jdoubleArray transferArray = NULL;
	static jdouble* transferData = NULL;
	static int transferArrayLength = 0;

	int transferSize = sizeof(double) * channelBuffers[0]->getLengthSamples();

	if (transferArray == NULL || transferArrayLength != transferSize) {
		if (transferArray != NULL) {
			env->DeleteLocalRef(transferArray);
		}
		transferArrayLength = transferSize;
		transferArray = env->NewDoubleArray(channelBuffers[0]->getLengthSamples());
		//		reportToJava(env, "Allocate transfer array buffer");
	}


	for (int iChan = 0; iChan < nChannels; iChan++) {
		rawPointer = (float64*) ( buff + transferSize * iChan);
		//		transferArray = env->NewDoubleArray(channelBuffers[0]->getLengthSamples());
		transferData = env->GetDoubleArrayElements(transferArray, &isCopy);
		for (int i = 0; i<sampsRead; i++) {
			transferData[i] = *rawPointer/rangeListLo[iChan];
			rawPointer++;
		}
		//		memcpy(transferData, rawPointer, transferSize);
		//		reportToJava(env, "EveryNCallback transfer data");
		env->ReleaseDoubleArrayElements(transferArray, transferData, 0);
		env->CallStaticVoidMethod(niclass, javaFullBuffer, iChan, transferArray);
		//		env->ReleaseDoubleArrayElements(transferArray, transferData, 0);
	}

	// Release the JVM - causes a crash!
	//jvm->DetachCurrentThread();

	return 0;

}

// used
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData)
{
	JNIEnv *env;
	// need this call to get the thread specific link to the Java environment.
	jvm->AttachCurrentThread((void**)&env, NULL);
	reportToJava(env, "DoneCallback called");
	isRunning = false;

	// Release the JVM - causes a crash!
	//jvm->DetachCurrentThread();

}

// used
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniStartDAQ
(JNIEnv *env, jobject obj) {
	jint niStat;
	keepRunning = true;

	//	if (javaObject == NULL) {
	jobject jo = env->NewGlobalRef(obj);
	javaObject = &jo;
	//	}

	//	reportToJava(env, "call StartTask");
	// start all the tasks in reverse order so that the
	// mster starts last.
	for (int iT = nTasks-1; iT >= 0; iT--) {
		niStat = DAQmxStartTask(mxTasks[iT]);
		if (niStat) {
			return reportError(env, niStat, " in DAQmxStartTask");
		}
	}
	//	reportToJava(env, "called StartTask");

	isRunning = true;

	return 1;

}
// used
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniStopDAQ
(JNIEnv *env, jobject obj) {
	return stopDaq(env);
}
jint stopDaq(JNIEnv *env) {
	//	reportToJava(env, "call stop task");
	for (int iT = 0; iT < nTasks; iT++) {
		jint niStat = DAQmxStopTask(mxTasks[iT]);
		if (niStat) {
			reportError(env, niStat, "DAQmxStopTask");
		}
		//		reportToJava(env, "call clear task");
		niStat = DAQmxClearTask(mxTasks[iT]);
		if (niStat) {
			reportError(env, niStat, "DAQmxClearTask");
		}
	}
	nTasks = 0;

	//	reportToJava(env, obj, "set task = 0");
	mxTasks[0] = 0;
	//	printf("Total NI Samples read = %d",

	keepRunning = false;
	isRunning = false;
	while (isRunning) {
#ifdef NILINUX
		// this should really check if win32 or linux - todo!
		sleep(10);
#else
		Sleep(10);
#endif
		//		reportToJava(env, obj, "Waiting end");
		// wait for the task to be cleared.
	}
	//	reportToJava(env, obj, "DAQ Ended");

	if (channelBuffers != NULL) {
		for (int i = 0; i < nChannels; i++) {
			delete(channelBuffers[i]);
		}
		free(channelBuffers);
		channelBuffers = NULL;
	}

}


void resetPamguard(JNIEnv *env, int32 errorCode) {

	stopDaq(env);

	char string[1024];
	jboolean isCopy;
	int32 NIStat = DAQmxGetErrorString(errorCode, string, sizeof(string));
	int len = strlen(string);
	jcharArray javaArray = env->NewCharArray(len);
	jchar* jData = env->GetCharArrayElements(javaArray, &isCopy);
	for (int i = 0; i < len; i++) {
		jData[i] = string[i];
	}
	if (isCopy == JNI_TRUE) {
		env->ReleaseCharArrayElements(javaArray, jData, JNI_COMMIT);
	}

	env->CallStaticVoidMethod(niclass, javaResetPamguard, errorCode, javaArray);
}

// used
void reportToJava(JNIEnv *env, char* string) {
	//	jmethodID javaReportError;
	//	jclass cls = env->GetObjectClass(obj);
	//	javaReportError = env->GetMethodID(cls, "showString", "([C)V");
	//	if (javaReportError == 0){
	//		printf("*********** Unable to locate reporting method in Java *************");
	//		printf(string);
	//		return;
	//	}

	jboolean isCopy;
	int len = strlen(string);
	jcharArray javaArray = env->NewCharArray(len);
	jchar* jData = env->GetCharArrayElements(javaArray, &isCopy);
	for (int i = 0; i < len; i++) {
		jData[i] = string[i];
	}
	if (isCopy == JNI_TRUE) {
		env->ReleaseCharArrayElements(javaArray, jData, JNI_COMMIT);
	}
	//	env->CallVoidMethod(obj, javaReportError, javaArray);
	env->CallStaticVoidMethod(niclass, sayStringId, javaArray);


	env->ReleaseCharArrayElements(javaArray, jData, 0);
	// turn the char* into a String.
}

/**
 * Function to prepare an NI card for playback of sound data.
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniPreparePlayback
(JNIEnv *env, jobject obj, jint boardNumber, jint jSampleRate, jfloat outputRange,
		jint bufferSamples, jintArray jOutputChannelList)
{

	env->GetJavaVM(&jvm);
	niclass = env->GetObjectClass(obj);
	sayStringId = env->GetStaticMethodID(niclass, "sayString", "([C)V");
	char str[256];

	jint niStat;
	sampleRate = (int) jSampleRate;

	// un pack the channel lists.
	nChannels = env->GetArrayLength(jOutputChannelList);
	if (nChannels == 0) {
		return reportError(env, DAQmxErrorChanIndexInvalid, "No Playback Channels");
	}
	if (nChannels >= MAX_NI_CHANS) nChannels = MAX_NI_CHANS;
	jboolean isCopy;
	jint* pChList = env->GetIntArrayElements(jOutputChannelList, &isCopy);
	for (int i = 0; i < nChannels; i++) {
		channelList[i] = (int) pChList[i];
	}
	env->ReleaseIntArrayElements(jOutputChannelList, pChList, 0);

	deviceList[0] = boardNumber;

	TaskHandle* pTaskHandle;
	char chanName[20];
	char taskName[50];

	sprintf(taskName, "PAMGUARD_PLAY_0");
	sprintf(str, "creating task %s", taskName);
	pTaskHandle = (TaskHandle*) &mxTasks[0]; // use temp pointer since problem of cast of volatile handle
	niStat = DAQmxCreateTask (taskName, pTaskHandle);
	if (niStat) {
		return reportError(env, niStat, "DAQmxCreateTask");
	}
	reportToJava(env, str);
	int currentDevice = 0;
	channelsPerTask[0] = nChannels;

	for (int i = 0; i < nChannels; i++) {
		sprintf(chanName, "%s/ao%d", getDeviceName(boardNumber), i);
		sprintf(str, "Create playback channel %s", chanName);
		reportToJava(env, str);
		//		sprintf(chanName, "Dev1/ai%d", channelList[i]);

		niStat = DAQmxCreateAOVoltageChan(*pTaskHandle, chanName, "",
				-outputRange, +outputRange, DAQmx_Val_Volts, NULL);
		if (niStat) {
			return reportError(env, niStat, chanName);
		}
	}

	//	DAQmxRegisterEveryNSamplesEvent(mxTasks[0],DAQmx_Val_Transferred_From_Buffer,bufferSamples,0,
	//			EveryNPlayCallback, NULL);
	//
	//	DAQmxRegisterDoneEvent(mxTasks[0],0,DonePlayCallback,NULL);

	niStat = DAQmxCfgOutputBuffer(mxTasks[0], bufferSamples);
	if (niStat) {
		return reportError(env, niStat, "DAQmxCfgOutputBuffer");
	}

	niStat = DAQmxCfgSampClkTiming(mxTasks[0], NULL, sampleRate,
			DAQmx_Val_Rising, DAQmx_Val_ContSamps, sampleRate);
	if (niStat) {
		return reportError(env, niStat, "Can't configure external DAQmxCfgSampClkTiming");
	}

	return 0;
}

JNIEXPORT jboolean JNICALL Java_nidaqdev_Nidaq_jniStartPlayback
(JNIEnv *env, jobject obj)
{

	jint niStat = DAQmxStartTask(mxTasks[0]);
	if (niStat) {
		reportError(env, niStat, "DAQmxStartTask");
		return false;
	}
	return true;
}

JNIEXPORT jboolean JNICALL Java_nidaqdev_Nidaq_jniStopPlayback
(JNIEnv *env, jobject obj)
{

	jint niStat = DAQmxStopTask(mxTasks[0]);
	if (niStat) {
		return reportError(env, niStat, "DAQmxStopPlaybck");
	}
	//	reportToJava(env, obj, "call clear task");
	niStat = DAQmxClearTask(mxTasks[0]);
	if (niStat) {
		return reportError(env, niStat, "DAQmxClearTask");
	}
	return true;
}

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniPlaybackData
(JNIEnv *env, jobject obj, jdoubleArray playbackData)
{

	jboolean isCopy;

	int nData = env->GetArrayLength(playbackData);
	jdouble* pData = env->GetDoubleArrayElements(playbackData, &isCopy);
	int32 sampsWrote;
	int err = DAQmxWriteAnalogF64(mxTasks[0], nData/nChannels, true, 1.0,
			DAQmx_Val_GroupByChannel, pData, &sampsWrote, NULL);

	if (err) {
		reportError(env, err, "PlaybackData");
	}
	//	for (int i = 0; i < nChannels; i++) {
	//		channelList[i] = (int) pChList[i];
	//	}
	env->ReleaseDoubleArrayElements(playbackData, pData, 0);
	return sampsWrote;
}

int32 CVICALLBACK EveryNPlayCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData)
{
	return 0;
}
int32 CVICALLBACK DonePlayCallback(TaskHandle taskHandle, int32 status, void *callbackData)
{
	return 0;
}
// used
jint reportError(JNIEnv *env, jint errorCode, char* str) {

	char buff[1024];
	int32 NIStat = DAQmxGetErrorString(errorCode, buff, sizeof(buff));
	int l = strlen(buff) + 100;
	if (str) {
		l += strlen(str);
	}
	char* cString = (char*)calloc(l, sizeof(char));
	if (str) {
		sprintf(cString, "NIDaq Error %s code %d %s\n", str, errorCode, buff);

	}
	else {
		sprintf(cString, "NIDaq Error code %d %s\n", errorCode, buff);
	}
	reportToJava(env, cString);
	free(cString);
	return errorCode;
}

// probably not used
double getNIGainFactor(int NIGainCode)
{
	if (NIGainCode == -1) return 0.5;
	else return (double) NIGainCode;
}


