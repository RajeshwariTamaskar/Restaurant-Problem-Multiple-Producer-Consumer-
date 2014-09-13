default: Project3

SOURCES=scufastfood.cpp OrderedItem.cpp

Project3: scufastfood.cpp OrderedItem.cpp 
	g++ -g -Wall -o Project3 $(SOURCES) -lpthread



