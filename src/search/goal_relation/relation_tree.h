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

    void expand(Node* node);
    Node* get_next_node();
    void print(TaskProxy proxy);
};


class Node {
private:
    std::vector<Node*> children;

    uint sleep_set_i;
    std::vector<FactPair> goals;

    bool solvable = false;
    bool printed = false;

public:
    Node(const std::vector<FactPair>& goals);
    Node(unsigned sleep_set_i, const std::vector<FactPair>& goals);
    ~Node();

    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    std::vector<FactPair> get_goals(){
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

    void print();
    void print_relation(TaskProxy proxy);
    std::vector<Node*> expand();

};
}

#endif
