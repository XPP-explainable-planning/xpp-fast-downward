#ifndef TRAP_UNSAT_HEURISTIC_H
#define TRAP_UNSAT_HEURISTIC_H

#include "formula.h"
#include "../heuristic.h"
#include "../algorithms/segmented_vector.h"

#include <vector>
#include <functional>

namespace conflict_driven_learning {

class PartialStateEvaluator;

namespace strips {
struct Task;
}

namespace traps {

struct ForwardHyperTransition {
    unsigned label;
    unsigned source;
    std::vector<unsigned> destinations;
    ForwardHyperTransition(unsigned label, unsigned source)
        : label(label)
        , source(source)
    {}
    ForwardHyperTransition(const ForwardHyperTransition& other) = default;
    ForwardHyperTransition& operator=(ForwardHyperTransition&& other) = default;
    bool operator==(const ForwardHyperTransition& other) const;
};

class TrapUnsatHeuristic : public Heuristic {
public:
    using Formula = CounterBasedFormula;

    TrapUnsatHeuristic(const options::Options& opts);
    virtual ~TrapUnsatHeuristic() = default;
    virtual void set_abstract_task(std::shared_ptr<AbstractTask> task) override;

    bool for_every_regression_action(
            const std::vector<unsigned>& conj,
            std::function<bool(unsigned)> callback);
    bool for_every_progression_action(
            const std::vector<unsigned>& conj,
            std::function<bool(unsigned)> callback);
    void progression(const std::vector<unsigned>& conj,
                     unsigned op,
                     std::vector<unsigned>& post);
    bool are_dead_ends(const std::vector<unsigned>& conj);
    bool can_reach_goal(unsigned conjid) const;
    const Formula& get_all_conjunctions_formula() const;
    Formula& get_all_conjunctions_formula();
    bool evaluate_check_dead_end(const GlobalState& state);

    std::pair<unsigned, bool> insert_conjunction(const std::vector<unsigned>& conj);
    void set_transitions(unsigned conj_id, std::vector<ForwardHyperTransition>&& transitions);
    bool add_to_transition_post(const unsigned& source, const unsigned& op, const unsigned& dest);

    void update_reachability_insert_conjunctions();

    static void add_options_to_parser(options::OptionParser& parser);
protected:
    void initialize(unsigned k);
    virtual int compute_heuristic(const GlobalState &state) override;
    int compute_heuristic(const std::vector<unsigned>& state);

    bool are_mutex(const std::vector<unsigned>& conj, unsigned op) const;

    void propagate_reachability_setup_formula();

    std::vector<PartialStateEvaluator*> m_evaluators;
    const conflict_driven_learning::strips::Task *m_task;

    std::vector<std::vector<unsigned> > m_action_post;
    std::vector<std::vector<bool> > m_is_mutex;

    segmented_vector::SegmentedVector<std::vector<unsigned> > m_conjunctions;
    std::vector<bool> m_mutex_with_goal;
    std::vector<int> m_goal_reachable;

    segmented_vector::SegmentedVector<ForwardHyperTransition> m_hyperarcs;
    segmented_vector::SegmentedVector<std::vector<unsigned> > m_src_to_transition_ids;
    segmented_vector::SegmentedVector<std::vector<unsigned> > m_dest_to_transition_ids;
    std::vector<unsigned> m_transition_source;
    std::vector<unsigned> m_num_post_conjunctions;
    std::vector<unsigned> m_no_post_transitions;

    UBTreeFormula<unsigned> m_formula;
    Formula m_formula_all;
};

}
}

#endif
