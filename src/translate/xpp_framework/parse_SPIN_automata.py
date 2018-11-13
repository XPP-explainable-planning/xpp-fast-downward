from automata import *
from logic_formula import *
import re


def parseGuard(guard_string):

    parts = guard_string.split()
    (guard, rest, constants) = LogicalOperator.parse(parts)

    assert(len(rest) == 0)

    return guard

def parseTransition(line, automata, source):
    #print("Trans: " + line)
    m = re.match(r":: \((.+)\) -> goto (.+)", line)
    assert(m)
    
    guard_string = m.group(1)
    guard, rest, constants = LogicalOperator.parse(guard_string.split())
    assert(len(rest) == 0)


    targetName = m.group(2)
    if not targetName in automata.states:
        target = State(targetName)
        automata.addState(target)
    else:
        target = automata.states[targetName]


    trans = Transition(str(guard) + "->" + target.name, source, target, guard)
    automata.transitions.append(trans)
    source.transitions.append(trans)

    return constants

def parseStateName(line, automata):
    m = re.match("(.+):", line)
    
    name = m.group(1)

    isInit = name.endswith("init")
    accepting = name.startswith("accept")

    if not name in automata.states:
        state = State(name)
        automata.addState(state)
    else:
        state = automata.states[name]

    state.isInit = isInit
    state.accepting = accepting

    if state.isInit:
        automata.init = state

    return state




def parseNFA(path):
    automata = Automata("test_automata")

    reader = open(path, "r")
    lines = reader.readlines()

    source = None
    constants = []

    while len(lines) > 0:
        line = lines.pop(0)
        line = line.replace("\t", "")
        line = line.replace("\n", "")
        #print("|" + line + "|")

        if line.startswith("never") or line.startswith("}"):
            continue

        if line.startswith("if"):
            constants += parseTransition(lines.pop(0).replace("\t",""), automata, source)
            continue

        if line.startswith("::"):
            constants += parseTransition(line, automata, source)
            continue

        if line.startswith("fi"):
            source = None
            continue

        # the skip action is interpreted as a "true" self loop 
        if line.startswith("skip"):
            true_exp = LConstant("true", None)
            trans = Transition("skip", source, source, true_exp)
            automata.transitions.append(trans)
            source.transitions.append(trans)
            continue

        source = parseStateName(line, automata)

    

    return automata    


def removeDuplicates(list):
    new_list = []
    for e in list:
        if e in new_list:
            continue
        new_list.append(e)