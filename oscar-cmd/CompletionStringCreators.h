#ifndef OSCAR_CMD_COMPLETION_STRING_CREATORS_H
#define OSCAR_CMD_COMPLETION_STRING_CREATORS_H
#include <deque>
#include <liboscar/OsmKeyValueObjectStore.h>

namespace oscarcmd {

std::deque< std::string > fromDB(const liboscar::Static::OsmKeyValueObjectStore & db, sserialize::StringCompleter::QuerryType qt, uint32_t count);



}//end namespace

#endif