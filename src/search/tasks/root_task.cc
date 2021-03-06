#include "root_task.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state_registry.h"

#include "../utils/collections.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>
#include <iostream>
#include <string>


using namespace std;
using utils::ExitCode;

namespace tasks {
static const int PRE_FILE_VERSION = 3;
shared_ptr<AbstractTask> g_root_task = nullptr;
shared_ptr<AbstractTask> g_mod_task = nullptr;

struct ExplicitVariable {
    int domain_size;
    string name;
    vector<string> fact_names;
    int axiom_layer;
    int axiom_default_value;

    explicit ExplicitVariable(istream &in);
};


struct ExplicitEffect {
    FactPair fact;
    vector<FactPair> conditions;

    ExplicitEffect(int var, int value, vector<FactPair> &&conditions);
};


struct ExplicitOperator {
    vector<FactPair> preconditions;
    vector<ExplicitEffect> effects;
    int cost;
    string name;
    bool is_an_axiom;

    void read_pre_post(istream &in);
    ExplicitOperator(istream &in, bool is_an_axiom, bool use_metric);
};


class RootTask : public AbstractTask {
    vector<ExplicitVariable> variables;
    // TODO: think about using hash sets here.
    vector<vector<set<FactPair>>> mutexes;
    vector<ExplicitOperator> operators;
    vector<ExplicitOperator> axioms;
    vector<int> initial_state_values;
    vector<FactPair> goals;
    vector<FactPair> hard_goals;
    vector<FactPair> soft_goals;
    vector<FactPair> question;
    vector<Property> LTL_properties;
    vector<vector<FactPair>> entailments;

    const ExplicitVariable &get_variable(int var) const;
    const ExplicitEffect &get_effect(int op_id, int effect_id, bool is_axiom) const;
    const ExplicitOperator &get_operator_or_axiom(int index, bool is_axiom) const;

public:
    explicit RootTask(istream &in);

    virtual int get_num_variables() const override;
    virtual string get_variable_name(int var) const override;
    virtual int get_variable_domain_size(int var) const override;
    virtual int get_variable_axiom_layer(int var) const override;
    virtual int get_variable_default_axiom_value(int var) const override;
    virtual string get_fact_name(const FactPair &fact) const override;
    virtual bool are_facts_mutex(
        const FactPair &fact1, const FactPair &fact2) const override;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
    virtual string get_operator_name(
        int index, bool is_axiom) const override;
    virtual int get_num_operators() const override;
    virtual int get_num_operator_preconditions(
        int index, bool is_axiom) const override;
    virtual FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual int get_num_operator_effects(
        int op_index, bool is_axiom) const override;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual OperatorID get_global_operator_id(OperatorID id) const override;

    virtual int get_num_axioms() const override;

    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;

    virtual int get_num_hard_goals() const override;
    virtual FactPair get_hard_goal_fact(int index) const override;

    virtual int get_num_soft_goals() const override;
    virtual FactPair get_soft_goal_fact(int index) const override;

    virtual int get_num_question() const override;
    virtual FactPair get_question_fact(int index) const override;

    virtual int get_num_LTL_properties() const override;
    virtual Property get_LTL_property(int index) const override;

    virtual int get_num_entailments() const override;
    virtual vector<FactPair> get_entailment(int index) const override;

