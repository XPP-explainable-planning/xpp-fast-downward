#include "relation_tree.h"

#include "../task_proxy.h"
#include "../tasks/root_task.h"

#include "../utils/system.h"

using namespace std;
using namespace options;

namespace goalre {

Node::Node()
    : sleep_set_i(0), goals(0){
}

Node::~Node() {
}

std::vector<FactPair> Node::get_goals(const std::vector<FactPair>& all_goals) const
{
    std::vector<FactPair> res;
    for (uint i = 0; i < all_goals.size(); i++) {
        if ((goals >> i) & 1U) {
            res.push_back(all_goals[i]);
        }
    }
    return res;
}

void Node::print(const std::vector<FactPair>& all_goals){
        cout << ".........................." << endl;
        cout << "G:"  << endl;
        for(FactPair l : get_goals(all_goals)){
            cout << "var" << l.var << " = " << l.value << ", ";
        }
        cout << endl;
        cout << ".........................." << endl;
    }

void Node::print_relation(const std::vector<FactPair>& all_goals){
    if(printed){
        return;
    }
    printed = true;
    TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
    bool c_solved = true;
    for(Node* c : children){
        c_solved = c_solved && c->isSolvable(); //or or and ?
    }
    //if((c_solved || constrained_goals.size() == 0) && ! isSolvable()){
    if((c_solved) && ! isSolvable()){
        cout << ".........................." << endl;
        //bool solved = isSolvable();
        //cout << "Solved: " << solved << endl;
        cout << "G:"  << endl;
        for(FactPair l : get_goals(all_goals)){
            cout << "var" << l.var << " = " << l.value << ", ";
        }
        cout << endl;
        //cout << ".........................." << endl;
    }
    
    for(Node* c : children){
        c->print_relation(all_goals);
    }
    
}

void printList(vector<FactPair> list){
    for(FactPair l : list){
        cout << "var" << l.var << " = " << l.value << ", ";
    }
    cout << endl;
}

//generate all possible children nodes
std::vector<Node*> Node::expand(std::vector<Node>& nodes){
    vector<Node*> new_nodes;

    /*
    cout << "Expand next node in relation tree" << endl;
    printList(goals);
    */

    // cout << "Expand(" << goals << ", " << sleep_set_i << ")" << std::endl;

    //take g in A and put it into F, B stays unchanged
    // successors who have been expanded before
    for (uint i = 0; i < sleep_set_i; i++) {
        if ((goals >> i) & 1U) {
            uint new_goals = goals & ~(1U << i);
            Node* succ = &nodes[new_goals];
            if (succ->goals != new_goals) {
                succ->goals = new_goals;
                succ->solvable = true;
            }
            children.push_back(succ);
        }
    }
    // new successors
    for(uint i = sleep_set_i; ((goals >> i) & 1U); i++){
        uint new_goals = goals & ~(1U << i);
        new_nodes.push_back(&nodes[new_goals]);
        new_nodes.back()->sleep_set_i = i + 1;
        new_nodes.back()->goals = new_goals;
        children.push_back(new_nodes.back());

        // std::cout << goals << " -> " << new_goals << std::endl;

         /*
        // cout << "Expand next node in relation tree" << endl;
        cout << "Successor node in relation tree (i=" << i << "):" << endl << "  ";
        printList(new_goals);
        */
    }

    return new_nodes;
}

RelationTree::RelationTree(GoalsProxy goals){
    for(uint i = 0; i < goals.size(); i++){
        goal_list.push_back(goals[i].get_pair());
    }
    std::sort(goal_list.begin(), goal_list.end());
    if (goal_list.size() > 32) {
        std::cerr << "too many goal facts, aborting" << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    nodes.resize((1U << goal_list.size()));
    uint id = (1U << goal_list.size()) - 1;
    root = &nodes[id];
    root->set_goals(id);
    open_list.push_back(root);
}

void RelationTree::expand(Node* node){
    vector<Node*> new_nodes =node->expand(nodes);
    open_list.insert(open_list.begin(), new_nodes.begin(), new_nodes.end());
}

Node* RelationTree::get_next_node(){
    Node* next_node = open_list.front();
    open_list.pop_front();
    return next_node;
}

std::vector<FactPair> RelationTree::get_goals(const Node* node) const
{
    return node->get_goals(goal_list);
}

void RelationTree::print(){
    cout << "*********************************"  << endl;
    cout << "Size of tree: " << nodes.size() << endl;
    root->print_relation(goal_list);
    cout << "*********************************"  << endl;
}

}
