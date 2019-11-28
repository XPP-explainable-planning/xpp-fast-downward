#include "monitor_mugs_pruning.h"

#include "bitset"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace monitor_mugs_pruning {
MonitorMugsPruning::MonitorMugsPruning(const options::Options &opts)
        :   all_soft_goals(opts.get<bool>("all_softgoals")),
            prune(opts.get<bool>("prune")),
            max_heuristic(opts.get<Evaluator*>("h")){

}

void MonitorMugsPruning::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    TaskProxy task_proxy = TaskProxy(*task);

    // speparate hard and soft goals
    num_goal_facts = task_proxy.get_goals().size();
    for (int i = 0; i < num_goal_facts; i++) {
        FactProxy gp = task_proxy.get_goals()[i];
        int id = gp.get_variable().get_id();
        int value = gp.get_value();
        hard_goals =
                (hard_goals << 1) | (!(task_proxy.get_variables()[id].get_fact(value).get_name().find("soft") == 0));
        goal_fact_names.push_back(task_proxy.get_variables()[id].get_fact(value).get_name());
        //cout << "Pos " << i << ": "  << task_proxy.get_variables()[id].get_fact(value).get_name() << endl;
    }

    if (all_soft_goals) {
        hard_goals = 0U;
    }
    cout << "Hard goals: " << std::bitset<32>(hard_goals) << endl;
    cout << "pruning method: monitor_mugs_pruning prune: " << prune << endl;


    //init monitors
    for (uint i = 0; i < task_proxy.get_LTL_properties().size(); i++) {
        this->monitors.push_back(new Monitor(task, task_proxy.get_LTL_properties()[i]));
    }
}


bool MonitorMugsPruning::is_superset(uint super, uint sub) const{
    uint diff = super ^ sub;
    return (diff == 0) || (((diff & super) > 0) && ((diff & sub) == 0));
}

bool MonitorMugsPruning::superset_contained(uint goal_subset, const unordered_set<uint> &set) const{
    for(uint gs : set){
        if(is_superset(gs, goal_subset)){
            //cout <<  "Superset: "  << std::bitset<32>(gs) << endl;
            return true;
        }
    }
    //msgs.insert(goal_subset);
    return false;
}

bool MonitorMugsPruning::insert_new_subset(uint goal_subset, unordered_set<uint> &set) const{
    for(uint gs : set){
        if(is_superset(goal_subset, gs))
            return true;
    }

    std::unordered_set<uint>::iterator it=set.begin();
    while (it!=msgs.end()){
        uint gs = *it;
        if(is_superset(gs, goal_subset)){
            //cout <<  "Subset: "  << std::bitset<32>(gs) << endl;
            it = set.erase(it);
        }
        else{
            it++;
        }
    }
    set.insert(goal_subset);
    return false;
}

bool MonitorMugsPruning::insert_new_superset(uint goal_subset,  unordered_set<uint> &set, bool& inserted) const{
    inserted = false;

    //cout << "Add new superset" << endl;
    //check if superset is already contained in set
    for(uint gs : set){
        if(is_superset(gs, goal_subset))
            return true;
    }

    //cout <<  "New superset:  "  << std::bitset<32>(goal_subset) << endl;
    //remove all sets which are a subset of the new set
    std::unordered_set<uint>::iterator it=set.begin();
    while (it!=msgs.end()){
        uint gs = *it;
        if(is_superset(goal_subset, gs)){
            //cout <<  "\tdelete "  << std::bitset<32>(gs) << endl;
            it = set.erase(it);
        }
        else{
            it++;
        }
    }

    inserted = true;

    // add new superset
    set.insert(goal_subset);
    return false;
}

