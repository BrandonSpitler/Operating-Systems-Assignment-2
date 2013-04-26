/******************************************************************************
 * FILE: pthread_hello.c
 * DESCRIPTION:
 *   A "hello world" Pthreads program.  Demonstrates thread creation and
 *   termination.
 * AUTHOR: Blaise Barney, Junho Ahn, Andy Sayler
 * LAST REVISED: 02/08/12
 ******************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "multi-lookup.h"
#include "util.h"
#include "queue.h"

#define PRINT_DEBUG 0

#define REQUESTER_CHECK_RESULTS 1
#define MAX_IP_ADDRESSES 255

pthread_mutex_t queueMutex;
pthread_mutex_t outputMutex; 
pthread_mutex_t requesterCountMutex;
queue hostnameQueue; 
int requesterCount = 0; 

char* outputFileName; 

int CheckRequests(char* inputFileName, int* currentPosition)
{
  FILE* inputfp = NULL;
  inputfp = fopen((char*)inputFileName, "r");
  FILE* outputfp = NULL;
  outputfp = fopen(outputFileName, "r");
  int ii; 
  char hostnameIn[SBUFSIZE]; 
  char outputLine[SBUFSIZE+INET6_ADDRSTRLEN*MAX_IP_ADDRESSES]; // Max of MAX_IP_ADDRESSES addresses
  
  for(ii = 0; ii < *currentPosition + 1; ii++)
  {
    if(fscanf(inputfp, INPUTFS, (char*) &hostnameIn) < 1)
    {
      fclose(inputfp); 
      return 0; //We are at the end of the file and finished
    }
  }
  
  pthread_mutex_lock(&outputMutex);
  
  while(fgets((char*) &outputLine, SBUFSIZE+INET6_ADDRSTRLEN*MAX_IP_ADDRESSES, outputfp) != NULL)
  { 
    int currentLetter = 0; 
    while(outputLine[currentLetter] != (int) NULL &&
	  outputLine[currentLetter] != ',' &&
	  hostnameIn[currentLetter] != (int) NULL &&
	  outputLine[currentLetter] == hostnameIn[currentLetter])
    {
      currentLetter++; 
    } 
    int isSame = (outputLine[currentLetter] == ',' || 
		      outputLine[currentLetter] == (int) NULL ||
			outputLine[currentLetter] == '\n') &&
		 (hostnameIn[currentLetter] == (int) NULL);
		 
    if(isSame) 
    {
      printf("%s\n", outputLine); 
      if(fscanf(inputfp, INPUTFS, (char*) &hostnameIn) < 1)
      {
	fclose(outputfp); 
	pthread_mutex_unlock(&outputMutex);
	fclose(inputfp); 
	return 0; //We are at the end of the file and finished
      }
      else
      {
	(*currentPosition)++; 
      }
    }
  }
   //printf("hostnameIn %s outputLine %s\n", hostnameIn, outputLine);
  
  fclose(outputfp); 
  pthread_mutex_unlock(&outputMutex);
  
  fclose(inputfp); 
  return 1; // Not done
}

void *Requester(void* inputFileNamePointer)
{ 
  pthread_mutex_lock(&requesterCountMutex);
  requesterCount++; 
  pthread_mutex_unlock(&requesterCountMutex);
  char* inputFileName = inputFileNamePointer; 
  FILE* inputfp = NULL;
  inputfp = fopen((char*)inputFileName, "r");
  char* hostname = malloc(SBUFSIZE*sizeof(char));
  char* errorstr = malloc(SBUFSIZE*sizeof(char));
  
  if(!inputfp){
    sprintf(errorstr, "Error Opening Input File: %s", inputFileName);
    perror(errorstr);
    exit(EXIT_FAILURE);
  }
  
  #if PRINT_DEBUG == 1
  printf("Opened: %s\n", inputFileName); 
  #endif
  
  /* Read File and Process*/
  while(fscanf(inputfp, INPUTFS, hostname) > 0)
  {
    #if PRINT_DEBUG == 1 
    printf("Adding Hostname %s\n", hostname); 
    #endif
    int queueStatus = QUEUE_FAILURE; 
    char * newString = strdup(hostname);
    
    // Wait for QUEUE to empty if full
    while(queueStatus == QUEUE_FAILURE)
    {
      pthread_mutex_lock(&queueMutex); 
      // Add to QUEUE 
      queueStatus = queue_push(&hostnameQueue, (void*) newString); 
      pthread_mutex_unlock(&queueMutex);
      if(queueStatus == QUEUE_FAILURE)
      { 
	usleep(rand()%100000); //Sleeps for between 0 and 100 ms
      }
    }
  }

  /* Close Input File */
  fclose(inputfp);
  #if REQUESTER_CHECK_RESULTS == 1
  int currentPosition = 0; 
  while(CheckRequests(inputFileName, &currentPosition))
  {
    //Sleep for 250ms
    usleep(250000); 
  }
  #endif
  
  pthread_mutex_lock(&requesterCountMutex);
  requesterCount--; 
  pthread_mutex_unlock(&requesterCountMutex);
  free(hostname); 
  free(errorstr); 
  return NULL; 
}

