#ifndef STATE_MINIMIZATION_HC_NOGOODS_H
#define STATE_MINIMIZATION_HC_NOGOODS_H

#include "hc_heuristic.h"
#include "formula.h"

#include <vector>

namespace conflict_driven_learning
{
namespace hc_heuristic
{

class StateMinimizationNoGoods : public NoGoodFormula
{
protected:
    UBTreeFormula<unsigned> m_formula;
    std::vector<unsigned> m_clause;
    std::vector<unsigned> m_new_facts;
    std::vector<unsigned> m_reachable_conjunctions;
    virtual bool evaluate(const std::vector<unsigned> &conjunction_ids) override;
    virtual void refine(const PartialState &state) override;
public:
    using NoGoodFormula::NoGoodFormula;
    virtual void print_statistics() const override;
};

}
}

#endif
