#include "core/generator.h"

Generator::Generator(const NodeParam &param) {
	LOG_IF(FATAL, param.has_generator_param() == false) << "param.has_generator_param() == false";
}