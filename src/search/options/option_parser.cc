#include "option_parser.h"

#include "doc_printer.h"
#include "errors.h"
#include "plugin.h"

#include "../plan_manager.h"
#include "../search_engine.h"

#include "../ext/tree_util.hh"

#include "../utils/rng.h"
#include "../utils/system.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace landmarks {
class LandmarkFactory;
}

namespace options {
const string OptionParser::NONE = "<none>";

static void ltrim(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch) {
                                   return !isspace(ch);
                               }));
}

static void rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(), [](int ch) {
                        return !isspace(ch);
                    }).base(), s.end());
}

static void trim(string &s) {
    ltrim(s);
    rtrim(s);
}

static pair<string, string> split_predefinition(const string &arg) {
    int split_pos = arg.find("=");
    string lhs = arg.substr(0, split_pos);
    trim(lhs);
    string rhs = arg.substr(split_pos + 1);
    trim(rhs);
    return make_pair(lhs, rhs);
}

/*
  Predefine landmarks and heuristics.
*/

static void predefine_evaluator(const string &arg, bool dry_run) {
    pair<string, string> predefinition = split_predefinition(arg);
    OptionParser parser(predefinition.second, dry_run);
    Predefinitions<Evaluator *>::instance()->predefine(
        predefinition.first, parser.start_parsing<Evaluator *>());
}

static void predefine_lmgraph(const string &arg, bool dry_run) {
    pair<string, string> predefinition = split_predefinition(arg);
    OptionParser parser(predefinition.second, dry_run);
    Predefinitions<shared_ptr<landmarks::LandmarkFactory>>::instance()->predefine(
        predefinition.first, parser.start_parsing<shared_ptr<landmarks::LandmarkFactory>>());
}


template<class T>
void _check_bounds(
    OptionParser &parser, const string &key, T value,
    T lower_bound, T upper_bound) {
    if (lower_bound > upper_bound)
        ABORT("lower bound is greater than upper bound for " + key);
    if (value < lower_bound || value > upper_bound) {
        ostringstream stream;
        stream << key << " (" << value << ") must be in range ["
               << lower_bound << ", " << upper_bound << "]";
        parser.error(stream.str());
    }
}

template<>
void OptionParser::check_bounds<int>(
    const string &key, const int &value, const Bounds &bounds) {
    int lower_bound = numeric_limits<int>::lowest();
    int upper_bound = numeric_limits<int>::max();
    if (!bounds.min.empty()) {
        OptionParser bound_parser(bounds.min, dry_run());
        lower_bound = TokenParser<int>::parse(bound_parser);
    }
    if (!bounds.max.empty()) {
        OptionParser bound_parser(bounds.max, dry_run());
        upper_bound = TokenParser<int>::parse(bound_parser);
    }
    _check_bounds(*this, key, value, lower_bound, upper_bound);
}

template<>
void OptionParser::check_bounds<double>(
    const string &key, const double &value, const Bounds &bounds) {
    double lower_bound = -numeric_limits<double>::infinity();
    double upper_bound = numeric_limits<double>::infinity();
    if (!bounds.min.empty()) {
        OptionParser bound_parser(bounds.min, dry_run());
        lower_bound = TokenParser<double>::parse(bound_parser);
    }
    if (!bounds.max.empty()) {
        OptionParser bound_parser(bounds.max, dry_run());
        upper_bound = TokenParser<double>::parse(bound_parser);
    }
    _check_bounds(*this, key, value, lower_bound, upper_bound);
}


string sanitize_argument(string s) {
    // Convert newlines to spaces.
    replace(s.begin(), s.end(), '\n', ' ');
    // Convert string to lower case.
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}


shared_ptr<SearchEngine> OptionParser::parse_cmd_line(
    int argc, const char **argv, bool dry_run, bool is_unit_cost) {
    vector<string> args;
    bool active = true;
    for (int i = 1; i < argc; ++i) {
        string arg = sanitize_argument(argv[i]);

        if (arg == "--if-unit-cost") {
            active = is_unit_cost;
        } else if (arg == "--if-non-unit-cost") {
            active = !is_unit_cost;
        } else if (arg == "--always") {
            active = true;
        } else if (active) {
            // We use the unsanitized arguments because sanitizing is inappropriate for things like filenames.
            args.push_back(argv[i]);
        }
    }
    return parse_cmd_line_aux(args, dry_run);
}


