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

class TrapUnsatHeuristic : public Heuristic {
public:
    TrapUnsatHeuristic(const options::Options& opts);
    virtual ~TrapUnsatHeuristic() = default;
    virtual void set_abstract_task(std::shared_ptr<AbstractTask> task) override;
    static void add_options_to_parser(options::OptionParser& parser);
protected:
    void initialize(unsigned k);
    virtual int compute_heuristic(const GlobalState &state) override;
    int compute_heuristic(const std::vector<unsigned>& state);

    bool are_dead_ends(const std::vector<unsigned>& phi);
    void progression(const std::vector<unsigned>& phi,
                     unsigned op,
                     std::vector<unsigned>& post);
    bool are_mutex(const std::vector<unsigned>& phi, unsigned op) const;

    void for_every_post_action(
            const std::vector<unsigned>& phi,
            std::function<bool(unsigned)> callback);

    void propagate_reachability_setup_formula();
    void forward_mark_unreachable(std::vector<unsigned>& queue);

    std::vector<PartialStateEvaluator*> m_evaluators;
    const conflict_driven_learning::strips::Task *m_task;

    std::vector<std::vector<unsigned> > m_action_post;
    std::vector<std::vector<bool> > m_is_mutex;

    segmented_vector::SegmentedVector<std::vector<unsigned> > m_conjunctions;
    std::vector<bool> m_mutex_with_goal;
    std::vector<int> m_goal_reachable;

    struct ForwardHyperTransition {
        unsigned label;
        unsigned source;
        std::vector<unsigned> destinations;
        ForwardHyperTransition(unsigned label, unsigned source)
            : label(label)
            , source(source)
        {}
    };
    segmented_vector::SegmentedVector<ForwardHyperTransition> m_hyperarcs;
    segmented_vector::SegmentedVector<std::vector<unsigned> > m_src_to_transition_ids;
    segmented_vector::SegmentedVector<std::vector<unsigned> > m_dest_to_transition_ids;
    std::vector<unsigned> m_transition_source;
    std::vector<unsigned> m_num_post_conjunctions;
    std::vector<unsigned> m_no_post_transitions;

    CounterBasedFormula m_formula;
};

}
}

#endif
