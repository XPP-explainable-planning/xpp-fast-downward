#include "modified_disjunctive_goal_task.h"
#include "../task_proxy.h"

using namespace std;

namespace extra_tasks {
ModifiedDisjunctiveGoalTask::ModifiedDisjunctiveGoalTask(
    const shared_ptr<AbstractTask> &parent, const vector<FactPair> and_goals, const vector<FactPair> or_goals)
    : DelegatingTask(parent),
      and_goals(move(and_goals)),
      or_goals(move(or_goals)),
      new_goal(parent->get_num_variables(), 1) {

    TaskProxy taskproxy = TaskProxy(*parent);

    
    //cout << "------ Modified Task-----" << endl;
    cout << "AND goals: " << endl;
    for(FactPair fp : and_goals){
        cout << "\t var: " << fp.var << " = " << fp.value << " (" << taskproxy.get_variables()[fp.var].get_fact(fp.value).get_name() << ")" << endl;
    }
    /*
    cout << "OR goals: " << endl;
    for(FactPair fp : or_goals){
        cout << "\t var: " << fp.var << " = " << fp.value << " (" << taskproxy.get_variables()[fp.var].get_fact(fp.value).get_name() << ")" << endl;
    }
    cout << "New goal: var: " << new_goal.var << " = " << new_goal.value << endl;
    cout <<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
    for(int i = get_num_operators() - (or_goals.size() > 1 ? or_goals.size() : 1) ; i < get_num_operators(); i++){
        cout << "op: " << i << endl;
        cout << "pre: " << endl;
        for(int j = 0; j < get_num_operator_preconditions(i, false); j++){
            FactPair fp = get_operator_precondition(i,j,false);
            cout << "\t var: " << fp.var << " = " << fp.value << endl; 
        }
        cout << "eff: " << endl;
        for(int j = 0; j < get_num_operator_effects(i, false); j++){
            FactPair fp = get_operator_effect(i,j,false);
            cout << "\t var: " << fp.var << " = " << fp.value << endl; 
        }
    }
    cout << "------ Modified Task-----" << endl;
    */
}

int ModifiedDisjunctiveGoalTask::get_num_goals() const {
    //cout << "-------------------------------------------------------> get num goals"  << endl;
    return 1;
}

FactPair ModifiedDisjunctiveGoalTask::get_goal_fact(int) const {
    //cout << "-------------------------------------------------------> get goal"  << endl;
    return new_goal;
}

int ModifiedDisjunctiveGoalTask::get_num_variables() const {
    //cout << "-------------------------------------------------------> get num variables"  << endl;
    return parent->get_num_variables() + 1;
}

std::string ModifiedDisjunctiveGoalTask::get_variable_name(int var) const {
    //cout << "-------------------------------------------------------> get variable name"  << endl;
    if(var < parent->get_num_variables()){
        return parent->get_variable_name(var);
    }

    return "artificial_goal";
}

int ModifiedDisjunctiveGoalTask::get_variable_domain_size(int var) const {
    //cout << "-------------------------------------------------------> get variable domain size"  << endl;
    if(var < parent->get_num_variables()){
        return parent->get_variable_domain_size(var);
    }

    return 2;
}

int ModifiedDisjunctiveGoalTask::get_variable_axiom_layer(int var) const {
    //cout << "-------------------------------------------------------> get axiom layer"  << endl;
    if(var < parent->get_num_variables()){
        return parent->get_variable_axiom_layer(var);
    }

    //TODO
    return 1;
}

int ModifiedDisjunctiveGoalTask::get_variable_default_axiom_value(int var) const {
    //cout << "-------------------------------------------------------> get default axiom value"  << endl;
    if(var < parent->get_num_variables()){
        return parent->get_variable_default_axiom_value(var);
    }

    //TODO
    return 1;
}

int ModifiedDisjunctiveGoalTask::get_operator_cost(int index, bool is_axiom) const {
    //cout << "-------------------------------------------------------> get operator cost"  << endl;
    if(index < parent->get_num_operators()){
        return parent->get_operator_cost(index, is_axiom);
    }

    return 0;
}

std::string ModifiedDisjunctiveGoalTask::get_operator_name(int index, bool is_axiom) const {
    //cout << "-------------------------------------------------------> get operator name"  << endl;
    if(index < parent->get_num_operators()){
        return parent->get_operator_name(index, is_axiom);
    }

    return "artificial_action";
}

int ModifiedDisjunctiveGoalTask::get_num_operators() const {
    //cout << "-------------------------------------------------------> get num operators num or goals: " <<  or_goals.size() << endl;
    if(or_goals.size() > 1)
        return parent->get_num_operators() + or_goals.size();
    else
        return parent->get_num_operators() + 1;
}

int ModifiedDisjunctiveGoalTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    //cout << "-------------------------------------------------------> get num operators precon"  << endl;
    if(index < parent->get_num_operators()){
        return parent->get_num_operator_preconditions(index, is_axiom);
    }

    return and_goals.size() + (or_goals.size() > 0 ? 1: 0);
}

FactPair ModifiedDisjunctiveGoalTask::get_operator_precondition(int op_index, int fact_index, bool is_axiom) const {
    //cout << "-------------------------------------------------------> get op precondition"  << endl;
    if(op_index < parent->get_num_operators()){
        return parent->get_operator_precondition(op_index, fact_index, is_axiom);
    }

    if(((uint) fact_index) < and_goals.size()){
        return and_goals[fact_index];
    }
    else{
        return or_goals[op_index - parent->get_num_operators()];
    }
}

int ModifiedDisjunctiveGoalTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    //cout << "-------------------------------------------------------> get num op effect"  << endl;
    if(op_index < parent->get_num_operators()){
        return parent->get_num_operator_effects(op_index, is_axiom);
    }

    return 1;
}

int ModifiedDisjunctiveGoalTask::get_num_operator_effect_conditions(int op_index, int eff_index, bool is_axiom) const {
    //cout << "-------------------------------------------------------> get num effect conditions"  << endl;
    if(op_index < parent->get_num_operators()){
        return parent->get_num_operator_effect_conditions(op_index, eff_index, is_axiom);
    }

    return 0;
}

FactPair ModifiedDisjunctiveGoalTask::get_operator_effect_condition(int op_index, int eff_index, int cond_index, bool is_axiom) const {
    //cout << "-------------------------------------------------------> get effect condition"  << endl;
    if(op_index < parent->get_num_operators()){
        return parent->get_operator_effect_condition(op_index, eff_index, cond_index, is_axiom);
    }

    //TODO
    return FactPair::no_fact;
}

FactPair ModifiedDisjunctiveGoalTask::get_operator_effect( int op_index, int eff_index, bool is_axiom) const {
    //cout << "-------------------------------------------------------> get op effect"  << endl;
    if(op_index < parent->get_num_operators()){
        return parent->get_operator_effect(op_index, eff_index, is_axiom);
    }
    //cout << "-------------------------------------------------------> get op effect goal new"  << endl;
    return new_goal;
}

std::vector<int> ModifiedDisjunctiveGoalTask::get_initial_state_values() const {
    //cout << "-------------------------------------------------------> get initial state"  << endl;
    vector<int> original = parent->get_initial_state_values();
    original.push_back(0);

    /*
    cout << "Initial state:" << endl;
    for(int n : original){
        cout << n << " " ;
    }
    cout << endl;
    */
    return original;
}

}
