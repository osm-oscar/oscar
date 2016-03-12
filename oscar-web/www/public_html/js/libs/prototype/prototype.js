define(["state", "oscar", "tools", "conf", "leafletCluster"], function (state, oscar, tools, config) {
    /**
     * Extend MarkerCluster:
     * 1) show names of subregions of a cluster in a popup
     * 2) show region boundaries of sub-clusters either by merging them on-the-fly or
     *    showing all sub-regions
     */
    L.MarkerCluster.prototype.on("mouseover", function (e) {
        if (e.target.getChildCount() > 1 && e.target.getChildCount() <= config.maxNumSubClusters) {
            var children = [];
            var leafletItem, key = "";
            for (var i in e.target.getAllChildMarkers()) {
                if (e.target.getAllChildMarkers()[i].rid) {
                    children.push(e.target.getAllChildMarkers()[i].rid);
                    key += e.target.getAllChildMarkers()[i].rid;
                }
            }

            if (e.target.polygons) {
                for (var polygon in e.target.polygons) {
                    e.target.polygons[polygon].addTo(state.map);
                }
            } else if (children.length) {
                oscar.getShapes(children, function (shapes) {
                    e.target.polygons = [];
                    for (var shape in shapes) {
                        leafletItem = oscar.leafletItemFromShape(shapes[shape]);
                        leafletItem.setStyle(config.styles.shapes['regions']['normal']);
                        e.target.polygons.push(leafletItem);
                        leafletItem.addTo(state.map);
                    }
                }, oscar.defErrorCB);
            }
        }

        var names = e.target.getChildClustersNames();
        var text = "";
        if (names.length > 0) {
            for (var i in names) {
                if(i > config.maxNumSubClusters){
                    text += "...";
                    break;
                }
                text += names[i];
                if (i < names.length - 1) {
                    text += ", ";
                }
            }
            L.popup({offset: new L.Point(0, -10)}).setLatLng(e.latlng).setContent(text).openOn(state.map);
        }
    });

    /**
     * Extend Markercluster: close popups and remove region-boundaries of sub-clusters
     */
    L.MarkerCluster.prototype.on("mouseout", function (e) {
        map.removeBoundaries(e.target);
        map.closePopups();
    });

    /**
     * Remove displayed region boundaries when the mouse is over a cluster-marker & the user zooms-in
     */
    var old = L.FeatureGroup.prototype.removeLayer;
    L.FeatureGroup.prototype.removeLayer = function (e) {
        map.removeBoundaries(e);
        old.call(this, e);
    };

});
