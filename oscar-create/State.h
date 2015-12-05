#ifndef OSCAR_CREATE_STATE_H
#define OSCAR_CREATE_STATE_H
#include <sserialize/containers/ItemIndexFactory.h>
#include <liboscar/OsmKeyValueObjectStore.h>

namespace oscar_create {

struct State {
	sserialize::UByteArrayAdapter indexFile;
	sserialize::ItemIndexFactory indexFactory;
	liboscar::Static::OsmKeyValueObjectStore store;
};

}//end namespace

#endif