#include "GeoHierarchySubSetSerializer.h"
#include <sserialize/utility/printers.h>
#include <sserialize/iterator/TransformIterator.h>
#include <cppcms/util.h>
#include <cppcms/json.h>
#include <stack>
#include <queue>

namespace oscar_web {

template<typename T_UINT_IT>
void printUintArray(std::ostream& out, T_UINT_IT begin, T_UINT_IT end) {
	out << "[";
	sserialize::print<','>(out, begin, end);
	out << "]";
}

void GeoHierarchySubSetSerializer::toTreeXML(std::ostream & out, const sserialize::Static::spatial::detail::SubSet::NodePtr & node) const {
	out << "<region id=\"" << m_gh.ghIdToStoreId(node->ghId()) << "\" apxitems=\"" << node->maxItemsSize() << "\">";
	out << "<cellpositions>";
	printUintArray(out, node->cellPositions().cbegin(), node->cellPositions().cend());
	out << "</cellpositions>";
	if (node->size()) {
		out << "<children>";
		for(const auto & child : *node) {
			toTreeXML(out, child);
		}
		out << "</children>";
	}
	out << "</region>";
}


std::ostream& GeoHierarchySubSetSerializer::toFlatXML(std::ostream& out, const sserialize::Static::spatial::GeoHierarchy::SubSet& subSet) const {
	std::unordered_set<uint32_t> writtenRegions;
	out << "<oscarresponse><geohierarchy><subset type=\"" << (subSet.sparse() ? "sparse" : "full") << "\" rep=\"flat\">";
	if (subSet.root().priv()) {
		std::queue<sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr> nodes;
		nodes.push(subSet.root());
		const sserialize::Static::spatial::GeoHierarchy * gh = &m_gh;
		auto childToStoreIdDerfer = [gh](const sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr & n){ return gh->ghIdToStoreId(n->ghId()); };
		while (nodes.size()) {
			sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr node = nodes.front();
			nodes.pop();
			out << "<region id=\"" << m_gh.ghIdToStoreId(node->ghId()) << "\" apxitems=\"" << node->maxItemsSize() << "\">";
			out << "<cellpositions>";
			printUintArray(out, node->cellPositions().cbegin(), node->cellPositions().cend());
			out << "</cellpositions>";
			if (node->size()) {
				out << "<children>";
				typedef sserialize::TransformIterator<decltype(childToStoreIdDerfer), uint32_t, sserialize::Static::spatial::GeoHierarchy::SubSet::Node::ChildrenStorageContainer::const_iterator> ChildIdIterator;
				printUintArray(out, ChildIdIterator(childToStoreIdDerfer, node->cbegin()), ChildIdIterator(childToStoreIdDerfer, node->cend()));
				out << "</children>";
			}
			out << "</region>";
			for(const sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr & child : *node) {
				if (!writtenRegions.count(child->ghId())) {
					writtenRegions.insert(child->ghId());
					nodes.push(child);
				}
			}
		}
	}
	out << "</subset></geohierarchy></oscarresponse>";
	return out;
}

std::ostream & GeoHierarchySubSetSerializer::toTreeXML(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::SubSet & subSet) const {
	out << "<oscarresponse><geohierarchy><subset type=\"" << (subSet.sparse() ? "sparse" : "full") << "\" rep=\"tree\">";
	toTreeXML(out, subSet.root());
	out << "</subset></geohierarchy></oscarresponse>";
	return out;
}

std::ostream & GeoHierarchySubSetSerializer::toJson(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::SubSet & subSet) const {
	std::unordered_set<uint32_t> writtenRegions;
	out << "{\"type\":\""  << (subSet.sparse() ? "sparse" : "full") << "\"";
	if (subSet.root().priv() && subSet.root()->size()) {
		std::queue<sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr> nodes;
		out << ",\"rootchildren\":";
		char regionSeparator = '[';
		for(const sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr & child : *subSet.root()) {;
			out << regionSeparator;
			regionSeparator = ',';
			out << m_gh.ghIdToStoreId(child->ghId());
			nodes.push(child);
		}
		out << "],\"regions\":";
		regionSeparator = '{';
		const sserialize::Static::spatial::GeoHierarchy * gh = &m_gh;
		auto childToStoreIdDerfer = [gh](const sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr & n){ return gh->ghIdToStoreId(n->ghId()); };
		while (nodes.size()) {
			sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr node = nodes.front();
			nodes.pop();
			out << regionSeparator;
			regionSeparator = ','; //should be faster than a branch
			out << "\"" << m_gh.ghIdToStoreId(node->ghId()) << "\":{\"apxitems\":" << node->maxItemsSize() << ",";
			out << "\"cellpositions\":";
			printUintArray(out, node->cellPositions().cbegin(), node->cellPositions().cend());
			if (node->size()) {
				out << ",\"children\":";
				typedef sserialize::TransformIterator<decltype(childToStoreIdDerfer), uint32_t, sserialize::Static::spatial::GeoHierarchy::SubSet::Node::ChildrenStorageContainer::const_iterator> ChildIdIterator;
				printUintArray(out, ChildIdIterator(childToStoreIdDerfer, node->cbegin()), ChildIdIterator(childToStoreIdDerfer, node->cend()));
			}
			out << "}";
			for(const sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr & child : *node) {
				if (!writtenRegions.count(child->ghId())) {
					writtenRegions.insert(child->ghId());
					nodes.push(child);
				}
			}
		}
		out << "}";
	}
	else {
		out << ",\"rootchildren\":[],\"regions\":{}";
	}
	out << "}";
	return out;
}

std::ostream & GeoHierarchySubSetSerializer::toBinary(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::SubSet & subSet) const {
	std::unordered_set<uint32_t> writtenRegions;
	char buf[sizeof(uint32_t)];
	auto writeU32 = [&buf, &out](uint32_t s) {
		s = htole32(s);
		memmove(buf, &s, sizeof(uint32_t));
		out.write(buf, sizeof(uint32_t));
	};
	out << (subSet.sparse() ? 's' : 'f');
	if (subSet.root().priv()) {
		std::queue<sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr> nodes;
		writeU32(subSet.root()->size());
		for(const sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr & child : *subSet.root()) {
			writeU32(m_gh.ghIdToStoreId(child->ghId()));
			nodes.push(child);
		}
		while (nodes.size()) {
			sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr node = nodes.front();
			nodes.pop();
			writeU32(m_gh.ghIdToStoreId(node->ghId()));
			writeU32(node->maxItemsSize());
			writeU32(node->cellPositions().size());
			writeU32(node->size());
			for(auto it(node->cellPositions().cbegin()), end(node->cellPositions().cend()); it != end; ++it) {
				writeU32(*it);
			}
			for(auto it(node->cbegin()), end(node->cend()); it != end; ++it) {
				writeU32(m_gh.ghIdToStoreId((*it)->ghId()));
			}
			for(const sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr & child : *node) {
				if (!writtenRegions.count(child->ghId())) {
					writtenRegions.insert(child->ghId());
					nodes.push(child);
				}
			}
		}
	}
	else { //no root children
		writeU32(0);
	}
	return out;
}

}//end namespace