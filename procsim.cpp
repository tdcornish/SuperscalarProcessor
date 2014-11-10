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

int cycle_count;
uint64_t inst_count;

double dispatch_size_per_cycle;
double instructions_fired_per_cycle;
double instructions_retired_per_cycle;

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

    dispatch_size_per_cycle = 0;
    instructions_fired_per_cycle = 0;
    instructions_retired_per_cycle = 0;
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
    uint64_t goal = instruction_queue->size();

    uint64_t max_disp_size = 0;
    while(completed_instruction_queue->size() != goal){
        ++cycle_count;

        //printf("Cycle %d\n", cycle_count);
        if(dispatch_queue->size() > max_disp_size){
            max_disp_size = dispatch_queue->size();
        }
        dispatch_size_per_cycle += dispatch_queue->size();

        state_update();
        execute();
        schedule();
        dispatch();
        fetch();

        //schedule_queue->printQueue();
        //scoreboard->printFunctionUnits();
        //printResultBus();
        //printf("\n\n");
    }

    p_stats->retired_instruction = completed_instruction_queue->size();
    p_stats->avg_disp_size = dispatch_size_per_cycle/cycle_count;
    p_stats->max_disp_size = max_disp_size;
    p_stats->avg_inst_fired = instructions_fired_per_cycle/cycle_count;
    p_stats->avg_inst_retired = instructions_retired_per_cycle/cycle_count;
    p_stats->cycle_count = cycle_count;
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
    sort(completed_instruction_queue->begin(), completed_instruction_queue->end(), sort_by_inst_number);
    printf("INST\tFETCH\tDISP\tSCHED\tEXEC\tSTATE\n");
    for(auto inst : *completed_instruction_queue){
        printf("%d\t%d\t%d\t%d\t%d\t%d\n", inst.inst_number, inst.fetch, inst.disp, inst.sched, inst.exec, inst.state);
    }
    printf("\n");
    delete(scoreboard);
    delete(schedule_queue);
    delete(register_file);
    delete(result_buses);
    delete(instruction_queue);
    delete(dispatch_queue);
    delete(completed_instruction_queue);
}

/**
 * Funtion to use when calling std::sort() on completed_instruction_queue to sort by instruction number.
 */
bool sort_by_inst_number(proc_inst_t i, proc_inst_t j){
    return i.inst_number < j.inst_number;
}

/**
 * State update function of the processor:
 *      Deletes any instructions marked for deletion from the queue, freeing them up for dispatch.
 *      Marks and completed instructions for deletion the next cycle.
 */
void state_update(){
    schedule_queue->deleteInstructions();
    schedule_queue->markCompletedInstructionsForDeletion();
}

/**
 * Execute function of the processor:
 *      Completed instructions put on available result buses and FU's freed up
 *      Set any uncompleted instructions in FU as completed.
 *      Completed instructions put on available result buses and FU's freed up
 *      Register file is updated by result buses
 *      Fire any instructions that can be fired
 */
void execute(){
    scoreboard->broadcastCompletedInstructions();
    scoreboard->completeBusyUnits(cycle_count);
    scoreboard->broadcastCompletedInstructions();
    scoreboard->markStalledUnits();
    scoreboard->updateRegisterFile(register_file);
    schedule_queue->fireInstructions(scoreboard);
}

/**
 * Schedule function of the processor:
 *      Updates the schedule queue with whatever is on the result buses.
 */
void schedule(){
    schedule_queue->readResultBuses(result_buses);
}

/**
 * Dispatch function of the processor:
 *      Puts instructions from the dispatch queue into any available slots in the scheduling queue.
 */
void dispatch(){
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

    schedule_queue->sort();
}

/**
 * Fetch function of the processor:
 *      Fetches F instructions from the instruction queue and puts them in the dispatch queue.
 */
void fetch(){
    if(!instruction_queue->empty()){
        for(int i = 0; i < number_of_instructions_to_fetch; ++i){
            proc_inst_t fetched_inst = instruction_queue->front();
            fetched_inst.fetch = cycle_count;
            fetched_inst.disp = cycle_count + 1;
            dispatch_queue->push_back(fetched_inst);
            instruction_queue->erase(instruction_queue->begin());
        }
    }
}

/**
 * Initializes a reservation station in the scheduling queue by reading and updating the register file.
 */
