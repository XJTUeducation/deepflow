#pragma once

#include "core/node.h"

class DeepFlowDllExport LeakyRelu : public Node {
public:
	LeakyRelu(const NodeParam &param);	
	int minNumInputs() { return 1; }
	int minNumOutputs() { return 1; }
	void initForward();
	void initBackward();
	void forward();
	void backward();
	virtual ForwardType forwardType() { return DEPENDS_ON_OUTPUTS; }
	virtual BackwardType backwardType() { return DEPENDS_ON_INPUTS; }
private:
	float _negative_slope;
};
