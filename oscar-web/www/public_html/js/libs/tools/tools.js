define(function () {
    SimpleHash = function () {
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
    };

    TreeNode = function (id, parent) {
        var parents = [];
        parents.push(parent);
        return {
            id: id,
            parents: parents,
            children: [],
            count: undefined,
            marker: undefined,
            bbox: undefined,
            addChild: function (id) {
                var node = new TreeNode(id, this);
                this.children.push(node);
                return node;
            },
            kill: function(){
                // kill all references to this node
                for(var parent in parents){
                    for(var child in parents[parent].children){
                        if(parents[parent].children[child] == this.id){
                            parents[parent].children.splice(child, child);
                        }
                    }
                }
                // kill the actual node
                delete this;
            }
        };
    };

    percentOfOverlap = function (map, viewport, bbox) {
        // basic version: http://math.stackexchange.com/questions/99565/simplest-way-to-calculate-the-intersect-area-of-two-rectangles
        var d0 = map.project(viewport.getSouthWest()),
            w0 = Math.abs(map.project(viewport.getNorthEast()).x - d0.x), // width
            h0 = Math.abs(map.project(viewport.getNorthEast()).y - d0.y), // height

            d1 = map.project(bbox[0]),
            w1 = Math.abs(map.project(bbox[1]).x - d1.x), // width
            h1 = Math.abs(map.project(bbox[1]).y - d1.y), // height

            x11 = d0.x,
            y11 = d0.y,
            x12 = d0.x + w0,
            y12 = d0.y + h0,
            x21 = d1.x,
            y21 = d1.y,
            x22 = d1.x + w1,
            y22 = d1.y + h1,

            xOverlap = Math.max(0, Math.min(x12, x22) - Math.max(x11, x21)),
            yOverlap = Math.max(0, Math.min(y12, y22) - Math.max(y11, y21)),
            totalOverlap = xOverlap * yOverlap;

        return totalOverlap / (w0 * h0); // compare the overlap to the size of the viewport
    };

});

