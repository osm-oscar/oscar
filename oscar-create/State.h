#ifndef OSCAR_CREATE_STATE_H
#define OSCAR_CREATE_STATE_H
#include <sserialize/containers/ItemIndexFactory.h>
#include <liboscar/OsmKeyValueObjectStore.h>

namespace oscar_create {

struct Data {
	sserialize::UByteArrayAdapter indexFile;
	sserialize::UByteArrayAdapter storeData;
	~Data() {
		#ifdef WITH_OSCAR_CREATE_NO_DATA_REFCOUNTING
		indexFile.enableRefCounting();
		storeData.enableRefCounting();
		#endif
	}
};

struct State {
	sserialize::UByteArrayAdapter & indexFile;
	sserialize::ItemIndexFactory indexFactory;
	sserialize::UByteArrayAdapter & storeData;
	liboscar::Static::OsmKeyValueObjectStore store;
	State(Data & data) : indexFile(data.indexFile), storeData(data.storeData) {}
	~State() {}
};

}//end namespace

#endif