from enum import Enum
import types

class Parameter:

    def __init__(self, name, id):
        self.name = name
        self.id = id

class Predicate:

    def __init__(self, name):
        self.name = name
        self.params = []

    def addParam(self, param):
        self.params.append(param)


class LogicalOperator:

    def __init__(self, left, right):
        self.left = left
        self.right = right

    #parse logic formula in prefix notation
    @staticmethod
    def parse(parts):
        if parts[0] == "!":
            return LNot.parse(parts)
        elif parts[0] == "||": 
            return LOr.parse(parts)
        elif parts[0] == "&&": 
            return LAnd.parse(parts)
        else:
            return LConstant.parse(parts)


class LConstant(LogicalOperator):

    @staticmethod
    def parse(parts):
        operand = parts.pop(0)
        return (LConstant(operand, None), parts, [operand])

    def __init__(self, name, id):
        self.name = name
        self.id = id

    def isTrue(self):
        return self.name == "true"

    def getClauses(self, clauses):
        clauses.append(LLiteral(self, False))
        return clauses

    def negate(self):
        return LNot(self)

    def distribute(self):
        return self

    def addPostfix(self, fix):
        return LConstant(self.name + fix, self.id)

    def replaceConstantsName(self, map):
        return LConstant(map[self.name], self.id)

    def toPrefixForm(self):
        return self.name

    def __repr__(self):
        return self.name

#Literals are used to generate the actions which decide if a property is satisfied
class LLiteral(LogicalOperator):

    def __init__(self, constant, negated):
        self.constant = constant
        self.negated = negated

    def __repr__(self):
        if self.negated:
            return "! " + self.constant.name
        else:   
            return self.constant.name


class LNot(LogicalOperator):
    @staticmethod
    def parse(parts):
        operatorString = parts.pop(0)
        assert(operatorString == "!")
        (operand, rest, constants) = LogicalOperator.parse(parts)

        return (LNot(operand), rest, constants)

    
    def __init__(self, operand):
        self.operand = operand

    def getClauses(self, clauses):
        clauses.append(LLiteral(self.operand, True))
        return clauses

    def negate(self):
        return self.operand

    def distribute(self):
        return self

    def addPostfix(self, fix):
        return LNot(self.operand.addPostfix(fix))

    def toPrefixForm(self):
        return " ! " + self.operand.toPrefixForm()

    def replaceConstantsName(self, map):
        return LNot(self.operand.replaceConstantsName(map))
    

    def __repr__(self):
        return " (! " + str(self.operand) + ") "

class LAnd(LogicalOperator):
    @staticmethod
    def parse(parts):
        operatorString = parts.pop(0)
        assert(operatorString == "&&")
        (operand_left, rest1, constants1) = LogicalOperator.parse(parts)
        (operand_right, rest2, constants2) = LogicalOperator.parse(rest1)

        return (LAnd(operand_left, operand_right), rest2, constants1 + constants2)


    def __init__(self, left, right):
        self.left = left
        self.right = right

    def getClauses(self, clauses):
        clauses_left = self.left.getClauses([])
        clauses_right = self.right.getClauses([])
        return clauses + clauses_left + clauses_right

    def negate(self):
        left = self.left.negate()
        right = self.right.negate()

        return LOr(left, right)

    def distribute(self):
        if isinstance(self.right, LOr):
            return LOr(LAnd(self.left.distribute(), self.right.left.distribute()), LAnd(self.left.distribute(), self.right.right.distribute()))
        if isinstance(self.left, LOr):
            return LOr(LAnd(self.left.left.distribute(), self.right.distribute()), LAnd(self.left.right.distribute(), self.right.distribute()))
        
        return LAnd(self.left.distribute(), self.right.distribute())

    #only works for the special case a & b where a and b are in DNF (#TODO is this realy the case?)
    def toDNF(self):
        print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>")
        print("toDNF: " + self.toPrefixForm())

        #apply distributive law to push all ANDs in until the formula does not change anymore
        current_f = self
        new_f = self.distribute()
        steps = 1
        while(current_f.toPrefixForm() != new_f.toPrefixForm()):
            current_f = new_f
            new_f = current_f.distribute()
            #print("Step: " + str(steps))
            steps += 1
            #print(new_f.toPrefixForm())

        DNF = new_f
            
        print("Result: " + DNF.toPrefixForm())
        print("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<")
        return DNF


    def addPostfix(self, fix):
        return LAnd(self.left.addPostfix(fix), self.right.addPostfix(fix))

    def toPrefixForm(self):
        return " && "  + self.left.toPrefixForm() + " " + self.right.toPrefixForm()

    def replaceConstantsName(self, map):
        return LAnd(self.left.replaceConstantsName(map), self.right.replaceConstantsName(map))

    def __repr__(self):
        return "(" + str(self.left) + " && " + str(self.right) + ")"

