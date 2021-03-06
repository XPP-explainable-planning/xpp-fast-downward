#ifndef ABSTRACT_TASK_H
#define ABSTRACT_TASK_H

#include "operator_id.h"

#include "algorithms/subscriber.h"
#include "utils/hash.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <string>

namespace options {
class Options;
}

struct FactPair {
    int var;
    int value;

    FactPair(int var, int value)
        : var(var), value(value) {
    }

    bool operator<(const FactPair &other) const {
        return var < other.var || (var == other.var && value < other.value);
    }

    bool operator==(const FactPair &other) const {
        return var == other.var && value == other.value;
    }

    bool operator!=(const FactPair &other) const {
        return var != other.var || value != other.value;
    }

    /*
      This special object represents "no such fact". E.g., functions
      that search a fact can return "no_fact" when no matching fact is
      found.
    */
    static const FactPair no_fact;
};


struct Property {
    std::string name;
    std::string formula;

    Property(std::string name, std::string formula)
            : name(name), formula(formula) {
    }
};

std::ostream &operator<<(std::ostream &os, const FactPair &fact_pair);

namespace std {
template<>
struct hash<FactPair> {
    size_t operator()(const FactPair &fact) const {
        std::pair<int, int> raw_fact(fact.var, fact.value);
        std::hash<std::pair<int, int>> hasher;
        return hasher(raw_fact);
    }
};
}

class AbstractTask : public subscriber::SubscriberService<AbstractTask> {
public:
    AbstractTask() = default;
    virtual ~AbstractTask() override = default;
    virtual int get_num_variables() const = 0;
    virtual std::string get_variable_name(int var) const = 0;
    virtual int get_variable_domain_size(int var) const = 0;
    virtual int get_variable_axiom_layer(int var) const = 0;
    virtual int get_variable_default_axiom_value(int var) const = 0;
    virtual std::string get_fact_name(const FactPair &fact) const = 0;
    virtual bool are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const = 0;

    virtual int get_operator_cost(int index, bool is_axiom) const = 0;
    virtual std::string get_operator_name(int index, bool is_axiom) const = 0;
    virtual int get_num_operators() const = 0;
    virtual int get_num_operator_preconditions(int index, bool is_axiom) const = 0;
    virtual FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const = 0;
    virtual int get_num_operator_effects(int op_index, bool is_axiom) const = 0;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const = 0;
    virtual FactPair get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const = 0;
    virtual FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const = 0;
    virtual OperatorID get_global_operator_id(OperatorID id) const = 0;

    virtual int get_num_axioms() const = 0;

    virtual int get_num_goals() const = 0;
    virtual FactPair get_goal_fact(int index) const = 0;

    virtual int get_num_hard_goals() const = 0;
    virtual FactPair get_hard_goal_fact(int index) const = 0;

    virtual int get_num_soft_goals() const = 0;
    virtual FactPair get_soft_goal_fact(int index) const = 0;

    virtual int get_num_question() const = 0;
    virtual FactPair get_question_fact(int index) const = 0;

    virtual int get_num_LTL_properties() const = 0;
    virtual Property get_LTL_property(int index) const = 0;

    virtual int get_num_entailments() const = 0;
    virtual std::vector<FactPair> get_entailment(int index) const = 0;

    virtual std::vector<int> get_initial_state_values() const = 0;
    /*
      Convert state values from an ancestor task A (ancestor) into
      state values from this task, C (child). Task A has to be an
      ancestor of C in the sense that C is the result of a sequence of
      task transformations on A.
      The values are converted in-place to avoid unnecessary copies. If a
      subclass needs to create a new vector, e.g., because the size changes,
      it should create the new vector in a local variable and then swap it with
      the parameter.
    */
    virtual void convert_state_values(
        std::vector<int> &values,
        const AbstractTask *ancestor_task) const = 0;
};

#endif
