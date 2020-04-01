#include "mugs_hmax_heuristic.h"

#include "../global_state.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../evaluation_context.h"
#include "../plan_properties/MUGS.h"

#include <cstddef>
#include <limits>
#include <utility>
#include <bitset>

using namespace std;

namespace mugs_hmax_heuristic {
    MugsHmaxHeuristic::MugsHmaxHeuristic(const Options &opts)
    : Heuristic(opts),
      all_soft_goals(opts.get<bool>("all_softgoals")),
      prune(opts.get<bool>("prune")),
      use_cost_bound_reachable(opts.get<bool>("use_cost_bound_reachable")),
      cost_bound(opts.get<int>("cost_bound")),
      max_heuristic(opts.get<Evaluator*>("h")){

        num_goal_facts = task_proxy.get_goals().size();

        for(int i = 0; i < num_goal_facts; i++){
            FactProxy gp = task_proxy.get_goals()[i];
            int id = gp.get_variable().get_id();
            int value = gp.get_value();
            // old way to check if softgoal
            //hard_goals = (hard_goals << 1) |(task_proxy.get_variables()[id].get_fact(value).get_name().find("soft") != 0);
            bool found = false;
            for(uint j = 0; task_proxy.get_hard_goals().size(); j++) {
                found = found | (task_proxy.get_hard_goals()[j].get_variable().get_id() == id);
            }
            hard_goals = (hard_goals << 1) | found;
            goal_fact_names.push_back(task_proxy.get_variables()[id].get_fact(value).get_name());
        }

        if(all_soft_goals){
            hard_goals = 0U;
        }

        cout <<  "Hard goals: "  << std::bitset<32>(hard_goals) << endl;
        cout << "Initializing mugs hmax heuristic..." << endl;
}

    MugsHmaxHeuristic::~MugsHmaxHeuristic() {
}

    bool MugsHmaxHeuristic::is_superset(uint super, uint sub) const{
        uint diff = super ^ sub;
        return (diff == 0) || (((diff & super) > 0) && ((diff & sub) == 0));
    }

    bool MugsHmaxHeuristic::superset_contained(uint goal_subset, const unordered_set<uint> &set) const{

        for(uint gs : set){
            if(is_superset(gs, goal_subset)){
                return true;
            }
        }
        return false;
    }

    bool MugsHmaxHeuristic::insert_new_subset(uint goal_subset, unordered_set<uint> &set) const{
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

    bool MugsHmaxHeuristic::insert_new_superset(uint goal_subset,  unordered_set<uint> &set, bool& inserted) const{
        inserted = false;

        //check if superset is already contained in set
        for(uint gs : set){
            if(is_superset(gs, goal_subset))
                return true;
        }

        //remove all sets which are a subset of the new set
        auto it=set.begin();
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

    std::unordered_set<uint> MugsHmaxHeuristic::unsolvable_subgoals() const{
        unordered_set<uint> ugs;
        unordered_set<uint> candidates;
        //all subset with one goal fact
        for(int i = 0; i < num_goal_facts; i++){
            candidates.insert(1U << i);
        }

        while(!candidates.empty()){
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

        return ugs;
    }



    std::unordered_set<uint> MugsHmaxHeuristic::minimal_unsolvable_subgoals(std::unordered_set<uint> &ugs) const{
        unordered_set<uint> mugs;
        for(uint gs : ugs){
            insert_new_subset(gs, mugs);
        }
        return mugs;
    }


    void MugsHmaxHeuristic::print_set(std::unordered_set<uint> s) const{
        cout << "Size: "  << s.size() << endl;
        for(uint gs : s){
            cout << std::bitset<32>(gs) << endl;
        }
    }

    bool MugsHmaxHeuristic::check_reachable(const State &state, int remaining_cost) {
        uint reachable_gs = 0;
        if(use_cost_bound_reachable) {
            reachable_gs = ((max_heuristic::HSPMaxHeuristic *) max_heuristic)->compute_relaxed_reachable_goal_facts(
                    state, remaining_cost);
        }
        else {
            reachable_gs = ((max_heuristic::HSPMaxHeuristic *) max_heuristic)->compute_relaxed_reachable_goal_facts(state);
        }

        //if a hard goal is not reachable prune the state
        if((hard_goals & reachable_gs) != hard_goals){
            //cout << "Hard goal not reachable" << endl;
            return true;
        }

        // if a superset of states was already reached -> prune state
        bool prune_state = superset_contained(reachable_gs, msgs);

        return prune_state;
    }

    void MugsHmaxHeuristic::add_goal_to_msgs(const State &state) {
        uint current_sat_goal_facts = 0;
        TaskProxy task_proxy = TaskProxy(*task);
        GoalsProxy g_proxy = task_proxy.get_goals();

        for(uint i = 0; i < g_proxy.size(); i++){
            current_sat_goal_facts = (current_sat_goal_facts << 1) |
                    (state[g_proxy[i].get_variable().get_id()].get_value() == g_proxy[i].get_value());
        }

        //if all hard goals are satisfied add set
        msgs_changed = false;
        if((hard_goals & current_sat_goal_facts) == hard_goals){
            // only adds set of msgs does not contain any superset
            insert_new_superset(current_sat_goal_facts, msgs, msgs_changed);
        }

    }


    EvaluationResult MugsHmaxHeuristic::compute_result(EvaluationContext &eval_context){
        int remaining_cost = cost_bound - eval_context.get_g_value();
        State state = convert_global_state(eval_context.get_state());
        bool prune_state = check_reachable(state, remaining_cost);
        add_goal_to_msgs(state);

        EvaluationResult result;
        if(prune && prune_state) {
                pruned_states++;
                result.set_evaluator_value(remaining_cost + 1);
                return result;
        }

        result.set_evaluator_value(0);
        return result;

    }

    int MugsHmaxHeuristic::compute_heuristic(const GlobalState &) {
        return 0;
    }

    void MugsHmaxHeuristic::print_statistics() const{
        cout << "++++++++++ MSGS HEURISTIC +++++++++++++++" << endl;
        print_set(msgs);
        print_mugs();

        cout << "Pruned states: " << pruned_states << endl;
    }

    void MugsHmaxHeuristic::print_evaluator_statistics() const {
        this->print_statistics();
    }

    void MugsHmaxHeuristic::print_mugs() const{

        unordered_set<uint> ugs = unsolvable_subgoals();
        unordered_set<uint> mugs = minimal_unsolvable_subgoals(ugs);

        //print mugs to file
        MUGS mugs_store =  MUGS(mugs, goal_fact_names);
        mugs_store.output_mugs();

        cout << "++++++++++ MUGS HEURISTIC +++++++++++++++" << endl;
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


static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("MUGS Hmax heuristic",
                             "TODO "
                             "non-goal states, "
                             "0 for goal states");
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
    parser.add_option<bool>(
            "use_cost_bound_reachable",
            "TODO",
            "false");
    parser.add_option<int>(
            "cost_bound",
            "TODO",
            "infinity");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new MugsHmaxHeuristic(opts);
}

static Plugin<Evaluator> _plugin("mugs_hmax", _parse);
}