    virtual vector<int> get_initial_state_values() const override;
    virtual void convert_state_values(
        vector<int> &values,
        const AbstractTask *ancestor_task) const override;
};


static void check_fact(const FactPair &fact, const vector<ExplicitVariable> &variables) {
    if (!utils::in_bounds(fact.var, variables)) {
        cerr << "Invalid variable id: " << fact.var << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    if (fact.value < 0 || fact.value >= variables[fact.var].domain_size) {
        cerr << "Invalid value for variable " << fact.var << ": " << fact.value << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
}

static void check_facts(const vector<FactPair> &facts, const vector<ExplicitVariable> &variables) {
    for (FactPair fact : facts) {
        check_fact(fact, variables);
    }
}

static void check_facts(const ExplicitOperator &action, const vector<ExplicitVariable> &variables) {
    check_facts(action.preconditions, variables);
    for (const ExplicitEffect &eff : action.effects) {
        check_fact(eff.fact, variables);
        check_facts(eff.conditions, variables);
    }
}

void check_magic(istream &in, const string &magic) {
    string word;
    in >> word;
    if (word != magic) {
        cerr << "Failed to match magic word '" << magic << "'." << endl
             << "Got '" << word << "'." << endl;
        if (magic == "begin_version") {
            cerr << "Possible cause: you are running the planner "
                 << "on a translator output file from " << endl
                 << "an older version." << endl;
        }
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
}

vector<FactPair> read_facts(istream &in) {
    int count;
    in >> count;
    vector<FactPair> conditions;
    conditions.reserve(count);
    for (int i = 0; i < count; ++i) {
        FactPair condition = FactPair::no_fact;
        in >> condition.var >> condition.value;
        conditions.push_back(condition);
    }
    return conditions;
}

vector<string> read_lines(istream &in) {
    int count;
    in >> count;
    vector<string> lines;
    if(count == 0){
        return lines;
    }
    //cout << "num props: " << count << endl;
    lines.reserve(count);
    string line;
    getline(in, line);
    for (int i = 0; i < count; ++i) {
        getline(in, line);
//        cout << i << " " << line << endl;
        lines.push_back(line);
    }
    return lines;
}

ExplicitVariable::ExplicitVariable(istream &in) {
    check_magic(in, "begin_variable");
    in >> name;
    in >> axiom_layer;
    in >> domain_size;
    in >> ws;
    fact_names.resize(domain_size);
    for (int i = 0; i < domain_size; ++i)
        getline(in, fact_names[i]);
    check_magic(in, "end_variable");
}


ExplicitEffect::ExplicitEffect(
    int var, int value, vector<FactPair> &&conditions)
    : fact(var, value), conditions(move(conditions)) {
}


void ExplicitOperator::read_pre_post(istream &in) {
    vector<FactPair> conditions = read_facts(in);
    int var, value_pre, value_post;
    in >> var >> value_pre >> value_post;
    if (value_pre != -1) {
        preconditions.emplace_back(var, value_pre);
    }
    effects.emplace_back(var, value_post, move(conditions));
}

ExplicitOperator::ExplicitOperator(istream &in, bool is_an_axiom, bool use_metric)
    : is_an_axiom(is_an_axiom) {
    if (!is_an_axiom) {
        check_magic(in, "begin_operator");
        in >> ws;
        getline(in, name);
        preconditions = read_facts(in);
        int count;
        in >> count;
        effects.reserve(count);
        for (int i = 0; i < count; ++i) {
            read_pre_post(in);
        }

        int op_cost;
        in >> op_cost;
        cost = use_metric ? op_cost : 1;
        check_magic(in, "end_operator");
    } else {
        name = "<axiom>";
        cost = 0;
        check_magic(in, "begin_rule");
        read_pre_post(in);
        check_magic(in, "end_rule");
    }
    assert(cost >= 0);
}

void read_and_verify_version(istream &in) {
    int version;
    check_magic(in, "begin_version");
    in >> version;
    check_magic(in, "end_version");
    if (version != PRE_FILE_VERSION) {
        cerr << "Expected translator output file version " << PRE_FILE_VERSION
             << ", got " << version << "." << endl
             << "Exiting." << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
}

bool read_metric(istream &in) {
    bool use_metric;
    check_magic(in, "begin_metric");
    in >> use_metric;
    check_magic(in, "end_metric");
    return use_metric;
}

vector<ExplicitVariable> read_variables(istream &in) {
    int count;
    in >> count;
    vector<ExplicitVariable> variables;
    variables.reserve(count);
    for (int i = 0; i < count; ++i) {
        variables.emplace_back(in);
    }
    return variables;
}

vector<vector<set<FactPair>>> read_mutexes(istream &in, const vector<ExplicitVariable> &variables) {
    vector<vector<set<FactPair>>> inconsistent_facts(variables.size());
    for (size_t i = 0; i < variables.size(); ++i)
        inconsistent_facts[i].resize(variables[i].domain_size);

    int num_mutex_groups;
    in >> num_mutex_groups;

    /*
      NOTE: Mutex groups can overlap, in which case the same mutex
      should not be represented multiple times. The current
      representation takes care of that automatically by using sets.
      If we ever change this representation, this is something to be
      aware of.
    */
    for (int i = 0; i < num_mutex_groups; ++i) {
        check_magic(in, "begin_mutex_group");
        int num_facts;
        in >> num_facts;
        vector<FactPair> invariant_group;
        invariant_group.reserve(num_facts);
        for (int j = 0; j < num_facts; ++j) {
            int var;
            int value;
            in >> var >> value;
            invariant_group.emplace_back(var, value);
        }
        check_magic(in, "end_mutex_group");
        for (const FactPair &fact1 : invariant_group) {
            for (const FactPair &fact2 : invariant_group) {
                if (fact1.var != fact2.var) {
                    /* The "different variable" test makes sure we
                       don't mark a fact as mutex with itself
                       (important for correctness) and don't include
                       redundant mutexes (important to conserve
                       memory). Note that the translator (at least
                       with default settings) removes mutex groups
                       that contain *only* redundant mutexes, but it
                       can of course generate mutex groups which lead
                       to *some* redundant mutexes, where some but not
                       all facts talk about the same variable. */
                    inconsistent_facts[fact1.var][fact1.value].insert(fact2);
                }
            }
        }
    }
    return inconsistent_facts;
}

vector<FactPair> read_goal(istream &in) {
    check_magic(in, "begin_goal");
    vector<FactPair> goals = read_facts(in);
    check_magic(in, "end_goal");
    if (goals.empty()) {
        cerr << "Task has no goal condition!" << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    return goals;
}

vector<FactPair> read_hard_goal(istream &in) {
    check_magic(in, "begin_hard_goal");
    vector<FactPair> goals = read_facts(in);
    check_magic(in, "end_hard_goal");
    return goals;
}

vector<FactPair> read_soft_goal(istream &in) {
    check_magic(in, "begin_soft_goal");
    vector<FactPair> goals = read_facts(in);
    check_magic(in, "end_soft_goal");
    return goals;
}

vector<FactPair> read_question(istream &in) {
    check_magic(in, "begin_question");
    vector<FactPair> question = read_facts(in);
    check_magic(in, "end_question");
    /*if (question.empty()) {
        cerr << "Task has no question condition!" << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }*/
    return question;
}

vector<Property> read_LTL_properties(istream &in) {
    check_magic(in, "begin_ltlproperty");
    vector<string> ltlstrings = read_lines(in);
    check_magic(in, "end_ltlproperty");
    vector<Property> properties;
    if(ltlstrings.size() == 0){
        return  properties;
    }
    for(uint i = 0; i < ltlstrings.size() - 1; i += 2){
        properties.push_back(Property(ltlstrings[i], ltlstrings[i+1]));
    }
    cout << "test" << endl;
    return properties;
}

vector<vector<FactPair>> read_entailments(istream &in) {
    vector<vector<FactPair>> entailments;
    check_magic(in, "begin_entailments");
    int count;
    in >> count;
    for(int i = 0; i < count; i++){
        check_magic(in, "begin_entail");
        vector<FactPair> entailment = read_facts(in);
        check_magic(in, "end_entail");
        entailments.push_back(entailment);
    }   
    check_magic(in, "end_entailments");
    
    return entailments;
}

vector<ExplicitOperator> read_actions(
    istream &in, bool is_axiom, bool use_metric,
    const vector<ExplicitVariable> &variables) {
    int count;
    in >> count;
    vector<ExplicitOperator> actions;
    actions.reserve(count);
    for (int i = 0; i < count; ++i) {
        actions.emplace_back(in, is_axiom, use_metric);
        check_facts(actions.back(), variables);
    }
    return actions;
}

RootTask::RootTask(std::istream &in) {
    read_and_verify_version(in);
    bool use_metric = read_metric(in);
    variables = read_variables(in);
    int num_variables = variables.size();

    mutexes = read_mutexes(in, variables);

    initial_state_values.resize(num_variables);
    check_magic(in, "begin_state");
    for (int i = 0; i < num_variables; ++i) {
        in >> initial_state_values[i];
    }
    check_magic(in, "end_state");

    for (int i = 0; i < num_variables; ++i) {
        variables[i].axiom_default_value = initial_state_values[i];
    }

    goals = read_goal(in);
    hard_goals = read_hard_goal(in);
    soft_goals = read_soft_goal(in);
    question = read_question(in);
    entailments = read_entailments(in);
    LTL_properties = read_LTL_properties(in);
    check_facts(goals, variables);
    operators = read_actions(in, false, use_metric, variables);
    axioms = read_actions(in, true, use_metric, variables);
    /* TODO: We should be stricter here and verify that we
       have reached the end of "in". */

    /*
      HACK: Accessing g_axiom_evaluators here creates a TaskProxy which assumes
      that this task is completely constructed.
    */
    AxiomEvaluator &axiom_evaluator = g_axiom_evaluators[this];
    axiom_evaluator.evaluate(initial_state_values);
}

const ExplicitVariable &RootTask::get_variable(int var) const {
    assert(utils::in_bounds(var, variables));
    return variables[var];
}

const ExplicitEffect &RootTask::get_effect(
    int op_id, int effect_id, bool is_axiom) const {
    const ExplicitOperator &op = get_operator_or_axiom(op_id, is_axiom);
    assert(utils::in_bounds(effect_id, op.effects));
    return op.effects[effect_id];
}

const ExplicitOperator &RootTask::get_operator_or_axiom(
    int index, bool is_axiom) const {
    if (is_axiom) {
        assert(utils::in_bounds(index, axioms));
        return axioms[index];
    } else {
        assert(utils::in_bounds(index, operators));
        return operators[index];
    }
}

int RootTask::get_num_variables() const {
    return variables.size();
}

string RootTask::get_variable_name(int var) const {
    return get_variable(var).name;
}

int RootTask::get_variable_domain_size(int var) const {
    return get_variable(var).domain_size;
}

int RootTask::get_variable_axiom_layer(int var) const {
    return get_variable(var).axiom_layer;
}

int RootTask::get_variable_default_axiom_value(int var) const {
    return get_variable(var).axiom_default_value;
}

string RootTask::get_fact_name(const FactPair &fact) const {
    assert(utils::in_bounds(fact.value, get_variable(fact.var).fact_names));
    return get_variable(fact.var).fact_names[fact.value];
}

bool RootTask::are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const {
    if (fact1.var == fact2.var) {
        // Same variable: mutex iff different value.
        return fact1.value != fact2.value;
    }
    assert(utils::in_bounds(fact1.var, mutexes));
    assert(utils::in_bounds(fact1.value, mutexes[fact1.var]));
    return bool(mutexes[fact1.var][fact1.value].count(fact2));
}

int RootTask::get_operator_cost(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).cost;
}

string RootTask::get_operator_name(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).name;
}

int RootTask::get_num_operators() const {
    return operators.size();
}

int RootTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).preconditions.size();
}

FactPair RootTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    const ExplicitOperator &op = get_operator_or_axiom(op_index, is_axiom);
    assert(utils::in_bounds(fact_index, op.preconditions));
    return op.preconditions[fact_index];
}

int RootTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).effects.size();
}

