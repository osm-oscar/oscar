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
        "moment": "moment/moment.min",
        "mustache": "mustache/mustache",
        "mustacheLoader": "mustache/jquery.mustache.loader",
        "jqueryui": "jquery-ui/jquery-ui",
        "slimbox": "slimbox/js/slimbox2",
        "tools": "tools/tools",
        "conf": "config/config",
        "menu": "menu/menu",
        "flickr": "flickr/flickr",
        "manager": "connection/manager"
    },
    shim: {
        'bootstrap': {deps: ['jquery']},
        'leafletCluster': {deps: ['leaflet', 'jquery']},
        'sidebar': {deps: ['leaflet', 'jquery']},
        'mustacheLoader': {deps: ['jquery']},
        'slimbox': {deps: ['jquery']},
    },
    waitSeconds: 10
});

requirejs(["oscar", "leaflet", "jquery", "bootstrap", "jbinary", "mustache", "jqueryui", "leafletCluster", "spin", "sidebar", "mustacheLoader", "slimbox", "tools", "conf", "menu", "flickr", "manager"],
    function (oscar, L, jQuery, bootstrap, jbinary, mustache, jqueryui, leafletCluster, spinner, sidebar, mustacheLoader, slimbox, tools, config, menu, flickr, manager) {
        //main entry point
        var osmAttr = '&copy; <a target="_blank" href="http://www.openstreetmap.org">OpenStreetMap</a>';
        var state = {
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
            loadingtasks: 0,
            oldZoomLevel: undefined,
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
            spinner: new spinner(myConfig.spinnerOpts)
        };

        // show names of subregions of a cluster in a popup
        L.MarkerCluster.prototype.on("mouseover", function (e) {
            var names = e.target.getChildClustersNames();
            var text = "";
            if (names.length > 1) {
                for (var i in names) {
                    text += names[i];
                    if (i < names.length - 1) {
                        text += ", ";
                    }
                }
                L.popup({offset: new L.Point(0, -10)}).setLatLng(e.latlng).setContent(text).openOn(state.map);
            }
        });

        L.MarkerCluster.prototype.on("mouseout", function (e) {
            closePopups();
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
            endLoadingSpinner();
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
                    //highlightShape(itemId, shapeSrcType);
                });
            }
        }

        function clearListAndShapes(shapeSrcType) {
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
            state.map.addLayer(state.markers);
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
            state.DAG = SimpleHash(); // TODO: kill whole Tree?
            //$('#MyTree').tree('destroy');
            $('<ul>', {'id': "MyTree", 'class': "tree", 'role': "tree"}).appendTo('#fuelux_tree_parent');
            $('#MyTree').append($.Mustache.render('tree_template', null));
            $('#search_results_counter').empty();
        }

        function clearHighlightedShapes(shapeSrcType) {
            for (var i in state[shapeSrcType].shapes.highlighted) {
                if (state[shapeSrcType].shapes.drawn.at(i) === undefined) {
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

            startLoadingSpinner();
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
                    state.DAG.at(itemId).marker = marker;
                    addShapeToMap(marker, itemId, "items");
                }
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

                    var geopos;
                    var itemShape = state.items.shapes.drawn.at(itemId);
                    if (itemShape instanceof L.MultiPolygon) {
                        geopos = itemShape.getLatLngs()[0][0];
                    } else if (itemShape instanceof L.Polygon) {
                        geopos = itemShape.getLatLngs()[0];
                    } else if (itemShape instanceof L.Polyline) {
                        geopos = itemShape.getLatLngs()[0];
                    } else {
                        geopos = itemShape.getLatLng();
                    }

                    if ($('#show_flickr').is(':checked')) {
                        flickr.getImagesForLocation($.trim($(this).text()), geopos);
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

        /*function updateMapRegions(regions) {
         if ($('#load_region_boundaries').is(':checked')) {
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
         }*/

        /*function displayRegionsItem() {
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
                startLoadingSpinner();
                oscar.getItem(rid, function (item) {
                    endLoadingSpinner();

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
        }*/

        function closePopups() {
            if ($(".leaflet-popup-close-button")[0] !== undefined) {
                $(".leaflet-popup-close-button")[0].click();
            }
        }

        function flatCqrTreeDataSource(cqr) {
            function getItems(regionChildrenInfo, context) {
                //fetch the items
                var regionChildrenApxItemsMap = {};
                var childIds = [];
                var parentRid = context.rid;
                var parentNode = state.DAG.at(parentRid);
                var parentCount = parentNode.count;

                for (var i in regionChildrenInfo) {
                    var ci = regionChildrenInfo[i];
                    regionChildrenApxItemsMap[ci['id']] = ci['apxitems'];
                    childIds.push(ci['id']);
                }

                oscar.getItems(childIds,
                    function (items) {
                        var itemMap = {};

                        // modify DAG
                        for (var i in items) {
                            var item = items[i];
                            var itemId = item.id();
                            itemMap[itemId] = item;
                            var node = parentNode.addChild(itemId);
                            node.count = regionChildrenApxItemsMap[itemId];
                            node.bbox = item.bbox();
                            state.DAG.insert(itemId, node);
                        }

                        // download locations, if end of hierarchy is reached or the region counts less than maxFetchItems
                        if (!items.length || (parentCount < oscar.maxFetchItems)) {
                            $("#left_menu_parent").css("display", "block");
                            state.items.listview.selectedRegionId = context.rid;
                            state.cqr.regionItemIds(state.items.listview.selectedRegionId,
                                getItemIds,
                                defErrorCB,
                                0 // offset
                            );
                        } else if (context.draw) {
                            cqr.getMaximumIndependetSet(parentRid, function (regions) {
                                state.items.clusters.drawn.erase(parentRid);

                                var j;
                                for (var i in regions) {
                                    j = itemMap[regions[i]];
                                    var marker = L.marker(j.centerPoint());
                                    marker.count = regionChildrenApxItemsMap[j.id()];
                                    marker.rid = j.id();
                                    marker.name = j.name();
                                    marker.bbox = j.bbox();

                                    marker.on("click", function (e) {
                                        closePopups();
                                        state.items.clusters.drawn.erase(e.target.rid);
                                        state.markers.removeLayer(e.target);
                                        state.regionHandler({
                                            rid: e.target.rid,
                                            draw: true,
                                            bbox: state.DAG.at(e.target.rid).bbox
                                        });
                                    });

                                    marker.on("mouseover", function (e) {
                                        L.popup({offset: new L.Point(0, -10)})
                                            .setLatLng(e.latlng)
                                            .setContent(e.target.name).openOn(state.map);
                                        e.target.rec = L.rectangle(e.target.bbox, {color: "#ff7800", weight: 1});
                                        e.target.rec.addTo(state.map);
                                    });

                                    marker.on("mouseout", function (e) {
                                        closePopups();
                                        state.map.removeLayer(e.target.rec);
                                    });

                                    if (!state.items.clusters.drawn.count(j.id())) {
                                        state.items.clusters.drawn.insert(j.id(), marker);
                                        state.markers.addLayer(marker);
                                        state.DAG.at(marker.rid).marker = marker;
                                    }
                                }
                                //state.map.addLayer(state.markers);

                            }, defErrorCB);
                        }

                        if (context.pathProcessor) {
                            context.pathProcessor.process();
                        }
                    },
                    defErrorCB
                );
            }

            return function (context) {
                startLoadingSpinner();
                cqr.regionChildrenInfo(context.rid, function (regionChildrenInfo) {
                        endLoadingSpinner()
                        getItems(regionChildrenInfo, context);
                    },
                    defErrorCB
                );
            };
        }

        function getItemIds(regionId, itemIds) {
            for (var i in itemIds) {
                var itemId = itemIds[i];
                state.items.listview.promised.insert(itemId, itemId);
            }

            oscar.getItems(itemIds,
                function (items) {
                    state.items.clusters.drawn.erase(regionId);
                    // manage items -> kill old items if there are too many of them and show clsuters again
                    if (state.items.listview.drawn.size() + items.length > myConfig.maxBufferedItems) {
                        for (var i in state.items.listview.drawn.values()) {
                            for (var parent in state.DAG.at(i).parents) {
                                if (!state.items.clusters.drawn.count(state.DAG.at(i).parents[parent].id)) {
                                    state.markers.addLayer(state.DAG.at(i).parents[parent].marker);
                                    state.items.clusters.drawn.insert(state.DAG.at(i).parents[parent].id, state.DAG.at(i).parents[parent].marker);
                                }
                            }
                            if (state.DAG.at(i).marker !== undefined) {
                                state.markers.removeLayer(state.DAG.at(i).marker);
                            } else {
                                alert("Marker: " + i);
                            }
                            state.DAG.at(i).kill();
                        }
                        //clearListAndShapes("items");
                        state.items.listview.drawn.clear();
                        state.items.shapes.drawn.clear();
                    }

                    for (var i in items) {
                        var item = items[i];
                        var itemId = item.id();
                        if (state.items.listview.promised.count(itemId)) {
                            state.DAG.insert(itemId, state.DAG.at(regionId).addChild(itemId));
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

        function displayCqr(cqr) {

            var pathProcessor = {
                path: cqr.ohPath(),
                i: 0,
                process: function () {
                    var draw;
                    if (this.i < this.path.length) {
                        if (this.i != this.path.length - 1) {
                            draw: false;
                        } else {
                            draw = true;
                        }
                        this.i++;
                        state.regionHandler({rid: this.path[this.i - 1], draw: draw, pathProcessor: this});
                    } else {
                        // fit the viewport to the target region
                        if (this.path.length) {
                            state.map.fitBounds(state.DAG.at(this.path[this.path.length - 1]).bbox);
                        } else {
                            state.map.fitWorld();
                        }
                    }
                }
            };

            clearViews();
            state.regionHandler = flatCqrTreeDataSource(cqr);

            var root = new TreeNode(0xFFFFFFFF, undefined);
            root.count = cqr.rootRegionApxItemCount();
            state.DAG.insert(0xFFFFFFFF, root);
            if (cqr.ohPath().length) {
                state.regionHandler({rid: 0xFFFFFFFF, draw: false, pathProcessor: pathProcessor});
            } else {
                state.regionHandler({rid: 0xFFFFFFFF, draw: true, pathProcessor: pathProcessor});
            }

            state.map.on("zoomstart", function () {
                state.oldZoomLevel = state.map.getZoom();
            });

            state.map.on("zoomend dragend", function () {
                $("#zoom").html("zoom-level: " + state.map.getZoom());
                // zoom-in or dragging
                //if (state.oldZoomLevel <= state.map.getZoom()) {
                state.markers.eachLayer(function (marker) {
                    // first step: get all markers currently shown in the viewport
                    var bounds = state.map.getBounds();
                    if (marker.rid && bounds.contains(marker.getLatLng())) {
                        // second step: we iterate only "leaf"-markers, so there could be a merged cluster viewed -> check whether the
                        // marker has a cluster-parent for the current zoom-level
                        var parent = marker.__parent;
                        while (parent && parent._zoom >= state.map.getZoom()) {
                            // found a locally present cluster -> skip loading additional data
                            if (parent._zoom == state.map.getZoom()) {
                                return;
                            }
                            parent = parent.__parent;
                        }
                        // third step: calculate the overlap of the bbox and the viewport. If it is bigger
                        // than the config-value -> load additional data
                        var node = state.DAG.at(marker.rid);
                        var percent = percentOfOverlap(state.map, state.map.getBounds(), node.bbox);
                        if (!(marker instanceof L.MarkerCluster) && percent >= myConfig.overlap) {
                            removeMarker(marker);
                            state.regionHandler({rid: marker.rid, draw: true, bbox: node.bbox});
                        }
                    }
                });
                //}
            });
        }

        function removeMarker(marker){
            if(marker.rec) {
                state.map.removeLayer(marker.rec);
            }
            state.markers.removeLayer(marker);
            closePopups();
        }

        function doCompletion() {
            if ($("#search_text").val() === state.queries.lastQuery) {
                return;
            }
            state.sidebar.open("search");
            $("#flickr").hide("slide", {direction: "right"}, myConfig.styles.slide.speed);

            //query has changed, ddos the server!
            var myQuery = $("#search_text").val();
            state.queries.lastQuery = myQuery + "";//make sure state hold a copy

            var ohf = (parseInt($('#ohf_spinner').val()) / 100.0);
            var globalOht = $('#oht_checkbox').is(':checked');
            //ready for lift-off
            var myQueryCounter = state.queries.lastSubmited + 0;
            state.queries.lastSubmited += 1;

            var callFunc;
            callFunc = function (q, scb, ecb) {
                oscar.completeSimple(q, scb, ecb, ohf, globalOht);
            };

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
                        displayCqr(cqr);
                    }
                },
                function (jqXHR, textStatus, errorThrown) {
                    //BOOM!
                    alert("Failed to retrieve completion results. textstatus=" + textStatus + "; errorThrown=" + errorThrown);
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

            if (!$('#results_tree_parent').is(':checked')) {
                $('#results_tree_parent').hide();
            }

            $('#show_tree').click(function () {
                $('#results_tree_parent').toggle();
            });

            $('#show_flickr').click(function () {
                var flickr = $("#flickr");
                if (!$(this).is(':checked')) {
                    flickr.hide();
                } else {
                    flickr.show();
                }
            });

            // TODO: parent area example
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
            $('.example-query-string').on('click', function () {
                $('#search_text').val(this.firstChild.data);
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
        });
    });