#ifndef HC_HEURISTIC_H
#define HC_HEURISTIC_H

#include "../algorithms/segmented_vector.h"
/* #include "../algorithms/priority_queues.h" */
#include "../heuristic.h"
#include "../global_state.h"
#include "../utils/timer.h"
#include "../option_parser.h"

#include <algorithm>
#include <memory>

namespace conflict_driven_learning
{
namespace hc_heuristic
{

/* void print_conjunction(const std::vector<unsigned> &conj, bool e = true); */

class PartialState
{
    std::vector<int> m_values;
public:
    enum {UNDEFINED = -1};
    PartialState();
    PartialState(const PartialState &state);
    PartialState(size_t size);
    void copy_from(const GlobalState &state);
    void clear(unsigned i);
    void clear();
    void resize(unsigned size);
    int operator[](unsigned i) const;
    int &operator[](unsigned i);
    bool is_defined(unsigned i) const;
    const std::vector<int> &values() const
    {
        return m_values;
    }
};

struct ConjunctionData;

struct Counter {
    unsigned id;

    unsigned preconditions;
    unsigned unsat;
    int action_cost;
    ConjunctionData *effect;

    ConjunctionData *max_pre;

    Counter()
        : id(-1), preconditions(-1), unsat(-1), action_cost(0), effect(NULL),
          max_pre(NULL)
    {}

    Counter(unsigned id, int action_cost, ConjunctionData *eff)
        : id(id), preconditions(-1), unsat(-1), action_cost(action_cost), effect(eff),
          max_pre(NULL)
    {}

    Counter(unsigned id, int action_cost, unsigned pre, ConjunctionData *eff)
        : id(id), preconditions(pre), unsat(pre), action_cost(action_cost), effect(eff),
          max_pre(NULL)
    {}
};

struct ConjunctionData {
    static constexpr const int UNACHIEVED = -1;
    unsigned id;
    int cost;
    std::vector<Counter *> pre_of;
    ConjunctionData() : id(-1), cost(UNACHIEVED) {}
    ConjunctionData(unsigned id) : id(id), cost(UNACHIEVED) {}
    ConjunctionData(unsigned id, int cost) : id(id), cost(cost) {}
    bool achieved() const
    {
        return cost != UNACHIEVED;
    }
};

class HCHeuristic;

class NoGoodFormula
{
private:
    utils::Timer m_refinement_timer;
    utils::Timer m_evaluation_timer;
protected:
    const AbstractTask& m_task;
    HCHeuristic *m_hc;
    virtual bool evaluate(const std::vector<unsigned> &conjunction_ids) = 0;
    virtual void refine(const PartialState &state) = 0;
public:
    NoGoodFormula(const AbstractTask& task,
                  HCHeuristic *hc);
    virtual void initialize() {}
    bool evaluate_formula(const std::vector<unsigned> &conjunction_ids);
    void refine_formula(const PartialState &state);
    const utils::Timer &get_refinement_timer() const;
    const utils::Timer &get_evaluation_timer() const;
    virtual void print_statistics() const = 0;
};

class HCHeuristic : public Heuristic
{
protected:
    const bool c_prune_subsumed_preconditions;
    bool c_early_termination;
    bool c_nogood_evaluation_enabled;
    size_t m_hc_evaluations;

    std::unique_ptr<NoGoodFormula> m_nogood_formula;

    size_t m_num_atomic_counters;

    // essential for hc computation
    ConjunctionData m_true_conjunction;
    ConjunctionData m_goal_conjunction;
    unsigned m_goal_counter;

    segmented_vector::SegmentedVector<Counter> m_counters;
    segmented_vector::SegmentedVector<ConjunctionData> m_conjunction_data;

    std::vector<std::vector<unsigned> > m_fact_to_conjunctions;
    std::vector<unsigned> m_conjunction_size;
    std::vector<unsigned> m_subset_count;

    std::vector<unsigned> m_state;
    PartialState m_partial_state;

    ////
    // data for counter/conjunction construction and C computation
    // per fact
    std::vector<std::vector<unsigned> > m_counters_with_fact_precondition;
    // per conjunction
    segmented_vector::SegmentedVector<std::vector<unsigned> > m_conjunctions;
    segmented_vector::SegmentedVector<std::vector<unsigned> > m_conjunction_achievers;
    // per counter
    segmented_vector::SegmentedVector<std::vector<unsigned> > m_counter_precondition;
    std::vector<unsigned> m_counter_to_action;
    //// adding new conjunctions

    void update_fact_conjunction_mapping(
        const std::vector<unsigned> &conj,
        unsigned conjid);

    int heuristic_computation_wrapper(const std::vector<unsigned> &conjids);
    int compute_heuristic(const PartialState &state);
    virtual int compute_heuristic(const GlobalState &state) override;

