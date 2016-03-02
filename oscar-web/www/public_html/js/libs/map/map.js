define(["state", "jquery", "conf", "oscar", "flickr", "tools", "tree", "bootstrap"], function (state, $, config, oscar, flickr, tools, tree) {
    return map = {
        /**
         * displays the spinner
         */
        displayLoadingSpinner: function () {
            if (state.timers.loadingSpinner !== undefined) {
                clearTimeout(state.timers.loadingSpinner);
                state.timers.loadingSpinner = undefined;
            }
            if (state.loadingtasks > 0) {
                var target = document.getElementById('loader'); // TODO: use jquery
                state.spinner.spin(target);
            }
        },

        /**
         * start the loading spinner, which displays after a timeout the spinner
         */
        startLoadingSpinner: function () {
            if (state.timers.loadingSpinner === undefined) {
                state.timers.loadingSpinner = setTimeout(map.displayLoadingSpinner, config.timeouts.loadingSpinner);
            }
            state.loadingtasks += 1;
        },

        /**
         * ends a spinner instance
         */
        endLoadingSpinner: function () {
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
        },

        /**
         * Error-function used for interaction with the server
         *
         * @param textStatus
         * @param errorThrown
         */
        defErrorCB: function (textStatus, errorThrown) {
            map.endLoadingSpinner();
            console.log("xmlhttprequest error textstatus=" + textStatus + "; errorThrown=" + errorThrown);
            if (confirm("Error occured. Refresh automatically?")) {
                location.reload();
            }
        },

        clearGeoQueryMapShape: function () {
            if (state.geoquery.mapshape !== undefined) {
                state.map.removeLayer(state.geoquery.mapshape);
                state.geoquery.mapshape = undefined;
            }
        },

        clearGeoQuerySelect: function () {
            map.endGeoQuerySelect();
            $('#geoquery_acceptbutton').removeClass('btn-info');
            $('#geoquery_selectbutton').html('Select rectangle');
            state.geoquery.active = false;
            map.clearGeoQueryMapShape();
        },

        updateGeoQueryMapShape: function () {
            map.clearGeoQueryMapShape();
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

        },

        geoQuerySelect: function (e) {
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
                map.endGeoQuerySelect();
            }
        },

        endGeoQuerySelect: function () {
            if (state.timers.geoquery !== undefined) {
                clearTimeout(state.timers.geoquery);
                state.timers.geoquery = undefined;
            }
            $('#geoquery_selectbutton').removeClass("btn-info");
            $('[data-class="geoquery_min_coord"]').removeClass('bg-info');
            $('[data-class="geoquery_max_coord"]').removeClass('bg-info');
            state.map.removeEventListener('click', map.geoQuerySelect);
            $('#geoquery_acceptbutton').addClass('btn-info');
            map.updateGeoQueryMapShape();
        },

        startGeoQuerySelect: function () {
            if (state.timers.geoquery !== undefined) {
                clearTimeout(state.timers.geoquery);
            }
            $('#geoquery_acceptbutton').removeClass('btn-info');
            state.geoquery.clickcount = 0;
            state.geoquery.active = true;
            $('#geoquery_selectbutton').addClass("btn-info");
            $('#geoquery_selectbutton').html('Clear rectangle');
            $('[data-class="geoquery_min_coord"]').addClass('bg-info');
            state.map.on('click', map.geoQuerySelect);
            state.timers.geoquery = setTimeout(map.endGeoQuerySelect, config.timeouts.geoquery_select);
        },

        resetPathQuery: function () {
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
        },

        pathQuerySelect: function (e) {
            if (state.timers.pathquery !== undefined) {
                clearTimeout(state.timers.pathquery);
                state.timers.pathquery = setTimeout(endPathQuery, config.timeouts.pathquery.select);
            }
            if (state.pathquery.mapshape === undefined) {
                state.pathquery.mapshape = L.polyline([], config.styles.shapes.pathquery.highlight).addTo(state.map);
            }
            state.pathquery.mapshape.addLatLng(e.latlng);
        },

        endPathQuery: function () {
            if (state.timers.pathquery !== undefined) {
                clearTimeout(state.timers.pathquery);
            }
            state.map.removeEventListener('click', map.pathQuerySelect);
            $('#pathquery_acceptbutton').addClass("btn-info");
            $('#pathquery_selectbutton').removeClass("btn-info");
            $('#pathquery_selectbutton').html('Clear path');
            state.pathquery.selectButtonState = 'clear';
        },

        startPathQuery: function () {
            if (state.timers.pathquery !== undefined) {
                clearTimeout(state.timers.pathquery);
            }
            state.pathquery.points = []
            $('#pathquery_acceptbutton').removeClass("btn-info");
            $('#pathquery_selectbutton').addClass("btn-info");
            $('#pathquery_selectbutton').html("Finish path");
            state.pathquery.selectButtonState = 'finish';
            state.map.on('click', map.pathQuerySelect);
            state.timers.pathquery = setTimeout(map.endPathQuery, config.timeouts.pathquery.select);
        },

        addShapeToMap: function (leafletItem, itemId, shapeSrcType) {
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
                    map.showItemRelatives(itemId);
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
        },

        visualizeResultListItems: function () {
            state.items.shapes.promised = {};
            var itemsToDraw = [];
            for (var i in state.items.listview.drawn.values()) {
                if (!state.items.shapes.drawn.count(i)) {
                    state.items.shapes.promised[i] = state.items.listview.drawn.at(i);
                    itemsToDraw.push(i);
                }
            }

            map.startLoadingSpinner();
            oscar.getShapes(itemsToDraw, function (shapes) {
                map.endLoadingSpinner();

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
                    map.addShapeToMap(marker, itemId, "items");
                }
            }, map.defErrorCB);
        },

        highlightShape: function (itemId, shapeSrcType) {
            if (state[shapeSrcType].shapes.highlighted[itemId] !== undefined) {//already highlighted
                return;
            }
            if (state[shapeSrcType].shapes.drawn.count(itemId)) { //this already on the map, change the style
                var lfi = state[shapeSrcType].shapes.drawn.at(itemId);
                state.clearHighlightedShapes(shapeSrcType);
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
                        state.clearHighlightedShapes(shapeSrcType);
                        var leafLetItem = oscar.leafletItemFromShape(shape);
                        leafLetItem.setStyle(config.styles.shapes[shapeSrcType]['highlight']);
                        state[shapeSrcType].shapes.highlighted[itemId] = itemId;
                        map.addShapeToMap(leafLetItem, itemId, shapeSrcType);
                    },
                    map.defErrorCB
                );
                oscar.getItem(itemId,
                    function (item) {
                        //state.map.fitBounds(item.bbox());
                    }, map.defErrorCB);
            }
        },

        appendItemToListView: function (item, shapeSrcType, parentElement) {
            if (item === undefined) {
                console.log("Undefined item displayed");
                return;
            }
            var itemId = item.id();
            /*if (state[shapeSrcType].listview.drawn.count(itemId)) {
             return;
             }*/
            state[shapeSrcType].listview.drawn.insertOrIncrement(itemId, item);

            var itemTemplateData = state.resultListTemplateDataFromItem(item, shapeSrcType);
            var rendered = $.Mustache.render('itemListEntryHtmlTemplate', itemTemplateData);
            var inserted = $($(rendered).appendTo(parentElement));
            $('#' + shapeSrcType + 'NameLink' + itemId, inserted).click(
                function () {
                    map.highlightShape(itemId, shapeSrcType);
                    if (shapeSrcType !== "relatives") {
                        map.showItemRelatives(itemId);
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
                tools.addSingleQueryStatementToQuery(myQstr);
                return false;
            }

            $('#' + shapeSrcType + 'Details' + itemId + " .item-detail-key", inserted).click(itemDetailQuery);
            $('#' + shapeSrcType + 'Details' + itemId + " .item-detail-value", inserted).click(itemDetailQuery);
            $('#' + shapeSrcType + 'List_counter').empty().append("(" + state[shapeSrcType].listview.drawn.size() + ")");
        },

        showItemRelatives: function (itemId) {
            if (!$('#show_item_relatives_checkbox').is(":checked")) {
                return;
            }
            oscar.getItemsRelativesIds(itemId, function (relativesIds) {
                oscar.getItems(relativesIds, function (relatives) {
                    map.clearListAndShapes("relatives");
                    for (var i in relatives) {
                        map.appendItemToListView(relatives[i], "relatives");
                    }
                    {
                        var tmp = $('#relativesList_itemId');
                        tmp.empty();
                        tmp.append("" + itemId);
                    }
                }, map.defErrorCB);
            }, map.defErrorCB);
        },

        closePopups: function () {
            if ($(".leaflet-popup-close-button")[0] !== undefined) {
                $(".leaflet-popup-close-button")[0].click();
            }
        },

        flatCqrTreeDataSource: function (cqr) {
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
                                    if (!state.DAG.count(itemId)) {
                                        node = parentNode.addChild(itemId);
                                        node.count = regionChildrenApxItemsMap[itemId];
                                        node.bbox = item.bbox();
                                        node.name = item.name();
                                        marker = L.marker(item.centerPoint());
                                        marker.count = regionChildrenApxItemsMap[item.id()];
                                        marker.rid = item.id();
                                        marker.name = item.name();
                                        marker.bbox = item.bbox();
                                        map.decorateMarker(marker);
                                        node.marker = marker;
                                        state.DAG.insert(itemId, node);
                                    } else {
                                        state.DAG.at(itemId).parents.push(parentNode);
                                        parentNode.children.push(state.DAG.at(itemId));
                                    }
                                }
                            }
                        } else if (parentCount > oscar.maxFetchItems) {
                            for (var i in items) {
                                item = items[i];
                                itemId = item.id();
                                itemMap[itemId] = item;
                                if (!state.DAG.count(itemId)) {
                                    node = parentNode.addChild(itemId);
                                    node.count = regionChildrenApxItemsMap[itemId];
                                    node.bbox = item.bbox();
                                    node.name = item.name();
                                    marker = L.marker(item.centerPoint());
                                    marker.count = regionChildrenApxItemsMap[item.id()];
                                    marker.rid = item.id();
                                    marker.name = item.name();
                                    marker.bbox = item.bbox();
                                    map.decorateMarker(marker);
                                    node.marker = marker;
                                    state.DAG.insert(itemId, node);
                                } else {
                                    state.DAG.at(itemId).parents.push(parentNode);
                                    parentNode.children.push(state.DAG.at(itemId));
                                }
                            }
                        }

                        // download locations, if end of hierarchy is reached or the region counts less than maxFetchItems
                        if (!items.length || (parentCount < oscar.maxFetchItems)) {
                            if (context.draw || !cqr.ohPath().length || cqr.ohPath()[cqr.ohPath().length - 1] == parentRid) {
                                $("#left_menu_parent").css("display", "block");

                                if (cqr.ohPath().length) {
                                    state.items.listview.selectedRegionId = parentRid;
                                    state.cqr.regionItemIds(state.items.listview.selectedRegionId,
                                        map.getItemIds,
                                        map.defErrorCB,
                                        0 // offset
                                    );
                                } else {
                                    if (state.DAG.at(parentRid).children.length == 0) {
                                        state.items.listview.selectedRegionId = parentRid;
                                        state.cqr.regionItemIds(state.items.listview.selectedRegionId,
                                            map.getItemIds,
                                            map.defErrorCB,
                                            0 // offset
                                        );
                                    } else {
                                        for (var child in state.DAG.at(parentRid).children) {
                                            state.items.listview.selectedRegionId = state.DAG.at(parentRid).children[child].id;
                                            state.cqr.regionItemIds(state.items.listview.selectedRegionId,
                                                map.getItemIds,
                                                map.defErrorCB,
                                                0 // offset
                                            );
                                        }
                                    }
                                }
                            }
                        } else if (context.draw) {
                            //cqr.getMaximumIndependetSet(parentRid, 100, function (regions) {
                            /*for (var i in items) {
                             if ($.inArray(items[i].id(), regions) == -1 && state.DAG.at(items[i].id()) !== undefined) {
                             state.DAG.at(items[i].id()).kill();
                             delete state.DAG.at(items[i]);
                             state.DAG.erase(items[i].id());
                             }
                             }*/
                            state.items.clusters.drawn.erase(parentRid);

                            var j;
                            var regions = [];
                            for (var i in items) {
                                j = itemMap[items[i].id()];

                                if (!state.items.clusters.drawn.count(j.id())) {
                                    regions.push(j.id());
                                    marker = state.DAG.at(j.id()).marker;
                                    state.items.clusters.drawn.insert(j.id(), marker);
                                    state.markers.addLayer(marker);
                                    state.DAG.at(marker.rid).marker = marker;
                                }
                            }

                            // just load regionShapes into the cache
                            oscar.getShapes(regions, function (res) {
                            }, map.defErrorCB);

                            if (state.visualizationActive) {
                                tree.refresh(parentRid);
                            }

                            //}, defErrorCB);
                        }

                        if (context.pathProcessor) {
                            context.pathProcessor.process();
                        }
                    },
                    map.defErrorCB
                );
            }

            return function (context) {
                map.startLoadingSpinner();
                cqr.regionChildrenInfo(context.rid, function (regionChildrenInfo) {
                        map.endLoadingSpinner()
                        getItems(regionChildrenInfo, context);
                    },
                    map.defErrorCB
                );
            };
        },

        getItemIds: function (regionId, itemIds) {

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
                    if (!$("a[href='#tab-" + regionId + "']").length) {
                        $('#tabs').append(tab);
                    }
                    if (isInitNecessary == 0) {
                        $('#items_parent').tabs();
                    } else {
                        $('#items_parent').tabs("refresh");
                    }

                    var regionDiv = "<div id='tab-" + regionId + "'></div>";
                    if (!$("#tab-" + regionId).length) {
                        $('#itemsList').append(regionDiv);
                    }
                    var parentElement = $('#tab-' + regionId);
                    for (var i in items) {
                        var item = items[i];
                        var itemId = item.id();
                        if (state.items.listview.promised.count(itemId)) {
                            if (!state.DAG.count(itemId)) {
                                state.DAG.insert(itemId, state.DAG.at(regionId).addChild(itemId));
                            } else {
                                state.DAG.at(regionId).children.push(state.DAG.at(itemId));
                                state.DAG.at(itemId).parents.push(state.DAG.at(regionId));
                            }
                            map.appendItemToListView(item, "items", parentElement);
                            state.items.listview.promised.erase(itemId);
                        }
                    }
                    $('#items_parent').tabs("refresh");

                    map.visualizeResultListItems();

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
                map.defErrorCB
            );
        },

        loadSub: function (rid) {
            state.cqr.regionChildrenInfo(rid, function (regionChildrenInfo) {
                var children = [];
                var regionChildrenApxItemsMap = {};

                for (var i in regionChildrenInfo) {
                    var ci = regionChildrenInfo[i];
                    regionChildrenApxItemsMap[ci['id']] = ci['apxitems'];
                    children.push(ci['id']);
                }

                oscar.getItems(children, function (items) {
                        var itemId, item, node, parentNode, marker;
                        parentNode = state.DAG.at(rid);

                        for (var i in items) {
                            item = items[i];
                            itemId = item.id();
                            if (!state.DAG.count(itemId)) {
                                node = parentNode.addChild(itemId);
                                node.count = regionChildrenApxItemsMap[itemId];
                                node.bbox = item.bbox();
                                node.name = item.name();
                                marker = L.marker(item.centerPoint());
                                marker.count = regionChildrenApxItemsMap[item.id()];
                                marker.rid = item.id();
                                marker.name = item.name();
                                marker.bbox = item.bbox();
                                map.decorateMarker(marker);
                                node.marker = marker;
                                state.DAG.insert(itemId, node);
                                map.addClusterMarker(state.DAG.at(itemId));
                            }
                        }
                        tree.refresh(rid);
                    }, function () {
                    }
                );
            }, function () {
            });
        },

        decorateMarker: function (marker) {
            marker.on("click", function (e) {
                map.closePopups();
                state.items.clusters.drawn.erase(e.target.rid);
                map.removeMarker(e.target);
                state.regionHandler({rid: e.target.rid, draw: true, dynamic: true});
            });

            marker.on("mouseover", function (e) {
                if (oscar.isShapeInCache(e.target.rid)) {
                    oscar.getShape(e.target.rid, function (shape) {
                        var leafletItem = oscar.leafletItemFromShape(shape);
                        leafletItem.setStyle(config.styles.shapes['regions']['normal']);
                        e.target.shape = leafletItem;
                        state.map.addLayer(leafletItem);
                    }, map.defErrorCB);
                }

                L.popup({offset: new L.Point(0, -10)})
                    .setLatLng(e.latlng)
                    .setContent(e.target.name).openOn(state.map);
            });

            marker.on("mouseout", function (e) {
                map.closePopups();
                if (e.target.shape) {
                    state.map.removeLayer(e.target.shape);
                }
            });
        },

        pathProcessor: function (cqr) {
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
                            var drawn = tools.SimpleHash();
                            var currentItemMarkers = state.items.shapes.drawn.values();
                            var currentClusterMarker = state.items.clusters.drawn.values();
                            var bulkMarkerBuffer = [];

                            for (var item in currentItemMarkers) {
                                drawn.insert(item, false);
                            }

                            for (var cluster in currentClusterMarker) {
                                map.removeClusterMarker(state.DAG.at(cluster));
                            }

                            if (this.path && this.path.length) {
                                // start at the target region (last element of ohPath)
                                map.drawClusters(state.DAG.at(this.path[this.path.length - 1]), drawn, bulkMarkerBuffer);
                            } else {
                                // start at Node "World"
                                map.drawClusters(state.DAG.at(0xFFFFFFFF), drawn, bulkMarkerBuffer);
                            }

                            state.markers.addLayers(bulkMarkerBuffer);

                            // remove all markers (and tabs) that are redundant
                            for (var item in drawn.values()) {
                                if (drawn.at(item) == false) {
                                    map.removeItemMarker(state.DAG.at(item));
                                    map.removeParentsTabs(state.DAG.at(item));
                                }
                            }
                        }.bind(this);
                        state.map.on("zoomend dragend", state.handler);
                    }
                }
            };
        },

        drawClusters: function (node, drawn, markerBuffer) {
            if (!node) {
                return;
            }

            var childNode;
            if (node.children.length) {
                for (var child in node.children) {
                    childNode = node.children[child];
                    if (tools.percentOfOverlap(state.map, childNode.bbox) >= config.overlap) {
                        map.drawClusters(childNode, drawn, markerBuffer);
                    } else {
                        if (childNode.count) {
                            map.addClusterMarker(childNode);
                        } else if (!childNode.count) {
                            if (!drawn.count(childNode.id)) {
                                if (childNode.marker !== undefined) { // TODO: WHY?
                                    map.addItemMarker(childNode);
                                }
                                map.setupTabsForItemAndAddToListView(childNode);
                            } else {
                                drawn.insert(childNode.id, true);
                            }
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
        },

        displayCqr: function (cqr) {
            state.clearViews();
            state.regionHandler = map.flatCqrTreeDataSource(cqr);
            var process = map.pathProcessor(cqr);
            var root = new tools.TreeNode(0xFFFFFFFF, undefined);
            root.count = cqr.rootRegionApxItemCount();
            root.name = "World";
            state.DAG.insert(0xFFFFFFFF, root);

            if (cqr.ohPath().length) {
                state.regionHandler({rid: 0xFFFFFFFF, draw: false, pathProcessor: process});
            } else {
                state.regionHandler({rid: 0xFFFFFFFF, draw: true, pathProcessor: process});
            }
        },

        removeMarker: function (marker) {
            if (marker.shape) {
                state.map.removeLayer(marker.shape);
            }
            state.markers.removeLayer(marker);
            map.closePopups();
        },

        removeClusterMarker: function (node) {
            map.removeMarker(node.marker);
            state.items.clusters.drawn.erase(node.id);
        },

        addClusterMarker: function (node, buffer) {
            state.items.clusters.drawn.insert(node.id, node.marker);
            if (buffer) {
                buffer.push(node.marker)
            } else {
                state.markers.addLayer(node.marker);
            }
        },

        removeItemMarker: function (node) {
            state.markers.removeLayer(node.marker);
            state.items.shapes.drawn.erase(node.id);
        },

        addItemMarker: function (node, buffer) {
            state.items.shapes.drawn.insert(node.id, node.marker);
            if (buffer) {
                buffer.push(node.marker);
            } else {
                state.markers.addLayer(node.marker);
            }
        },

        setupTabsForItemAndAddToListView: function (node) {
            for (var parent in node.parents) {
                if (!$("a[href='#tab-" + node.parents[parent].id + "']").length) {
                    var tab = "<li><a href='#tab-" + node.parents[parent].id + "'>" + state.DAG.at(node.parents[parent].id).name + " [" + node.parents[parent].count + "]" + "</a></li>";
                    $('#tabs').append(tab);
                }
                if (!$("#tab-" + node.parents[parent].id).length) {
                    var regionDiv = "<div id='tab-" + node.parents[parent].id + "'></div>";
                    $('#itemsList').append(regionDiv);
                }
                oscar.getItem(node.id, function (item) {
                    map.appendItemToListView(item, "items", $("#tab-" + node.parents[parent].id));
                }, map.defErrorCB);
            }
        },

        removeParentTab: function (childNode, parentNode) {
            for (var parent in childNode.parents) {
                if (childNode.parents[parent].id == parentNode.id && $("a[href='#tab-" + childNode.parents[parent].id + "']").length) {
                    $("a[href='#tab-" + childNode.parents[parent].id + "']").parent().remove();
                    $("#tab-" + childNode.parents[parent].id).remove();
                }
            }
            $('#items_parent').tabs("refresh");
        },

        removeParentsTabs: function (childNode) {
            for (var parent in childNode.parents) {
                map.removeParentTab(childNode, childNode.parents[parent]);
            }
        },

        doCompletion: function () {
            if ($("#search_text").val() === state.queries.lastQuery) {
                return;
            }

            if ($('#searchModi input').is(":checked")) {
                //TODO: wrong placement of markers if clsutering is aktive. Cause: region midpoint is outside of search rectangle
                tools.addSingleQueryStatementToQuery("$geo:" + state.map.getBounds().getSouthWest().lng
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
                        map.displayCqr(cqr);
                    }
                },
                function (jqXHR, textStatus, errorThrown) {
                    //BOOM!
                    alert("Failed to retrieve completion results. textstatus=" + textStatus + "; errorThrown=" + errorThrown);
                });
        },

        instantCompletion: function () {
            if (state.timers.query !== undefined) {
                clearTimeout(state.timers.query);
            }
            map.doCompletion();
        },

        delayedCompletion: function () {
            if (state.timers.query !== undefined) {
                clearTimeout(state.timers.query);
            }
            state.timers.query = setTimeout(map.instantCompletion, config.timeouts.query);
        },

        queryFromSearchLocation: function () {
            var myQ = tools.getParameterByName("q");
            if (myQ.length) {
                $('#search_text').val(myQ);
                map.instantCompletion();
            }
        }

    };
})
;