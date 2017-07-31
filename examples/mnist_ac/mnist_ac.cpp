
#include "core/deep_flow.h"
#include "core/session.h"

#include <random>
#include <gflags/gflags.h>

DEFINE_string(mnist, "C:/Projects/deepflow/data/mnist", "Path to mnist folder dataset");
DEFINE_int32(batch, 100, "Batch size");
DEFINE_int32(debug, 0, "Level of debug");
DEFINE_int32(epoch, 10000, "Maximum epochs");
DEFINE_string(load, "", "Load from gXXXX.bin and dXXXX.bin");
DEFINE_int32(save, 0 , "Save model epoch frequency (Don't Save = 0)");
DEFINE_bool(train, false, "Train");
DEFINE_int32(bottleneck, 16, "Bottleneck Channel Size");

std::shared_ptr<Session> create_mnist_train_session() {
	DeepFlow df;
	df.mnist_reader(FLAGS_mnist, 100, MNISTReader::MNISTReaderType::Train, MNISTReader::Data, "mnist_data");
	return df.session();
}

std::shared_ptr<Session> create_mnist_test_session() {
	DeepFlow df;
	df.mnist_reader(FLAGS_mnist, 100, MNISTReader::MNISTReaderType::Test, MNISTReader::Data, "mnist_data");
	return df.session();
}

std::shared_ptr<Session> create_loss_session() {
	
	DeepFlow df;
	
	auto loss_t1 = df.place_holder({ 100, 1, 28, 28 }, Tensor::Float, "loss_t1");
	auto loss_t2 = df.place_holder({ 100, 1, 28, 28 }, Tensor::Float, "loss_t2");
	auto sqerr = df.square_error(loss_t1, loss_t2);
	auto loss = df.loss(sqerr, DeepFlow::AVG);

	df.print({ loss }, "Epoch: {ep} - Loss: {0}\n", DeepFlow::EVERY_PASS);

	return df.session();
}

std::shared_ptr<Session> create_display_session() {
	DeepFlow df;
	auto input = df.place_holder({ 100, 1, 28, 28 }, Tensor::Float, "input");
	df.display(input, 1, DeepFlow::EVERY_PASS, DeepFlow::VALUES, 1, "disp");
	return df.session();
}

std::shared_ptr<Session> create_encoder_session() {
	DeepFlow df;
	auto mean = 0;
	auto stddev = 0.01;
	auto enc_solver = df.adam_solver(0.0002f, 0.5f);
	auto enc_negative_slope = 0;
	auto alpha_param = 1.0f;
	auto decay = 0.01f;
	auto depth = 16;



	auto enc_input = df.place_holder({ 100, 1, 28, 28 }, Tensor::Float, "enc_input");

	auto conv1_w = df.variable(df.random_normal({ depth, 1, 3, 3 }, mean, stddev), enc_solver, "conv1_w");
	auto conv1 = df.conv2d(enc_input, conv1_w, 1, 1, 1, 1, 1, 1, "conv1");
	auto conv1_r = df.leaky_relu(conv1, enc_negative_slope);

	auto conv11_w = df.variable(df.random_normal({ depth * 2, depth, 3, 3 }, mean, stddev), enc_solver, "conv1_w");
	auto conv11 = df.conv2d(conv1_r, conv11_w, 1, 1, 1, 1, 1, 1, "conv1");
	auto conv11_r = df.leaky_relu(conv11, enc_negative_slope);

	auto conv1_p = df.pooling(conv11_r, 2, 2, 0, 0, 2, 2, "conv1_p");

	auto conv2_w = df.variable(df.random_normal({ depth * 4, depth * 2, 3, 3 }, mean, stddev), enc_solver, "conv2_w");
	auto conv2 = df.conv2d(conv1_p, conv2_w, 1, 1, 1, 1, 1, 1, "conv2");
	auto conv2_r = df.leaky_relu(conv2, enc_negative_slope);

	auto conv21_w = df.variable(df.random_normal({ depth * 8, depth * 4, 3, 3 }, mean, stddev), enc_solver, "conv2_w");
	auto conv21 = df.conv2d(conv2_r, conv21_w, 1, 1, 1, 1, 1, 1, "conv2");
	auto conv21_r = df.leaky_relu(conv21, enc_negative_slope);

	auto conv2_p = df.pooling(conv21_r, 2, 2, 0, 0, 2, 2, "conv2_p");

	auto conv3_w = df.variable(df.random_normal({ FLAGS_bottleneck, depth * 8, 3, 3 }, mean, stddev), enc_solver, "conv3_w");
	auto conv3 = df.conv2d(conv2_p, conv3_w, 1, 1, 1, 1, 1, 1, "conv3");
	auto conv3_r = df.leaky_relu(conv3, enc_negative_slope);
	auto conv3_p = df.pooling(conv3_r, 2, 2, 1, 1, 2, 2, "enc_output");

	return df.session();
}