int RootTask::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    return get_effect(op_index, eff_index, is_axiom).conditions.size();
}

FactPair RootTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    const ExplicitEffect &effect = get_effect(op_index, eff_index, is_axiom);
    assert(utils::in_bounds(cond_index, effect.conditions));
    return effect.conditions[cond_index];
}

FactPair RootTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return get_effect(op_index, eff_index, is_axiom).fact;
}

OperatorID RootTask::get_global_operator_id(OperatorID id) const {
    return id;
}

int RootTask::get_num_axioms() const {
    return axioms.size();
}

int RootTask::get_num_goals() const {
    return goals.size();
}

FactPair RootTask::get_goal_fact(int index) const {
    assert(utils::in_bounds(index, goals));
    return goals[index];
}

int RootTask::get_num_hard_goals() const {
    return hard_goals.size();
}

FactPair RootTask::get_hard_goal_fact(int index) const {
    assert(utils::in_bounds(index, hard_goals));
    return hard_goals[index];
}


int RootTask::get_num_soft_goals() const {
    return soft_goals.size();
}

FactPair RootTask::get_soft_goal_fact(int index) const {
    assert(utils::in_bounds(index, soft_goals));
    return soft_goals[index];
}

int RootTask::get_num_question() const {
    return question.size();
}

FactPair RootTask::get_question_fact(int index) const {
    assert(utils::in_bounds(index, question));
    return question[index];
}

int RootTask::get_num_LTL_properties() const {
    return LTL_properties.size();
}

Property RootTask::get_LTL_property(int index) const {
    assert(utils::in_bounds(index, LTL_properties));
    return LTL_properties[index];
}

int RootTask::get_num_entailments() const {
    return entailments.size();
}

vector<FactPair> RootTask::get_entailment(int index) const {
    assert(utils::in_bounds(index, entailments));
    return entailments[index];
}


vector<int> RootTask::get_initial_state_values() const {
    return initial_state_values;
}

void RootTask::convert_state_values(
    vector<int> &, const AbstractTask *ancestor_task) const {
    if (this != ancestor_task) {
        ABORT("Invalid state conversion");
    }
}

void read_root_task(std::istream &in) {
    assert(!g_root_task);
    g_root_task = make_shared<RootTask>(in);
    g_mod_task = g_root_task;
}

static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    if (parser.dry_run())
        return nullptr;
    else
        return g_root_task;
}

static PluginShared<AbstractTask> _plugin("no_transform", _parse);
}
