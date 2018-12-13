#include "relation_tree.h"

#include "../task_proxy.h"
#include "../tasks/root_task.h"

#include "../utils/system.h"

using namespace std;
using namespace options;

namespace goalre {

Node::Node()
    : sleep_set_i(0), goals(1U << 31){
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
        //cout << ".........................." << endl;
        //cout << "Current soft goals: "  << goals << endl;
        for(FactPair l : get_goals(all_goals)){
            cout << "var" << l.var << " = " << l.value << ", ";
        }
        cout << endl;
        //cout << ".........................." << endl;
    }


int Node::print_relation(const std::vector<FactPair>& all_goals){
    if(printed){
        return 0;
    }
    printed = true;
    //cout << ">>.........................." << endl;
    TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
    //Check if all children are solvable
    bool c_solved = true;
    for(Node* c : children){
        c_solved = c_solved && c->isSolvable(); //or or and ?
    }
    if(children.size() == 0)
        c_solved = true;
    int printed_nodes = 0;
    if((c_solved) && ! isSolvable()){
        printed_nodes++;
        //cout << ".........................." << endl;
        //bool solved = isSolvable();
        //cout << "Solved: " << solved << endl;
        cout << "Unsolvable: " << endl;

        for(uint i = 0; i < get_goals(all_goals).size(); i++){
            FactPair g = get_goals(all_goals)[i];
            //cout << "var" << l.var << " = " << l.value << ", ";
            cout << taskproxy.get_variables()[g.var].get_fact(g.value).get_name();
            if(i < get_goals(all_goals).size()-1){
                cout << "|";
            }

        }
        cout << endl;
        //cout << ".........................." << endl;
    }
    /*
    if((! c_solved) && ! isSolvable()){
        cout << "Not minimal: softgoals: " << goals << endl;
    }
    if(isSolvable()){
        cout << "Solvable: softgoals: " << goals << endl;
    }
    */
    
    
    for(Node* c : children){
        printed_nodes += c->print_relation(all_goals);
    }
    //cout << "<<.........................." << endl;
    
    return printed_nodes;
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

    
    //cout << "Expand next node in relation tree" << endl;
    //printList(goals);
    

    //cout << "Expand(" << goals << ", " << sleep_set_i << ")" << std::endl;

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

RelationTree::RelationTree(GoalsProxy goals, bool all_soft_goals){
    TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
    for(uint i = 0; i < goals.size(); i++){
        if(all_soft_goals || taskproxy.get_variables()[goals[i].get_pair().var].get_fact(goals[i].get_pair().value).get_name().find("soft") == 0){
            soft_goal_list.push_back(goals[i].get_pair());
        }
        else{
            hard_goal_list.push_back(goals[i].get_pair());
        }
        
    }
    std::sort(soft_goal_list.begin(), soft_goal_list.end());
    if (soft_goal_list.size() > 31) {
        std::cerr << "too many goal facts, aborting" << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    nodes.resize((1U << soft_goal_list.size()));
    uint id = (1U << soft_goal_list.size()) - 1;
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
    std::vector<FactPair> current_goals;
    std::vector<FactPair> soft_goals = node->get_goals(soft_goal_list);
    current_goals.insert( current_goals.end(), hard_goal_list.begin(), hard_goal_list.end() );
    current_goals.insert( current_goals.end(), soft_goals.begin(), soft_goals.end() );
    return current_goals;
}

void RelationTree::print(){
    cout << "*********************************"  << endl;
    cout << "Size of tree: " << nodes.size() << endl;
    cout << "Hard goals: " << endl;
    TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
    for(FactPair g : hard_goal_list){
        cout << taskproxy.get_variables()[g.var].get_fact(g.value).get_name() << endl;
    }
    cout << "Soft goals: " << endl;
    for(FactPair g : soft_goal_list){
        cout << taskproxy.get_variables()[g.var].get_fact(g.value).get_name() << endl;
    }
    cout << "*********************************"  << endl;
    int printed_nodes = root->print_relation(soft_goal_list);
    cout << "*********************************"  << endl;
    cout << "Number of minimal unsolvable goal subsets: " << printed_nodes << endl;
    cout << "*********************************"  << endl;
}

}
