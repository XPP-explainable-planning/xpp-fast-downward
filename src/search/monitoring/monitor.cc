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
#include <bddx.h>

using namespace std;

Monitor::Monitor(const std::shared_ptr<AbstractTask> &task_, string LTL_formula){
    assert(!task);
    task = task_;
    TaskProxy taskproxy = TaskProxy(*task);
    cout << "################# MONITOR ##############" << endl;

    //Register variables
//    for(uint i = 0; i < taskproxy.get_variables().size(); i++){
//        for(int j = 0; j < taskproxy.get_variables()[i].get_domain_size(); j++){
//            //cout << taskproxy.get_variables()[i].get_fact(j).get_name() << endl;
//            string var_name = taskproxy.get_variables()[i].get_fact(j).get_name();
//            if(var_name.rfind("Atom ", 0) == 0){
//                var_name = var_name.replace(0,5, "");
//            }
//            var_name = regex_replace(var_name, std::regex("\\("), "_");
//            var_name = regex_replace(var_name, std::regex("\\)"), "_");
//            var_name = regex_replace(var_name, std::regex(", "), "_");
//            //int varid = dict->register_proposition(spot::parse_infix_psl(var_name).f, this);
//            cout << i << " " << j << ": " << " " << var_name << endl;
//        }
//    }

    //spot::parsed_formula pf = spot::parse_infix_psl("!F(red & X(yellow))");
    cout << "LTL: " << LTL_formula << endl;
    spot::parsed_formula pf = spot::parse_infix_psl(LTL_formula);
    if (pf.format_errors(std::cerr))
        throw invalid_argument("LTL formula is not valid");
    spot::translator trans;
    trans.set_type(spot::postprocessor::Monitor);
    trans.set_pref(spot::postprocessor::Deterministic);
    this->automaton = trans.run(pf.f);

    cout << "----------------------------------------------------------" << endl;
    print_hoa(std::cout, this->automaton) << '\n';
    cout << "----------------------------------------------------------" << endl;

    cout << "Dict Mapping: " << endl;
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
                    cout << ap << " -> " << i << " " << j << endl;
                }
            }
        }

    }

}

void Monitor::init(const GlobalState &global_state) {

    cout << "----- Init Monitor ---------" << endl;
    int initial_node_id = this->automaton->get_init_state_number();
    this->current_state[global_state.get_id()] = initial_node_id;
    cout << global_state.get_id() << " -> " << initial_node_id << endl;
    cout << "----- Init Monitor ---------" << endl;
}

int Monitor::check_state(StateID parent_id, const GlobalState &global_state) {
    State state(*task, global_state.get_values());

    //cout << "----- Check State ---------" << endl;
    //const spot::bdd_dict_ptr& dict = this->automaton->get_dict();
    int current_automaton_state = this->current_state[parent_id];
    for (auto& t: this->automaton->out(current_automaton_state)) {
        //spot::bdd_print_formula(cout, dict, t.cond);
        //cout << endl;
        if(this->sat_bdd(t.cond, global_state)){
            //cout << "SAT" << endl;
            this-> current_state[global_state.get_id()] = t.dst;
            this->truth_values[global_state.get_id()] = TruthValue::UNKNOWN;
            //TODO add check if property can still be unsat
            return 1;
        }
    }

    //cout << "UNSAT" << endl;
    this->truth_values[global_state.get_id()] = TruthValue::FALSE;
    return 0;
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