int OptionParser::parse_int_arg(const string &name, const string &value) {
    try {
        return stoi(value);
    } catch (invalid_argument &) {
        throw ArgError("argument for " + name + " must be an integer");
    } catch (out_of_range &) {
        throw ArgError("argument for " + name + " is out of range");
    }
}


shared_ptr<SearchEngine> OptionParser::parse_cmd_line_aux(
    const vector<string> &args, bool dry_run) {
    string plan_filename = "sas_plan";
    int num_previously_generated_plans = 0;
    bool is_part_of_anytime_portfolio = false;

    shared_ptr<SearchEngine> engine;
    /*
      Note that we don???t sanitize all arguments beforehand because filenames should remain as-is
      (no conversion to lower-case, no conversion of newlines to spaces).
    */
    // TODO: Remove code duplication.
    for (size_t i = 0; i < args.size(); ++i) {
        string arg = sanitize_argument(args[i]);
        bool is_last = (i == args.size() - 1);
        if (arg == "--heuristic") {
            if (is_last)
                throw ArgError("missing argument after --heuristic");
            ++i;
            predefine_evaluator(sanitize_argument(args[i]), dry_run);
        } else if (arg == "--landmarks") {
            if (is_last)
                throw ArgError("missing argument after --landmarks");
            ++i;
            predefine_lmgraph(sanitize_argument(args[i]), dry_run);
        } else if (arg == "--search") {
            if (is_last)
                throw ArgError("missing argument after --search");
            ++i;
            OptionParser parser(sanitize_argument(args[i]), dry_run);
            engine = parser.start_parsing<shared_ptr<SearchEngine>>();
        } else if (arg == "--help" && dry_run) {
            cout << "Help:" << endl;
            bool txt2tags = false;
            vector<string> plugin_names;
            for (size_t j = i + 1; j < args.size(); ++j) {
                string help_arg = sanitize_argument(args[j]);
                if (help_arg == "--txt2tags") {
                    txt2tags = true;
                } else {
                    plugin_names.push_back(help_arg);
                }
            }
            unique_ptr<DocPrinter> doc_printer;
            if (txt2tags)
                doc_printer = utils::make_unique_ptr<Txt2TagsPrinter>(cout);
            else
                doc_printer = utils::make_unique_ptr<PlainPrinter>(cout);
            if (plugin_names.empty()) {
                doc_printer->print_all();
            } else {
                for (const string &name : plugin_names) {
                    doc_printer->print_plugin(name);
                }
            }
            cout << "Help output finished." << endl;
            exit(0);
        } else if (arg == "--internal-plan-file") {
            if (is_last)
                throw ArgError("missing argument after --internal-plan-file");
            ++i;
            plan_filename = args[i];
        } else if (arg == "--internal-previous-portfolio-plans") {
            if (is_last)
                throw ArgError("missing argument after --internal-previous-portfolio-plans");
            ++i;
            is_part_of_anytime_portfolio = true;
            num_previously_generated_plans = parse_int_arg(arg, args[i]);
            if (num_previously_generated_plans < 0)
                throw ArgError("argument for --internal-previous-portfolio-plans must be positive");
        } else {
            throw ArgError("unknown option " + arg);
        }
    }

    if (engine) {
        PlanManager &plan_manager = engine->get_plan_manager();
        plan_manager.set_plan_filename(plan_filename);
        plan_manager.set_num_previously_generated_plans(num_previously_generated_plans);
        plan_manager.set_is_part_of_anytime_portfolio(is_part_of_anytime_portfolio);
    }
    return engine;
}

