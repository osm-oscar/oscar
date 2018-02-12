#include "CellTextCompleter.h"
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/Static/UnicodeTrie/detail/SimpleNode.h>
#include <sserialize/containers/CFLArray.h>
#include <sserialize/containers/RLEStream.h>

namespace oscar_create {
namespace detail {
namespace CellTextCompleter {

void MatchDesc::dump(std::ostream& out) {
	out << "MatchDesc--BEGIN" << "\n";
	for(ContainerIterator it(this->begin()), end(this->end()); it != end; ++it) {
		out << it.cellId() << ":";
		if (it.fullyMatchedCell()) {
			out << "full";
		}
		else {
			sserialize::print<',', const uint32_t *>(out, it.itemsBegin(), it.itemsEnd());
		}
		out << "\n";
	}
	out << "MatchDesc--END" << "\n";
}

void Data::dump(std::ostream& out) {
	out << "Data--BEGIN" << "\n";
	out << "Exact--BEGIN\n";
	exact.dump(out);
	out << "Exact--END\n";
	out << "Suffix--BEGIN\n";
	suffix.dump(out);
	out << "Suffix-END\n";
	out << "id=" << id << "\n";
	out << "Data--END\n";
}



void ContainerMerger::operator()(MatchDesc & a, MatchDesc & b) {
	m_tmpD.clear();
	ContainerIterator aIt(a.begin()), aEnd(a.end());
	ContainerIterator bIt(b.begin()), bEnd(b.end());
	
	for(;aIt != aEnd && bIt != bEnd;) {
		uint32_t aCellId = aIt.cellId();
		uint32_t bCellId = bIt.cellId();
		if (aCellId < bCellId) {
			m_tmpD.insert(m_tmpD.end(), aIt.cellBegin(), aIt.cellEnd());
			++aIt;
		}
		else if (bCellId < aCellId) {
			m_tmpD.insert(m_tmpD.end(), bIt.cellBegin(), bIt.cellEnd());
			++bIt;
		}
		else {
			uint8_t ct = (aIt.fullyMatchedCell() << 1) |  bIt.fullyMatchedCell();
			switch (ct) {
			case 0x0: //both are not fully matched
				{
					m_tmpD.push_back(aIt.cellIdType());
					uint32_t tmpSize = (uint32_t) m_tmpD.size();
					m_tmpD.push_back(0);//dummy items count
					std::set_union(aIt.itemsBegin(), aIt.itemsEnd(), bIt.itemsBegin(), bIt.itemsEnd(), std::back_inserter<decltype(m_tmpD)>(m_tmpD));
					m_tmpD[tmpSize] = (uint32_t) (m_tmpD.size()-(tmpSize+1));
					if (m_tmpD[tmpSize] == cellItemCounts->at(aCellId)) { //cell is now fully matched
						m_tmpD.resize(tmpSize);
						m_tmpD.back() |= 0x1;
					}
				}
				break;
			case 0x1: //at least one cell is fully matched
			case 0x2:
			case 0x3:
			default:
				m_tmpD.push_back(aIt.cellIdType() | 0x1);
			};
			++aIt;
			++bIt;
		}
	}
	m_tmpD.insert(m_tmpD.end(), aIt.cellBegin(), aEnd.cellBegin());
	m_tmpD.insert(m_tmpD.end(), bIt.cellBegin(), bEnd.cellBegin());
	
	a.cellIds = Container::unite(a.cellIds, b.cellIds);
	a.cellIds.push_back(m_tmpD.cbegin(), m_tmpD.cend());
}

//c must hold at least cellIdsStorageNeed uints
void MatchDesc::initValueContainer(uint32_t* &dataBegin, const std::pair<uint32_t*, uint32_t*> & dataWindow, MyNodeStorageHashTable::iterator & begin, const MyNodeStorageHashTable::iterator & end) {
	uint32_t myNodeId = begin->first.nodeId;
	uint32_t* myDataBegin = dataBegin;
	uint32_t curOffset = 0;
	for(; begin != end && begin->first.nodeId == myNodeId; ++begin) {
		uint32_t & cellIdType = myDataBegin[curOffset];
		uint32_t cellId = begin->first.cellId;
		++curOffset;
		uint32_t & t = begin->second;//the number of elements in this cell or a fully matched cell
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(dataBegin+curOffset, dataWindow.first);
		SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(dataBegin+curOffset, dataWindow.second);
		if (t == std::numeric_limits<uint32_t>::max()) { //a fully matched cell
			cellIdType = (cellId << 1) | 0x1;
		}
		else {
			cellIdType = (cellId << 1);//a partial matched cell
			myDataBegin[curOffset] = t; //store the number of elements in this cell in data storage
			++curOffset;
			uint32_t itemsCount = t;
			t = curOffset;//store push pointer for elements in MyNodeStorageHashTable as value
			curOffset += itemsCount;//reserve itemsCount many items in data storage
		}
	}
	dataBegin += curOffset;
	SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(dataBegin, dataWindow.first);
	SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(dataBegin, dataWindow.second);
	cellIds = Container(myDataBegin, dataBegin, dataBegin);
}

bool MatchDesc::isConsistent(bool checkItemIndex) const {
	int64_t lastId = -1;
	for(ContainerIterator it(this->begin()), end(this->end()); it != end; ++it) {
		uint32_t tmp = it.cellId();
		if (tmp <= lastId) {
			return false;
		}
		if (checkItemIndex && !it.fullyMatchedCell()) {
			if (!std::is_sorted(it.itemsBegin(), it.itemsEnd())) {
				return false;
			}
			if (!sserialize::is_strong_monotone_ascending(it.itemsBegin(), it.itemsEnd())) {
				sserialize::is_strong_monotone_ascending(it.itemsBegin(), it.itemsEnd());
				return false;
			}
		}
	}
	return true;
}

void MatchDesc::makeConsistent(std::vector<uint32_t> & tmp) {
	tmp.clear();
	tmp.insert(tmp.end(), cellIds.cbegin(), cellIds.cend());
	cellIds.clear();
	for(ContainerIterator it(&*tmp.begin()), end(&*tmp.end()); it != end; ++it) {
		if (it.fullyMatchedCell()) {
			cellIds.push_back(it.cellBegin(), it.cellEnd());
		}
		else {
			std::sort( (uint32_t*) it.itemsBegin(), (uint32_t*) it.itemsEnd());
			uint32_t * uniqueItemsEnd = std::unique( (uint32_t*) it.itemsBegin(), (uint32_t*) it.itemsEnd());
			cellIds.push_back(it.cellIdType());
			cellIds.push_back((uint32_t) (uniqueItemsEnd - it.itemsBegin()));
			cellIds.push_back<const uint32_t *>(it.itemsBegin(), uniqueItemsEnd);
		}
	}
}

//StorageHandler begin

StorageHandler::StorageHandler(sserialize::MmappedMemoryType hashMMT, sserialize::MmappedMemoryType storageMMT) :
m_storageMMT(storageMMT),
m_exactH(detail::CellTextCompleter::MyNodeStorageHTValueStorage(hashMMT), detail::CellTextCompleter::MyNodeStorageHTStorage(hashMMT)),
m_suffixH(detail::CellTextCompleter::MyNodeStorageHTValueStorage(hashMMT), detail::CellTextCompleter::MyNodeStorageHTStorage(hashMMT)),
m_exactStorageNeed(0),
m_suffixStorageNeed(0),
m_eBht(0),
m_sBht(0)
{
	m_exactH.rehashMultiplier(1.2);
	m_suffixH.rehashMultiplier(1.2);
	m_exactH.maxCollisions(100);
	m_suffixH.maxCollisions(100);
	m_eBht = detail::CellTextCompleter::MyBoundedHashTable(&exactH());
	m_sBht = detail::CellTextCompleter::MyBoundedHashTable(&suffixH());
}

void StorageHandler::setHashParams(uint32_t nodeCount, uint32_t cellCount) {
	//TODO:suffixH and exactH could use the same parameters, this would save about 6 GiB for planet
	m_exactH.hash1().setParams(nodeCount, cellCount);
	m_exactH.hash2().setParams(nodeCount, cellCount);
	m_suffixH.hash1().setParams(nodeCount, cellCount);
	m_suffixH.hash2().setParams(nodeCount, cellCount);
}

void StorageHandler::finalizeHashTable() {
	auto sortFunc = [](const MyHashTable::value_type & a, const MyHashTable::value_type & b) {
		return a.first < b.first;
	};
	m_exactH.sort(sortFunc);
	m_suffixH.sort(sortFunc);
	std::cout << std::endl;
	std::cout << "CellTextCompleter::StorageHandler::finalizeHashTable::exact.load_Factor=" << m_exactH.load_factor() << std::endl;
	std::cout << "CellTextCompleter::StorageHandler::finalizeHashTable::suffix.load_Factor=" << m_suffixH.load_factor() << std::endl;
	uint64_t exactHStorageSize = m_exactH.capacity()*sizeof(uint64_t) + m_exactH.storageCapacity()*sizeof(MyHashTable::value_type);
	uint64_t suffixHStorageSize = m_suffixH.capacity()*sizeof(uint64_t) + m_suffixH.storageCapacity()*sizeof(MyHashTable::value_type);
	std::cout << "CellTextCompleter::StorageHandler::finalizeHashTable::exactH.storageUsage=" << sserialize::prettyFormatSize(exactHStorageSize) << std::endl;
	std::cout << "CellTextCompleter::StorageHandler::finalizeHashTable::suffixH.storageUsage=" << sserialize::prettyFormatSize(suffixHStorageSize) << std::endl;
}

void StorageHandler::calculateStorage() {
	m_exactStorageNeed = 0;
	m_suffixStorageNeed = 0;
	for(auto x : m_exactH) {
		m_exactStorageNeed += (x.second == std::numeric_limits<uint32_t>::max()) ? 1 : 2+x.second;
	}
	for(auto x : m_suffixH) {
		m_suffixStorageNeed += (x.second == std::numeric_limits<uint32_t>::max()) ? 1 : 2+x.second;
	}
}


void StorageHandler::clearHashes() {
	m_exactH = MyHashTable(detail::CellTextCompleter::MyNodeStorageHTValueStorage(sserialize::MM_PROGRAM_MEMORY), detail::CellTextCompleter::MyNodeStorageHTStorage(sserialize::MM_PROGRAM_MEMORY));
	m_suffixH = MyHashTable(detail::CellTextCompleter::MyNodeStorageHTValueStorage(sserialize::MM_PROGRAM_MEMORY), detail::CellTextCompleter::MyNodeStorageHTStorage(sserialize::MM_PROGRAM_MEMORY));
}


//StorageHandler end

uint8_t serialize(const Payload::ResultDesc & src, sserialize::UByteArrayAdapter & dest) {
	std::vector<uint32_t> tmp;
	tmp.push_back(src.fmPtr);
	tmp.push_back(src.pPtr);
	tmp.insert(tmp.end(), src.pItemsPtrs.cbegin(), src.pItemsPtrs.cend());
	return sserialize::CompactUintArray::create(tmp, dest);
}

void testPayloadSerializer(sserialize::RLEStream::Creator & dest, const Payload::ResultDesc & rd) {
	dest.checkpoint(rd.fmPtr);
	dest.put(rd.pPtr);
	for(uint32_t x : rd.pItemsPtrs) {
		dest.put(x);
	}
}

void testPayloadSerializer(sserialize::UByteArrayAdapter & dest, const Payload::ResultDesc & rd) {
	dest.putVlPackedUint32(rd.fmPtr);
	dest.putVlPackedInt32(sserialize::narrow_check<int32_t>((int64_t)rd.fmPtr - (int64_t)rd.pPtr));
	int64_t prev = 0;
	for(uint32_t x : rd.pItemsPtrs) {
		dest.putVlPackedInt32(sserialize::narrow_check<int32_t>((int64_t)x - prev));
		prev = x;
	}
}
/*
void testPayloadSerializer(sserialize::UByteArrayAdapter & dest, const Payload::ResultDesc & rd) {
	dest.putVlPackedUint32(rd.fmPtr);
	dest.putVlPackedUint32(rd.pPtr);
	int64_t prev = 0;
	for(uint32_t x : rd.pItemsPtrs) {
		dest.putVlPackedUint32(x);
	}
}
*/

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const Payload & src) {
	if (src.availableDescs == sserialize::StringCompleter::QT_NONE) {
		dest.putUint8(0);
		return dest;
	}

	std::vector<uint8_t> tmpData;
	std::vector<uint32_t> offsets;
	sserialize::UByteArrayAdapter tmpAdap(&tmpData, false);
	sserialize::RLEStream::Creator cto(tmpAdap);
	if (src.availableDescs & sserialize::StringCompleter::QT_EXACT) {
		offsets.push_back(sserialize::narrow_check<uint32_t>(tmpAdap.size()));
		testPayloadSerializer(cto, src.exact);
		cto.flush();
		#ifdef WITH_SSERIALIZE_EXPENSIVE_ASSERT
		sserialize::RLEStream rs(tmpAdap+offsets.back());
		SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(*rs, src.exact.fmPtr);
		++rs;
		SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(*rs, src.exact.pPtr);
		++rs;
		for(auto x : src.exact.pItemsPtrs) {
			SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(*rs, x);
			++rs;
		}
		#endif
	}
	if (src.availableDescs & sserialize::StringCompleter::QT_PREFIX) {
		offsets.push_back(sserialize::narrow_check<uint32_t>( tmpAdap.size() ) );
		testPayloadSerializer(cto, src.prefix);
		cto.flush();
		#ifdef WITH_SSERIALIZE_EXPENSIVE_ASSERT
		sserialize::RLEStream rs(tmpAdap+offsets.back());
		SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(*rs, src.prefix.fmPtr);
		++rs;
		SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(*rs, src.prefix.pPtr);
		++rs;
		for(auto x : src.prefix.pItemsPtrs) {
			SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(*rs, x);
			++rs;
		}
		#endif
	}
	if (src.availableDescs & sserialize::StringCompleter::QT_SUFFIX) {
		offsets.push_back(sserialize::narrow_check<uint32_t>( tmpAdap.size() ) );
		testPayloadSerializer(cto, src.suffix);
		cto.flush();
		#ifdef WITH_SSERIALIZE_EXPENSIVE_ASSERT
		sserialize::RLEStream rs(tmpAdap+offsets.back());
		SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(*rs, src.suffix.fmPtr);
		++rs;
		SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(*rs, src.suffix.pPtr);
		++rs;
		for(auto x : src.suffix.pItemsPtrs) {
			SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(*rs, x);
			++rs;
		}
		#endif
	}
	if (src.availableDescs & sserialize::StringCompleter::QT_SUBSTRING) {
		offsets.push_back(sserialize::narrow_check<uint32_t>( tmpAdap.size() ) );
		testPayloadSerializer(cto, src.substr);
		cto.flush();
		#ifdef WITH_SSERIALIZE_EXPENSIVE_ASSERT
		sserialize::RLEStream rs(tmpAdap+offsets.back());
		SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(*rs, src.substr.fmPtr);
		++rs;
		SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(*rs, src.substr.pPtr);
		++rs;
		for(auto x : src.substr.pItemsPtrs) {
			SSERIALIZE_EXPENSIVE_ASSERT_EQUAL(*rs, x);
			++rs;
		}
		#endif
	}
	dest.putUint8(src.availableDescs);
	if (offsets.size() > 1) {
		uint32_t prevOffset = 0;
		for(std::vector<uint32_t>::const_iterator it(offsets.cbegin()+1), end(offsets.cend()); it != end; ++it) {
			dest.putVlPackedUint32(*it-prevOffset);
			prevOffset = *it;
		}
	}
	dest.putData(tmpAdap);
	return dest;
}

/*
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const Payload & src) {
	if (src.availableDescs == sserialize::StringCompleter::QT_NONE) {
		dest.putUint16(0);
		return dest;
	}

	std::vector<uint8_t> tmpData;
	sserialize::UByteArrayAdapter tmpAdap(&tmpData, false);
	std::vector<uint32_t> offsetBits;
	if (src.availableDescs & sserialize::StringCompleter::QT_EXACT) {
		uint32_t o = (tmpAdap.size() << 5);
		offsetBits.push_back( o | ((uint32_t) serialize(src.exact, tmpAdap) - 1) );
	}
	if (src.availableDescs & sserialize::StringCompleter::QT_PREFIX) {
		uint32_t o = (tmpAdap.size() << 5);
		offsetBits.push_back( o | ((uint32_t) serialize(src.prefix, tmpAdap) - 1));
	}
	if (src.availableDescs & sserialize::StringCompleter::QT_SUFFIX) {
		uint32_t o = (tmpAdap.size() << 5);
		offsetBits.push_back( o | ((uint32_t) serialize(src.suffix, tmpAdap) - 1) );
	}
	if (src.availableDescs & sserialize::StringCompleter::QT_SUBSTRING) {
		uint32_t o = (tmpAdap.size() << 5);
		offsetBits.push_back( o | ((uint32_t) serialize(src.substr, tmpAdap) - 1) );
	}
	uint16_t bitTypes = (((uint16_t)sserialize::CompactUintArray::minStorageBits(offsetBits.back())-1) << 4) | (src.availableDescs & 0xF);
	
	//now assemble it
	dest.putUint16(bitTypes);
	sserialize::CompactUintArray::create(offsetBits, dest);
	dest.put(tmpData);
	return dest;
}
*/
/*
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const Payload & src) {
	if (src.availableDescs == sserialize::StringCompleter::QT_NONE) {
		dest.putUint8(0);
		return dest;
	}

	std::vector<uint8_t> tmpData;
	std::vector<uint32_t> offsets;
	sserialize::UByteArrayAdapter tmpAdap(&tmpData, false);
	if (src.availableDescs & sserialize::StringCompleter::QT_EXACT) {
		offsets.push_back(tmpAdap.size());
		testPayloadSerializer(tmpAdap, src.exact);
	}
	if (src.availableDescs & sserialize::StringCompleter::QT_PREFIX) {
		offsets.push_back(tmpAdap.size());
		testPayloadSerializer(tmpAdap, src.prefix);
	}
	if (src.availableDescs & sserialize::StringCompleter::QT_SUFFIX) {
		offsets.push_back(tmpAdap.size());
		testPayloadSerializer(tmpAdap, src.suffix);

	}
	if (src.availableDescs & sserialize::StringCompleter::QT_SUBSTRING) {
		offsets.push_back(tmpAdap.size());
		testPayloadSerializer(tmpAdap, src.substr);
	}
	dest.putUint8(src.availableDescs);
	if (offsets.size() > 1) {
		for(std::vector<uint32_t>::const_iterator it(offsets.cbegin()+1), end(offsets.cend()); it != end; ++it) {
			dest.putVlPackedUint32(*it-*(it-1));
		}
	}
	dest.put(tmpData);
	return dest;
}
*/

void Finalizer::finalize(sserialize::UnicodeTrie::Trie<Data> & tt) {
	distributeIds(tt.root());
}

void Finalizer::finalize(sserialize::HashBasedFlatTrie<Data> & ft) {
	ft.finalize();
	distributeIds(ft.root());
	std::cout << "HashBasedFlatTrie::minStorageSize=" << sserialize::prettyFormatSize(ft.minStorageSize()) << std::endl;
}

Payload::ResultDesc PayloadHandler::createResultDesc(MatchDesc & src) {
	m_createResultDescPMIdx.clear();
	m_createResultDescFMIdx.clear();
	Payload::ResultDesc r;

	ContainerIterator it(src.begin());
	ContainerIterator end(src.end());

	for(; it != end; ++it) {
		if (it.fullyMatchedCell()) {
			m_createResultDescFMIdx.push_back(it.cellId());
		}
		else {
			m_createResultDescPMIdx.push_back(it.cellId());
			sserialize::CFLArray<const uint32_t> itc(it.itemsBegin(), it.itemsCount());
			r.pItemsPtrs.push_back(idxFactory->addIndex<decltype(itc)>(itc));
		}
	}

	r.fmPtr = idxFactory->addIndex(m_createResultDescFMIdx);
	r.pPtr = idxFactory->addIndex(m_createResultDescPMIdx);
	return r;
}

void PayloadComparatorBase::DataGatherer::MatchDesc::insert(const detail::CellTextCompleter::MatchDesc & matchDesc) {
	for(detail::CellTextCompleter::ContainerIterator it(matchDesc.begin()), end(matchDesc.end()); it != end; ++it) {
		if (it.fullyMatchedCell()) {
			fullyMatchedCells.insert(it.cellId());
		}
		else {
			partialMatchedCells[it.cellId()].insert(it.itemsBegin(), it.itemsEnd());
		}
	}
}
bool PayloadComparatorBase::eq(const DataGatherer::MatchDesc & /*a*/, const sserialize::Static::CellTextCompleter::Payload::Type & /*b*/) const {
	return false;//it's not possible to check the payload for equality as it get's destroyed during serialization
}

bool PayloadComparatorBase::eq(const detail::CellTextCompleter::MatchDesc & /*a*/, const sserialize::Static::CellTextCompleter::Payload::Type & /*b*/) const {
	return false;
}

}}//end namespace detail::CellTextCompleter

