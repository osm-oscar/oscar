#ifndef LIBOSCAR_ADVANCED_CELL_OP_TREE_H
#define LIBOSCAR_ADVANCED_CELL_OP_TREE_H
#include <string>
#include <vector>
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/Static/CellTextCompleter.h>
#include <sserialize/strings/stringfunctions.h>
#include <sserialize/utility/assert.h>
#include "CQRDilator.h"
#include "CQRFromComplexSpatialQuery.h"

/** The AdvancedCellOpTree supports the following query language:
  *
  *
  * Q := UNARY_OP Q | DILATION_OP Q | COMPASS_OP Q | Q BETWEEN_OP Q | Q BINARY_OP Q | (Q) | Q Q | GEO_RECT | GEO_PATH | REGION | CELL
  * UNARY_OP := %
  * DILATION_OP := %NUMBER%
  * COMPASS_OP := :^ | :v | :> | :< | :north-of | :east-of | :south-of | :west-of
  * BETWEEN_OP := <->
  * BINARY_OP := - | + | / | ^ 
  * GEO_RECT := $geo[]
  * POLYGON := $poly[]
  * GEO_PATH := $path[]
  * REGION := $region:id
  * CELL := $cell:id
  */
namespace liboscar {
namespace detail {
namespace AdvancedCellOpTree {

struct Node {
	enum Type : int { UNARY_OP, BINARY_OP, LEAF};
	enum OpType : int {
		FM_CONVERSION_OP, DILATION_OP, COMPASS_OP,
		SET_OP, BETWEEN_OP,
		RECT, POLYGON, PATH, REGION, CELL, STRING
	};
	int baseType;
	int subType;
	std::string value;
	std::vector<Node*> children;
	Node() {}
	Node(int baseType, int subType, const std::string & value) : baseType(baseType), subType(subType), value(value) {}
	~Node() {
		for(Node* & n : children) {
			delete n;
			n = 0;
		}
	}
};

namespace parser {

struct Token {
	enum Type : int {
		//store chars in the lower 8 bits
		ENDOFFILE = 0,
		INVALID_TOKEN = 258,
		INVALID_CHAR,
		FM_CONVERSION_OP,
		DILATION_OP,
		COMPASS_OP,
		BETWEEN_OP,
		SET_OP,
		GEO_RECT,
		GEO_POLYGON,
		GEO_PATH,
		REGION,
		CELL,
		STRING
		
	};
	int type;
	std::string value;
	Token() : type(INVALID_TOKEN) {}
	Token(int type) : type(type) {}
};

class Tokenizer {
public:
	struct State {
		std::string::const_iterator it;
		std::string::const_iterator end;
	};
public:
	Tokenizer();
	Tokenizer(std::string::const_iterator begin, std::string::const_iterator end); 
	Tokenizer(const State & state);
	Token next();
private:
	static bool isWhiteSpace(char c);
	static bool isOperator(char c);
	static bool isScope(char c);
private:
	State m_state;
};

class Parser {
public:
	Parser();
	Node * parse(const std::string & str);
private:
	Token peek();
	bool eat(liboscar::detail::AdvancedCellOpTree::parser::Token::Type t);
	bool pop();
private:
	Node* parseUnaryOps();
	Node* parseSingleQ();
	Node* parseQ();
private:
	std::string m_str;
	Token m_prevToken;
	Token m_lastToken;
	Tokenizer m_tokenizer;
};

}}}//end namespace detail::AdvancedCellOpTree::parser

class AdvancedCellOpTree final {
public:
	typedef detail::AdvancedCellOpTree::Node Node;
public:
	AdvancedCellOpTree(const sserialize::Static::CellTextCompleter & ctc, const CQRDilator & cqrd, const CQRFromComplexSpatialQuery & csq);
	~AdvancedCellOpTree();
	void parse(const std::string & str);
	template<typename T_CQR_TYPE>
	T_CQR_TYPE calc();
private:
	struct CalcBase {
		CalcBase(sserialize::Static::CellTextCompleter & ctc, const CQRDilator & cqrd, const CQRFromComplexSpatialQuery & csq) :
		m_ctc(ctc),
		m_cqrd(cqrd),
		m_csq(csq)
		{}
		sserialize::Static::CellTextCompleter & m_ctc;
		const CQRDilator & m_cqrd;
		const CQRFromComplexSpatialQuery & m_csq;
		sserialize::ItemIndex calcBetweenOp(const sserialize::CellQueryResult & c1, const sserialize::CellQueryResult & c2);
		sserialize::ItemIndex calcCompassOp(Node * node, const sserialize::CellQueryResult & cqr);
	};

