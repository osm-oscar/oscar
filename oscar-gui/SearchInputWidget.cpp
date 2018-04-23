#include "SearchInputWidget.h"

#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>

namespace oscar_gui {

SearchInputWidget::SearchInputWidget(const States & states) :
QWidget(),
m_goButton(new QPushButton("Go")),
m_clearButton(new QPushButton("Clear")),
m_search(new QLineEdit()),
m_tss(states.tss)
{
	connect(m_search, &QLineEdit::returnPressed, this, &SearchInputWidget::goButtonPressed);
	connect(m_goButton, &QPushButton::pressed, this, &SearchInputWidget::goButtonPressed);
	connect(m_clearButton, &QPushButton::pressed, this, &SearchInputWidget::clearButtonPressed);
	connect(this, &SearchInputWidget::searchTextChanged, m_tss.get(), &TextSearchState::searchTextChanged);
	
	QVBoxLayout * mainLayout = new QVBoxLayout();
	mainLayout->addWidget(m_search);
	mainLayout->addWidget(m_goButton);
	mainLayout->addWidget(m_clearButton);
	this->setLayout(mainLayout);
}

SearchInputWidget::~SearchInputWidget() {}

void SearchInputWidget::setSearchText(const QString & s) {
	m_search->setText(s);
}

void SearchInputWidget::goButtonPressed() {
	QString qs = m_search->text();
	emit(searchTextChanged(qs));
}

void SearchInputWidget::clearButtonPressed() {
	m_search->setText(QString());
	emit(searchTextChanged(QString()));
}

}//end namespace oscar_gui
