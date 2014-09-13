/*
 * scufastfood.cpp
 *
 *  Created on: Nov 28, 2013
 *      Author: rajeshwari
 */
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <algorithm>
#include <time.h>
#include <queue>
#include <pthread.h>
#include "OrderedItem.h"
#include </usr/include/semaphore.h>

using namespace std;

#define NUM_CLIENT_THREADS 5
#define NUM_CASHIER_THREADS 2
#define NUM_SERVER_THREADS 1
#define NUM_KITCHEN_THREADS 2

bool allClientsExited = false;

typedef struct scu_fast_food_menu {
	int itemId;
	char description[100];
	float price;
	int prepTime;
} scu_fast_food_menu_t;

scu_fast_food_menu_t menuList[11] = {
		{0, "",0,0},
		{1, "BBQ-Chicken-Salad", 8.95, 180},
		{2, "Spinach-Powder", 9.15, 120},
		{3, "Garden Salad", 4.75, 100},
		{4, "Steak Blue Cheese", 7.25, 220},
		{5, "Slovenian-Salad", 6.75, 130},
		{6, "Hong-Kong-Chicken-Salad", 9.15, 150},
		{7, "Mongolian-BBQ-Plate", 9.75, 210},
		{8, "Club-Sandwich", 6.35, 135},
		{9, "Belgian-Cheese-Sub", 10.25, 150},
		{10, "Chipotle-Beef-Sub", 9.35, 180},
};


// Semaphore names follow the <producer><Consumer> pattern.
sem_t clientCashierMutex;
sem_t clientCashierFull;
sem_t cashierKitchenMutex;
sem_t cashierKitchenFull;
sem_t kitchenServerMutex;
sem_t kitchenServerFull;
sem_t serverClient[NUM_CLIENT_THREADS];
sem_t finishedClientOrdersMutex;

OrderedItem clientOrders[1000];
int clientExitCount = 0;

queue <OrderedItem> orderTaken;
queue <OrderedItem> orderList;
queue <OrderedItem> orderCompleted;
queue <OrderedItem> finishedClientOrders;

static void *clientMethod(void *threadid);
static void *cashierMethod(void *threadid);
static void *kitchenMethod(void *threadid);
static void *serverMethod(void *threadid);
static void SetupSemaphores(void);
static void FreeSemaphores(void);
static int RandomInteger(int low, int high);
static void generateStatistics(void);

static void *clientMethod(void *threadid)
{
	   long tid;
	   tid = (long)threadid;
	   OrderedItem *item = new OrderedItem();
	   //Set current time as arrival time.
	   time_t aTime;
	   time(&aTime);
       item->setArrivalTime(aTime);

       printf("Client[%ld] entered the restaurant.\n",tid);
       sem_wait(&clientCashierMutex);
	   //Get the item ID from the array of items scanned from the file
	   item->setItemId(clientOrders[(int)tid].itemId);
	   //Set the client ID to the orderedItem
	   item->setClientId((long)threadid);
	   //Set the prep time of the item from the table.
	   item->setPrepTime(menuList[item->itemId].prepTime);
	   orderTaken.push(*item);
	   sem_post(&clientCashierMutex);
	   sem_post(&clientCashierFull);

	   //Wait for the appropriate semaphore
	   sem_wait(&serverClient[(long)threadid]);//Waiting for the server for order
	   time_t dTime;
	   time(&dTime);//Client has been served and exits
	   item->setDepartureTime(dTime);
	   clientExitCount++;

	   sem_wait(&finishedClientOrdersMutex);
	   finishedClientOrders.push(*item);
	   sem_post(&finishedClientOrdersMutex);
	   printf("Client[%ld] is leaving.\n", tid);
	   //If all clients have been served, then signal other threads
	   //they will check for allClientsExited flag.
	   if(clientExitCount == NUM_CLIENT_THREADS)
	   {
		   allClientsExited = true;
		   for (int i = 0; i < NUM_CASHIER_THREADS; i++) {
			   	  sem_post(&clientCashierFull);
		   }
		   for (int i = 0; i < NUM_KITCHEN_THREADS; i++) {
			   	   sem_post(&cashierKitchenFull);
		   }
		   for (int i = 0; i < NUM_SERVER_THREADS; i++) {
			   	   sem_post(&kitchenServerFull);
		   }
	   }
	   pthread_exit(NULL);
}