std::unordered_set<uint> MonitorMugsPruning::unsolvable_subgoals(int number) const{
    unordered_set<uint> ugs;
    unordered_set<uint> candidates;
    //all subset with one goal fact
    for(int i = 0; i < number; i++){
        candidates.insert(1U << i);
    }

    while(candidates.size() > 0){
        unordered_set<uint> new_ugs;
        //add goal facts until set is not solvable anymore
        auto it = candidates.begin();
        while( it != candidates.end()){
            uint gs = *it;
            it = candidates.erase(it);
            if(! superset_contained(gs, msgs)){
                ugs.insert(gs);               
            }
            else{
                //create new candidates sets with one additional goal fact
                for(int i = 0; i < number; i++){
                    if(((gs & (1U << i)) == 0)){
                        candidates.insert(gs | (1U << i));
                    }
                }
            }
        }
    }

    return ugs;
}



std::unordered_set<uint> MonitorMugsPruning::minimal_unsolvable_subgoals(std::unordered_set<uint> &ugs) const{
     unordered_set<uint> mugs;
     for(uint gs : ugs){
         insert_new_subset(gs, mugs);
     }
     return mugs;
}


void MonitorMugsPruning::print_set(std::unordered_set<uint> s) const{
    for(uint gs : s){
        cout << std::bitset<32>(gs) << endl;
    }
}

bool MonitorMugsPruning::check_reachable(const State &state) {
    //cout << "---------------------------------------------------------" << endl;
    //state.dump_fdr();
    uint reachable_gs = ((max_heuristic::HSPMaxHeuristic*) max_heuristic)->compute_relaxed_reachable_goal_facts(state);
    //cout << "Reachable: " << std::bitset<32>(reachable_gs) << endl;

    //if a hard goal is not reachable prune the state
    if((hard_goals & reachable_gs) != hard_goals){
        //cout << "Hard goal not reachable" << endl;
        return true;
    }

    // if a superset of states was already reached -> prune state
    //bool prune_state = superset_contained(reachable_gs, msgs);

    return false;
}

void MonitorMugsPruning::add_goal_to_msgs(const State &state) {
    uint current_sat_goal_facts = 0;
    TaskProxy task_proxy = TaskProxy(*task);
    GoalsProxy g_proxy = task_proxy.get_goals();
    for(uint i = 0; i < g_proxy.size(); i++){
        current_sat_goal_facts = (current_sat_goal_facts << 1) | (state[g_proxy[i].get_variable().get_id()].get_value() == g_proxy[i].get_value());
    }
    //cout << "Current sat goal: " << std::bitset<32>(current_sat_goal_facts) << endl;

    //if all hard goals are satisfied add set
    msgs_changed = false;
    if((hard_goals & current_sat_goal_facts) == hard_goals){
        //cout << "insert" << endl;
        insert_new_superset(current_sat_goal_facts, msgs, msgs_changed); // only adds set of msgs does not contain any superset
    }

    /*
    cout << "++++++++++ MUGS PRUNING +++++++++++++++" << endl;
    for(uint gs : msgs){
        cout << std::bitset<32>(gs) << endl;
    }
     */
}

bool MonitorMugsPruning::prune_state(const State &state){

    //check if hard goals are still relaxed reachable
    //cout << "Prune relaxed reachable" << endl;
    bool prune_state_hard_goals = check_reachable(state);

    if(prune) {
        if (prune_state_hard_goals) {
            pruned_states++;
        }
        return prune_state_hard_goals;
    }
    else
        return false;
}


bool MonitorMugsPruning::prune_init_state(const GlobalState &state) {
    cout << "Prune init state" << endl;
    for(auto m : monitors){
        m->init(state);
    }
    return false;
}

