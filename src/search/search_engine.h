#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include "operator_cost.h"
#include "operator_id.h"
#include "plan_manager.h"
#include "search_progress.h"
#include "search_space.h"
#include "search_statistics.h"
#include "state_registry.h"
#include "task_proxy.h"
#include "tasks/modified_disjunctive_goal_task.h"

#include <vector>

namespace options {
class OptionParser;
class Options;
}

namespace ordered_set {
template<typename T>
class OrderedSet;
}

enum SearchStatus {IN_PROGRESS, TIMEOUT, FAILED, SOLVED};

class SearchEngine {
    SearchStatus status;
    bool solution_found;

    Plan plan;
protected:
    // Hold a reference to the task implementation and pass it to objects that need it.
    std::shared_ptr<AbstractTask> task;
    // Use task_proxy to access task information.
    TaskProxy task_proxy;
    

    PlanManager plan_manager;
    StateRegistry state_registry;
    SearchSpace search_space;
    SearchProgress search_progress;
    SearchStatistics statistics;
    int bound;
    OperatorCost cost_type;
    double max_time;

    virtual void initialize() {}
    virtual SearchStatus step() = 0;

    void set_plan(const Plan &plan);
    bool check_goal_and_set_plan(const GlobalState &state);
    int get_adjusted_cost(const OperatorProxy &op) const;
public:
    bool run_finished_successfully; // mugs search finished successfully
    SearchEngine(const options::Options &opts);
    SearchEngine(const options::Options &opts, std::shared_ptr<AbstractTask> t);
    virtual ~SearchEngine();
    virtual void print_statistics() const;
    virtual void save_plan_if_necessary();
    bool found_solution() const;
    SearchStatus get_status() const;
    const Plan &get_plan() const;
    void search();
    const SearchStatistics &get_statistics() const {return statistics;}
    void set_bound(int b) {bound = b;}
    int get_bound() {return bound;}
    PlanManager &get_plan_manager() {return plan_manager;}
    void setTask(const std::shared_ptr<AbstractTask> t){ 
        //task = std::make_shared<extra_tasks::ModifiedDisjunctiveGoalTask>(t.get()); //TODO
        task = t;
        task_proxy = TaskProxy(*t.get());
        state_registry.set_task(*task);
        /*
        std::cout << "setTask" << std::endl;

        std::cout <<  "Num goals: " << t->get_num_goals() << std::endl;

        std::cout << "Goals: " << std::endl;
        for(uint i = 0; i < task_proxy.get_goals().size(); i++){
            FactProxy gp = task_proxy.get_goals()[i];
            std::cout << "var" << gp.get_variable().get_id() << " = " << gp.get_value() << std::endl;
        }
        */
    }
    const std::shared_ptr<AbstractTask> & getTask() {return task;}
    virtual double get_heuristic_refinement_time() const { return 0; }

    /* The following three methods should become functions as they
       do not require access to private/protected class members. */
    static void add_pruning_option(options::OptionParser &parser);
    static void add_options_to_parser(options::OptionParser &parser);
    static void add_succ_order_options(options::OptionParser &parser);
};

/*
  Print evaluator values of all evaluators evaluated in the evaluation context.
*/
extern void print_initial_evaluator_values(const EvaluationContext &eval_context);

extern ordered_set::OrderedSet<OperatorID> collect_preferred_operators(
    EvaluationContext &eval_context,
    const std::vector<Evaluator *> &preferred_operator_evaluators);

#endif
