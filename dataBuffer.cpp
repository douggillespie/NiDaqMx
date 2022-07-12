/*
 * dataBuffer.cpp
 *
 *  Created on: 03-Nov-2008
 *      Author: Doug Gillespie
 */

#ifdef NILINUX
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
#else
	#include <windows.h>
#endif

#include "dataBuffer.h"

ChannelBuffer::ChannelBuffer(int iChan, int nSubBuffers, int lengthSamples) {

	this->iChan = iChan;
	this->nSubBuffers = nSubBuffers;
	this->lengthSamples = lengthSamples;
	data = (double**) calloc(nSubBuffers, sizeof(double*));
	for (int i = 0; i < nSubBuffers; i++) {
		data[i] = (double*) calloc(lengthSamples, sizeof(double));
	}
	currentWriteBuffer = data[0];

	writeBufferNumber = 0;
	writeBufferPosition = 0;
	sendBufferNumber = 0;
	totalWrittenBuffers = 0;
	totalSentBuffers = 0;
}
ChannelBuffer::~ChannelBuffer() {
	for (int i = 0; i < nSubBuffers; i++) {
		free(data[i]);
	}
	free(data);
}
double* ChannelBuffer::writeData(double dataPoint) {
	currentWriteBuffer[writeBufferPosition++] = dataPoint;
	if (writeBufferPosition >= lengthSamples) {
		totalWrittenBuffers++;
		writeBufferPosition = 0;
		double* readBuffer = currentWriteBuffer;
		if (++writeBufferNumber >= nSubBuffers) {
			writeBufferNumber = 0;
			currentWriteBuffer = data[writeBufferNumber];
		}
		return readBuffer;
	}
	return NULL;
}
double* ChannelBuffer::getBuffer(int iBuffer, bool autoIncrement) {
	return data[iBuffer];
}

