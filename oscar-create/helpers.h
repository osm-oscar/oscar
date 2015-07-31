#ifndef OSCAR_CREATE_HELPERS_H
#define OSCAR_CREATE_HELPERS_H
#include <sserialize/storage/MmappedMemory.h>
#include <sserialize/storage/MmappedFile.h>
#include <sserialize/strings/stringfunctions.h>


namespace oscar_create {

bool findNodeIdBounds(const std::string & fileName, uint64_t & smallestId, uint64_t & largestId);
uint64_t getNumBlocks(const std::string & fileName);

template<typename TOutPutIterator>
bool readKeysFromFile(const std::string & fileName, TOutPutIterator out) {
	if (!sserialize::MmappedFile::fileExists(fileName)) {
		return false;
	}
	sserialize::MmappedMemory<char> mf(fileName);
	typedef sserialize::OneValueSet<char> MySet;
	sserialize::split<const char*, MySet, MySet, TOutPutIterator>(
	mf.begin(), mf.end(), MySet('\n'), MySet('\\'), out );
	return true;
}

template<typename TOutPutMap>
bool readKeyValuesFromFile(const std::string & fileName, TOutPutMap out) {
	if (!sserialize::MmappedFile::fileExists(fileName)) {
		return false;
	}
	sserialize::MmappedMemory<char> mf(fileName);
	typedef sserialize::OneValueSet<char> MySet;
	struct MyOutIt {
		std::string kv[2];
		uint32_t pos;
		TOutPutMap & out;
		void operator++() {
			++pos;
			if (pos >= 2) {
				out[kv[0]].insert(kv[1]);
				pos = 0;
			}
		}
		std::string & operator*() { return kv[pos]; }
		MyOutIt(TOutPutMap & out) : pos(0), out(out) {}
	};
	MyOutIt outIt(out);
	sserialize::split<const char*, MySet, MySet, MyOutIt>(
	mf.begin(), mf.end(), MySet('\n'), MySet('\\'), outIt);
	return true;
}

}//end namespace

#endif