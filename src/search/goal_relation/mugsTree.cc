#include "mugsTree.h"

#include "../task_proxy.h"
#include "../tasks/root_task.h"

#include "../utils/system.h"

#include "meta_search_tree.h"

#include "bitset"

using namespace std;
using namespace options;
using namespace mst;

namespace mugstree {

std::vector<FactPair> MUGSNode::get_goals(const std::vector<FactPair>& all_goals) const{
    std::vector<FactPair> res;
    for (uint i = 0; i < all_goals.size(); i++) {
        if ((goals >> i) & 1U) {
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

void MUGSNode::print(const std::vector<FactPair>& all_goals){
   //cout << ".........................." << endl;
    TaskProxy taskproxy = TaskProxy(*tasks::g_root_task.get());
    //cout << goals << ": ";
    //cout << goals << endl;
    /*
    for(FactPair l : get_goals(all_goals)){
        cout << "var" << l.var << " = " << l.value << ", ";
    }
    cout << endl;
    */
    //cout << ".........................." << endl;
    for(uint i = 0; i < get_goals(all_goals).size(); i++){
        FactPair g = get_goals(all_goals)[i];
        //cout << "var" << l.var << " = " << l.value << ", ";
        cout << taskproxy.get_variables()[g.var].get_fact(g.value).get_name();
        if(i < get_goals(all_goals).size()-1){
            cout << "|";
        }
    }
    /*
    uint print_id=0;
    uint goals_id = this->get_goals_id();
    for(uint i = 0; i < all_goals.size(); i++){
        print_id = (print_id << 1) | ( goals_id & (1U));
        goals_id = goals_id  >> 1;
    }
    cout << endl << "\t->" << std::bitset<32>(print_id);
    */
}


std::vector<FactPair> MUGSTree::get_goals(const MUGSNode* node) const
{
    std::vector<FactPair> current_goals;
    std::vector<FactPair> soft_goals = node->get_goals(soft_goal_list);
    current_goals.insert( current_goals.end(), hard_goal_list.begin(), hard_goal_list.end() );
    current_goals.insert( current_goals.end(), soft_goals.begin(), soft_goals.end() );
    return current_goals;
}

void  MUGSTree::next_node(){
     current_node = get_next_node();
}

std::vector<FactPair> MUGSTree::get_next_goals()
{
    /*
	cout << "Current node: " << endl;
	current_node->print(soft_goal_list);
    cout << endl;
    */
    return get_goals(current_node);
}

std::vector<FactPair>  MUGSTree::get_next_init(){
    std::vector<FactPair> change_init;
    return change_init;
}

void MUGSTree::current_goals_solved(){
    current_node->solved();
}

void MUGSTree::current_goals_not_solved(){
    current_node->not_solved();
}

void MUGSTree::print(){
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
    int printed_nodes = print_relation();
    cout << "*********************************"  << endl;
    cout << "Number of minimal unsolvable goal subsets: " << printed_nodes << endl;
    cout << "*********************************"  << endl;
}

}
