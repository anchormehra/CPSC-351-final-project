/*
The producer process opens the text file:

3@csu.fullerton.edu      6/23/2021 20:54                6/23/2021 20:55
3@csu.fullerton.edu      6/17/2021 18:34                6/17/2021 18:36
1@csu.fullerton.edu      6/23/2021 17:53                6/23/2021 20:56
3@csu.fullerton.edu      6/14/2021 18:48                6/14/2021 19:50
4@csu.fullerton.edu      6/21/2021 17:56                6/21/2021 20:39
3@csu.fullerton.edu      6/21/2021 19:44                6/21/2021 19:50

Build

g++ producer.cpp -o producer

Run

./ producer <Filename>

*/
#include <iostream>
#include <fstream>
#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>
using namespace std;

//buffer size
#define BUF_SIZE 1024
//shared memory key
#define SHM_KEY 0x1234
//maximum of records
#define MAX_RECORDS 100

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
main function to start C++ application
it is producer that read file and write to the 
shared memory 
then the consumer process will read it
*/
int main(int argc, char *argv[])
{

    int shmid;
    ShareSection *shmp;
    string line;

    //validate argument
    if (argc != 2)
    {
        printf("Please provide the input file\n");
        return 0;
    }

    //read file and write to the shared session
    ifstream inputFile(argv[1]);

    //check file canbe opened
    if (!inputFile.is_open())
    {
        printf("Could not open input file\n");
        return 0;
    }

    //create the shared memory to store data
    shmid = shmget(SHM_KEY, sizeof(ShareSection), 0644 | IPC_CREAT);
    if (shmid == -1)
    {
        fprintf(stderr, "Shared memory");
        return 1;
    }

    //attach to the pointer
    shmp = shmat(shmid, NULL, 0);
    if (shmp == (void *)-1)
    {
        fprintf(stderr, "Shared memory attach");
        return 1;
    }

    shmp->count = 0;
    shmp->complete = 0;

    //count the number of lines
    string line;
    while (getline(inputFile, line))
    {
        cout << line << endl;
    }

    //close file
    inputFile.close();

    //read students from file and
    //write the shared session
    for (int i = 0; i < shmp->count; i++)
    {
        //for information for one student
        inputFile >> shmp->students[i].email;
        inputFile >> shmp->students[i].fromDate;
        inputFile >> shmp->students[i].fromTime;
        inputFile >> shmp->students[i].toDate;
        inputFile >> shmp->students[i].toTime;
    }

    shmp->complete = 1; //mark as completed

    //close file
    inputFile.close();

    //detaches the shared memory segment
    if (shmdt(shmp) == -1)
    {
        perror("shmdt");
        return 1;
    }

    return 0;
}
