#pragma once

#include "core/node_observer.h"

class DeepFlowDllExport ForwardObserver : public NodeObserver {
public:
	void apply(std::shared_ptr<Node> node);
};