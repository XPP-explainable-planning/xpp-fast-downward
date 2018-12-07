#! /usr/bin/env python3

import sys
from sets import Set
import logic_formula
from sas_tasks import *
import pddl
import re

class ParamMatcher:

    def __init__(self):
        self.matcher = {}
        self.num_params = -1
  
    def addAction(self,a):
        assert(self.num_params  == -1 or len(a.params) != self.num_params)
        self.num_params = len(a.params)
        current_matcher = self.matcher
        for i in range(len(a.params)):
            param = a.params[i]
            if not param in current_matcher:
                current_matcher[param] = {}
            current_matcher = current_matcher[param]

    def match(self,params):
        assert(self.num_params == len(params))
        current_matcher = self.matcher
        for i in range(len(params)):
            param = params[i]
            if not param in current_matcher:
                if not "*" in current_matcher:
                    return False
                else:
                    current_matcher = current_matcher["*"]
                    continue
            current_matcher = current_matcher[param]
        return True



class Action:

    @staticmethod
    def parse(string, typeObjectMap):
        parts = string.split()
        actionName = parts[0]
        action = Action(actionName, string)

        for n in range(1, len(parts)):
            #remove whitespaces
            param = parts[n].replace(" ","")
            if param == " " or param == "" or param == "\t" or param == "\n":
                    continue


            #if the param is a type instantiate the action with each object of the corresponsing type
            #it does not matter if any of the combinations does not exists in the planning task
            if param in typeObjectMap:              
                action.addParam("*")

            else:
                # if the param is an object just use the object
                #check if the object exists in the planning instance
                found = False
                for (o_type, objects) in typeObjectMap.iteritems():
                    if param in objects:
                        found = True
                        break
                assert found, "Param: " + param + " not found in planning task."
                
                action.addParam(param)

        return action


    def copy(self):
        nAction = Action(self.name, self.string)
        for p in self.params:
            nAction.addParam(p)

        return nAction

    def __init__(self, name, string):
        self.name = name
        self.string = string
        self.params = []

    def addParam(self, param):
        self.params.append(param.replace(" ",""))

    def __repr__(self):
        s = "(" + self.name.lower() + " "
        for i in range(len(self.params)):
            s += self.params[i]
            if i < len(self.params) - 1:
                s+= " " 
        s += ")"

        return s

class ActionSet:

    def __init__(self, name):
        self.name = name
        self.actions = []
        self.actionStrings = []
        self.definition = []
        self.var_id = None

        self.number_of_contained_ops = 0

    @staticmethod
    def parse(lines):
        line = lines.pop(0)
        #print("Actionset: " + line)
        set_parts = line.replace("\n","").split()
        if(len(set_parts) != 3):
            print("Set definition should have the form: set <name> <number of elmes> \n but is: " + line)
            assert(False)
        
        name = set_parts[1]
        number = int(set_parts[2])
        newActionSet = ActionSet(name)
        #print("Number of actions: " + str(number))

        for n in range(number):

            line = lines.pop(0)
            #print(line)

            # ignore comment and empty lines
            if line.startswith("#") or line == "\n" or line == "":
                continue

            actionString = line.replace("\n","")
            newActionSet.addDefinition(actionString)               

            n += 1

        
        return (newActionSet, lines)

    def generateActions(self, typeObjectMap):
        for d in self. definition:
            self.addAction(Action.parse(d, typeObjectMap))

        #generate action dict to check if a action in contained in the set
        self.action_dict = {}
        for a in self.actions:
            if not a.name in self.action_dict:
                self.action_dict[a.name] =  ParamMatcher()
                self.action_dict[a.name].addAction(a)


    def generateInstance(self, vars, params):

        assert(len(vars) == len(params))

        name = self.name
        for p in params:
            name += "_" + p

        newActionSet = ActionSet(name)
        for d in self.definition:
            new_d = d
            #print("-------------------------------------------------------------------")
            #print(newActionSet)
            #print(d + " -> ")
            i = 0
            while i < len(vars):
                #print("Map: " + vars[i] + " -> " + params[i] + "------")
                new_d = new_d.replace(vars[i], params[i])               
                i += 1

            #print("\t" + new_d)
            newActionSet.addDefinition(new_d)

        return newActionSet


    def addAction(self, a):
        self.actions.append(a)
        #self.actionStrings.append(str(a))

    def addDefinition(self, d):
        self.definition.append(d)

    def containsOperator(self, op):
        parts = op.name.replace("(","").replace(")","").split()
        if parts[0] in self.action_dict:
            if self.action_dict[parts[0]].match(parts[1:]):
                self.number_of_contained_ops += 1
                return True
        return False

    def genSetDefinition(self):
        s = "set " + self.name + " " + str(len(self.definition)) + "\n"
        for a in self.definition:
            s += a + "\n"
        s += "\n\n"
        return s

    def __repr__(self):
        s = "***************************\n"
        s += self.name + "\n"
        s += "Number: " + str(len(self.actions)) + "\n" 
        s += "Def: \n" 
        for d in self.definition:
            s += "\t" + d + "\n"
        s += "Actions:\n"
        for a in self.actions:
            s += str(a) + "\n"
        s += "***************************\n"

        return s

