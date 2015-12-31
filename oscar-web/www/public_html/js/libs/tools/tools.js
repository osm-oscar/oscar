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
            }
        };
    };
});

