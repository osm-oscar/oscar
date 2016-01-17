#include "AdvancedCellOpTree.h"

namespace liboscar {
namespace detail {
namespace AdvancedCellOpTree {
namespace parser {

Tokenizer::Tokenizer() {}

Tokenizer::Tokenizer(const Tokenizer::State& state) :
m_state(state)
{}

Tokenizer::Tokenizer(std::string::const_iterator begin, std::string::const_iterator end) {
	m_state.it = begin;
	m_state.end = end;
}

bool Tokenizer::isSeperator(char c) {
	return (c == ' ' || c == '\t' || //white space
			c == '(' || c == ')' || //braces open new queries
			c == '+' || c == '-' || c == '/' || c == '%' || c == '^'); //operators
}


//TODO:use ragel to parse?
Token Tokenizer::next() {
	if (m_state.it == m_state.end) {
		return Token(Token::ENDOFFILE);
	}
	Token t;
	while (m_state.it != m_state.end) {
		switch (*m_state.it) {
		case '%':
		{
			t.type = Token::UNARY_OP;
			t.value += *m_state.it;
			++m_state.it;
			//check if theres a number afterwards, because then this is dilation operation
			if (m_state.it != m_state.end && ('0' <= *m_state.it && '9' >= *m_state.it)) {
				for(auto it(m_state.it); it != m_state.end; ++it) {
					if ('0' > *it || '9' < *it) {
						if (*it == '%') {
							t.type = Token::DILATION_OP;
							t.value.insert(t.value.end(), m_state.it, it);
							m_state.it = it;
						}
						break;
					}
				}
			}
			return t;
		}
		case '+':
		case '-':
		case '/':
		case '^':
			t.type = Token::BINARY_OP;
			t.value += *m_state.it;
			++m_state.it;
			return t;
		case ' ': //ignore whitespace
		case '\t':
			++m_state.it;
			break;
		case '(':
		case ')':
			t.type = *m_state.it;
			t.value += *m_state.it;
			++m_state.it;
			return t;
		case '$': //parse a region/cell/geo/path query
		{
			//read until the first occurence of :
			std::string tmp;
			for(++m_state.it; m_state.it != m_state.end;) {
				if (*m_state.it == ':') {
					++m_state.it;
					break;
				}
				else {
					tmp += *m_state.it;
					++m_state.it;
				}
			}
			if (tmp == "region") {
				t.type = Token::REGION;
			}
			else if (tmp == "cell") {
				t.type = Token::CELL;
			}
			else if (tmp == "geo") {
				t.type = Token::GEO_RECT;
			}
			else if (tmp == "path") {
				t.type = Token::GEO_PATH;
			}
			for(; m_state.it != m_state.end;) {
				if (isSeperator(*m_state.it)) {
					break;
				}
				else {
					t.value += *m_state.it;
					++m_state.it;
				}
			}
			return t;
			break;
		}
		default: //read as normal string
		{
			t.type = Token::STRING;
			for(; m_state.it != m_state.end;) {
				char c = *m_state.it;
				if (c == '\\') { //next char is escaped
					++m_state.it;
					if (m_state.it != m_state.end) {
						t.value += *m_state.it;
						++m_state.it;
					}
				}
				else if (isSeperator(c)) {
					break;
				}
				else {
					t.value += c;
					++m_state.it;
				}
			}
			return t;
			break;
		}
		};
	}
	return t;
}

bool Parser::pop() {
	if (-1 == m_lastToken.type) {
		throw std::runtime_error("pop() without peek()");
	}
	m_prevToken = m_lastToken;
	// stop at EOF, otherwise consume it
	if (Token::ENDOFFILE != m_lastToken.type) {
		m_lastToken.type = -1;
	}
	return true;
}

//dont turn return type to reference or const reference since this function may be called while tehre ist still a reference on the return value
Token Parser::peek() {
	if (-1 == m_lastToken.type) {
		m_lastToken = m_tokenizer.next();
	}
	return m_lastToken;
}

bool Parser::eat(Token::Type t) {
	if (t != peek().type) {
		return false;
	}
	pop();
	return true;
}

//parses a Single query like STRING, REGION, CELL, GEO_RECT, GEO_PATH
//calls parseQ() on opening a new SCOPE
detail::AdvancedCellOpTree::Node* Parser::parseSingleQ() {
	Token t = peek();
	switch (t.type) {
	case '(': //opens a new query
	{
		pop();
		Node * tmp = parseQ();
		eat((Token::Type)')');
		return tmp;
	}
	case Token::CELL:
	{
		pop();
		return new Node(Node::CELL, t.value);
	}
	case Token::REGION:
	{
		pop();
		return new Node(Node::REGION, t.value);
		break;
	}
	case Token::GEO_PATH:
	{
		pop();
		return new Node(Node::PATH, t.value);
		break;
	}
	case Token::GEO_RECT:
	{
		pop();
		return new Node(Node::RECT, t.value);
		break;
	}
	case Token::STRING:
	{
		pop();
		return new Node(Node::STRING, t.value);
		break;
	}
	case Token::ENDOFFILE:
	default: //hand back to caller, somethings wrong
		return 0;
	};
}


//(((())) ggh)
//Q ++ Q
detail::AdvancedCellOpTree::Node* Parser::parseQ() {
	Node * n = 0;
	for(;;) {
		Token t = peek();
		Node * curTokenNode = 0;
		switch (t.type) {
		case ')': //leave scope, caller removes closing brace
			//check if there's an unfinished operation, if there is ditch it
			if (n && n->type == Node::BINARY_OP && n->children.size() == 1) {
				Node* tmp = n;
				n = tmp->children.front();
				tmp->children.clear();
				delete tmp;
			}
			return n;
			break;
		case Token::UNARY_OP:
		case Token::DILATION_OP:
		{
			pop();
			Node * unaryOpNode = new Node();
			unaryOpNode->type = (t.type == Token::UNARY_OP ? Node::UNARY_OP : Node::DILATION_OP);
			unaryOpNode->value = t.value;
			Node* cn = parseSingleQ();
			if (cn) {
				unaryOpNode->children.push_back(cn);
				curTokenNode = unaryOpNode;
			}
			else { //something went wrong, skip this op
				continue;
			}
			break;
		}
		case Token::BINARY_OP:
		{
			pop();
			 //we need to have a valid child, otherwise this operation is bogus (i.e. Q ++ Q)
			if (n && !(n->type == Node::BINARY_OP && n->children.size() < 2)) {
				Node * opNode = new Node();
				opNode->type = Node::BINARY_OP;
				opNode->value = t.value;
				opNode->children.push_back(n);
				n = 0;
				//get the other child in the next round
				curTokenNode = opNode;
			}
			else { //skip this op
				continue;
			}
			break;
		}
		case Token::ENDOFFILE:
			return n;
			break;
		default:
			curTokenNode = parseSingleQ();
			break;
		};
		if (!curTokenNode) {
			continue;
		}
		if (n) {
			if (n->type == Node::BINARY_OP && n->children.size() < 2) {
				n->children.push_back(curTokenNode);
			}
			else {//implicit intersection
				Node * opNode = new Node(Node::BINARY_OP, " ");
				opNode->children.push_back(n);
				opNode->children.push_back(curTokenNode);
				n = opNode;
			}
		}
		else {
			n = curTokenNode;
		}
	}
	//this should never happen
	SSERIALIZE_CHEAP_ASSERT(false);
	return n;
}

Parser::Parser() {
	m_prevToken.type = -1;
	m_lastToken.type = -1;
}

detail::AdvancedCellOpTree::Node* Parser::parse(const std::string & str) {
	m_str.clear();
	//sanitize string, add as many openeing braces at the beginning as neccarray or as manny closing braces at the end
	{
		int obCount = 0;
		for(char c : str) {
			if (c == '(') {
				++obCount;
				m_str += c;
			}
			else if (c == ')') {
				if (obCount > 0) {
					m_str += c;
					--obCount;
				}
				//else not enough opening braces, so skip this one
			}
			else {
				m_str += c;
			}
		}
		//add remaining closing braces to get a well-formed query
		for(; obCount > 0;) {
			m_str += ')';
			--obCount;
		}
	}
	m_tokenizer  = Tokenizer(m_str.begin(), m_str.end());
	m_lastToken.type = -1;
	m_prevToken.type = -1;
	Node* n = parseQ();
	return n;
}

}}}//end namespace detail::AdvancedCellOpTree::parser

AdvancedCellOpTree::AdvancedCellOpTree(const sserialize::Static::CellTextCompleter & ctc, const CQRDilator & cqrd) :
m_ctc(ctc),
m_cqrd(cqrd),
m_root(0)
{}

AdvancedCellOpTree::~AdvancedCellOpTree() {
	if (m_root) {
		delete m_root;
	}
}

void AdvancedCellOpTree::parse(const std::string& str) {
	if (m_root) {
		delete m_root;
		m_root = 0;
	}
	detail::AdvancedCellOpTree::parser::Parser p;
	m_root = p.parse(str);
}

template<>
sserialize::CellQueryResult
AdvancedCellOpTree::Calc<sserialize::CellQueryResult>::calcDilationOp(AdvancedCellOpTree::Node* node) {
	double diameter = ::atof(node->value.c_str());
	sserialize::CellQueryResult cqr( calc(node->children.front()) );
	return sserialize::CellQueryResult( m_cqrd.dilate(cqr, diameter), cqr.geoHierarchy(), cqr.idxStore() ) + cqr;
}

template<>
sserialize::TreedCellQueryResult
AdvancedCellOpTree::Calc<sserialize::TreedCellQueryResult>::calcDilationOp(AdvancedCellOpTree::Node* node) {
	double diameter = ::atof(node->value.c_str());
	sserialize::TreedCellQueryResult cqr( calc(node->children.front()) );
	return sserialize::TreedCellQueryResult( m_cqrd.dilate(cqr.toCQR(), diameter), cqr.geoHierarchy(), cqr.idxStore() ) + cqr;
}


}//end namespace
