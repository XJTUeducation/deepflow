#include "nodes/pooling.h"

Pooling::Pooling(deepflow::NodeParam *param) : Node(param) {
	LOG_IF(FATAL, param->has_pooling_param() == false) << "param.has_pooling_param() == false";
}

void Pooling::init() {
	DF_NODE_CUDNN_CHECK(cudnnCreate(&_cudnnHandle));
	auto param = _param->pooling_param();
	LOG_IF(FATAL, param.window_w() < 1);
	LOG_IF(FATAL, param.window_h() < 1);
	LOG_IF(FATAL, param.v_pad() < 0);
	LOG_IF(FATAL, param.h_pad() < 0);
	DF_NODE_CUDNN_CHECK(cudnnCreatePoolingDescriptor(&_poolingDesc));	
	DF_NODE_CUDNN_CHECK(cudnnSetPooling2dDescriptor(_poolingDesc, CUDNN_POOLING_MAX, CUDNN_PROPAGATE_NAN, param.window_h(), param.window_w(), param.v_pad(), param.h_pad(), param.v_stride(), param.h_stride()));
	int n, c, h, w;
	DF_NODE_CUDNN_CHECK(cudnnGetPooling2dForwardOutputDim(_poolingDesc, _inputs[0]->value()->descriptor(), &n, &c, &h, &w));
	_outputs[0]->initValue({ n, c, h, w });	
	_outputs[0]->initDiff();	
}

void Pooling::forward() {
	DF_NODE_CUDNN_CHECK(cudnnPoolingForward(_cudnnHandle, _poolingDesc, &one, _inputs[0]->value()->descriptor(), _inputs[0]->value()->gpu_data(), &zero, _outputs[0]->value()->descriptor(), _outputs[0]->value()->gpu_data()));
}

void Pooling::backward() {
	if (_inputs[0]->diff())
		DF_NODE_CUDNN_CHECK(cudnnPoolingBackward(_cudnnHandle, _poolingDesc, &one, _outputs[0]->value()->descriptor(), _outputs[0]->value()->gpu_data(), _outputs[0]->diff()->descriptor(), _outputs[0]->diff()->gpu_data(), _inputs[0]->value()->descriptor(), _inputs[0]->value()->gpu_data(), &zero, _inputs[0]->diff()->descriptor(), _inputs[0]->diff()->gpu_data()));
}

std::string Pooling::to_cpp() const
{
	auto param = _param->pooling_param();	
	std::string cpp = "auto " + _name + " = df.pooling(" + _input_name_for_cpp(0) + ", ";
	cpp += std::to_string(param.window_h()) + ", ";
	cpp += std::to_string(param.window_w()) + ", ";
	cpp += std::to_string(param.v_pad()) + ", ";
	cpp += std::to_string(param.h_pad()) + ", ";
	cpp += std::to_string(param.v_stride()) + ", ";
	cpp += std::to_string(param.h_stride()) + ", ";
	cpp += "\"" + _name + "\");";	
	return cpp;
}
