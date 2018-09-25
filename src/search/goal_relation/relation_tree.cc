#include "relation_tree.h"

#include "../task_proxy.h"
#include "../tasks/root_task.h"

using namespace std;
using namespace options;

namespace goalre {
Node::Node(vector<FactPair> g)
    : goals(g){
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
        bool solved = isSolvable();
        cout << "Solved: " << solved << endl;
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
    vector<Node*> new_nodes;

    if(goals.size() == 1){
        return new_nodes;
    }

    /*
    cout << "Expand next node in relation tree" << endl;
    printList(goals);
    */

    //take g in A and put it into F, B stays unchanged
    for(uint i = 0; i < goals.size(); i++){

        vector<FactPair> new_goals;
        for(uint j = 0; j < goals.size(); j++){
            if(j != i)
                new_goals.push_back(goals[j]);
        }

        //sort lists
        std::sort(new_goals.begin(), new_goals.end());

         /*
        cout << "Expand next node in relation tree" << endl;
        printList(new_goals);
        */

        new_nodes.push_back(new Node(new_goals));
    }

    return new_nodes;
}

RelationTree::RelationTree(GoalsProxy goals){
    vector<FactPair> goal_list;
    for(uint i = 0; i < goals.size(); i++){
        goal_list.push_back(goals[i].get_pair());
    }
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
        for(Node* comp_node : nodes){
            if(new_node->get_goals() == comp_node->get_goals()){
                found = true;
                node->addChild(comp_node);
                break;
            }
        }
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
