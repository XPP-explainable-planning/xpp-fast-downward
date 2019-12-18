#include "mugs_uc_refiner.h"

#include "../abstract_task.h"
#include "../global_state.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../tasks/root_task.h"
#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "mugs_utils.h"
#include "state_component.h"
#include "strips_compilation.h"

#include <algorithm>
#include <unordered_set>

#define _DEBUG_PRINTS 0

namespace conflict_driven_learning {
namespace mugs {

#if _DEBUG_PRINTS
static void
print_state(const GlobalState& state)
{
    for (int var = 0; var < tasks::g_root_task->get_num_variables(); var++) {
        std::cout << (var > 0 ? "|" : "")
                  << tasks::g_root_task->get_fact_name(
                         FactPair(var, state[var]));
    }
}
#endif

#ifndef NDEBUG
static bool
check_subgoal_reachability(
    const subgoal_t& sg,
    MugsCriticalPathHeuristic* mugs_hc)
{
    hc_heuristic::HCHeuristic* hc = mugs_hc->get_underlying_heuristic();
    std::vector<std::pair<int, int>> facts;
    mugs_hc->get_goal_facts(sg, facts);
    std::vector<unsigned> fact_ids;
    strips::get_fact_ids(fact_ids, facts);
    assert(std::is_sorted(fact_ids.begin(), fact_ids.end()));
    std::vector<unsigned> conjs;
    hc->get_satisfied_conjunctions(fact_ids, conjs);
    for (const unsigned& c : conjs) {
        if (!hc->get_conjunction_data(c).achieved()) {
            return false;
        }
    }
    return true;
}
#endif

MugsUCRefiner::MugsUCRefiner(const options::Options& opts)
    : mugs_hc_(dynamic_cast<MugsCriticalPathHeuristic*>(
        opts.get<Evaluator*>("mugs_hc")))
    , hc_refiner_(opts.get<std::shared_ptr<ConflictLearner>>("hc_refiner"))
{
}

void
MugsUCRefiner::add_options_to_parser(options::OptionParser& parser)
{
    parser.add_option<Evaluator*>("mugs_hc");
    parser.add_option<std::shared_ptr<ConflictLearner>>("hc_refiner");
}

Evaluator*
MugsUCRefiner::get_underlying_heuristic()
{
    return mugs_hc_;
}

bool
MugsUCRefiner::requires_recognized_neighbors() const
{
    return hc_refiner_->requires_recognized_neighbors();
}

void
MugsUCRefiner::print_statistics() const
{
    hc_refiner_->print_statistics();
}

void
MugsUCRefiner::initialize()
{
}

bool
MugsUCRefiner::learn_from_dead_end_component(
    StateComponent& states,
    StateComponent& neighbors)
{
    std::unordered_set<subgoal_t> mugs = mugs_hc_->get_mugs();
    assert(!mugs.empty());

#if _DEBUG_PRINTS
    std::cout << "++++ REFINE MUGS HC ++++" << std::endl;

    std::cout << ">>>> STATE <<<<" << std::endl;
    const GlobalState& state = states.current();
    print_state(state);
    std::cout << std::endl;

    std::cout << ">>>> MUGS <<<<" << std::endl;
    print_set(mugs.begin(), mugs.end(), mugs_hc_->get_goal_fact_names());
#endif

#ifndef NDEBUG
    if (requires_recognized_neighbors()) {
        while (!neighbors.end()) {
            const GlobalState& t = neighbors.current();
            EvaluationContext ctxt(t, 0, false, nullptr);
            assert(mugs_hc_->compute_result(ctxt).is_infinite());
            hc_heuristic::HCHeuristic* hc =
                mugs_hc_->get_underlying_heuristic();
            hc->evaluate(t, 0);
            for (auto it = mugs.begin(); it != mugs.end(); it++) {
                subgoal_t mug = mugs_hc_->get_hard_goal() | *it;
                bool mug_reachable = check_subgoal_reachability(mug, mugs_hc_);
#if _DEBUG_PRINTS
                if (mug_reachable) {
                    std::cout
                        << "ERROR: MUG should be detected unreachable in state!"
                        << std::endl;
                    std::cout << "state: ";
                    print_state(t);
                    std::cout << std::endl;
                    std::cout << "mug: ";
                    std::vector<subgoal_t> tmp({ mug });
                    print_set(
                        tmp.begin(),
                        tmp.end(),
                        mugs_hc_->get_goal_fact_names());
                }
#endif
                assert(!mug_reachable);
            }
            neighbors.next();
        }
        neighbors.reset();
    }
#endif

    bool res = true;
    for (auto it = mugs.begin(); res && it != mugs.end(); it++) {
        subgoal_t mug = mugs_hc_->get_hard_goal() | *it;

#if _DEBUG_PRINTS
        std::cout << "++++ " << to_string(mug) << " ++++" << std::endl;
#endif

        std::vector<std::pair<int, int>> facts;
        mugs_hc_->get_goal_facts(mug, facts);
        mugs_hc_->get_underlying_heuristic()->set_auxiliary_goal(
            std::move(facts));
        res = hc_refiner_->notify_dead_end_component(states, neighbors);
        states.reset();
        neighbors.reset();

#ifndef NDEBUG
        facts.clear();
        hc_heuristic::HCHeuristic* hc = mugs_hc_->get_underlying_heuristic();
        mugs_hc_->get_goal_facts(mugs_hc_->get_hard_goal() | *it, facts);
        std::vector<unsigned> fids;
        strips::get_fact_ids(fids, facts);
        std::vector<unsigned> cids;
        hc->get_satisfied_conjunctions(fids, cids);
        hc->evaluate(states.current(), 0);
        bool reached = true;
        for (const unsigned& cid : cids) {
            if (!hc->get_conjunction_data(cid).achieved()) {
                reached = false;
                break;
            }
        }
        assert(!reached);
#endif
    }

#if _DEBUG_PRINTS
    std::cout << "++++ RESULT: " << res << " ++++" << std::endl;
    mugs_hc_->get_underlying_heuristic()->print_statistics();
#endif

    mugs_hc_->sync();

    return res;
}

static std::shared_ptr<ConflictLearner>
_parse(options::OptionParser& p)
{
    MugsUCRefiner::add_options_to_parser(p);
    options::Options opts = p.parse();
    if (p.dry_run()) {
        return nullptr;
    }
    return std::make_shared<MugsUCRefiner>(opts);
}

static PluginShared<ConflictLearner> _plugin("mugs_uc_refiner", _parse);

} // namespace mugs
} // namespace conflict_driven_learning

#undef _DEBUG_PRINTS
