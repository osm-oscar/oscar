#ifndef LIBOSCAR_ADVANCED_CELL_OP_TREE_H
#define LIBOSCAR_ADVANCED_CELL_OP_TREE_H
#include <string>
#include <vector>
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/Static/CellTextCompleter.h>
#include <sserialize/strings/stringfunctions.h>

/** The AdvancedCellOpTree supports the following query language:
  *
  *
  * Q := UNARY_OP Q | Q BINARY_OP Q | (Q) | Q Q | GEO_RECT | GEO_PATH | REGION | CELL
  * UNARY_OP := %
  * BINARY_OP := - | + | / | ^
  * GEO_RECT := $geo[]
  * GEO_PATH := $path[]
  * REGION := $region:id
  * CELL := $cell:id
  */
namespace liboscar {
namespace detail {
namespace AdvancedCellOpTree {

struct Node {
	enum Type : int { UNARY_OP, BINARY_OP, RECT, PATH, REGION, CELL, STRING};
	int type;
	std::string value;
	std::vector<Node*> children;
	Node() {}
	Node(int type, const std::string & value) : type(type), value(value) {}
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
		UNARY_OP,
		BINARY_OP,
		GEO_RECT,
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
	static bool isSeperator(char c);
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
	Node* parseSingleQ();
	Node* parseQ();
private:
	std::string m_str;
	Token m_prevToken;
	Token m_lastToken;
	Tokenizer m_tokenizer;
};

}}}//end namespace detail::AdvancedCellOpTree::parser

class AdvancedCellOpTree {
public:
	typedef detail::AdvancedCellOpTree::Node Node;
public:
	AdvancedCellOpTree(const sserialize::Static::CellTextCompleter & ctc);
	~AdvancedCellOpTree();
	void parse(const std::string & str);
	template<typename T_CQR_TYPE>
	T_CQR_TYPE calc();
private:
	template<typename T_CQR_TYPE>
	struct Calc {
		typedef T_CQR_TYPE CQRType;
		Calc(sserialize::Static::CellTextCompleter & ctc) : m_ctc(ctc) {}
		sserialize::Static::CellTextCompleter & m_ctc;
		CQRType calc(Node * node);
		CQRType calcString(Node * node);
		CQRType calcRect(Node * node);
		CQRType calcPath(Node * node);
		CQRType calcRegion(Node * node);
		CQRType calcCell(Node * node);
		CQRType calcUnaryOp(Node * node);
		CQRType calcBinaryOp(Node * node);
	};
private:
	sserialize::Static::CellTextCompleter m_ctc;
	Node * m_root;
};

template<typename T_CQR_TYPE>
T_CQR_TYPE
AdvancedCellOpTree::calc() {
	typedef T_CQR_TYPE CQRType;
	if (m_root) {
		Calc<CQRType> calculator( m_ctc );
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

template<typename T_CQR_TYPE>
T_CQR_TYPE
AdvancedCellOpTree::Calc<T_CQR_TYPE>::calc(AdvancedCellOpTree::Node* node) {
	if (!node) {
		return CQRType();
	}
	switch (node->type) {
	case Node::STRING:
		return calcString(node);
	case Node::RECT:
		return calcRect(node);
	case Node::REGION:
		return calcRegion(node);
	case Node::PATH:
		return calcPath(node);
	case Node::UNARY_OP:
		return calcUnaryOp(node);
	case Node::BINARY_OP:
		return calcBinaryOp(node);
	default:
		return CQRType();
	};
}

}//end namespace

#endif