class actionSetPropertyClass:

    def __init__(self, name, action_sets, property_def, instances):
        self.name = name
        self.action_sets = action_sets
        self.property_def = property_def
        self.instances = instances

    @staticmethod
    def parse(lines):
        line = lines.pop(0)
        #print("PropertyClass: " + line)
         #name of the property
        name_param_parts = line.split()[1]
        class_name = name_param_parts.split("(")[0]
        class_params = name_param_parts.split("(")[1].replace(")","").replace(" ","").split(",")
        #print("Class params: ")
        #print(class_params)

        assert(lines.pop(0).startswith("{"))

        actionSets = []
        propertyDefs = []

        while(not line.startswith("}")):

            # ignore comment and empty lines
            line = lines[0]
           
            while not re.match('[\s]*[a-zA-B_-\}\{]+', line):
                #print(re.match('(\S)+', line))
                lines.pop(0)
                line = lines[0]


            #if line.startswith("}"):
            #    break
            line = line.replace("\t", "")

            if line.startswith("set"):
                #print(lines)
                (newActionSet, remaining_lines) = ActionSet.parse(lines)
                lines = remaining_lines
                actionSets.append(newActionSet)
                continue

            if line.startswith("property") or line.startswith("soft-property"):
                #print("parse property")
                (asProperty, remaining_lines) = actionSetProperty.parse(lines)
                lines = remaining_lines
                propertyDefs.append(asProperty)
                continue

            #assert(False)

        lines.pop(0)
        assert(lines.pop(0).startswith("{"))
        assert len(propertyDefs) > 0, "Missing property definition."

        #print("Parse instancs: " + lines[0])
        #parse instances
        instances = []
        while(not lines[0].startswith("}")):
            line = lines.pop(0)
            #print("Instances: " + line)
            while line.startswith("#") or line == "\n" or line == "":
                lines.pop(0)
                line = lines[0]


            line = line.replace(" ","").replace("\t","").replace("\n","")
            instance_name = line.split("(")[0]
            assert class_name == instance_name, "Reason: " + instance_name + " is not " + class_name
            instance_params = line.split("(")[1].replace(")","").replace(" ","").split(",")
            instances.append(instance_params)

        lines.pop(0)

        #print("Generate Instances")

        final_action_sets = []
        final_properties = []
        for instance in instances:
            for aSet in actionSets:
                final_set = aSet.generateInstance(class_params, instance)
                final_action_sets.append(final_set)

            postfix = ""
            for p in instance:
                postfix += "_" + p

            for prop in propertyDefs:
                final_properties.append(prop.generateInstance(postfix))

        
        return (final_action_sets, final_properties, lines)

        

class actionSetProperty:

    def __init__(self, name, soft, formula, constants):
        self.name = name
        self.soft = soft
        self.formula = formula
        self.constants = constants
        self.var_id = None

    @staticmethod
    def parse(lines):
        line = lines.pop(0)

        soft = line.split()[0].startswith("soft")

         #name of the property
        name = line.split()[1]

        # ignore comment and empty lines
        line = lines.pop(0)
        while line.startswith("#") or line == "\n" or line == "":
            line = lines.pop(0)
                    
        #parse prefix notion logic formula
        property_string = line.replace("\n","")
        (formula, rest, constants) = logic_formula.Operator.parse(property_string.split())
        asProperty = actionSetProperty(name, soft, formula, constants)

        return (asProperty, lines)

    def generateInstance(self, instance_postfix):
        formula_instance = self.formula.addPostfix(instance_postfix)
        #print(formula_instance)
        instance_constants = []
        for c in self.constants:
            instance_constants.append(c +instance_postfix)

        return actionSetProperty(self.name + instance_postfix, self.soft, formula_instance, instance_constants)

    def containsSet(self, set_name):
        #print("Contains: " + set_name)
        #print(self.constants)
        return set_name in self.constants 

    def getClauses(self):
        if(isinstance(self.formula, logic_formula.LOr)):
            return self.formula.getClauses([])
        else:
            return [self.formula.getClauses([])]

    def __repr__(self):
        return self.name + ": \n\t" + str(self.formula)