static void *cashierMethod(void *threadid)
{
   long tid;
   tid = (long)threadid;
   while(1) {
	   	   OrderedItem *item = new OrderedItem();
	   	   sem_wait(&clientCashierFull);
	   	   //Check if all clients are finished, if true then exit the while loop.
	   	   if (allClientsExited) {
			   if (tid == 0) {
				   printf("Manager generating reports.\n");
				   generateStatistics();
			   }
			   break;
		   }
	   	   sem_wait(&clientCashierMutex);
	   	   //If there is no client waiting then takes a break
	   	   if (orderTaken.empty()) {
			   printf("No Client: Cashier[%ld] is taking break.\n", tid);
			   int tbreak = RandomInteger(0,5);
			   sleep(tbreak);
			   printf("Break Complete: Cashier[%ld] is back to work.\n", tid);
		   } else {
			   int tservice = RandomInteger(0,5);
			   sleep(tservice);
			   *item = orderTaken.front();
			   printf("Cashier[%ld] has taken order from Client[%ld].\n", tid,item->clientId);
			   orderTaken.pop();
		   }
	   	   sem_post(&clientCashierMutex);
	   	   //Push the order in the kitchen's queue.
	   	   sem_wait(&cashierKitchenMutex);
	   	   orderList.push(*item);
	   	   sem_post(&cashierKitchenMutex);
	   	   //Notify Kitchen that item has been ordered.
	   	   sem_post(&cashierKitchenFull);
   }
   pthread_exit(NULL);
}

static void *kitchenMethod(void *threadid)
{
   long tid;
   tid = (long)threadid;
   while(1) {
		OrderedItem *item = new OrderedItem();
		//Waits for cashier
		sem_wait(&cashierKitchenFull);
		if (allClientsExited) {
			 //Check if all clients are finished, if true then exit the while loop.
			 break;
		}
		sem_wait(&cashierKitchenMutex);
		*item = orderList.front();
		orderList.pop();
		sem_post(&cashierKitchenMutex);
		printf("Kitchen Chef[%ld] is preparing item[%d] for Client[%ld]\n", tid,item->itemId, item->clientId);
		//Preparing the food.
		sleep(item->prepTime/10);
		sem_wait(&kitchenServerMutex);
		printf("Kitchen Chef[%ld] completed preparing item[%d] for Client[%ld]\n", tid,item->itemId, item->clientId);
		orderCompleted.push(*item);
		//Releases the kitchen semaphores
		sem_post(&kitchenServerMutex);
		sem_post(&kitchenServerFull);
   }
   pthread_exit(NULL);
}

//Server Method
static void *serverMethod(void *threadid)
{
   long tid;
   tid = (long)threadid;
   while(1) {
	   OrderedItem *item = new OrderedItem();
	   //Waits for kitchen
	   sem_wait(&kitchenServerFull);
	   if (allClientsExited) {
		   break;
	   }
	   sem_wait(&kitchenServerMutex);
	   //Gets the completed order
	   *item = orderCompleted.front();
	   orderCompleted.pop();
	   printf("Server[%ld] serving item[%d] to Client[%ld]\n", tid,item->itemId,item->clientId);
	   //Releases the kitchen semaphore
	   sem_post(&kitchenServerMutex);
	   int tserver = RandomInteger(0,2);
	   sleep(tserver);
	   sem_post(&serverClient[item->clientId]);
   }
   pthread_exit(NULL);
}

//Function to setup all the semaphores
static void SetupSemaphores(void)
{
	sem_init(&clientCashierMutex,0,1);
	sem_init(&clientCashierFull,0,0);
	sem_init(&cashierKitchenMutex,0,1);
	sem_init(&cashierKitchenFull,0,0);
	sem_init(&kitchenServerMutex,0,1);
	sem_init(&kitchenServerFull,0,0);
	sem_init(&finishedClientOrdersMutex,0,1);
	for(int i = 0; i<NUM_CLIENT_THREADS; i++) {
		sem_init(&serverClient[i],0,0);
	}
}
//Function to free all the semaphores
static void FreeSemaphores(void)
{
	sem_destroy(&clientCashierMutex);
	sem_destroy(&clientCashierFull);
	sem_destroy(&cashierKitchenMutex);
	sem_destroy(&cashierKitchenFull);
	sem_destroy(&kitchenServerMutex);
	sem_destroy(&kitchenServerFull);
	sem_destroy(&finishedClientOrdersMutex);
	for(int i = 0; i<NUM_CLIENT_THREADS; i++) {
		sem_destroy(&serverClient[i]);
	}
}
//Function to generate random times(tservice, tarrival, tbreak and tserver)
static int RandomInteger(int low, int high)
{
	extern long random();
	long choice;
	int range = high - low + 1;
	choice = random();
	return low + choice % range;
}

struct orderCount
{
	int id;
	int freq;
};

//Sort the frequency to find the top 5 popular items ordered
static void sortOrdersByFreq(orderCount *arr,int n)
{
	for(int x = 0; x < n; x++) {
		for(int y = 0; y < (n - x - 1); y++) {
			if(arr[y].freq>arr[y+1].freq) {
				orderCount temp = arr[y+1];
				arr[y+1] = arr[y];
				arr[y] = temp;
			}
		}
	}
}


