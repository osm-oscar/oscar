requirejs.config({
    baseUrl: "js/libs",
    config: {
        'oscar': {url: "http://oscar.fmi.uni-stuttgart.de/oscar"}
    },
    paths: {
        "jquery": "jquery/jquery.min",
        "spin": "spin/spin.min",
        "leaflet": "leaflet/leaflet.0.7.2",
        "leafletCluster": "leaflet/leaflet.markercluster.min",
        "sidebar": "leaflet/leaflet-sidebar.min",
        "jdataview": "jdataview/jdataview",
        "jbinary": "jbinary/jbinary",
        "sserialize": "sserialize/sserialize.min",
        "bootstrap": "twitter-bootstrap/js/bootstrap.min",
        "oscar": "oscar/oscar",
        "moment": "moment/moment.min",
        "mustache": "mustache/mustache.min",
        "mustacheLoader": "mustache/jquery.mustache.loader",
        "jqueryui": "jquery-ui/jquery-ui.min",
        "slimbox": "slimbox/js/slimbox2",
        "tools": "tools/tools",
        "conf": "config/config",
        "menu": "menu/menu",
        "flickr": "flickr/flickr",
        "manager": "connection/manager.min",
        "switch": "switch-button/jquery.switchButton.min",
        "d3": "dagre-d3/d3.min",
        "dagre-d3": "dagre-d3/dagre-d3.min",
        "tree": "tree/tree",
        "tokenfield": "tokenfield/bootstrap-tokenfield.min"
    },
    shim: {
        'bootstrap': {deps: ['jquery']},
        'leafletCluster': {deps: ['leaflet', 'jquery']},
        'sidebar': {deps: ['leaflet', 'jquery']},
        'mustacheLoader': {deps: ['jquery']},
        'slimbox': {deps: ['jquery']},
        'switch': {deps: ['jquery']}
    },
    waitSeconds: 20
});

