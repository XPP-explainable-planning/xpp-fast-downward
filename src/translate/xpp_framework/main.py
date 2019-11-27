from . import AS_property
from . import action_sets
from . import entailment
from . import LTL_property
from .parser import parse
from . import question

def run(options, task, sas_task):
    properties_path = options.plan_property
    print("----------------------------------------------------------------------------------------------")

    if properties_path != "None" and properties_path != "PROPERTY":
        print("Compile properties")
        
        #build typeObjectMap
        typeObjectMap = {}
        for o in task.objects:
            if not o.type_name in typeObjectMap:
                typeObjectMap[o.type_name] = []

            typeObjectMap[o.type_name].append(o.name)

        #print("++++++++++ typeObjectMap ++++++++++")
        #print(typeObjectMap)

        (actionSets, AS_properties, LTL_properties) = parse(properties_path, typeObjectMap)

        for s in actionSets.values():
            action_sets.compileActionSet(sas_task, s)

        AS_property.compileActionSetProperties(sas_task, AS_properties, actionSets)

        LTL_property.compileLTLProperties(options.only_add_LTL_prop_to_SAS, sas_task, LTL_properties, actionSets)
    


    # add question (subset of goal facts) for online explanation
    if options.question != "None":
        print("Add questions")
        question.add_question(options.question, sas_task)
    
    #TODO
    #if options.property_type == 2:
    #    print("--------------------------- ENTAILMENT COMPILATION ------------------------------------------------")
    #    properties = action_set_property.addActionSetPropertiesToTask(path, task, sas_task, options, False, True)
    #    #entailment.entailCompilation.addEntailmentsToTask(sas_task, properties)
    #    entailment.entailCompilation.addEntailmentsToTask(sas_task, properties)
