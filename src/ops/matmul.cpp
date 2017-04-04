#include "ops/matmul.h"

#include <glog/logging.h>

MatMul::MatMul(NodeParam param) : Node(param) {
	LOG_IF(FATAL, param.has_op_matmul_param() == false);
}

void MatMul::initForward() {	
	auto a = _inputs[0];
	auto b = _inputs[1];
	
	_alpha = _param.op_matmul_param().alpha();
	_beta = _param.op_matmul_param().beta();

	auto ad = a->value()->dims();
	auto bd = b->value()->dims();

	_row_A = ad[0];
	_col_A = ad[1] * ad[2] * ad[3];
	_row_B = bd[0];
	_col_B = bd[1] * bd[2] * bd[3];
	
	LOG_IF(FATAL, _col_A != _row_B) << "_col_A != _row_B - " << a->value()->toString() << " * " << b->value()->toString();
	
	_outputs[0]->initValue({ _row_A, _col_B, 1, 1 });

	LOG(INFO) << "Initializing MatMul (name: " << _name << " )  - " << _outputs[0]->value()->toString();	
	
	cublasCreate(&_handle);	
}

void MatMul::initBackward() {
	_outputs[0]->initDiff();
	LOG_IF(FATAL, cudaStreamCreate(&_stream[0]) != 0);
	LOG_IF(FATAL, cudaStreamCreate(&_stream[1]) != 0);
}

void MatMul::forward() {	
	auto a = _inputs[0];
	auto b = _inputs[1];
	auto c = _outputs[0];

	// C(row_A,col_B) = A(row_A,col_A) * B(row_B,col_B)
	LOG_IF(FATAL, cublasSgemm(_handle, CUBLAS_OP_N, CUBLAS_OP_N, _col_B, _row_A, _row_B, &_alpha, (float *) b->value()->data(), _col_B, (float *) a->value()->data(), _col_A, &_beta, (float*) c->value()->mutableData(), _col_B) != 0) << "cublasSgemm [FAILED]";	
}

void MatMul::backward() {			
	auto a = _inputs[0];
	auto b = _inputs[1];
	auto c = _outputs[0];

	// col_A = row_B
	//A(row_A,col_A) = diff(row_A,col_B) * B(row_B,col_B).T
	LOG_IF(FATAL, cublasSetStream_v2(_handle, _stream[0]) != 0);
	LOG_IF(FATAL, cublasSgemm(_handle, CUBLAS_OP_T, CUBLAS_OP_N, _row_B, _row_A, _col_B, &_alpha, (float*) b->value()->data(), _col_B, (float*) c->diff()->data(), _col_B, &_beta, (float*) a->diff()->mutableData(), _col_A) != 0);

	//B(row_B,col_B) = A(row_A,col_A).T * diff(row_A,col_B)	
	LOG_IF(FATAL, cublasSetStream_v2(_handle, _stream[1]) != 0);
	LOG_IF(FATAL, cublasSgemm(_handle, CUBLAS_OP_N, CUBLAS_OP_T, _col_B, _col_A, _row_A, &_alpha, (float *) c->diff()->data(), _col_B, (float *) a->value()->data(), _col_A, &_beta, (float*) b->diff()->mutableData(), _col_B) != 0);

	LOG_IF(FATAL, cublasSetStream_v2(_handle, 0) != 0);

	LOG_IF(FATAL, cudaDeviceSynchronize() != 0);
}