std::shared_ptr<Session> create_decoder_session() {

	DeepFlow df;
	auto mean = 0;
	auto stddev = 0.02;	
	auto dec_solver = df.adam_solver(0.0002f, 0.5f);
	auto dec_negative_slope = 0.05;		
	auto alpha_param = 1.0f;
	auto decay = 0.01f;
	auto depth = 32;

	auto dec_input = df.place_holder({ 100, FLAGS_bottleneck, 4, 4 }, Tensor::Float, "dec_input");

	auto tconv1_f = df.variable(df.random_normal({ FLAGS_bottleneck, depth, 2, 2 }, mean, stddev), dec_solver, "tconv1_f");
	auto tconv1_t = df.transposed_conv2d(dec_input, tconv1_f, 1, 1, 2, 2, 1, 1, "tconv1_t");
	auto tconv1_b = df.batch_normalization(tconv1_t, DeepFlow::PER_ACTIVATION, 0, 1, 0, alpha_param, 1 - decay);
	auto tconv1_r = df.leaky_relu(tconv1_b, dec_negative_slope);

	auto tconv2_f = df.variable(df.random_normal({ depth, depth, 3, 3 }, mean, stddev), dec_solver, "tconv2_f");
	auto tconv2_t = df.transposed_conv2d(tconv1_r, tconv2_f, 1, 1, 2, 2, 1, 1, "tconv2_t");
	auto tconv2_b = df.batch_normalization(tconv2_t, DeepFlow::PER_ACTIVATION, 0, 1, 0, alpha_param, 1 - decay);
	auto tconv2_r = df.leaky_relu(tconv2_b, dec_negative_slope);

	auto tconv3_f = df.variable(df.random_normal({ depth, 1, 9, 9 }, mean, stddev), dec_solver, "tconv3_f");
	auto tconv3_t = df.transposed_conv2d(tconv2_r, tconv3_f, 4, 4, 2, 2, 1, 1, "tconv3_t");

	auto output = df.tanh(tconv3_t, "dec_output");	

	return df.session();
}

void main(int argc, char** argv) {
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	CudaHelper::setOptimalThreadsPerBlock();	
	
	auto execution_context = std::make_shared<ExecutionContext>();
	execution_context->debug_level = FLAGS_debug;	

	std::shared_ptr<Session> encoder_session;
	encoder_session = create_encoder_session();
	encoder_session->initialize();
	encoder_session->set_execution_context(execution_context);

	std::shared_ptr<Session> decoder_session;
	decoder_session = create_decoder_session();
	decoder_session->initialize();
	decoder_session->set_execution_context(execution_context);

	std::shared_ptr<Session> mnist_train_session = create_mnist_train_session();
	mnist_train_session->initialize();
	mnist_train_session->set_execution_context(execution_context);

	std::shared_ptr<Session> mnist_test_session = create_mnist_test_session();
	mnist_test_session->initialize();
	mnist_test_session->set_execution_context(execution_context);

	std::shared_ptr<Session> loss_session = create_loss_session();
	loss_session->initialize();
	loss_session->set_execution_context(execution_context);
	
	auto display_session = create_display_session();
	display_session->initialize();
	display_session->set_execution_context(execution_context);
	
	auto mnist_train_data = mnist_train_session->get_node("mnist_data");
	auto mnist_test_data = mnist_test_session->get_node("mnist_data");
	auto enc_input = encoder_session->get_node("enc_input");
	auto enc_output = encoder_session->get_node("enc_output");
	auto dec_input = decoder_session->get_node("dec_input");
	auto dec_output = decoder_session->get_node("dec_output");
	auto disp_input = display_session->get_node("input");

	auto loss_t1 = loss_session->get_node("loss_t1");
	auto loss_t2 = loss_session->get_node("loss_t2");

	int epoch;
	for (epoch = 1; epoch <= FLAGS_epoch && execution_context->quit != true; ++epoch) {
		
		execution_context->current_epoch = epoch;
	
		mnist_train_session->forward();
		enc_input->write_values(mnist_train_data->output(0)->value());
		encoder_session->forward();
		dec_input->write_values(enc_output->output(0)->value());
		decoder_session->forward();
		loss_session->reset_gradients();
		loss_t1->write_values(dec_output->output(0)->value());
		loss_t2->write_values(mnist_train_data->output(0)->value());
		loss_session->forward();
		loss_session->backward();
		decoder_session->reset_gradients();
		dec_output->write_diffs(loss_t1->output(0)->diff());
		decoder_session->backward();
		decoder_session->apply_solvers();
		encoder_session->reset_gradients();
		enc_output->write_diffs(dec_input->output(0)->diff());
		encoder_session->backward();
		encoder_session->apply_solvers();

		mnist_test_session->forward();
		enc_input->write_values(mnist_test_data->output(0)->value());
		encoder_session->forward();
		dec_input->write_values(enc_output->output(0)->value());
		decoder_session->forward();
		disp_input->write_values(dec_output->output(0)->value());
		display_session->forward();

	}
	
}