void *Resolver()
{
  FILE* outputfp = NULL;
  char* hostname = NULL; 
  char** ipArray; 
  
  while(1)
  {
    pthread_mutex_lock(&queueMutex);
    hostname = queue_pop(&hostnameQueue); 
    pthread_mutex_unlock(&queueMutex);
    
    if(hostname != NULL)
    {
      int addressCount = 0; 
      #if PRINT_DEBUG == 1 
      printf("Looking up hostname: %s\n", hostname); 
      #endif
      
      ipArray = malloc(255*sizeof(char*)); //Max of 255 addresses 
      /* Lookup hostname and get IP string */
      if(dnslookupAll(hostname, ipArray, 255, &addressCount)
	  == UTIL_FAILURE){
	  fprintf(stderr, "dnslookup error: %s\n", hostname);
      }
      #if PRINT_DEBUG == 1 
      printf("addressCount: %d\n", addressCount); 
      #endif
      /* Write to Output File */
      
	pthread_mutex_lock(&outputMutex);
	outputfp = fopen(outputFileName, "a");
	if(!outputfp){
	    perror("Error Opening Output File");
	    exit(EXIT_FAILURE);
	}
	fprintf(outputfp, "%s", hostname); 
	int ii;
	for(ii = 0; ii < addressCount; ii++)
	{
	  fprintf(outputfp,", %s", ipArray[ii]);
	  if(ipArray[ii] != NULL)
	  {
	    free(ipArray[ii]);
	  }
	}
	fprintf(outputfp,"\n");
	if(ipArray != NULL)
	{
	  free(ipArray); 
	}
	/* Close Output File */
	fclose(outputfp);
	pthread_mutex_unlock(&outputMutex);
	free(hostname); 
    }
    // If we are done
    else if( requesterCount == 0)
    {
      free(hostname); 
      break; 
    }
  }
  return NULL; 
}

void initEverything()
{
  //Assume 100 hosts in Queue at a time
  // Will wait if we need more
  queue_init(&hostnameQueue, QUEUEMAXSIZE); 
  pthread_mutex_init(&outputMutex, NULL);
  pthread_mutex_init(&queueMutex, NULL);
  pthread_mutex_init(&requesterCountMutex, NULL);
  srand(time(NULL)); 
}

void destoryEverything()
{
  pthread_mutex_destroy(&queueMutex);
  pthread_mutex_destroy(&outputMutex);
  pthread_mutex_destroy(&requesterCountMutex);
  queue_cleanup(&hostnameQueue); 
}

int main(int argc, char *argv[])
{
    /* void unused vars */
    (void) argc;
    (void) argv;
    char errorstr[SBUFSIZE];
    
    int inputFileCount = argc-2; 
    outputFileName = argv[(argc-1)];
    //Clear Output File
    FILE* outputfp = NULL;
    outputfp = fopen(outputFileName, "w");
    fclose(outputfp); 
    
    int numCores = sysconf( _SC_NPROCESSORS_ONLN ) * 2;
    int numThread = (numCores < MAX_RESOLVER_THREADS) ? numCores : MAX_RESOLVER_THREADS; 
    numThread = (numCores > MIN_RESOLVER_THREADS) ? numCores : MIN_RESOLVER_THREADS; 
    
    printf("Using number of threads (scaled with cores, within Constraints): %d\n", numThread); 
    
    /* Check Arguments */
    if(argc < MINARGS){
	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	exit(EXIT_FAILURE);
    }
   
    initEverything();

    /* Setup Local Vars */
    pthread_t resolverThreads[MAX_RESOLVER_THREADS];
    pthread_t requesterThreads[inputFileCount];
    int rc;
    long ii;
    for(ii=0; ii < inputFileCount; ii++)
    {
      #if PRINT_DEBUG == 1 
      printf("Filename: %s\n", argv[ii+1]); 
      #endif
      rc = pthread_create(&(requesterThreads[ii]), NULL, Requester, (void*) argv[ii+1]);
      if (rc){
	  sprintf(errorstr, "ERROR; return code from pthread_create() is %d\n", rc);
	  perror(errorstr);
	  exit(EXIT_FAILURE);
      }
    }
    
    for(ii=0; ii < numThread; ii++)
    {
	rc = pthread_create(&(resolverThreads[ii]), NULL, Resolver, NULL);
	if (rc){
	    sprintf(errorstr, "ERROR; return code from pthread_create() is %d\n", rc);
	    perror(errorstr);
	    exit(EXIT_FAILURE);
	}
    }
    
    for(ii=0; ii < numThread; ii++)
    {
	int success = pthread_join(resolverThreads[ii],NULL);
	if(success != 0)
	{
	  printf("Error Detaching Pthreads in Resolver\n"); 
	} 
    }
    
    /* Wait for All Theads to Finish */
    for(ii=0; ii < inputFileCount; ii++)
    {
	int success = pthread_join(requesterThreads[ii],NULL);
	if(success != 0)
	{
	  printf("Error Detaching Pthreads in requester\n"); 
	}
    }
    
    #if PRINT_DEBUG == 1 
    printf("All of the threads were completed!\n");
    #endif
      
    destoryEverything();

    /* Last thing that main() should do */
    /* pthread_exit unnecessary due to previous join */ 
    // pthread_exit(NULL);
    

    return 0;
}
