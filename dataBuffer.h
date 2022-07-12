/*
 * dataBuffer.h
 *
 *  Created on: 03-Nov-2008
 *      Author: Doug Gillespie
 *
 *      Each ChannelBuffer contains data for one DAQ channel.
 *      Each contains multple sub buffers of data of a size suitable
 *      for sending off to the JAVA side of things.
 *      Data are stored as doubles.
 */

#ifndef DATABUFFER_H_
#define DATABUFFER_H_

class ChannelBuffer {
public:
	ChannelBuffer(int iChan, int nSubBuffers, int lengthSamples);
	~ChannelBuffer();
	double* writeData(double dataPoint);
	double* getBuffer(int iBuffer, bool autoIncrement = false);
	int getChannelNumber(){return iChan;};
	int getLengthSamples(){return lengthSamples;};
private:
	double** data;
	double* currentWriteBuffer;
	int iChan;
	int nSubBuffers;
	int lengthSamples; // length of an individual sub buffer.
	int writeBufferNumber;
	int writeBufferPosition;
	int sendBufferNumber;
	int totalWrittenBuffers;
	int totalSentBuffers;
};

#endif /* DATABUFFER_H_ */
