#include "headers.h"


int main(int argc, char * argv[])
{
    initClk();

    int AlgoNumber;
    int QuantumNumber;
    int ProcessesCount;

    ProcessesCount=atoi(argv[1]);
    AlgoNumber=atoi(argv[2]);
    QuantumNumber=atoi(argv[3]);


    
    
    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    
    destroyClk(true);
}
