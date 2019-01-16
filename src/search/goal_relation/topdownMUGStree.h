#ifndef TOP_DOWN_MUGS_TREE_H
#define TOP_DOWN_MUGS_TREE_H

#include <cassert>
#include <memory>
#include <utility>
#include <vector>
#include "../task_proxy.h"
#include "../abstract_task.h"
#include "mugsTree.h"

#include <deque>

class State;

namespace tdmugs {
class TopDownMUGSNode;


class TopDownMUGSTree : public mugstree::MUGSTree {

public:
    TopDownMUGSTree(GoalsProxy goals, bool all_soft_goals);

    virtual void expand(bool solved) override;
    //virtual void print() override;
    virtual int print_relation() override;
};


class TopDownMUGSNode : public mugstree::MUGSNode {

public:
    TopDownMUGSNode();
    ~TopDownMUGSNode();

    virtual int print_relation(const std::vector<FactPair>& all_goals) override;
    virtual std::vector<MUGSNode*> expand(std::vector<MUGSNode*>& nodes) override;


};
}

#endif
