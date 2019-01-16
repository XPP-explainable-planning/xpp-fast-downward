from sas_tasks import *

def addEntailmentsToTask(sas_task, propertyCollection):
    for p1 in propertyCollection.properties :
        for p2 in propertyCollection.properties :
            if p1 != p2 :
                sas_task.entail.pairs.append((p1.var_id, p2.var_id)) 
