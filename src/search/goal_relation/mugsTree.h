#ifndef MUGS_TREE_H
#define MUGS_TREE_H

#include <cassert>
#include <memory>
#include <utility>
#include <vector>
#include "../task_proxy.h"
#include "../abstract_task.h"
#include "meta_search_tree.h"

#include <deque>

class State;

namespace mugstree {
class MUGSNode;


class MUGSTree : public mst::MetaSearchTree {

protected:
    // internal openlist of meta search
    std::deque<MUGSNode*> open_list;

    // root node of the meta search tree
    MUGSNode* root;
    MUGSNode* current_node;

    std::vector<FactPair> soft_goal_list;
    std::vector<FactPair> hard_goal_list;

    // all nodes of the meta search tree
    std::vector<MUGSNode*> nodes;

public:

    bool continue_search() override{
        return ! open_list.empty();
    }

    /**
     * removes the first node from the openlist and returns it
     * @return the first node in the openlist
     */
    MUGSNode* get_next_node(){
        MUGSNode* next_node = open_list.front();
        open_list.pop_front();
        return next_node;
    }

    MUGSNode* get_root() const {
        return root;
    }

    std::vector<FactPair> getSoftGoals() {
        return soft_goal_list;
    }

    std::vector<FactPair> getHardGoals() {
        return hard_goal_list;
    }

    /**
     * Get the goal facts which are in the node @node
     * @param node
     * @return
     */
    std::vector<FactPair> get_goals(const MUGSNode* node) const;

    /**
     * updates the current_node with the node returned by get_next_node
     */
    void next_node() override;

    /**
     * TODO
     * @return
     */
    std::vector<FactPair> get_next_goals() override;
    std::vector<FactPair> get_next_init() override;
    void current_goals_solved() override;
    void current_goals_not_solved() override;
    virtual int print_relation() = 0;
    void print() override;
     
};


class MUGSNode {
protected:
    std::vector<MUGSNode*> children;
    uint sleep_set_i = 0;
    uint goals = (1U << 31);
    bool solvable = false;
    bool printed = false;


public:

    void solved(){
        solvable = true;
    }

    void not_solved(){
        solvable = false;
    }

    bool isSolvable(){
        return solvable;
    }

    void addChild(MUGSNode* node){
        children.push_back(node);
    }

   
    void set_goals_id(uint goals){
        this->goals = goals;
    }

    uint get_goals_id(){
        return goals;
    }

    void set_sleep_set_id(uint id){
        this->sleep_set_i = id;
    }

    bool hasChildren(){
        return children.size() > 0;
    }

    std::vector<MUGSNode*> getChildren(){
        return children;
    }

    void resetPrinted(){
        printed = false;
    }

    virtual std::vector<FactPair>  get_goals(const std::vector<FactPair>& all_goals) const;
    virtual void print(const std::vector<FactPair>& all_goals);
    virtual int print_relation(const std::vector<FactPair>& all_goals) = 0;
    virtual std::vector<MUGSNode*> expand(std::vector<MUGSNode*>& nodes) = 0;


};
}

#endif
