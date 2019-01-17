#ifndef TASKS_MODIFIED_GOALS_INIT_TASK_H
#define TASKS_MODIFIED_GOALS_INIT_TASK_H

#include "delegating_task.h"

#include <vector>

namespace extra_tasks {
class ModifiedGoalsInitTask : public tasks::DelegatingTask {
    const std::vector<FactPair> goals;
    std::vector<int> init;

public:
    ModifiedGoalsInitTask(
        const std::shared_ptr<AbstractTask> &parent,
        std::vector<FactPair> &&goals, std::vector<FactPair> &&change_init);
    ~ModifiedGoalsInitTask() = default;

    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;
    virtual std::vector<int> get_initial_state_values() const override;
};
}

#endif
