//
// Created by eifler on 22.11.19.
//

#include <regex>

#include "monitor.h"
#include <spot/tl/parse.hh>
#include <spot/twaalgos/translate.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twa/bddprint.hh>
#include <spot/twaalgos/ltl2tgba_fm.hh>
#include <spot/twaalgos/sccfilter.hh>
#include <spot/twaalgos/stripacc.hh>
#include <spot/twaalgos/minimize.hh>
#include <spot/twa/bdddict.hh>
#include <spot/twaalgos/remprop.hh>
#include <spot/tl/ltlf.hh>
#include <utility>
#include <bddx.h>

using namespace std;

Monitor::Monitor(const std::shared_ptr<AbstractTask> &task_, Property LTL_property):
    property(std::move(LTL_property)){
    assert(!task);
    task = task_;
    TaskProxy taskproxy = TaskProxy(*task);
    cout << "################# MONITOR ##############" << endl;

    //spot::parsed_formula pf = spot::parse_infix_psl("!F(red & X(yellow))");
    cout << "Name: " << property.name << endl;
    cout << "formula: " << property.formula << endl;
    spot::parsed_formula pf = spot::parse_infix_psl(property.formula);
    if (pf.format_errors(std::cerr))
        throw invalid_argument("LTL formula is not valid");

    spot::translator trans;
    trans.set_type(spot::postprocessor::BA);
    trans.set_pref(spot::postprocessor::Small);
    spot::twa_graph_ptr aut = trans.run(spot::from_ltlf(pf.f));

    // remove ap "alive" and simplify the automaton
    spot::remove_ap rem;
    rem.add_ap("alive");
    aut = rem.strip(aut);

    spot::postprocessor post;
    post.set_type(spot::postprocessor::BA);
    post.set_pref(spot::postprocessor::Deterministic); // or ::Deterministic
    aut = post.run(aut);

    this->automaton = aut;

//    cout << "----------------------------------------------------------" << endl;
//    print_hoa(std::cout, this->automaton) << '\n';
//    cout << "----------------------------------------------------------" << endl;


    //Find which variable id in the BDDs correponds to which fact
    //cout << "Dict Mapping: " << endl;
    const spot::bdd_dict_ptr& dict = this->automaton->get_dict();
    //cout << "Atomic propositions:";
    for (spot::formula ap: this->automaton->ap()) {
        //cout << ' ' << ap << " (=" << dict->varnum(ap) << ')' << endl;
        for (uint i = 0; i < taskproxy.get_variables().size(); i++) {
            for (int j = 0; j < taskproxy.get_variables()[i].get_domain_size(); j++) {
                //cout << taskproxy.get_variables()[i].get_fact(j).get_name() << endl;
                string var_name = taskproxy.get_variables()[i].get_fact(j).get_name();
                if (var_name.rfind("Atom ", 0) == 0) {
                    var_name = var_name.replace(0, 5, "");
                }
                var_name = regex_replace(var_name, std::regex("\\("), "_");
                var_name = regex_replace(var_name, std::regex("\\)"), "_");
                var_name = regex_replace(var_name, std::regex(", "), "_");
                if (var_name.compare(ap.ap_name()) == 0) {
                    bdd_varid_fact[dict->varnum(ap)] = make_pair(i, j);
                    //cout << ap << " -> " << i << " " << j << endl;
                }
            }
        }

    }

}

void Monitor::init_reached_automaton_states(StateID id) {
    vector<bool> reached_automaton_states;
    for(uint i = 0; i < automaton->num_states(); i++){
        reached_automaton_states.push_back(false);
    }
    this->reachable_automaton_state[id] = reached_automaton_states;
}

void Monitor::init(const GlobalState &global_state) {

    //initialise the monitor with the goal state
    int initial_node_id = this->automaton->get_init_state_number();
    this->init_reached_automaton_states(global_state.get_id());
    reachable_automaton_state[global_state.get_id()][initial_node_id] = true;

    this->truth_values[global_state.get_id()] = TruthValue::UNKNOWN;

}

bool Monitor::is_accepting_loop(uint s) {
    for (auto &t: this->automaton->out(s)) {
        //cout << "Acepting: "  << (t.acc) << endl;
        if(t.cond == bdd_true() && t.dst == s && (t.acc.sets().begin() != t.acc.sets().end())){
            return true;
        }
    }
    return false;
}

