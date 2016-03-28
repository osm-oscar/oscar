requirejs.config({
    baseUrl: "js/libs",
    config: {
        'oscar': {url: "http://oscardev.fmi.uni-stuttgart.de/oscar"}
    },
    paths: {
        "jquery": "jquery/jquery.min",
        "spin": "spin/spin.min",
        "spinner": "spin/spinner",
        "leaflet": "leaflet/leaflet.0.7.2",
        "leafletCluster": "leaflet/leaflet.markercluster-src",
        "sidebar": "leaflet/leaflet-sidebar",
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
        "switch": "switch-button/jquery.switchButton",
        "d3": "dagre-d3/d3.min",
        "dagre-d3": "dagre-d3/dagre-d3.min",
        "tree": "tree/tree",
        "tokenfield": "tokenfield/bootstrap-tokenfield",
        "map": "map/map",
        "prototype": "prototype/prototype",
        "state": "state/manager",
        "query": "query/query",
        "search": "search/search"
    },
    shim: {
        'bootstrap': {deps: ['jquery']},
        'leafletCluster': {deps: ['leaflet', 'jquery']},
        'sidebar': {deps: ['leaflet', 'jquery']},
        'mustacheLoader': {deps: ['jquery']},
        'slimbox': {deps: ['jquery']},
    },
    waitSeconds: 20
});

requirejs(["leaflet", "jquery", "mustache", "jqueryui", "sidebar", "mustacheLoader", "conf", "menu", "tokenfield", "switch", "state", "map", "tree", "prototype", "query", "tools", "search"],
    function (L, jQuery, mustache, jqueryui, sidebar, mustacheLoader, config, menu, tokenfield, switchButton, state, map, tree) {
        var query = require("query");
		var tools = require("tools");
        var search = require("search");

        // mustache-template-loader needs this
        window.Mustache = mustache;

        // load template files
        $.Mustache.load('template/itemListEntryTemplate.mst');
        $.Mustache.load('template/spatialQueryTableRowTemplate.mst');
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
        var osmAttr = '&copy; <a target="_blank" href="http://www.openstreetmap.org">OpenStreetMap</a>';
        L.tileLayer('http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {attribution: osmAttr}).addTo(state.map);

        $(document).ready(function () {
            $("#tree").resizable();

            $('[data-toggle="tooltip"]').tooltip();

            var search_form = $("#search_form");
            var search_text = $('#search_text');
            search_form.click(function () {
                if (!$('#categories').is(":visible")) {
                    $("#showCategories a").click();
                }
            });
            search_text.tokenfield({minWidth: 250, delimiter: "|"});
            $(search_form[0].children).css("width", "100%");
            search_text.bind('change', search.delayedCompletion).bind('keyup', search.delayedCompletion);
            search_form.bind('submit', function (e) {
                e.preventDefault();
                search.instantCompletion();
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
                $("#onePath").button();
                $("#wholeTree").button().click(function () {
                    map.loadWholeTree();
                    tree.visualizeDAG(state.DAG.at(0xFFFFFFFF));
                });
                tree.visualizeDAG(state.DAG.at(0xFFFFFFFF));
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
			
			state.sidebar.on('tab-closed', function(e) {
				if (e.id !== "item_relatives") {
					return;
				}
				map.clearHighlightedShapes("relatives");
				map.clearHighlightedShapes("activeItems");
			});
			
			state.sidebar.on('tab-opened', function(e) {
				if (e.id !== "item_relatives") {
					return;
				}
				//check if the active item is opened, if so add its shape to the map
				var activeItemsList = $('#activeItemsList');
				var activeItems = activeItemsList.find("[class~='collapse'][class~='in']");
				if (activeItems.length) {
					var activeItem = activeItems.first();
					var itemId = parseInt(activeItem.attr("data-item-id"));
					map.highlightShape(itemId, "activeItems");
				}
				//do the same for the relatives
				var relativesList = $('#relativesList');
				var activeRelatives = relativesList.find("[class~='collapse'][class~='in']");
				activeRelatives.each(function() {
					var itemId = parseInt($(this).attr("data-item-id"));
					map.highlightShape(itemId, "relatives");
				});
			});

            $('#spatialquery_selectbutton').click(function() {
                if (state.spatialquery.selectButtonState === 'select') {
                    query.startSpatialQuery();
                }
                else if (state.spatialquery.selectButtonState === 'finish') {
                    query.endSpatialQuery();
                }
                else if (state.spatialquery.selectButtonState === 'clear') {
                    query.clearSpatialQuery();
                }
            });
			
            $('#spatialquery_acceptbutton').click(function() {
                if (state.spatialquery.type === undefined) {
                    return;
                }
                query.endSpatialQuery();
                var qStr = ""
                if (state.spatialquery.type === "rect") {
                    var minLat = Math.min(state.spatialquery.coords[0].lat, state.spatialquery.coords[1].lat);
                    var maxLat = Math.max(state.spatialquery.coords[0].lat, state.spatialquery.coords[1].lat);
                    var minLng = Math.min(state.spatialquery.coords[0].lng, state.spatialquery.coords[1].lng);
                    var maxLng = Math.max(state.spatialquery.coords[0].lng, state.spatialquery.coords[1].lng);
                    qStr = "$geo:" + minLng + "," + minLat + "," + maxLng + "," + maxLat;
                }
                else if (state.spatialquery.type === "path") {
                    if (state.spatialquery.coords.length > 0) {
                        qStr = "$path:" + jQuery('#spatialquery_radius').val();
                        for(i in state.spatialquery.coords) {
                            qStr += "," + state.spatialquery.coords[i].lat + "," + state.spatialquery.coords[i].lng;
                        }
                    }
                }
                else if (state.spatialquery.type === "poly") {
                    if (state.spatialquery.coords.length > 3) {
                        qStr = "$poly";
                        var delim = ":"
                        for(i in state.spatialquery.coords) {
                            qStr += delim + state.spatialquery.coords[i].lat + "," + state.spatialquery.coords[i].lng;
                            delim = ",";
                        }
                    }
                }
                if (qStr.length) {
					var id;
					for(i=0; true; ++i) {
						if (!state.spatialObjects.store.count(i)) {
							id = i;
							break;
						}
					}
					var data = {
						name : id,
						type : state.spatialquery.type,
						mapshape : state.spatialquery.mapshape,
						query : qStr
					};
					state.spatialObjects.store.insert(id, data);
					state.spatialObjects.names.insert(id, id);
					map.appendSpatialObjectToTable(id);
                }
                query.clearSpatialQuery();
            });
			
            $('#spatialquery_type').change(function(e) {
                if (e.target.value !== state.spatialquery.type) {
                    query.clearSpatialQuery();
                }
                if (e.target.value === 'path') {
                    $('#spatialquery_radius_group').removeClass('hidden');
                }
                else {
                    $('#spatialquery_radius_group').addClass('hidden');
                }
            });

            $(window).bind('popstate', function (e) {
                search.queryFromSearchLocation();
            });

            //check if there's a query in our location string
            search.queryFromSearchLocation();
        });
    });