bool MonitorMugsPruning::prune_state(StateID parent_id, const GlobalState &global_state, bool* new_automaton_state_reached){

    State state(*task, global_state.get_values());
    if(this->prune_state(state)){
        *new_automaton_state_reached = false;
        for (auto m : monitors) {
            pair<bool, bool> result = m->check_state(parent_id, global_state);
            *new_automaton_state_reached |= result.second;
        }
        return true;
    }

    //-> all goal facts are still reachable
    //current sat goals
    uint current_sat_goal_facts = 0;
    TaskProxy task_proxy = TaskProxy(*task);
    GoalsProxy g_proxy = task_proxy.get_goals();
    for(uint i = 0; i < g_proxy.size(); i++){
        current_sat_goal_facts = (current_sat_goal_facts << 1) | (state[g_proxy[i].get_variable().get_id()].get_value() == g_proxy[i].get_value());
    }

    //if all hard goals are satisfied check which properties can still be satisfied
    *new_automaton_state_reached = false;
    if((hard_goals & current_sat_goal_facts) == hard_goals) {
        //cout << "+++++++++++++++++++++++++ HARD GOAL SAT ++++++++++++++++++" << endl;
        //uint satisfiable_props = 0; //TODO is this still somehow possible
        uint satisfied_props = 0;
        for (auto m : monitors) {
            //cout << " ***** Monitor: " << m->get_property().name << "****************" << endl;
            //satisfiable_props = satisfiable_props << 1;
            satisfied_props = satisfied_props << 1;
            pair<bool,bool>  result = m->check_state(parent_id, global_state);
            bool satisfied = result.first;
            *new_automaton_state_reached |= result.second;
            if (satisfied) {
                satisfied_props |= 1;
            }
        }

//        cout << "satisfiable props: " << std::bitset<32>(satisfiable_props) << endl;
//        cout << "satisfied props:   " << std::bitset<32>(satisfied_props) << endl;
//        cout << "-------" << endl;
        //bool prune_state = superset_contained(satisfiable_props, msgs);
        bool prune_state = false; //TODO implement

        msgs_changed = false;
        insert_new_superset(satisfied_props, msgs, msgs_changed);
        if(prune_state){
            pruned_states_props++;
        }
        //return prune && prune_state;
    }
    else{
        for (auto m : monitors) {
            pair<bool, bool> result = m->check_state(parent_id, global_state);
            *new_automaton_state_reached |= result.second;
        }
        return false;
    }

    return false;
}


void MonitorMugsPruning::prune_operators(const State &state, std::vector<OperatorID> &){
    this->prune_state(state);
}

void MonitorMugsPruning::print_statistics() const{
    cout << "++++++++++ MSGS PRUNING +++++++++++++++" << endl;
    print_set(msgs);
    print_mugs();

    cout << "Pruned states: " << pruned_states << endl;
    cout << "Pruned states props: " << pruned_states_props << endl;
}

void MonitorMugsPruning::print_mugs() const{

    unordered_set<uint> ugs = unsolvable_subgoals(task->get_num_LTL_properties());
    unordered_set<uint> mugs = minimal_unsolvable_subgoals(ugs);
    cout << "++++++++++ MUGS PRUNING +++++++++++++++" << endl;
//    print_set(mugs);
//    cout << "++++++++++++++++++++++++++++++++++++++++++++++++"  << endl;
    //cout << "num goal fact names: " << goal_fact_names.size() << endl;
    uint num_properties = task->get_num_LTL_properties();
    for(uint gs : mugs){
        for(uint i = 0; i < num_properties; i++){
            if(((1U << (num_properties - 1 - i)) & gs) >= 1){
                cout << task->get_LTL_property(i).name << "|";
            }
        }
        cout << endl;
    }
    cout << "++++++++++++++++++++++++++++++++++++++++++++++++"  << endl;
    cout << "Number of minimal unsolvable goal subsets: " << mugs.size() << endl;
}

static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "TODO",
        "TODO");

    parser.add_option<Evaluator*>(
        "h",
        "TODO",
        "hmax(early_term=false)");
    parser.add_option<bool>(
        "all_softgoals",
        "TODO",
        "false");
    parser.add_option<bool>(
            "prune",
            "TODO",
            "true");

    Options opts = parser.parse();

    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<MonitorMugsPruning>(opts);
}


static PluginShared<PruningMethod> _plugin("monitor_mugs_pruning", _parse);
}
