/*
 * File: queue.h
 * Author: Chris Wailes <chris.wailes@gmail.com>
 * Author: Wei-Te Chen <weite.chen@colorado.edu>
 * Author: Andy Sayler <andy.sayler@gmail.com>
 * Project: CSCI 3753 Programming Assignment 2
 * Create Date: 2010/02/12
 * Modify Date: 2011/02/05
 * Modify Date: 2012/02/01
 * Description:
 * 	This is the header file for an implemenation of a simple FIFO queue.
 * 
 */

#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

int CheckRequests(char* inputFileName, int* currentPosition);
/* Function to do all requests */
void *Requester(void* inputFilePath);
/* Does the resolving of the addresses */
void *Resolver(); 
/* Initializes queue and mutexes */
void initEverything();
/* Cleans up queue and mutexes */ 
void destoryEverything();


#endif
