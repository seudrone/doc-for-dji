TARGET = guidance_example

OBJECTS = main1.o DJI_utility.o

CFLAGS = -g -Wall -I/usr/local/include -I../../../include -DHAVE_OPENCV -I/usr/local/include/opencv2

LDFLAGS = -Wl,-rpath,./ -lpthread -lrt -L./ -L/usr/local/lib/ -lDJI_guidance -lusb-1.0 -lopencv_core -lopencv_highgui #-l**DJI\_guidance**

CXX = g++

$(TARGET) : $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
	rm *.gch
	
main1.o : main1.cpp DJI_utility.h
	$(CXX) $(CFLAGS) -c main1.cpp DJI_utility.h
	
DJI_utility.o : DJI_utility.cpp DJI_utility.h
	$(CXX) $(CFLAGS) -c DJI_utility.cpp DJI_utility.h
	
clean:
	rm *.o *.gch $(TARGET)
	
