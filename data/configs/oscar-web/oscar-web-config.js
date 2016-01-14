{
	"service" : {
		"api" : "http",
		"port" : 8080
	},
	"http" : {
		"script" : ""
	},
	"dbfile" :
		{
			"name" : "Baden Württemberg",
			"path" : "bw",
			"logfile": "oscarweb.log",
			"limit" : 128, //limit the result size of old style queries
			"chunklimit" : 8, //limit the chunk size of old style queries
			"fullsubsetlimit" : 150, //maximum number of cells in a result for which a full subset will be created
			"maxindexdbreq": 100, //maximum number of indexes returned per request
			"maxitemdbreq": 100, //maximum number of returned items per request
			"geocellcompleter" : 0, //select the geocell completer
			"itemscompleter" : 0, //select items completer
			"geohcompleter" : 0, //select the geo hierarchy completer
			"geocompleter" : 0, //select geocompleter
			"treedCQR" : true //Use treed cqr calculation instead of normal one
		},
	"ghfilters" :
		[
			{"name" : "admin_level", "k" : ["admin_level"], "kv" : {"key", "value"}}
			{"name" : "all" "__COMMENT" : "implicit, dont't define this"}
			{"name" : "natural_landuse", "k" : ["natural", "landuse"]}
			{"name" : "named", "k" : ["name"]}
		]
}
