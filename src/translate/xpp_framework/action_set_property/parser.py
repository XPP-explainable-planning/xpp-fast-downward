from .action import *
from .property import *


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