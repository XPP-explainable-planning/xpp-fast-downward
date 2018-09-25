#include "goal_relation_search.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../tasks/modified_goals_task.h"
#include "../tasks/root_task.h"
#include "../task_utils/successor_generator.h"
#include "../heuristic.h"

#include <iostream>

using namespace std;
using namespace goalre;

namespace goal_relation_search {
GoalRelationSearch::GoalRelationSearch(const Options &opts)
    : SearchEngine(opts),
      engine_configs(opts.get_list<ParseTree>("engine_configs")),
      repeat_last_phase(opts.get<bool>("repeat_last")),
      continue_on_fail(opts.get<bool>("continue_on_fail")),
      continue_on_solve(opts.get<bool>("continue_on_solve")),
      heuristic(opts.get<Evaluator*>("heu")),
      phase(0),
      algo_phase(1),
      last_phase_found_solution(false),
      best_bound(bound),
      iterated_found_solution(false),
      relation_tree(task_proxy.get_goals()) {

      //current_node = relation_tree.get_root();
      //cout << "Current Node: " << endl;
      //current_node->print(); 
      //tasks::g_root_task = make_shared<extra_tasks::ModifiedGoalsTask>(getTask(), current_node->get_goals());

}

shared_ptr<SearchEngine> GoalRelationSearch::get_search_engine(int engine_configs_index) {
    //adapt goals of current task according to the current goals relation node   
    current_node = relation_tree.get_next_node();
    cout << "Current Node: " << endl;
    current_node->print();
    tasks::g_root_task = make_shared<extra_tasks::ModifiedGoalsTask>(getTask(), current_node->get_goals());
    ((Heuristic*) heuristic)->set_abstract_task(tasks::g_root_task);
    //TODO find an other way
    //not necessary if only the goal is changed
    //g_successor_generator = new successor_generator::SuccessorGenerator(TaskProxy(*(tasks::g_root_task).get()));
    

    OptionParser parser(engine_configs[engine_configs_index], false);
    shared_ptr<SearchEngine> engine(parser.start_parsing<shared_ptr<SearchEngine>>());

    cout << "Starting search: ";
    kptree::print_tree_bracketed(engine_configs[engine_configs_index], cout);
    cout << endl;

    return engine;
}

shared_ptr<SearchEngine> GoalRelationSearch::create_phase(int phase) {
    return get_search_engine(phase);
}

SearchStatus GoalRelationSearch::step() {
    cout << "-------------------------------------------------------------------------------------------" << endl;
    shared_ptr<SearchEngine> current_search = create_phase(0);

    //TODO
    if (!current_search) {
        return found_solution() ? SOLVED : FAILED;
    }
    ++phase;

    current_search->search();

    //Plan found_plan;
    last_phase_found_solution = current_search->found_solution();

    //stop search in this branch
    if (last_phase_found_solution) {
        cout << "++++++++ SOLUTION +++++++"  << endl;
        iterated_found_solution = true;
        current_node->solved();
    }
    else{
        cout << "-------- FAILS ----------"  << endl;
        relation_tree.expand(current_node);
        current_node->not_solved();
    }

    //current_search->print_statistics();

    const SearchStatistics &current_stats = current_search->get_statistics();
    statistics.inc_expanded(current_stats.get_expanded());
    statistics.inc_evaluated_states(current_stats.get_evaluated_states());
    statistics.inc_evaluations(current_stats.get_evaluations());
    statistics.inc_generated(current_stats.get_generated());
    statistics.inc_generated_ops(current_stats.get_generated_ops());
    statistics.inc_reopened(current_stats.get_reopened());

    return step_return_value();
}

SearchStatus GoalRelationSearch::step_return_value() {
    if(relation_tree.continue_search()){
        return IN_PROGRESS;
    }
    else{
        relation_tree.print(task_proxy);
        return SOLVED;
    }

    return FAILED;
}

void GoalRelationSearch::print_statistics() const {
    cout << "Cumulative statistics:" << endl;
    statistics.print_detailed_statistics();
}

void GoalRelationSearch::save_plan_if_necessary() {
    // We don't need to save here, as we automatically save after
    // each successful search iteration.
}

static shared_ptr<SearchEngine> _parse(OptionParser &parser) {
    parser.document_synopsis("Iterated search", "");
    parser.document_note(
        "Note 1",
        "We don't cache heuristic values between search iterations at"
        " the moment. If you perform a LAMA-style iterative search,"
        " heuristic values will be computed multiple times.");
    parser.document_note(
        "Note 2",
        "The configuration\n```\n"
        "--search \"iterated([lazy_wastar(merge_and_shrink(),w=10), "
        "lazy_wastar(merge_and_shrink(),w=5), lazy_wastar(merge_and_shrink(),w=3), "
        "lazy_wastar(merge_and_shrink(),w=2), lazy_wastar(merge_and_shrink(),w=1)])\"\n"
        "```\nwould perform the preprocessing phase of the merge and shrink heuristic "
        "5 times (once before each iteration).\n\n"
        "To avoid this, use heuristic predefinition, which avoids duplicate "
        "preprocessing, as follows:\n```\n"
        "--heuristic \"h=merge_and_shrink()\" --search "
        "\"iterated([lazy_wastar(h,w=10), lazy_wastar(h,w=5), lazy_wastar(h,w=3), "
        "lazy_wastar(h,w=2), lazy_wastar(h,w=1)])\"\n"
        "```");
    parser.document_note(
        "Note 3",
        "If you reuse the same landmark count heuristic "
        "(using heuristic predefinition) between iterations, "
        "the path data (that is, landmark status for each visited state) "
        "will be saved between iterations.");
    parser.add_list_option<ParseTree>("engine_configs",
                                      "list of search engines for each phase");
    parser.add_option<bool>("repeat_last",
                            "repeat last phase of search",
                            "false");
    parser.add_option<bool>("continue_on_fail",
                            "continue search after no solution found",
                            "false");
    parser.add_option<bool>("continue_on_solve",
                            "continue search after solution found",
                            "true");
    parser.add_option<Evaluator*>("heu", "reference to heuristic to update abstract task");
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    opts.verify_list_non_empty<ParseTree>("engine_configs");

    if (parser.help_mode()) {
        return nullptr;
    } else if (parser.dry_run()) {
        //check if the supplied search engines can be parsed
        for (const ParseTree &config : opts.get_list<ParseTree>("engine_configs")) {
            OptionParser test_parser(config, true);
            test_parser.start_parsing<shared_ptr<SearchEngine>>();
        }
        return nullptr;
    } else {
        return make_shared<GoalRelationSearch>(opts);
    }
}

static PluginShared<SearchEngine> _plugin("goal_relation", _parse);
}
