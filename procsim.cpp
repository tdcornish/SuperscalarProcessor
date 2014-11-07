#include "procsim.hpp"


reg* register_file;
result_bus* result_buses;

vector<proc_inst_t>* dispatch_queue;
SchedulingQueue* schedule_queue;
Scoreboard* scoreboard;

int number_of_instructions_to_fetch;
int number_of_results_buses;

uint64_t inst_count;

/**
 * Subroutine for initializing the processor. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @r Number of Result Buses
 * @k0 Number of k0 FUs
 * @k1 Number of k1 FUs
 * @k2 Number of k2 FUs
 * @f Number of instructions to fetch
 */
void setup_proc(uint64_t r, uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f)
{
    register_file = new reg[128];
    result_buses = new result_bus[r];
    dispatch_queue = new vector<proc_inst_t>;

    schedule_queue = new SchedulingQueue(k0, k1, k2);
    scoreboard = new Scoreboard(k0, k1, k2, r);

    number_of_instructions_to_fetch = f;
    number_of_results_buses = r;
    inst_count = 0;
}

/**
 * Subroutine that simulates the processor.
 *   The processor should fetch instructions as appropriate, until all instructions have executed
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void run_proc(proc_stats_t* p_stats)
{
    state_update();
    execute();
    schedule();
    dispatch();
    fetch();
}

/**
 * Subroutine for cleaning up any outstanding instructions and calculating overall statistics
 * such as average IPC, average fire rate etc.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_proc(proc_stats_t *p_stats)
{

}

void state_update(){
    schedule_queue->deleteInstructions();
    schedule_queue->markCompletedInstructionsForDeletion();
}

void execute(){
    scoreboard->broadcastCompletedInstructions();
}

void schedule(){

}

void dispatch(){
    vector<reservation_station*>* unused_slots = schedule_queue->getUnusedSlots(dispatch_queue);

    vector<reservation_station*>::iterator entry;
    for(entry=unused_slots->begin(); entry != unused_slots->end(); ++entry){
    }
}

void fetch(){
    proc_inst_t* current_instruction = new proc_inst_t();
    for(int i = 0; i < number_of_instructions_to_fetch; ++i){
        ++inst_count;
        read_instruction(current_instruction);

        dispatch_queue->push_back(*current_instruction);
    }
}


vector<reservation_station*>* SchedulingQueue::getUnusedSlots(vector<proc_inst_t>* dispatch_queue){
    vector<reservation_station*> *reserved_slots = new vector<reservation_station*>;

    vector<reservation_station>::iterator entry;
    for(entry=scheduling_queue->begin(); entry != scheduling_queue->end(); ++entry){
        if(!entry->in_use){
            reserved_slots->push_back(&(*entry));
        }
    }

    return reserved_slots;
}

void SchedulingQueue::deleteInstructions(){
    vector<reservation_station>::iterator entry;
    for(entry=scheduling_queue->begin(); entry != scheduling_queue->end(); ++entry){
        if(entry->mark_for_delete){
            entry->in_use = false;
        }
    }
}

void SchedulingQueue::markCompletedInstructionsForDeletion(){
    vector<reservation_station>::iterator entry;
    for(entry=scheduling_queue->begin(); entry != scheduling_queue->end(); ++entry){
        if(entry->completed){
            entry->mark_for_delete = true;
        }
    }
}

void Scoreboard::broadcastCompletedInstructions(){
}

