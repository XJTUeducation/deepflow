#pragma once

#include "core/export.h"

#include <functional>
#include <memory>
#include <vector>

#include "cudnn.h"

#include "core/tensor.h"
#include "core/cuda_helper.h"
#include "core/execution_context.h"

#include <list>

#include <initializer_list>
#include "core/terminal.h"

#include <string>
#include <vector>

#include <iostream>

#include "proto/deepflow.pb.h"

class NodeObserver;

class DeepFlowDllExport Node : public std::enable_shared_from_this<Node>, public CudaHelper {
public:
	enum TraverseOrder {
		PRE_ORDER,
		POST_ORDER
	};
	enum ForwardType {
		ALWAYS_FORWARD,
		DEPENDS_ON_OUTPUTS,
		NEVER_FORWARD
	};
	enum BackwardType {
		ALWAYS_BACKWARD,
		DEPENDS_ON_INPUTS,
		NEVER_BACKWARD
	};
	Node(const deepflow::NodeParam &param);
	void createIO();
	virtual void initForward() = 0;
	virtual void initBackward() = 0;
	virtual int minNumInputs() = 0;
	virtual int minNumOutputs() = 0;
	virtual void forward() {}
	virtual void backward() {}
	virtual ForwardType forwardType() = 0;
	virtual BackwardType backwardType() = 0;
	virtual std::string to_cpp() const = 0;
	std::string name() const;	
	std::string _to_cpp_phases() const;
	void _unvisit();
	void _forward();
	void _backward();
	void _shouldForward();
	void _shouldBackward();
	bool shouldForward() const;
	bool shouldBackward() const;
	void setShouldForward(bool state);
	void setShouldBackward(bool state);
	void _traverse(std::function<void(Node*)> fun, TraverseOrder order, bool visit_condition);
	void setVisited(bool state);
	std::vector<NodeInputPtr> &inputs();
	std::vector<NodeOutputPtr> &outputs();
	NodeInputPtr input(int index);
	NodeOutputPtr output(int index);
	bool isInitialized() const;
	void setInitialized(bool status);
	deepflow::NodeParam &param();
	bool includePhase(const std::string &phase);
	void setExecutionContext(ExecutionContextPtr context);
	virtual std::list<std::shared_ptr<Node>> inputNodes() const;
	virtual std::list<std::shared_ptr<Node>> outputNodes() const;
protected:	
	std::vector<NodeInputPtr> _inputs;
	std::vector<NodeOutputPtr> _outputs;
	std::string _name;	
	bool _visited = false;
	bool __should_forward = false;
	bool __should_backward = false;
	bool _initialized = false;	
	deepflow::NodeParam _param;
	ExecutionContextPtr _context;
};

using NodePtr = std::shared_ptr<Node>;