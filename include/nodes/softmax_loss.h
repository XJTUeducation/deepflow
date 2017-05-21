#pragma once

#include "core/loss.h"
#include "proto/deepflow.pb.h"

class DeepFlowDllExport SoftmaxLoss : public Loss {
public:
	SoftmaxLoss(const deepflow::NodeParam &param);
	int minNumInputs() { return 2; }
	int minNumOutputs() { return 1; }
	void initForward();	
	void initBackward();
	void forward();
	void backward();
	std::string to_cpp() const;
	ForwardType forwardType() { return ALWAYS_FORWARD; }
	BackwardType backwardType() { return ALWAYS_BACKWARD; }
private:
	cudnnHandle_t _cudnnHandle;	
};