void initReservationStation(reservation_station* entry){
    reservation_station* current_slot = entry;
    proc_inst_t inst = dispatch_queue->front();
    dispatch_queue->erase(dispatch_queue->begin());

    inst.sched = cycle_count + 1;
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

/**
 * Reads all instructions from the trace file into instruction_queue.
 */
void readInstructions(){
    proc_inst_t* current_instruction = new proc_inst_t();
    while(read_instruction(current_instruction)){
        ++inst_count;
        current_instruction->tag = inst_count;
        current_instruction->inst_number = inst_count;
        if(current_instruction->op_code == -1){
            current_instruction->op_code = 1;
        }
        instruction_queue->push_back(*current_instruction);
    }
}

/**
 * Helper function to print out result buses.
 */
void printResultBus(){
    for(auto rs : *result_buses){
        printf("Busy: %d\tTag: %lld\t Reg: %d\n", rs.busy, rs.tag, rs.register_number);
    }
}

//Class functions

/**
 * Gets all the currently available slots in the scheduling queue.
 */
vector<reservation_station*>* SchedulingQueue::getUnusedSlots(vector<proc_inst_t>* dispatch_queue){
    vector<reservation_station*> *reserved_slots = new vector<reservation_station*>;

    for(auto& entry : *scheduling_queue){
        if(!entry.in_use){
            reserved_slots->push_back(&entry);
        }
    }

    return reserved_slots;
}

/**
 * Deletes any instructions marked for deletion from the scheduling queue.
 */
void SchedulingQueue::deleteInstructions(){
    for(auto& entry : *scheduling_queue){
        if(entry.mark_for_delete){
            entry.in_use = false;
            entry.fired = 0;
            entry.completed = 0;
            entry.mark_for_delete = 0;
        }
    }
}

/**
 * Marks any instructions that have completed for deletion in the next cycle.
 */
void SchedulingQueue::markCompletedInstructionsForDeletion(){
    for(auto& entry : *scheduling_queue){
        if(entry.completed && entry.in_use && !entry.mark_for_delete){
            entry.mark_for_delete = true;
            entry.original_instruction.state = cycle_count;
            completed_instruction_queue->push_back(entry.original_instruction);
            ++instructions_retired_per_cycle;
        }
    }
}

/**
 * Reads the result buses and updates the scheduling queue accordingly.
 */
void SchedulingQueue::readResultBuses(vector<result_bus>* result_buses){
    for(auto& entry : *scheduling_queue){
        if(!entry.src1_ready){
            for(auto bus : *result_buses){
                if(bus.busy && bus.tag == entry.src1_tag){
                    entry.src1_ready = true;
                    entry.src1_tag = 0;
                }
            }
        }

        if(!entry.src2_ready){
            for(auto bus : *result_buses){
                if(bus.busy && bus.tag == entry.src2_tag){
                    entry.src2_ready = true;
                    entry.src2_tag = 0;
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

/**
 * Fires any instructions that are ready to fire and have an available function unit.
 */
void SchedulingQueue::fireInstructions(Scoreboard* scoreboard){
    for(auto& entry : *scheduling_queue){
        if(entry.in_use && entry.src1_ready && entry.src2_ready && !entry.fired){
            function_unit* fu_to_use;
            bool found_available_fu = scoreboard->reserveAvailableFunctionUnit(entry.fu, fu_to_use);

            if(found_available_fu){
                ++instructions_fired_per_cycle;
                //printf("firing instruction %lld\n", entry.dest_reg_tag);
                fu_to_use->busy = true;
                fu_to_use->tag = entry.dest_reg_tag;
                fu_to_use->register_number = entry.dest_reg;
                fu_to_use->completed = false;
                fu_to_use->original_instruction = &entry.original_instruction;
                fu_to_use->cycles_stalled = 0;

                entry.fired = true;
                entry.original_instruction.exec = cycle_count + 1;
            }
        }
    }

    scoreboard->updateFunctionUnitQueues();
}

/**
 * Put any completed function units results on any available result buses, giving priority to instructions that have stalled the longest and then tag order.
 */
void Scoreboard::broadcastCompletedInstructions(){
    for(auto& rb : *result_buses){
        if(!rb.busy && !completed_function_units->empty()){
            function_unit* lowestCompletedTag = &completed_function_units->front();
            for(auto& fu : *completed_function_units){
                if(fu.cycles_stalled > lowestCompletedTag->cycles_stalled){
                    lowestCompletedTag = &fu;
                }
                else if(fu.cycles_stalled == lowestCompletedTag->cycles_stalled){
                    if(fu.tag < lowestCompletedTag->tag){
                        lowestCompletedTag = &fu;
                    }
                }
            }

            rb.busy = true;
            rb.tag = lowestCompletedTag->tag;
            rb.register_number = lowestCompletedTag->register_number;
            lowestCompletedTag->busy = false;

            updateFunctionUnitQueues();
        }
    }

    scoreboard->updateFunctionUnitQueues();
}

/**
 * Return if there is an available function units of type k and sets there is sets fu to point to it.
 */
bool Scoreboard::reserveAvailableFunctionUnit(int k, function_unit*& fu){
    bool found = false;
    for(auto& function_unit : *available_function_units){
        if(!function_unit.busy && function_unit.type == k){
            fu = &function_unit;
            found = true;
        }
    }

    return found;
}

/**
 * Updates the register file with what is on the result buses.
 */
void Scoreboard::updateRegisterFile(vector<reg>* register_file){
    for(auto rb : *result_buses){
        int register_number = rb.register_number;
        if(register_number != -1){
            reg* reg = &register_file->at(register_number);

            if(reg->tag == rb.tag){
                reg->ready = true;
                reg->tag = 0;
            }
        }
    }
}
