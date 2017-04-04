#pragma once

#include "core/node.h"

class DeepFlowDllExport Square : public Node {
public:
	Square(NodeParam param);
	int minNumInputs() { return 1; }
	int minNumOutputs() { return 1; }
	void initForward();
	void initBackward();
	void forward();
	void backward();
};