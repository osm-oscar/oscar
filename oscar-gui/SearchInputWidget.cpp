#include "SearchInputWidget.h"

#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>

namespace oscar_gui {

SearchInputWidget::SearchInputWidget(QWidget* parent, Qt::WindowFlags f) :
QWidget(parent, f),
m_search(new QLineEdit())
{
	connect(m_search, SIGNAL(textChanged(QString)), this, SIGNAL(searchTextChanged(QString)));
	QVBoxLayout * mainLayout = new QVBoxLayout();
	mainLayout->addWidget(m_search);
	this->setLayout(mainLayout);
}

SearchInputWidget::~SearchInputWidget() {}

void SearchInputWidget::setSearchText(const QString & s) {
	m_search->setText(s);
}



}//end namespace oscar_gui