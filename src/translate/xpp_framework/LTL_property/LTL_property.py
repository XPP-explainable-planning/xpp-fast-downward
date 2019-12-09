from .parse_SPIN_automata import *
import xpp_framework.logic.logic_formula as logic_formula
import re
import os



class LTLProperty:

    def __init__(self, name, formula, constants):
        self.name = name
        self.formula = formula

        self.generateAutomataRepresentation(constants)
        
    def SAS_repr(self, actionSets):
        return self.name, self.formula.SAS_repr(actionSets)

    def generateAutomataRepresentation(self, constants):
        constant_name_map = {}
        constant_id_map = {}
        #generate map for LTL2B friendly names
        constants = removeDuplicates(constants)
        for i in range(len(constants)):
            constant_id_map[constants[i]] = "c" + str(i)
            constant_name_map["c" + str(i)] = constants[i]

        constant_name_map["1"] = "true"
        constant_name_map["true"] = "true"

        #replace the fact names in the formula with valied names for the LTL to BA programm
        self.genericFormula = self.formula.replaceConstantsName(constant_id_map)

        #folder to store the output files of the LTL2BA programm
        #temp_folder = os.environ['TEMP_FOLDER']

        # -C complete automaton
        # -s state based acceptance
        # -D deterministic
        #spot_bin = "/mnt/data_server/eifler/LTL2BA/spot-2.6.3/bin/"
        #spot_bin = "~/Uni/XAI/programms/Spot/spot-2.6.3/bin/"
        spot_bin = os.environ.get("SPOT_BIN_PATH", "/mnt/data_server/eifler/LTL2BA/spot-2.6.3/bin/")
        formula = str(self.genericFormula)
        output_file = self.name
        ltl2hoa_path = os.environ.get("LTL2HAO_PATH", "/mnt/data_server/eifler/ltl-mode/ltlfkit/")
        #cmd = spot_bin + "ltlfilt --from-ltlf -f '" + formula + "' | " + spot_bin + "ltl2tgba -B -D -s -C | " + spot_bin + "autfilt --remove-ap=alive -B -D -C -s --small > " + output_file
        cmd = "python3 " + ltl2hoa_path + "ltlf2hoa.py '" + formula + "' | " + spot_bin + "autfilt --small -C -D -s --spin > " + output_file

        os.system(cmd)

        self.automata = parseNFA(self.name)
        self.automata.name = self.name

        cmd = "rm " + self.name
        os.system(cmd)

        #print(self.automata)
        #replace the generic variable name with the original state facts
        self.automata.replaceConstantsName(constant_name_map)

        


    def __repr__(self):
        s = self.name + ":\n\t" + str(self.formula)
        return s


def removeDuplicates(list):
    new_list = []
    for e in list:
        if e in new_list:
            continue
        new_list.append(e)
    return new_list
