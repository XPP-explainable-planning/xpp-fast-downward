from parameter_matcher import ParamMatcher

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

    def __init__(self, name, string):
        self.name = name
        self.string = string
        self.params = []


    def copy(self):
        nAction = Action(self.name, self.string)
        for p in self.params:
            nAction.addParam(p)

        return nAction
   

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

        n = 0
        #for n in range(number):
        while True:

            line = lines.pop(0)
            #print(line)

            # ignore comment and empty lines
            if line.startswith("#"):
                continue

            if line == "\n" or line == "": # stop action set definition if there is a newline
                break

            actionString = line.replace("\n","")
            newActionSet.addDefinition(actionString)               

            n += 1

        #print("Num actions: " + str(n))
        return (newActionSet, lines)

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
                #print(op.name)
                return True
        return False

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


    

    def genSetDefinition(self):
        s = "set " + self.name + " " + str(len(self.definition)) + "\n"
        for a in self.definition:
            s += a + "\n"
        s += "\n\n"
        return s

    def __eq__(self, other):
        return self.name == other.name

    def __hash__(self):
        return hash(self.name)

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
