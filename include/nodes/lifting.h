#pragma once

#include "core/node.h"

class DeepFlowDllExport Lifting : public Node {
public:
	Lifting(deepflow::NodeParam *param);
	int minNumInputs() { return 1; }
	int minNumOutputs() { return 1; }
	void init();	
	void forward();
	void backward();
	std::string to_cpp() const;
private:
	deepflow::LiftingParam_Mode _mode;
};

