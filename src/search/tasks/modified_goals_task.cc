#include "modified_goals_task.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <string>

using namespace std;

namespace extra_tasks {
ModifiedGoalsTask::ModifiedGoalsTask(
    const shared_ptr<AbstractTask> &parent,
    vector<FactPair> &&goals)
    : DelegatingTask(parent),
      goals(move(goals)) {
}

int ModifiedGoalsTask::get_num_goals() const {
    return goals.size();
}

FactPair ModifiedGoalsTask::get_goal_fact(int index) const {
    return goals[index];
}

static shared_ptr<AbstractTask> _parse_hard_goals(OptionParser &parser) {
    parser.add_option<shared_ptr<AbstractTask>>("transform", "", "no_transform()");
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else {
        shared_ptr<AbstractTask> task = opts.get<shared_ptr<AbstractTask>>("transform");
        std::vector<FactPair> hard_goals;
        for (int i = 0; i < task->get_num_goals(); i++) {
            FactPair g = task->get_goal_fact(i);
            std::string name = task->get_fact_name(g);
            if (name.compare(0, 5, "soft_") != 0) {
                hard_goals.push_back(std::move(g));
                // std::cout << name << std::endl;
            } else {
                // std::cout << "***NOT*** " << name << std::endl;
            }
        }
        return std::make_shared<ModifiedGoalsTask>(
                task,
                std::move(hard_goals));
    }
}

static PluginShared<AbstractTask> _plugin_hard_goals("hard_goals", _parse_hard_goals);
}