bool Monitor::is_not_accepting_loop(uint s) {
    for (auto &t: this->automaton->out(s)) {
        //cout << "Acepting: "  << (t.acc) << endl;
        if(t.cond == bdd_true() && t.dst == s && (t.acc.sets().begin() == t.acc.sets().end())){
            return true;
        }
    }
    return false;
}

bool Monitor::is_accepting(uint s) {
    for (auto &t: this->automaton->out(s)) {
//        cout << "is accepting: "  << (t.acc) << endl;
        if(t.acc.sets().begin() != t.acc.sets().end()){
            return true;
        }
    }
    return false;
}

pair<bool, bool> Monitor::check_state(StateID parent_id, const GlobalState &global_state) {

//    cout << "----- Check State ---------" << endl;
//    cout << "State: "  << global_state.get_id() << " parent: "  << parent_id << endl;
    //cout << "Parent truth value: " << this->truth_values[parent_id] << endl;

//    this->truth_values[global_state.get_id()] = this->truth_values[parent_id];
//
//    if(this->truth_values[parent_id] == TruthValue ::TRUE){
////        cout << "PARENT TRUE" << endl;
//        return make_pair(true, TruthValue::TRUE);
//    }
//    if(this->truth_values[parent_id] == TruthValue ::FALSE){
////        cout << "PARENT FALSE" << endl;
//        return make_pair(false, TruthValue ::FALSE);
//    }
    bool new_automaton_state_reached = false;

    //if state is visited for the first time init reach automaton states
    if(! reachable_automaton_state.count(global_state.get_id())){
        init_reached_automaton_states(global_state.get_id());
        new_automaton_state_reached = true;
    }

    bool satisfied = false;
    vector<bool> current_automaton_state_flags = this->reachable_automaton_state[parent_id];
//    for(uint i = 0; i < automaton->num_states(); i++){
//        if(this->reachable_automaton_state[global_state.get_id()][i]){
//            cout << i << " ";
//        }
//    }
//    cout << endl;
    for(uint current_automaton_state = 0; current_automaton_state < automaton->num_states(); current_automaton_state++) {
        if(current_automaton_state_flags[current_automaton_state]) {
//          cout << "Current state: " << current_automaton_state << endl;
            for (auto &t: this->automaton->out(current_automaton_state)) {

                //check transition applicable
                if (this->sat_bdd(t.cond, global_state)) {

                    new_automaton_state_reached = ! this->reachable_automaton_state[global_state.get_id()][t.dst];
                    this->reachable_automaton_state[global_state.get_id()][t.dst] = true;

                    //cout << "dest state: " << t.dst << endl;

                    if (this->is_accepting_loop(t.dst)) {
                        satisfied = true;
                        this->truth_values[global_state.get_id()] = TruthValue::TRUE;
                    }
                    else if(this->is_accepting(t.dst)){
                        satisfied = true;
                    }
                    //else if (this->is_not_accepting_loop(t.dst)) {
                    //    satisfied = satisfied | false;
                    //}

                    //satisfied = satisfied | false;
                }
            }
            //if there is no matching outgoing transition the trace does not satisfy the formula
            //satisfied = satisfied | false;
        }
    }
//    for(uint i = 0; i < automaton->num_states(); i++){
//        if(this->reachable_automaton_state[global_state.get_id()][i]){
//            cout << i << " ";
//        }
//    }
//    cout << endl;
    return make_pair(satisfied, new_automaton_state_reached);
}

bool Monitor::sat_bdd(bdd bdd_, const GlobalState &global_state) {
    State state(*task, global_state.get_values());
    while(true){
        if(bdd_ == bdd_true()){
            return true;
        }
        if(bdd_ == bdd_false()){
            return false;
        }
        int bdd_v = bdd_var(bdd_);
        pair<int, int> fact = this->bdd_varid_fact[bdd_v];
        if(state.get_values()[fact.first] == fact.second){
            bdd_ = bdd_high(bdd_);
        }
        else{
            bdd_ = bdd_low(bdd_);
        }
    }
}
