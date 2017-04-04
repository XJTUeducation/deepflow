#pragma once

#include "core/export.h"
#include "core/cuda_helper.h"
#include "core/tensor.h"
#include "proto/deepflow.pb.h"

class Variable;

class DeepFlowDllExport Initializer : public CudaHelper {
public:
	Initializer(InitParam param);
	Initializer(std::array<int, 4> dims, Tensor::TensorType type);
	virtual void init() = 0;
	virtual void apply(Variable *variable) = 0;	
	std::array<int, 4> dims();
protected:	
	InitParam _param;
	std::array<int, 4> _dims;
	Tensor::TensorType _type;
};
