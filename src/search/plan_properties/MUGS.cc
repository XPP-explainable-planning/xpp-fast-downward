//
// Created by eifler on 27.03.20.
//

#include "MUGS.h"

#include "../tasks/root_task.h"

#include <fstream>
#include <iostream>
#include <bitset>

using namespace std;

MUGS::MUGS(unordered_set <uint> mugss, std::vector<std::string> goal_fact_names) {
    this->mugss = mugss;
    this->goal_fact_names = goal_fact_names;
}


void MUGS::generate_mugs_string() {
    int num_goal_facts = goal_fact_names.size();

    auto it = mugss.begin();
    while(it != mugss.end()) {
        uint gs = *it;

        // get names of all facts contained in the MUGS
        vector<string> fact_names;
        for (int i = 0; i < num_goal_facts; i++) {
            if (((1U << (num_goal_facts - 1 - i)) & gs) >= 1) {
                cout << goal_fact_names[i] << endl;
                fact_names.push_back(goal_fact_names[i]);
            }
        }
        this->mugs_facts_names.push_back(fact_names);
    }
}

void MUGS::generate_mugs_string_reverse(const subgoal_t& hide) {
    auto begin = mugss.begin();
    auto end = mugss.end();

    while (begin != end) {
        vector<string> fact_names;
        subgoal_t sg = *begin;
        for (unsigned i = 0; i < this->goal_fact_names.size(); i++) {
            if (((1U << i) & sg) && !(((1U << i) & hide))) {
                fact_names.push_back(this->goal_fact_names[i]);
            }
        }
        ++begin;
        this->mugs_facts_names.push_back(fact_names);
    }
}

void MUGS::output_mugs() {
    cout << "---------------- Print MUGS to FILE ----------------" << endl;
    ofstream outfile;
    outfile.open("mugs.json");

    outfile << "{\"MUGS\": [\n";
    auto it = mugs_facts_names.begin();
    while(it != mugs_facts_names.end()){
        vector<string> fact_names = *it;

        // print fact names
        outfile << "\t[";
        for(uint i = 0; i < fact_names.size(); i++){
            outfile << "\"" << fact_names[i] << "\"";
            if (i < fact_names.size() - 1){
                outfile << ", ";
            }
        }

        outfile << "]";
        if (next(it) != mugs_facts_names.end()){
            outfile << ",\n";
        }
        it++;
    }
    outfile << "\n]}";
    outfile.close();
}