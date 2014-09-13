/*
 * OrderedItem.h
 *
 *  Created on: Nov 28, 2013
 *      Author: rajeshwari
 */


#ifndef ORDEREDITEM_H_
#define ORDEREDITEM_H_
#include <time.h>



class OrderedItem {
public:
	OrderedItem();
	virtual ~OrderedItem();
	int itemId;
	long clientId;
	time_t arrivalTime;
	time_t departTime;
	int prepTime;
	OrderedItem* getOrder(int index);
	void setItemId(int iId);
	void setClientId(long id);
	void setArrivalTime(time_t aTime);
	void setDepartureTime(time_t dTime);
	void setPrepTime(int pTime);
};

#endif /* ORDEREDITEM_H_ */
