from . import AS_property
from . import action_sets
from . import entailment
from . import LTL_property
from .parser import parse

def run(options, task, sas_task):
    path = options.plan_property
    print("----------------------------------------------------------------------------------------------")
    print("Plan property path: ", path)
    if path == "None":
        return

    #build typeObjectMap
    typeObjectMap = {}
    for o in task.objects:
        if not o.type_name in typeObjectMap:
            typeObjectMap[o.type_name] = []

        typeObjectMap[o.type_name].append(o.name)

    #print("++++++++++ typeObjectMap ++++++++++")
    #print(typeObjectMap)


    (actionSets, AS_properties, LTL_properties) = parse(path, typeObjectMap)

    for s in actionSets.values():
        action_sets.compileActionSet(sas_task, s)

    AS_property.compileActionSetProperties(sas_task, AS_properties, actionSets)

    LTL_property.compileLTLProperties(sas_task, LTL_properties, actionSets)
    
    #TODO
    #if options.property_type == 2:
    #    print("--------------------------- ENTAILMENT COMPILATION ------------------------------------------------")
    #    properties = action_set_property.addActionSetPropertiesToTask(path, task, sas_task, options, False, True)
    #    #entailment.entailCompilation.addEntailmentsToTask(sas_task, properties)
    #    entailment.entailCompilation.addEntailmentsToTask(sas_task, properties)
