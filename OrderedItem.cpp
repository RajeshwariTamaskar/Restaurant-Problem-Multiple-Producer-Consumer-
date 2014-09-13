/*
 * OrderedItem.cpp
 *
 *  Created on: Nov 28, 2013
 *      Author: rajeshwari
 */

using namespace std;
#include "OrderedItem.h"
#include <stdio.h>

OrderedItem::OrderedItem() {

}

OrderedItem::~OrderedItem() {

}

OrderedItem* OrderedItem::getOrder(int index){
	//Scan through the input file
	//return &client_orders[index];
	return NULL;
}

void OrderedItem::setItemId(int iId){
	this->itemId = iId;
}

void OrderedItem::setClientId(long id){
	this->clientId = id;
}

void OrderedItem::setArrivalTime(time_t aTime){
	this->arrivalTime = aTime;
}

void OrderedItem::setDepartureTime(time_t dTime){
	this->departTime = dTime;
}

void OrderedItem::setPrepTime(int pTime){
	this->prepTime = pTime;
}
//void OrderedItem::readOrderList() {
//	    FILE *inputFile;
//	    int idx_orders = 0;
//		inputFile = fopen("input.txt","r");
//
//		char string[200];
//		while (fgets(string, 200, inputFile)) {
//			sscanf(string, "%d %d %d",&client_orders[idx_orders].clientId,
//					&client_orders[idx_orders].itemId,
//					&client_orders[idx_orders].delayFromPrevArrival);
//		}
//}


