//
// Created by Jared Krogsrud on 3/30/2023.
//

#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define BUFLEN 32
#define SHBUFLEN 10
#define PROMPT "> "

typedef struct {
    char buf[SHBUFLEN]; // The buffer
    int in;             // Location to which producer will write
    int out;            // Location to which consumer will read
    int counter;        // total # items in the buffer
    int numWritten;     // # items that producer has written
    int numRead;        // # items that consumer has read
    int done;           // Whether task is complete or not
    pthread_mutex_t lock; // The lock
} ThreadInfo;

void *producer(void *param)
{
    ThreadInfo *tinfo = (ThreadInfo *) param;
    int done = 0;
    int len; // length of string of user input
    int numWritten; // local variable keeping track of how many chars have been written
    char toCopy;

    while (! done)
    {
        // Read input string
        char buffer[BUFLEN];
        char* chp;

        printf("%s", PROMPT);

        chp = fgets(buffer, BUFLEN, stdin);
        if (chp != NULL)
        {
            len = strlen(buffer);
            // Strip the end character from the buffer replace it
            if (buffer[len-1] == '\n')
            {
                buffer[len-1] = '\0';
                len = len - 1;
            }
            // Check that the input was not 'quit' or 'exit'
            if ( ! strcmp(buffer, "quit") || ! strcmp(buffer, "exit") )
            {
                pthread_mutex_lock(&tinfo->lock);
                tinfo->done = 1;
                done = 1;
                pthread_mutex_unlock(&tinfo->lock);
            }
            else
            {
                numWritten = 0;
                // Loop through the input buffer and insert 1 character at a time
                while (numWritten < len)
                {
                    pthread_mutex_lock(&tinfo->lock); // Acquire Lock
                    // Check if buffer can currently be written to
                    if (tinfo->counter != SHBUFLEN)
                    {
                        // Buffer is not full so write to the next location
                        toCopy = buffer[numWritten];
                        tinfo->buf[tinfo->in] = toCopy;
                        printf("producer writes %c\n", toCopy);
                        //printf("producer writes %c to location %d\n", toCopy, tinfo->in);
                        tinfo->numWritten += 1;
                        numWritten += 1;
                        tinfo->counter += 1; // increase items in shared buffer
                        //printf("producer reports counter at %d\n", tinfo->counter);
                        tinfo->in += 1;
                        // If we reach the end of the buffer loop to beginning
                        if (tinfo->in == SHBUFLEN)
                        {
                            //printf("producer has reached end of shared buffer, resetting in\n");
                            tinfo->in = 0;
                        }
                    }
                    pthread_mutex_unlock(&tinfo->lock);
                }
            }
        }
    }
    printf("producer is exiting\n");
}

void *consumer(void *param)
{
    ThreadInfo *tinfo = (ThreadInfo *) param;
    int done = 0;
    char toPrint;

    while (!done)
    {
        // Check if shared flag is set to done or not
        pthread_mutex_lock(&tinfo->lock);
        if (tinfo->done)
        {
            done = 1;
        }
        else
        {
            if (tinfo->counter > 0)
            {
                toPrint = tinfo->buf[tinfo->out];
                printf("consumer read %c\n", toPrint);
                //printf("consumer reads %c from location %d\n", toPrint, tinfo->out);
                tinfo->counter -= 1;
                //printf("consumer reports counter at %d\n", tinfo->counter);
                tinfo->numRead += 1;
                tinfo->out += 1;
                if (tinfo->out == SHBUFLEN)
                {
                    //printf("consumer has reached end of shared buffer, resetting out\n");
                    tinfo->out = 0;
                }
            }
        }
        pthread_mutex_unlock(&tinfo->lock);
    }
    printf("consumer is exiting\n");
}

int main() {
    ThreadInfo tinfo;
    pthread_t producerTid, consumerTid;

    pthread_mutex_init(&tinfo.lock, NULL);

    // Set up base values
    tinfo.in = 0;
    tinfo.out = 0;
    tinfo.counter = 0;
    tinfo.numWritten = 0;
    tinfo.numRead = 0;
    tinfo.done = 0;

    pthread_create(&producerTid, NULL, producer, &tinfo);
    pthread_create(&consumerTid, NULL, consumer, &tinfo);

    pthread_join(producerTid, NULL);
    pthread_join(consumerTid, NULL);

    printf("producer wrote %d times\n", tinfo.numWritten);
    printf("consumer read %d times\n", tinfo.numRead);

    return 0;
}