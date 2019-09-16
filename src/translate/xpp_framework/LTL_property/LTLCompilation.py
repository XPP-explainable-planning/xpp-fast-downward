from sas_tasks import *
import xpp_framework.logic.logic_formula as logic_formula
from .automata import State
import os

auxillary_vars = {}

class AuxillaryVariable:

    def __init__(self, constant, sas_task):
        self.name = "aux_" + constant.name
        self.id = len(sas_task.variables.value_names)
        self.domain = ["false_" + self.name, "true_" + self.name]

        sas_task.variables.value_names.append(self.domain)
        sas_task.variables.ranges.append(len(self.domain))
        sas_task.variables.axiom_layers.append(-1)
        
        #print(self.name)
        (var_id, value_id) = literalVarValue(sas_task, constant, False)[0]

        eff_false_con = (self.id, -1, 0, [])
        eff_true_con = (self.id, -1, 1, [])

        for o in sas_task.operators:
            for (id, pc, e, ce) in o.pre_post:
                if id == var_id:
                    if e == value_id:
                        o.pre_post.append(eff_true_con)
                    else:
                        o.pre_post.append(eff_false_con) 

        #init state: 
        if sas_task.init.values[var_id] == value_id:
            sas_task.init.values.append(1)
        else:
            sas_task.init.values.append(0)

# adds the fluents which describe the state of automata
def addFluents(automata, id, sas_task):
    states_sorted = list(automata.get_states())
    #sort the states according to their id TODO which ID ?
    states_sorted.sort(key=State.get_name)

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
    accept_var_domain = ["not_accepting(" + automata.name +")", "soft_accepting(" + automata.name + ")"]
    sas_task.variables.value_names.append(accept_var_domain)
    sas_task.variables.ranges.append(len(accept_var_domain))
    sas_task.variables.axiom_layers.append(-1)

    #add to the initial state of the planning task if the initial state of the automata is accepting
    sas_task.init.values.append(int(automata.init.accepting))
    #in the goal the automata has to be in an accepting state
    sas_task.goal.pairs.append((automata.accept_var, 1))


    #variable which indicates if a transition in the automata should be executed
    # world an automaton sync vars are merged
    #automata.sync_var = len(sas_task.variables.value_names)
    #sync_var_domain = ["not_sync(" + automata.name +")", "syn(" + automata.name + ")"]
    #sas_task.variables.value_names.append(sync_var_domain)
    #sas_task.variables.ranges.append(len(sync_var_domain))
    #sas_task.variables.axiom_layers.append(-1)
    #initially we are in a wold state
    #sas_task.init.values.append(0)

#add the transitions of the automata to the planning task
def automataTransitionOperators(automata, sas_task, actionSets):
    new_operators = []
    for name, s in automata.states.items():
        for t in s.transitions:
            #print("Process action: " + t.name) 

            #encoding of the precondition and effect (var, pre, post, cond)
            pre_post = []
                
            #automata state: from state source to state target
            pre_post.append((automata.pos_var, t.source.id, t.target.id, []))

            #accepting
            # if the target state is accepting than the variable which indicates the acceptance of the #TODO why if
            # automata is set to true
            #if t.target.accepting:
            pre_post.append((automata.accept_var, int(t.source.accepting), int(t.target.accepting), []))
            #sync: condition for the alternating execution of task and automata actions
            #pre_post.append((automata.sync_var, 1, 0, []))

            #transition name:
            t_name = automata.name + ": " + t.name
            #encode the guard of the transition in the precondition of the action
            if not t.guard.isTrue():
                # returns a disjunction of pre_post, such that every pre_post belongs to one action
                clauses = t.getClauses()
                #print("Clauses: ")
                #print(clauses)
                for clause in clauses:
                    con = [[]]
                    for l in clause:
                        new_con = []

                        #print(actionSets)
                        if l.constant.name in actionSets:
                            if l.negated:
                                new_values = [(actionSets[l.constant.name].var_id, 0)]
                            else:
                                new_values = [(actionSets[l.constant.name].var_id, 1)]
                        else:
                            new_values = literalVarValue(sas_task, l.constant, l.negated)
                            if l.negated and len(new_values) > 2:
                                if not l.constant.name in auxillary_vars:
                                    # create new auxillary var
                                    new_aux_var = AuxillaryVariable(l.constant, sas_task)
                                    auxillary_vars[l.constant.name] = new_aux_var

                                aux_var = auxillary_vars[l.constant.name]
                                new_values = [(aux_var.id, 0)]


                        #print("Literal values:")
                        #print(new_values)
                        assert new_values and len(new_values) > 0, "value not found: " + str(l.constant)
                        for var, value in new_values:
                            for c in con:
                                new_con.append(c + [(var, value, value, [])])
                        con = new_con
                    for c in con:
                        final_pre_post = pre_post + c
                        new_operators.append(SASOperator(t_name,[], final_pre_post, 0))
            else:
                new_operators.append(SASOperator(t_name,[], pre_post, 0))

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

def addWorldSyncvar(sas_task, properties):
    world_sync_var = len(sas_task.variables.value_names)
    sync_var_domain = ["sync(world)"]
    for p in properties:
        sync_var_domain.append("sync(" + p.automata.name + ")")

    sas_task.variables.value_names.append(sync_var_domain)
    sas_task.variables.ranges.append(len(sync_var_domain))
    sas_task.variables.axiom_layers.append(-1)

    #initially we are in a wold state
    sas_task.init.values.append(0)

    return world_sync_var

def add_sync_conditions(sas_task, operators, properties, world_sync_var):

    operators_phases = [sas_task.operators] + operators

    #add the synchronization precondition and effects to the different operator sets
    for i in range(len(properties)+1):

        sync_con = (world_sync_var, i, (i+1)%(len(properties)+1), [])

        for o in operators_phases[i]:
            o.pre_post.append(sync_con)



def add_reset_state_sets(last_ops, actionSets):
    for s in actionSets.values():
        if s.state_set:
            for op in last_ops:
                op.pre_post.append((s.var_id, -1, 0, []))

    
def compileLTLProperties(sas_task, properties, actionSets):

    if len(properties) == 0:
        return 

    new_operators = []

    #each automata and the world itself have a synchronization variable to constrain the execution order
    world_sync_var = addWorldSyncvar(sas_task, properties)

    for i in range(len(properties)):
        automata = properties[i].automata
        #print(str(a))
        addFluents(automata, i, sas_task) #also addd the sync vars
        new_operators.append(automataTransitionOperators(automata, sas_task, actionSets))
    
    
    add_sync_conditions(sas_task, new_operators, properties, world_sync_var)
    add_reset_state_sets(new_operators[len(properties)-1], actionSets)
    
    for ol in new_operators:
        for o in ol:
            sas_task.operators.append(o)

    print("Number of auxiliary variables: " + str(len(auxillary_vars)))

