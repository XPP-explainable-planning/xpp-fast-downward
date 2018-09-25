#include "relation_tree.h"

#include "../task_proxy.h"
#include "../tasks/root_task.h"

using namespace std;
using namespace options;

#define _GOALREL_SLEEP_SETS 0

namespace goalre {
Node::Node(const vector<FactPair>& g)
    : sleep_set_i(0), goals(g){
}

Node::Node(unsigned sleep_set_i, const vector<FactPair>& g)
    : sleep_set_i(sleep_set_i), goals(g){
}

Node::~Node() {
    for(Node* n : children){
        delete n;
    }
}

void Node::print(){
        cout << ".........................." << endl;
        cout << "G:"  << endl;
        for(FactPair l : goals){
            cout << "var" << l.var << " = " << l.value << ", ";
        }
        cout << endl;
        cout << ".........................." << endl;
    }

void Node::print_relation(){
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
        for(FactPair l : goals){
            cout << "var" << l.var << " = " << l.value << ", ";
        }
        cout << endl;
        //cout << ".........................." << endl;
    }
    
    for(Node* c : children){
        c->print_relation();
    }
    
}

void printList(vector<FactPair> list){
    for(FactPair l : list){
        cout << "var" << l.var << " = " << l.value << ", ";
    }
    cout << endl;
}

//generate all possible children nodes
std::vector<Node*> Node::expand(){
    /*
        cout << "Expanding node (sleep=" << sleep_set_i << ")" << endl << "  ";
        printList(goals);
        */

    vector<Node*> new_nodes;

    if(sleep_set_i >= goals.size() || goals.size() == 1){
        return new_nodes;
    }

    /*
    cout << "Expand next node in relation tree" << endl;
    printList(goals);
    */

    //take g in A and put it into F, B stays unchanged
    vector<FactPair> new_goals;
    for (uint i = 0; i < sleep_set_i; i++) {
        new_goals.push_back(goals[i]);
    }
    for(uint i = sleep_set_i; i < goals.size(); i++){
        while (new_goals.size() > sleep_set_i) new_goals.pop_back();

        for(uint j = sleep_set_i; j < goals.size(); j++){
            if(j != i)
                new_goals.push_back(goals[j]);
        }

        //sort lists
        //    should remain sorted, no need to call sort again
        // std::sort(new_goals.begin(), new_goals.end());

         /*
        // cout << "Expand next node in relation tree" << endl;
        cout << "Successor node in relation tree (i=" << i << "):" << endl << "  ";
        printList(new_goals);
        */

#if _GOALREL_SLEEP_SETS
        new_nodes.push_back(new Node(i, new_goals));
#else
        new_nodes.push_back(new Node(new_goals));
#endif
    }

    return new_nodes;
}

RelationTree::RelationTree(GoalsProxy goals){
    vector<FactPair> goal_list;
    for(uint i = 0; i < goals.size(); i++){
        goal_list.push_back(goals[i].get_pair());
    }
    std::sort(goal_list.begin(), goal_list.end());
    root = new Node(goal_list);
    open_list.push_back(root);
    nodes.push_back(root);
}

void RelationTree::expand(Node* node){
    vector<Node*> new_nodes =node->expand();
   
    //check duplicates    
    for(Node* new_node : new_nodes){
        //new_node->print();
        bool found = false;
        //Node* found_node = NULL;
#if _GOALREL_SLEEP_SETS
        // sleep set method guarantees that no goal subset is generated twice
#else
        for(Node* comp_node : nodes){
            if(new_node->get_goals() == comp_node->get_goals()){
                found = true;
                node->addChild(comp_node);
                break;
            }
        }
#endif
        if(! found){
            node->addChild(new_node);
            open_list.push_back(new_node);
            nodes.push_back(new_node);
            cout << "0 ";
        }
        else{
            delete new_node;
            cout << "1 ";
        }
    }
    cout << endl;
}

Node* RelationTree::get_next_node(){
    Node* next_node = open_list.back();
    open_list.erase(open_list.end()-1);
    return next_node;
}

void RelationTree::print(){
    cout << "*********************************"  << endl;
    cout << "Size of tree: " << nodes.size() << endl;
    root->print_relation();
    cout << "*********************************"  << endl;
}

}
