#ifndef PROCSIM_HPP
#define PROCSIM_HPP

#include <cstdint>
#include <stdint.h>
#include <cstdio>
#include <vector>
#include <memory>
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

typedef struct
{
    bool ready;
    int64_t tag;
} reg;

typedef struct {
    bool in_use;
    int fu;
    int dest_reg;
    int64_t dest_reg_tag;
    bool src1_ready;
    int64_t src1_tag;
    bool src2_ready;
    int64_t src2_tag;
    bool fired;
    bool completed;
    bool mark_for_delete;
} reservation_station;

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
    void deleteInstructions();
    void markCompletedInstructionsForDeletion();
    vector<reservation_station*>* getUnusedSlots(vector<proc_inst_t>* dispatch_queue);
};

typedef struct {
    bool busy;
    int64_t tag;
    int64_t register_number;
    bool completed;
} function_unit;

typedef struct {
    bool busy;
    uint64_t tag;
    int register_number;
} result_bus;

class Scoreboard {
    vector<function_unit>* k0_function_units;
    vector<function_unit>* k1_function_units;
    vector<function_unit>* k2_function_units;

    vector<result_bus>* result_buses;

    public:
    Scoreboard(uint64_t k0, uint64_t k1, uint64_t k2, uint64_t r){
        k0_function_units = new vector<function_unit> (k0);
        k1_function_units = new vector<function_unit> (k1);
        k2_function_units = new vector<function_unit> (k2);

        result_buses = new vector<result_bus> (r);
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


#endif /* PROCSIM_HPP */