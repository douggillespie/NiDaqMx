#cl -MD -LD demolib.cpp NIDAQmx.lib -Femylib.dll 
CPP  = g++.exe
CC   = gcc.exe
#WINDRES = windres.exe
RES  = 
OBJ  = jninidaqmx.o  dataBuffer.o $(RES)
LINKOBJ  = jninidaqmx.o dataBuffer.o $(RES)
LIBS =  -L"C:/WINDOWS/system32"-mwindows -lole32 -lwinmm -L"C:\Program Files\National Instruments\NI-DAQ\DAQmx ANSI C Dev\lib\msvc" -lNIDAQmx
#LIBS = -L NIDAQmx.lib

DLL = jninidaqmx.dll
RM = C:/MinGW/msys/1.0/bin/rm -f

.PHONY: all all-before all-after clean clean-custom

clean: clean-custom
	${RM} $(OBJ) $(BIN) jninidaqmx33.dll

$(DLL): $(OBJ)
	$(CPP) $(LINKOBJ) -shared -o "jninidaqmx.dll" jninidaqmx.def $(LIBS)	

jninidaqmx.o: jninidaqmx.cpp
	$(CPP) -c 	-I"C:\Program Files\Java\jdk1.6.0_21\include"	-I"C:\Program Files\Java\jdk1.6.0_21\include\win32" -I"C:\Program Files\National Instruments\NI-DAQ\DAQmx ANSI C Dev\include" jninidaqmx.cpp -o jninidaqmx.o

	
dataBuffer.o: dataBuffer.cpp
	$(CPP) -c dataBuffer.cpp -o dataBuffer.o