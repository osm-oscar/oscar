#ifndef LIBOSCAR_CONSTANTS_H
#define LIBOSCAR_CONSTANTS_H
#include <string>

namespace liboscar {

typedef enum {
	OSMIT_INVALID=0, OSMIT_NODE=1, OSMIT_WAY=2, OSMIT_RELATION=3
} OsmItemTypes;

enum FileConfig {
	FC_INVALID=0, FC_BEGIN=1,
	FC_INDEX=1, FC_KV_STORE=2, FC_TAGSTORE=3,
	FC_TEXT_SEARCH=4, FC_GEO_SEARCH=5, FC_END=6,
	FC_TAGSTORE_PHRASES=7
};

FileConfig fileConfigFromString(const std::string & str);
std::string toString(FileConfig fc);
std::string fileNameFromFileConfig(const std::string & prefix, FileConfig fc, bool useCompression);


bool fileNameFromPrefix(const std::string & prefix, FileConfig fc, std::string & filePath, bool & compression);

}//end namespace liboscar

#endif