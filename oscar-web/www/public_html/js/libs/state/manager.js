define(["jquery", "mustache", "tools", "leaflet", "spin","conf", "leafletCluster"], function ($, mustache, tools, L, spinner, config) {
    var state = {
        map: {},
        visualizationActive: false,
        DAG: tools.SimpleHash(),
        markers: L.markerClusterGroup(),
        sidebar: undefined,
        handler: undefined,
        items: {
            shapes: {
                promised: {},//referenced by id
                drawn: tools.SimpleHash(),//id -> marker
                highlighted: {}//id -> id (stored in drawn)
            },
            listview: {
                promised: tools.SimpleHash(),//referenced by id
                drawn: tools.SimpleHash(),//referenced by id
                selectedRegionId: undefined
            },
            clusters: {
                drawn: tools.SimpleHash()//id -> marker
            }
        },
        loadingtasks: 0,
        cqr: {},
        regionHandler: undefined,
        cqrRegExp: undefined,
        queries: {
            activeCqrId: -1,
            lastReturned: -1,
            lastSubmited: 0,
            lastQuery: "",
            queryInBound: false
        },
        timers: {
            geoquery: undefined,
            pathquery: undefined,
            query: undefined,
            loadingSpinner: undefined
        },
        geoquery: {
            active: false,
            clickcount: 0,
            mapshape: undefined
        },
        pathquery: {
            mapshape: undefined,
            selectButtonState: 'select'
        },
        domcache: {
            searchResultsCounter: undefined
        },
        spinner: new spinner(config.spinnerOpts),

        clearListAndShapes: function (shapeSrcType) {
            $('#' + shapeSrcType + 'List').empty();
            state.map.removeLayer(state.markers);
            for (var i in state[shapeSrcType].shapes.drawn.values()) {
                state.markers.removeLayer(state[shapeSrcType].shapes.drawn.at(i));
                //state.markers.removeLayer(state.items.shapes.drawn.at(i));
                state[shapeSrcType].shapes.drawn.erase(i);
            }
            state.map.addLayer(state.markers);
            state[shapeSrcType].listview.drawn.clear();
            //state[shapeSrcType].listview.promised.clear();
            state[shapeSrcType].shapes.highlighted = {};
            state[shapeSrcType].shapes.promised = {};

        },

        isMatchedTag: function (key, value) {
            var testString = key + ":" + value;
            var res = state.cqrRegExp.test(testString);
            return res;
        },

        resultListTemplateDataFromItem: function (item, shapeSrcType) {
            var itemKv = [];
            var itemUrls = [];
            var wikiLink = undefined;
            var hasMatchingTag = false;
            var protoRegExp = /^.*:\/\//;
            var postcode, street, city, houseNumber;

            for (var i = 0; i < item.size(); ++i) {
                var itemKey = item.key(i);
                var itemValue = item.value(i);

                switch (itemKey) {
                    case "addr:city":
                        city = itemValue;
                        break;
                    case "addr:postcode":
                        postcode = itemValue;
                        break;
                    case "addr:street":
                        street = itemValue;
                        break;
                    case "addr:housenumber":
                        houseNumber = itemValue;
                        break;
                }

                if (itemKey === "wikipedia") {
                    if (protoRegExp.test(itemValue)) {
                        itemUrls.push({"k": itemKey, "v": itemValue});
                    }
                    else {
                        wikiLink = itemValue;
                    }
                }
                else if (config.resultsview.urltags[itemKey] !== undefined) {
                    if (!protoRegExp.test(itemValue)) {
                        itemValue = "http://" + itemValue;
                    }
                    itemUrls.push({"k": itemKey, "v": itemValue});
                }
                else {
                    var tmp = {"k": itemKey, "v": itemValue};
                    if (state.isMatchedTag(itemKey, itemValue)) {
                        tmp["kc"] = "matched-key-color";
                        tmp["vc"] = "matched-value-color";
                        hasMatchingTag = true;
                    }
                    itemKv.push(tmp);
                }
            }
            return {
                "shapeSrcType": shapeSrcType,
                "itemId": item.id(),
                "score": item.score(),
                "osmId": item.osmid(),
                "osmType": item.type(),
                "itemName": item.name(),
                "postcode": postcode,
                "city": city,
                "street": street,
                "housenumber": houseNumber,
                "matchingTagClass": (hasMatchingTag ? "name-has-matched-tag" : false),
                "wikilink": wikiLink,
                "urlkvs": itemUrls,
                "kv": itemKv //{k,v, kc, vc}
            };
        },

        clearViews: function () {
            $('#itemsList').empty();
            $('#tabs').empty();
            $('#relativesList').empty();
            $('#relativesList_counter').empty();
            $('#relativesList_itemId').empty();
            if (state.handler !== undefined) {
                state.map.off("zoomend dragend", state.handler);
            }
            state.map.removeLayer(state.markers);
            delete state.markers;
            state.markers = L.markerClusterGroup();
            state.map.addLayer(state.markers);
            state.items.listview.drawn.clear();
            state.items.listview.promised.clear();
            state.items.listview.selectedRegionId = undefined;
            state.items.shapes.highlighted = {};
            state.items.shapes.promised = {};

            for (var i in state.items.shapes.drawn.values()) {
                state.map.removeLayer(state.items.shapes.drawn.at(i));
                state.items.shapes.drawn.erase(i);
            }
            for (var i in state.items.clusters.drawn.values()) {
                state.map.removeLayer(state.items.clusters.drawn.at(i));
                state.items.clusters.drawn.erase(i);
            }
            state.DAG = tools.SimpleHash(); // TODO: kill whole Tree?
            $('<ul>', {'id': "MyTree", 'class': "tree", 'role': "tree"}).appendTo('#fuelux_tree_parent');
            $('#MyTree').append($.Mustache.render('tree_template', null));
            $('#search_results_counter').empty();
        },

        clearHighlightedShapes: function (shapeSrcType) {
            for (var i in state[shapeSrcType].shapes.highlighted) {
                if (state[shapeSrcType].shapes.drawn.at(i) === undefined) {
                    state.map.removeLayer(state[shapeSrcType].shapes.drawn.at(i));
                    state[shapeSrcType].shapes.drawn.erase(i);
                }
                else {
                    state[shapeSrcType].shapes.drawn.at(i).setStyle(config.styles.shapes[shapeSrcType]['normal']);
                }
            }
            state[shapeSrcType].shapes.highlighted = {};
        }
    };

    return state;
});