//Statics generated by the Manager
static void generateStatistics()
{
	//Process the queue to generate the results.
	int totalWaitingTime = 0;
	orderCount itemOrderedFreq[10];
	bzero(&itemOrderedFreq, sizeof(orderCount)*10);
	float totalRevenue = 0;
    FILE *outputFile;
    	outputFile = fopen("output.txt","w");
	sem_wait(&finishedClientOrdersMutex);
	while (!finishedClientOrders.empty()) {
		OrderedItem item = finishedClientOrders.front();
		int waitingTime = difftime(item.departTime, item.arrivalTime);
		fprintf(outputFile, "ClientId: %ld Waiting Time: %d:%d:%d Money spent: $ %.2f\n", item.clientId,
		int(waitingTime/3600), int(waitingTime/60), int(waitingTime%60), menuList[item.itemId].price);
		totalWaitingTime += waitingTime;
		totalRevenue += menuList[item.itemId].price;
		itemOrderedFreq[item.itemId-1].id = item.itemId;
		itemOrderedFreq[item.itemId-1].freq++;
		finishedClientOrders.pop();
	}
	sem_post(&finishedClientOrdersMutex);
	int avgWaiting = totalWaitingTime / NUM_CLIENT_THREADS;
	fprintf(outputFile, "Average waiting time: %d:%d:%d\n", int(avgWaiting/3600), int(avgWaiting/60), int(avgWaiting%60));
	sortOrdersByFreq(itemOrderedFreq,10);
	fprintf(outputFile, "Top 5 popular item:\n");
	for(int i = 9; i>=5; i--) {
		int n = itemOrderedFreq[i].id;
		int freq = itemOrderedFreq[i].freq;
		fprintf(outputFile, "%s - $ %.2f\n", menuList[n].description, menuList[n].price*freq);
	}
	
	fprintf(outputFile, "Total number of clients: %d \nTotal Revenue generated: $ %.2f\n", NUM_CLIENT_THREADS, totalRevenue);

	fclose (outputFile);
}


int main(int argc, char **argv)
{
	pthread_t client_threads[NUM_CLIENT_THREADS];
	pthread_t cashier_threads[NUM_CASHIER_THREADS];
	pthread_t server_threads[NUM_SERVER_THREADS];
	pthread_t kitchen_threads[NUM_KITCHEN_THREADS];

    SetupSemaphores();//Initialize all the semaphores

    FILE *inputFile;//Scan the input file
    int idx_orders = 0;
	inputFile = fopen("input.txt","r");
	char string[200] = {0};
	while (fgets(string, 200, inputFile)) {
		sscanf(string, "%d",&clientOrders[idx_orders].itemId);//Scan all the item IDs
		if (clientOrders[idx_orders].itemId <= 0 && clientOrders[idx_orders].itemId > 11) {
			printf("Error: Enter appropriate menu item.\n");
			exit(1);
		}
		idx_orders++;
	}
	fclose (inputFile);

   //Create all the client, cashier, kitchen and server threads
   int rc_cs;
   for(int t=0; t<NUM_CASHIER_THREADS; t++)
   {
	   rc_cs =  pthread_create(&cashier_threads[t], NULL, cashierMethod, (void *)t);
	   if (rc_cs) {
		 printf("ERROR; return code from pthread_create() is %d\n", rc_cs);
		 exit(-1);
	  }
	}

   int rc_sr;
   for(int t=0; t<NUM_SERVER_THREADS; t++)
   {
		rc_sr =  pthread_create(&server_threads[t], NULL, serverMethod, (void *)t);
		if (rc_sr) {
		 printf("ERROR; return code from pthread_create() is %d\n", rc_sr);
		 exit(-1);
		}
   	}

   int rc_kt;
   for(int t=0; t<NUM_KITCHEN_THREADS; t++)
   {
		rc_kt =  pthread_create(&kitchen_threads[t], NULL, kitchenMethod, (void *)t);
		if (rc_kt) {
		 printf("ERROR; return code from pthread_create() is %d\n", rc_kt);
		 exit(-1);
		}
   }

   int rc_cl;
   for(int t=0; t<NUM_CLIENT_THREADS; t++)
   {
		if(t!=0) {
			  int tArrival = RandomInteger(1,5);
			  sleep(tArrival);//Randomly arriving clients
		}
		rc_cl =  pthread_create(&client_threads[t], NULL, clientMethod, (void *)t);
		if (rc_cl) {
		 printf("ERROR; return code from pthread_create() is %d\n", rc_cl);
		 exit(-1);
		}
    }
	pthread_exit(NULL);
	FreeSemaphores();//Free all the semaphores
	return 0;
}
