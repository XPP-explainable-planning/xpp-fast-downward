#include "modified_goals_init_task.h"

using namespace std;

namespace extra_tasks {
ModifiedGoalsInitTask::ModifiedGoalsInitTask(
    const shared_ptr<AbstractTask> &parent,
    vector<FactPair> &&goals, vector<FactPair> &&change_init)
    : DelegatingTask(parent),
      goals(move(goals)) {

          init = parent->get_initial_state_values();
          for(FactPair fp : change_init){
              init[fp.var] = fp.value;
          }
}

int ModifiedGoalsInitTask::get_num_goals() const {
    return goals.size();
}

FactPair ModifiedGoalsInitTask::get_goal_fact(int index) const {
    return goals[index];
}

std::vector<int> ModifiedGoalsInitTask::get_initial_state_values() const {
    /*
    cout << "Current inital state: " << endl;
    for(int i : init){
        cout << i << " " ;
    }
    cout << endl;
    */
    return init;
}
}
