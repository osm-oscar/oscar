#ifndef OSCAR_WEB_GEO_HIERARCHY_SUB_SET_SERIALIZER_H
#define OSCAR_WEB_GEO_HIERARCHY_SUB_SET_SERIALIZER_H
#include <sserialize/Static/GeoHierarchy.h>

namespace oscar_web {

/** This helper class converts a GeoHierarchy::SubSet to an xml-with-json representation or complete json representation
  *
  *
  * <oscarresponse>
  * 	<GeoHierarchy>
  * 		<SubSet type="full/sparse" rep="tree">
  * 			<region id="0" apxitems="">
  * 				<cellpositions>jsonarray_of_int</cellpositions>
  * 				<children>
  * 					<region>
  * 					</region>
  * 				</children>
  * 			</region>
  * 		</SubSet>
  * 		<SubSet type="full/sparse" rep="flat">
  * 			<region id="0" apxitems="">
  * 				<cellpositions>jsonarray_of_int</cellpositions>//only usefull in conjuntion with the cqr
  * 				<children>jsonarray_of_int</children>//region ids
  * 			</region>
  * 		</SubSet>
  * 	</GeoHierarchy>
  * </oscarresponse>
  *
  * JSON-Only representation (flat only), the first node is the root node
  * SubSet: {type : (full|sparse), rootchildren: [int] regions: {id : int, region-desc-json{}}} where id is an integer
  * region-desc-json:
  * { cellpositions: [int], children: [int] }
  *
  * Binary version:
  * ------------------------------------------------------------
  * Type    |RootRegionChildrenSize|(RootChildrenIds)|RegionDesc
  * ------------------------------------------------------------
  * uint8   |uint32                |(uint32)         |(uint32)*
  * 
  * 
  * RegionDesc:
  * ----------------------------------------------------------------------------------------------------------
  * (ID|MAX_ITEMS|CELLPOSITIONS_SIZE|CHILDREN_SIZE|(CELLPOSITION)*CELLPOSITIONS_SIZE|(CHILDID)*CHILDREN_SIZE
  * ----------------------------------------------------------------------------------------------------------
  * u32|u32      |u32               |u32          |(u32)                            |(u32)
  */

class GeoHierarchySubSetSerializer {
private:
	sserialize::Static::spatial::GeoHierarchy m_gh;
private:
	void toTreeXML(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr & node) const;
public:
	GeoHierarchySubSetSerializer(const sserialize::Static::spatial::GeoHierarchy& gh) : m_gh(gh) {}
	~GeoHierarchySubSetSerializer() {}
	std::ostream & toFlatXML(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::SubSet & subSet) const;
	std::ostream & toTreeXML(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::SubSet & subSet) const;
	std::ostream & toJson(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::SubSet & subSet) const;
	std::ostream & toBinary(std::ostream & out, const sserialize::Static::spatial::GeoHierarchy::SubSet & subSet) const;
};

}

#endif