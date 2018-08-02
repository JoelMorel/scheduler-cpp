//  Created by Joel Morel on 4/5/18.
//  Copyright © 2018 Joel Morel. All rights reserved.
//

#include <iostream>
#include <fstream>
using namespace std;

ifstream inFile1, inFile2;
ofstream outFile;

class Node
{
    friend class Schdueler;
    
public:
    int jobId;
    int jobTime;
    Node* next = NULL;
    
    Node()
    {
        jobId = 0;
        jobTime = 0;
        next = NULL;
    }
    
    Node(int id, int time, Node* next)
    {
        jobId = id;
        jobTime = time;
        next = NULL;
    }
};

class Schdueler
{
    friend class Node;
    
public:
    int numNodes, currentTime, procUsed, procsGiven; // the total number of nodes in the input graph.
    int totalJobTimes = 0; // the total of nodes's times in the input graph.
    int** adjacencyMatrix;
    // a 2-D integer array, size numNodes+1 by numNodes+1,
    // representing the input dependency graph, need to be dynamically allocated;
    // adjacencyMatrix[i][j] >= 1, means job j is a dependent of job i.
    
    int** scheduleTable;
    // a 2-D integer array, (need to be dynamically allocated)
    // to record the schedule. The dimension should be determined at run time,
    // (numNodes +1) by (totalJobTimes +1)
    
    int* jobTimeAry;
    // an 1-D array (intialize to 0) to store the time requirement for each job; get it from input2
    // If you like, you can also store the job time of i, in adjacencyMatrix[i][i]
    
    Node* OPEN;
    // OPEN is a linked list (with a dummy node), to store jobs that do not have any parents.
    // *** For the simplicity for this project, the nodes in OPEN
    // will be ordered by the node's jobTime (shorter time in the front.)
    // Better yet (not for this project), order by the # of dependants.
    
    int* processJob; // an 1-D array (intialize to 0) to indicate
    // which job is being processed by processor[i].
    //  i.e., if processJob[i] is 5,
    // it means processor,i, is currently processing job 5.
    
    int* processTime; // an 1-D array (intialize to 0) to keep track of
    // the processing time remiain on processor, i,
    // When ProcessTime[i] > 0, it means the processor, i, is busy
    // When ProcessTime[i] <= 0, it means the processor, i, is available.
    
    int* parentCount; // an 1-D array (intialize to 0) to store number of parents of each job
    // (0 means no parent, an orphan.)
    // the sum of each *column j* in the matrix is the total parent counts of node j.
    
    int* jobDone; // an 1-D array (intialize to 0) to keep track which jobs remain in the graph
    // jobDone[i] == 1 means job i has been deleted from the graph,
    // == 0 means still in the graph.
    
    int* jobMarked; // an 1-D array (intialize to 0) to keep track if jobs are currently on Open or
    // on processor.(1 means is on OPEN/rocessor, 0 means is not).
    
    // The dimension of all 1-D arrays should be (numberNodes + 1) array index 0 will not be used.
    
