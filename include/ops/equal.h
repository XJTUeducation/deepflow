#pragma once

#include "core/node.h"

class DeepFlowDllExport Equal : public Node {
public:
	Equal(NodeParam param);
	int minNumInputs() { return 2; }
	int minNumOutputs() { return 1; }
	void initForward();
	void initBackward();
	void forward();
	void backward();
};