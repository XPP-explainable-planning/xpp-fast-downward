#ifndef RELATION_TREE_H
#define RELATION_TREE_H

#include <cassert>
#include <memory>
#include <utility>
#include <vector>
#include "../task_proxy.h"
#include "../abstract_task.h"

#include <deque>

class State;

namespace goalre {
class Node;


class RelationTree {
    Node* root;
    std::vector<FactPair> goal_list;
    std::vector<Node> nodes;
    std::deque<Node*> open_list;

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

    std::vector<FactPair> get_goals(const Node* node) const;
    void expand(Node* node);
    Node* get_next_node();
    void print();
};


class Node {
private:
    std::vector<Node*> children;
    uint sleep_set_i;
    uint goals;
    bool solvable = false;
    bool printed = false;
public:
    Node();
    ~Node();

    // Node(const Node &) = delete;
    // Node &operator=(const Node &) = delete;

    std::vector<FactPair>  get_goals(const std::vector<FactPair>& all_goals) const;

    void set_goals(uint goals){
        this->goals = goals;
    }

    uint id(){
        return goals;
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

    void addChild(Node* node){
        children.push_back(node);
    }

    void resetPrinted(){
        printed = false;
    }

    void print(const std::vector<FactPair>& all_goals);
    void print_relation(const std::vector<FactPair>& all_goals);
    std::vector<Node*> expand(std::vector<Node>& nodes);


};
}

#endif
