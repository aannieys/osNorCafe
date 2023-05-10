// ITCS343 Principles of Operating Systems
// Implement a simulation of Nor’s roastery

// Members:
// Miss Suphavadee	    Cheng 		6488120
// Miss Ponnapassorn	Iamborisut 	6488179

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <stdbool.h>

#define SIZE 20

int k; // number of beans
int m; // number of roaster machine
int n; // number of resting machine

int *roaster; // buffer size 20
int *resting; // buffer size 20

int useRO = 0;  // use roaster buffer
int fillRO = 0; // fill roaster buffer

int useRE = 0;  // use resting buffer
int fillRE = 0; // fill resting buffer

int first = 0; // use this to check whether all of the beans is all done or not in rest machines...

// set of semaphore for protect buffer of roaster buffer
sem_t empty1;
sem_t full1;
sem_t mutex1;

// set of semaphore for protect buffer of resting buffer
sem_t empty2;
sem_t full2;
sem_t mutex2;

// total time
time_t startT; // compute total time: start running all process
time_t endT;   // compute total time: finish running all process

// turnaround time
time_t startTA[500] = {0};
time_t endTA[500] = {0};
long long int TA[500] = {0}; //end-start

// wait time
time_t startWT[500] = {0};
time_t endWT[500] = {0};
long long int WT[500] = {0}; //end-start

// count for index
int countTA = -1; // count index for Turnaround time
int countWT = 0;  // count index for Wait time

// ฺBONUS
// utilization of the roaster and resting machine
long int roasterUti = 0.0;
long int restingUti = 0.0;

// check for the last machine that's finish all of this process
// (the last machine that get terminate by -1 in roasterMachine function)
int last = 0; 

void fill_roaster(int val)
{
    roaster[fillRO] = val;
    fillRO++;
    if (fillRO == SIZE) 
        fillRO = 0;
}

int use_roaster()
{
    int tmp = roaster[useRO];
    useRO++;
    if (useRO == SIZE) 
        useRO = 0;
    return tmp;
}

void fill_resting(int val)
{
    resting[fillRE] = val;
    fillRE++;
    if (fillRE == SIZE) 
        fillRE = 0;
}

int use_resting()
{
    int tmp = resting[useRE];
    useRE++;
    if (useRE == SIZE) 
        useRE = 0;
    return tmp;
}

void *producer(void *arg)
{
    int i;

    time_t t;
    srand((unsigned) time(&t)); // makes the random[rand()] numbers come out unique

    time(&startT); // start total time: the first bean will start here in the loop of i=0
    for (i = 0; i < k; i++) // produce k beans and fill it in the roaster buffer
    {
        time(&startTA[i]); // turn around time
        time(&startWT[i]); // #1[start] start wait time of filling in the roaster buffer
       
        sem_wait(&empty1);
        sem_wait(&mutex1);

        int num = (rand()%3); // num use for keep value
        // print the coffee bean number and show its type
        if (num == 0)
        {
            printf("Fill bean no.%d [lighter]\n", i + 1);
        }
        else if (num == 1)
        {
            printf("Fill bean no.%d [medium]\n", i + 1);
        }
        else
        {
            printf("Fill bean no.%d [dark]\n", i + 1);
        }
        time(&endWT[i]);                  // #1[end] end wait time of filling in the roaster buffer
        WT[i] += (endWT[i] - startWT[i]); // #1 compute interval time of waiting for fill in roaster buffer
        
        // ------------------------------------------------------
        fill_roaster(num); // ***** CRITICAL REGION *****
        // ------------------------------------------------------

        time(&startWT[i]);                // #2[start] start wait time of going in the roaster machine
        sem_post(&mutex1);
        sem_post(&full1);
    }
    printf(" === End Filling All Beans in Roaster Buffer ===\n");

    // after filling every beans, we fill -1 for terminate the loop in roasterMachine
    // this -1 value will go through the both of buffer (roaster & resting)
    for (i = 0; i < m; i++)
    {
        sem_wait(&empty1);
        sem_wait(&mutex1);

        // ------------------------------------------------------
        fill_roaster(-1); // ***** CRITICAL REGION *****
        // ------------------------------------------------------

        sem_post(&mutex1);
        sem_post(&full1);
    }

    return NULL;
}

