#ifndef ENTAILMENT_SEARCH_H
#define ENTAILMENT_SEARCH_H

#include <cassert>
#include <memory>
#include <utility>
#include <vector>
#include "../task_proxy.h"
#include "../abstract_task.h"
#include "meta_search_tree.h"

#include <deque>

namespace entailsearch {
class EntailNode;


class EntailmentSearch : public mst::MetaSearchTree {

protected:
    std::deque<EntailNode*> open_list;

    std::vector<EntailNode*> root_nodes;
    EntailNode* current_node;

    std::vector<std::vector<FactProxy>> entailment_list;
    std::vector<FactPair> hard_goal_list;

public:

    EntailmentSearch();

    bool continue_search() override{
    	std::cout << "Continue Search size openlist: "  << open_list.size() << std::endl;
        return ! open_list.empty();
    }

    EntailNode* get_next_node(){
        EntailNode* next_node = open_list.front();
        open_list.pop_front();
        return next_node;
    }

    std::vector<FactPair> getHardGoals() {
        return hard_goal_list;
    }

    virtual void expand(bool solved) override;
    std::vector<FactPair> get_goals(const EntailNode* node) const;
    virtual void next_node() override;
    std::vector<FactPair> get_next_goals() override;
    std::vector<FactPair> get_next_init() override;
    virtual void current_goals_solved() override;
    virtual void current_goals_not_solved() override;
    virtual int print_relation();
    virtual void print() override;
     
};


class EntailNode {
protected:
    std::vector<EntailNode*> children;
    std::vector<FactPair> additional_goals;
    FactPair premise = FactPair(-1, -1);
    std::vector<FactPair> entailments;
    bool solvable = false;
    bool printed = false;
    bool basic = false;


public:
    EntailNode(std::vector<FactPair> entailments);
    EntailNode(FactPair premise, FactPair conclusion);
    ~EntailNode();

    void solved(){
        solvable = true;
    }

    void not_solved(){
        solvable = false;
    }

    bool isSolvable(){
        return solvable;
    }

    void add_child(EntailNode* node){
        children.push_back(node);
    }

    std::vector<EntailNode*> get_children(){
        return children;
    }

    void resetPrinted(){
        printed = false;
    }

    virtual std::vector<FactPair>  get_goals() const;
    virtual std::vector<FactPair>  get_init() const;
    virtual void print();
    virtual std::vector<EntailNode*> expand();


};
}

#endif
