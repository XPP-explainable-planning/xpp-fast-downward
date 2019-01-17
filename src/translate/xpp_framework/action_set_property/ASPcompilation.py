import xpp_framework.logic.logic_formula as logic_formula
from sas_tasks import *


#compile the action sets properties into the planning task


def addChangePhase(sas_task):

    #variable to change from execution to one eval phase per property
    eval_phase_var_id = len(sas_task.variables.value_names)
    eval_var = [] 
    for i in range(2): 
        eval_var.append("phase_" + str(i))

    sas_task.variables.value_names.append(eval_var)
    sas_task.variables.ranges.append(len(eval_var)) #binary var
    sas_task.variables.axiom_layers.append(-1)

    #start with not eval phase
    sas_task.init.values.append(0)

    for op in sas_task.operators:
        #original operators can only be executed in the execution phase
        op.pre_post.append((eval_phase_var_id, 0, 0, []))

    #action which changes from execution to evaluation phase    
    pre_post_change_phase = []
    pre_post_change_phase.append((eval_phase_var_id, 0, 1, []))
    sas_task.operators.append(SASOperator("(change_to_evaluation)",[], pre_post_change_phase, 0))

    

    return eval_phase_var_id


def addPropertySatVariables(sas_task, asp, addToGoal):
    # compile property into one (goal) fact 
    #fact that indicates if the property is satisfied at the end of the plan
    #for each property add one new binary variable
    for prop in asp.properties:
        prop.var_id = len(sas_task.variables.value_names)
        soft_prefix = "hard"
        if prop.soft:
            soft_prefix = "soft"
        prop_vars = [soft_prefix + "_not_sat_" + prop.name, soft_prefix + "_sat_" + prop.name]

        #print(prop.name + " : " + str(prop.var_id))

        sas_task.variables.value_names.append(prop_vars)
        sas_task.variables.ranges.append(len(prop_vars)) #binary var
        sas_task.variables.axiom_layers.append(-1)

        # update initial state -> at the beginning the property is not satisfied
        sas_task.init.values.append(0)

        if addToGoal:
            #update goal -> at the end the property has to be satisfied
            sas_task.goal.pairs.append((prop.var_id, 1))


def addActionSetIndicators(sas_task, asp):

    #for every action set add an variable which indicates if one of the actions in the set has been used
    for (n,s) in asp.actionSets.iteritems():
        
        new_vars =  ["not_used_" + s.name, "used_" + s.name]

        s.var_id = len(sas_task.variables.value_names) # store id of the variable in the action set
        sas_task.variables.value_names.append(new_vars)
        sas_task.variables.ranges.append(len(new_vars))
        sas_task.variables.axiom_layers.append(-1)

        # update initial state
        sas_task.init.values.append(0) # at the beginning of the planning task no action has been used


def updateOriginalActions(sas_task, asp):
    # every action in the set assigns the corresponsing variable to true
    for (n,s) in asp.actionSets.iteritems():
        #print("-------------------------------------------------------------------")
        #print(s)
        for op in sas_task.operators:        
            #if the action is in the action set than is assigns the variable to True/Used              
            if s.containsOperator(op):
                #print(op.name)
                op.pre_post.append((s.var_id, -1, 1, []))
    
    for (n,s) in asp.actionSets.iteritems():
        if s.number_of_contained_ops == 0:
            print("WARNING: " + s.name + " does not contain any action, no actions maps to the definition \n" + str(s.definition))

def addPropertyCheckingActions(sas_task, asp, eval_phase_var_id, addNegativeActions=False):
    #add the actions checking if the property ist true
    # Assumption: property is in disjunctive normal form
    for prop in asp.properties:

        #print("Clauses: \n Property: " + str(prop))
        #for every clause in DNF we have to create one action
        clauses = prop.getClauses()
        #print(clauses)
        for c in clauses:
            pre_post = []
            # literals form the preconditions
            for l in c:
                #print(l.constant.name)
                if l.negated:
                    pre_post.append((asp.actionSets[l.constant.name].var_id,0,0,[]))
                else:
                    pre_post.append((asp.actionSets[l.constant.name].var_id,1,1,[]))
                #print(pre_post)

            # as effect the property fact is set to true
            pre_post.append((prop.var_id, -1, 1, []))

            #can only be executed in its evaluation phase
            pre_post.append((eval_phase_var_id, 1, 1, []))

            #TODO assigning a cost of 0 does not work -> only possible in a pddl version which uses action costs
           
            #print(pre_post)
            sas_task.operators.append(SASOperator("(" + prop.name + "_ " + str(c) + ")",[], pre_post, 0))

        if addNegativeActions:
            clauses = prop.get_negated_Clauses()
            #print(clauses)
            for c in clauses:
                pre_post = []
                # literals form the preconditions
                for l in c:
                    #print(l.constant.name)
                    if l.negated:
                        pre_post.append((asp.actionSets[l.constant.name].var_id,0,0,[]))
                    else:
                        pre_post.append((asp.actionSets[l.constant.name].var_id,1,1,[]))
                    #print(pre_post)

                # as effect the property fact is set to false
                pre_post.append((prop.var_id, -1, 0, []))

                #can only be executed in its evaluation phase
                pre_post.append((eval_phase_var_id, 1, 1, []))

                #TODO assigning a cost of 0 does not work -> only possible in a pddl version which uses action costs
            
                #print(pre_post)
                sas_task.operators.append(SASOperator("(neg_" + prop.name + "_ " + str(c) + ")",[], pre_post, 0))


def specifySoftGoals(sas_task, asp):
     #specify soft goals
    for g in asp.soft_goals:
        #print("Add soft_goal: " + g)
        for i in range(len(sas_task.variables.value_names)):
            for j in range(len(sas_task.variables.value_names[i])):
                value_name = sas_task.variables.value_names[i][j]
                #print("\t " + value_name)
                if value_name.endswith(g):
                    sas_task.variables.value_names[i][j] = "soft_" + value_name

def compileToTask(sas_task, asp, addPropertiesToGoal=True, addNegativeSatActions=False):

    ######################## VARIABLES ###############################
    eval_phase_var_id = addChangePhase(sas_task)
    addPropertySatVariables(sas_task, asp, addPropertiesToGoal)
    addActionSetIndicators(sas_task, asp)    

    ######################## ACTIONS ###############################

    updateOriginalActions(sas_task, asp)
    addPropertyCheckingActions(sas_task, asp, eval_phase_var_id, addNegativeActions=addNegativeSatActions)

    ####################### GOALS ########################
    specifySoftGoals(sas_task, asp)
        
        

       

       