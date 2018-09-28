#ifndef HEURISTIC_REFINER_H
#define HEURISTIC_REFINER_H

#include "state_component.h"
#include "../evaluator.h"
#include "../utils/timer.h"

#include <memory>

namespace conflict_driven_learning {

class HeuristicRefiner
{
private:
    bool m_initialized;
    utils::Timer m_refinement_timer;
protected:
    virtual bool refine_heuristic(int bound, StateComponent &, const std::vector<std::pair<int, GlobalState> >&) = 0;
    virtual void initialize();
public:
    HeuristicRefiner();
    virtual ~HeuristicRefiner() = default;

    virtual std::shared_ptr<Evaluator> get_underlying_heuristic() = 0;

    virtual void print_statistics() const;

    bool notify(int bound,
                StateComponent &component,
                const std::vector<std::pair<int, GlobalState> >& successors);

    //bool notify(int bound,
    //            StateComponent &&component,
    //            std::vector<std::pair<int, GlobalState> >& successors);

    const utils::Timer &get_refinement_timer() const;
};

}

#endif
