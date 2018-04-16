#pragma once
#ifndef OSCAR_CMD_DECELLED_ACOT_H
#define OSCAR_CMD_DECELLED_ACOT_H
#include <liboscar/AdvancedCellOpTree.h>


namespace oscar_cmd {
	
class DecelledACOT final {
public:
	using Node = liboscar::detail::AdvancedCellOpTree::Node;
public:
	DecelledACOT(liboscar::AdvancedCellOpTree & tree);
	DecelledACOT(const DecelledACOT &) = delete;
	~DecelledACOT();
	void clear();
public:
	///retrieve indexes of leaf nodes
	///throws UnsupportedFeatureException if tree contains queries currently not supported
	void prepare();
	///flattens the cqrs of leaf nodes
	void flaten(uint32_t threadCount);
	///execute set operations
	sserialize::ItemIndex execute();
private:
	void prepare(Node * node);
	sserialize::ItemIndex execute(Node * node);
private:
	liboscar::AdvancedCellOpTree & m_tree;
	liboscar::AdvancedCellOpTree::Calc<sserialize::CellQueryResult> m_calc;
	std::unordered_map<Node*, sserialize::CellQueryResult> m_cqrs;
	std::unordered_map<Node*, sserialize::ItemIndex> m_idxs;
};
	
}//end namespace liboscar

#endif
