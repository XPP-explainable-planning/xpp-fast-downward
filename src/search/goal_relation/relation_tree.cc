#include "relation_tree.h"

#include "../task_proxy.h"
#include "../tasks/root_task.h"

using namespace std;
using namespace options;

namespace goalre {
Node::Node(vector<FactPair> con_goals, vector<FactPair> const_goals, vector<FactPair> remain_goals)
    : constraining_goals(con_goals),
      constrained_goals(const_goals),
      remaining_goals(remain_goals) {
}

Node::Node(vector<FactPair> goals)
    : constraining_goals(goals){
}

Node::~Node() {
    for(Node* n : children){
        delete n;
    }
}

void Node::print(){
        cout << ".........................." << endl;
        cout << "A:"  << endl;
        for(FactPair l : constraining_goals){
            cout << "var" << l.var << " = " << l.value << ", ";
        }
        cout << endl;
        cout << "B:"  << endl;
        for(FactPair l : constrained_goals){
            cout << "var" << l.var << " = " << l.value << ", ";
        }
        cout << endl;
        cout << "F:"  << endl;
        for(FactPair l : remaining_goals){
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
    if((c_solved) && ! isSolvable() && ! oneParentSolvable){
        cout << ".........................." << endl;
        //bool solved = isSolvable();
        //cout << "Solved: " << solved << endl;
        cout << "A:"  << endl;
        for(FactPair l : constraining_goals){
            cout << taskproxy.get_variables()[l.var].get_fact(l.value).get_name() << "|";
        }
        cout << endl;
        cout << "B:"  << endl;
        for(FactPair l : constrained_goals){
            cout << taskproxy.get_variables()[l.var].get_fact(l.value).get_name() << "|";
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
std::vector<Node*> Node::expand_fact_from_F_to_B(){
    assert(remaining_goals.size() > 0);
    vector<Node*> new_nodes;

    /*
    cout << "Size A: " << constraining_goals.size() << endl;
    printList(constraining_goals);
    cout << "Size B: " << constrained_goals.size() << endl;
    printList(constrained_goals);
    cout << "Size F: " << remaining_goals.size() << endl;
    printList(remaining_goals);
    */
    
    //put one g from F int B, A stays unchanged
    for(uint i=0; i<remaining_goals.size(); i++){
        vector<FactPair> new_constraining_goals = constraining_goals;
        vector<FactPair> new_remaining_goals;
        for(uint j = 0; j < remaining_goals.size(); j++){
            if(j != i)
                new_remaining_goals.push_back(remaining_goals[j]);
        }

        vector<FactPair> new_constrained_goals = constrained_goals;
        new_constrained_goals.push_back(remaining_goals[i]);

        //sort lists
        std::sort(new_constraining_goals.begin(), new_constraining_goals.end());
        std::sort(new_constrained_goals.begin(), new_constrained_goals.end());
        std::sort(new_remaining_goals.begin(), new_remaining_goals.end());

        /*
        cout << "------- from F to B --------" << endl;
        cout << "Size A: " << new_constraining_goals.size() << endl;
        printList(new_constraining_goals);
        cout << "Size B: " << new_constrained_goals.size() << endl;
        printList(new_constrained_goals);
        cout << "Size F: " << new_remaining_goals.size() << endl;
        printList(new_remaining_goals);
        */
        new_nodes.push_back(new Node(new_constraining_goals, new_constrained_goals, new_remaining_goals));
    }
    
    return new_nodes;
}

std::vector<Node*> Node::expand_remove_fact_from_A(){
    vector<Node*> new_nodes;

    if(constraining_goals.size() == 0){
        return new_nodes;
    }

    /*
    cout << "Expand next node in relation tree" << endl;
    cout << "Size A: " << constraining_goals.size() << endl;
    printList(constraining_goals);
    cout << "Size B: " << constrained_goals.size() << endl;
    printList(constrained_goals);
    cout << "Size F: " << remaining_goals.size() << endl;
    printList(remaining_goals);
    */

    //take g in A and put it into F, B stays unchanged
    for(uint i = 0; i < constraining_goals.size(); i++){
        if(constraining_goals.size() == 1){
            break;
        }
        vector<FactPair> new_constraining_goals;
        for(uint j = 0; j < constraining_goals.size(); j++){
            if(j != i)
                new_constraining_goals.push_back(constraining_goals[j]);
        }
        vector<FactPair> new_remaining_goals = remaining_goals;
        vector<FactPair> new_constrained_goals = constrained_goals;
        //if(constrained_goals.size() == 0){
        //    new_constrained_goals.push_back(constraining_goals[i]);
        //}
        //else{
            new_remaining_goals.push_back(constraining_goals[i]);
        //}

        //sort lists
        std::sort(new_constraining_goals.begin(), new_constraining_goals.end());
        std::sort(new_constrained_goals.begin(), new_constrained_goals.end());
        std::sort(new_remaining_goals.begin(), new_remaining_goals.end());

        /*
        cout << "-------- remove from A -------" << endl;
        cout << "Size A: " << new_constraining_goals.size() << endl;
        printList(new_constraining_goals);
        cout << "Size B: " << new_constrained_goals.size() << endl;
        printList(new_constrained_goals);
        cout << "Size F: " << new_remaining_goals.size() << endl;
        printList(new_remaining_goals);
        */

        new_nodes.push_back(new Node(new_constraining_goals, new_constrained_goals, new_remaining_goals));
    }

    return new_nodes;
}

std::vector<Node*> Node::expand_remove_fact_from_B(){
    assert(constrained_goals.size() > 0);

    vector<Node*> new_nodes;

    /*
    cout << "Expand next node in relation tree" << endl;
    cout << "Size A: " << constraining_goals.size() << endl;
    printList(constraining_goals);
    cout << "Size B: " << constrained_goals.size() << endl;
    printList(constrained_goals);
    cout << "Size F: " << remaining_goals.size() << endl;
    printList(remaining_goals);
    */

    //take g in B and put it into F, A stays unchanged
    for(uint i = 0; i < constrained_goals.size(); i++){
       
        vector<FactPair> new_constrained_goals;
        for(uint j = 0; j < constrained_goals.size(); j++){
            if(j != i)
                new_constrained_goals.push_back(constrained_goals[j]);
        }
        vector<FactPair> new_remaining_goals = remaining_goals;
        vector<FactPair> new_constraining_goals = constraining_goals;
        
        new_remaining_goals.push_back(constrained_goals[i]);
        

        //sort lists
        std::sort(new_constraining_goals.begin(), new_constraining_goals.end());
        std::sort(new_constrained_goals.begin(), new_constrained_goals.end());
        std::sort(new_remaining_goals.begin(), new_remaining_goals.end());

        /*
        cout << "-------- remove from A -------" << endl;
        cout << "Size A: " << new_constraining_goals.size() << endl;
        printList(new_constraining_goals);
        cout << "Size B: " << new_constrained_goals.size() << endl;
        printList(new_constrained_goals);
        cout << "Size F: " << new_remaining_goals.size() << endl;
        printList(new_remaining_goals);
        */

        new_nodes.push_back(new Node(new_constraining_goals, new_constrained_goals, new_remaining_goals));
    }

    return new_nodes;
}

Node* Node::copy_F_to_B(){
    vector<FactPair> new_constraining_goals = constraining_goals;
    vector<FactPair> new_constrained_goals = remaining_goals;
    vector<FactPair> new_remaining_goals;
    
    return new Node(new_constraining_goals, new_constrained_goals, new_remaining_goals);
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

// TODO better implementation
bool myincludes(vector<FactPair> l1, vector<FactPair> l2){

    for (FactPair fp1 : l1){
        bool found = false;
        for(FactPair fp2 : l2){
            if(fp1 == fp2){
                found = true;
            }
        }
        if(!found){
            return false;
        }
    }   
    return true;
}

void RelationTree::init_phase_2(){
    cout << "--------------------------------------------- INIT PHASE 2 -------------------------"  << endl;

    for(Node* solved : solved_phase_1){
        cout << "Solved Node: " << endl;
        solved->resetPrinted();
        solved->print();
        //test inclusion
        /*
        bool superset_found = false;
        for(Node* comp : solved_phase_1){
            //cout << "check inclusion" << endl;
            //comp->print();
            if(myincludes(comp->get_constraining_goals(), solved->get_constraining_goals()) && 
                solved->get_constraining_goals().size() > comp->get_constraining_goals().size()){
                    cout << "inclusion found" << endl;
                    superset_found = true;
                    break;
            }
        }
        if(superset_found){
            continue;
        }
        */
        vector<Node*> new_nodes = solved->expand_fact_from_F_to_B();
        for(Node* new_node : new_nodes){
            solved->addChild(new_node);
            nodes.push_back(new_node);
            open_list.push_back(new_node);
        }
    }
}

void RelationTree::expand(Node* node, int phase){
    cout << "Expand phase: " << phase << endl;
    vector<Node*> new_nodes;
    if(phase == 1)
        new_nodes = node->expand_remove_fact_from_A();
    else
        new_nodes = node->expand_fact_from_F_to_B();

    //vector<Node*> no_duplicates;
    
    //check duplicates
    
    for(Node* new_node : new_nodes){
        bool found = false;
        //Node* found_node = NULL;
        for(Node* comp_node : nodes){
            if(new_node->get_constraining_goals() == comp_node->get_constraining_goals()
                && new_node->get_constrained_goals() == comp_node->get_constrained_goals()
            ){
                found = true;
                //found_node = comp_node;
                node->addChild(comp_node);
                break;
            }
        }
        if(! found){
            //no_duplicates.push_back(new_node);
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
    
    /*
    cout << "num new nodes: " << new_nodes.size() << endl;
    cout << "no dup ratio: " << (((float) no_duplicates.size()) / new_nodes.size()) << endl;
    
    for(Node* n : new_nodes)
        node->addChild(n);
    open_list.insert(open_list.end(), new_nodes.begin(), new_nodes.end() );
    nodes.insert(nodes.end(), new_nodes.begin(), new_nodes.end() );
    */
}

Node* RelationTree::get_next_node(){
    Node* next_node = open_list.back();
    open_list.erase(open_list.end()-1);
    return next_node;
}

void RelationTree::print_ppd1(){
    cout << "*************** PPD1 ******************"  << endl;
    cout << "Size of tree: " << nodes.size() << endl;
    root->print_relation();
}

void RelationTree::print_ppd2(){
    cout << "*************** PPD2 ******************"  << endl;
    cout << "Num solved phase 1: " << solved_phase_1.size() << endl;
    for(Node* n : solved_phase_1){
        n->print_relation();
    }
}
}
