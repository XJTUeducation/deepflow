#pragma once

#include "core/export.h"

#include "readers/mnist_reader.h"
#include "initializers/fill.h"
#include "initializers/index_fill.h"
#include "initializers/random_uniform.h"
#include "initializers/step.h"

#include "core/block.h"
#include "solvers/sgd_solver.h"
#include "solvers/gain_solver.h"

class DeepFlowDllExport DeepFlow {
	friend class ::Node;
public:
	// READERS
	std::shared_ptr<MNISTReader> mnist_reader(std::string folder_path, int batch_size, MNISTReaderType type, std::string name = "mnist", std::initializer_list<std::string> phases = {});

	// INITIALIZERS
	std::shared_ptr<Fill> fill(std::initializer_list<int> dims, float value, Tensor::TensorType type = Tensor::Float);
	std::shared_ptr<IndexFill> index_fill(std::initializer_list<int> dims, float offset, Tensor::TensorType type = Tensor::Float);
	std::shared_ptr<Fill> zeros(std::initializer_list<int> dims, Tensor::TensorType type = Tensor::Float);
	std::shared_ptr<Fill> ones(std::initializer_list<int> dims, Tensor::TensorType type = Tensor::Float);
	std::shared_ptr<RandomUniform> random_uniform(std::initializer_list<int> dims, float min, float max, Tensor::TensorType type = Tensor::Float);
	std::shared_ptr<Step> step(std::initializer_list<int> dims, float min, float max, Tensor::TensorType type = Tensor::Float);

	// VARIABLES & PLACE HOLDERS
	NodeOutputPtr variable(std::shared_ptr<Initializer> initializer, std::string name = "Variable", std::initializer_list<std::string> phases = {});
	NodeOutputPtr variable(std::shared_ptr<Initializer> initializer, int interval , std::string prefix, int perImageHeight, int perImageWidth ,  std::string name = "VariableWithSnapshot", std::initializer_list<std::string> phases = {});
	NodeOutputPtr place_holder(std::array<int,4> dims, Tensor::TensorType type = Tensor::Float, std::string name = "PlaceHolder", std::initializer_list<std::string> phases = {});

	// OPS
	NodeOutputPtr add(NodeOutputPtr a, NodeOutputPtr b, std::string name = "Add", std::initializer_list<std::string> phases = {});
	NodeOutputPtr bias_add(NodeOutputPtr a, NodeOutputPtr b, std::string name = "BiasAdd", std::initializer_list<std::string> phases = {});
	NodeOutputPtr subtract(NodeOutputPtr a, NodeOutputPtr b, std::string name = "Subtract", std::initializer_list<std::string> phases = {});
	NodeOutputPtr softmax(NodeOutputPtr a, std::string name = "Softmax", std::initializer_list<std::string> phases = {});
	NodeOutputPtr square(NodeOutputPtr a, std::string name = "Square", std::initializer_list<std::string> phases = {});
	NodeOutputPtr matmul(NodeOutputPtr a, NodeOutputPtr b, std::string name = "InnerProduct", std::initializer_list<std::string> phases = {});
	NodeOutputPtr relu(NodeOutputPtr a, float negative_slope = -0.01f, std::string name = "Relu", std::initializer_list<std::string> phases = {});
	NodeOutputPtr dropout(NodeOutputPtr a, float dropout = 0.5f, std::string name = "Dropout", std::initializer_list<std::string> phases = {});
	NodeOutputPtr conv2d(NodeOutputPtr input, NodeOutputPtr filter, int pad_top_bottom, int pad_left_right, int vertical_filter_stride, int horizontal_filter_stride, int filter_height_dilation, int filter_width_dialation , std::string name = "Conv", std::initializer_list<std::string> phases = {});
	NodeOutputPtr conv2d(NodeOutputPtr input, NodeOutputPtr filter, std::string name = "Conv", std::initializer_list<std::string> phases = {});
	NodeOutputPtr pooling(NodeOutputPtr input, int windowHeight = 3, int windowWidth = 3, int verticalPadding = 0, int horizontalPadding = 0, int verticalStride = 1, int horizontalStride = 1, std::string name = "MaxPool", std::initializer_list<std::string> phases = {});
	NodeOutputPtr argmax(NodeOutputPtr input, int reduceDimension, std::string name = "ReduceArgmax", std::initializer_list<std::string> phases = {});
	NodeOutputPtr reduce_max(NodeOutputPtr input, int reduceDimension, std::string name = "ReduceMax", std::initializer_list<std::string> phases = {});
	NodeOutputPtr equal(NodeOutputPtr a, NodeOutputPtr b, std::string name = "Equal", std::initializer_list<std::string> phases = {});
	NodeOutputPtr reduce_mean(NodeOutputPtr input, int reduceDimension, std::string name = "ReduceMean", std::initializer_list<std::string> phases = {});
	NodeOutputPtr reduce_sum(NodeOutputPtr input, int reduceDimension, std::string name = "ReduceSum", std::initializer_list<std::string> phases = {});

	// PHASE
	NodeOutputPtr phaseplexer(NodeOutputPtr input_1, std::string phase_1, NodeOutputPtr input_2, std::string phase_2, std::string name = "Phaseplexer", std::initializer_list<std::string> phases = {});

	// BLOCK
	std::shared_ptr<Block> block(std::initializer_list<NodeInputPtr> inputs, std::initializer_list<NodeOutputPtr> outputs, std::string name = "Block", std::initializer_list<std::string> phases = {});

	// LOSS
	NodeOutputPtr softmax_loss(NodeOutputPtr a, NodeOutputPtr b, std::string name = "SoftmaxWithLoss", std::initializer_list<std::string> phases = {});
		
	// SOLVERS
	std::shared_ptr<SGDSolver> sgd_solver(NodeOutputPtr loss, int max_iteration, float momentum, float learning_rate);
	std::shared_ptr<GainSolver> gain_solver(NodeOutputPtr loss, int max_iteration = 1000, float momentum = 0.99999, float learning_rate = 0.0001, float max_gain = 10, float min_gain = 0.1, float gain_plus = 0.05f, float gain_mult = 0.95f);
	
	// UTILITIES
	void global_node_initializer();
	void eval(NodeOutputPtr terminal, bool restart = true);
	std::tuple<float, float, int> run(NodeOutputPtr loss, NodeOutputPtr accuracy, std::map<NodeOutputPtr, NodeOutputPtr> feed);
	std::shared_ptr<Node> findNodeByName(const std::string &name) const;
	std::string getUniqueNodeName(const std::string &prefix) const;
	void save_as_binary(std::string filePath, bool include_inits);
	void save_as_text(std::string filePath, bool include_weights = false, bool include_inits = false);
	void define_phase(std::string phase, PhaseParam_PhaseBehaviour behaviour);
	std::shared_ptr<NetworkParam> createNetworkParam(bool include_weights, bool include_inits);
	void set_solver(std::shared_ptr<Solver> solver);
private:
	std::list<std::shared_ptr<Node>> _nodes;
	std::list<std::shared_ptr<Variable>> _variables;
	std::shared_ptr<Solver> _solver;
	std::map<std::string, PhaseParam_PhaseBehaviour> _phases;
};

