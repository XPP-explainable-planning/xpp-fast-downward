#ifndef RELATION_TREE_H
#define RELATION_TREE_H

#include <cassert>
#include <memory>
#include <utility>
#include <vector>
#include "../task_proxy.h"
#include "../abstract_task.h"

class State;

namespace goalre {
class Node;


class RelationTree {
    Node* root;
    std::vector<Node*> nodes;
    std::vector<Node*> solved_phase_1;
    std::vector<Node*> open_list;

public:
    RelationTree(GoalsProxy goals);

    // Visual Studio 2013 needs an explicit implementation.
    RelationTree(RelationTree &&other)
        : root(std::move(other.root)) {
    }

    Node* get_root() const {
        return root;
    }

    bool continue_search(){
        return ! open_list.empty();
    }

    void addSolvedNode(Node* n){
        solved_phase_1.push_back(n);
    }


    void expand(Node* node, int phase);
    void init_phase_2();
    Node* get_next_node();
    void print_ppd1();
    void print_ppd2();
};


class Node {
private:
    std::vector<Node*> children;

    std::vector<FactPair> constraining_goals;
    std::vector<FactPair> constrained_goals;
    std::vector<FactPair> remaining_goals;

    bool solvable = false;
    bool printed = false;

    bool oneParentSolvable = false;

public:
    Node(std::vector<FactPair> con_goals, std::vector<FactPair> const_goals, std::vector<FactPair> remain_goals);
    Node(std::vector<FactPair> goals);
    ~Node();

    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    const std::vector<FactPair> get_constraining_goals(){
        return constraining_goals;
    }

    const std::vector<FactPair> get_constrained_goals(){
        return constrained_goals;
    }

    const std::vector<FactPair> get_remaining_goals(){
        return remaining_goals;
    }

    bool hasChildren(){
        return children.size() > 0;
    }

    bool isSolvable(){
        return solvable;
    }

    void solved(){
        solvable = true;
    }

    void not_solved(){
        solvable = false;
    }

    void parentResult(bool r){
        //std::cout << "Parent solvable: " << oneParentSolvable << " new: " << r << std::endl;
        oneParentSolvable |= r;
    }

    bool isOneParentSolved(){
        return oneParentSolvable;
    }

    void addChild(Node* node){
        children.push_back(node);
    }

    void resetPrinted(){
        printed = false;
    }

    void print();
    void print_relation();

    std::vector<Node*> expand_remove_fact_from_A();
    std::vector<Node*> expand_remove_fact_from_B();
    std::vector<Node*> expand_fact_from_F_to_B();

    Node* copy_F_to_B();
};
}

#endif
