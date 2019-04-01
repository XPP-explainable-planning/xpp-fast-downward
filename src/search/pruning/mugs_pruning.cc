#include "mugs_pruning.h"

#include "bitset"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace mugs_pruning {
void MugsPruning::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    cout << "pruning method: mugs_pruning" << endl;
}

MugsPruning::MugsPruning(const options::Options &opts)
    : max_heuristic(opts.get<Evaluator*>("h")){

    }

bool MugsPruning::superset_contained(uint goal_subset){
    for(uint gs : msgs){
        uint diff = gs ^ goal_subset;
        if((diff == 0) || ((diff > 0) && ((diff & gs) > 0) && ((diff & goal_subset) == 0))){
            cout <<  "Superset: "  << std::bitset<32>(gs) << endl;
            return true;
        }
    }
    //msgs.insert(goal_subset);
    return false;
}

bool MugsPruning::prune_state(const State &state){
    cout << "---------------------------------------------------------" << endl;
    uint reachable_gs = ((max_heuristic::HSPMaxHeuristic*) max_heuristic)->compute_relaxed_reachable_goal_facts(state);
    cout << "Reachable: " << std::bitset<32>(reachable_gs) << endl;
    bool superset_known = superset_contained(reachable_gs);

    uint current_sat_goal_facts = 0;
    TaskProxy task_proxy = TaskProxy(*task);
    GoalsProxy g_proxy = task_proxy.get_goals();
    for(uint i = 0; i < g_proxy.size(); i++){
       //cout << state[g_proxy[i].get_variable().get_id()].get_value() << " ?=? " << g_proxy[i].get_value() << endl;
       current_sat_goal_facts = (current_sat_goal_facts << 1) | (state[g_proxy[i].get_variable().get_id()].get_value() == g_proxy[i].get_value());
    }
    cout << "Current sat goal: " << std::bitset<32>(current_sat_goal_facts) << endl;
    
    if(! superset_contained(current_sat_goal_facts)){
        msgs.insert(current_sat_goal_facts);
        cout << " add " << endl;
    }
    cout << "++++++++++ MUGS PRUNING +++++++++++++++" << endl;
    for(uint gs : msgs){
        cout << std::bitset<32>(gs) << endl;
    }
    
    if(superset_known){
        pruned_states++;
    }
    return superset_known;
    //return false;
}

void MugsPruning::print_statistics() const{
    cout << "++++++++++ MUGS PRUNING +++++++++++++++" << endl;
    for(uint gs : msgs){
        cout << std::bitset<32>(gs) << endl;
    }
    cout << "Pruned states: " << pruned_states << endl;
}

static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "TODO",
        "TODO");

    parser.add_option<Evaluator*>(
        "h",
        "TODO",
        "hmax()");

    Options opts = parser.parse();

    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<MugsPruning>(opts);
}


static PluginShared<PruningMethod> _plugin("mugs_pruning", _parse);
}
