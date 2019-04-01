import xpp_framework.logic.logic_formula as logic_formula


class actionSetProperty:

    def __init__(self, name, soft, formula, constants):
        self.name = name
        self.soft = soft
        self.formula = formula
        # names of the set names that are used in the property
        self.constants = constants
        # id of the sat variable in the SAS encoding
        self.var_id = None

    

    #used to generate one instance of the property in a propertylass
    def generateInstance(self, instance_postfix):
        formula_instance = self.formula.addPostfix(instance_postfix)
        #print(formula_instance)
        instance_constants = []
        for c in self.constants:
            instance_constants.append(c +instance_postfix)

        return actionSetProperty(self.name + instance_postfix, self.soft, formula_instance, instance_constants)

    def containsSet(self, set_name):
        return set_name in self.constants 

    def getClauses(self):
        if(isinstance(self.formula, logic_formula.LOr)): #TODO
            return self.formula.getClauses([])
        else:
            return [self.formula.getClauses([])]

    def get_negated_Clauses(self):
        neg_formula = self.formula.negate()
        DNF_neg_formula = neg_formula.toDNF()
        if(isinstance(DNF_neg_formula, logic_formula.LOr)): #TODO
            return DNF_neg_formula.getClauses([])
        else:
            return [DNF_neg_formula.getClauses([])]

    def __eq__(self, other):
        return self.name == other.name

    def __hash__(self):
        return hash(self.name)

    def __repr__(self):
        return self.name + ": \n\t" + str(self.formula)


#collection of action set properties
class ActionSetProperties:

    def __init__(self):
        self.actionSets = {}
        self.properties = []
        self.soft_goals = [] #TODO move to separate file

    def addActionSet(self, actions):
        if(actions.name in self.actionSets):
            return False #TODO assertion?
        self.actionSets[actions.name]  = actions

    def addProperty(self, p):
        #if a property with the same name already exists, an unique id is added to the name
        if p in self.properties:
            p.name = p.name + "_" + str(len(self.properties))
            print("Property: " + p.name + " exists multiple times.")
        self.properties.append(p)


    #TODO replace with more general entailment function
    def generateImpPropertyFiles(self, folder):

        #print("Number of Properties: " + str(len(self.properties)))

        #for every property combination generate one file with the 
        #property p1 && ! p2
        for p1 in self.properties:

            #create one file with only p1
            DNF = p1.formula

            w_file = open(folder + "/basic-" + p1.name, "w")

            #comments
            w_file.write("#" + p1.formula.toPrefixForm() + "\n")

            #check which action sets we need
            #only write the action sets definition we need to the file
            needed_action_sets = Set()
            for (n, actionSet) in self.actionSets.iteritems():
                #print("ActionSet: " + n)
                if p1.containsSet(n):
                    needed_action_sets.add(actionSet)

            for actionSet in needed_action_sets:
                w_file.write(actionSet.genSetDefinition()) 

            w_file.write("\nproperty " + p1.name + "\n")               
            w_file.write(DNF.toPrefixForm()) 
            w_file.close()

            #create one file for every implication
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

                w_file = open(folder + "/entail-" + p1.name + "-" + p2.name, "w")

                #comments
                w_file.write("#" + p1.formula.toPrefixForm() + "\n")
                w_file.write("#" + p2.formula.toPrefixForm() + "\n\n")

                #check which action sets we need
                #only write the action sets definition we need to the file
                needed_action_sets = Set()
                for p in [p1,p2]:
                    #print("Property: " + p.name)
                    for (n, actionSet) in self.actionSets.iteritems():
                        #print("ActionSet: " + n)
                        if p.containsSet(n):
                            needed_action_sets.add(actionSet)

                for actionSet in needed_action_sets:
                    w_file.write(actionSet.genSetDefinition()) 

                w_file.write("\nproperty " + p1.name + "-" + p2.name + "\n")               
                w_file.write(DNF.toPrefixForm()) 
                w_file.close()



    

    def __repr__(self):
        s = "--------------------------------------------------------------\n"
        for (n,aS) in self.actionSets.iteritems():
            s += str(aS) + "\n"
        for p in self.properties:
            s += str(p) + "\n"
        s += "--------------------------------------------------------------\n"

        return s