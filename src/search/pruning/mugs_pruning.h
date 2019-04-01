#ifndef MUGS_PRUNING_H
#define MUGS_PRUNING_H

#include <unordered_set>
#include "../pruning_method.h"
#include "../task_proxy.h"
#include "../heuristics/max_heuristic.h"

class GlobalState;

namespace mugs_pruning {
class MugsPruning : public PruningMethod {

std::unordered_set<uint> msgs;
Evaluator* max_heuristic;
int pruned_states = 0;

private:
    bool superset_contained(uint goal_subset);


public:
    explicit MugsPruning(const options::Options &opts);

    virtual void initialize(const std::shared_ptr<AbstractTask> &) override;
    virtual void prune_operators(const State &,std::vector<OperatorID> &) override {}
    virtual bool prune_state(const State &state) override;

    virtual void print_statistics() const override;
};
}

#endif
