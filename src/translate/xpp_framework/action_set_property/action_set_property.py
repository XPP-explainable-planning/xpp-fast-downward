from . import parser
from . import ASPcompilation

def addActionSetPropertiesToTask(path, task, sas_task, options, addGoalFacts, addNegSatActions):
    #build typeObjectMap
    typeObjectMap = {}
    for o in task.objects:
        if not o.type_name in typeObjectMap:
            typeObjectMap[o.type_name] = []

        typeObjectMap[o.type_name].append(o.name)

    #print("++++++++++ typeObjectMap ++++++++++")
    #print(typeObjectMap)

    # parse action sets and properties
    asps = parser.parseActionSetProperty(path, typeObjectMap)

    #compile properties into sas_task

    print("property_compilation_type: " + str(options.property_compilation_type))
    if options.property_compilation_type == None or options.property_compilation_type == 0:
        ASPcompilation.compileToTask(sas_task, asps, addPropertiesToGoal=addGoalFacts, addNegativeSatActions=addNegSatActions)
       
        return asps

    if options.property_compilation_type == 1:
        print("Properties folder: " + options.properties_folder)
        asps.generateImpPropertyFiles(options.properties_folder)
        return asps



    


