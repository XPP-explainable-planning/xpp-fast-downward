#ifndef CRITICAL_PATH_MUGS_PRUNING_H
#define CRITICAL_PATH_MUGS_PRUNING_H

#include "mugs_pruning.h"
#include "../task_proxy.h"
#include "../conflict_driven_learning/hc_heuristic.h"

#include <vector>

namespace mugs_pruning {

class CriticalPathMugsPruning : public MugsPruning
{
public:
    explicit CriticalPathMugsPruning(const options::Options& opts);
    virtual void initialize(const std::shared_ptr<AbstractTask>& task) override;
    virtual void prune_operators(const State& state, std::vector<OperatorID>& ops) override;

protected:
    virtual bool check_reachable(const State &state) override;
    bool is_mug_reachable(uint mug, int gidx, int unreached);
    // void update_refinement_goal();

private:
    conflict_driven_learning::hc_heuristic::HCHeuristic* hc_;
    uint all_goals_;
    std::vector<int> goal_var_idx_;
    std::vector<unsigned> full_goal_;
    std::vector<unsigned> goal_conjunctions_;
    std::vector<std::vector<unsigned> > references_; 
    unsigned prev_num_conjunctions_;
    std::vector<int> not_in_mug_;
};

}

#endif
