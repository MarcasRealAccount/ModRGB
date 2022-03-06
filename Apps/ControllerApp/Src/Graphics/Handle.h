#pragma once

#include "Graph/Acyclic/ACyclic.h"

namespace Graphics
{
	template <class T, bool Debuggable, bool Destructible>
	struct Handle;

	template <class T, bool Debuggable>
	struct Handle<T, Debuggable, true> : public Graph::ACyclic::Node
	{
	};

	template <class T, bool Debuggable>
	struct Handle<T, Debuggable, false> : public Graph::ACyclic::Node
	{
	};
} // namespace Graphics