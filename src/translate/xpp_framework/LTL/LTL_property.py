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

        if line.startswith("#") or not re.match('[\s]*[a-zA-Z_-\}\{&|\[\]]+', line):
            continue
            
        m = re.match("property ([a-zA-Z0-9_]+)", line)
        if m:

            name = m.group(1)
            #print("Property name:" + name)

            parts = lines.pop(0).split()
            #print(parts)
            (formula, rest, c) = logic_formula.Operator.parse(parts)
            constants += c
            assert len(rest) == 0, "Parse LTL property failed:\n\tresult: " + str(formula) + "\n\trest is not empty: " + str(rest)

            properties.append(LTLProperty(name, formula))
            continue

        assert False, "Parse LTL properties: Wrong format in line: " + line

    return (properties, constants)

class LTLProperty:

    def __init__(self, name, formula):
        self.name = name
        self.formula = formula
        

    def generateAutomataRepresentation(self, constant_name_map, constant_id_map):

        #replace the fact names in the formula with valied names for the LTL to BA programm
        self.genericFormula = self.formula.replaceConstantsName(constant_id_map)

        #folder to store the output files of the LTL2BA programm
        temp_folder = os.environ['TEMP_FOLDER']

        cmd = "ltl2tgba -C -s \'" + str(self.genericFormula) + "\' > " + temp_folder + "/" + self.name
        os.system(cmd)

        self.automata = parse_SPIN_automata.parseNFA(temp_folder + "/" + self.name)
        self.automata.name = self.name

        #print(self.automata)
        #replace the generic variable name with the original state facts
        self.automata.replaceConstantsName(constant_name_map)

        


    def __repr__(self):
        s = self.name + ":\n\t" + str(self.formula)
        return s