from sas_tasks import *
from logic_formula import *
from parse_SPIN_automata import parseNFA
import LTL_property
from automata import compStates
import os

# adds the fluents which describe the state of automata
def addFluents(automata, id, sas_task):
    states_sorted = list(automata.states.values())
    #sort the states according to their id
    states_sorted.sort(compStates)

    variable = []
    #if the automata has more than one state one value per state is added
    if len(states_sorted) > 1:
        for s in states_sorted:
            variable.append("at(" + s.name + "," + automata.name + ")")
    else:
        #if the automata has only one state you can be either in this state or not 
        # fast downward variables have to have at least a domain size of 2
        for s in states_sorted:
            variable.append("at(" + s.name + "," + automata.name + ")")
            variable.append("not_at(" + s.name + "," + automata.name + ")")

    # id of the position var in the encoding of the variables
    automata.pos_var = len(sas_task.variables.value_names)

    #add variables to task
    sas_task.variables.value_names.append(variable)
    sas_task.variables.ranges.append(len(variable))
    sas_task.variables.axiom_layers.append(-1)

    # at to the initial state the initial state of the automata
    sas_task.init.values.append(automata.init.id)

    #variable to indicate if the automata is currently in an accepting state -> later used in the "goal fact dependencies"
    # id of the accepting var in the encoding to the variables 
    automata.accept_var = len(sas_task.variables.value_names)
    accept_var_domain = ["not_accepting(" + automata.name +")", "accepting(" + automata.name + ")"]
    sas_task.variables.value_names.append(accept_var_domain)
    sas_task.variables.ranges.append(len(accept_var_domain))
    sas_task.variables.axiom_layers.append(-1)

    #add to the initial state of the planning task if the initial state of the automata is accepting
    sas_task.init.values.append(int(automata.init.accepting))
    #in the goal the automata has to be in an accepting state
    sas_task.goal.pairs.append((automata.accept_var, 1))


    #variable which indicates if a transition in the automata should be executed
    automata.sync_var = len(sas_task.variables.value_names)
    sync_var_domain = ["not_sync(" + automata.name +")", "syn(" + automata.name + ")"]
    sas_task.variables.value_names.append(sync_var_domain)
    sas_task.variables.ranges.append(len(sync_var_domain))
    sas_task.variables.axiom_layers.append(-1)

    #initially we are in a wold state
    sas_task.init.values.append(0)

#add the transitions of the automata to the planning task
def automataTransitionOperators(automata, sas_task):
    new_operators = []
    for name, s in automata.states.iteritems():
        for t in s.transitions:
            print("Process action: " + t.name) 

            #encoding of the precondition and effect (var, pre, post, cond)
            pre_post = []
                
            #automata state: from state source to state target
            pre_post.append((automata.pos_var, t.source.id, t.target.id, []))

            #accepting
            # if the target state is accepting than the variable which indicates the acceptance of the 
            # automata is set to true
            if t.target.accepting:
                pre_post.append((automata.accept_var, int(t.source.accepting), int(t.target.accepting), []))

            #sync: condition for the alternating execution of task and automata actions
            #pre_post.append((automata.sync_var, 1, 0, []))

            #encode the guard of the transition in the precondition of the action
            if not t.guard.isTrue():
                # returns a disjunction of pre_post, such that every pre_post belongs to one action
                #guard_pre_post = t.guard.generateActionCondition(sas_task.variables.value_names)
                clauses = t.getClauses()
                #print(guard_pre_post)
                for clause in clauses:
                    con = [[]]
                    for l in clause:
                        new_con = []
                        new_values = literalVarValue(sas_task, l.constant, l.negated)
                        for var, value in new_values:
                            for c in con:
                                new_con.append(c + [(var, value, value, [])])
                        con = new_con
                    for c in con:
                        final_pre_post = pre_post + c
                        new_operators.append(SASOperator(t.name,[], final_pre_post, 0))
            else:
                new_operators.append(SASOperator(t.name,[], pre_post, 0))

    return new_operators