template<>
sserialize::UByteArrayAdapter&
CellTextCompleter< sserialize::UnicodeTrie::Trie<detail::CellTextCompleter::Data>  >::append(sserialize::StringCompleter::SupportedQuerries sq, sserialize::UByteArrayAdapter & dest, sserialize::ItemIndexFactory & idxFactory, uint32_t /*threadCount*/) {
	detail::CellTextCompleter::PayloadHandler ph(&m_cellItemCounts);
	ph.idxFactory = &idxFactory;
	sserialize::UnicodeTrie::Trie<TrieData>::NodeCreatorPtr nc(new sserialize::Static::UnicodeTrie::detail::SimpleNodeCreator());
	dest.putUint8(3); //Version
	dest.putUint8(sq); //SupportedQuerries
	dest.putUint8(sserialize::CellQueryResult::FF_CELL_GLOBAL_ITEM_IDS);
	dest.putUint8(sserialize::Static::CellTextCompleter::TrieTypeMarker::TT_TRIE);//Trie type marker
	m_trie.append<detail::CellTextCompleter::PayloadHandler, detail::CellTextCompleter::Payload>(dest, ph, nc);
	return dest;
}

template<>
sserialize::UByteArrayAdapter&
CellTextCompleter< sserialize::HashBasedFlatTrie<detail::CellTextCompleter::Data> >::append(sserialize::StringCompleter::SupportedQuerries sq, sserialize::UByteArrayAdapter& dest, sserialize::ItemIndexFactory & idxFactory, uint32_t threadCount) {
	detail::CellTextCompleter::PayloadHandler ph(&m_cellItemCounts);
	ph.idxFactory = &idxFactory;
	dest.putUint8(3); //Version
	dest.putUint8(sq); //SupportedQuerries
	dest.putUint8(sserialize::CellQueryResult::FF_CELL_GLOBAL_ITEM_IDS);
	dest.putUint8(sserialize::Static::CellTextCompleter::TrieTypeMarker::TT_FLAT_TRIE);//Trie type marker
	m_trie.append<detail::CellTextCompleter::PayloadHandler, detail::CellTextCompleter::Payload>(dest, ph, threadCount);
	return dest;
}

