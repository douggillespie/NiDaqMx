/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class nidaqdev_Nidaq */

#ifndef _Included_nidaqdev_Nidaq
#define _Included_nidaqdev_Nidaq
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     nidaqdev_Nidaq
 * Method:    helloworld
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_nidaqdev_Nidaq_helloworld
  (JNIEnv *, jobject);

/*
 * Class:     nidaqdev_Nidaq
 * Method:    daqmxCreateTask
 * Signature: (Lnidaqdev/Nidaq/DAQmxCreateTaskParams;)I
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_daqmxCreateTask
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nidaqdev_Nidaq
 * Method:    daqmxStartTask
 * Signature: (Lnidaqdev/Nidaq/DAQmxStartTaskParams;)I
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_daqmxStartTask
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nidaqdev_Nidaq
 * Method:    daqmxStopTask
 * Signature: (Lnidaqdev/Nidaq/DAQmxStopTaskParams;)I
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_daqmxStopTask
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nidaqdev_Nidaq
 * Method:    daqmxClearTask
 * Signature: (Lnidaqdev/Nidaq/DAQmxClearTaskParams;)I
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_daqmxClearTask
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nidaqdev_Nidaq
 * Method:    daqmxCreateAIVoltageChan
 * Signature: (Lnidaqdev/Nidaq/DAQmxCreateAIVoltageChanParams;)I
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_daqmxCreateAIVoltageChan
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nidaqdev_Nidaq
 * Method:    daqmxCfgSampClkTiming
 * Signature: (Lnidaqdev/Nidaq/DAQmxCfgSampClkTimingParams;)I
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_daqmxCfgSampClkTiming
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nidaqdev_Nidaq
 * Method:    daqmxReadAnalogF64
 * Signature: (Lnidaqdev/Nidaq/DAQmxReadAnalogF64Params;Lnidaqdev/Nidaq/DaqData;[D)I
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_daqmxReadAnalogF64
  (JNIEnv *, jobject, jobject, jobject, jdoubleArray);

/*
 * Class:     nidaqdev_Nidaq
 * Method:    daqmxGetErrorString
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_nidaqdev_Nidaq_daqmxGetErrorString
  (JNIEnv *, jobject, jint);

/*
 * Class:     nidaqdev_Nidaq
 * Method:    daqmxGetDevIsSimulated
 * Signature: (Lnidaqdev/Nidaq/DAQmxGetDevIsSimulatedParams;)I
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_daqmxGetDevIsSimulated
  (JNIEnv *, jobject, jobject);

JNIEXPORT jdouble JNICALL Java_nidaqdev_Nidaq_jniGetMaxSingleChannelRate
(JNIEnv *env, jobject, jint boardNumber);

JNIEXPORT jdouble JNICALL Java_nidaqdev_Nidaq_jniGetMaxMultiChannelRate
(JNIEnv *env, jobject, jint boardNumber);

/*
 * Class:     nidaqdev_Nidaq
 * Method:    javaDAQmxGetDeviceName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_javaDAQmxGetNumDevices
(JNIEnv *env, jobject);

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_javaDAQmxGetMajorVersion
(JNIEnv *env, jobject);

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_javaDAQmxGetMinorVersion
(JNIEnv *env, jobject);

JNIEXPORT jstring JNICALL Java_nidaqdev_Nidaq_javaDAQmxGetDeviceType
  (JNIEnv *, jobject, jint);

JNIEXPORT jstring JNICALL Java_nidaqdev_Nidaq_javaDAQmxGetDeviceName
  (JNIEnv *, jobject, jint);

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_javaDAQmxGetSerialNum
(JNIEnv *env, jobject, jint boardNumber);

JNIEXPORT jboolean JNICALL Java_nidaqdev_Nidaq_javaDAQmxIsSimulated
(JNIEnv *env, jobject, jint boardNumber);

JNIEXPORT jboolean JNICALL Java_nidaqdev_Nidaq_jniAISimultaneousSampling
(JNIEnv *env, jobject, jint boardNumber);

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniGetMaxInputChannels
(JNIEnv *env, jobject o, jint boardNumber);

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniGetMaxOutputChannels
(JNIEnv *env, jobject o, jint boardNumber);

JNIEXPORT jdoubleArray Java_nidaqdev_Nidaq_javaGetAIVoltageRanges
(JNIEnv *env, jobject, jint boardNumber);

JNIEXPORT jdoubleArray Java_nidaqdev_Nidaq_javaGetAOVoltageRanges
(JNIEnv *env, jobject, jint boardNumber);

JNIEXPORT jboolean Java_nidaqdev_Nidaq_jniSetAOVoltageChannel
(JNIEnv *env, jobject, int boardNumber, int channel,
		jdoubleArray range, double voltage);

JNIEXPORT void JNICALL Java_nidaqdev_Nidaq_jniSetTerminalConfig
(JNIEnv *env, jobject obj, jint newTerminalConfig);

JNIEXPORT void JNICALL Java_nidaqdev_Nidaq_jniSetCallBacksPerSecond
(JNIEnv *env, jobject obj, jint newCallBacksPerSecond);

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniGetCallBacksPerSecond
(JNIEnv *env, jobject obj);

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniPrepareDAQ
(JNIEnv *env, jobject obj, jint boardNumber, jint jSampleRate,
		jint bufferSeconds, jintArray jInputChannelList,
		jdoubleArray jRangeListLo, jdoubleArray jRangeListHi,
		jintArray jDevicesList);

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniStartDAQ
(JNIEnv *env, jobject);

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniStopDAQ
(JNIEnv *env, jobject);

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniPreparePlayback
(JNIEnv *env, jobject obj, jint boardNumber, jint jSampleRate,
		jfloat outputRange,
		jint bufferSamples, jintArray jOutputChannelList);

JNIEXPORT jboolean JNICALL Java_nidaqdev_Nidaq_jniStartPlayback
(JNIEnv *env, jobject obj);

JNIEXPORT jboolean JNICALL Java_nidaqdev_Nidaq_jniStopPlayback
(JNIEnv *env, jobject obj);

JNIEXPORT jint JNICALL Java_nidaqdev_Nidaq_jniPlaybackData
(JNIEnv *env, jobject obj, jdoubleArray playbackData);

#ifdef __cplusplus
}
#endif
#endif
