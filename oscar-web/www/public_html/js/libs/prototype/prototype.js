define(["state", "oscar", "tools", "conf", "turf"], function(state, oscar, tools, config, turf){
    /**
     * Extend MarkerCluster:
     * 1) show names of subregions of a cluster in a popup
     * 2) show region boundaries of sub-clusters either by merging them on-the-fly or
     *    showing all sub-regions
     */
    L.MarkerCluster.prototype.on("mouseover", function (e) {
        // merge the region boundaries of sub-clusters
        if (e.target.getChildCount() > 1) {
            var children = [];
            var leafletItem, key = "", mergedRegion;
            for (var i in e.target.getAllChildMarkers()) {
                if (e.target.getAllChildMarkers()[i].rid) {
                    children.push(e.target.getAllChildMarkers()[i].rid);
                    key += e.target.getAllChildMarkers()[i].rid;
                }
            }

            mergedRegion = e.target.merged || state.turfCache.at(key);
            if (mergedRegion) {
                e.target.merged = mergedRegion;
                mergedRegion.addTo(state.map);
            } else if (children.length && !state.boundariesInProcessing.count(key)) {
                oscar.getShapes(children, function (shapes) {
                    // collect all boundaries of sub-clusters and convert to GeoJSON
                    var boundaries = [];
                    var edges = 0;
                    for (var shape in shapes) {
                        leafletItem = oscar.leafletItemFromShape(shapes[shape]);
                        boundaries.push(oscar.leafletItemFromShape(shapes[shape]).toGeoJSON());
                    }

                    for (var boundary in boundaries) {
                        for (var coordinate in boundaries[boundary].geometry.coordinates) {
                            edges += boundaries[boundary].geometry.coordinates[coordinate][0].length;
                        }
                    }

                    if (edges < config.maxNumPolygonEdges) {
                        // merge them in a background-job
                        var worker = new Worker('js/libs/oscar/polygonMerger.js'); // TODO: path anpassen
                        var timer = tools.timer("Polygon-Merge");
                        worker.addEventListener('message', function (e) {
                            timer.stop();
                            e.target.merged = L.geoJson(e.data.merged);
                            e.target.merged.setStyle(config.styles.shapes['regions']['normal']);
                            state.turfCache.insert(key, e.target.merged);
                            state.boundariesInProcessing.erase(key);
                        }, false);
                        state.boundariesInProcessing.insert(key, key);
                        worker.postMessage({"shapes": turf.featurecollection(boundaries)});
                    } else {
                        // TODO: Show the regions boundaries without merging
                        e.target.polygons = [];
                        var regionBoundary;
                        for (var boundary in boundaries) {
                            regionBoundary = L.geoJson(boundaries[boundary]);
                            regionBoundary.setStyle(config.styles.shapes['regions']['normal']);
                            e.target.polygons.push(regionBoundary);
                            regionBoundary.addTo(state.map);
                        }
                    }

                }, tools.defErrorCB);
            }
        }

        var names = e.target.getChildClustersNames();
        var text = "";
        if (names.length > 0) {
            for (var i in names) {
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
        if (e.target.merged) {
            state.map.removeLayer(e.target.merged);
        } else if (e.target.polygons && e.target.polygons.length) {
            for (var polygon in e.target.polygons) {
                state.map.removeLayer(e.target.polygons[polygon]);
            }
        }
        map.closePopups();
    });

});