string OptionParser::usage(const string &progname) {
    return "usage: \n" +
           progname + " [OPTIONS] --search SEARCH < OUTPUT\n\n"
           "* SEARCH (SearchEngine): configuration of the search algorithm\n"
           "* OUTPUT (filename): translator output\n\n"
           "Options:\n"
           "--help [NAME]\n"
           "    Prints help for all heuristics, open lists, etc. called NAME.\n"
           "    Without parameter: prints help for everything available\n"
           "--landmarks LANDMARKS_PREDEFINITION\n"
           "    Predefines a set of landmarks that can afterwards be referenced\n"
           "    by the name that is specified in the definition.\n"
           "--heuristic HEURISTIC_PREDEFINITION\n"
           "    Predefines a heuristic that can afterwards be referenced\n"
           "    by the name that is specified in the definition.\n"
           "--internal-plan-file FILENAME\n"
           "    Plan will be output to a file called FILENAME\n\n"
           "--internal-previous-portfolio-plans COUNTER\n"
           "    This planner call is part of a portfolio which already created\n"
           "    plan files FILENAME.1 up to FILENAME.COUNTER.\n"
           "    Start enumerating plan files with COUNTER+1, i.e. FILENAME.COUNTER+1\n\n"
           "See http://www.fast-downward.org/ for details.";
}


static ParseTree generate_parse_tree(const string &config) {
    ParseTree tree;
    ParseTree::sibling_iterator pseudoroot =
        tree.insert(tree.begin(), ParseNode("pseudoroot", ""));
    ParseTree::sibling_iterator cur_node = pseudoroot;
    string buffer;
    string key;
    for (size_t i = 0; i < config.size(); ++i) {
        char next = config.at(i);
        if ((next == '(' || next == ')' || next == ',') && !buffer.empty()) {
            tree.append_child(cur_node, ParseNode(buffer, key));
            buffer.clear();
            key.clear();
        } else if (next == '(' && buffer.empty()) {
            throw ParseError("misplaced opening bracket (", *cur_node, config.substr(0, i));
        }
        switch (next) {
        case ' ':
            break;
        case '(':
            cur_node = last_child(tree, cur_node);
            break;
        case ')':
            if (cur_node == pseudoroot)
                throw ParseError("missing (", *cur_node, config.substr(0, i));
            cur_node = tree.parent(cur_node);
            break;
        case '[':
            if (!buffer.empty())
                throw ParseError("misplaced opening bracket [", *cur_node, config.substr(0, i));
            tree.append_child(cur_node, ParseNode("list", key));
            key.clear();
            cur_node = last_child(tree, cur_node);
            break;
        case ']':
            if (!buffer.empty()) {
                tree.append_child(cur_node, ParseNode(buffer, key));
                buffer.clear();
                key.clear();
            }
            if (cur_node->value != "list") {
                throw ParseError("mismatched brackets", *cur_node, config.substr(0, i));
            }
            cur_node = tree.parent(cur_node);
            break;
        case ',':
            break;
        case '=':
            if (buffer.empty())
                throw ParseError("expected keyword before =", *cur_node, config.substr(0, i));
            key = buffer;
            buffer.clear();
            break;
        default:
            buffer.push_back(next);
            break;
        }
    }
    if (cur_node->value != "pseudoroot")
        throw ParseError("missing )", *cur_node);
    if (!buffer.empty())
        tree.append_child(cur_node, ParseNode(buffer, key));

    // The real parse tree is the first (and only) child of the pseudoroot placeholder.
    return subtree(tree, tree.begin(pseudoroot));
}


OptionParser::OptionParser(const ParseTree &parse_tree, bool dry_run, bool help_mode)
    : opts(help_mode),
      parse_tree(parse_tree),
      dry_run_(dry_run),
      help_mode_(help_mode),
      next_unparsed_argument(first_child_of_root(this->parse_tree)) {
}

OptionParser::OptionParser(const string &config, bool dry_run, bool help_mode)
    : OptionParser(generate_parse_tree(config), dry_run, help_mode) {
}

string OptionParser::get_unparsed_config() const {
    ostringstream stream;
    kptree::print_tree_bracketed<ParseNode>(parse_tree, stream);
    return stream.str();
}