void *roasterMachine(void *arg) // this acts like a producer and consumer.
{
    int tmp = 0;
    while (tmp != -1)
    {
        sem_wait(&full1);
        sem_wait(&mutex1);

        // ------------------------------------------------------
        tmp = use_roaster(); // ***** CRITICAL REGION *****
        // ------------------------------------------------------

        time(&endWT[countWT]);                              // #2[end] end wait time of going in the roaster machine
        WT[countWT] += (endWT[countWT] - startWT[countWT]); // #2 compute interval time of waiting for going in the roaster machine

        printf("Roast: beans %d\n", tmp); // print the item in the buffer of roaster machine that is selected to roast in this machine
        sem_post(&mutex1);
        sem_post(&empty1);

        time_t utilizeS, utilizeE; // use for calculate BONUS 
        
        time(&utilizeS); // utilizeS => start
        if (tmp == 0)
        {
            sleep(1); // light
        }
        else if (tmp == 1)
        {
            sleep(2); // med
        }
        else if (tmp == 2)
        {
            sleep(3); // dark
        }
        time(&utilizeE); // utilizeE = > end
        roasterUti += (utilizeE - utilizeS); // roasterUti will keep all of the beans utilization in this machine (end-start)

        time(&startWT[countWT]);                            // #3[start] start wait time of filling in the resting buffer
        sem_wait(&empty2);
        sem_wait(&mutex2);
        time(&endWT[countWT]);                              // #3[end] end wait time of filling in the resting buffer
        WT[countWT] += (endWT[countWT] - startWT[countWT]); // #3 compute interval time of waiting for fill in resting buffer
        
        // ------------------------------------------------------
        fill_resting(tmp); // ***** CRITICAL REGION *****
        // ------------------------------------------------------
        
        time(&startWT[countWT]);                            // #4[start] start wait time of going in the resting machine
        sem_post(&mutex2);
        sem_post(&full2);
    }

    last++;
    if(last==m && tmp==-1 && n>m){ // this loop will add -1 for stop other resting machine 
                                   // this will happen in case of n>m because -1 is not enough
                                   // for some resting machine. It will add -1 when the finished
                                   // running of the last roasting maching.
      printf("Last Machine (that's finish) : %d | item: %d\n",(int *)(arg+1),tmp);
      // item should be -1
      // 0 < arg < m  
        for (int i = 0; i < n-m; i++) // it will add more n-m slot of -1
        {
            sem_wait(&empty2);
            sem_wait(&mutex2);
            
            // ------------------------------------------------------
            fill_resting(-1); // ***** CRITICAL REGION *****
            // ------------------------------------------------------
            
            sem_post(&mutex2);
            sem_post(&full2);
        }
    }
    return NULL;
}

