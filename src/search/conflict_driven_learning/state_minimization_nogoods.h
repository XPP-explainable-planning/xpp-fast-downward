#ifndef STATE_MINIMIZATION_HC_NOGOODS_H
#define STATE_MINIMIZATION_HC_NOGOODS_H

#include "hc_heuristic.h"
#include "formula.h"
#include "../algorithms/segmented_vector.h"

#include <vector>

namespace conflict_driven_learning
{
namespace hc_heuristic
{

class StateMinimizationNoGoods : public NoGoodFormula
{
protected:
    using Formula = UBTreeFormula<unsigned>;
    Formula m_formula;

    std::vector<unsigned> m_clause;
    std::vector<unsigned> m_new_facts;
    std::vector<unsigned> m_reachable_conjunctions;

    /* multiple goal sets support */
    std::vector<unsigned> m_full_goal;
    segmented_vector::SegmentedVector<std::vector<unsigned> > m_clauses;
    std::map<unsigned, std::vector<unsigned> > m_conjs_to_clauses;

    virtual bool evaluate(const std::vector<unsigned> &conjunction_ids) override;
    virtual void refine(const PartialState &state) override;
public:
    using NoGoodFormula::NoGoodFormula;
    virtual void initialize() override;
    virtual void synchronize_goal() override;
    virtual void print_statistics() const override;
};

}
}

#endif
