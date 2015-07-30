#ifndef OSCAR_WEB_MAIN_HANDLER_H
#define OSCAR_WEB_MAIN_HANDLER_H
#include <cppcms/application.h>
#include "types.h"

namespace oscar_web {

class MainHandler: public cppcms::application {
private:
	CompletionFileDataPtr m_data;
public:
	MainHandler(cppcms::service & srv, const CompletionFileDataPtr & data);
	virtual ~MainHandler();
	void describe();
};

}//end namespace

#endif