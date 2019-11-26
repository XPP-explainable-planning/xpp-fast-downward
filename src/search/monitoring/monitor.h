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

#include "../task_proxy.h"

using namespace std;

enum TruthValue {TRUE, FALSE, UNKNOWN};

class Monitor {

    std::shared_ptr<AbstractTask> task;
    spot::twa_graph_ptr automaton;
    unordered_map<StateID,TruthValue> truth_values;
    unordered_map<StateID,int> current_state;
    unordered_map<int, pair<int,int>>  bdd_varid_fact;


public:
    Monitor(const std::shared_ptr<AbstractTask> &task, string LTL_formula);

    void init(const GlobalState &global_state);
    int check_state(StateID parent_id, const GlobalState &global_state);


private:
    bool sat_bdd(bdd bdd_, const GlobalState &global_state);
};


#endif //FAST_DOWNWARD_MONITOR_H
