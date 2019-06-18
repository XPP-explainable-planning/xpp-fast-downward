#include "topdownMUGStree.h"

#include "../task_proxy.h"
#include "../tasks/root_task.h"

#include "../utils/system.h"

#include "meta_search_tree.h"

using namespace std;
using namespace options;
using namespace mugstree;

namespace tdmugs {

 TopDownMUGSNode::TopDownMUGSNode(){}

 TopDownMUGSNode::~TopDownMUGSNode(){}

int TopDownMUGSNode::print_relation(const std::vector<FactPair>& all_goals){
    if(printed){
        return 0;
    }
    printed = true;

    //Check if all children are solvable
    bool c_solved = true;
    for(MUGSNode* c : children){
        c_solved = c_solved && c->isSolvable(); //or or and ?
    }
    if(children.size() == 0)
        c_solved = true;

    int printed_nodes = 0;
    if((c_solved) && ! isSolvable()){
        printed_nodes++;
        this->print(all_goals);
        cout << endl;
    }
    else{   
        for(MUGSNode* c : children){
            printed_nodes += c->print_relation(all_goals);
        }
    }
    
    return printed_nodes;
}


//generate all possible children nodes
std::vector<MUGSNode*> TopDownMUGSNode::expand(std::vector<MUGSNode*>& nodes){
    vector<MUGSNode*> new_nodes;

    
    //cout << "Expand next node in relation tree" << endl;
    //printList(goals);
    

    //cout << "Expand(" << goals << ", " << sleep_set_i << ")" << std::endl;

    // successors who have been expanded before
    for (uint i = 0; i < sleep_set_i; i++) {
        if ((goals >> i) & 1U) {
            uint new_goals = goals & ~(1U << i);
            MUGSNode* succ = nodes[new_goals];
            if (succ->get_goals_id() != new_goals) {
                succ->set_goals_id(new_goals);
                succ->solved();
            }
            children.push_back(succ);
        }
    }

    // new successors
    for(uint i = sleep_set_i; ((goals >> i) & 1U); i++){
        uint new_goals = goals & ~(1U << i);
        MUGSNode* succ = nodes[new_goals];
        if(new_goals != 0){
            new_nodes.push_back(succ);
            new_nodes.back()->set_sleep_set_id(i + 1);
            new_nodes.back()->set_goals_id(new_goals);
        }
        else{
            succ->set_goals_id(new_goals);
            succ->solved();
        }
        children.push_back(succ);

        //std::cout << goals << " -> " << new_goals << std::endl;

        
        // cout << "Expand next node in relation tree" << endl;
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

TopDownMUGSTree::TopDownMUGSTree(GoalsProxy goals, bool all_soft_goals){
    TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
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
        for(uint i = 0; i < goals.size(); i++){
            QuestionProxy questproxy = taskproxy.get_Question();
            //cout << goals[i].get_value() << " " << goals[i].get_variable().get_id() << endl;
            bool found = false;
            for(uint j = 0; j < questproxy.size(); j++){
                if(questproxy[j].get_value() == goals[i].get_value() && questproxy[j].get_variable().get_id() == goals[i].get_variable().get_id()){
                    hard_goal_list.push_back(goals[i].get_pair());
                    found = true;
                    break;
                    cout << "hard" << endl;
                }
            }
            if (!found){
                soft_goal_list.push_back(goals[i].get_pair());
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
        nodes[i] = new TopDownMUGSNode();
    }

    uint id = (1U << soft_goal_list.size()) - 1;
    root = nodes[id];
    current_node = root;
    root->set_goals_id(id);
    open_list.push_back(root); 
}

void TopDownMUGSTree::expand(bool solvable){
    if(! solvable){
        vector<MUGSNode*> new_nodes = current_node->expand(nodes);
        open_list.insert(open_list.begin(), new_nodes.begin(), new_nodes.end());
    }
}

int TopDownMUGSTree::print_relation(){
    cout << "MUGS: " << endl;
    return root->print_relation(soft_goal_list);
}


}
