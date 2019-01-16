#include "entailmentSearch.h"

#include "../task_proxy.h"
#include "../tasks/root_task.h"

#include "../utils/system.h"

#include "meta_search_tree.h"

using namespace std;
using namespace options;
using namespace mst;

namespace entailsearch {

EntailNode::EntailNode(FactPair entailment, bool basic)
    : entailment(entailment), basic(basic){

    if(basic){
        additional_goals.push_back(FactPair(entailment.var, 1));
    }
    else{
        additional_goals.push_back(FactPair(entailment.var, 1));
        additional_goals.push_back(FactPair(entailment.value, 0));
    }
}

EntailNode::~EntailNode(){

}

std::vector<FactPair> EntailNode::get_goals() const{
    return additional_goals;
}

void printList(vector<FactPair> list){
    
    for(FactPair l : list){
        
        cout << "var" << l.var << " = " << l.value << ", ";
    }
    cout << endl;
}

void EntailNode::print(){
    TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
    if(! basic){
        cout << "\tSolvable: " << isSolvable() << endl;
        cout << "\t" << taskproxy.get_variables()[entailment.var].get_fact(1).get_name() << " -> " << taskproxy.get_variables()[entailment.value].get_fact(1).get_name() << endl;
    }
    else{
        cout << "\tSolvable: " << isSolvable() << endl;
        cout << "\tBasic: " << taskproxy.get_variables()[entailment.var].get_fact(1).get_name() << endl;
    }
}


std::vector<EntailNode*> EntailNode::expand(){
    std::vector<EntailNode*> children;
    if(basic && isSolvable()){
        EntailNode* child = new EntailNode(entailment, false);
        this->set_child(child);


        /*
        cout << "Expand: " << endl;
        child->print();
        TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
        cout << "Current goal facts: " <<  child->get_goals().size() << endl;
        for(uint i = 0; i < child->get_goals().size(); i++){
            FactPair g = child->get_goals()[i];
            cout << taskproxy.get_variables()[g.var].get_fact(g.value).get_name();
            if(i < child->get_goals().size()-1){
                cout << "|";
            }

        }
        cout << endl;
        */

        children.push_back(child);
    }
    return children;
}


EntailmentSearch::EntailmentSearch(){
    TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
    for(uint i = 0; i < taskproxy.get_goals().size(); i++){
        hard_goal_list.push_back(taskproxy.get_goals()[i].get_pair());

    }

    for(uint i = 0; i < taskproxy.get_entailments().size(); i++){
        FactProxy fp = taskproxy.get_entailments()[i];
        entailment_list.push_back(fp.get_pair());
        EntailNode* new_node = new EntailNode(fp.get_pair(), true);
        root_nodes.push_back(new_node);
        open_list.push_back(new_node);
    }
}

void EntailmentSearch::expand(bool solved){
    if(solved){
        std::vector<EntailNode*> new_nodes = current_node->expand();
        open_list.insert(open_list.end(), new_nodes.begin(), new_nodes.end());
    }
}


std::vector<FactPair> EntailmentSearch::get_goals(const EntailNode* node) const
{
    std::vector<FactPair> current_goals;
    std::vector<FactPair> soft_goals = node->get_goals();
    current_goals.insert( current_goals.end(), hard_goal_list.begin(), hard_goal_list.end() );
    current_goals.insert( current_goals.end(), soft_goals.begin(), soft_goals.end() );
    return current_goals;
}

std::vector<FactPair> EntailmentSearch::get_next_goals()
{
    
    current_node = get_next_node();

    
    TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
    cout << "Current goal facts: " <<  get_goals(current_node).size() << endl;
    for(uint i = 0; i < get_goals(current_node).size(); i++){
            FactPair g = get_goals(current_node)[i];
            cout << taskproxy.get_variables()[g.var].get_fact(g.value).get_name();
            if(i < get_goals(current_node).size()-1){
                cout << "|";
            }

        }
    cout << endl;
    current_node->print();
    

    /*
    for(FactPair fp : get_goals(current_node)){
        cout << fp.var << ": " << fp.value << endl;
    }
    */
    return get_goals(current_node);
}

void EntailmentSearch::current_goals_solved(){
    current_node->solved();
}

void EntailmentSearch::current_goals_not_solved(){
    current_node->not_solved();
}

int EntailmentSearch::print_relation(){
    int num_entailments = 0;
    for(EntailNode* n : root_nodes){
        n->print();
        if(n->isSolvable()){
            if(! n->get_child()->isSolvable()){
                num_entailments++;
                      
            }   
             n->get_child()->print();
        }
        cout << "--------------------------------------" << endl;
    }
    return num_entailments;
}

void EntailmentSearch::print(){
    cout << "*********************************"  << endl;
    cout << "Number of entailments: " << entailment_list.size() << endl;
    cout << "Hard goals: " << endl;
    TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
    for(FactPair g : hard_goal_list){
        cout << taskproxy.get_variables()[g.var].get_fact(g.value).get_name() << endl;
    }
    cout << "Entailments: " << endl;
    for(FactPair g : entailment_list){
        cout << g.var << " -> " << g.value << endl;
    }
    cout << "*********************************"  << endl;
    int printed_nodes = print_relation();
    cout << "*********************************"  << endl;
    cout << "Number of true entailments: " << printed_nodes << endl;
    cout << "*********************************"  << endl;
}

}