    Schdueler(ifstream& inFile)
    {
        loadMatrix(inFile);
        computeTotalJobTimes();
        
        cout << "How many procs do you want to use? " << endl;
        cin >> procsGiven;
        
        if(procsGiven <= 0)
        {
            cout << "Error, there are cycles. Exiting..." << endl;
            exit(0);
        }
        
        if(procsGiven > numNodes)
        {
            procsGiven = numNodes;
        }
        
        OPEN = NULL;
        OPEN = new Node(0,0,NULL);
        OPEN = OPEN->next;
        
        processJob = new int[numNodes+1];
        processTime = new int[numNodes+1];
        parentCount = new int[numNodes+1];
        jobDone = new int[numNodes+1];
        jobMarked = new int[numNodes+1];
        scheduleTable = new int*[numNodes+1];
        for(int i = 0; i < numNodes+1; i++)
        {
            scheduleTable[i] = new int[totalJobTimes+1];
        }
        
        for(int i = 0; i < numNodes+1; i++)
        {
            for(int j = 0; j < totalJobTimes+1; j++)
            {
                scheduleTable[i][j] = 0;
            }
        }
        
        for(int i = 0; i <= numNodes; i++)
        {
            processJob[i] = 0;
            processTime[i] = 0;
            parentCount[i] = 0;
            jobDone[i] = 0;
            jobMarked[i] = 0;
        }
        
        for(int i = 0; i < numNodes+1; i++)
        {
            for(int j = 0; j < numNodes+1; j++)
            {
                if(adjacencyMatrix[i][j] > 0)
                {
                    parentCount[j]++;
                }
            }
        }
        
        cout << "Parent count array: " << endl;
        for(int i = 1; i < numNodes+1; i++)
        {
            cout << parentCount[i] << " ";
        }
        cout << endl;
        
        
        procUsed = 0;
        currentTime = 0;
        
        do
        {
        // Step 1: Get orphan nodes
        // Step 2: Repeat step 1 until there no more orphan nodes
        int orphanNode = 0;
        while(orphanNode != -1) // get stuck here
        {
            orphanNode = getUnMarkOrphan();
            cout << "Orphan Node: " << orphanNode << endl;
            if(orphanNode != -1)
            {
                jobMarked[orphanNode] = 1;
                Node* newNode = new Node(orphanNode,jobTimeAry[orphanNode], NULL);
                insertOpen(newNode);
            }
        }
        
        // Step 3: Print list
        printList(OPEN);
        
        // Step 4: Check to see if there are any available processors
        // Step 5: Repeat step 4 while OPEN is not empty AND ProcUsed < ProcGiven
        while(OPEN != NULL && procUsed < procsGiven)
        {
            //cout << "inside while loop " << procUsed << " " << procsGiven << endl;
            int availableProc = findProcessor();
            
            if(availableProc > 0)
            {
                //cout << "inside if statement";
                procUsed++;
                Node* newJob = OPEN; // remove from the front of OPEN list
                OPEN = OPEN->next;
                processJob[availableProc] = newJob->jobId;
                processTime[availableProc] = newJob->jobTime;
                updateTable(availableProc, currentTime, newJob);
            }
            else break;
        }
        
        procUsed = 0;
        
            //cout << "Outside while loop";
        // Step 6: Check to see if there any cycles in the graph
        if(checkCycle())
        {
            cerr << "There are cycles in the graph. Exiting..." << endl;
            exit(0);
        }
        
        // Step 7: Print the table
        printTable();
        
        // Step 8: Increase the time by 1
        currentTime++;
        
        // Step 9: Decrease all processTime[i] by 1
        for(int i = 1; i < numNodes+1; i++)
        {
            if(processTime[i] > 0)
            {
                processTime[i]--;
            }
            
        }
        
        // Step 10: job <-- findDoneJob() // find a job that is done, ie., processTIME [i] == 0 ;
        // findDoneJob also deletes the job from the processJob[i] (set processJob[i] to 0)
        // deleteNode(job)
        // deleteEdge(job)
        for(int i = 1; i < numNodes+1; i++) // Step 11: repeat until no more finished jobs
        {
            int job = findDoneJob();
            if(job != -1)
            {
                deleteNode(job);
                deleteEdge(job);
            }
            
        }
        
        // Step 12: debugging print the following to the console with readable headings:
        
        cout << "Current Time: " << currentTime << endl;
        
        cout << "Job Marked Array: " << endl;
        for(int i = 1; i < numNodes+1; i++)
        {
            cout << jobMarked[i] << " ";
        }
        cout << endl;
        
        cout << "Process Time Array: " << endl;
        for(int i = 1; i < numNodes+1; i++)
        {
            cout << processTime[i] << " ";
        }
        cout << endl;
        
        cout << "Process Job Array: " << endl;
        for(int i = 1; i < numNodes+1; i++)
        {
            cout << processJob[i] << " ";
        }
        cout << endl;

        cout << "Job Done Array: " << endl;
        for(int i = 1; i < numNodes+1; i++)
        {
            cout << jobDone[i] << " ";
        }
        cout << endl;
        }
        while(!isGraphDone() && currentTime < totalJobTimes);
        // Step 13: repeat step 1 to step 12 until the graph is empty (i.e., jobDone[i] are == 1. )
        
        // Step 14: printTable()
        printTable();
    }
    
    
    void loadMatrix (ifstream& inFile)
    {
        inFile >> numNodes;
        
        adjacencyMatrix = new int*[numNodes+1];
        for(int i = 0; i < numNodes+1; i++)
        {
            adjacencyMatrix[i] = new int[numNodes+1];
        }
        
        for(int i = 0; i < numNodes+1; i++)
        {
            for(int j = 0; j < numNodes+1; j++)
            {
                adjacencyMatrix[i][j] = 0;
            }
        }
        
        int node = 0, dependency = 0;
        while(inFile1 >> node)
        {
            inFile1 >> dependency;
            
            for(int i = 0; i <= numNodes+1; i++)
            {
                for(int j = 0; j <= numNodes+1; j++)
                {
                    if(i == node && j == dependency)
                    {
                        adjacencyMatrix[i][j] = dependency;
                    }
                }
            }
        }
        
        for(int i = 1; i < numNodes+1; i++)
        {
            cout << "Node " << i << ": ";
            for(int j = 1; j < numNodes+1; j++)
            {
                cout << adjacencyMatrix[i][j] << " " ;
            }
            cout << endl;
        }
    }
    
