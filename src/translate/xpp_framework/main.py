from . import action_set_property
from . import entailment
from . import LTL

def run(options, task, sas_task):
    path = options.plan_property
    print("----------------------------------------------------------------------------------------------")
    print("Plan property path: ", path)
    if path == "None":
        return
    
    if options.property_type == 0:
        action_set_property.addActionSetPropertiesToTask(path, task, sas_task, options, True, False)
        return
    
    if options.property_type == 1:
        LTL.addLTLPlanProperties(sas_task, path)    
        return

    if options.property_type == 2:
        print("--------------------------- ENTAILMENT COMPILATION ------------------------------------------------")
        properties = action_set_property.addActionSetPropertiesToTask(path, task, sas_task, options, False, True)
        #entailment.entailCompilation.addEntailmentsToTask(sas_task, properties)
        entailment.entailCompilation.addEntailmentsToTask(sas_task, properties)
