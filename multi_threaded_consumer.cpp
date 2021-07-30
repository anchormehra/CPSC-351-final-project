/*
 the consumer process shall receive data, load it into the dynamic array of objects, 
 and then create n number of threads. The consumer program shall be ran using the 
 following command line:

./multi_threaded_consumer <Email> <Number of threads>

Each child thread shall search one of the n sections of the array objects for the 
specified student name.

If the thread finds the student record, 
it gets the amount of minutes for the particular date and their total.

In addition, each thread should be allowed to write their findings into the single text file 
studentreport.txt in this format: email, date, the total number of minutes.

The main threads, meanwhile, executes the threads''’'' join function.

After all threads complete their search, then the main thread prints 
"the student with <email> attended a total of N of minutes during the entire semester, 
and the following dates: date/minutes per class date.”

*/

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <ctime>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

using namespace std;

//buffer size
#define BUF_SIZE 1024
//shared memory key
#define SHM_KEY 0x1234
//maximum of records
#define MAX_RECORDS 100

//output file name
#define OUTPUT_FILENAME "studentreport.txt"

//mutext to proctect critical session (write file)
sem_t mutex;

//student structure
struct Student
{
    char email[BUF_SIZE];
    char fromDate[BUF_SIZE];
    char fromTime[BUF_SIZE];
    char toDate[BUF_SIZE];
    char toTime[BUF_SIZE];
};

/*
shared section
*/
struct ShareSection
{
    int count;
    int complete; //if shared memory has data
    Student students[MAX_RECORDS];
};

/*
argument of the threads
*/
struct ThreadArg{
    
    char email[BUF_SIZE];

    //common data to search
    Student* students;
    int numStudents; //number of students

    //the search range
    int fromPos;
    int toPos;

    //statistics 
    //each thread will write to one specific index
    int* minutes;
    int index;
};

/*
the function run for one thread
*/
void* doConsumerWork(void* data){

    ThreadArg* arg = (ThreadArg*)data;

    //search in the range assigned to this thread    
    for  (int i = arg->fromPos; i < arg->toPos; i++){

        //find email
        if (strcmp(arg->students[i].email, arg->email) == 0){
            
            //gets the amount of minutes for the particular date and their total

            //each thread should be allowed to write their findings into the single text file 
            //studentreport.txt in this format: email, date, the total number of minutes.
            int year1, month1, days1, hours1, minutes1;
            sscanf(arg->students[i].fromDate, "%d/%d/%d", &year1, &days1, &month1);
            sscanf(arg->students[i].fromTime, "%d:%d", &hours1, &minutes1);

            int year2, month2, days2, hours2, minutes2;
            sscanf(arg->students[i].toDate, "%d/%d/%d", &year2, &days2, &month2);
            sscanf(arg->students[i].toTime, "%d:%d", &hours2, &minutes2);

            //cout << year1 << month1 << days1 << hours1 << minutes1 << endl;
            //cout << year2 << month2 << days2 << hours2 << minutes2 << endl;

            //number of minutes
            int numMinutes = 0;    
            while (hours1 < hours2 || (hours1 == hours2 && minutes1 < minutes2)){
                numMinutes++;

                minutes1++;
                if (minutes1 == 60){
                    hours1 += 1;
                    minutes1 = 0;
                }
                
            }

            //write statistics
            arg->minutes[arg->index] += numMinutes;

            //write to file
            //protect shared file: do later

            //wait
            sem_wait(&mutex);

            ofstream outfile (OUTPUT_FILENAME, ios::app);
            if (outfile.is_open())
            {
                outfile << arg->email << " ";
                outfile << arg->students[i].fromDate << " ";
                outfile << numMinutes << endl;

                //close file
                outfile.close();
            }

            //signal
            sem_post(&mutex);
        }
    }

    return NULL;
}

