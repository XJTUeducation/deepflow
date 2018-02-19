
#include "core/deep_flow.h"
#include "core/session.h"

#include <random>
#include <gflags/gflags.h>

DEFINE_string(faces, "C:/Projects/deepflow/data/celeba128", "Path to face dataset folder");
DEFINE_int32(channels, 3, "Image channels");
DEFINE_int32(size, 128, "Image size");
DEFINE_int32(total, 2000000, "Image channels"); // 13230
DEFINE_int32(batch, 20, "Batch size");
DEFINE_int32(debug, 0, "Level of debug");
DEFINE_int32(iter, 10000, "Maximum iterations");
DEFINE_string(load, "", "Load from gXXXX.bin and dXXXX.bin");
DEFINE_int32(save_image, 0, "Save image iter frequency (Don't Save = 0)");
DEFINE_int32(save_model, 0, "Save model iter frequency (Don't Save = 0)");

std::shared_ptr<Session> load_session(std::string suffix) {
	std::string filename = "s" + suffix + ".bin";
	std::cout << "Loading session from " << filename << std::endl;
	DeepFlow df;
	df.block()->load_from_binary(filename);
	return df.session();
}

std::string batchnorm(DeepFlow *df, std::string input, std::string solver, int output_channels, std::string name) {
	auto bns = df->variable(df->random_normal({ 1, output_channels, 1, 1 }, 1, 0.02), solver, name + "_bns");
	auto bnb = df->variable(df->fill({ 1, output_channels, 1, 1 }, 0), solver, name + "_bnb");
	return df->batch_normalization(input, bns, bnb, DeepFlow::SPATIAL, true, name + "_bn");
}

std::string dense(DeepFlow *df, std::string input, std::initializer_list<int> dims, std::string solver, bool has_batchnorm, std::string name) {
	auto w = df->variable(df->random_normal(dims, 0, 0.02), solver, name + "_w");
	auto node = df->matmul(input, w, name + "_m");
	if (has_batchnorm)
		node = batchnorm(df, node, solver, *(dims.begin()+1), name);		
	return node;
}

std::string deconv(DeepFlow *df, std::string input, std::string solver, int input_channels, int output_channels, int kernel, int pad, bool upsample, std::string name) {

	auto node = input;
	if (upsample)
		node = df->resize(node, 2, 2, name + "_resize");
	auto f = df->variable(df->random_normal({ output_channels, input_channels, kernel, kernel }, 0, 0.02), solver, name + "_f");
	node = df->conv2d(node, f, pad, pad, 1, 1, 1, 1, name + "_conv");
	node = batchnorm(df, node, solver, output_channels, name + "_bn");	
	//auto bnb = df->variable(df->fill({ 1, output_channels, 1, 1 }, 0), solver, name + "_bnb");
	//node = df->bias_add(node, bnb, name + "_bias");
	//node = df->lrn(node, name + "_lrn", 5, 1e-4, 0.5, 1);
	return node;
}

std::string to_rgb(DeepFlow *df, std::string input, std::string solver, int input_channels, std::string name) {
	auto node = input;
	auto f = df->variable(df->random_normal({ 3, input_channels, 3, 3 }, 0, 0.02), solver, name + "_f");
	node = df->conv2d(node, f, 1, 1, 1, 1, 1, 1, name + "_conv");
	auto bnb = df->variable(df->fill({ 1, 3, 1, 1 }, 0), solver, name + "_bnb");
	node = df->bias_add(node, bnb, name + "_bias");
	//node = df->tanh(node, name + "_tanh");
	return node;
}

std::string from_rgb(DeepFlow *df, std::string input, std::string solver, int output_channels, std::string name) {
	auto f = df->variable(df->random_normal({ output_channels, 3, 3, 3 }, 0, 0.02), solver, name + "_f");
	auto node = df->conv2d(input, f, 1, 1, 1, 1, 1, 1, name + "_conv");
	auto bnb = df->variable(df->fill({ 1, output_channels, 1, 1 }, 0), solver, name + "_bnb");
	node = df->bias_add(node, bnb, name + "_bias");
	return node;
}

std::string conv(DeepFlow *df, std::string input, std::string solver, int input_channels, int output_channels, int kernel, int pad, int stride, bool activation, std::string name) {
	auto f = df->variable(df->random_normal({ output_channels, input_channels, kernel, kernel }, 0, 0.02), solver, name + "_f");
	auto node = df->conv2d(input, f, pad, pad, stride, stride, 1, 1, name + "_conv");
	node = df->resize(node, 0.5, 0.5, name + "_ds");
	auto bnb = df->variable(df->fill({ 1, output_channels, 1, 1 }, 0), solver, name + "_bnb");
	node = df->bias_add(node, bnb, name + "_bias");
	if (activation) {
		return df->leaky_relu(node, 0.2, name + "_relu");
	}
	return node;
}