	template<typename T_CQR_TYPE>
	struct Calc: public CalcBase {
		typedef T_CQR_TYPE CQRType;
		Calc(sserialize::Static::CellTextCompleter & ctc, const CQRDilator & cqrd, const CQRFromComplexSpatialQuery & csq) :
		CalcBase(ctc, cqrd, csq)
		{}
		CQRType calc(Node * node);
		CQRType calcString(Node * node);
		CQRType calcRect(Node * node);
		CQRType calcPolygon(Node * node);
		CQRType calcPath(Node * node);
		CQRType calcRegion(Node * node);
		CQRType calcCell(Node * node);
		CQRType calcUnaryOp(Node * node);
		CQRType calcDilationOp(Node * node);
		CQRType calcCompassOp(Node * node);
		CQRType calcBinaryOp(Node * node);
		CQRType calcBetweenOp(Node * node);
	};
private:
	sserialize::Static::CellTextCompleter m_ctc;
	CQRDilator m_cqrd;
	CQRFromComplexSpatialQuery m_csq;
	Node * m_root;
};

template<typename T_CQR_TYPE>
T_CQR_TYPE
AdvancedCellOpTree::calc() {
	typedef T_CQR_TYPE CQRType;
	if (m_root) {
		Calc<CQRType> calculator(m_ctc, m_cqrd, m_csq);
		return calculator.calc( m_root );
	}
	else {
		return CQRType();
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE
AdvancedCellOpTree::Calc<T_CQR_TYPE>::calcRect(AdvancedCellOpTree::Node* node) {
	return m_ctc.cqrFromRect<CQRType>(sserialize::spatial::GeoRect(node->value, true));
}

template<typename T_CQR_TYPE>
T_CQR_TYPE
AdvancedCellOpTree::Calc<T_CQR_TYPE>::calcPolygon(AdvancedCellOpTree::Node* node) {
	//first construct the polygon out of the values
	std::vector<sserialize::spatial::GeoPoint> gps;
	{
		struct MyOut {
			std::vector<sserialize::spatial::GeoPoint> * dest;
			double m_firstCoord;
			MyOut & operator++() { return *this; }
			MyOut & operator*() { return *this; }
			MyOut & operator=(const std::string & str) {
				double t = ::atof(str.c_str());
				if (m_firstCoord == std::numeric_limits<double>::max()) {
					m_firstCoord = t;
				}
				else {
					dest->emplace_back(m_firstCoord, t);
					m_firstCoord = std::numeric_limits<double>::max();
				}
				return *this;
			}
			MyOut(std::vector<sserialize::spatial::GeoPoint> * d) : dest(d), m_firstCoord(std::numeric_limits<double>::max()) {}
		};
		typedef sserialize::OneValueSet<uint32_t> MyS;
		sserialize::split<std::string::const_iterator, MyS, MyS, MyOut>(node->value.begin(), node->value.end(), MyS(','), MyS('\\'), MyOut(&gps));
	}
	if (gps.size() < 3) {
		return T_CQR_TYPE();
	}
	//check if back and front are the same, if not, close the polygon:
	if (gps.back() != gps.front()) {
		gps.push_back(gps.front());
	}
	
	sserialize::ItemIndex res = m_csq.cqrfp().fullMatches(sserialize::spatial::GeoPolygon(std::move(gps)), CQRFromPolygon::AC_POLYGON_CELL_BBOX);
	return T_CQR_TYPE(res, m_ctc.geoHierarchy(), m_ctc.idxStore());
}

template<typename T_CQR_TYPE>
T_CQR_TYPE
AdvancedCellOpTree::Calc<T_CQR_TYPE>::calcPath(AdvancedCellOpTree::Node* node) {
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
		sserialize::split<std::string::const_iterator, MyS, MyS, MyOut>(node->value.begin(), node->value.end(), MyS(','), MyS('\\'), MyOut(&tmp));
	}
	if (tmp.size() < 3 || tmp.size() % 2 == 0) {
		return CQRType();
	}
	double radius(tmp[0]);
	if (tmp.size() == 5) {
		sserialize::spatial::GeoPoint startPoint(tmp[1], tmp[2]), endPoint(tmp[3], tmp[4]);
		return m_ctc.cqrBetween<CQRType>(startPoint, endPoint, radius);
	}
	else {
		std::vector<sserialize::spatial::GeoPoint> gp;
		gp.reserve(tmp.size()/2);
		for(std::vector<double>::const_iterator it(tmp.begin()+1), end(tmp.end()); it != end; it += 2) {
			gp.emplace_back(*it, *(it+1));
		}
		return m_ctc.cqrAlongPath<CQRType>(radius, gp.begin(), gp.end());
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE
AdvancedCellOpTree::Calc<T_CQR_TYPE>::calcRegion(AdvancedCellOpTree::Node * node) {
	uint32_t id = atoi(node->value.c_str()); //-1 fo the terminating 0
	return m_ctc.cqrFromRegionStoreId<CQRType>(id);
}

template<typename T_CQR_TYPE>
T_CQR_TYPE
AdvancedCellOpTree::Calc<T_CQR_TYPE>::calcCell(AdvancedCellOpTree::Node* node) {
	uint32_t id = atoi(node->value.c_str()); //-1 fo the terminating 0
	return m_ctc.cqrFromCellId<CQRType>(id);
}

template<typename T_CQR_TYPE>
T_CQR_TYPE
AdvancedCellOpTree::Calc<T_CQR_TYPE>::calcString(AdvancedCellOpTree::Node* node) {
	if (!node->value.size()) {
		return CQRType();
	}
	const std::string & str = node->value;
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
		return m_ctc.items<T_CQR_TYPE>(qstr, qt);
	}
	else if ('#' == str[0]) {
		return m_ctc.regions<T_CQR_TYPE>(qstr, qt);
	}
	else {
		return m_ctc.complete<T_CQR_TYPE>(qstr, qt);
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE
AdvancedCellOpTree::Calc<T_CQR_TYPE>::calcBinaryOp(AdvancedCellOpTree::Node* node) {
	SSERIALIZE_CHEAP_ASSERT_EQUAL((std::string::size_type)1, node->value.size());
	switch (node->value.front()) {
	case '+':
		return calc(node->children.front()) + calc(node->children.back());
	case '/':
	case ' ':
		return calc(node->children.front()) / calc(node->children.back());
	case '-':
		return calc(node->children.front()) - calc(node->children.back());
	case '^':
		return calc(node->children.front()) ^ calc(node->children.back());
	default:
		return CQRType();
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE
AdvancedCellOpTree::Calc<T_CQR_TYPE>::calcUnaryOp(AdvancedCellOpTree::Node* node) {
	SSERIALIZE_CHEAP_ASSERT_EQUAL((std::string::size_type)1, node->value.size());
	switch (node->value.front()) {
	case '%':
		return calc(node->children.front()).allToFull();
	default:
		return CQRType();
	}
}

template<>
sserialize::CellQueryResult
AdvancedCellOpTree::Calc<sserialize::CellQueryResult>::calcBetweenOp(AdvancedCellOpTree::Node* node);

template<>
sserialize::TreedCellQueryResult
AdvancedCellOpTree::Calc<sserialize::TreedCellQueryResult>::calcBetweenOp(AdvancedCellOpTree::Node* node);

template<>
sserialize::CellQueryResult
AdvancedCellOpTree::Calc<sserialize::CellQueryResult>::calcCompassOp(AdvancedCellOpTree::Node* node);

template<>
sserialize::TreedCellQueryResult
AdvancedCellOpTree::Calc<sserialize::TreedCellQueryResult>::calcCompassOp(AdvancedCellOpTree::Node* node);

template<>
sserialize::CellQueryResult
AdvancedCellOpTree::Calc<sserialize::CellQueryResult>::calcDilationOp(AdvancedCellOpTree::Node* node);

template<>
sserialize::TreedCellQueryResult
AdvancedCellOpTree::Calc<sserialize::TreedCellQueryResult>::calcDilationOp(AdvancedCellOpTree::Node* node);

template<typename T_CQR_TYPE>
T_CQR_TYPE
AdvancedCellOpTree::Calc<T_CQR_TYPE>::calc(AdvancedCellOpTree::Node* node) {
	if (!node) {
		return CQRType();
	}
	switch (node->baseType) {
	case Node::LEAF:
		switch (node->subType) {
		case Node::STRING:
			return calcString(node);
		case Node::REGION:
			return calcRegion(node);
		case Node::CELL:
			return calcCell(node);
		case Node::RECT:
			return calcRect(node);
		case Node::POLYGON:
			return calcPolygon(node);
		case Node::PATH:
			return calcPath(node);
		default:
			break;
		};
		break;
	case Node::UNARY_OP:
		switch(node->subType) {
		case Node::FM_CONVERSION_OP:
			return calcUnaryOp(node);
		case Node::DILATION_OP:
			return calcDilationOp(node);
		case Node::COMPASS_OP:
			return calcCompassOp(node);
		default:
			break;
		};
		break;
	case Node::BINARY_OP:
		switch(node->subType) {
		case Node::SET_OP:
			return calcBinaryOp(node);
		case Node::BETWEEN_OP:
			return calcBetweenOp(node);
		default:
			break;
		};
		break;
	default:
		break;
	};
	return CQRType();
}

}//end namespace

#endif