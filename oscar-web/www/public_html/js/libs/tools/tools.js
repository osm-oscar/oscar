function SimpleHash() {
    var m_size = 0;
    var m_values = {};
    return {
        values: function () {
            return m_values;
        },
        size: function () {
            return m_size;
        },
        insert: function (key, value) {
            if (m_values[key] === undefined) {
                m_size += 1;
            }
            m_values[key] = value;
        },
        count: function (key) {
            return m_values[key] !== undefined;
        },
        at: function (key) {
            return m_values[key];
        },
        erase: function (key) {
            if (m_values[key] !== undefined) {
                m_size -= 1;
                delete m_values[key];
            }
        },
        clear: function () {
            m_size = 0;
            m_values = {};
        }
    };
};

TreeNode = function (id, parent) {
    this.id = id;
    this.parent = parent;
    this.children = [];
    return this;
};

TreeNode.prototype.addChild = function (id) {
    var node = new TreeNode(id, this);
    this.children.push(node);
    return node;
};

TreeNode.prototype.isDagPathInOhPath = function (ohPath) {
    var tmp = this;
    while (tmp !== undefined) {
        if ($.inArray(tmp.id, ohPath) != -1) {
            return true;
        } else {
            tmp = tmp.parent;
        }
    }
    return false;
};
