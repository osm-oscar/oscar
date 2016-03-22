define(["state", "tools", "conf", "oscar", "map"], function(state, tools, config, oscar, map){
    return search = {
       //replace spatial objects with the real deal
	   replaceSpatialObjects: function(qstr) {
			var res = "";
			var withInExact = false;
			for(i=0; i < qstr.length;) {
				if (qstr[i] === '"') {
					withInExact = !withInExact;
					res += '"';
					++i;
				}
				else if (qstr[i] === '\\') {
					++i;
					if (i < qstr.length) {
						res += '\\' + qstr[i];
						++i;
					}
				}
				else if (qstr[i] === '&' && !withInExact) {
					var name = "";
					for(++i; i < qstr.length && qstr[i] !== ' '; ++i) {
						name += qstr[i];
					}
					if (state.spatialObjects.names.count(name)) {
						res += state.spatialObjects.store.at(state.spatialObjects.names.at(name)).query;
					}
				}
				else {
					res += qstr[i];
					++i;
				}
			}
			return res;
	   },
        doCompletion: function () {
            if ($("#search_text").val() === state.queries.lastQuery) {
                return;
            }
            state.clearViews();

            $("#showCategories a").click();
            state.sidebar.open("search");
            $("#flickr").hide("slide", {direction: "right"}, config.styles.slide.speed);

            //query has changed, ddos the server!
            var myQuery = $("#search_text").val();
            
            state.queries.lastQuery = myQuery + "";//make sure state hold a copy

            var ohf = (parseInt($('#ohf_spinner').val()) / 100.0);
            var globalOht = $('#oht_checkbox').is(':checked');
			var regionFilter = jQuery('#region_filter').val();
            //ready for lift-off
            var myQueryCounter = state.queries.lastSubmited + 0;
            state.queries.lastSubmited += 1;

            var callFunc;
			//call sequence starts
			if ($("#full_cqr_checkbox").is(':checked')) {
				callFunc = function(q, scb, ecb) {
					oscar.completeFull(q, scb, ecb);
				};
			}
			else {
				callFunc = function(q, scb, ecb) {
					oscar.completeSimple(q, scb, ecb, ohf, globalOht, regionFilter);  
				};
			}

			var myRealQuery =  map.replaceSpatialObjects(myQuery);
			
            if ($('#searchModi input').is(":checked")) {
                //TODO: wrong placement of markers if clustering is active. Cause: region midpoint is outside of search rectangle
                myRealQuery =  "(" + myRealQuery + ") $geo:" + state.map.getBounds().getSouthWest().lng
                    + "," + state.map.getBounds().getSouthWest().lat + ","
                    + state.map.getBounds().getNorthEast().lng + "," + state.map.getBounds().getNorthEast().lat;
            }

            //push our query as history state
            window.history.pushState({"q": myRealQuery}, undefined, location.pathname + "?q=" + encodeURIComponent(myRealQuery));

            //lift-off
            callFunc(myRealQuery,
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
            search.doCompletion();
        },

        delayedCompletion: function () {
            if (state.timers.query !== undefined) {
                clearTimeout(state.timers.query);
            }
            state.timers.query = setTimeout(search.instantCompletion, config.timeouts.query);
        },
		
        queryFromSearchLocation: function () {
            var myQ = tools.getParameterByName("q");
            if (myQ.length) {
				tools.setQuery(myQ);
                map.instantCompletion();
			}
		}
    };
});
