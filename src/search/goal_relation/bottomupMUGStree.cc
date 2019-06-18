#include "bottomupMUGStree.h"

#include "../task_proxy.h"
#include "../tasks/root_task.h"

#include "../utils/system.h"

#include "meta_search_tree.h"

using namespace std;
using namespace options;
using namespace mugstree;

namespace bumugs {

BottomUpMUGSNode::BottomUpMUGSNode(){}

 BottomUpMUGSNode::~BottomUpMUGSNode(){}

std::vector<FactPair> BottomUpMUGSNode::get_goals(const std::vector<FactPair>& all_goals) const {
    std::vector<FactPair> res;
    for (uint i = 0; i < all_goals.size(); i++) {
        if (! ((goals >> i) & 1U)) {    //0 -> use goal 1 -> you goal not start with solvable task and add goals until it is not solvable anymore
            res.push_back(all_goals[i]);
        }
    }
    return res;
}

void printList(vector<FactPair> list){
    for(FactPair l : list){
        cout << "var" << l.var << " = " << l.value << ", ";
    }
    cout << endl;
}


//generate all possible children nodes
std::vector<MUGSNode*> BottomUpMUGSNode::expand(std::vector<MUGSNode*>& nodes){
    vector<MUGSNode*> new_nodes;

    //printList(goals);
    

    //cout << "Expand(" << goals << ", " << sleep_set_i << ")" << std::endl;

    // successors who have been expanded before
    for (uint i = 0; i < sleep_set_i; i++) {
        if ((goals >> i) & 1U) {
            uint new_goals = goals & ~(1U << i);
            MUGSNode* succ = nodes[new_goals];
            if (succ->get_goals_id() != new_goals) {
                succ->set_goals_id(new_goals);
                succ->not_solved();
            }
            children.push_back(succ);
            //std::cout << goals << " -> " << new_goals << " exists already " << std::endl;
            
        }
    }

    // new successors
    for(uint i = sleep_set_i; ((goals >> i) & 1U); i++){
        uint new_goals = goals & ~(1U << i);
        MUGSNode* succ = nodes[new_goals];
        if(new_goals != ((1U << 31) -1)){ //TODO correct?
            new_nodes.push_back(succ);
            new_nodes.back()->set_sleep_set_id(i + 1);
            new_nodes.back()->set_goals_id(new_goals);
        }
        else{
            succ->set_goals_id(new_goals);
            succ->solved();
        }
        //cout << succ->goals << endl;
        children.push_back(succ);

        //std::cout << goals << " -> " << new_goals << std::endl;

        
        //cout << "Expand next node in relation tree" << endl;
        //cout << "Successor node in relation tree (i=" << i << "):" << endl << "  ";
        //printList(new_goals);
        
    }

    /*
    cout << "Number of children: " << children.size() << endl;
    for(uint i = 0; i < children.size(); i++){
        cout << "\t id " << children[i]->id() << " solvable" << children[i]->isSolvable() << endl;
    }
    cout << "Number of new nodes: " << new_nodes.size() << endl;
    for(uint i = 0; i < new_nodes.size(); i++){
        cout << "\t " << new_nodes[i]->id() << endl;
    }
    */
    return new_nodes;
}

int BottomUpMUGSNode::print_relation(const std::vector<FactPair>&){
    return 0;
}

BottomUpMUGSTree::BottomUpMUGSTree(GoalsProxy goals, bool all_soft_goals){
    TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
    for(uint i = 0; i < goals.size(); i++){
        //if(all_soft_goals || taskproxy.get_variables()[goals[i].get_pair().var].get_fact(goals[i].get_pair().value).get_name().find("soft") == 0){
        if (taskproxy.get_Question().size() == 0){
            for(uint i = 0; i < goals.size(); i++){
                if(all_soft_goals || taskproxy.get_variables()[goals[i].get_pair().var].get_fact(goals[i].get_pair().value).get_name().find("soft") == 0){
                    soft_goal_list.push_back(goals[i].get_pair());
                }
                else{
                    hard_goal_list.push_back(goals[i].get_pair());
                }
            }
        }
        else{
            QuestionProxy questproxy = taskproxy.get_Question();
            for(uint j = 0; j < questproxy.size(); j++){
                if(questproxy[j].get_value() == goals[i].get_value() && questproxy[j].get_variable().get_id() == goals[i].get_variable().get_id()){
                    hard_goal_list.push_back(goals[i].get_pair());
                }
                else{
                    soft_goal_list.push_back(goals[i].get_pair());
                }
            } 
        }       
    }
    std::sort(soft_goal_list.begin(), soft_goal_list.end());
    if (soft_goal_list.size() > 31) {
        std::cerr << "too many goal facts, aborting" << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    nodes.resize((1U << soft_goal_list.size()));
    for(uint i = 0; i < nodes.size(); i++){
        nodes[i] = new BottomUpMUGSNode();
    }
    uint id = (1U << soft_goal_list.size()) - 1;
    root = nodes[id];
    current_node = root;
    root->set_goals_id(id);
    open_list.push_back(root); 
}

void BottomUpMUGSTree::expand(bool solvable){
    if(solvable){

        vector<MUGSNode*> new_nodes = current_node->expand(nodes);
        //cout << "Expand: " << new_nodes.size() << endl;
        open_list.insert(open_list.begin(), new_nodes.begin(), new_nodes.end());
    }
}

int BottomUpMUGSTree::print_relation(){
    std::set<MUGSNode*> mugs;
    std::vector<MUGSNode*> openlist;

    open_list.push_back(root);

    while(open_list.size() > 0){
        //cout << "-----------------------------------------------" << endl;
        //cout << "Current mugs: " << mugs.size() << endl;
    	/*
        for(MUGSNode* n :mugs){
            n->print(soft_goal_list);
            cout << endl;
        }
        */

        MUGSNode* c_node = open_list[0];
        //cout << "Current node: " << " solvable: " << c_node->isSolvable() << endl;
        //c_node->print(soft_goal_list);
        //cout << endl;
        open_list.pop_front();

        if(! c_node->isSolvable()){
            bool is_superset = false;
            for(set<MUGSNode*>::iterator it = mugs.begin(); it != mugs.end(); it++){
                //cout << "Check insert" << endl;
                MUGSNode* mugs_n = *it;
                uint diff = mugs_n->get_goals_id() ^ c_node->get_goals_id();
                //c_node superset
                if((diff & mugs_n->get_goals_id()) > 0 && (diff & c_node->get_goals_id()) == 0){
                    is_superset = true;
                    break;
                }
                //c_node subset -> replace
                if((diff & mugs_n->get_goals_id()) == 0 && (diff & c_node->get_goals_id()) > 0){
                    it = mugs.erase(it);                  
                    break;
                }
            }
            if( ! is_superset){
                mugs.insert(c_node);
                //cout << "Add current node"  << endl;
            }
        }
        else{
            vector<MUGSNode*> children = c_node->getChildren();
            //cout << "Num added nodes " << children.size() << endl;
            open_list.insert(open_list.end(), children.begin(), children.end());
            //cout << "Length open list: " << open_list.size() << endl;
        }

    }
   

    cout << "MUGS" << endl;
    for(MUGSNode* n : mugs){
        n->print(soft_goal_list);
        cout << endl;
    }
    
    return mugs.size();
}


}
