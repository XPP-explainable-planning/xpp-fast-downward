import parse_SPIN_automata
import logic_formula
import re
import os

def parse(path):

    reader = open(path, "r")
    lines = reader.readlines()

    properties = []
    constants = []

    while len(lines) > 0:
        line = lines.pop(0)

        if line.startswith("#") or not re.match('[\s]*[a-zA-B_-\}\{&|\[\]]+', line):
            #print(re.match('(\S)+', line))
            #print("whitespace or comment: " + line)
            continue
            
        m = re.match("property ([a-zA-B0-9_]+)", line)
        if m:

            name = m.group(1)
            (formula, rest, c) = logic_formula.ModalOperator.parse(lines.pop(0).split())
            constants += c
            assert(len(rest) == 0)

            properties.append(LTLProperty(name, formula))
            continue

        assert False, "Parse LTL properties: Wrong format in line: " + line

    #TODO use constants
    return (properties, constants)

class LTLProperty:

    def __init__(self, name, formula):
        self.name = name
        self.formula = formula
        

    def generateAutomataRepresentation(self, constant_name_map, constant_id_map):
        
        self.genericFormula = self.formula.replaceConstantsName(constant_id_map)
        #print("Generic  formula: ")
        #print(self.genericFormula)

        #print(os.environ)
        generator = os.environ['LTL2B']
        temp_folder = os.environ['TEMP_FOLDER']

        cmd = generator + " -f \'" + str(self.genericFormula) + "\' > " + temp_folder + "/" + self.name
        os.system(cmd)

        self.automata = parse_SPIN_automata.parseNFA(temp_folder + "/" + self.name)
        self.automata.name = self.name
        #print("Before")
        #print(self.automata)
        self.automata.replaceConstantsName(constant_name_map)
        print(self.automata)

        


    def __repr__(self):
        s = self.name + ":\n\t" + str(self.formula)
        return s