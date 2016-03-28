define([], function() {
    var protoRegExp = /^.*:\/\//;
    var uriParsers = {
            "id" : function(x) {
                    return x;
            },
            "http" : function(x) {
                    return (protoRegExp.test(x) ? x : "http://" + x);
            },
            "wikipedia" : function(x) {
                    if (protoRegExp.test(x)) {
                            return x;
                    }
                    else {
                            return "http://en.wikipedia.org/wiki/" + x;
                    }
            },
            "wikicommons" : function(x) {
                    if (protoRegExp.test(x)) {
                            return x;
                    }
                    else {
                            return "http://en.wikipedia.org/wiki/commons:" + x;
                    }
            }
    };
    var cfg = {
        styles: {
            shapes: {
                regions: {
                    normal: {color: 'yellow', stroke: true, fill: false, opacity: 0.8},
                    highlight: {color: 'yellow', stroke: true, fill: false, opacity: 1.0}
                },
                items: {
                    normal: {color: 'blue', stroke: true, fill: false, opacity: 0.8},
                    highlight: {color: 'red', stroke: true, fill: false, opacity: 1.0}
                },
                relatives: {
                    normal: {color: 'green', stroke: true, fill: false, opacity: 0.7},
                    highlight: {color: 'green', stroke: true, fill: false, opacity: 1.0}
                },
                activeItems: {
                    normal: {color: 'blue', stroke: true, fill: false, opacity: 0.8},
                    highlight: {color: 'red', stroke: true, fill: false, opacity: 1.0}
                },
                geoquery: {
                    normal: {color: '#00BFFF', stroke: true, fill: true, opacity: 0.5},
                    highlight: {color: '#00BFFF', stroke: true, fill: true, opacity: 1.0}
                },
                pathquery: {
                    normal: {color: '#00BFFF', stroke: true, fill: false, opacity: 0.5},
                    highlight: {color: '#00BFFF', stroke: true, fill: false, opacity: 1.0}
                },
                polyquery: {
                    normal: {color: '#00BFFF', stroke: true, fill: false, opacity: 0.5},
                    highlight: {color: '#00BFFF', stroke: true, fill: false, opacity: 1.0}
                }
            },
            menu: {
                fadeopacity: 0.9
            },
            slide: {
                speed: 2000
            }
        },
        resultsview : {
            urltags : {
                "url" : uriParsers["http"],
                "website" : uriParsers["http"],
                "link" : uriParsers["http"],
                "contact:website" : uriParsers["http"],
                "wikipedia" : uriParsers["wikipedia"],
                "wikimedia_commons" : uriParsers["wikicommons"],
                "image" : uriParsers["wikicommons"]
            }
        },
        geoquery: {
            samplecount: 10
        },
        functionality: {
            shapes: {
                "highlightListItemOnClick": {
                    "items": true,
                    "relatives": false
                }
            }
        },
        timeouts: {
            query: 10000,
            loadingSpinner: 1000,
            spatialquery : {
                select : 10000
            }
        },
		overlap: 0.40, // threshold for overlap of a bbox with viewport
		maxBufferedItems: 350, // buffered locations
		maxNumSubClusters: 10,
		spinnerOpts: {
			lines: 13 // The number of lines to draw
			, length: 5 // The length of each line
			, width: 10 // The line thickness
			, radius: 10 // The radius of the inner circle
			, scale: 1 // Scales overall size of the spinner
			, corners: 1 // Corner roundness (0..1)
			, color: '#000' // #rgb or #rrggbb or array of colors
			, opacity: 0.25 // Opacity of the lines
			, rotate: 0 // The rotation offset
			, direction: 1 // 1: clockwise, -1: counterclockwise
			, speed: 1 // Rounds per second
			, trail: 60 // Afterglow percentage
			, fps: 20 // Frames per second when using setTimeout() as a fallback for CSS
			, zIndex: 2e9 // The z-index (defaults to 2000000000)
			, className: 'spinner' // The CSS class to assign to the spinner
			, top: '50%' // Top position relative to parent
			, left: '50%' // Left position relative to parent
			, shadow: false // Whether to render a shadow
			, hwaccel: false // Whether to use hardware acceleration
			, position: 'absolute' // Element positioning
		}
	}
    return cfg;
});