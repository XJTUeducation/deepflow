#pragma once

#include "core/node.h"

class DeepFlowDllExport Equal : public Node {
public:
	Equal(deepflow::NodeParam *param);
	int minNumInputs() { return 2; }
	int minNumOutputs() { return 1; }	
	void init();	
	void forward();
	void backward();
	std::string to_cpp() const;
};
