#ifndef PROCSIM_HPP
#define PROCSIM_HPP

#include <cstdint>
#include <stdint.h>
#include <cstdio>
#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>

using namespace std;

#define DEFAULT_K0 1
#define DEFAULT_K1 2
#define DEFAULT_K2 3
#define DEFAULT_R 8
#define DEFAULT_F 4

typedef struct _proc_inst_t
{
    uint32_t instruction_address;
    int32_t op_code;
    int32_t src_reg[2];
    int32_t dest_reg;

    uint64_t tag;

    int fetch;
    int disp;
    int sched;
    int exec;
    int state;
} proc_inst_t;

typedef struct _proc_stats_t
{
    float avg_inst_retired;
    float avg_inst_fired;
    float avg_disp_size;
    unsigned long max_disp_size;
    unsigned long retired_instruction;
    unsigned long cycle_count;
} proc_stats_t;

typedef struct reg
{
    bool ready;
    int64_t tag;

    reg(){
        ready = true;
        tag = 0;
    }
} reg;

typedef struct {
    proc_inst_t original_instruction;
    bool in_use;
    int fu;
    int dest_reg;
    uint64_t dest_reg_tag;
    bool src1_ready;
    uint64_t src1_tag;
    bool src2_ready;
    uint64_t src2_tag;
    bool fired;
    bool completed;
    bool mark_for_delete;
} reservation_station;

typedef struct {
    int type;
    bool busy;
    uint64_t tag;
    uint64_t register_number;
    bool completed;
} function_unit;

typedef struct {
    bool busy;
    uint64_t tag;
    int register_number;
} result_bus;

class SchedulingQueue {
    vector<reservation_station>* scheduling_queue;
    int queue_size;
    public:
    SchedulingQueue(uint64_t k0, uint64_t k1, uint64_t k2){
        this->queue_size = 2 * (k0 + k1 + k2);
        scheduling_queue = new vector<reservation_station> (queue_size);
    };

    void printQueueSize(){
        cout << "Schedule Queue Size " <<  scheduling_queue->size() << endl;
    }
    void printQueue(){
        vector<reservation_station>::iterator entry;
        printf("in use\tfu\tdest_reg\tdest_reg_tag\tsrc1_ready\tsrc1_tag\tsrc2_ready\tsrc2_tag\tfired\t     completed\t      delete\t\n");
        for(entry=scheduling_queue->begin(); entry != scheduling_queue->end(); ++entry){
            printf("%d \t %d \t %d \t\t %lld \t\t %d \t\t %lld \t\t %d \t\t %lld \t\t %d \t\t %d \t\t %d\n",
                    entry->in_use, entry->fu, entry->dest_reg, entry->dest_reg_tag, entry->src1_ready, entry->src1_tag, entry->src2_ready, entry->src2_tag, entry->fired, entry->completed, entry->mark_for_delete);
        }

        printf("\n");
    }
    void deleteInstructions();
    void markCompletedInstructionsForDeletion();
    void readResultBuses(vector<result_bus>* result_buses);
    vector<reservation_station*>* getUnusedSlots(vector<proc_inst_t>* dispatch_queue);
};

class Scoreboard {
    vector<function_unit>* function_units;

    public:
    Scoreboard(uint64_t k0, uint64_t k1, uint64_t k2){
        function_units = new vector<function_unit>;
        for(uint64_t i = 0; i < k0; ++i){
            function_units->push_back({0,0,0,0,0});
        }
        for(uint64_t i = 0; i < k1; ++i){
            function_units->push_back({1,0,0,0,0});
        }
        for(uint64_t i = 0; i < k2; ++i){
            function_units->push_back({2,0,0,0,0});
        }
    }

    void broadcastCompletedInstructions();
};

bool read_instruction(proc_inst_t* p_inst);

void setup_proc(uint64_t r, uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f);
void run_proc(proc_stats_t* p_stats);
void complete_proc(proc_stats_t* p_stats);

void state_update();
void execute();
void schedule();
void dispatch();
void fetch();
void initReservationStation(reservation_station* entry);
void readInstructions();


#endif /* PROCSIM_HPP */
