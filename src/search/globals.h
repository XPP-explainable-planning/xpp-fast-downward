#ifndef GLOBALS_H
#define GLOBALS_H

#include <istream>
#include <vector>

class TaskProxy;

namespace successor_generator {
class SuccessorGenerator;
}

namespace utils {
struct Log;
}

void read_everything(std::istream &in);
// TODO: move this to task_utils or a new file with dump methods for all proxy objects.
void dump_everything();

// The following function is deprecated. Use task_properties.h instead.
bool is_unit_cost();

extern successor_generator::SuccessorGenerator *g_successor_generator;

extern utils::Log g_log;

#endif
