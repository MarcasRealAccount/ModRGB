#pragma once

#include <vector>

namespace Graph::ACyclic
{
	struct Node
	{
	public:
		void addChild(Node* child);
		void removeChild(Node* child);

		auto& getParents() const { return m_Parents; }
		auto& getChildren() const { return m_Children; }

	private:
		std::vector<Node*> m_Parents;
		std::vector<Node*> m_Children;
	};

	struct Graph
	{
	public:
		auto getRoot() const { return m_Root; }
		void setRoot(Node* root) { m_Root = root; }

	private:
		Node* m_Root = nullptr;
	};
} // namespace Graph::ACyclic