#ifndef SEARCH_ENGINES_GOAL_RELATION_SEARCH_H
#define SEARCH_ENGINES_GOAL_RELATION_SEARCH_H

#include "../option_parser_util.h"
#include "../search_engine.h"
#include "../goal_relation/meta_search_tree.h"

namespace options {
class Options;
}

class Heuristic;

namespace goal_relation_search {
class GoalRelationSearch : public SearchEngine {
    const std::vector<options::ParseTree> engine_configs;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;
    bool all_soft_goals;
    std::vector<Heuristic *> heuristic;

    MetaSearchType meta_search_type;

    int phase;
    //int algo_phase;
    bool last_phase_found_solution;
    //int best_bound;
    bool iterated_found_solution;

    int num_solved_nodes = 0;

    mst::MetaSearchTree* metasearchtree;

    std::shared_ptr<SearchEngine> get_search_engine(int engine_config_start_index);
    std::shared_ptr<SearchEngine> create_phase(int phase);
    SearchStatus step_return_value();

    virtual SearchStatus step() override;

public:
    explicit GoalRelationSearch(const options::Options &opts);

    virtual void save_plan_if_necessary() override;
    virtual void print_statistics() const override;
};
}

#endif