    void computeTotalJobTimes ()
    {
        jobTimeAry = new int[numNodes+1];
        for(int i = 0; i < numNodes; i++)
        {
            jobTimeAry[i] = 0;
        }
        
        int i = 1, time = 0;
        inFile2 >> time;
        while(inFile2 >> time)
        {
            inFile2 >> time;
            jobTimeAry[i] = time;
            totalJobTimes += time;
            i++;
        }
        
        cout << "Total jobs time: " << totalJobTimes << endl;
    }
    
    int getUnMarkOrphan()
    {
        for(int i = 1; i < numNodes+1; i++)
        {
            if(jobMarked[i] == 0 && parentCount[i] == 0)
            {
                return i;
            }
        }
        return -1;
    }
    
    void insertOpen(Node* newNode)
    {
        if (OPEN == NULL)
        {
            OPEN = newNode;
            return;
        }
        
            Node* walker = OPEN;
            
            while(walker->next != NULL && walker->next->jobTime < newNode->jobTime)
            {
                walker = walker->next;
            }
            
            newNode->next = walker->next;
            walker->next = newNode;
        
    }
    
    void printList(Node* OPEN)
    {
        cout << "*** OPEN Job list in format (jobID, jobTime)--> ***" << endl;
        Node* walker = OPEN;
        while(walker != NULL) // bad exec
        {
            if(walker == NULL)
            {
                cout << "(" << walker->jobId << ", " << walker->jobTime << ")" << endl;
            }
            cout << "(" << walker->jobId << ", " << walker->jobTime << ") --> ";
            walker = walker->next;
        }
        cout << endl;
    }
    
    void printTable()
    {
        for (int i = 0; i < totalJobTimes + 1; i++)
        {
            outFile << "-" << i << "--";
        }
        
        outFile << endl;
        
        for (int i = 1; i <= procsGiven; i++)
        {
            outFile <<  "P" << "(" <<  i <<  ")";
            outFile << " ";
            
            for (int j = 1; j <= currentTime; j++)
            {
                outFile << scheduleTable[i][j] <<  " |";
            }
            outFile << endl;
        }
        outFile << endl << endl;
    }
    
    int findProcessor()
    {
        for(int i = 1; i <= procsGiven; i++)
        {
            if(processTime[i] <= 0)
            {
                return i;
            }
        }
        return -1;
    }
        
    void updateTable(int availProc, int currentTime, Node* newJob)
    {
        for(int i = currentTime; i <= currentTime+newJob->jobTime; i++ )
        {
            scheduleTable[availProc][i] = newJob->jobId;
        }
    }
    
    int checkCycle()
    {
        if(OPEN == NULL) // Problem here
        {
            if(!isGraphDone())
            {
                if(isAllProcsDone())
                {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    
                
    int findDoneJob()
    {
        for(int i = 1; i < numNodes+1; i++)
        {
            if(processTime[i] == 0)
            {
                int jobID = processJob[i];
                processJob[i] = 0;
                return jobID;
            }
        }
        return -1; // ???
    }
                
                
    void deleteNode(int node)
    {
        for(int i = 1; i < numNodes+1; i++)
        {
            if(i == node)
            {
                jobDone[node] = 1;
            }
        }
    }
                
                
    void deleteEdge(int node)
    {
        for(int kidIndex = 1; kidIndex < numNodes+1; kidIndex++)
        {
            if(adjacencyMatrix[node][kidIndex] > 0)
            {
                parentCount[kidIndex]--;
            }
        }
    }
    
    bool isAllProcsDone()
    {
        for(int i = 1; i < numNodes+1; i++)
        {
            if(processJob[i] > 0)
            {
                return false;
            }
        }
        
        return true;
    }
    
    bool isGraphDone()
    {
        for(int i = 1; i < numNodes+1; i++)
        {
            if(jobDone[i] != 1)
            {
                return false;
            }
        }
        
        return true;
    }
             
};

int main(int argc, const char * argv[])
{
    inFile1.open(argv[1]); // data
    inFile2.open(argv[2]); // time
    outFile.open(argv[3]);
    
    Schdueler* obj = new Schdueler(inFile1);

    inFile1.close();
    inFile2.close();
    outFile.close();
    return 0;
}
