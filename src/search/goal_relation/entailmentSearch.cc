#include "entailmentSearch.h"

#include "../task_proxy.h"
#include "../tasks/root_task.h"

#include "../utils/system.h"

#include "meta_search_tree.h"

using namespace std;
using namespace options;
using namespace mst;

namespace entailsearch {

EntailNode::EntailNode(vector<FactPair> entail){
    basic = true;
    entailments.insert( entailments.end(), entail.begin()+1, entail.end() );
    premise = entail[0];
    additional_goals.push_back(FactPair(entail[0].var, 1));
}

EntailNode::EntailNode(FactPair premise, FactPair conclusion){
    basic = false;
    entailments.push_back(conclusion);
    this->premise = premise;
    additional_goals.push_back(FactPair(premise.var, 1));
    additional_goals.push_back(FactPair(conclusion.var, 0));
}

EntailNode::~EntailNode(){

}


std::vector<FactPair> EntailNode::get_goals() const{  
    return additional_goals;
}

std::vector<FactPair>  EntailNode::get_init() const{
    std::vector<FactPair> change_init;
    if(! basic){
        change_init.push_back(FactPair(entailments[0].var, 1));   
        //cout << "change init" << endl;
    }
    /*
    for(FactPair fp : change_init){
        cout << fp.var << ": " << fp.value << endl;
    }
    */

    return change_init;
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
        cout << "\t" << premise.var << ": " << taskproxy.get_variables()[premise.var].get_fact(1).get_name() << " -> " << entailments[0].var << ": " << taskproxy.get_variables()[entailments[0].var].get_fact(1).get_name() << endl;
    }
    else{
        cout << "\tSolvable: " << isSolvable() << endl;
        cout << "\tBasic: " << premise.var << ": " << taskproxy.get_variables()[premise.var].get_fact(1).get_name() << endl;
    }
}


std::vector<EntailNode*> EntailNode::expand(){
    std::vector<EntailNode*> children;
    if(basic && isSolvable()){
        for(FactPair fp : entailments){
             EntailNode* child = new EntailNode(premise, fp);
            this->add_child(child);

            children.push_back(child);
        }      
    }
    return children;
}


EntailmentSearch::EntailmentSearch(){
    TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
    for(uint i = 0; i < taskproxy.get_goals().size(); i++){
        hard_goal_list.push_back(taskproxy.get_goals()[i].get_pair());

    }

    for(uint i = 0; i < taskproxy.get_entailments().size(); i++){
        vector<FactProxy> fps = taskproxy.get_entailments()[i];
        
        entailment_list.push_back(fps);
        vector<FactPair> facts;
        for(FactProxy fp : fps){
            facts.push_back(fp.get_pair());
        }
        EntailNode* new_node = new EntailNode(facts);
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

void EntailmentSearch::next_node(){
     current_node = get_next_node();
}

std::vector<FactPair> EntailmentSearch::get_next_goals()
{
    /*
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
    */

    /*
    for(FactPair fp : get_goals(current_node)){
        cout << fp.var << ": " << fp.value << endl;
    }
    */
    return get_goals(current_node);
}

std::vector<FactPair> EntailmentSearch::get_next_init(){
    return current_node->get_init();
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
        //n->print();
        if(n->isSolvable()){
            for(EntailNode* en : n->get_children()){
                if(! en->isSolvable()){
                    num_entailments++;
                    en->print();     
                    cout << "--------------------------------------" << endl;               
                }  
            } 
             
        }
        
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
    //for(FactProxy g : entailment_list){
    //    cout << g. << " -> " << g.value << endl;
    //}
    cout << "*********************************"  << endl;
    int printed_nodes = print_relation();
    cout << "*********************************"  << endl;
    cout << "Number of true entailments: " << printed_nodes << endl;
    cout << "*********************************"  << endl;
}

}
