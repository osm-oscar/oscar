#include "MainHandler.h"
#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <cppcms/applications_pool.h>
#include <cppcms/util.h>
#include <iostream>
#include <stdlib.h>
#include "IndexDB.h"
#include "ItemDB.h"
#include "CQRCompleter.h"

namespace oscar_web {

MainHandler::MainHandler(cppcms::service& srv, const oscar_web::CompletionFileDataPtr & data): application(srv), m_data(data) {
	std::cout << "Created MainHandler" << std::endl;
	
	ItemDB * itemDB = new ItemDB(srv, m_data->completer->store());
	IndexDB * indexDB = new IndexDB(srv, m_data->completer->indexStore(), m_data->completer->store().geoHierarchy());
	CQRCompleter * cqrCompleter = new CQRCompleter(srv, data);
	
	itemDB->setMaxPerRequest(m_data->maxItemDBReq);
	indexDB->setMaxPerRequest(m_data->maxIndexDBReq);
	
	attach(itemDB, "itemdb", "/itemdb{1}", "/itemdb(/(.*))?", 1);
	attach(indexDB, "indexdb", "/indexdb{1}", "/indexdb(/(.*))?", 1);
	attach(cqrCompleter, "cqr", "/cqr{1}", "/cqr(/(.*))?", 1);

	dispatcher().assign("",&MainHandler::describe,this);
	mapper().assign(""); // default URL
	mapper().root("/oscar-web");
}

MainHandler::~MainHandler() {}

void MainHandler::describe() {
    // Handle CORS
	response().set_header("Access-Control-Allow-Origin","*");
	response().set_content_header("text/xml");
	std::ostream & out = response().out();
	out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
}


}//end namespace