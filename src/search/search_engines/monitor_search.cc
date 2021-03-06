#include "monitor_search.h"

#include "../evaluation_context.h"
#include "../evaluator.h"
#include "../globals.h"
#include "../open_list_factory.h"
#include "../option_parser.h"
#include "../pruning_method.h"
#include "../plugin.h"
#include "search_common.h"

#include "../algorithms/ordered_set.h"
#include "../task_utils/successor_generator.h"
#include "../heuristic.h"
#include "../evaluators/combining_evaluator.h"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <set>

using namespace std;

namespace monitor_search {
MonitorSearch::MonitorSearch(const Options &opts)
    : SearchEngine(opts),
      reopen_closed_nodes(opts.get<bool>("reopen_closed")),
      prune_by_f(opts.get<bool>("prune_by_f")),
      open_list(opts.get<shared_ptr<OpenListFactory>>("open")->
                create_state_open_list()),
      f_evaluator(opts.get<Evaluator *>("f_eval", nullptr)),
      preferred_operator_evaluators(opts.get_list<Evaluator *>("preferred")),
      lazy_evaluator(opts.get<Evaluator *>("lazy_evaluator", nullptr)),
      pruning_method(opts.get<shared_ptr<PruningMethod>>("pruning")) {
    if (lazy_evaluator && !lazy_evaluator->does_cache_estimates()) {
        cerr << "lazy_evaluator must cache its estimates" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }

}

void MonitorSearch::initialize() {
    /*
    cout << "Conducting best first search"
         << (reopen_closed_nodes ? " with" : " without")
         << " reopening closed nodes, (real) bound = " << bound
         << endl;
    */
    assert(open_list);

    
    cout << "**************** EAGER SEARCH Goals ****************"  << endl;
    for(uint i = 0; i < task_proxy.get_goals().size(); i++){
        FactProxy gp = task_proxy.get_goals()[i];
        cout << "var" << gp.get_variable().get_id() << " = " << gp.get_value() << endl;
    }
    cout << "Num operators: " << task_proxy.get_operators().size() << endl;
    cout << "**************** EAGER SEARCH Goals ****************" << endl;

    set<Evaluator *> evals;
    open_list->get_path_dependent_evaluators(evals);

    /*
      Collect path-dependent evaluators that are used for preferred operators
      (in case they are not also used in the open list).
    */
    for (Evaluator *evaluator : preferred_operator_evaluators) {
        evaluator->get_path_dependent_evaluators(evals);
    }

    /*
      Collect path-dependent evaluators that are used in the f_evaluator.
      They are usually also used in the open list and will hence already be
      included, but we want to be sure.
    */
    if (f_evaluator) {
        f_evaluator->get_path_dependent_evaluators(evals);
    }

    /*
      Collect path-dependent evaluators that are used in the lazy_evaluator
      (in case they are not already included).
    */
    if (lazy_evaluator) {
        lazy_evaluator->get_path_dependent_evaluators(evals);
    }

    path_dependent_evaluators.assign(evals.begin(), evals.end());

    const GlobalState &initial_state = state_registry.get_initial_state();
    //cout << "have initial state" << endl;
    for (Evaluator *evaluator : path_dependent_evaluators) {
        evaluator->notify_initial_state(initial_state);
    }

    /*
      Note: we consider the initial state as reached by a preferred
      operator.
    */
    EvaluationContext eval_context(initial_state, 0, true, &statistics);

    statistics.inc_evaluated_states();

    if (open_list->is_dead_end(eval_context)) {
        cout << "Initial state is a dead end." << endl;
    } else {
        if (search_progress.check_progress(eval_context))
            print_checkpoint_line(0);
        start_f_value_statistics(eval_context);
        SearchNode node = search_space.get_node(initial_state);
        node.open_initial();

        open_list->insert(eval_context, initial_state.get_id());
    }

    print_initial_evaluator_values(eval_context);

    pruning_method->initialize(task);
    pruning_method->prune_init_state(initial_state);
    //cout << "Init finished" << endl;
}

void MonitorSearch::print_checkpoint_line(int ) const { //g) const {
    /*
    cout << "[g=" << g << ", ";
    statistics.print_basic_statistics();
    cout << "]" << endl;
    */
}

void MonitorSearch::print_statistics() const {
    
    statistics.print_detailed_statistics();
    search_space.print_statistics();
    pruning_method->print_statistics();
    
}

SearchStatus MonitorSearch::step() {
    pair<SearchNode, bool> n = fetch_next_node();
    if (!n.second) {
        return FAILED;
    }
    SearchNode node = n.first;

    GlobalState s = node.get_state();
//    if (check_goal_and_set_plan(s)){
//        cout << "*************************** GOAL *****************" << endl;
//        //return SOLVED;
//    }
        

    /*
    cout << "State size: " << s.get_values().size() << endl;
    s.dump_fdr();
    cout << "-------------------------" << endl;
    */

    bool expand = true;
    /*
    if (prune_by_f && f_evaluator->does_cache_estimates()) {
        EvaluationContext eval_context(s, node.get_real_g(), false, &statistics, true);
        EvaluationResult res = f_evaluator->compute_result(eval_context);
        expand = res.get_evaluator_value() < bound;
    }*/

    if (expand) {
        vector<OperatorID> applicable_ops;
        g_successor_generator->generate_applicable_ops(s, applicable_ops);
    
        /*
          TODO: When preferred operators are in use, a preferred operator will be
          considered by the preferred operator queues even when it is pruned.
        */
        //pruning_method->prune_operators(s, applicable_ops);
    
        // This evaluates the expanded state (again) to get preferred ops
        EvaluationContext eval_context(s, node.get_g(), false, &statistics, true);
        ordered_set::OrderedSet<OperatorID> preferred_operators =
            collect_preferred_operators(eval_context, preferred_operator_evaluators);
    
        for (OperatorID op_id : applicable_ops) {
            OperatorProxy op = task_proxy.get_operators()[op_id];
            //if ((node.get_real_g() + op.get_cost()) >= bound)
            //    continue;

            //cout << op.get_name() << endl;
    
            GlobalState succ_state = state_registry.get_successor_state(s, op);

            bool new_automaton_state_reached;
            //cout << succ_state.get_id() << ": new automaton state reached: " << new_automaton_state_reached << endl;
            if(pruning_method->prune_state(s.get_id(), succ_state, &new_automaton_state_reached)){
                //cout << "*************************** PRUNE *****************" << endl;
                continue;
            }

            statistics.inc_generated();
            bool is_preferred = preferred_operators.contains(op_id);
    
            SearchNode succ_node = search_space.get_node(succ_state);
    
            for (Evaluator *evaluator : path_dependent_evaluators) {
                evaluator->notify_state_transition(s, op_id, succ_state);
            }
    
            // Previously encountered dead end. Don't re-evaluate.
            //if (succ_node.is_dead_end())
            //    continue;

            //cout << (! succ_node.is_new() & new_automaton_state_reached) << " " << (succ_node.get_g() > node.get_g() + get_adjusted_cost(op)) << endl;
            if (succ_node.is_new()) {
                //cout << "-----------------> New State" << endl;
                // We have not seen this state before.
                // Evaluate and create a new node.
    
                // Careful: succ_node.get_g() is not available here yet,
                // hence the stupid computation of succ_g.
                // TODO: Make this less fragile.
                int succ_g = node.get_g() + get_adjusted_cost(op);
    
                EvaluationContext eval_context(
                    succ_state, succ_g, is_preferred, &statistics);
                statistics.inc_evaluated_states();
    
                /*
                if (open_list->is_dead_end(eval_context)) {
                    succ_node.mark_as_dead_end();
                    statistics.inc_dead_ends();
                    continue;
                }*/
                
                succ_node.open(node, op);
    
                open_list->insert(eval_context, succ_state.get_id());
                if (search_progress.check_progress(eval_context)) {
                    print_checkpoint_line(succ_node.get_g());
                    reward_progress();
                }
            } else if (new_automaton_state_reached){ //(succ_node.get_g() > node.get_g() + get_adjusted_cost(op)) {
            //} else if (succ_node.get_g() > node.get_g() + get_adjusted_cost(op)) {
                // We found a new cheapest path to an open or closed state.
                if (new_automaton_state_reached) {
                    if (succ_node.is_closed()) {
                        /*
                          TODO: It would be nice if we had a way to test
                          that reopening is expected behaviour, i.e., exit
                          with an error when this is something where
                          reopening should not occur (e.g. A* with a
                          consistent heuristic).
                        */
                        statistics.inc_reopened();
                    }
                    succ_node.reopen(node, op);
    
                    EvaluationContext eval_context(
                        succ_state, succ_node.get_g(), is_preferred, &statistics);
    
                    /*
                      Note: our old code used to retrieve the h value from
                      the search node here. Our new code recomputes it as
                      necessary, thus avoiding the incredible ugliness of
                      the old "set_evaluator_value" approach, which also
                      did not generalize properly to settings with more
                      than one evaluator.
    
                      Reopening should not happen all that frequently, so
                      the performance impact of this is hopefully not that
                      large. In the medium term, we want the evaluators to
                      remember evaluator values for states themselves if
                      desired by the user, so that such recomputations
                      will just involve a look-up by the Evaluator object
                      rather than a recomputation of the evaluator value
                      from scratch.
                    */
                    open_list->insert(eval_context, succ_state.get_id());
                } else {
                    // If we do not reopen closed nodes, we just update the parent pointers.
                    // Note that this could cause an incompatibility between
                    // the g-value and the actual path that is traced back.
                    succ_node.update_parent(node, op);
                }
            }
        }
    }

    return IN_PROGRESS;
}

pair<SearchNode, bool> MonitorSearch::fetch_next_node() {
    /* TODO: The bulk of this code deals with multi-path dependence,
       which is a bit unfortunate since that is a special case that
       makes the common case look more complicated than it would need
       to be. We could refactor this by implementing multi-path
       dependence as a separate search algorithm that wraps the "usual"
       search algorithm and adds the extra processing in the desired
       places. I think this would lead to much cleaner code. */

    while (true) {
        if (open_list->empty()) {
            cout << "Completely explored state space -- no solution!" << endl;
            // HACK! HACK! we do this because SearchNode has no default/copy constructor
            const GlobalState &initial_state = state_registry.get_initial_state();
            SearchNode dummy_node = search_space.get_node(initial_state);
            return make_pair(dummy_node, false);
        }
        StateID id = open_list->remove_min();
        // TODO is there a way we can avoid creating the state here and then
        //      recreate it outside of this function with node.get_state()?
        //      One way would be to store GlobalState objects inside SearchNodes
        //      instead of StateIDs
        GlobalState s = state_registry.lookup_state(id);
        SearchNode node = search_space.get_node(s);

        //if (node.is_closed())
        //    continue;

        if (!lazy_evaluator)
            assert(!node.is_dead_end());

        node.close();
        //assert(!node.is_dead_end());
        update_f_value_statistics(node);
        statistics.inc_expanded();
        return make_pair(node, true);
    }
}

void MonitorSearch::reward_progress() {
    // Boost the "preferred operator" open lists somewhat whenever
    // one of the heuristics finds a state with a new best h value.
    open_list->boost_preferred();
}

void MonitorSearch::dump_search_space() const {
    search_space.dump(task_proxy);
}

void MonitorSearch::start_f_value_statistics(EvaluationContext &eval_context) {
    if (f_evaluator) {
        int f_value = eval_context.get_evaluator_value(f_evaluator);
        statistics.report_f_value_progress(f_value);
    }
}

/* TODO: HACK! This is very inefficient for simply looking up an h value.
   Also, if h values are not saved it would recompute h for each and every state. */
void MonitorSearch::update_f_value_statistics(const SearchNode &node) {
    if (f_evaluator) {
        /*
          TODO: This code doesn't fit the idea of supporting
          an arbitrary f evaluator.
        */
        EvaluationContext eval_context(node.get_state(), node.get_g(), false, &statistics);
        int f_value = eval_context.get_evaluator_value(f_evaluator);
        statistics.report_f_value_progress(f_value);
    }
}

}