    void initialize();
public:
    static const int DEAD_END;

    HCHeuristic(const options::Options &opts);

    virtual void set_abstract_task(std::shared_ptr<AbstractTask> task) override;

    int evaluate(const GlobalState& state);

    bool set_early_termination(bool t);
    bool set_early_termination_and_nogoods(bool e);

    void get_satisfied_conjunctions(const std::vector<unsigned> &conj,
                                    std::vector<unsigned> &ids);
    void get_satisfied_conjunctions(const GlobalState &fdr_state,
                                    std::vector<unsigned> &ids);

    void refine_nogood_formulas(const PartialState &state);

    unsigned lookup_conjunction_id(
        const std::vector<unsigned> &conj,
        std::vector<unsigned> &subsumed,
        std::vector<unsigned> &subsumed_by);
    std::pair<unsigned, bool> insert_conjunction_and_update_data_structures(
        const std::vector<unsigned> &conj,
        int cost = 0,
        bool ensure_correct_cost_bound = false);
    unsigned insert_conjunction(const std::vector<unsigned> &conj, int cost = 0);
    unsigned create_counter(unsigned action_id, int cost, ConjunctionData *eff);

    int get_cost_of_subgoal(const std::vector<unsigned> &subgoal);
    bool is_subgoal_reachable(const std::vector<unsigned> &subgoal);
    int compute_heuristic_incremental(
        const std::vector<unsigned> &new_facts,
        std::vector<unsigned> &reachable_conjunctions);
    void revert_incremental_computation(
        const std::vector<unsigned> &new_facts,
        const std::vector<unsigned> &reachable_conjunctions);

    virtual void cleanup_previous_computation() = 0;
    virtual int compute_heuristic(const std::vector<unsigned> &conjunction_ids) = 0;
    virtual int compute_heuristic_get_reachable_conjunctions(
        std::vector<unsigned> &reachable_conjunctions) = 0;

    double get_counter_ratio() const;
    bool prune_subsumed_preconditions() const;
    size_t num_conjunctions() const;
    size_t num_counters() const;
    size_t num_atomic_counters() const;

    unsigned get_action_id(unsigned counter) const;
    unsigned get_conjunction_size(unsigned cid) const;
    const std::vector<unsigned> &get_conjunction(unsigned id) const;
    ConjunctionData &get_conjunction_data(unsigned id);
    Counter &get_counter(unsigned id);

    void mark_unachieved(unsigned id);

    const std::vector<unsigned> &get_conjunction_achievers(unsigned cid) const;

    ConjunctionData &get_true_conjunction_data();
    ConjunctionData &get_goal_conjunction_data();
    Counter &get_goal_counter();

    std::vector<unsigned> &get_counters_with_fact_precondition(unsigned p);
    std::vector<unsigned> &get_counter_precondition(unsigned id);

    virtual void print_statistics() const;
    virtual void print_options() const;

    virtual bool supports_partial_state_evaluation() const;

    static void add_options_to_parser(options::OptionParser &parser);

    template<typename Callback>
    void get_satisfied_conjunctions(const std::vector<unsigned> &conj,
                                    const Callback &callback)
    {
        std::fill(m_subset_count.begin(), m_subset_count.end(), 0);
        unsigned i = 0;
        unsigned end = conj.size();
        for (; i < end; i++) {
            const unsigned &p = conj[i];
            for (const unsigned &c : m_fact_to_conjunctions[p]) {
                if (++m_subset_count[c] == m_conjunction_size[c]) {
                    callback(c);
                }
            }
        }
    }
    template<typename Callback>
    void forall_superset_conjunctions(const std::vector<unsigned> &conj,
                                      const Callback &callback)
    {
        std::fill(m_subset_count.begin(), m_subset_count.end(), 0);
        unsigned i = 0;
        unsigned end = conj.size();
        for (; i < end; i++) {
            const unsigned &p = conj[i];
            for (const unsigned &c : m_fact_to_conjunctions[p]) {
                if (++m_subset_count[c] == conj.size()) {
                    callback(c);
                }
            }
        }
    }
};

class HCHeuristicUnitCost : public HCHeuristic
{
protected:
    std::vector<ConjunctionData *> m_open;
    bool enqueue_if_necessary(ConjunctionData *conj, const int &lvl);
public:
    using HCHeuristic::HCHeuristic;
    virtual void cleanup_previous_computation() override;
    virtual int compute_heuristic(const std::vector<unsigned> &conjunction_ids)
    override;
    virtual int compute_heuristic_get_reachable_conjunctions(
        std::vector<unsigned> &reachable_conjunctions)
    override;
};

}
}


#endif
