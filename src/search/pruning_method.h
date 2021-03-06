#ifndef PRUNING_METHOD_H
#define PRUNING_METHOD_H

#include "operator_id.h"
#include "task_proxy.h"

#include <memory>
#include <vector>

class AbstractTask;
class GlobalState;

class PruningMethod {
protected:
    std::shared_ptr<AbstractTask> task;

public:
    PruningMethod();
    virtual ~PruningMethod() = default;

    virtual void initialize(const std::shared_ptr<AbstractTask> &task);

    /* This method must not be called for goal states. This can be checked
       with assertions in derived classes. */
    virtual void prune_operators(const State &state,
                                 std::vector<OperatorID> &op_ids) = 0;
    // TODO remove this overload once the search uses the task interface.
    virtual void prune_operators(const GlobalState &state,
                                 std::vector<OperatorID> &op_ids);

    virtual bool prune_state(const State &state);
    virtual bool prune_state(const GlobalState &state);
    virtual bool prune_init_state(const GlobalState &state);
    virtual bool prune_state(StateID parent_id, const GlobalState &state, bool* new_automaton_state_reached);

    virtual void print_statistics() const = 0;
};

#endif
