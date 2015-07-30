#include "constants.h"
#include <sserialize/storage/MmappedFile.h>

namespace liboscar {


FileConfig fileConfigFromString(const std::string & str) {
	if (str == "index") {
		return FC_INDEX;
	}
	else if (str == "tagstore") {
		return FC_TAGSTORE;
	}
	else if (str == "tagstore.phrases") {
		return FC_TAGSTORE_PHRASES;
	}
	else if (str == "kvstore") {
		return FC_KV_STORE;
	}
	else if (str == "textsearch") {
		return FC_TEXT_SEARCH;
	}
	else if (str == "geosearch") {
		return FC_GEO_SEARCH;
	}
	else {
		return FC_INVALID;
	}
}

std::string toString(FileConfig fc) {
	switch (fc) {
	case(FC_INDEX):
		return std::string("index");
	case(FC_TAGSTORE):
		return std::string("tagstore");
	case (FC_TAGSTORE_PHRASES):
		return std::string("tagstore.phrases");
	case (FC_KV_STORE):
		return std::string("kvstore");
	case (FC_GEO_SEARCH):
		return std::string("geosearch");
	case (FC_TEXT_SEARCH):
		return std::string("textsearch");
	default:
		return "invalid";
	}
}


std::string fileNameFromFileConfig(const std::string & prefix, FileConfig fc, bool useCompression) {
	if (useCompression)
		return prefix + "/" + liboscar::toString(fc) + ".compressed";
	else
		return prefix + "/" + liboscar::toString(fc);
}


bool fileNameFromPrefix(const std::string & prefix, FileConfig fc, std::string & filePath, bool & compression) {
	filePath = fileNameFromFileConfig(prefix, fc, false);
	if (sserialize::MmappedFile::fileExists(filePath) && sserialize::MmappedFile::fileSize(filePath) > 0) {
		compression = false;
		return true;
	}
	filePath = fileNameFromFileConfig(prefix, fc, true);
	if (sserialize::MmappedFile::fileExists(filePath) && sserialize::MmappedFile::fileSize(filePath) > 0) {
		compression = true;
		return true;
	}
	return false;
}

}//end namespace