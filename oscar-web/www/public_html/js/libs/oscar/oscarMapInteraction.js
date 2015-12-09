requirejs.config({
    baseUrl: "js/libs",
    config: {
        'oscar': {url: "http://oscardev.fmi.uni-stuttgart.de/oscar"}
    },
    paths: {
        "jquery": "jquery/jquery",
        "spin": "spin/spin",
        "leaflet": "leaflet/leaflet.0.7.2",
        "leafletCluster": "leaflet/leaflet.markercluster-src",
        "sidebar": "leaflet/leaflet-sidebar",
        "jdataview": "jdataview/jdataview",
        "jbinary": "jbinary/jbinary",
        "sserialize": "sserialize/sserialize",
        "bootstrap": "twitter-bootstrap/js/bootstrap",
        "oscar": "oscar/oscar",
        "fuelux": "fuelux/js/fuelux",
        "moment": "moment/moment.min",
        "mustache": "mustache/mustache",
        "mustacheLoader": "mustache/jquery.mustache.loader",
        "jqueryui": "jqueryui/jquery-ui"
    },
    shim: {
        'bootstrap': {deps: ['jquery']},
        'leafletCluster': {deps: ['leaflet', 'jquery']},
        'sidebar': {deps: ['leaflet', 'jquery']},
        'mustacheLoader': {deps: ['jquery']}
    },
    waitSeconds: 10
});

