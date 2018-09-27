#ifndef STATE_COMPONENT_H
#define STATE_COMPONENT_H

#include "../global_state.h"

namespace conflict_driven_learning
{

struct StateComponent {
    virtual const GlobalState &current() = 0;
    virtual void next(unsigned i = 1) = 0;
    virtual bool end() = 0;
    virtual void reset() = 0;
};

struct SingleStateComponent : public StateComponent {
private:
    bool ended;
    GlobalState state;
public:
    SingleStateComponent(const GlobalState& state);
    virtual const GlobalState &current() override;
    virtual void next(unsigned) override;
    virtual void reset() override;
    virtual bool end() override;
};

template<typename Iterator>
struct StateComponentIterator : public StateComponent {
private:
    Iterator m_begin;
    Iterator m_current;
    Iterator m_end;
public:
    StateComponentIterator(Iterator i, Iterator end)
        : m_begin(i), m_current(i), m_end(end)
    {}
    virtual const GlobalState &current() override
    {
        return *m_current;
    }
    virtual void next(unsigned i = 1) override
    {
        while (i-- > 0) {
            m_current++;
        }
    }
    virtual void reset() override
    {
        m_current = m_begin;
    }
    virtual bool end() override
    {
        return m_current == m_end;
    }
};

// namespace state_component
// {
// template<typename Iterator>
// StateComponent &&from_iterator(Iterator i, Iterator end)
// {
//     return StateComponentIterator<Iterator>(i, end);
// }
// }

}

#endif
