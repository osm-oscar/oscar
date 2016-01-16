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
	Token t;
	while (m_state.it != m_state.end) {
		switch (*m_state.it) {
		case '%':
			t.type = Token::UNARY_OP;
			t.value += *m_state.it;
			++m_state.it;
			return t;
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
						t.value += c;
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

//(((())) ggh)
//Q ++ Q
detail::AdvancedCellOpTree::Node* Parser::parseQ() {
	Node * n = 0;
	for(;;) {
		Token t = peek();
		Node * curTokenNode = 0;
		switch (t.type) {
		case '(': //opens a new query
		{
			pop();
			curTokenNode = parseQ();
			eat((Token::Type)')');
			break;
		}
		case ')': //leave scope, caller removes closing brace
			return n;
			break;
		case Token::UNARY_OP:
		{
			pop();
			Node * unaryOpNode = new Node();
			unaryOpNode->type = Node::UNARY_OP;
			unaryOpNode->value = t.value;
			curTokenNode = unaryOpNode;
			break;
		}
		case Token::BINARY_OP:
		{
			pop();
			 //we have a valid child
			if (n && !(n->type == Node::BINARY_OP && n->children.size() < 2) && !(n->type == Node::UNARY_OP && n->children.size() < 1)) {
				Node * opNode = new Node();
				opNode->type = Node::BINARY_OP;
				opNode->value = t.value;
				opNode->children.push_back(n);
				n = 0;
				curTokenNode = opNode;
			}
			//else: no valid child, skip this op
			continue;
		}
		case Token::CELL:
		{
			assert(!n);
			pop();
			curTokenNode = new Node(Node::CELL, t.value);
			break;
		}
		case Token::REGION:
		{
			pop();
			assert(!n);
			curTokenNode = new Node(Node::REGION, t.value);
			break;
		}
		case Token::GEO_PATH:
		{
			pop();
			assert(!n);
			curTokenNode = new Node(Node::PATH, t.value);
			break;
		}
		case Token::GEO_RECT:
		{
			pop();
			assert(!n);
			curTokenNode = new Node(Node::RECT, t.value);
			break;
		}
		case Token::STRING:
		{
			pop();
			curTokenNode = new Node(Node::STRING, t.value);
			break;
		}
		case Token::ENDOFFILE:
		default:
			return n;
		};
		if (n) {
			if (n->type == Node::BINARY_OP && n->children.size() < 2) {
				n->children.push_back(curTokenNode);
			}
			else if (n->type == Node::UNARY_OP && n->children.size() < 1) {
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
}

Parser::Parser() {
	m_prevToken.type = -1;
	m_lastToken.type = -1;
}

detail::AdvancedCellOpTree::Node* Parser::parse(const std::string & str) {
	m_str.clear();
	//sanizize string, add as many openeing braces at the beginning as neccarray or as manny closing braces at the end
	{
		std::string::size_type obCount = 0;
		std::string::size_type cbCount = 0;
		for(char c : str) {
			if (c == '(') {
				++obCount;
			}
			else if (c == ')') {
				++cbCount;
			}
		}
		for(;obCount < cbCount; ++obCount) {
			m_str += '(';
		}
		m_str += str;
		for(;cbCount < obCount; ++cbCount) {
			m_str += ')';
		}
	}
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
