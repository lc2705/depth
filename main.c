
#include "ac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#define MAXLEN 1502 
#define MAXLINE 10000
#define MAXPATTERNLEN 202

/*
*  Text Data Buffer
*/

unsigned char text[MAXLINE][MAXLEN];   
unsigned char pattern[MAXPATTERNLEN]; 
unsigned int line = 0;
unsigned int *valid_len_array;   //record the validation stage length of each thread except 
                                 //for the last one which record the fragment length

unsigned int        thread_num = 1;
pthread_t           *thread_array;
ACSM_STRUCT         *acsm;

void* SearchThread(void* args);

int main (int argc, char **argv) 
{
    int i,j;
    unsigned int total_len = 0;
    struct timeval begtime,endtime;
    FILE *sfd,*pfd;
    char sfilename[20] = "string";
    char pfilename[20] = "pattern";
    
//=============================================== 
    if (argc < 4)
    {
        fprintf (stderr,"Usage: acsmx stringfile patternfile thread_num...  -nocase\n");
        exit (0);
    }
    strcpy (sfilename, argv[1]);
    sfd = fopen(sfilename,"r");
    if(sfd == NULL)
    {
        fprintf(stderr,"Open file error!\n");
        exit(1);
    }

    strcpy(pfilename,argv[2]);
    pfd = fopen(pfilename,"r");
    if(sfd == NULL)
    {
        fprintf(stderr,"Open file error!\n");
        exit(1);
    }
    thread_num = atoi(argv[3]);

   	acsm = acsmNew (thread_num); 
   
//read patterns    
	i = 0;
    while(fgets(pattern,MAXPATTERNLEN,pfd))
    {
    	int len = strlen(pattern);
    	acsmAddPattern (acsm, pattern, len-1);
		i++;
    }
    fclose(pfd);
    printf("\n\nread %d patterns\n\n===============================",i);
    /* Generate GtoTo Table and Fail Table */
    acsmCompile (acsm);
//========================================================= 

    /*read string*/
    for(i = 0;i < MAXLINE;i++)
    {
    	if(!fgets(text[i],MAXLEN,sfd))
    		break;
   		total_len += strlen(text[i]) - 1; //ignore the last char '\n'
    }
    line = i;
    fclose(sfd);
    printf("\n\nreading finished...\n=============================\n\n");
    printf("%d lines\t%dbytes",line,total_len);
    printf("\n\n=============================\n");
    //begin time
    gettimeofday(&begtime,0);

    //create multi_thread
    thread_array = calloc(thread_num,sizeof(pthread_t));
    valid_len_array = calloc(thread_num,sizeof(unsigned int));
	for(i = 0;i < thread_num; i++)
	{
		pthread_create(&thread_array[i], NULL, SearchThread, (void*)i);
    }
    //sleep(5);
//=========================================================== 
    int err;
    for(i = 0;i < thread_num;i++)
    {
        err = pthread_join(thread_array[i],NULL);
        if(err != 0)
        {
            printf("can not join with thread %d:%s\n", i,strerror(err));
        }
    }
    //end time
    gettimeofday(&endtime,0);

    PrintSummary(acsm);
    acsmFree (acsm);

    printf ("\n### AC Match Finished ###\n");
    printf ("\nTime Cost : %lu us \n\n",(endtime.tv_sec - begtime.tv_sec)*1000000 + (endtime.tv_usec - begtime.tv_usec));
    printf ("\n====================================\n\n");
    printf ("Validation Stage Len:\n\n");
    for(i = 0;i < thread_num;i++)
        printf("rank%d\t%u\n",i,valid_len_array[i]);
    printf ("\n====================================\n\n");
   
    free(valid_len_array);
    free(thread_array);
    return 0;
}

void* SearchThread(void* args)
{
	unsigned int rank = (unsigned int)args;
    int i,state;
    unsigned int index,index_end,len,vlen;

	for(i = 0;i < line;i++)
	{
		state = 0;
		vlen = 0;
		len = strlen(text[i]) - 1;
		index = len/thread_num * rank;
		if(rank != thread_num - 1)
			index_end = index + len/thread_num;
		else
			index_end = len;
		for( ;index < index_end;index++)
		{
			acsmSearch(acsm,&text[i][index],&state,rank,PrintMatch);
			vlen++;
		}
		if(rank != thread_num - 1)
		{
			vlen = 0;
			for( ;index < len; index++)
 			{
		 		if(acsmSearchWithDepthCompare(acsm,&text[i][index],&state,vlen,rank,PrintMatch) == -1)
		 			break;
	 			vlen++;
 			}
		}		
        valid_len_array[rank] += vlen;
	}
}
