#include "initializers/three_state.h"

#include "nodes/variable.h"

ThreeState::ThreeState(const deepflow::InitParam & param) : Initializer(param)
{
	LOG_IF(FATAL, param.has_three_state_param() == false) << "param.has_three_state_param() == false";
}

void ThreeState::apply(Variable * variable)
{
	auto size = variable->output(0)->value()->size();
	std::uniform_int_distribution<int> distribution(0, 3);
	float *h_rand = new float[size];
	for (int i = 0; i < size; ++i) {
		int state = distribution(generator);
		if (state == 0)
			h_rand[i] = -1;
		else if (state == 1)
			h_rand[i] = 0;
		else 
			h_rand[i] = 1;
	}
	DF_CUDA_CHECK(cudaMemcpy((float*)variable->output(0)->value()->mutableData(), h_rand, variable->output(0)->value()->sizeInBytes(), cudaMemcpyHostToDevice));
	delete[] h_rand;
}

std::string ThreeState::to_cpp() const
{
	std::string cpp = "df.three_state(";
	cpp += "{" + std::to_string(_dims[0]) + ", " + std::to_string(_dims[1]) + ", " + std::to_string(_dims[2]) + ", " + std::to_string(_dims[3]) + "})";	
	return cpp;
}
