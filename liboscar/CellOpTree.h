#ifndef LIBOSCAR_CELL_OP_TREE_H
#define LIBOSCAR_CELL_OP_TREE_H
#include <sserialize/strings/stringfunctions.h>
#include <sserialize/search/OpTree.h>
#include <sserialize/Static/CellTextCompleter.h>

namespace liboscar {
namespace detail {
namespace CellOpTree {

struct CTCStringHinter: sserialize::OpTree::detail::SetOpsOpTreeParser::StringHinter {
	CTCStringHinter(const sserialize::Static::CellTextCompleter & qc);
	~CTCStringHinter();
	virtual bool operator()(const std::string::const_iterator& begin, const std::string::const_iterator end) const;
	sserialize::Static::CellTextCompleter m_qc;
};
struct AlwaysFalseStringHinter: sserialize::OpTree::detail::SetOpsOpTreeParser::StringHinter {
	virtual bool operator()(const std::string::const_iterator& begin, const std::string::const_iterator end) const;
};

/**
  * The CellOpTree currently support the following special syntax:
  * STATEMENT = STATEMENT OP STATEMENT
  * OP = (as support by sserialize::OpTree::detail::SetOpsOpTreeParser)
  *   <space>
  *   /
  *   +
  *   -
  *   ^
  * STATEMENT =
  *   $region:<regionid> = query for region with id=regionid
  *   $cell:<cellid> = query for cell with id=cellid
  *   $rect:<sserialize::spatial::GeoRect> = define a rectangle 
  *   $path:radius,lat,lon,[lat,lon] = define a path
  *   STRING a region+items query
  *   !STRING a items query
  *   #STRING a region query
  * STRING = as defined by StringCompleter::normalize
  *
  */

template<typename T_CQR_TYPE = sserialize::CellQueryResult>
class CellOpTreeImp: public sserialize::OpTree::detail::OpTree<T_CQR_TYPE> {
public:
	typedef sserialize::OpTree::detail::OpTree<T_CQR_TYPE> MyBaseClass;
private:

private:
	sserialize::Static::CellTextCompleter m_qc;
	sserialize::OpTree::detail::SetOpsOpTreeParser m_parser;
	sserialize::OpTree::detail::Node * m_root;
private:
	T_CQR_TYPE qc(const std::string & str);
	T_CQR_TYPE calc(sserialize::OpTree::detail::Node * node);
public:
	CellOpTreeImp(const sserialize::Static::CellTextCompleter & qc, bool spacesAreIntersections);
	CellOpTreeImp(const CellOpTreeImp & other);
	virtual ~CellOpTreeImp() {
		delete m_root;
	}
	virtual bool parse(const std::string & query) override {
		m_root = m_parser.parse(query);
		return m_root;
	}
	virtual T_CQR_TYPE calc() override {
		return calc(m_root);
	}
	virtual MyBaseClass * copy() const override {
		return new CellOpTreeImp<T_CQR_TYPE>(*this);
	}
};


template<typename T_CQR_TYPE>
CellOpTreeImp<T_CQR_TYPE>::CellOpTreeImp(const CellOpTreeImp & other) :
m_qc(other.m_qc),
m_parser(other.m_parser),
m_root((other.m_root ? other.m_root->copy() : 0))
{}

template<typename T_CQR_TYPE>
CellOpTreeImp<T_CQR_TYPE>::CellOpTreeImp(const sserialize::Static::CellTextCompleter & qc, bool spacesAreIntersections) :
m_qc(qc),
m_parser(sserialize::OpTree::detail::SetOpsOpTreeParser::StringHinterSharedPtr(
			spacesAreIntersections ? 
				(sserialize::OpTree::detail::SetOpsOpTreeParser::StringHinter*) (new AlwaysFalseStringHinter()) :
				(sserialize::OpTree::detail::SetOpsOpTreeParser::StringHinter*) (new CTCStringHinter(qc))
		)
),
m_root(0)
{}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellOpTreeImp<T_CQR_TYPE>::qc(const std::string & str) {
	if (!str.size()) {
		return T_CQR_TYPE();
	}
	if (str[0] == '$') { //for internal purposes, inserted by the gui
		if (str.compare(1, sizeof("region")-1, "region") == 0) {//special query
			uint32_t id = atoi(str.c_str()+(sizeof("$region:")-1)); //-1 fo the terminating 0
			return m_qc.cqrFromRegionStoreId<T_CQR_TYPE>(id);
		}
		else if (str.compare(1, sizeof("cell")-1, "cell") == 0) {//special query
			uint32_t id = atoi(str.c_str()+(sizeof("$cell:")-1)); //-1 fo the terminating 0
			return m_qc.cqrFromCellId<T_CQR_TYPE>(id);
		}
		else if (str.compare(1, sizeof("geo")-1, "geo") == 0) {//special query
			std::string rectString(str.c_str()+(sizeof("$geo:")-1)); //-1 fo the terminating 0
			return m_qc.cqrFromRect<T_CQR_TYPE>(sserialize::spatial::GeoRect(rectString, true));
		}
		else if (str.compare(1, sizeof("path")-1, "path") == 0) {//special query, format is $path:radius,lat,lon,[lat,lon]
			std::vector<double> tmp;
			{
				struct MyOut {
					std::vector<double> * dest;
					MyOut & operator++() { return *this; }
					MyOut & operator*() { return *this; }
					MyOut & operator=(const std::string & str) {
						double t = atof(str.c_str());
						dest->push_back(t);
						return *this;
					}
					MyOut(std::vector<double> * d) : dest(d) {}
				};
				typedef sserialize::OneValueSet<uint32_t> MyS;
				sserialize::split<std::string::const_iterator, MyS, MyS, MyOut>(str.begin()+(sizeof("$path:")-1), str.end(), MyS(','), MyS('\\'), MyOut(&tmp));
			}
			if (tmp.size() < 3 || tmp.size() % 2 == 0) {
				return T_CQR_TYPE();
			}
			double radius(tmp[0]);
			if (tmp.size() == 5) {
				sserialize::spatial::GeoPoint startPoint(tmp[1], tmp[2]), endPoint(tmp[3], tmp[4]);
				return m_qc.cqrBetween<T_CQR_TYPE>(startPoint, endPoint, radius);
			}
			else {
				std::vector<sserialize::spatial::GeoPoint> gp;
				gp.reserve(tmp.size()/2);
				for(std::vector<double>::const_iterator it(tmp.begin()+1), end(tmp.end()); it != end; it += 2) {
					gp.emplace_back(*it, *(it+1));
				}
				return m_qc.cqrAlongPath<T_CQR_TYPE>(radius, gp.begin(), gp.end());
			}
		}
	}
	std::string qstr;
	if ('!' == str[0] || '#' == str[0]) {
		qstr.insert(qstr.end(), str.begin()+1, str.end());
	}
	else {
		qstr = str;
	}
	sserialize::StringCompleter::QuerryType qt = sserialize::StringCompleter::QT_NONE;
	qt = sserialize::StringCompleter::normalize(qstr);
	if ('!' == str[0]) {
		return m_qc.items<T_CQR_TYPE>(qstr, qt);
	}
	else if ('#' == str[0]) {
		return m_qc.regions<T_CQR_TYPE>(qstr, qt);
	}
	else {
		return m_qc.complete<T_CQR_TYPE>(qstr, qt);
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellOpTreeImp<T_CQR_TYPE>::calc(sserialize::OpTree::detail::Node * node) {
	if (node) {
		if (node->type == sserialize::OpTree::detail::Node::LEAF) {
			return qc(static_cast<sserialize::OpTree::detail::LeafNode*>(node)->q);
		}
		else if (node->type == sserialize::OpTree::detail::Node::OPERATION) {
			sserialize::OpTree::detail::OpNode* opNode = static_cast<sserialize::OpTree::detail::OpNode*>(node);
			uint32_t cs = node->children.size();
			if (!cs)
				return T_CQR_TYPE();
			if (cs == 1)
				return calc(opNode->children.front());
			switch(opNode->opType) {
			case sserialize::OpTree::detail::SetOpsOpTreeParser::OT_DIFF:
				return calc(opNode->children.front()) - calc(opNode->children.back());
			case sserialize::OpTree::detail::SetOpsOpTreeParser::OT_INTERSECT:
				return calc(opNode->children.front()) / calc(opNode->children.back());
			case sserialize::OpTree::detail::SetOpsOpTreeParser::OT_SYM_DIFF:
				return calc(opNode->children.front()) ^ calc(opNode->children.back());
			case sserialize::OpTree::detail::SetOpsOpTreeParser::OT_UNITE:
				return calc(opNode->children.front()) + calc(opNode->children.back());
			default:
				return T_CQR_TYPE();
			}
		}
	}
	return T_CQR_TYPE();
}

}}//end namespace detail::CellOpTree

template<typename T_CQR_TYPE = sserialize::CellQueryResult>
class CellOpTree: public sserialize::OpTree::OpTree<T_CQR_TYPE> {
public:
	typedef sserialize::OpTree::OpTree<T_CQR_TYPE> MyBaseClass;
public:
	CellOpTree(const sserialize::Static::CellTextCompleter& qc, bool spacesAreIntersections): 
	MyBaseClass(new detail::CellOpTree::CellOpTreeImp<T_CQR_TYPE>(qc, spacesAreIntersections))
	{}
	virtual ~CellOpTree() {}
};



}//end namespace

#endif