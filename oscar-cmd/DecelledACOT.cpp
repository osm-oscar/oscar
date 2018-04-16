#include "DecelledACOT.h"
#include <sserialize/utility/exceptions.h>

namespace oscar_cmd {

DecelledACOT::DecelledACOT(liboscar::AdvancedCellOpTree & tree) :
m_tree(tree),
m_calc(m_tree.ctc(), m_tree.cqrd(), m_tree.csq(), m_tree.ghsg(), 0)
{}

DecelledACOT::~DecelledACOT() {}

void DecelledACOT::prepare() {
	prepare(m_tree.root());
}

void DecelledACOT::clear() {
	m_cqrs.clear();
	m_idxs.clear();
}

void DecelledACOT::flaten(uint32_t threadCount) {
	for(auto & x : m_cqrs) {
		m_idxs[x.first] = x.second.flaten(threadCount);
	}
}

sserialize::ItemIndex DecelledACOT::execute() {
	return execute(m_tree.root());
}

sserialize::ItemIndex DecelledACOT::execute(Node * node) {
	if (!node) {
		return sserialize::ItemIndex();
	}
	switch (node->baseType) {
	case Node::LEAF:
		switch (node->subType) {
		case Node::STRING:
		case Node::STRING_REGION:
		case Node::STRING_ITEM:
		case Node::REGION:
		case Node::REGION_EXCLUSIVE_CELLS:
		case Node::CELL:
		case Node::CELLS:
		case Node::RECT:
		case Node::POLYGON:
		case Node::PATH:
		case Node::POINT:
		case Node::ITEM:
			return m_idxs.at(node);
		default:
			break;
		};
		break;
	case Node::UNARY_OP:
		switch(node->subType) {
		case Node::FM_CONVERSION_OP:
		case Node::CELL_DILATION_OP:
		case Node::REGION_DILATION_OP:
		case Node::COMPASS_OP:
		case Node::RELEVANT_ELEMENT_OP:
		case Node::QUERY_EXCLUSIVE_CELLS:
			throw sserialize::UnsupportedFeatureException("DecelledACOT: did you forget to call prepare?");
		default:
			break;
		};
		break;
	case Node::BINARY_OP:
		switch(node->subType) {
		case Node::SET_OP:
			switch (node->value.front()) {
			case '+':
				return execute(node->children.front()) + execute(node->children.back());
			case '/':
			case ' ':
				return execute(node->children.front()) / execute(node->children.back());
			case '-':
				return execute(node->children.front()) - execute(node->children.back());
			case '^':
				return execute(node->children.front()) ^ execute(node->children.back());
			default:
				return sserialize::ItemIndex();
			}
			break;
		case Node::BETWEEN_OP:
			throw sserialize::UnsupportedFeatureException("DecelledACOT: did you forget to call prepare?");
		default:
			break;
		};
		break;
	default:
		break;
	};
	return sserialize::ItemIndex();
}

void DecelledACOT::prepare(Node * node) {
	if (!node) {
		return;
	}
	switch (node->baseType) {
	case Node::LEAF:
		switch (node->subType) {
		case Node::STRING:
		case Node::STRING_REGION:
		case Node::STRING_ITEM:
			m_cqrs[node] = m_calc.calcString(node);
			break;
		case Node::REGION:
			m_cqrs[node] = m_calc.calcRegion(node);
			break;
		case Node::REGION_EXCLUSIVE_CELLS:
			m_cqrs[node] = m_calc.calcRegionExclusiveCells(node);
			break;
		case Node::CELL:
			m_cqrs[node] = m_calc.calcCell(node);
			break;
		case Node::CELLS:
			m_cqrs[node] = m_calc.calcCells(node);
			break;
		case Node::RECT:
			m_cqrs[node] = m_calc.calcRect(node);
			break;
		case Node::POLYGON:
			m_cqrs[node] = m_calc.calcPolygon(node);
			break;
		case Node::PATH:
			m_cqrs[node] = m_calc.calcPath(node);
			break;
		case Node::POINT:
			m_cqrs[node] = m_calc.calcPoint(node);
			break;
		case Node::ITEM:
			m_cqrs[node] = m_calc.calcItem(node);
			break;
		default:
			break;
		};
		break;
	case Node::UNARY_OP:
		switch(node->subType) {
		case Node::FM_CONVERSION_OP:
			throw sserialize::UnsupportedFeatureException("DecelledACOT: FM_CONVERSION_OP");
			break;
		case Node::CELL_DILATION_OP:
			throw sserialize::UnsupportedFeatureException("DecelledACOT: CELL_DILATION_OP");
			break;
		case Node::REGION_DILATION_OP:
			throw sserialize::UnsupportedFeatureException("DecelledACOT: REGION_DILATION_OP");
			break;
		case Node::COMPASS_OP:
			throw sserialize::UnsupportedFeatureException("DecelledACOT: COMPASS_OP");
			break;
		case Node::RELEVANT_ELEMENT_OP:
			throw sserialize::UnsupportedFeatureException("DecelledACOT: RELEVANT_ELEMENT_OP");
			break;
		case Node::QUERY_EXCLUSIVE_CELLS:
			throw sserialize::UnsupportedFeatureException("DecelledACOT: QUERY_EXCLUSIVE_CELLS");
			break;
		default:
			break;
		};
		break;
	case Node::BINARY_OP:
		switch(node->subType) {
		case Node::SET_OP:
			for(Node * child : node->children) {
				prepare(child);
			}
			break;
		case Node::BETWEEN_OP:
			throw sserialize::UnsupportedFeatureException("DecelledACOT: BETWEEN_OP");
			break;
		default:
			break;
		};
		break;
	default:
		break;
	};
}

}//end namespace liboscar
