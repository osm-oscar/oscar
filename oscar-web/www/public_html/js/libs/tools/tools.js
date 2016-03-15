define(["jquery"], function ($) {
    var tools = {
        /**
         * Represents a small API for a hashmap
         *
         * @returns {{}} hashmap-object
         */
        SimpleHash: function () {
            return {
                m_size: 0,
                m_values: {},
                values: function () {
                    return this.m_values;
                },
                size: function () {
                    return this.m_size;
                },
                insert: function (key, value) {
                    if (this.m_values[key] === undefined) {
                        this.m_size += 1;
                    }
                    this.m_values[key] = value;
                },
				set: function (key, value) {
					this.insert(key, value);
				},
                count: function (key) {
                    return this.m_values[key] !== undefined;
                },
                at: function (key) {
                    return this.m_values[key];
                },
                erase: function (key) {
                    if (this.m_values[key] !== undefined) {
                        this.m_size -= 1;
                        delete this.m_values[key];
                    }
                },
                clear: function () {
                    this.m_size = 0;
                    this.m_values = {};
                }
            };
        },

        /**
         * Represents a Treenode, which can be used to model directed-acylic graphs.
         *
         * @param id of the node
         * @param parent one parent of the node
         * @returns
         */
        TreeNode: function (id, parent) {
            var parents = [];
            parents.push(parent);
            return {
                id: id,
                name: undefined,
                parents: parents,
                children: [],
                count: undefined,
                marker: undefined,
                bbox: undefined,
                addChild: function (id) {
                    var node = tools.TreeNode(id, this);
                    this.children.push(node);
                    return node;
                },
                kill: function () {
                    // kill all references to this node
                    for (var parent in parents) {
                        for (var child in parents[parent].children) {
                            if (parents[parent].children[child].id == this.id) {
                                parents[parent].children.splice(child, 1);
                            }
                        }
                    }
                }
            };
        },

        /**
         * Calculates the overlap of the viewport and a bbox. Returns the percentage of overlap.
         *
         * @param map contains the viewport coordinates
         * @param bbox
         * @returns {number} defines the overlap (0 <= overlap <= 1)
         */
        percentOfOverlap: function (map, bbox) {
            if (bbox) {
                // basic version: http://math.stackexchange.com/questions/99565/simplest-way-to-calculate-the-intersect-area-of-two-rectangles
                var viewport = map.getBounds();
                var d0 = map.project(viewport.getSouthWest()),
                    w0 = Math.abs(map.project(viewport.getNorthEast()).x - d0.x), // width
                    h0 = Math.abs(map.project(viewport.getNorthEast()).y - d0.y), // height

                    d1 = map.project(bbox[0]),
                    w1 = Math.abs(map.project(bbox[1]).x - d1.x), // width
                    h1 = Math.abs(map.project(bbox[1]).y - d1.y), // height

                    x11 = d0.x,
                    y11 = d0.y,
                    x12 = d0.x + w0,
                    y12 = d0.y - h0,
                    x21 = d1.x,
                    y21 = d1.y,
                    x22 = d1.x + w1,
                    y22 = d1.y - h1,

                    xOverlap = Math.max(0, Math.min(x12, x22) - Math.max(x11, x21)),
                    yOverlap = Math.max(0, Math.min(y11, y21) - Math.max(y12, y22)),
                    totalOverlap = xOverlap * yOverlap;

                return totalOverlap / (w0 * h0); // compare the overlap to the size of the viewport
            } else {
                return 0;
            }
        },

        /**
         * Timer-utility for benchmarking, logs on the console.
         *
         * @param name of the timer
         * @returns {{stop: Function} stops the timer}
         */
        timer: function (name) {
            var start = new Date();
            return {
                stop: function () {
                    var end = new Date();
                    var time = end.getTime() - start.getTime();
                    console.log('Timer:', name, 'finished in', time, 'ms');
                }
            }
        },

        /**
         * adds a string to the search input
         *
         * @param qstr the string that should be added
         */
        addSingleQueryStatementToQuery: function (qstr) {
            var search_text = $('#search_text');
			search_text.tokenfield('createToken', qstr);
//             search_text.change();
        },
	   
		setQuery: function(qstr) {
            var search_text = $('#search_text');
			search_text.tokenfield('setTokens', [{value : qstr, label : qstr}]);
		},

        //https://stackoverflow.com/questions/901115/how-can-i-get-query-string-values-in-javascript
        getParameterByName: function (name) {
            name = name.replace(/[\[]/, "\\[").replace(/[\]]/, "\\]");
            var regex = new RegExp("[\\?&]" + name + "=([^&#]*)"),
                results = regex.exec(location.search);
            return results === null ? "" : decodeURIComponent(results[1].replace(/\+/g, " "));
        }

    };

    return tools;
});

