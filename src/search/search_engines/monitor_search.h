#ifndef SEARCH_ENGINES_MONITOR_SEARCH_H
#define SEARCH_ENGINES_MONITOR_SEARCH_H

#include "../open_list.h"
#include "../search_engine.h"

#include <memory>
#include <vector>

class Evaluator;
class PruningMethod;

namespace options {
class Options;
}

namespace monitor_search {
class MonitorSearch : public SearchEngine {
    const bool reopen_closed_nodes;
    const bool prune_by_f;

    std::unique_ptr<StateOpenList> open_list;
    Evaluator *f_evaluator;

    std::vector<Evaluator *> path_dependent_evaluators;
    std::vector<Evaluator *> preferred_operator_evaluators;
    Evaluator *lazy_evaluator;

    std::shared_ptr<PruningMethod> pruning_method;

    std::pair<SearchNode, bool> fetch_next_node();
    void start_f_value_statistics(EvaluationContext &eval_context);
    void update_f_value_statistics(const SearchNode &node);
    void reward_progress();
    void print_checkpoint_line(int g) const;

protected:
    virtual void initialize() override;
    virtual SearchStatus step() override;

public:
    explicit MonitorSearch(const options::Options &opts);
    virtual ~MonitorSearch() = default;

    virtual void print_statistics() const override;

    void dump_search_space() const;
};
}

#endif
