#include "ACyclic.h"

namespace Graph::ACyclic
{
	void Node::addChild(Node* child)
	{
		m_Children.push_back(child);
		child->m_Parents.push_back(this);
	}

	void Node::removeChild(Node* child)
	{
		if (std::erase(m_Children, child) != 0)
			std::erase(child->m_Parents, this);
	}
} // namespace Graph::ACyclic