define(["jquery", "state", "map", "conf"], function($, state, map, config){
    return query = {
        clearSpatialQueryMapShape: function() {
            if (state.spatialquery.mapshape !== undefined) {
                state.map.removeLayer(state.spatialquery.mapshape);
                state.spatialquery.mapshape = undefined;
            }
        },
        //begin spatial query functions
        spatialQueryOnClick: function(e) {
            //reset timers
            if (state.timers.spatialquery !== undefined) {
                clearTimeout(state.timers.spatialquery);
                state.timers.spatialquery = setTimeout(endSpatialQuery, myConfig.timeouts.spatialquery.select);
            }
            state.spatialquery.coords.push(e.latlng);
            if (state.spatialquery.type === "rect") {
                if (state.spatialquery.coords.length === 2) {
                    endSpatialQuery();
                }
            }
            else {
                this.updateSpatialQueryMapShape();
            }
        },
        startSpatialQuery: function() {
            if (state.timers.spatialquery !== undefined) {
                clearTimeout(state.timers.spatialquery);
                state.timers.spatialquery = undefined;
            }
            $('#spatialquery_acceptbutton').removeClass("btn-info");
            $('#spatialquery_selectbutton').addClass("btn-info");
            $('#spatialquery_selectbutton').html("Finish");
            state.spatialquery.selectButtonState = 'finish';
            state.spatialquery.type = $('#spatialquery_type').val();
            state.map.on('click', spatialQueryOnClick);
            state.timers.spatialquery = setTimeout(endSpatialQuery, myConfig.timeouts.spatialquery.select);
        },
        endSpatialQuery: function() {
            this.updateSpatialQueryMapShape();
            if (state.timers.spatialquery !== undefined) {
                clearTimeout(state.timers.spatialquery);
                state.timers.spatialquery = undefined;
            }
            state.map.removeEventListener('click', spatialQueryOnClick);
            
            state.spatialquery.selectButtonState = 'clear';
            $('#spatialquery_selectbutton').html('Clear');
            $('#spatialquery_acceptbutton').addClass('btn-info');
        },
        clearSpatialQuery: function() {
            if (state.timers.spatialquery !== undefined) {
                clearTimeout(state.timers.spatialquery);
                state.timers.spatialquery = undefined;
            }
            
            this.clearSpatialQueryMapShape();
            
            state.map.removeEventListener('click', spatialQueryOnClick);
            
            state.spatialquery.coords = [];
            state.spatialquery.selectButtonState = 'select';
            state.spatialquery.type = undefined;
            
            $('#spatialquery_acceptbutton').removeClass("btn-info");
            $('#spatialquery_selectbutton').removeClass("btn-info");
            $('#spatialquery_selectbutton').html('Create');
        },
        updateSpatialQueryMapShape: function() {
            //clear old mapshape
            this.clearSpatialQueryMapShape();
            if (state.spatialquery.type === undefined) {
                console.log("updateSpatialQueryMapShape called with undefined query type");
                return;
            }
            else if (state.spatialquery.type === "rect") {
                if (state.spatialquery.coords.length < 2) {
                    return;
                }
                var sampleStep = 1.0 / config.geoquery.samplecount;
                var sampleCount = config.geoquery.samplecount;
                var pts = [];
                function fillPts(sLat, sLng, tLat, tLng) {
                    var latDiff = (tLat-sLat)*sampleStep;
                    var lngDiff = (tLng-sLng)*sampleStep;
                    for(i=0; i < sampleCount; ++i) {
                        pts.push(L.latLng(sLat, sLng));
                        sLat += latDiff;
                        sLng += lngDiff;
                    }
                }
                var minLat = Math.min(state.spatialquery.coords[0].lat, state.spatialquery.coords[1].lat);
                var maxLat = Math.max(state.spatialquery.coords[0].lat, state.spatialquery.coords[1].lat);
                var minLng = Math.min(state.spatialquery.coords[0].lng, state.spatialquery.coords[1].lng);
                var maxLng = Math.max(state.spatialquery.coords[0].lng, state.spatialquery.coords[1].lng);

                fillPts(minLat, minLng, minLat, maxLng);
                fillPts(minLat, maxLng, maxLat, maxLng);
                fillPts(maxLat, maxLng, maxLat, minLng);
                fillPts(maxLat, minLng, minLat, minLng);

                state.spatialquery.mapshape = L.polygon(pts, config.styles.shapes.geoquery.normal);
            }
            else if (state.spatialquery.type === "poly") {
                state.spatialquery.mapshape = L.polygon(state.spatialquery.coords, config.styles.shapes.polyquery.highlight);
            }
            else if (state.spatialquery.type === "path") {
                state.spatialquery.mapshape = L.polyline(state.spatialquery.coords, config.styles.shapes.pathquery.highlight);
            }
            state.map.addLayer(state.spatialquery.mapshape);
        }
    };
});