std::shared_ptr<Session> create() {
	DeepFlow df;

	int fn = 256;	
	
	auto g_solver = df.adam_solver(0.0002f, 0.0f, 0.5f, 10e-8f, "g_adam");	
	auto d_solver = df.adam_solver(0.0002f, 0.0f, 0.5f, 10e-8f, "d_adam");

	auto z = df.data_generator(df.random_normal({ FLAGS_batch, 100, 1, 1 }, 0, 1), FLAGS_total, "", "z");	
	auto face_data_128 = df.image_batch_reader(FLAGS_faces, { FLAGS_batch, FLAGS_channels, FLAGS_size, FLAGS_size }, false, "face_data_128");
	auto face_data_64 = df.resize(face_data_128, 0.5, 0.5, "face_data_64");
	auto face_data_32 = df.resize(face_data_64, 0.5, 0.5, "face_data_32");
	auto face_data_16 = df.resize(face_data_32, 0.5, 0.5, "face_data_16");
	auto face_data_8 = df.resize(face_data_16, 0.5, 0.5, "face_data_8");
	auto face_labels = df.data_generator(df.fill({ FLAGS_batch, 1, 1, 1 }, 1), FLAGS_total, "", "face_labels");
	auto generator_labels = df.data_generator(df.fill({ FLAGS_batch, 1, 1, 1 }, 0), FLAGS_total, "", "generator_labels");

	auto g4 = dense(&df, z, { 100, fn , 4 , 4 }, g_solver, true, "g4"); // 4x4	

	auto g8 = deconv(&df, g4, g_solver, fn, fn, 3, 1, 1, "g8"); // 8x8
	auto g8elu = df.leaky_relu(g8, 0.2, "g8elu");
	auto g8trgb = to_rgb(&df, g8, g_solver, fn, "g8trgb");
	auto im8 = df.switcher(g8trgb, "im8");
	df.imwrite(im8, "{it}", "imw8");

	auto g16 = deconv(&df, g8elu, g_solver, fn, fn, 3, 1, 1, "g16"); // 16x16
	auto g16elu = df.leaky_relu(g16, 0.2, "g16elu");
	auto g16trgb = to_rgb(&df, g16, g_solver, fn, "g16trgb");
	auto im16 = df.switcher(g16trgb, "im16");
	df.imwrite(im16, "{it}", "imw16");

	auto g32 = deconv(&df, g16elu, g_solver, fn, fn, 3, 1, 1, "g32"); // 32x32
	auto g32elu = df.leaky_relu(g32, 0.2, "g32elu");
	auto g32trgb = to_rgb(&df, g32, g_solver, fn, "g32trgb");
	auto im32 = df.switcher(g32trgb, "im32");
	df.imwrite(im32, "{it}", "imw32");

	auto g64 = deconv(&df, g32elu, g_solver, fn, fn, 3, 1, 1, "g64"); // 64x64
	auto g64elu = df.leaky_relu(g64, 0.2, "g64elu");
	auto g64trgb = to_rgb(&df, g64, g_solver, fn, "g64trgb");
	auto im64 = df.switcher(g64trgb, "im64");
	df.imwrite(im64, "{it}", "imw64");

	auto g128 = deconv(&df, g64elu, g_solver, fn, fn, 3, 1, 1, "g128"); // 64x64 -> 128x128
	auto g128trgb = to_rgb(&df, g128, g_solver, fn, "g128trgb");	
	auto im128 = df.switcher(g128trgb, "im128");
	df.imwrite(im128, "{it}", "imw128");

	auto m128 = df.multiplexer({ face_data_128, g128trgb }, "m128");
	auto m128frgb = from_rgb(&df, m128, d_solver, fn, "m128frgb");
	
	auto d128 = conv(&df, m128frgb, d_solver, fn, fn, 3, 1, 1, true, "d128"); // 128x128 -> 64x64
	auto m64 = df.multiplexer({ face_data_64, g64trgb }, "m64");
	auto m64frgb = from_rgb(&df, m64, d_solver, fn, "m64frgb");
	auto m642 = df.multiplexer({ d128, m64frgb }, "m642");

	auto d64 = conv(&df, m642, d_solver, fn, fn, 3, 1, 1, true, "d64"); // 64x64 -> 32x32
	auto m32 = df.multiplexer({ face_data_32, g32trgb }, "m32");
	auto m32frgb = from_rgb(&df, m32, d_solver, fn, "m32frgb");
	auto m322 = df.multiplexer({ d64, m32frgb }, "m322");

	auto d32 = conv(&df, m322, d_solver, fn, fn, 3, 1, 1, true, "d32"); // 32x32 -> 16x16
	auto m16 = df.multiplexer({ face_data_16, g16trgb }, "m16");
	auto m16frgb = from_rgb(&df, m16, d_solver, fn, "m16frgb");
	auto m162 = df.multiplexer({ d32, m16frgb }, "m162");

	auto d16 = conv(&df, m162, d_solver, fn, fn, 3, 1, 1, true, "d16"); // 16x16 -> 8x8
	auto m8 = df.multiplexer({ face_data_8, g8trgb }, "m8");
	auto m8frgb = from_rgb(&df, m8, d_solver, fn, "m8frgb");
	auto m82 = df.multiplexer({ d16, m8frgb }, "m82");

	auto d8 = dense(&df, m82, { fn * 8 * 8, 1, 1, 1 }, d_solver, false, "d8");

	//auto dout = df.sigmoid(d8, "dout");

	auto mloss = df.multiplexer({ face_labels, generator_labels }, "mloss");

	auto coef = df.place_holder({ 1, 1, 1, 1 }, Tensor::Float, "coef");
	std::string err;
	err = df.reduce_mean(df.square_error(d8, mloss));
	auto loss = df.loss(err, coef, DeepFlow::AVG, "loss");
	df.print({ loss }, "", DeepFlow::END_OF_EPOCH);

	return df.session();
}