requirejs(["oscar", "leaflet", "jquery", "bootstrap", "fuelux", "jbinary", "mustache", "jqueryui", "leafletCluster", "spin", "sidebar", "mustacheLoader"],
    function (oscar, L, jQuery, bootstrap, fuelux, jbinary, mustache, jqueryui, leafletCluster, spinner, sidebar, mustacheLoader) {
        //main entry point

        var osmAttr = '&copy; <a target="_blank" href="http://www.openstreetmap.org">OpenStreetMap</a>';
        var state = {
            clustering: true, // gets set either when oscar.maxFetchItems is smaller than the final cell in ohPath or ohPath isn't even set in cqr
            map: {},
            DAG: SimpleHash(), // represents the hierarchie-tree for a query
            markers: L.markerClusterGroup(),
            sidebar: undefined,
            regions: {
                promised: {},//referenced by id
                drawn: SimpleHash(),//id -> [Leaflet-Item]
                highlighted: {}//id -> leaflet-item (stored in drawn)
            },
            items: {
                shapes: {
                    promised: {},//referenced by id
                    drawn: SimpleHash(),//id -> [leaflet-item]
                    highlighted: {}//id -> id (stored in drawn)
                },
                listview: {
                    promised: SimpleHash(),//referenced by id
                    drawn: SimpleHash(),//referenced by id
                    loadsmore: false, //currently loads more into the result list
                    selectedRegionId: undefined
                },
                clusters: {
                    drawn: SimpleHash(),//id -> marker
                }
            },
            relatives: {
                shapes: {
                    promised: {},//referenced by id
                    drawn: SimpleHash(),//id -> [leaflet-item]
                    highlighted: {}//id -> id (stored in drawn)
                },
                listview: {
                    promised: SimpleHash(),//referenced by id
                    drawn: SimpleHash()//referenced by id
                }
            },
            loadingtasks: 1, //number of tasks currently loading, initially 1 for this page
            cqr: {},
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
            spinner: new spinner(myConfig.spinnerOpts)
        };

        L.MarkerCluster.prototype.on("mouseover", function (e) {
            var names = e.target.getChildClustersNames();
            var text = "";
            for (var i in names) {
                text += names[i];
                if (i < names.length - 1) {
                    text += ", ";
                }
            }
            L.popup().setLatLng(e.latlng).setContent(text).openOn(state.map);
        });

        // mustache-template-loader needs this
        window.Mustache = mustache;

        // load template files
        $.Mustache.load('template/itemListEntryTemplate.mst');
        $.Mustache.load('template/treeTemplate.mst');
        $("#help").load('template/help.html');
        menu.displayCategories();

        // init the map and sidebar
        state.map = L.map('map').setView([48.74568, 9.1047], 17);
        state.sidebar = L.control.sidebar('sidebar').addTo(map);
        L.tileLayer('http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {attribution: osmAttr}).addTo(state.map);

        var defErrorCB = function (textStatus, errorThrown) {
            console.log("xmlhttprequest error textstatus=" + textStatus + "; errorThrown=" + errorThrown);
            if (confirm("Error occured. Refresh automatically?")) {
                location.reload();
            }
        };

        function isMatchedTag(key, value) {
            var testString = key + ":" + value;
            var res = state.cqrRegExp.test(testString);
            return res;
        }

        function resultListTemplateDataFromItem(item, shapeSrcType) {
            var itemKv = [];
            var itemUrls = [];
            var wikiLink = undefined;
            var hasMatchingTag = false;
            var protoRegExp = /^.*:\/\//;

            for (var i = 0; i < item.size(); ++i) {
                var itemKey = item.key(i);
                var itemValue = item.value(i);
                if (itemKey === "wikipedia") {
                    if (protoRegExp.test(itemValue)) {
                        itemUrls.push({"k": itemKey, "v": itemValue});
                    }
                    else {
                        wikiLink = itemValue;
                    }
                }
                else if (myConfig.resultsview.urltags[itemKey] !== undefined) {
                    if (!protoRegExp.test(itemValue)) {
                        itemValue = "http://" + itemValue;
                    }
                    itemUrls.push({"k": itemKey, "v": itemValue});
                }
                else {
                    var tmp = {"k": itemKey, "v": itemValue};
                    if (isMatchedTag(itemKey, itemValue)) {
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
                "matchingTagClass": (hasMatchingTag ? "name-has-matched-tag" : false),
                "wikilink": wikiLink,
                "urlkvs": itemUrls,
                "kv": itemKv //{k,v, kc, vc}
            };
        }

        function displayLoadingSpinner() {
            if (state.timers.loadingSpinner !== undefined) {
                clearTimeout(state.timers.loadingSpinner);
                state.timers.loadingSpinner = undefined;
            }
            if (state.loadingtasks > 0) {
                var target = document.getElementById('loader');
                state.spinner.spin(target);
            }
        }

        function startLoadingSpinner() {
            if (state.timers.loadingSpinner === undefined) {
                state.timers.loadingSpinner = setTimeout(displayLoadingSpinner, myConfig.timeouts.loadingSpinner);
            }
            state.loadingtasks += 1;
        }

        function endLoadingSpinner() {
            if (state.loadingtasks === 1) {
                state.loadingtasks = 0;
                if (state.timers.loadingSpinner !== undefined) {
                    clearTimeout(state.timers.loadingSpinner);
                    state.timers.loadingSpinner = undefined;
                }
                state.spinner.stop();
            }
            else {
                state.loadingtasks -= 1;
            }
        };

        function clearGeoQueryMapShape() {
            if (state.geoquery.mapshape !== undefined) {
                state.map.removeLayer(state.geoquery.mapshape);
                state.geoquery.mapshape = undefined;
            }
        }

        function clearGeoQuerySelect() {
            endGeoQuerySelect();
            $('#geoquery_acceptbutton').removeClass('btn-info');
            $('#geoquery_selectbutton').html('Select rectangle');
            state.geoquery.active = false;
            clearGeoQueryMapShape();
        }

        function updateGeoQueryMapShape() {
            clearGeoQueryMapShape();
            var sampleStep = 1.0 / myConfig.geoquery.samplecount;
            var sampleCount = myConfig.geoquery.samplecount;
            var pts = [];

            function fillPts(sLat, sLng, tLat, tLng) {
                var latDiff = (tLat - sLat) * sampleStep;
                var lngDiff = (tLng - sLng) * sampleStep;
                for (var i = 0; i < sampleCount; ++i) {
                    pts.push(L.latLng(sLat, sLng));
                    sLat += latDiff;
                    sLng += lngDiff;
                }
            }

            var minLat = parseFloat($('#geoquery_minlat').val());
            var maxLat = parseFloat($('#geoquery_maxlat').val());
            var minLng = parseFloat($('#geoquery_minlon').val());
            var maxLng = parseFloat($('#geoquery_maxlon').val());

            fillPts(minLat, minLng, minLat, maxLng);
            fillPts(minLat, maxLng, maxLat, maxLng);
            fillPts(maxLat, maxLng, maxLat, minLng);
            fillPts(maxLat, minLng, minLat, minLng);

            state.geoquery.mapshape = L.polygon(pts, myConfig.styles.shapes.geoquery.normal);
            state.map.addLayer(state.geoquery.mapshape);

        }

        function geoQuerySelect(e) {
            if (state.geoquery.clickcount === 0) {
                $('#geoquery_minlat').val(e.latlng.lat);
                $('#geoquery_minlon').val(e.latlng.lng);
                $('[data-class="geoquery_min_coord"]').removeClass('bg-info');
                $('[data-class="geoquery_max_coord"]').addClass('bg-info');
                state.geoquery.clickcount = 1;
            }
            else if (state.geoquery.clickcount === 1) {
                $('#geoquery_maxlat').val(e.latlng.lat);
                $('#geoquery_maxlon').val(e.latlng.lng);
                state.geoquery.clickcount = 2;
                endGeoQuerySelect();
            }
        }

        function endGeoQuerySelect() {
            if (state.timers.geoquery !== undefined) {
                clearTimeout(state.timers.geoquery);
                state.timers.geoquery = undefined;
            }
            $('#geoquery_selectbutton').removeClass("btn-info");
            $('[data-class="geoquery_min_coord"]').removeClass('bg-info');
            $('[data-class="geoquery_max_coord"]').removeClass('bg-info');
            state.map.removeEventListener('click', geoQuerySelect);
            $('#geoquery_acceptbutton').addClass('btn-info');
            updateGeoQueryMapShape();
        }

        function startGeoQuerySelect() {
            if (state.timers.geoquery !== undefined) {
                clearTimeout(state.timers.geoquery);
            }
            $('#geoquery_acceptbutton').removeClass('btn-info');
            state.geoquery.clickcount = 0;
            state.geoquery.active = true;
            $('#geoquery_selectbutton').addClass("btn-info");
            $('#geoquery_selectbutton').html('Clear rectangle');
            $('[data-class="geoquery_min_coord"]').addClass('bg-info');
            state.map.on('click', geoQuerySelect);
            state.timers.geoquery = setTimeout(endGeoQuerySelect, myConfig.timeouts.geoquery_select);
        }

        function resetPathQuery() {
            if (state.timers.pathquery !== undefined) {
                clearTimeout(state.timers.pathquery);
            }
            state.map.removeEventListener('click', pathQuerySelect);
            state.map.removeLayer(state.pathquery.mapshape);
            state.pathquery.mapshape = undefined;
            state.pathquery.selectButtonState = 'select';
            $('#pathquery_acceptbutton').removeClass("btn-info");
            $('#pathquery_selectbutton').removeClass("btn-info");
            $('#pathquery_selectbutton').html('Select path');
        }

        function pathQuerySelect(e) {
            if (state.timers.pathquery !== undefined) {
                clearTimeout(state.timers.pathquery);
                state.timers.pathquery = setTimeout(endPathQuery, myConfig.timeouts.pathquery.select);
            }
            if (state.pathquery.mapshape === undefined) {
                state.pathquery.mapshape = L.polyline([], myConfig.styles.shapes.pathquery.highlight).addTo(state.map);
            }
            state.pathquery.mapshape.addLatLng(e.latlng);
        }

        function endPathQuery() {
            if (state.timers.pathquery !== undefined) {
                clearTimeout(state.timers.pathquery);
            }
            state.map.removeEventListener('click', pathQuerySelect);
            $('#pathquery_acceptbutton').addClass("btn-info");
            $('#pathquery_selectbutton').removeClass("btn-info");
            $('#pathquery_selectbutton').html('Clear path');
            state.pathquery.selectButtonState = 'clear';
        }

        function startPathQuery() {
            if (state.timers.pathquery !== undefined) {
                clearTimeout(state.timers.pathquery);
            }
            state.pathquery.points = []
            $('#pathquery_acceptbutton').removeClass("btn-info");
            $('#pathquery_selectbutton').addClass("btn-info");
            $('#pathquery_selectbutton').html("Finish path");
            state.pathquery.selectButtonState = 'finish';
            state.map.on('click', pathQuerySelect);
            state.timers.pathquery = setTimeout(endPathQuery, myConfig.timeouts.pathquery.select);
        }

        function addSingleQueryStatementToQuery(qstr) {
            var searchInput = $('#search_text');
            qstr = searchInput.val() + " " + qstr.replace(' ', '\\ ');
            searchInput.val(qstr);
            searchInput.change();
        }

        function addShapeToMap(leafletItem, itemId, shapeSrcType) {
            var itemDetailsId = '#' + shapeSrcType + 'Details' + itemId;
            var itemPanelRootId = '#' + shapeSrcType + 'PanelRoot' + itemId;
            if (myConfig.functionality.shapes.highlightListItemOnClick[shapeSrcType]) {
                leafletItem.on('click', function () {
                    $('#' + shapeSrcType + "List").find('.panel-collapse').each(
                        function () {
                            if ($(this).hasClass('in')) {
                                $(this).collapse('hide');
                            }
                        }
                    );
                    $(itemDetailsId).collapse('show');
                    showItemRelatives(itemId);
                    //var container = $('#'+ shapeSrcType +'_parent');
                    var container = $(".sidebar-content");
                    var itemPanelRootDiv = $(itemPanelRootId);
                    if (itemPanelRootDiv === undefined) {
                        console.log("addShapeToMap: undefined PanelRoot", leafletItem, itemId, shapeSrcType, state);
                    }
                    else {
                        var scrollPos = itemPanelRootDiv.offset().top - container.offset().top + container.scrollTop();
                        container.animate({scrollTop: scrollPos});
                        //container.animate({scrollTop: $(itemsDetailsId).offset().top});
                    }
                    highlightShape(itemId, shapeSrcType);
                });
            }
        }

        function clearListAndShapes(shapeSrcType) {
            $('#' + shapeSrcType + 'List').empty();
            for (var i in state[shapeSrcType].shapes.drawn.values()) {
                state.map.removeLayer(state[shapeSrcType].shapes.drawn.at(i));
                state[shapeSrcType].shapes.drawn.erase(i);
            }
            state[shapeSrcType].listview.drawn.clear();
            state[shapeSrcType].listview.promised.clear();
            state[shapeSrcType].shapes.highlighted = {};
            state[shapeSrcType].shapes.promised = {};

        }

        function clearViews() {
            $('#itemsList').empty();
            $('#itemsList_counter').empty();
            $('#relativesList').empty();
            $('#relativesList_counter').empty();
            $('#relativesList_itemId').empty();
            state.map.removeLayer(state.markers);
            delete state.markers;
            state.markers = L.markerClusterGroup();
            state.regions.highlighted = {};
            state.regions.promised = {};
            state.items.listview.drawn.clear();
            state.items.listview.promised.clear();
            state.items.listview.selectedRegionId = undefined;
            state.items.shapes.highlighted = {};
            state.items.shapes.promised = {};
            state.relatives.listview.drawn.clear();
            state.relatives.listview.promised.clear();
            state.relatives.shapes.highlighted = {};
            state.relatives.shapes.promised = {};
            for (var i in state.regions.drawn.values()) {
                state.map.removeLayer(state.regions.drawn.at(i));
                state.regions.drawn.erase(i);
            }
            for (var i in state.items.shapes.drawn.values()) {
                state.map.removeLayer(state.items.shapes.drawn.at(i));
                state.items.shapes.drawn.erase(i);
            }
            for (var i in state.relatives.shapes.drawn.values()) {
                state.map.removeLayer(state.relatives.shapes.drawn.at(i));
                state.relatives.shapes.drawn.erase(i);
            }
            for (var i in state.items.clusters.drawn.values()) {
                state.map.removeLayer(state.items.clusters.drawn.at(i));
                state.items.clusters.drawn.erase(i);
            }
            $('#MyTree').tree('destroy');
            $('<ul>', {'id': "MyTree", 'class': "tree", 'role': "tree"}).appendTo('#fuelux_tree_parent');
            $('#MyTree').append($.Mustache.render('tree_template', null));
            $('#search_results_counter').empty();
        }

        function clearHighlightedShapes(shapeSrcType) {
            for (var i in state[shapeSrcType].shapes.highlighted) {
                if (state[shapeSrcType].shapes.drawn[i] === undefined) {
                    state.map.removeLayer(state[shapeSrcType].shapes.drawn.at(i));
                    state[shapeSrcType].shapes.drawn.erase(i);
                }
                else {
                    state[shapeSrcType].shapes.drawn.at(i).setStyle(myConfig.styles.shapes[shapeSrcType]['normal']);
                }
            }
            state[shapeSrcType].shapes.highlighted = {};
        }

        function clearVisualizedItems() {
            state.items.shapes.promised = {};
            state.items.shapes.highlighted = {};
            for (var i in state.items.shapes.drawn.values()) {
                if (state.items.shapes.highlighted[i] === undefined) {
                    state.map.removeLayer(state.items.shapes.drawn.at(i));
                }
            }
            for (var i in state.items.clusters.drawn.values()) {
                state.map.removeLayer(state.items.clusters.drawn.at(i));
            }
            state.items.clusters.drawn.clear();
            state.items.shapes.drawn.clear();
        }

        function visualizeResultListItems() {
            state.items.shapes.promised = {};
            var itemsToDraw = [];
            for (var i in state.items.listview.drawn.values()) {
                if (!state.items.shapes.drawn.count(i)) {
                    state.items.shapes.promised[i] = state.items.listview.drawn.at(i);
                    itemsToDraw.push(i);
                }
            }

            oscar.getShapes(itemsToDraw, function (shapes) {
                endLoadingSpinner();
                var marker;
                for (var i in itemsToDraw) {
                    var itemId = itemsToDraw[i];
                    if (state.items.shapes.promised[itemId] === undefined) {
                        continue;
                    }
                    if (shapes[itemId] === undefined || !state.items.listview.drawn.count(itemId) ||
                        state.items.shapes.drawn.count(itemId)) {
                        delete state.items.shapes.promised[itemId];
                        continue;
                    }

                    delete state.items.shapes.promised[itemId];
                    var itemShape = oscar.leafletItemFromShape(shapes[itemId]);
                    itemShape.setStyle(myConfig.styles.shapes.items.normal);

                    if (itemShape instanceof L.MultiPolygon) {
                        marker = L.marker(itemShape.getLatLngs()[0][0]);
                    } else if (itemShape instanceof L.Polygon) {
                        marker = L.marker(itemShape.getLatLngs()[0]);
                    } else if (itemShape instanceof L.Polyline) {
                        marker = L.marker(itemShape.getLatLngs()[0]);
                    } else {
                        marker = L.marker(itemShape.getLatLng());
                    }

                    state.markers.addLayer(marker);
                    state.items.shapes.drawn.insert(itemId, itemShape);
                    addShapeToMap(marker, itemId, "items");

                }
                state.map.addLayer(state.markers);
            }, defErrorCB);
        }

        function highlightShape(itemId, shapeSrcType) {
            if (state[shapeSrcType].shapes.highlighted[itemId] !== undefined) {//already highlighted
                return;
            }
            if (state[shapeSrcType].shapes.drawn.count(itemId)) { //this already on the map, change the style
                var lfi = state[shapeSrcType].shapes.drawn.at(itemId);
                clearHighlightedShapes(shapeSrcType);
                state[shapeSrcType].shapes.highlighted[itemId] = lfi;
                lfi.setStyle(myConfig.styles.shapes[shapeSrcType]['highlight']);
                state.map.fitBounds(lfi.getBounds());
            }
            else {
                state[shapeSrcType].shapes.promised = {};
                state[shapeSrcType].shapes.promised[itemId] = itemId;
                oscar.getShape(itemId,
                    function (shape) {
                        if ((state[shapeSrcType].shapes.promised[itemId] === undefined) || (state[shapeSrcType].shapes.drawn[itemId] !== undefined)) {
                            return;
                        }
                        clearHighlightedShapes(shapeSrcType);
                        var leafLetItem = oscar.leafletItemFromShape(shape);
                        leafLetItem.setStyle(myConfig.styles.shapes[shapeSrcType]['highlight']);
                        state[shapeSrcType].shapes.highlighted[itemId] = itemId;
                        addShapeToMap(leafLetItem, itemId, shapeSrcType);
                    },
                    defErrorCB
                );
                oscar.getItem(itemId,
                    function (item) {
                        state.map.fitBounds(item.bbox());
                    }, defErrorCB);
            }
        }

        function appendItemToListView(item, shapeSrcType) {
            if (item === undefined) {
                console.log("Undefined item displayed");
                return;
            }
            var itemId = item.id();
            if (state[shapeSrcType].listview.drawn.count(itemId)) {
                return;
            }
            state[shapeSrcType].listview.drawn.insert(itemId, itemId);

            var itemTemplateData = resultListTemplateDataFromItem(item, shapeSrcType);
            var rendered = $.Mustache.render('itemListEntryHtmlTemplate', itemTemplateData);
            var inserted = $($(rendered).appendTo('#' + shapeSrcType + 'List'));
            $('#' + shapeSrcType + 'NameLink' + itemId, inserted).click(
                function () {
                    highlightShape(itemId, shapeSrcType);
                    if (shapeSrcType !== "relatives") {
                        showItemRelatives(itemId);
                    }
                }
            );

            function itemDetailQuery(e) {
                var me = $(this);
                var myKey = me.attr('data-query-key');
                if (myKey === undefined) {
                    return false;
                }
                var myQstr = "@" + myKey;
                var myValue = me.attr('data-query-value');
                if (myValue !== undefined) {
                    myQstr += ":" + myValue;
                }
                addSingleQueryStatementToQuery(myQstr);
                return false;
            }

            $('#' + shapeSrcType + 'Details' + itemId + " .item-detail-key", inserted).click(itemDetailQuery);
            $('#' + shapeSrcType + 'Details' + itemId + " .item-detail-value", inserted).click(itemDetailQuery);
            $('#' + shapeSrcType + 'List_counter').empty().append("(" + state[shapeSrcType].listview.drawn.size() + ")");
        }

        function showItemRelatives(itemId) {
            if (!$('#show_item_relatives_checkbox').is(":checked")) {
                return;
            }
            oscar.getItemsRelativesIds(itemId, function (relativesIds) {
                oscar.getItems(relativesIds, function (relatives) {
                    clearListAndShapes("relatives");
                    for (var i in relatives) {
                        appendItemToListView(relatives[i], "relatives");
                    }
                    {
                        var tmp = $('#relativesList_itemId');
                        tmp.empty();
                        tmp.append("" + itemId);
                    }
                }, defErrorCB);
            }, defErrorCB);
        }

        function regionLayersToFront() {
            for (var i in state.regions.drawn.values()) {
                state.regions.drawn.values()[i].bringToFront();
            }
        }

        function updateMapRegions(regions) {
            state.regions.promised = {};
            var availableMapRegions = {};
            var fetchMapRegions = [];
            for (var rid in regions) {
                if (state.regions.drawn.count(rid)) {
                    availableMapRegions[rid] = state.regions.drawn.at(rid);
                    state.regions.drawn.erase(rid);
                }
                else {
                    fetchMapRegions.push(rid);
                }
            }
            //remove unused regions from map
            for (var rid in state.regions.drawn.values()) {
                state.map.removeLayer(state.regions.drawn.at(rid));
                state.regions.drawn.erase(rid);
            }
            //replace old state.regions.drawn with new one
            state.regions.drawn.clear();
            for (var i in availableMapRegions) {
                state.regions.drawn.insert(i, availableMapRegions[i]);
            }
            //fetch missing shapes
            if (fetchMapRegions.length) {
                for (var i in fetchMapRegions) {
                    state.regions.promised[fetchMapRegions[i]] = fetchMapRegions[i];
                }
                startLoadingSpinner();
                oscar.getShapes(fetchMapRegions,
                    function (shapes) {
                        endLoadingSpinner();
                        for (var shapeId in shapes) {
                            if (state.regions.promised[shapeId] !== undefined && !state.regions.drawn.count(shapeId)) {
                                delete state.regions.promised[shapeId];
                                var shape = shapes[shapeId];
                                var leafletItem = oscar.leafletItemFromShape(shape);
                                leafletItem.setStyle(myConfig.styles.shapes['regions']['normal']);
                                state.regions.drawn.insert(shapeId, leafletItem);
                                state.markers.addLayer(leafletItem);
                            }
                        }
                        state.map.addLayer(state.markers);
                        regionLayersToFront();
                    },
                    defErrorCB
                );
            }
            else {
                regionLayersToFront();
            }
        }

        function displayRegionsItem() {
            var cqr = state.cqr;
            var rid = state.items.listview.selectedRegionId;
            if (cqr === undefined || rid === undefined) {
                return;
            }
            clearHighlightedShapes("items");
            clearVisualizedItems();
            state.items.listview.selectedRegionId = rid;
            //don't fetch bbox of "world"
            if (rid !== 0xFFFFFFFF) {
                oscar.getItem(rid, function (item) {
                    state.map.fitBounds(item.bbox());
                    var mapRegion = state.regions.drawn.at(rid);
                    if (mapRegion !== undefined) {
                        for (var i in state.regions.highlighted) {
                            state.regions.highlighted[i].setStyle(myConfig.styles.shapes.regions.normal);
                            delete state.regions.highlighted[i];
                        }
                        state.regions.highlighted[rid] = mapRegion;
                        mapRegion.setStyle(myConfig.styles.shapes.regions.highlight);
                    }
                }, defErrorCB);
            }
            else {
                state.map.fitWorld();
            }
            $('#itemsList').empty();
            state.items.listview.drawn.clear();
            state.items.listview.promised.clear();
        }

        ///returns the region ids of opened regions in dest
        ///This only takes care of regions with children (inner nodes)
        function getOpenRegions(node, dest) {
            $(node).children('.tree\\-branch\\-children').not('.hidden').children('.tree\\-branch.tree\\-open').each(function () {
                var rid = $(this).attr('rid');
                dest[rid] = rid;
                getOpenRegions(this, dest);
            });
        }

        function getOpenRegionsInTree() {
            var dest = {};
            $('#MyTree').children('.tree\\-branch.tree\\-open').each(function () {
                var rid = $(this).attr('rid');
                dest[rid] = rid;
                getOpenRegions(this, dest);
            });
            //and get the selected regions
            var selectedRegions = $('#MyTree').tree('selectedItems');
            for (var i in selectedRegions) {
                var rid = selectedRegions[i].dataAttributes.rid;
                if (rid !== 0xFFFFFFFF) { //filter root
                    dest[rid] = rid;
                }
            }
            return dest;
        }

        function fullSubSetTreeDataSource(cqr) {
            return function (options, callback) {
                var subSet = cqr.subSet();
                var regionChildren = [];
                if (options.dataAttributes !== undefined) {
                    regionChildren = subSet.regions[options.dataAttributes.rid].children;
                }
                else {
                    regionChildren = subSet.rootchildren;
                }

                //fetch the items
                oscar.getItems(regionChildren,
                    function (items) {
                        var res = {data: []};
                        if (options.dataAttributes !== undefined) {
                            res.data.push({
                                name: options.name,
                                type: 'item',
                                dataAttributes: options.dataAttributes
                            });
                        }
                        for (var i in items) {
                            var item = items[i];
                            var itemId = item.id();
                            var regionInSubSet = subSet.regions[itemId];
                            res.data.push({
                                name: item.name() + ( regionInSubSet.apxitems > 0 ? " [~" + regionInSubSet.apxitems + "]" : ""),
                                type: ((regionInSubSet.children !== undefined && regionInSubSet.children.length) ? 'folder' : 'item'),
                                dataAttributes: {rid: itemId}
                            });
                        }
                        res.data.sort(function (a, b) {
                            var aSize = subSet.regions[a.dataAttributes.rid].apxitems;
                            var bSize = subSet.regions[b.dataAttributes.rid].apxitems;
                            return bSize - aSize;
                        });
                        callback(res);
                    },
                    defErrorCB
                );
            };
        }

        function flatCqrTreeDataSource(cqr) {
            function getItems(regionChildrenInfo, options, callback) {
                //fetch the items
                var regionChildrenApxItemsMap = {};
                var childIds = [];
                for (var i in regionChildrenInfo) {
                    var ci = regionChildrenInfo[i];
                    regionChildrenApxItemsMap[ci['id']] = ci['apxitems'];
                    childIds.push(ci['id']);
                }
                oscar.getItems(childIds,
                    function (items) {
                        var res = {data: []};
                        var itemMap = {};

                        if (options.dataAttributes !== undefined) {
                            res.data.push({
                                name: options.name,
                                type: 'item',
                                dataAttributes: options.dataAttributes
                            });
                        }
                        else {//add a World item
                            res.data.push({
                                name: "Everything [~" + cqr.rootRegionApxItemCount() + "]",
                                type: 'item',
                                dataAttributes: {rid: 0xFFFFFFFF, 'data-rid': 0xFFFFFFFF}
                            });
                        }

                        var parentRid = options.dataAttributes !== undefined ? options.dataAttributes.rid : undefined;

                        for (var i in items) {
                            var item = items[i];
                            var itemId = item.id();
                            var apxItems = regionChildrenApxItemsMap[itemId];
                            var parentNode = state.DAG.at(parentRid);
                            itemMap[itemId] = item;

                            state.DAG.insert(itemId, parentNode !== undefined ? parentNode.addChild(itemId) : new TreeNode(itemId, undefined));

                            res.data.push({
                                name: item.name() + ( apxItems > 0 ? " [~" + apxItems + ":" + itemId + "]" : ""),
                                type: 'folder',
                                bbox: item.bbox(),
                                dataAttributes: {rid: itemId, 'data-rid': itemId, "cnt": apxItems}
                            });
                        }

                        if (!items.length || (options.dataAttributes && options.dataAttributes["cnt"] < oscar.maxFetchItems)) {
                            state.items.listview.selectedRegionId = options.rid;
                            state.cqr.regionItemIds(state.items.listview.selectedRegionId,
                                getItemIds,
                                defErrorCB,
                                0 // offset
                            );
                            state.map.fitBounds(options.bbox);
                        } else if ((cqr.d.ohPath.length && parentRid !== undefined && (cqr.d.ohPath[cqr.d.ohPath.length - 1] == parentRid || state.DAG.at(parentRid).hasParentWithId(cqr.d.ohPath[cqr.d.ohPath.length - 1]))) || !cqr.d.ohPath.length) {
                            cqr.getMaximumIndependetSet(parentRid, function (regions) {
                                var j;
                                for (var i in regions) {
                                    j = itemMap[regions[i]];
                                    var marker = L.marker(j.centerPoint());
                                    marker.count = regionChildrenApxItemsMap[j.id()];
                                    marker.rid = j.id();
                                    marker.name = j.name();

                                    marker.on("click", function (e) {
                                        $(".leaflet-popup-close-button")[0].click(); // close all opened popups
                                        state.items.clusters.drawn.erase(e.target.rid);
                                        state.markers.removeLayer(e.target);
                                        $($("li[class='tree-branch'][rid='" + e.target.rid + "']").children()[0]).children()[0].click();
                                    });

                                    marker.on("mouseover", function (e) {
                                        L.popup()
                                            .setLatLng(e.latlng)
                                            .setContent(e.target.name).openOn(state.map);
                                    });

                                    if (!state.items.clusters.drawn.count(j.id())) {
                                        state.items.clusters.drawn.insert(j.id(), marker);
                                        state.markers.addLayer(marker);
                                    }
                                }

                                state.map.addLayer(state.markers);
                                if (!cqr.d.ohPath.length || state.clustering) {
                                    if (options.bbox) {
                                        state.map.fitBounds(options.bbox);
                                    } else {
                                        state.map.fitWorld();
                                    }
                                }

                            }, defErrorCB);
                        }

                        callback(res);
                    },
                    defErrorCB
                );
            }

            return function (options, callback) {
                var rid;
                if (options.dataAttributes !== undefined) {
                    rid = options.dataAttributes.rid;
                }
                else {
                    rid = undefined;
                }
                startLoadingSpinner();
                cqr.regionChildrenInfo(rid,
                    function (regionChildrenInfo) {
                        getItems(regionChildrenInfo, options, callback);
                    },
                    defErrorCB
                );
            };
        }

        function getItemIds(itemIds) {

            for (var i in itemIds) {
                var itemId = itemIds[i];
                state.items.listview.promised.insert(itemId, itemId);
            }

            oscar.getItems(itemIds,
                function (items) {
                    endLoadingSpinner();
                    for (var i in items) {
                        var item = items[i];
                        var itemId = item.id();
                        if (state.items.listview.promised.count(itemId)) {
                            appendItemToListView(item, "items");
                            state.items.listview.promised.erase(itemId);
                        }
                    }

                    visualizeResultListItems();

                    if (state.items.listview.drawn.size() === 1) {
                        for (var i in state.items.listview.drawn.values()) {
                            $('#itemsNameLink' + i).click();
                            break;
                        }
                    }
                },
                defErrorCB
            );
        }

        function displayCqrAsTree(cqr) {
            clearViews();
            $("#left_menu_parent").css("display", "block");
            var myTree = $('#MyTree');
            var myDataSource;

            if (cqr.isFull()) {
                myDataSource = fullSubSetTreeDataSource(cqr);
            }
            else {
                myDataSource = flatCqrTreeDataSource(cqr);
            }

            myTree.tree({
                dataSource: myDataSource,
                multiSelect: false,
                cacheItems: true,
                folderSelect: false
            });

            myTree.on('selected.fu.tree', function (e, node) {
                if (node !== undefined && node.selected !== undefined) {
                    for (var i in node.selected) {
                        state.items.listview.selectedRegionId = node.selected[i].dataAttributes.rid;
                        displayRegionsItem();
                        break;
                    }
                    updateMapRegions(getOpenRegionsInTree());
                }
            });

            myTree.on('deselected.fu.tree', function (e, node) {
                if (node !== undefined && node.selected !== undefined) {
                    updateMapRegions(getOpenRegionsInTree());
                }
            });

            myTree.on('disclosedFolder.fu.tree', function (e, node) {
                updateMapRegions(getOpenRegionsInTree());
            });

            myTree.on('closed.fu.tree', function (e, node) {
                updateMapRegions(getOpenRegionsInTree());
            });

            //open the tree if cqr.ohPath is available
            if (cqr.ohPath().length) {
                // decide whether clustering is necessary
                state.clustering = false;
                outer:
                    for (var i in cqr.d.regionInfo) {
                        if (cqr.d.regionInfo == cqr.ohPath()[cqr.ohPath().length - 1]) {
                            continue;
                        } else {
                            for (var j in cqr.d.regionInfo[i]) {
                                if (cqr.d.regionInfo[i][j].id == cqr.ohPath()[cqr.ohPath().length - 1]) {
                                    if (cqr.d.regionInfo[i][j].apxitems > oscar.maxFetchItems) {
                                        state.clustering = true;
                                    }
                                    break outer;
                                }
                            }
                        }
                    }

                var myCqr = cqr;
                var ohPath = myCqr.ohPath();
                var ohPathPos = 0;
                var evHandler = function (e, node) {
                    expand();
                };

                function expand() {
                    if (ohPathPos >= ohPath.length) {
                        //FullCQR currently has no opening hints, no need to check for that;
                        var mySelectNode = $("[class*='tree\\-item']" + '[rid=' + ohPath[ohPathPos - 1] + "]", myTree);
                        if (mySelectNode.length && mySelectNode.attr("cnt") <= oscar.maxFetchItems) {
                            var mySearchNode = $('[data-rid=' + ohPath[ohPathPos - 1] + '] .tree\\-branch\\-header', myTree);
                            myTree.off('loaded.fu.tree', evHandler);
                            var scrollPos = mySearchNode.offset().top - myTree.offset().top + myTree.scrollTop();
                            myTree.animate({scrollTop: scrollPos});
                            myTree.tree('selectItem', mySelectNode);
                        }
                    }
                    else {
                        var mySearchNode = $('[data-rid=' + ohPath[ohPathPos] + '] .tree\\-branch\\-header', myTree);
                        if (mySearchNode.length) {
                            ++ohPathPos;
                            myTree.tree('discloseFolder', mySearchNode);
                        }
                    }
                };
                myTree.on('loaded.fu.tree', evHandler);
                expand();

            } else {
                // no path available -> cluster available regions
                state.clustering = true;
                myTree.on('loaded.fu.tree', function (e, node) {
                    var a = $($(node).children("[class=tree-branch-children]")).children("[class=tree-branch]");
                    if (a.length == 1) {
                        myTree.tree('openFolder', a[0]);
                    }

                });
            }
        }

        function doCompletion() {
            if ($("#search_text").val() === state.queries.lastQuery) {
                return;
            }
            state.sidebar.open("search");

            //query has changed, ddos the server! TODO: call off all ajax loads from earlier calls
            var myQuery = $("#search_text").val();
            state.queries.lastQuery = myQuery + "";//make sure state hold a copy

            var ohf = (parseInt($('#ohf_spinner').val()) / 100.0);
            var globalOht = $('#oht_checkbox').is(':checked');
            //ready for lift-off
            var myQueryCounter = state.queries.lastSubmited + 0;
            state.queries.lastSubmited += 1;
            var callFunc;
            //call sequence starts
            if ($("#subset_type_checkbox").is(':checked')) {
                callFunc = function (q, scb, ecb) {
                    oscar.completeFull(q, scb, ecb);
                };
            }
            else {
                callFunc = function (q, scb, ecb) {
                    oscar.completeSimple(q, scb, ecb, ohf, globalOht);
                };
            }

            //ignite loading feedback
            startLoadingSpinner();

            //push our query as history state
            window.history.pushState({"q": myQuery}, undefined, location.pathname + "?q=" + encodeURIComponent(myQuery));

            //lift-off
            callFunc(myQuery,
                function (cqr) {
                    //orbit reached, iniate coupling with user
                    if (state.queries.lastReturned < myQueryCounter) {
                        state.queries.lastReturned = myQueryCounter + 0;
                        state.queries.activeCqrId = cqr.sequenceId();
                        state.cqr = cqr;
                        state.cqrRegExp = oscar.cqrRexExpFromQuery(cqr.query());
                        displayCqrAsTree(cqr);
                    }
                    endLoadingSpinner();
                },
                function (jqXHR, textStatus, errorThrown) {
                    //BOOM!
                    alert("Failed to retrieve completion results. textstatus=" + textStatus + "; errorThrown=" + errorThrown);
                    endLoadingSpinner();
                });
        }

        function instantCompletion() {
            if (state.timers.query !== undefined) {
                clearTimeout(state.timers.query);
            }
            doCompletion();
        }

        function delayedCompletion() {
            if (state.timers.query !== undefined) {
                clearTimeout(state.timers.query);
            }
            state.timers.query = setTimeout(instantCompletion, myConfig.timeouts.query);
        }

        //https://stackoverflow.com/questions/901115/how-can-i-get-query-string-values-in-javascript
        function getParameterByName(name) {
            name = name.replace(/[\[]/, "\\[").replace(/[\]]/, "\\]");
            var regex = new RegExp("[\\?&]" + name + "=([^&#]*)"),
                results = regex.exec(location.search);
            return results === null ? "" : decodeURIComponent(results[1].replace(/\+/g, " "));
        }

        function queryFromSearchLocation() {
            var myQ = getParameterByName("q");
            if (myQ.length) {
                $('#search_text').val(myQ);
                instantCompletion();
            }
        }

        $(document).ready(function () {
            //setup config panel

            $('#show_item_relatives_checkbox').bind('change',
                function () {
                    var helpOn = $('#show_help_checkbox').is(':checked');
                    var relativesOn = $('#show_item_relatives_checkbox').is(':checked');
                    if (relativesOn) {
                        $('#relatives_parent').removeClass('hidden');
                        //fetch parents of currently selected item
                        for (var i in state.items.shapes.highlighted) {
                            showItemRelatives(i);
                            break;
                        }
                    }
                    else {
                        $('#relatives_parent').addClass('hidden');
                        clearHighlightedShapes('relatives');
                    }
                    if (helpOn || relativesOn) {
                        $('#right_menu_parent').removeClass('hidden');
                    }
                    else {
                        $('#right_menu_parent').addClass('hidden');
                    }
                }
            );

            $('#geoquery_selectbutton').click(function () {
                if (state.geoquery.active) {
                    clearGeoQuerySelect();
                }
                else {
                    startGeoQuerySelect();
                }
            });

            $('#geoquery_acceptbutton').click(function () {
                //first fetch the coordinates
                var minlat = parseFloat($('#geoquery_minlat').val());
                var minlon = parseFloat($('#geoquery_minlon').val());
                var maxlat = parseFloat($('#geoquery_maxlat').val());
                var maxlon = parseFloat($('#geoquery_maxlon').val());

                //end the geoquery
                clearGeoQuerySelect();

                var tmp;
                if (minlat > maxlat) {
                    tmp = maxlat;
                    maxlat = minlat;
                    minlat = tmp;
                }
                if (minlon > maxlon) {
                    tmp = maxlon;
                    maxlon = minlon;
                    minlon = tmp;
                }
                var q = "$geo:" + minlon + "," + minlat + "," + maxlon + "," + maxlat;
                var st = $('#search_text');
                st.val(st.val() + " " + q);
                st.change();
                clearGeoQueryMapShape();
            });

            $('#geoquery_minlat, #geoquery_maxlat, #geoquery_minlon, #geoquery_maxlon').change(function (e) {
                updateGeoQueryMapShape();
            });

            $('#pathquery_selectbutton').click(function () {
                if (state.pathquery.selectButtonState === 'select') {
                    startPathQuery();
                }
                else if (state.pathquery.selectButtonState === 'finish') {
                    endPathQuery();
                }
                else {
                    resetPathQuery();
                }
            });

            $('#pathquery_acceptbutton').click(function () {
                endPathQuery();
                if (state.pathquery.mapshape !== undefined) {
                    var latLngs = state.pathquery.mapshape.getLatLngs();
                    if (latLngs.length) {
                        var qStr = "$path:" + $('#pathquery_radius').val();

                        for (var i in latLngs) {
                            qStr += "," + latLngs[i].lat + "," + latLngs[i].lng;
                        }

                        var st = $('#search_text');
                        st.val(st.val() + " " + qStr);
                        st.change();
                    }
                }
                resetPathQuery();
            });

            //setup help
            $('.example-query-string').bind('click', function () {
                var st = $('#search_text');
                st.val(this.firstChild.data);
                st.change();
            });

            //setup search field
            $('#search_text').bind('change', delayedCompletion);
            $('#search_text').bind('keyup', delayedCompletion);
            $('#search_form').bind('submit', function (e) {
                e.preventDefault();
                instantCompletion();
            });
            $('#search_button').bind('click', instantCompletion);

            $(window).bind('popstate', function (e) {
                queryFromSearchLocation();
            });

            //check if there's a query in our location string
            queryFromSearchLocation();
            endLoadingSpinner();
        });
    });