#include "procsim.hpp"


vector<reg>* register_file;
vector<result_bus>* result_buses;
vector<proc_inst_t>* instruction_queue;
vector<proc_inst_t>* dispatch_queue;
vector<proc_inst_t>* completed_instruction_queue;

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
    register_file = new vector<reg> (128);
    result_buses = new vector<result_bus> (r);

    instruction_queue = new vector<proc_inst_t>;
    dispatch_queue = new vector<proc_inst_t>;
    completed_instruction_queue = new vector<proc_inst_t>;

    schedule_queue = new SchedulingQueue(k0, k1, k2);
    scoreboard = new Scoreboard(k0, k1, k2);

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
    readInstructions();

    for(int i = 0; i < 4; i++){
        state_update();
        execute();
        schedule();
        dispatch();
        schedule_queue->printQueue();
        fetch();
    }
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
    std::cout << "state update" << std::endl;
    schedule_queue->deleteInstructions();
    schedule_queue->markCompletedInstructionsForDeletion();
}

void execute(){
    std::cout << "execute" << std::endl;
    scoreboard->broadcastCompletedInstructions();
}

void schedule(){
    std::cout << "schedule"<< std::endl;
    schedule_queue->readResultBuses(result_buses);
}

void dispatch(){
    std::cout << "dispatch" << std::endl;
    vector<reservation_station*>* unused_slots = schedule_queue->getUnusedSlots(dispatch_queue);

    vector<reservation_station*>::iterator entry;
    for(auto& entry : *unused_slots){
        if (!dispatch_queue->empty()) {
            initReservationStation(entry);
        }
        else{
            break;
        }
    }
}

void fetch(){
    std::cout << "fetch" << std::endl;
    for(int i = 0; i < number_of_instructions_to_fetch; ++i){
        dispatch_queue->push_back(instruction_queue->front());
        instruction_queue->erase(instruction_queue->begin());
    }
}


void initReservationStation(reservation_station* entry){
    reservation_station* current_slot = entry;
    proc_inst_t inst = dispatch_queue->front();
    dispatch_queue->erase(dispatch_queue->begin());

    current_slot->original_instruction = inst;
    current_slot->in_use = true;
    current_slot->fu = inst.op_code;
    current_slot->dest_reg = inst.dest_reg;
    current_slot->fired = false;
    current_slot->completed = false;
    current_slot->mark_for_delete = false;

    //look up source registers in the register file
    int32_t src1 = inst.src_reg[0];
    int32_t src2 = inst.src_reg[1];

    reg* reg;
    if(src1 == -1){
        current_slot->src1_ready = true;
    }
    else{
        reg = &register_file->at(src1);
        if(reg->ready){
            current_slot->src1_ready = true;
        }
        else{
            current_slot->src1_ready = false;
            current_slot->src1_tag = reg->tag;
        }
    }

    if(src2 == -1){
        current_slot->src2_ready = true;
    }
    else{
        reg = &register_file->at(src2);
        if(reg->ready){
            current_slot->src2_ready = true;
        }
        else{
            current_slot->src2_ready = false;
            current_slot->src2_tag = reg->tag;
        }
    }

    if(inst.dest_reg != -1){
        reg = &register_file->at(inst.dest_reg);
        reg->ready = false;
        reg->tag = inst.tag;
    }

    current_slot->dest_reg_tag = inst.tag;
}

void readInstructions(){
    proc_inst_t* current_instruction = new proc_inst_t();
    while(read_instruction(current_instruction)){
        ++inst_count;
        current_instruction->tag = inst_count;
        instruction_queue->push_back(*current_instruction);
    }
}

//Class functions
vector<reservation_station*>* SchedulingQueue::getUnusedSlots(vector<proc_inst_t>* dispatch_queue){
    vector<reservation_station*> *reserved_slots = new vector<reservation_station*>;

    vector<reservation_station>::iterator entry;
    for(auto& entry : *scheduling_queue){
        if(!entry.in_use){
            reserved_slots->push_back(&entry);
        }
    }

    return reserved_slots;
}

void SchedulingQueue::deleteInstructions(){
    vector<reservation_station>::iterator entry;
    for(auto& entry : *scheduling_queue){
        if(entry.mark_for_delete){
            entry.in_use = false;
        }
    }
}

void SchedulingQueue::markCompletedInstructionsForDeletion(){
    for(auto& entry : *scheduling_queue){
        if(entry.completed){
            entry.mark_for_delete = true;
        }
    }
}

void SchedulingQueue::readResultBuses(vector<result_bus>* result_buses){
    for(auto& entry : *scheduling_queue){
        if(!entry.src1_ready){
            for(auto bus : *result_buses){
                if(bus.busy && bus.tag == entry.src1_tag){
                    entry.src1_ready = true;
                }
            }
        }

        if(!entry.src2_ready){
            for(auto bus : *result_buses){
                if(bus.busy && bus.tag == entry.src2_tag){
                    entry.src2_ready = true;
                }
            }
        }

        if(entry.fired){
            for(auto bus : *result_buses){
                if(bus.busy && bus.tag == entry.dest_reg_tag){
                    entry.completed = true;
                }

            }
        }
    }

    for(auto& bus : *result_buses){
        bus.busy = false;
    }
}

void Scoreboard::broadcastCompletedInstructions(){
    for(auto& rs : *result_buses){
        if(!rs.busy){
            for(auto& fu : *function_units){

            }
        }
    }
}