void main(int argc, char** argv) {
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	CudaHelper::setOptimalThreadsPerBlock();

	auto execution_context = std::make_shared<ExecutionContext>();
	execution_context->debug_level = FLAGS_debug;

	std::shared_ptr<Session> session;
	if (FLAGS_load.empty())
		session = create();
	else
		session = load_session(FLAGS_load);
	session->initialize(execution_context);
	
	// m82, m162, m322, m642
	int main_multiplex_states[5][4] = {
		1, -1, -1, -1, // stage 0
		0,  1, -1, -1, // stage 1
		0,  0,  1, -1, // stage 2
		0,  0,  0,  1, // stage 3
		0,  0,  0,  0  // stage 4
	};

	std::shared_ptr<Multiplexer> main_multiplex[4] = {
		std::dynamic_pointer_cast<Multiplexer>(session->get_node("m82")),
		std::dynamic_pointer_cast<Multiplexer>(session->get_node("m162")),
		std::dynamic_pointer_cast<Multiplexer>(session->get_node("m322")),
		std::dynamic_pointer_cast<Multiplexer>(session->get_node("m642"))
	};

	std::shared_ptr<Multiplexer> per_stage_multiplex[6] = {
		std::dynamic_pointer_cast<Multiplexer>(session->get_node("m8")),
		std::dynamic_pointer_cast<Multiplexer>(session->get_node("m16")),
		std::dynamic_pointer_cast<Multiplexer>(session->get_node("m32")),
		std::dynamic_pointer_cast<Multiplexer>(session->get_node("m64")),
		std::dynamic_pointer_cast<Multiplexer>(session->get_node("m128")),
		std::dynamic_pointer_cast<Multiplexer>(session->get_node("mloss"))
	};
	
	std::shared_ptr<Switch> im_switches[5] = {
		std::dynamic_pointer_cast<Switch>(session->get_node("im8")),
		std::dynamic_pointer_cast<Switch>(session->get_node("im16")),
		std::dynamic_pointer_cast<Switch>(session->get_node("im32")),
		std::dynamic_pointer_cast<Switch>(session->get_node("im64")),
		std::dynamic_pointer_cast<Switch>(session->get_node("im128"))
	};

	auto loss_coef = session->get_node("coef");
	auto loss = session->get_node("loss");

	int iter = 1;	

	loss_coef->fill(1.0f);
	int stage;

	for (stage = 3; stage < 5; stage++) {
		for (iter = 1; iter <= FLAGS_iter && execution_context->quit != true; ++iter) {			
				execution_context->current_iteration = iter;
				stage = 4;

				std::cout << "Stage " << stage << " Iteration: [" << iter << "/" << FLAGS_iter << "]";
				for (int m = 0; m < 4; m++)
					main_multiplex[m]->selectInput(main_multiplex_states[stage][m]);

				for (int s = 0; s < 5; s++)
					im_switches[s]->setEnabled(false);

				for (int m = 0; m < 6; m++)
					per_stage_multiplex[m]->selectInput(-1);
				per_stage_multiplex[5]->selectInput(0);
				per_stage_multiplex[stage]->selectInput(0);

				session->forward();
				float d_loss = loss->output(0)->value()->toFloat();
				session->backward();

				for (int m = 0; m < 6; m++)
					per_stage_multiplex[m]->selectInput(-1);
				per_stage_multiplex[5]->selectInput(1);
				per_stage_multiplex[stage]->selectInput(1);

				session->forward();
				d_loss += loss->output(0)->value()->toFloat();
				session->backward();

				std::cout << " - d_loss: " << d_loss;

				session->apply_solvers({ "d_adam" });
				session->reset_gradients();

				if (FLAGS_save_image != 0 && iter % FLAGS_save_image == 0)
					im_switches[stage]->setEnabled(true);

				for (int m = 0; m < 6; m++)
					per_stage_multiplex[m]->selectInput(-1);
				per_stage_multiplex[5]->selectInput(0);
				per_stage_multiplex[stage]->selectInput(1);

				session->forward();
				float g_loss = loss->output(0)->value()->toFloat();
				session->backward();
				std::cout << " - g_loss: " << g_loss << std::endl;
				session->apply_solvers({ "g_adam" });
				session->reset_gradients();

				if (FLAGS_save_model != 0 && iter % FLAGS_save_model == 0) {
					session->save("model_s" + std::to_string(stage) + "_i" + std::to_string(iter) + ".bin");
				}
			}
		}
			
}

