#include "mugs_heuristic.h"

#include "../abstract_task.h"
#include "../evaluation_context.h"
#include "../global_state.h"
#include "../option_parser.h"

#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace conflict_driven_learning {
namespace mugs {

MugsHeuristic::MugsHeuristic(const options::Options& opts)
    : Heuristic(opts)
    , cost_bound_(opts.get<int>("cost_bound"))
    , is_cost_bounded_(cost_bound_ > 0)
    , mugs_based_pruning_(opts.get<bool>("prune"))
    , on_removed_([this](const subgoal_t& s) { on_removed_subgoal(s); })
{
    std::cout << "Initializing MugsHeuristic ..." << std::endl;
    std::cout << "Using cost-bound: ";
    if (is_cost_bounded_) {
        std::cout << cost_bound_ << std::endl;
    } else {
        std::cout << "infinity" << std::endl;
    }

    // two passes over goal to make sure goal_assignment is sorted

    goal_assignment_.reserve(task->get_num_goals());
    for (int i = 0; i < task->get_num_goals(); i++) {
        FactPair g = task->get_goal_fact(i);
        goal_assignment_.emplace_back(g.var, g.value);
    }
    std::sort(goal_assignment_.begin(), goal_assignment_.end());

    hard_goal_ = 0U;
    goal_fact_names_.reserve(task->get_num_goals());
    for (unsigned i = 0; i < goal_assignment_.size(); i++) {
        FactPair g(goal_assignment_[i].first, goal_assignment_[i].second);
        goal_fact_names_.push_back(task->get_fact_name(g));
        if (goal_fact_names_.back().find("soft") != 0) {
            hard_goal_ = hard_goal_ | (1U << i);
        }
    }

    if (opts.get<bool>("all_softgoals")) {
        hard_goal_ = 0U;
    }

    std::cout << "Hard goals: " << to_string(hard_goal_) << std::endl;
}

void
MugsHeuristic::add_options_to_parser(options::OptionParser& parser)
{
    parser.add_option<bool>("all_softgoals", "TODO", "false");
    parser.add_option<bool>("prune", "TODO", "true");
    parser.add_option<int>("cost_bound", "", "-1");
    Heuristic::add_options_to_parser(parser);
}

void
MugsHeuristic::get_goal_facts(
    const subgoal_t& sg,
    std::vector<std::pair<int, int>>& sat) const
{
    mugs::get_goal_facts(sg, goal_assignment_, sat);
}

EvaluationResult
MugsHeuristic::compute_result(EvaluationContext& context)
{
    subgoal_t sg = get_subgoals(context.get_state(), goal_assignment_);
    if ((sg & hard_goal_) == hard_goal_) {
#ifndef NDEBUG
        auto mugs_before = get_mugs();
#endif
        if (max_achieved_subgoals_.insert(sg, on_removed_)) {
            on_added_subgoal(sg);
        }
#ifndef NDEBUG
        auto mugs_after = get_mugs();
        for (auto it = mugs_after.begin(); it != mugs_after.end(); ++it) {
            bool iss = false;
            for (auto it2 = mugs_before.begin(); it2 != mugs_before.end();
                 ++it2) {
                if (is_superset(*it, *it2)) {
                    iss = true;
                    break;
                }
            }
            assert(iss);
        }
#endif
    }
    EvaluationResult result;
    result.set_count_evaluation(mugs_based_pruning_);
    if (mugs_based_pruning_ && !is_any_mug_reachable(context)) {
        result.set_evaluator_value(
            is_cost_bounded_ ? cost_bound_ - context.get_g_value()
                             : EvaluationResult::INFTY);
    } else {
        result.set_evaluator_value(0);
    }
    return result;
}

void
MugsHeuristic::print_evaluator_statistics() const
{
    auto mugs =
        max_achieved_subgoals_.get_minimal_extensions(goal_assignment_.size());
    std::cout << "++++++++++ MUGS HEURISTIC +++++++++++++++" << std::endl;
    std::cout << "Size: " << max_achieved_subgoals_.size() << std::endl;
    print_set(max_achieved_subgoals_.begin(), max_achieved_subgoals_.end());
    std::cout << "++++++++++ MUGS HEURISTIC +++++++++++++++" << std::endl;
    print_set(mugs.begin(), mugs.end(), goal_fact_names_, hard_goal_);
    std::cout << "++++++++++++++++++++++++++++++++++++++++++++++++"
              << std::endl;
    std::cout << "Number of minimal unsolvable goal subsets: " << mugs.size()
              << std::endl;
}

} // namespace mugs
} // namespace conflict_driven_learning