void OptionParser::add_enum_option(
    const string &key,
    const vector<string> &names,
    const string &help,
    const string &default_value,
    const vector<string> &docs) {
    if (help_mode_) {
        string enum_descr = "{";
        for (size_t i = 0; i < names.size(); ++i) {
            enum_descr += names[i];
            if (i != names.size() - 1) {
                enum_descr += ", ";
            }
        }
        enum_descr += "}";

        ValueExplanations value_explanations;
        if (!docs.empty() && docs.size() != names.size()) {
            ABORT("Please provide documentation for all or none of the values of " + key);
        }
        for (size_t i = 0; i < docs.size(); ++i) {
            value_explanations.emplace_back(names[i], docs[i]);
        }

        DocStore::instance()->add_arg(
            get_root_value(), key, help, enum_descr, default_value,
            Bounds::unlimited(), value_explanations);
        return;
    }

    // Enum arguments can be given by name or by number.
    // First, parse the corresponding string like a normal argument ...
    add_option<string>(key, help, default_value);

    if (!opts.contains(key))
        return;

    string value = opts.get<string>(key);

    // ... then check if the parsed string can be treated as a number.
    istringstream stream(value);
    int choice;
    if (!(stream >> choice).fail()) {
        int max_choice = names.size();
        if (choice > max_choice) {
            error("invalid enum argument " + value + " for option " + key);
        }
        opts.set<int>(key, choice);
    } else {
        // ... otherwise map the string to its position in the enumeration vector.
        auto it = find_if(names.begin(), names.end(),
                          [&](const string &name) {
                              if (name.size() != value.size())
                                  return false;
                              for (size_t i = 0; i < value.size(); ++i) {
                                  // Ignore case.
                                  if (tolower(name[i]) != tolower(value[i]))
                                      return false;
                              }
                              return true;
                          });
        if (it == names.end()) {
            error("invalid enum argument " + value + " for option " + key);
        }
        opts.set<int>(key, it - names.begin());
    }
}

Options OptionParser::parse() {
    /* Check if there were any arguments with invalid keywords,
       or positional arguments after keyword arguments. */
    string last_key;
    for (auto tree_it = first_child_of_root(parse_tree);
         tree_it != end_of_roots_children(parse_tree);
         ++tree_it) {
        if (!tree_it->key.empty()) {
            if (find(valid_keys.begin(), valid_keys.end(), tree_it->key) == valid_keys.end()) {
                error("invalid keyword " + tree_it->key + " for " + get_root_value());
            }
        }
        if (tree_it->key.empty() && !last_key.empty()) {
            error("positional argument after keyword argument");
        }
        last_key = tree_it->key;
    }
    opts.set_unparsed_config(get_unparsed_config());
    return opts;
}

void OptionParser::error(const string &msg) const {
    throw ParseError(msg, parse_tree);
}

void OptionParser::document_values(
    const string &argument,
    const ValueExplanations &value_explanations) const {
    DocStore::instance()->add_value_explanations(
        get_root_value(), argument, value_explanations);
}

void OptionParser::document_synopsis(const string &name, const string &note) const {
    DocStore::instance()->set_synopsis(get_root_value(), name, note);
}

void OptionParser::document_property(const string &property, const string &note) const {
    DocStore::instance()->add_property(get_root_value(), property, note);
}

void OptionParser::document_language_support(
    const string &feature, const string &note) const {
    DocStore::instance()->add_feature(get_root_value(), feature, note);
}

void OptionParser::document_note(
    const string &name, const string &note, bool long_text) const {
    DocStore::instance()->add_note(get_root_value(), name, note, long_text);
}

void OptionParser::document_hide() const {
    DocStore::instance()->hide(get_root_value());
}

bool OptionParser::dry_run() const {
    return dry_run_;
}

bool OptionParser::help_mode() const {
    return help_mode_;
}

const ParseTree *OptionParser::get_parse_tree() {
    return &parse_tree;
}

const string &OptionParser::get_root_value() const {
    return parse_tree.begin()->value;
}
}
