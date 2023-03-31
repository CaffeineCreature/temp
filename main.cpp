#include <iostream>
#include <string>
#include <random>
#include <cmath>
#include <ctime>
#include <queue>
#include <vector>

// Event states
constexpr uint32_t EVENT_ARRIVAL = 1;
constexpr uint32_t EVENT_CPU_COMPLETE = 2;
constexpr uint32_t EVENT_DISK_COMPLETE = 4;
constexpr uint32_t EVENT_EXIT = 8;

// Process event to report state change
struct event {
    double time;
    uint32_t pid;
    uint32_t eventType;
};

struct process{
    uint32_t pid;
    double arrivalTime;
    double completionTime;
};

struct stats{
    double turnaround;
    double throughput;
    double cpuUtilization;
    double diskUtilization;
    double readyQueueWaiting;
    double diskQueueWaiting;
};

// poissonDelay Generates a random delay based on the Poisson distribution
// input: lambda rate
// output: delay in seconds
inline double poissonDelay(const double lambda){
    // Adapted to fit the edge cases of std::rand() value. Absolute pain to debug :(
    return -log(1.0-((double)(std::rand()+1)/(RAND_MAX+2)))/lambda;
}

// runSimulation Runs the simulated system until n cycles are completed
// input: n number of processes to complete, lambda arrival rate, out .csv output destination
void runSimulation(const double lambda, const double cpuMean, const double diskMean){
    double simTime = 0.0;
    const int n = 10000;
    int completionCount = 0;
    process nextArrival = {0,0.0,0.0};
    //std::vector<event> eventLog;
    stats simStats = stats{0,0,0,0,0,0};

    // CPU state
    std::queue<process> cpuQueue;
    double cpuCompleteTime = 0.0;

    // Disc state
    std::queue<process> diskQueue;
    double diskCompleteTime = 0.0;

    // Time step is set based on the next event completion
    while(completionCount < n) {
        // Simulate arrival rate
        if(nextArrival.arrivalTime <= simTime){
            if(cpuQueue.empty())
                cpuCompleteTime = simTime + poissonDelay(1/cpuMean);

            cpuQueue.push(nextArrival);
            //eventLog.push_back(event{nextArrival.arrivalTime, nextArrival.pid,EVENT_ARRIVAL});

            //std::cout << "ARIV: pid" << nextArrival.pid << "@t="<< simTime << "\n";
            nextArrival = process{nextArrival.pid + 1,
                                  nextArrival.arrivalTime + poissonDelay(lambda),
                                  0.0};
        }
        // Simulate CPU completion
        if(!cpuQueue.empty() && cpuCompleteTime <= simTime){
            if(((double)std::rand()/RAND_MAX) <= 0.6){
                // Program exit
                completionCount++;
                //eventLog.push_back(event{cpuCompleteTime,
                //                         cpuQueue.front().pid,
                //                         EVENT_EXIT});
                //std::cout << "EXIT: pid" << cpuQueue.front().pid << "@t="<< cpuCompleteTime<< "\n";

                simStats.throughput++;
                simStats.turnaround += cpuCompleteTime - cpuQueue.front().arrivalTime;
            } else { //Program request disk
                if(diskQueue.empty())
                    diskCompleteTime = simTime + poissonDelay(1/diskMean);
                //std::cout << "QUED: pid" << cpuQueue.front().pid << "@t="<< cpuCompleteTime<< "\n";
                diskQueue.push(cpuQueue.front());
            }
            cpuQueue.pop();
            if(!cpuQueue.empty())
                cpuCompleteTime = simTime + poissonDelay(1/cpuMean);

        }
        // Simulate Disk completion
        if(!diskQueue.empty() && diskCompleteTime <= simTime){
            // Place in CPU queue
            if(cpuQueue.empty())
                cpuCompleteTime = simTime + poissonDelay(1/cpuMean);
            cpuQueue.push(diskQueue.front());


            //std::cout << "CMPD: pid" << diskQueue.front().pid << "@t="<< diskCompleteTime<< "\n";
            diskQueue.pop();
            if(!diskQueue.empty())
                diskCompleteTime = simTime + poissonDelay(1/diskMean);
        }

        // Update simTime
        double deltaTime = simTime;
        simTime = nextArrival.arrivalTime;
        if(!cpuQueue.empty()) {
            simTime = std::min(simTime, cpuCompleteTime);
        }
        if(!diskQueue.empty()) {
            simTime = std::min(simTime, diskCompleteTime);
        }

        // Utilization stats
        deltaTime = simTime-deltaTime;
        if(!cpuQueue.empty()) {
            simStats.cpuUtilization += deltaTime;
        }
        if(!diskQueue.empty()) {
            simStats.diskUtilization += deltaTime;
        }
        // Accumulate queue stats
        // Item at front of queue is being processed, not counted
        simStats.readyQueueWaiting += deltaTime * (double)std::max((int)cpuQueue.size()-1,0);
        simStats.diskQueueWaiting += deltaTime * (double)std::max((int)diskQueue.size()-1,0);
    }

    // Normalize stats
    simStats.turnaround = simStats.turnaround/n;
    simStats.throughput = simStats.throughput/simTime;
    simStats.cpuUtilization = simStats.cpuUtilization/simTime;
    simStats.diskUtilization = simStats.diskUtilization/simTime;
    simStats.readyQueueWaiting = simStats.readyQueueWaiting/simTime;
    simStats.diskQueueWaiting = simStats.diskQueueWaiting/simTime;

    // Print result
    std::cout << "\nLambda[" << lambda << "]\n";
    std::cout << "Turnaround:" << simStats.turnaround << "s\n";
    std::cout << "Throughput:" << simStats.throughput << "/s\n";
    std::cout << "CPU:" << simStats.cpuUtilization*100 << "%\n";
    std::cout << "Disk:" << simStats.diskUtilization*100 << "%\n";
    std::cout << "CPU Queue:" << simStats.readyQueueWaiting << "\n";
    std::cout << "Disk Queue:" << simStats.diskQueueWaiting << "\n";

    /*//CSV Style
    std::cout << "Lambda[" << lambda << "],";
    std::cout << simStats.turnaround << "," << simStats.throughput<< ","<< simStats.cpuUtilization*100<< ","
    << simStats.diskUtilization*100 << ","<< simStats.readyQueueWaiting<< ","<< simStats.diskQueueWaiting << "\n";
     */
}

int main(int argc, char** argv) {
    std::srand(std::time(nullptr));
    if(argc < 4) {
        std::cout << "---HELP---\n";
        std::cout << "Writen by Christopher Pearson 2023\n";
        std::cout << "CS4328\n";
        std::cout << "Command: $simulator #lambda #CPUTime #DiskTime\n";
        std::cout << "Example: $simulator 12 0.02 0.06\n";
        std::cout << "----------\n";
    } else {
        double lambda = std::stod(argv[1]);
        double cpuMean = std::stod(argv[2]);
        double diskMean = std::stod(argv[3]);
        runSimulation(lambda, cpuMean, diskMean);
    }
    return 0;
}
