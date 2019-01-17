#ifndef META_SEARCH_TREE_H
#define META_SEARCH_TREE_H

#include <cassert>
#include <memory>
#include <utility>
#include <vector>
#include "../task_proxy.h"
#include "../abstract_task.h"

#include <deque>

enum class MetaSearchType {
    TOPDOWNMUGSSEARCH,
    BOTTOMUPMUGSSEARCH,
    ENTAILMENTSEARCH
};

namespace mst {
class MetaSearchNode;


class MetaSearchTree {

public:

    virtual bool continue_search() = 0;
    virtual void expand(bool solved) = 0;
    virtual void next_node() = 0;
    virtual std::vector<FactPair> get_next_goals() = 0;
    virtual std::vector<FactPair> get_next_init() = 0;
    virtual void current_goals_solved() = 0;
    virtual void current_goals_not_solved() = 0;
    virtual void print() = 0;
};

}

#endif