#add to every action in the original planning task the syn variable as precondition
# it has to be false when an actions is executed and is true afterwads
def addSyncPreconditionEffects(pre_sync_var, eff_sync_var, operators):

    pre_sync_con = (pre_sync_var, 1, 0, [])
    eff_sync_con = (eff_sync_var, 0, 1, [])

    for o in operators:
        o.pre_post.append(pre_sync_con)
        o.pre_post.append(eff_sync_con)
            
            
# computes for a literal the variable and value id in the planning task encoding
# neg indicates if the literal is used in a negated context
#TODO
def literalVarValue(sas_task, constant, neg):
    #print("Literal: " + str(literal))
    # the literal has to be mapped from "l_id" back to e.g. "ontable(a)" to be able to find it
    for var in range(len(sas_task.variables.value_names)):
        values = sas_task.variables.value_names[var]
        for v in range(len(values)):
            value_name = values[v].replace(", ", ",")
            if "Atom " + constant.name == value_name:
                print(neg)
                if neg:
                    #print(len(values))
                    #if the domain size of the variable is larger than 2, return all other variables except the given one
                    if(len(values) > 2):
                        res = []
                        for i in range(len(values)):
                            if i != v:
                                res.append((var, i))
                        return res
                    else:
                        # if the domain size is 2, return the negated constant
                        return [(var, v+1)]
                return [(var, v)]

    return None

def addWorldSyncvar(sas_task):
    world_sync_var = len(sas_task.variables.value_names)
    sync_var_domain = ["not_sync(world)", "syn(world)"]
    sas_task.variables.value_names.append(sync_var_domain)
    sas_task.variables.ranges.append(len(sync_var_domain))
    sas_task.variables.axiom_layers.append(-1)

    #initially we are in a wold state
    sas_task.init.values.append(1)

    return world_sync_var


def addLTLPlanProperties(sas_task, path_propertyfile):

    #parseProperties and transform the input to a format the LTL2B program can read 
    #the constants are only allowed to consist of letters and numbers
    properties, constants = LTL_property.parse(path_propertyfile)     

    constant_name_map = {}
    constant_id_map = {}
    #generate map for LTL2B friendly names
    constants = removeDuplicates(constants)
    for i in range(len(constants)):
        constant_id_map[constants[i]] = "c" + str(i)
        constant_name_map["c" + str(i)] = constants[i]

    constant_name_map["1"] = "true"
    constant_name_map["true"] = "true"
    
    #for every property run the LTL2B program
    for p in properties:
        print(str(p))
        p.generateAutomataRepresentation(constant_name_map, constant_id_map)

    operators = []

    #each automata and the world itself have a synchronization variable to constrain the execution order
    world_sync_var = addWorldSyncvar(sas_task)

    for i in range(len(properties)):
        a = properties[i].automata
        print(str(a))
        addFluents(a, i, sas_task) #also addd the sync vars
        operators.append(automataTransitionOperators(a, sas_task))
    
    #add the synchronization precondition and effects to the different operator sets
    for i in range(-1, len(properties)):

        pre_sync_var = None
        if(i == -1):
            pre_sync_var = world_sync_var
        else:
            pre_sync_var = properties[i].automata.sync_var

        eff_sync_var = None
        if(i+1 < len(properties)):
            eff_sync_var = properties[i+1].automata.sync_var
        else:
            eff_sync_var = world_sync_var

        if i == -1:
            addSyncPreconditionEffects(pre_sync_var, eff_sync_var, sas_task.operators)
        else:
            addSyncPreconditionEffects(pre_sync_var, eff_sync_var, operators[i])

    for ol in operators:
        for o in ol:
            sas_task.operators.append(o)

    
       
def removeDuplicates(list):
    new_list = []
    for e in list:
        if e in new_list:
            continue
        new_list.append(e)
    return new_list