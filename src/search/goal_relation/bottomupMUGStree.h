#ifndef BOOTOM_UP_MUGS_TREE_H
#define BOOTOM_UP_MUGS_TREE_H

#include <cassert>
#include <memory>
#include <utility>
#include <vector>
#include "../task_proxy.h"
#include "../abstract_task.h"
#include "mugsTree.h"

#include <deque>

class State;

namespace bumugs {
class BottomUpMUGSNode;


class BottomUpMUGSTree : public mugstree::MUGSTree {

public:
    BottomUpMUGSTree(GoalsProxy goals, bool all_soft_goals);

    virtual void expand(bool solved) override;
    //virtual void print() override;
    virtual int print_relation() override;
    
};


class BottomUpMUGSNode : public mugstree::MUGSNode {

public:
    BottomUpMUGSNode();
    virtual ~BottomUpMUGSNode();

    virtual int print_relation(const std::vector<FactPair>& all_goals) override;
    virtual std::vector<MUGSNode*> expand(std::vector<MUGSNode*>& nodes) override;
    virtual std::vector<FactPair> get_goals(const std::vector<FactPair>& all_goals) const override;


};
}

#endif
