#include "state_component.h"

namespace conflict_driven_learning
{

    SingleStateComponent::SingleStateComponent(const GlobalState& state)
    : ended(false), state(state)
{}
const GlobalState &SingleStateComponent::current()
{
    return state;
}
void SingleStateComponent::next(unsigned)
{
    ended = true;
}
void SingleStateComponent::reset()
{
    ended = false;
}
bool SingleStateComponent::end()
{
    return ended;
}

}