/*
main function to start C++ application
it is consumer that read shared memory and
creates threads and process
*/
int main(int argc, char *argv[])
{
    //validate argument
    if (argc != 3)
    {
        printf("Please provide the email and number of threads\n");
        return 0;
    }

     //n thread
    int numThreads = atoi(argv[2]);

    if (numThreads <= 0){
        fprintf(stderr, "Number of threads must be positive number\n");
        return 1;
    }

    //cout << "numThreads" << numThreads << endl;

    //create array of thread handles
    pthread_t* threads = new pthread_t[numThreads]; 

    int shmid;
    struct ShareSection *shmp;
    shmid = shmget(SHM_KEY, sizeof(ShareSection), 0644 | IPC_CREAT);
    if (shmid == -1)
    {
        fprintf(stderr, "Shared memory");
        return 1;
    }

    //attach shared memory
    shmp = (ShareSection*)shmat(shmid, NULL, 0);
    if (shmp == (void *)-1)
    {
        fprintf(stderr, "Shared memory attach");
        return 1;
    }

    //wait for data
    while (shmp->complete != 1) {
        cout << "Wait for producer" << endl;
        usleep(1000000);
    }

    //create dynamic array of objects
    Student* students = new Student[shmp->count];

    //copy data
    for (int i = 0; i < shmp->count; i++)
    {
        strcpy(students[i].email, shmp->students[i].email); 
        strcpy(students[i].fromDate, shmp->students[i].fromDate); 
        strcpy(students[i].fromTime, shmp->students[i].fromTime); 
        strcpy(students[i].toDate, shmp->students[i].toDate); 
        strcpy(students[i].toTime, shmp->students[i].toTime); 
    }

    //search range for each thread
    int range = shmp->count / numThreads;

    //init semaphore
    sem_init(&mutex, 0, 1);

    //arguments
    ThreadArg* threadArgs = new ThreadArg[numThreads];

    //statistics
    int* minutes = new int[numThreads];

    for (int i = 0; i < numThreads; i++){
      strcpy(threadArgs[i].email, argv[1]);//copy email
      threadArgs[i].students = students;
      threadArgs[i].numStudents = shmp->count;
      threadArgs[i].fromPos = i * range;
      threadArgs[i].toPos = (i + 1) * range;

      threadArgs[i].minutes = minutes;
      threadArgs[i].minutes[i] = 0;
      threadArgs[i].index = i;

      if (i == numThreads - 1){ //argument for last thread
        threadArgs[i].toPos = shmp->count;
      }
    }

    //create and clear file
    ofstream outfile (OUTPUT_FILENAME);
    if (outfile.is_open())
    {
        //close file
        outfile.close();
    }

    //cout << "shmp->complete: " << shmp->complete << endl;
    //cout << "shmp->count: " << shmp->count << endl;
    //cout << "range: " << range << endl;   

    //create and run threads
    for (int i = 0; i < numThreads; i++){
        pthread_create( &threads[i], NULL, doConsumerWork, (void*) (&threadArgs[i]));
    }

    //wait for threads
    for (int i = 0; i < numThreads; i++){
        pthread_join(threads[i], NULL);
    }

    //write statistics
    int totalMinutes = 0;
    for (int i = 0; i < numThreads; i++){
        totalMinutes += threadArgs[i].minutes[i];
    }

    printf("the student with %s attended a total of %d of minutes during the entire semester,\n",
        argv[1], totalMinutes);

    //print details
    //read file
    ifstream inputFile(OUTPUT_FILENAME);

    //check file canbe opened
    if (!inputFile.is_open())
    {
        printf("Could not open input file\n");
        return 0;
    }

    //count the number of lines
    string line;
    while (getline(inputFile, line))
    {
        cout << line << endl;
    }

    //close file
    inputFile.close();

    //done reading shared memory
    shmp->complete = 0;

    //free resources
    delete [] students;
    delete [] threads;
    delete [] minutes;

    return 0;
}
