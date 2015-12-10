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
}

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

TreeNode.prototype.hasParentWithId = function (id) {
    var tmp = this.parent;
    while (tmp !== undefined && tmp.id != id) {
        tmp = tmp.parent;
    }
    return (tmp !== undefined && tmp.id == id);
};

function showImagesForLocation(urls) {
    var flickr = $('#flickr_images').empty();

    for (var i = 0; i < 50 && i < urls.length; i++) {
        var url = urls[i];
        var bigimg = url.replace("_t.jpg", "_b.jpg");
        var link = $("<a />").attr("href", bigimg);
        $("<img/>").attr("src", url).appendTo(link);
        link.appendTo(flickr);
    }

    if (urls.length > 0) {
        $('#flickr').show();
    } else {
        $('#flickr').hide();
    }

}

function getImagesForLocation(name, geopos) {
    flickr.query(name, geopos, showImagesForLocation);
}

var flickr = {
    api_key: "46f7a6dce46471b81aa7c1592fcc9733",

    query: function (text, geopos, callback) {
        var urls = [];
        var service = 'https://api.flickr.com/services/rest/?method=flickr.photos.search&api_key='
            + this.api_key + '&text=' + text + '&format=json&nojsoncallback=1&sort=relevance';

        service += '&lat=' + geopos.lat + '&lon=' + geopos.lng;

        $.getJSON(service, function (data) {
            var photo;
            for (var i in data.photos.photo) {
                photo = data.photos.photo[i];
                urls.push('https://farm' + photo.farm + '.staticflickr.com/' + photo.server + '/' + photo.id + '_' + photo.secret + '_t.jpg');
            }
            callback(urls);
        });
    }
};