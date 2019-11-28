//
// Created by eifler on 22.11.19.
//

#ifndef FAST_DOWNWARD_MONITOR_H
#define FAST_DOWNWARD_MONITOR_H

#include <string>
#include <iostream>
#include <spot/tl/parse.hh>
#include <spot/twaalgos/translate.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/misc/bddlt.hh>

#include <unordered_map>

#include "../task_proxy.h"

using namespace std;

enum TruthValue {UNKNOWN, TRUE, FALSE};

class Monitor {

    std::shared_ptr<AbstractTask> task;
    Property property;
    spot::twa_graph_ptr automaton;
    unordered_map<StateID,TruthValue> truth_values;
    unordered_map<StateID,vector<bool>> reachable_automaton_state;
    unordered_map<int, pair<int,int>>  bdd_varid_fact;


public:
    Monitor(const std::shared_ptr<AbstractTask> &task, Property LTL_property);

    void init(const GlobalState &global_state);
    pair<bool, bool> check_state(StateID parent_id, const GlobalState &global_state);
    Property get_property(){
        return property;
    }


private:
    bool sat_bdd(bdd bdd_, const GlobalState &global_state);

    bool is_accepting_loop(uint s);
    bool is_not_accepting_loop(uint s);
    bool is_accepting(uint s);
    void init_reached_automaton_states(StateID id);
};


#endif //FAST_DOWNWARD_MONITOR_H
