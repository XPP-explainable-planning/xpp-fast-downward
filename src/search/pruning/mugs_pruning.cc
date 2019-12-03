#include "mugs_pruning.h"

#include "bitset"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace mugs_pruning {
void MugsPruning::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    TaskProxy task_proxy = TaskProxy(*task);
    num_goal_facts = task_proxy.get_goals().size();
        
    for(int i = 0; i < num_goal_facts; i++){
        FactProxy gp = task_proxy.get_goals()[i];
        int id = gp.get_variable().get_id();
        int value = gp.get_value();
        hard_goals = (hard_goals << 1) | (! (task_proxy.get_variables()[id].get_fact(value).get_name().find("soft") == 0));
        goal_fact_names.push_back(task_proxy.get_variables()[id].get_fact(value).get_name());
        //cout << "Pos " << i << ": "  << task_proxy.get_variables()[id].get_fact(value).get_name() << endl;
    }
    
    if(all_soft_goals){
        hard_goals = 0U;
    }
    cout <<  "Hard goals: "  << std::bitset<32>(hard_goals) << endl;
        
    cout << "pruning method: mugs_pruning prune: " << prune << endl;
}

MugsPruning::MugsPruning(const options::Options &opts)
    :   all_soft_goals(opts.get<bool>("all_softgoals")),
        prune(opts.get<bool>("prune")),
        max_heuristic(opts.get<Evaluator*>("h")){

    }

bool MugsPruning::is_superset(uint super, uint sub) const{
    uint diff = super ^ sub;
    return (diff == 0) || (((diff & super) > 0) && ((diff & sub) == 0));
}

bool MugsPruning::superset_contained(uint goal_subset, const unordered_set<uint> &set) const{
    for(uint gs : set){
        if(is_superset(gs, goal_subset)){
            //cout <<  "Superset: "  << std::bitset<32>(gs) << endl;
            return true;
        }
    }
    //msgs.insert(goal_subset);
    return false;
}

bool MugsPruning::insert_new_subset(uint goal_subset, unordered_set<uint> &set) const{
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

bool MugsPruning::insert_new_superset(uint goal_subset,  unordered_set<uint> &set, bool& inserted) const{
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

std::unordered_set<uint> MugsPruning::unsolvable_subgoals() const{
    unordered_set<uint> ugs;
    unordered_set<uint> candidates;
    //all subset with one goal fact
    for(int i = 0; i < num_goal_facts; i++){
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
                for(int i = 0; i < num_goal_facts; i++){
                    if(((gs & (1U << i)) == 0)){
                        candidates.insert(gs | (1U << i));
                    }
                }
            }
        }
    }

    /*
    cout << "----------------------------------"  << endl;
    cout << "Unsolvable goal subsets: " << endl;
    print_set(ugs);
    */
    return ugs;
}



std::unordered_set<uint> MugsPruning::minimal_unsolvable_subgoals(std::unordered_set<uint> &ugs) const{
     unordered_set<uint> mugs;
     for(uint gs : ugs){
         insert_new_subset(gs, mugs);
     }
     return mugs;
}


void MugsPruning::print_set(std::unordered_set<uint> s) const{
    cout << "Size: "  << s.size() << endl;
    for(uint gs : s){
        cout << std::bitset<32>(gs) << endl;
    }
}

bool MugsPruning::check_reachable(const State &state) {
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
    bool prune_state = superset_contained(reachable_gs, msgs);

    return prune_state;
}

void MugsPruning::add_goal_to_msgs(const State &state) {
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

bool MugsPruning::prune_state(const State &state){

    bool prune_state = check_reachable(state);
    add_goal_to_msgs(state);

    if(prune) {
        if (prune_state) {
            pruned_states++;
        }
        return prune_state;
    }
    else
        return false;
}


void MugsPruning::prune_operators(const State &state, std::vector<OperatorID> &){
    this->prune_state(state);
}

void MugsPruning::print_statistics() const{
    cout << "++++++++++ MSGS PRUNING +++++++++++++++" << endl;
    print_set(msgs);
    print_mugs();

    cout << "Pruned states: " << pruned_states << endl;
}

void MugsPruning::print_mugs() const{

    unordered_set<uint> ugs = unsolvable_subgoals();
    unordered_set<uint> mugs = minimal_unsolvable_subgoals(ugs);
    cout << "++++++++++ MUGS PRUNING +++++++++++++++" << endl;
    //print_set(mugs);
    //cout << "++++++++++++++++++++++++++++++++++++++++++++++++"  << endl;
    //cout << "num goal fact names: " << goal_fact_names.size() << endl;
    for(uint gs : mugs){
        for(int i = 0; i < num_goal_facts; i++){
            if(((1U << (num_goal_facts - 1 - i)) & gs) >= 1){
                cout << goal_fact_names[i] << "|";
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

    return make_shared<MugsPruning>(opts);
}


static PluginShared<PruningMethod> _plugin("mugs_pruning", _parse);
}
