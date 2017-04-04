#pragma once

#include "core/node.h"

class DeepFlowDllExport Reduce : public Node {
public:
	Reduce(NodeParam param);
	int minNumInputs() { return 1; }
	int minNumOutputs() { return 2; }
	void initForward();
	void initBackward();
	void forward();
	void backward();
protected:
	const float alpha = 1.0f;
	const float beta = 0.0f;
	cudnnHandle_t _cudnnHandle;
	cudnnReduceTensorOp_t _reduceTensorOp;
	cudnnReduceTensorDescriptor_t _reduceTensorDesciptor;
	cudnnReduceTensorIndices_t _reduceTensotIndices;
	float *_d_workspace;
	size_t _workspaceSizeInBytes;
};