requirejs(["oscar", "leaflet", "jquery", "bootstrap", "jbinary", "mustache", "jqueryui", "leafletCluster", "spin", "sidebar", "mustacheLoader", "tools", "conf", "menu", "tokenfield", "switch", "tree", "flickr"],
    function (oscar, L, jQuery, bootstrap, jbinary, mustache, jqueryui, leafletCluster, spinner, sidebar, mustacheLoader, tools, config, menu, tokenfield, switchButton, tree, flickr) {
        //main entry point

        var osmAttr = '&copy; <a target="_blank" href="http://www.openstreetmap.org">OpenStreetMap</a>';
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
        $("#help").load('template/help.html', function () {
            $('.example-query-string').on('click', function () {
                $("#search_text").tokenfield('createToken', {value: this.firstChild.data, label: this.firstChild.data});
                state.sidebar.open("search");
            });
        });

        menu.displayCategories();

        // init the map and sidebar
        state.map = L.map('map', {
            zoomControl: true
        }).setView([48.74568, 9.1047], 17);
        state.map.zoomControl.setPosition('topright');
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
                "postcode": postcode,
                "city": city,
                "street": street,
                "housenumber": houseNumber,
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
                state.timers.loadingSpinner = setTimeout(displayLoadingSpinner, config.timeouts.loadingSpinner);
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
            var sampleStep = 1.0 / config.geoquery.samplecount;
            var sampleCount = config.geoquery.samplecount;
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

            state.geoquery.mapshape = L.polygon(pts, config.styles.shapes.geoquery.normal);
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
            state.timers.geoquery = setTimeout(endGeoQuerySelect, config.timeouts.geoquery_select);
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
                state.timers.pathquery = setTimeout(endPathQuery, config.timeouts.pathquery.select);
            }
            if (state.pathquery.mapshape === undefined) {
                state.pathquery.mapshape = L.polyline([], config.styles.shapes.pathquery.highlight).addTo(state.map);
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
            state.timers.pathquery = setTimeout(endPathQuery, config.timeouts.pathquery.select);
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
            if (config.functionality.shapes.highlightListItemOnClick[shapeSrcType]) {
                leafletItem.on('click', function () {
                    state.sidebar.open("search");
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
        }

        function clearHighlightedShapes(shapeSrcType) {
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
                    itemShape.setStyle(config.styles.shapes.items.normal);

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
                lfi.setStyle(config.styles.shapes[shapeSrcType]['highlight']);
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
                        leafLetItem.setStyle(config.styles.shapes[shapeSrcType]['highlight']);
                        state[shapeSrcType].shapes.highlighted[itemId] = itemId;
                        addShapeToMap(leafLetItem, itemId, shapeSrcType);
                    },
                    defErrorCB
                );
                oscar.getItem(itemId,
                    function (item) {
                        //state.map.fitBounds(item.bbox());
                    }, defErrorCB);
            }
        }

        function appendItemToListView(item, shapeSrcType, parentElement) {
            if (item === undefined) {
                console.log("Undefined item displayed");
                return;
            }
            var itemId = item.id();
            if (state[shapeSrcType].listview.drawn.count(itemId)) {
                return;
            }
            state[shapeSrcType].listview.drawn.insert(itemId, item);

            var itemTemplateData = resultListTemplateDataFromItem(item, shapeSrcType);
            var rendered = $.Mustache.render('itemListEntryHtmlTemplate', itemTemplateData);
            var inserted = $($(rendered).appendTo(parentElement));
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

        function closePopups() {
            if ($(".leaflet-popup-close-button")[0] !== undefined) {
                $(".leaflet-popup-close-button")[0].click();
            }
        }

        function flatCqrTreeDataSource(cqr) {
            function getItems(regionChildrenInfo, context) {
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
                        var itemMap = {}, node, item, itemId, marker;

                        // modify DAG
                        if (cqr.ohPath().length && !context.dynamic) {
                            for (var i in items) {
                                item = items[i];
                                itemId = item.id();
                                itemMap[itemId] = item;
                                if ($.inArray(itemId, cqr.ohPath()) != -1 || (parentRid == cqr.ohPath()[cqr.ohPath().length - 1] && parentCount > oscar.maxFetchItems)) {
                                    node = parentNode.addChild(itemId);
                                    node.count = regionChildrenApxItemsMap[itemId];
                                    node.bbox = item.bbox();
                                    node.name = item.name();
                                    marker = L.marker(item.centerPoint());
                                    marker.count = regionChildrenApxItemsMap[item.id()];
                                    marker.rid = item.id();
                                    marker.name = item.name();
                                    marker.bbox = item.bbox();
                                    decorateMarker(marker);
                                    node.marker = marker;
                                    state.DAG.insert(itemId, node);
                                }
                            }
                        } else {
                            if (parentCount > oscar.maxFetchItems) {
                                for (var i in items) {
                                    item = items[i];
                                    itemId = item.id();
                                    itemMap[itemId] = item;

                                    node = parentNode.addChild(itemId);
                                    node.count = regionChildrenApxItemsMap[itemId];
                                    node.bbox = item.bbox();
                                    node.name = item.name();
                                    marker = L.marker(item.centerPoint());
                                    marker.count = regionChildrenApxItemsMap[item.id()];
                                    marker.rid = item.id();
                                    marker.name = item.name();
                                    marker.bbox = item.bbox();
                                    decorateMarker(marker);
                                    node.marker = marker;
                                    state.DAG.insert(itemId, node);
                                }
                            }
                        }

                        // download locations, if end of hierarchy is reached or the region counts less than maxFetchItems
                        if (!items.length || (parentCount < oscar.maxFetchItems)) {
                            if (!context.draw && cqr.ohPath().length && cqr.ohPath()[cqr.ohPath().length - 1] != context.rid) {

                            } else {
                                $("#left_menu_parent").css("display", "block");
                                state.items.listview.selectedRegionId = context.rid;
                                state.cqr.regionItemIds(state.items.listview.selectedRegionId,
                                    getItemIds,
                                    defErrorCB,
                                    0 // offset
                                );
                            }
                        } else if (context.draw) {
                            cqr.getMaximumIndependetSet(parentRid, 0, function (regions) {
                                for (var i in items) {
                                    if ($.inArray(items[i].id(), regions) == -1 && state.DAG.at(items[i].id()) !== undefined) {
                                        state.DAG.at(items[i].id()).kill();
                                        delete state.DAG.at(items[i]);
                                        state.DAG.erase(items[i].id());
                                    }
                                }
                                state.items.clusters.drawn.erase(parentRid);

                                // just load regionShapes into the cache
                                //oscar.getShapes(regions, function (res) {
                                //}, defErrorCB);

                                var j;
                                for (var i in regions) {
                                    j = itemMap[regions[i]];

                                    if (!state.items.clusters.drawn.count(j.id())) {
                                        marker = state.DAG.at(j.id()).marker;
                                        state.items.clusters.drawn.insert(j.id(), marker);
                                        state.markers.addLayer(marker);
                                        if (state.DAG.at(marker.rid) !== undefined) {
                                            state.DAG.at(marker.rid).marker = marker;
                                        } else {
                                            console.log("FAIL: no marker!");
                                        }
                                    }
                                }

                                if (state.visualizationActive) {
                                    tree.refresh(parentRid);
                                }

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
                    var node;
                    state.items.clusters.drawn.erase(regionId);

                    // manage items -> kill old items if there are too many of them and show clusters again
                    if (state.items.listview.drawn.size() + items.length > config.maxBufferedItems) {
                        for (var i in state.items.listview.drawn.values()) {
                            node = state.DAG.at(i);
                            for (var parent in node.parents) {
                                if (!state.items.clusters.drawn.count(node.parents[parent].id)) {
                                    state.markers.addLayer(node.parents[parent].marker);
                                    state.items.clusters.drawn.insert(node.parents[parent].id, node.parents[parent].marker);
                                }
                            }
                            if (node.marker) {
                                state.markers.removeLayer(node.marker);
                            } else {
                                for (var parent in node.parents) {
                                    if (!state.items.clusters.drawn.at(node.parents[parent].id)) {
                                        console.log("Undefined marker!"); // this shouldn't happen => displayed search results would get killed
                                    }
                                }
                            }
                            node.kill();
                            delete node;
                        }
                        //clearListAndShapes("items");
                        $('#itemsList').empty();
                        $('#tabs').empty();
                        state.items.listview.drawn.clear();
                        state.items.shapes.drawn.clear();
                    }

                    var isInitNecessary = $('#tabs')[0].children.length;
                    var tab = "<li><a href='#tab-" + regionId + "'>" + state.DAG.at(regionId).name + " [" + items.length + "]" + "</a></li>";
                    $('#tabs').append(tab);
                    if (isInitNecessary == 0) {
                        $('#items_parent').tabs();
                    } else {
                        $('#items_parent').tabs("refresh");
                    }

                    var regionDiv = "<div id='tab-" + regionId + "'></div>";
                    $('#itemsList').append(regionDiv);
                    var parentElement = $('#tab-' + regionId);
                    for (var i in items) {
                        var item = items[i];
                        var itemId = item.id();
                        if (state.items.listview.promised.count(itemId)) {
                            state.DAG.insert(itemId, state.DAG.at(regionId).addChild(itemId));
                            appendItemToListView(item, "items", parentElement);
                            state.items.listview.promised.erase(itemId);
                        }
                    }
                    $('#items_parent').tabs("refresh");

                    visualizeResultListItems();

                    if (state.items.listview.drawn.size() === 1) {
                        for (var i in state.items.listview.drawn.values()) {
                            $('#itemsNameLink' + i).click();
                            break;
                        }
                    }

                    if (state.visualizationActive) {
                        tree.refresh(regionId);
                    }

                },
                defErrorCB
            );
        }

        function decorateMarker(marker) {
            marker.on("click", function (e) {
                closePopups();
                state.items.clusters.drawn.erase(e.target.rid);
                removeMarker(e.target);
                /*state.regionHandler({
                 rid: e.target.rid,
                 draw: true,
                 bbox: state.DAG.at(e.target.rid).bbox
                 });*/
                drawClusters(state.DAG.at(e.target.rid));
            });

            marker.on("mouseover", function (e) {
                /*if (oscar.isShapeInCache(e.target.rid)) {
                 oscar.getShape(e.target.rid, function (shape) {
                 var leafletItem = oscar.leafletItemFromShape(shape);
                 leafletItem.setStyle(config.styles.shapes['regions']['normal']);
                 e.target.shape = leafletItem;
                 state.map.addLayer(leafletItem);
                 }, defErrorCB);
                 }*/
                //e.target.rect = L.rectangle(e.target.bbox);
                //e.target.rect.addTo(state.map);

                L.popup({offset: new L.Point(0, -10)})
                    .setLatLng(e.latlng)
                    .setContent(e.target.name).openOn(state.map);
            });

            marker.on("mouseout", function (e) {
                closePopups();
                if (e.target.shape) {
                    state.map.removeLayer(e.target.shape);
                }
                /*if(e.target.rect){
                 state.map.removeLayer(e.target.rect);
                 }*/
            });
        }

        function pathProcessor(cqr) {
            return {
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
                        state.handler = function () {
                            if (this.path && this.path.length) {
                                // start at the target region (last element of ohPath)
                                drawClusters(state.DAG.at(this.path[this.path.length - 1]));
                            } else {
                                // start at Node "World"
                                drawClusters(state.DAG.at(0xFFFFFFFF));
                            }
                        }.bind(this);
                        state.map.on("zoomend dragend", state.handler);
                    }
                }
            };
        }

        function recursiveRemoveChildrenFromMap(a) {
            var markerCluster, markerItem;
            var node;

            for (var child in a.children) {
                node = a.children[child];
                markerCluster = state.items.clusters.drawn.at(node.id);
                markerItem = state.items.shapes.drawn.at(node.id);
                if (markerCluster !== undefined) {
                    removeMarker(markerCluster);
                    state.items.clusters.drawn.erase(node.id);
                    recursiveRemoveChildrenFromMap(node);
                } else if (markerItem !== undefined) {
                    if ($("a[href='#tab-" + node.parents[0].id + "']").length) {
                        $("a[href='#tab-" + node.parents[0].id + "']").parent().remove();
                        $("#tab-" + node.parents[0].id).remove();
                        $('#items_parent').tabs("refresh");
                    }
                    state.markers.removeLayer(node.marker);
                    state.items.shapes.drawn.erase(node.id);
                    state.items.listview.drawn.erase(node.id);
                }
            }
        }

        function drawClusters(node) {
            if (!node) {
                return;
            }
            var childNode;
            var marker;

            if (node.children.length) {
                for (var child in node.children) {
                    childNode = node.children[child];
                    if (tools.percentOfOverlap(state.map, childNode.bbox) >= config.overlap) {
                        if (state.items.clusters.drawn.count(childNode.id)) {
                            removeMarker(childNode.marker);
                            state.items.clusters.drawn.erase(childNode.id);
                        }
                        drawClusters(childNode);
                    } else {
                        marker = childNode.marker;
                        if (!state.items.clusters.drawn.count(childNode.id) && childNode.count) {
                            state.items.clusters.drawn.insert(childNode.id, marker);
                            state.markers.addLayer(marker);
                            recursiveRemoveChildrenFromMap(childNode);
                        } else if (!state.items.shapes.drawn.count(childNode.id) && !childNode.count) {
                            state.items.shapes.drawn.insert(childNode.id, marker);
                            state.markers.addLayer(marker);
                            if (!$("a[href='#tab-" + childNode.parents[0].id + "']").length) {
                                var tab = "<li><a href='#tab-" + childNode.parents[0].id + "'>" + state.DAG.at(childNode.parents[0].id).name + " [" + childNode.parents[0].count + "]" + "</a></li>";
                                $('#tabs').append(tab);
                            }
                            if (!$("#tab-" + childNode.parents[0].id).length) {
                                var regionDiv = "<div id='tab-" + childNode.parents[0].id + "'></div>";
                                $('#itemsList').append(regionDiv);
                            }
                            oscar.getItem(childNode.id, function (item) {
                                appendItemToListView(item, "items", $("#tab-" + childNode.parents[0].id));
                            }, defErrorCB);
                        }
                    }
                }
                try {
                    $('#items_parent').tabs("refresh");
                    $("#items_parent").tabs("option", "active", 0);
                } catch (exception) {
                }

            } else if (node.count) {
                state.regionHandler({rid: node.id, draw: true, dynamic: true});
            }

        }

        function displayCqr(cqr) {
            clearViews();
            state.regionHandler = flatCqrTreeDataSource(cqr);
            var process = pathProcessor(cqr);
            var root = new tools.TreeNode(0xFFFFFFFF, undefined);
            root.count = cqr.rootRegionApxItemCount();
            root.name = "World";
            state.DAG.insert(0xFFFFFFFF, root);

            if (cqr.ohPath().length) {
                state.regionHandler({rid: 0xFFFFFFFF, draw: false, pathProcessor: process});
            } else {
                state.regionHandler({rid: 0xFFFFFFFF, draw: true, pathProcessor: process});
            }
        }

        function removeMarker(marker) {
            if (marker.shape) {
                state.map.removeLayer(marker.shape);
            }
            state.markers.removeLayer(marker);
            closePopups();
        }

        function doCompletion() {
            if ($("#search_text").val() === state.queries.lastQuery) {
                return;
            }

            if ($('#searchModi input').is(":checked")) {
                //TODO: wrong placement of markers if clsutering is aktive. Cause: region midpoint is outside of search rectangle
                addSingleQueryStatementToQuery("$geo:" + state.map.getBounds().getSouthWest().lng
                    + "," + state.map.getBounds().getSouthWest().lat + ","
                    + state.map.getBounds().getNorthEast().lng + "," + state.map.getBounds().getNorthEast().lat);
            }

            $("#showCategories a").click();
            state.sidebar.open("search");
            $("#flickr").hide("slide", {direction: "right"}, config.styles.slide.speed);

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
            state.timers.query = setTimeout(instantCompletion, config.timeouts.query);
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

            $('#tree').resizable()

            $("#search_form").click(function () {
                if (!$('#categories').is(":visible")) {
                    $("#showCategories a").click();
                }
            });

            $("#showCategories a").click(function () {
                if ($(this).attr('mod') == 'hide') {
                    $('#categories').show(800);
                    $('#subCategories').show(800);
                    $(this).attr('mod', 'show');
                    $(this).text("Hide categories");
                } else {
                    $('#categories').hide(800);
                    $('#subCategories').hide(800);
                    $(this).attr('mod', 'hide');
                    $(this).text("Show categories");

                }
            });

            $('#advancedToggle a').click(function () {
                if ($(this).attr('mod') == 'hide') {
                    $('#advancedSearch').hide(800);
                    $(this).attr('mod', 'show');
                    $(this).text('Show advanced search');
                } else {
                    $('#advancedSearch').show(800);
                    $(this).attr('mod', 'hide');
                    $(this).text('Hide advanced search');
                }
            });

            $('#graph').click(function () {
                tree.visualizeDAG(state.DAG.at(0xFFFFFFFF), state);
            });

            $('#closeTree a').click(function () {
                state.visualizationActive = false;
                $('#tree').css("display", "none");
            });

            $('#closeFlickr a').click(function () {
                $("#flickr").hide("slide", {direction: "right"}, config.styles.slide.speed);
            });

            $("#searchModi input").switchButton({
                on_label: 'Local',
                off_label: 'Global'
            });

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

            $('#search_text').tokenfield({minWidth: 250, delimiter: "|"});
            $($('#search_form')[0].children).css("width", "100%");
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

            //setup search field
            $('#search_text').bind('change', delayedCompletion);
            $('#search_text').bind('keyup', delayedCompletion);
            $('#search_form').bind('submit', function (e) {
                e.preventDefault();
                instantCompletion();
            });

            $(window).bind('popstate', function (e) {
                queryFromSearchLocation();
            });

            //check if there's a query in our location string
            queryFromSearchLocation();

        });
    });