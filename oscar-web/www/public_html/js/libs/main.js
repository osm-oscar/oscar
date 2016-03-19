requirejs.config({
    baseUrl: "js/libs",
    config: {
        'oscar': {url: "http://oscar.fmi.uni-stuttgart.de/oscar"}
    },
    paths: {
        "jquery": "jquery/jquery.min",
        "spin": "spin/spin.min",
        "spinner": "spin/spinner",
        "leaflet": "leaflet/leaflet.0.7.2",
        "leafletCluster": "leaflet/leaflet.markercluster-src",
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

requirejs(["leaflet", "jquery", "mustache", "jqueryui", "sidebar", "mustacheLoader", "conf", "menu", "tokenfield", "switch", "state", "map", "tree", "prototype", "query", "search"],
    function (L, jQuery, mustache, jqueryui, sidebar, mustacheLoader, config, menu, tokenfield, switchButton, state, map, tree) {
        var query = require("query");
        var search = require("search");

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

            $('#geoquery_selectbutton').click(function () {
                if (state.geoquery.active) {
                    query.clearGeoQuerySelect();
                }
                else {
                    query.startGeoQuerySelect();
                }
            });

            $('#geoquery_acceptbutton').click(function () {
                //first fetch the coordinates
                var minlat = parseFloat($('#geoquery_minlat').val());
                var minlon = parseFloat($('#geoquery_minlon').val());
                var maxlat = parseFloat($('#geoquery_maxlat').val());
                var maxlon = parseFloat($('#geoquery_maxlon').val());

                //end the geoquery
                query.clearGeoQuerySelect();

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

                query.clearGeoQueryMapShape();
            });

            $('#geoquery_minlat, #geoquery_maxlat, #geoquery_minlon, #geoquery_maxlon').change(function (e) {
                query.updateGeoQueryMapShape();
            });

            $('#pathquery_selectbutton').click(function () {
                if (state.pathquery.selectButtonState === 'select') {
                    query.startPathQuery();
                }
                else if (state.pathquery.selectButtonState === 'finish') {
                    query.endPathQuery();
                }
                else {
                    query.resetPathQuery();
                }
            });

            $('#pathquery_acceptbutton').click(function () {
                query.endPathQuery();
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
                query.resetPathQuery();
            });

            $(window).bind('popstate', function (e) {
                search.queryFromSearchLocation();
            });

            //check if there's a query in our location string
            search.queryFromSearchLocation();
        });
    });