class Predicate(object):
    def __init__(self, name, arguments):
        self.name = name
        self.arguments = arguments

    def __str__(self):
        return "%s(%s)" % (self.name, ", ".join(map(str, self.arguments)))

    def to_json(self):
        s = "{\n"
        s += "\t\"name\": \"" + self.name + "\",\n"
        s += "\t\"arguments\": [" + ", ".join(["\"" + a.type_name + "\"" for a in self.arguments])
        s += "]"
        s += "\n}"
        return s

    def get_arity(self):
        return len(self.arguments)
