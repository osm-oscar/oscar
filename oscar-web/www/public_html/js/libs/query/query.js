define(["jquery", "state", "map"], function($, state, map){
    return query = {
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
            $('#geoquery_selectbutton').addClass("btn-info").html('Clear rectangle');
            $('[data-class="geoquery_min_coord"]').addClass('bg-info');
            state.map.on('click', map.geoQuerySelect);
            state.timers.geoquery = setTimeout(map.endGeoQuerySelect, config.timeouts.geoquery_select);
        },

        resetPathQuery: function () {
            if (state.timers.pathquery !== undefined) {
                clearTimeout(state.timers.pathquery);
            }
            state.map.removeEventListener('click', map.pathQuerySelect);
            state.map.removeLayer(state.pathquery.mapshape);
            state.pathquery.mapshape = undefined;
            state.pathquery.selectButtonState = 'select';
            $('#pathquery_acceptbutton').removeClass("btn-info");
            $('#pathquery_selectbutton').removeClass("btn-info").html('Select path');
        },

        pathQuerySelect: function (e) {
            if (state.timers.pathquery !== undefined) {
                clearTimeout(state.timers.pathquery);
                state.timers.pathquery = setTimeout(map.endPathQuery, config.timeouts.pathquery.select);
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
            $('#pathquery_selectbutton').removeClass("btn-info").html('Clear path');
            state.pathquery.selectButtonState = 'clear';
        },

        startPathQuery: function () {
            if (state.timers.pathquery !== undefined) {
                clearTimeout(state.timers.pathquery);
            }
            state.pathquery.points = []
            $('#pathquery_acceptbutton').removeClass("btn-info");
            $('#pathquery_selectbutton').addClass("btn-info").html("Finish path");
            state.pathquery.selectButtonState = 'finish';
            state.map.on('click', map.pathQuerySelect);
            state.timers.pathquery = setTimeout(map.endPathQuery, config.timeouts.pathquery.select);
        }
    };
});