void *restingMachine(void *arg)
{
    int tmp = 0;

    while (tmp != -1)
    {
        sem_wait(&full2);
        sem_wait(&mutex2);

        // ------------------------------------------------------
        tmp = use_resting(); // ***** CRITICAL REGION *****
        // ------------------------------------------------------

        time(&endWT[countWT]);                              // #4[end] end wait time of going in the resting machine
        WT[countWT] += (endWT[countWT] - startWT[countWT]); // #4 compute interval time of waiting for going in the resting machine

        // print out to show that each coffee beans use how much time to wait
        if(countWT<k && countWT>=0){ 
          printf("finish wait no.%d: %ld seconds\n", countWT+1, WT[countWT]);
        }
        
        countWT++; // increase the index for setting the Wait time of each bean
        printf("Rest: beans %d\n", tmp);
        int count_TA = countTA++; // use this to keep the index for setting the Turnaround time of each bean
        
        sem_post(&mutex2);
        sem_post(&empty2);

        time_t utilizeS, utilizeE; // use for calculate BONUS 

        time(&utilizeS); // utilizeS => start
        if (tmp == 0)
        {
            sleep(3); // light
        }
        else if (tmp == 1)
        {
            sleep(2); // med
        }
        else if (tmp == 2)
        {
            sleep(1); // dark
        }

        time(&endTA[count_TA]); // end of turnaround time (finish execution of this bean)

        time(&utilizeE); // utilizeE => end
        restingUti += (utilizeE - utilizeS); // restingUti will keep all of the beans utilization in this machine (end-start)
       
        TA[count_TA] = endTA[count_TA] - startTA[count_TA]; // calculate time of turnaround time of this bean
        
        // print out to show that each coffee beans use how much time to wait
        if(count_TA<k && count_TA>=0){
        //  printf("%d :: %d",count_TA+1, TA[count_TA+1]);
          printf("finish turnaround time no.%d: %ld seconds\n", count_TA+1, TA[count_TA]);
        }
        
    }
    time(&endT); // stop

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <beans> <roaster_machines> <resting_machines>\n", argv[0]);
        exit(1);
    }

    k = atoi(argv[1]); // number of beans
    m = atoi(argv[2]); // number of roaster machines
    n = atoi(argv[3]); // number of resting machines

    printf(" * number of beans: %d\n", k);
    printf(" * number of roaster machines: %d\n", m);
    printf(" * number of resting machines: %d\n\n", n);
    roaster = (int *)malloc(SIZE * sizeof(int)); // space of this buffer for 20 slots
    resting = (int *)malloc(SIZE * sizeof(int)); // space of this buffer for 20 slots

    //assert(roaster != NULL && resting != NULL);

    int i;
    for (i = 0; i < SIZE; i++)
    {
        roaster[i] = 0;
        resting[i] = 0;
    }

    sem_init(&empty1, 0, SIZE); // the roaster buffer has 20(SIZE) empty slots
    sem_init(&full1, 0, 0);     // the roaster buffer has 0 slots, that's mean it is no full
    sem_init(&mutex1, 0, 1);    // mutex

    sem_init(&empty2, 0, SIZE); // the resting buffer has 20(SIZE) empty slots
    sem_init(&full2, 0, 0);     // the resting buffer has 0 slots, that's mean it is no full
    sem_init(&mutex2, 0, 1);    // mutex

    pthread_t pid, bid[m], cid[n];

    pthread_create(&pid, NULL, producer, NULL);
    for (i = 0; i < m; i++)
    {
        //pthread_create(&bid[i], NULL, roasterMachine, NULL); //for Ubuntu
        pthread_create(&bid[i], NULL, roasterMachine, (void *)i);
    }
    for (i = 0; i < n; i++)
    {
        pthread_create(&cid[i], NULL, restingMachine, NULL);
    }

    pthread_join(pid, NULL);
    for (i = 0; i < m; i++)
    {
        pthread_join(bid[i], NULL);
    }
    for (i = 0; i < n; i++)
    {
        pthread_join(cid[i], NULL);
    }

    sem_destroy(&mutex1);
    sem_destroy(&mutex2);

    printf("\n:: Calculation Time ::\n");
    printf("  - Total Time:               %ld seconds\n", (endT - startT));

    time_t sumTA = 0; // sum of turnaround time
    for (int i = 0; i < k; i++)
    {
        sumTA += TA[i];
    }
    printf("  - Average Turnaround Time:  %.2f seconds\n", sumTA * 1.0 / k);

    time_t sumWT = 0; // sum of wait time
    for (int i = 0; i < k; i++)
    {
        sumWT += WT[i];
    }
    printf("  - Average Wait Time:        %.2f seconds\n", sumWT * 1.0 / k);

    // BONUS
    printf("\n                 ::: BONUS :::\n");
    printf("Utilization time of Roaster Machines was %ld seconds\n", roasterUti);
    printf("Utilization time of Resting Machines was %ld seconds\n\n", restingUti);
}