//
// Created by eifler on 27.03.20.
//

#ifndef FAST_DOWNWARD_MUGS_H
#define FAST_DOWNWARD_MUGS_H

#include "../task_proxy.h"
#include <unordered_set>

using subgoal_t = uint32_t;

class MUGS {
    std::unordered_set<uint> mugss;
    std::vector<std::string> goal_fact_names;
    std::vector<std::vector<std::string>> mugs_facts_names;

public:
    explicit MUGS(std::unordered_set<uint> mugss, std::vector<std::string> goal_fact_names);
    void generate_mugs_string();
    void generate_mugs_string_reverse(const subgoal_t& hide);
    void output_mugs();
};


#endif //FAST_DOWNWARD_MUGS_H
