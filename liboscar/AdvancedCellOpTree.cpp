#include "AdvancedCellOpTree.h"

namespace liboscar {
namespace detail {
namespace AdvancedCellOpTree {
namespace parser {

Tokenizer::Tokenizer(const Tokenizer::State& state) :
m_state(state)
{}

Tokenizer::Tokenizer(std::string::const_iterator begin, std::string::const_iterator end) {
	m_state.it = begin;
	m_state.end = end;
}

bool Tokenizer::isSeperator(char c) const {
	return (c == ' ' || c == '\t' || //white space
			c == '(' || c == ')' || //braces open new queries
			c == '+' || c == '-' || c == '/' || c == '%' || c == '^'); //operators
}


//TODO:use ragel to parse?
Token Tokenizer::next() {
	Token t;
	while (m_state.it != m_state.end) {
		switch (*m_state.it) {
		case '%':
			t = Token::UNARY_OP;
			t.value += *m_state.it;
			++m_state.it;
			return t;
		case '+':
		case '-':
		case '/':
		case '^':
			t = Token::BINARY_OP;
			t.value += *m_state.it;
			++m_state.it;
			return t;
		case ' ': //ignore whitespace
		case '\t':
			break;
		case '(':
		case ')':
			t = *m_state.it;
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
			}
			break;
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
						t.value += *m_state.it;
						++m_state.it;
					}
				}
				return t;
			}
			break;
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
}

Token Parser::peek() {
	if (-1 == m_lastToken.type) {
		m_lastToken = m_tokenizer.next();
	}
	return m_lastToken;
}


bool Parser::eat(Token::Type t) {
	if (t != peek()) {
		return false;
	}
	pop();
	return true;
}

detail::AdvancedCellOpTree::Node* Parser::parseQ() {
	Node * n = 0;
	for(;;) {
		Token t = peek();
		switch (t.type) {
		case '(': //opens a new 
			pop();
			Node * n = parseQ();
			eat(')');
			return n;
		case ')': //superficial closing brace, skip
			pop();
			break;
		case Token::UNARY_OP: //after that comes another query
			{
				pop();
				assert(!n);
				n = new Node();
				n->type = Node::UNARY_OP;
				n->value = t.value;
				Node * cn = parseQ();
				assert(cn);
				n->children.push_back(cn);
				return n;
			}
		case Token::BINARY_OP:
			{
				pop();
				//we need to have one child already
				assert(n && n->type != Node::BINARY_OP && n->type != Node::UNARY_OP);
				{
					Node * opNode = new Node();
					opNode->type = Node::BINARY_OP;
					opNode->value = t.value;
					opNode->children.push_back(n);
					n = opNode;
				}
				//get the other child:
				Node * cn = parseQ();
				assert(cn);
				n->children.push_back(cn);
				return n;
			}
		case Token::CELL:
			assert(!n);
			pop();
			return new Node(Node::CELL, t.value);
		case Token::REGION:
			pop();
			assert(!n);
			return new Node(Node::REGION, t.value);
		case Token::GEO_PATH:
			pop();
			assert(!n);
			return new Node(Node::PATH, t.value);
		case Token::GEO_RECT:
			pop();
			assert(!n);
			return new Node(Node::RECT, t.value);
		case Token::STRING:
			pop();
			if (n) { //this is a intersection
				Node * opNode = new Node(Node::BINARY_OP, ' ');
				Node * cn = new Node(Node::STRING, t.value);
				opNode->children.push_back(n);
				opNode->children.push_back(cn);
				n = opNode;
			}
			else {
				n = new Node(Node::STRING, t.value);
			}
			break;
		case Token::ENDOFFILE:
		default:
			return n;
		};
	}
}

detail::AdvancedCellOpTree::Node* Parser::parse(const std::string & str) {
	m_str = str;
	m_tokenizer  = Tokenizer(m_str.begin(), m_str.end());
	m_lastToken.type = -1;
	m_prevToken.type = -1;
	Node* n = parseQ();
	return n;
}

}}}//end namespace detail::AdvancedCellOpTree::parser

AdvancedCellOpTree::AdvancedCellOpTree(const sserialize::Static::CellTextCompleter& ctc) :
m_ctc(ctc),
m_root(0)
{}

AdvancedCellOpTree::~AdvancedCellOpTree() {
	if (m_root) {
		delete m_root;
	}
}

void AdvancedCellOpTree::parse(const std::string& str) {
	detail::AdvancedCellOpTree::parser::Parser p;
	m_root = p.parse(str);
}


}//end namespace