class ActionSetProperties:

    def __init__(self):
        self.actionSets = {}
        self.properties = []
        self.soft_goals = []

    def addActionSet(self, actions):
        if(actions.name in self.actionSets):
            #print("Name: " + actions.name + " ist not unique!!!")
            return False
        self.actionSets[actions.name]  = actions

    def addProperty(self, p):
        self.properties.append(p)


    def generateImpPropertyFiles(self, folder):

        #print("Number of Properties: " + str(len(self.properties)))

        #for every property combination generate one file with the 
        #property p1 && ! p2
        for p1 in self.properties:
            for p2 in self.properties:
                #print("Property file: " + folder + "/" + p1.name + "-" + p2.name)
                if p1.name == p2.name:
                    continue

                
                #print("F1:")
                #print(p1.formula.toPrefixForm())
                #print("F2:")
                #print(p2.formula.toPrefixForm())
                
                #construct the formula p1 && ! p2 
                #negate automatically pushes the negations to the constants
                f = logic_formula.LAnd(p1.formula, p2.formula.negate())
                DNF = f.toDNF() 

                w_file = open(folder + "/" + p1.name + "-" + p2.name, "w")

                #comments
                w_file.write("#" + p1.formula.toPrefixForm() + "\n")
                w_file.write("#" + p2.formula.toPrefixForm() + "\n\n")

                #check which action sets we need
                #only write the action sets definition we need to the file
                needed_action_sets = Set()
                for p in [p1,p2]:
                    for (n, actionSet) in self.actionSets.iteritems():
                        if p.containsSet(n):
                            needed_action_sets.add(actionSet)

                for actionSet in needed_action_sets:
                    w_file.write(actionSet.genSetDefinition()) 

                w_file.write("\nproperty " + p1.name + "-" + p2.name + "\n")               
                w_file.write(DNF.toPrefixForm()) 
                w_file.close()



    #compile the action sets properties into the planning task
    def compileToTask(self, sas_task):

        ######################## VARIABLES ###############################

        #variable to change from execution to evaluation phase
        eval_var_id = len(sas_task.variables.value_names)
        eval_var = ["not_eval", "eval"] 

        sas_task.variables.value_names.append(eval_var)
        sas_task.variables.ranges.append(len(eval_var)) #binary var
        sas_task.variables.axiom_layers.append(-1)

        #start with not eval phase
        sas_task.init.values.append(0)

        for op in sas_task.operators:
            #original operators can only be executed in the execution phase
            op.pre_post.append((eval_var_id, 0, 0, []))

        #action which changes from execution to evaluation phase    
        #TODO moved to the end
        #pre_post_change_phase = []
        #pre_post_change_phase.append((eval_var_id, 0, 1, []))
        #sas_task.operators.append(SASOperator("change_phase",[], pre_post_change_phase, 0))



        # compile property into one (goal) fact 
        #fact that indicates if the property is satisfied at the end of the plan
        #print("Properties: ")
        #for each property add one new binary variable
        for prop in self.properties:
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
            #update goal -> at the end the property has to be satisfied
            sas_task.goal.pairs.append((prop.var_id, 1))


        #for every action set add an variable which indicates if one of the actions in the set has been used
        #print("Actions Sets:")
        for (n,s) in self.actionSets.iteritems():
            new_vars =  ["not_used_" + s.name, "used_" + s.name]

            #new variable 
            s.var_id = len(sas_task.variables.value_names)
            #print(s.name + " id: " + str(s.var_id))
            sas_task.variables.value_names.append(new_vars)
            sas_task.variables.ranges.append(len(new_vars))
            sas_task.variables.axiom_layers.append(-1)

            # update initial state
            sas_task.init.values.append(0) # at the beginning of the planning task no action has been used

        ######################## ACTIONS ###############################


        # every action in the set assigns the corresponsing variable to true
        for op in sas_task.operators:
            for (n,s) in self.actionSets.iteritems():
                #if the action is in the action set than is assigns the variable to True/Used
                if s.containsOperator(op):
                    op.pre_post.append((s.var_id, -1, 1, []))
        
        for (n,s) in self.actionSets.iteritems():
            if s.number_of_contained_ops == 0:
                print("WARNING: " + s.name + " does not contain any action, no actions maps to the definition \n" + str(s.definition))
        
        #add the actions checking if the property ist true
        # Assumption: property is in disjunctive normal form
        #print("Actions checking satisfaction property:")
        for prop in self.properties:

            #print("Clauses: \n Property: " + str(prop))
            #for every clause in DNF we have to create one action
            clauses = prop.getClauses()
            #print("Result: ")
            #print(clauses)
            for c in clauses:
                pre_post = []
                # literals form the preconditions
                for l in c:
                    #print(l.constant.name)
                    if l.negated:
                        pre_post.append((self.actionSets[l.constant.name].var_id,0,0,[]))
                    else:
                        pre_post.append((self.actionSets[l.constant.name].var_id,1,1,[]))

                # as effect the property fact is set to true
                pre_post.append((prop.var_id, -1, 1, []))

                #can only be executed in the evaluation phase
                pre_post.append((eval_var_id, 1, 1, []))

                #TODO assigning a cost of 0 does not work -> only possible in a pddl version which uses action costs
                sas_task.operators.append(SASOperator(str(c),[], pre_post, 0))


        #action which changes from execution to evaluation phase 
        # only allow phase change if all hard goals are satisfied  
        pre_post_change_phase = []
        pre_post_change_phase.append((eval_var_id, 0, 1, []))
        

        #specify soft goals
        for g in self.soft_goals:
            #print("Add soft_goal: " + g)
            for i in range(len(sas_task.variables.value_names)):
                for j in range(len(sas_task.variables.value_names[i])):
                    value_name = sas_task.variables.value_names[i][j]
                    #print("\t " + value_name)
                    if value_name.endswith(g):
                        sas_task.variables.value_names[i][j] = "soft_" + value_name

        #add change phase action
        # only allow phase change if all hard goals are satisfied  
        #for (var, value) in sas_task.goal.pairs:
         #   if not sas_task.variables.value_names[var][value].startswith("soft"):
          #      pre_post_change_phase.append((var, value, value, []))
           #     print("New pre", var, "=", value)

        sas_task.operators.append(SASOperator("change_phase",[], pre_post_change_phase, 0))

    def __repr__(self):
        s = "--------------------------------------------------------------\n"
        for (n,aS) in self.actionSets.iteritems():
            s += str(aS) + "\n"
        for p in self.properties:
            s += str(p) + "\n"
        s += "--------------------------------------------------------------\n"

        return s