class LOr(LogicalOperator):
    @staticmethod
    def parse(parts):
        operatorString = parts.pop(0)
        assert(operatorString == "||")
        (operand_left, rest1, constants1) = LogicalOperator.parse(parts)
        (operand_right, rest2, constants2) = LogicalOperator.parse(rest1)

        return (LOr(operand_left, operand_right), rest2, constants1 + constants2)


    def __init__(self, left, right):
        self.left = left
        self.right = right

    def getClauses(self, clauses):
        clauses_left = self.left.getClauses([])
        clauses_right = self.right.getClauses([])
        
        if isinstance(self.left, LOr) and isinstance(self.right, LOr):
            return clauses_left + clauses_right
        if isinstance(self.left, LOr):
            return clauses_left + [clauses_right]
        if isinstance(self.right, LOr):
            return [clauses_left] + clauses_right
        
        return [clauses_left, clauses_right]

    def negate(self):
        left = self.left.negate()
        right = self.right.negate()

        return LAnd(left, right)

    def distribute(self):
        return LOr(self.left.distribute(), self.right.distribute()) 

    def addPostfix(self, fix):
        return LOr(self.left.addPostfix(fix), self.right.addPostfix(fix))

    def toPrefixForm(self):
        return " || "  + self.left.toPrefixForm() + " " + self.right.toPrefixForm()

    def replaceConstantsName(self, map):
        return LOr(self.left.replaceConstantsName(map), self.right.replaceConstantsName(map))

    def __repr__(self):
        return "(" + str(self.left) + " || " + str(self.right) + ")"


class MOp(Enum):
    SOMETIMES = 1
    ALSWAYS = 2
    UNTIL = 3
    WEAKUNTIL = 4
    NEXT = 5

#we do support nested modal operators 
class ModalOperator:


    @staticmethod
    def parse(parts):
        if parts[0] == "!":
            return LNot.parse(parts)
        elif parts[0] == "||": 
            return LOr.parse(parts)
        elif parts[0] == "&&": 
            return LAnd.parse(parts)

        elif parts[0] == "<>": 
            return OpSometimes.parse(parts)
        elif parts[0] == "[]": 
            return OpAlways.parse(parts)
        elif parts[0] == "X": 
            return OpNext.parse(parts)
        elif parts[0] == "U": 
            return OpUntil.parse(parts)
        elif parts[0] == "W": 
            return OpWeakUntil.parse(parts)
        else:
            return LConstant.parse(parts)


class OpSometimes(ModalOperator):

    @staticmethod
    def parse(parts):
        operatorString = parts.pop(0)
        assert(operatorString == "<>")
        (operand, rest, constants) = ModalOperator.parse(parts)

        return (OpSometimes(operand), rest, constants)

    def __init__(self, operand):
        self.operand = operand

    def replaceConstantsName(self, map):
        return OpSometimes(self.operand.replaceConstantsName(map))

    
    def __repr__(self):
        return " <> " + str(self.operand)    

class OpAlways(ModalOperator):

    @staticmethod
    def parse(parts):
        operatorString = parts.pop(0)
        assert(operatorString == "[]")
        (operand, rest, constants) = ModalOperator.parse(parts)

        return (OpAlways(operand), rest, constants)

    def __init__(self, operand):
        self.operand = operand

    def replaceConstantsName(self, map):
        return OpAlways(self.operand.replaceConstantsName(map))

    def __repr__(self):
        return " [] " + str(self.operand)

    
class OpNext(ModalOperator):

    @staticmethod
    def parse(parts):
        operatorString = parts.pop(0)
        assert(operatorString == "X")
        (operand, rest, constants) = ModalOperator.parse(parts)

        return (OpNext(operand), rest, constants)

    def __init__(self, operand):
        self.operand = operand

    def replaceConstantsName(self, map):
        return OpNext(self.operand.replaceConstantsName(map))

    def __repr__(self):
        return " X " + str(self.operand)

class OpUntil(ModalOperator):

    @staticmethod
    def parse(parts):
        operatorString = parts.pop(0)
        assert(operatorString == "U")
        (operand_left, rest1, constants1) = ModalOperator.parse(parts)
        (operand_right, rest2, constants2) = ModalOperator.parse(rest1)

        return (OpUntil(operand_left, operand_right), rest2, constants1 + constants2)


    def __init__(self, left, right):
        self.left = left
        self.right = right

    def replaceConstantsName(self, map):
        return OpUntil(self.left.replaceConstantsName(map), self.right.replaceConstantsName(map))

    def __repr__(self):
        return str(self.left) + " U " + str(self.right)


class OpWeakUntil(ModalOperator):

    @staticmethod
    def parse(parts):
        operatorString = parts.pop(0)
        assert(operatorString == "W")
        (operand_left, rest1, constants1) = ModalOperator.parse(parts)
        (operand_right, rest2, constants2) = ModalOperator.parse(rest1)

        return (OpWeakUntil(operand_left, operand_right), rest2, constants1 + constants2)


    def __init__(self, left, right):
        self.left = left
        self.right = right

    def replaceConstantsName(self, map):
        return OpWeakUntil(self.left.replaceConstantsName(map), self.right.replaceConstantsName(map))

    def __repr__(self):
        return str(self.left) + " W " + str(self.right)

