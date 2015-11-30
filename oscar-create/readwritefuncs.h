#ifndef OSCAR_CREATE_READ_WRITE_FUNCS_H
#define OSCAR_CREATE_READ_WRITE_FUNCS_H
#include <liboscar/OsmKeyValueObjectStore.h>
#include "Config.h"
#include "State.h"

namespace oscar_create {

void handleKVCreation(Config & opts, State & state);
void handleSearchCreation(Config & opts, State & state);

}

#endif