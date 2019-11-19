#include "critical_path_mugs_pruning.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../conflict_driven_learning/strips_compilation.h"

#include <cassert>
#include <algorithm>

using namespace conflict_driven_learning;

namespace mugs_pruning {

CriticalPathMugsPruning::CriticalPathMugsPruning(const options::Options& opts)
    : MugsPruning(opts)
    , hc_(dynamic_cast<hc_heuristic::HCHeuristic*>(opts.get<Evaluator*>("hc")))
    , prev_num_conjunctions_(0)
{
    assert(hc_ != nullptr);
}

void
CriticalPathMugsPruning::initialize(const std::shared_ptr<AbstractTask>& task)
{
    MugsPruning::initialize(task);
    std::vector<int> goal_vars;
    for (int i = 0; i < task->get_num_goals(); i++) {
        FactPair g = task->get_goal_fact(i);
        goal_vars.push_back(g.var);
    }
    std::sort(goal_vars.begin(), goal_vars.end());
    goal_var_idx_.resize(task->get_num_variables(), -1);
    for (int i = goal_vars.size() - 1; i >= 0; i--) {
        goal_var_idx_[goal_vars[i]] = i;
    }
}

void
CriticalPathMugsPruning::prune_operators(const State& , std::vector<OperatorID>& )
{
    return;
}

// void
// CriticalPathMugsPruning::update_refinement_goal()
// {
//     if (msgs_changed) {
//         // pick any unsolved sub-goal
//         // (ideally, would want to pick minimal unsolved sub-goal -- but not
//         // sure how to do this here efficiently)
//         uint msg = (*msgs.begin());
//         uint ug = msg;
//         for (int i = num_goal_facts - 1; msg == ug && i >= 0; i--) {
//             ug = ug | (1U << i); 
//         }
// 
//         std::vector<std::pair<int, int> > new_goal;
//         for (int i = 0; i < num_goal_facts; i++) {
//             if ((ug >> i) & 1U) {
//                 FactPair g = task->get_goal_fact(i);
//                 new_goal.emplace_back(g.var, g.value);
//             }
//         }
//         std::sort(new_goal.begin(), new_goal.end());
//         hc_->set_auxiliary_goal(std::move(new_goal));
//     }
// }

bool
CriticalPathMugsPruning::check_reachable(const State& state)
{
    // update_refinement_goal();

    std::vector<unsigned> fact_ids;
    for (unsigned var = 0; var < state.get_values().size(); var++) {
        fact_ids.push_back(strips::get_fact_id(var, state.get_values()[var]));
    }
    hc_->compute_heuristic_for_facts(fact_ids);

    if (prev_num_conjunctions_ != hc_->num_conjunctions()) {
        prev_num_conjunctions_ = hc_->num_conjunctions();
        not_in_mug_.resize(hc_->num_conjunctions());
        goal_conjunctions_.clear();
        goal_conjunctions_.resize(num_goal_facts);
        const std::vector<unsigned>& goal_conjs =
            hc_->get_counter_precondition(hc_->get_goal_counter().id);
        for (unsigned i = 0; i < goal_conjs.size(); i++) {
            const std::vector<unsigned>& facts = hc_->get_conjunction(goal_conjs[i]);
            for (int j = facts.size() - 1; j >= 0; j--) {
                int var = strips::get_variable_assignment(facts[j]).first;
                goal_conjunctions_[goal_var_idx_[var]].push_back(goal_conjs[i]);
            }
        }
    } 

    std::fill(not_in_mug_.begin(), not_in_mug_.end(), 0);
    int num_unreached = 0;
    const std::vector<unsigned>& goal_conjs =
        hc_->get_counter_precondition(hc_->get_goal_counter().id);
    for (int i = goal_conjs.size() - 1; i >= 0; i--) {
        if (!hc_->get_conjunction_data(goal_conjs[i]).achieved()) {
            num_unreached++;
        }
    }

    uint mug = ~0U;
    return num_unreached > 0
        && !is_mug_reachable(mug, num_goal_facts - 1, num_unreached);
}

bool
CriticalPathMugsPruning::is_mug_reachable(uint mug, int gidx, int num_unreached)
{
    // remove soft goals and recursively check reachability
    for (int i = gidx; i >= 0; i--) {
        // dont remove hard goals
        if ((hard_goals >> i) & 1U) {
            continue;
        }
        
        // set flag to false
        uint successor = mug & ~(1U << i);
        
        // don't care about sub-goals that have already been reached
        if (msgs.count(successor)) {
            continue;
        }

        // remove conjunctions that contain this fact
        int new_unreached = num_unreached;
        const auto& gcs = goal_conjunctions_[i];
        for (int j = gcs.size() - 1; j >= 0; j--) {
            unsigned c = gcs[j];
            if (++not_in_mug_[c] == 1
                    && !hc_->get_conjunction_data(c).achieved()
                    && --new_unreached == 0) {
                return true;
            }
        }

        // check reachability recursively
        if (is_mug_reachable(successor, i - 1, new_unreached)) {
            return true;
        }

        // reset data structures
        for (int j = gcs.size() - 1; j >= 0; j--) {
            --not_in_mug_[gcs[j]];
        }
    }

    return false;
}

static std::shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.add_option<Evaluator*>("hc");
    parser.add_option<Evaluator*>(
        "h",
        "TODO",
        "hmax()");
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

    return std::make_shared<CriticalPathMugsPruning>(opts);
}


static PluginShared<PruningMethod> _plugin("hc_mugs_pruning", _parse);

}