# typeObjectMap maps from a type to a list of objects which have this type
def parseActionSetProperty(path, typeObjectMap):

    reader = open(path, "r")
    lines = reader.readlines()
    reader.close()

    actionSetProperties = ActionSetProperties() 

    while len(lines) > 0:
        line = lines[0].replace("\n","") 
        

        # ignore comment and empty lines
        if line.startswith("#") or line == "\n" or line == "":
            lines.pop(0)
            continue

        #print("Main Loop: " + line)

        # parse set: set <name> <number of elems>
        if line.startswith("set"):
            (newActionSet, remaining_lines) = ActionSet.parse(lines)
            lines = remaining_lines
            newActionSet.generateActions(typeObjectMap)
            actionSetProperties.addActionSet(newActionSet)
            continue

        if line.startswith("property ") or line.startswith("soft-property "):
            (asProperty, remaining_lines) = actionSetProperty.parse(lines)
            lines = remaining_lines
            actionSetProperties.addProperty(asProperty)
            continue

        if line.startswith("propertyclass") or line.startswith("soft-propertyclass"):
            #print("--> propertyclass")
            (newActionSets, properties, remaining_lines) = actionSetPropertyClass.parse(lines)
            lines = remaining_lines
            for nas in newActionSets:
                #print(nas)
                nas.generateActions(typeObjectMap)
                #print(real_actions)
                actionSetProperties.addActionSet(nas)
            for p in properties:
                actionSetProperties.addProperty(p)
            continue

        if line.startswith("soft-goals"):
            lines.pop(0)
            line = lines.pop(0).replace("\n","")
            while line != "" and len(lines) > 0:
                actionSetProperties.soft_goals.append(line)
                line = lines.pop(0).replace("\n","")
            continue

        #print("nothing done")
        assert(False)

    #print(actionSetProperties)
    #print(">>>>>>>>>>>>>>>>>>>Parse finished>>>>>>>>>>>>>>>>>>>>>>>>>><")

    return actionSetProperties

def addActionSetPropertiesToTask(path, task, sas_task, options):
    #build typeObjectMap
    typeObjectMap = {}
    for o in task.objects:
        if not o.type_name in typeObjectMap:
            typeObjectMap[o.type_name] = []

        typeObjectMap[o.type_name].append(o.name)

    #print("++++++++++ typeObjectMap ++++++++++")
    #print(typeObjectMap)

    # parse action sets and properties
    asps = parseActionSetProperty(path, typeObjectMap)

    #compile properties into sas_task

    print("property_compilation_type: " + str(options.property_compilation_type))
    if options.property_compilation_type == None or options.property_compilation_type == 0:
        asps.compileToTask(sas_task)
        return
    if options.property_compilation_type == 1:
        print("Properties folder: " + options.properties_folder)
        asps.generateImpPropertyFiles(options.properties_folder)
        return



    


