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
            count : undefined,
            marker : undefined,
            bbox: undefined,
            addChild: function (id) {
                var node = new TreeNode(id, this);
                this.children.push(node);
                return node;
            },
            hasParentWithId: function (id) {
                // use BFS
                var queue = [];
                var node;
                queue.push(this);

                while (queue.length > 0) {
                    node = queue.shift();
                    if (node.id == id) {
                        return true;
                    } else {
                        for (var i in node.parents) {
                            if (node.parents[i] != undefined) {
                                queue.push(node.parents[i]);
                            }
                        }
                    }
                }
                return false;
            }
        };
    };

    /*fitsBoxIntoViewPort = function (viewport, bbox) {
        var southWestViewPort = viewport[0];
        var northEastViewPort = viewport[1];
        var southWestBbox = bbox[0];
        var northEastBbox = bbox[1];

        // check lat
        if (southWestBbox[0] >= southWestViewPort[0] && northEastBbox[0] <= northEastViewPort[0]) {
            // check lng
            if (southWestBbox[1] >= southWestViewPort[1] && northEastBbox[1] <= northEastViewPort[1]) {
                return true;
            }
        }

        return false;
    };*/

});

