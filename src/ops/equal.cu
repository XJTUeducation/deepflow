#include "ops/equal.h"

__global__
void EqualKernel(int n, const int * __restrict__ a,  const int * __restrict__ b, float * __restrict__ c)
{
	int i = blockIdx.x*blockDim.x + threadIdx.x;
	if (i < n)
		c[i] = (a[i] == b[i] ? 1.0 : 0.0);
}


Equal::Equal(NodeParam param) : Node(param) {
	LOG_IF(FATAL, param.has_op_equal_param() == false) << "param.has_op_equal_param() == false";
}

void Equal::initForward() {
	LOG_IF(FATAL, _inputs[0]->value()->size() != _inputs[0]->value()->size()) << "Size mismatch [FAILED]";
	_outputs[0]->initValue(_inputs[0]->value()->dims(),_inputs[0]->value()->type());
	LOG(INFO) << "Initializing Equal (name: " << _name << " ) | Shape : " << _outputs[0]->value()->toString();
}

void Equal::initBackward() {

}

void Equal::forward() {
	auto size = _inputs[0]->value()->size();
	EqualKernel << < numOfBlocks(size), maxThreadsPerBlock >> >(size, (int*)_inputs[0]->value()->data(), (int*)_inputs[1]->value()->data(), (float*)_outputs[0]->value()->mutableData());
	LOG_IF(FATAL, cudaPeekAtLastError() != 0);
}

void Equal::backward() {

}