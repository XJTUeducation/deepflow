#include "nodes/image_reader.h"
#include "core/common_cu.h"
#include <opencv2/opencv.hpp>

__global__
void GrayImageReaderKernel(const unsigned char *in, const int width, const int height, float *out)
{
	int i = blockIdx.x*blockDim.x + threadIdx.x;
	if (i < width * height) {
		out[i] = (((float)in[i] / 255.0f) - 0.5f) * 2;
	}
}

__global__
void ColorImageReaderKernel(const unsigned char *in, const int width, const int height, float *out)
{
	int i = blockIdx.x*blockDim.x + threadIdx.x;
	if (i < width*height*3) {
		int channel = i % 3;
		int input_pixel = (i - channel) / 3;
		int input_col = input_pixel % width;
		int input_row = (input_pixel - input_col) / width;
		out[ (2 - channel) * width * height + input_row * width + input_col] = (((float)in[i] / 255.0f) - 0.5f) * 2;
	}
}


ImageReader::ImageReader(deepflow::NodeParam *param) : Node(param) {
	LOG_IF(FATAL, param->has_image_reader_param() == false) << "param.generator_param().has_image_reader_param() == false";
}

void ImageReader::init() {
	auto image_reader_param = _param->image_reader_param();
	auto file_name = image_reader_param.file_name();
	auto type = image_reader_param.type();
	if (type == deepflow::ImageReaderParam_Type_GRAY_ONLY)
		img = cv::imread(file_name, 0);
	else
		img = cv::imread(file_name);
	LOG_IF(FATAL, img.empty()) << "Image " << file_name << " does not exist.";
	_outputs[0]->initValue({ 1, img.channels(), img.rows , img.cols });
	size_t size = _outputs[0]->value()->size();
	unsigned char *d_img;
	DF_NODE_CUDA_CHECK(cudaMalloc(&d_img, size));
	DF_NODE_CUDA_CHECK(cudaMemcpy(d_img, img.ptr<uchar>(), size, cudaMemcpyHostToDevice));
	if (img.channels() == 1) {
		GrayImageReaderKernel << < numOfBlocks(size), maxThreadsPerBlock >> > (d_img, img.cols, img.rows, (float*)_outputs[0]->value()->gpu_data());
		DF_KERNEL_CHECK();
	}
	else if (img.channels() == 3) {
		ColorImageReaderKernel << < numOfBlocks(size), maxThreadsPerBlock >> > (d_img, img.cols, img.rows, (float*)_outputs[0]->value()->gpu_data());
		DF_KERNEL_CHECK();
	}
	else {
		LOG(FATAL) << "Unsupported image.";
	}
	DF_NODE_CUDA_CHECK(cudaFree(d_img));	
}

std::string ImageReader::to_cpp() const
{
	auto image_reader_param = _param->image_reader_param();
	auto file_name = image_reader_param.file_name();
	auto type = image_reader_param.type();	
	std::string cpp = "auto " + _name + " = df.image_reader(\"" + file_name + "\", ";
	if (type == deepflow::ImageReaderParam_Type_GRAY_ONLY)
		cpp += "ImageReaderParam_Type_GRAY_ONLY, ";
	else
		cpp += "ImageReaderParam_Type_COLOR_IF_AVAILABLE, ";
	cpp += "\"" + _name + "\");";	
	return cpp;
}