template<>
CellTextCompleter< sserialize::HashBasedFlatTrie<detail::CellTextCompleter::Data> >::CellTextCompleter(sserialize::MmappedMemoryType mmt) :
m_trie(sserialize::MM_SHARED_MEMORY, mmt),
m_sh(sserialize::MM_SHARED_MEMORY, mmt)
{}



template<>
bool
CellTextCompleter< sserialize::UnicodeTrie::Trie<detail::CellTextCompleter::Data>  >::
equal(sserialize::Static::CellTextCompleter sct, std::function< sserialize::ItemIndex(uint32_t)> /*idxDerefer*/) {
	typedef sserialize::Static::CellTextCompleter::TrieType MyStaticTrieType;
	sserialize::RCPtrWrapper<MyStaticTrieType> st(sct.trie().as<MyStaticTrieType>());
	return false;
}

template<>
bool
CellTextCompleter< sserialize::HashBasedFlatTrie<detail::CellTextCompleter::Data> >::
equal(sserialize::Static::CellTextCompleter sct, std::function< sserialize::ItemIndex(uint32_t)> /*idxDerefer*/) {
	typedef sserialize::Static::CellTextCompleter::FlatTrieType MyStaticUnicodeMap;
	typedef MyStaticUnicodeMap::TrieType MyStaticTrieType;
	sserialize::RCPtrWrapper<MyStaticUnicodeMap> sum(sct.trie().as<MyStaticUnicodeMap>());
	const MyStaticTrieType & st = sum->trie();
	if (st.size() != m_trie.size()) {
		return false;
	}
	uint32_t i(0), s(st.size());
	MyTrie::const_iterator it(m_trie.cbegin()), end(m_trie.cend());
	for(; i < s && it != end; ++i, ++it) {
		if (st.strAt(i) != m_trie.toStr(it->first)) {
			std::cerr << "CTC: Strings at " << i << " differ: SHOULD=" <<  m_trie.toStr(it->first) << "; IS=" << st.strAt(i) << std::endl;
			return false;
		}
	}
	//TODO: check the payload equality
	return true;
}

}//end namespace

//TODO: check on merge if fullyMatchedCells contain a partial matched one and vice versa and remove accordingly


