define(["jqueryui", "slimbox"], function () {
    return {
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
                    urls.push({
                        url: 'https://farm' + photo.farm + '.staticflickr.com/' + photo.server + '/' + photo.id + '_' + photo.secret + '_t.jpg',
                        title: photo.title
                    });
                }
                callback(urls);
            });
        },

        getImagesForLocation: function (name, geopos) {
            this.query(name, geopos, this._showImagesForLocation);
        },

        _showImagesForLocation: function (urls) {
            var flickr = $('#flickr_images').empty();

            for (var i = 0; i < 50 && i < urls.length; i++) {
                var obj = urls[i];
                var bigimg = obj.url.replace("_t.jpg", "_b.jpg");
                var link = $("<a />").attr("href", bigimg).attr("rel", "lightbox-group").attr("title", obj.title || "");
                $("<img/>").attr("src", obj.url).appendTo(link);
                link.appendTo(flickr);
            }

            if (urls.length > 0) {
                $('#flickr').show("slide", {direction: "right"}, myConfig.styles.slide.speed);
                // slimbox fails with dynamic content => updated added images
                $("a[rel^='lightbox']").slimbox({}, null, function (el) {
                    return (this == el) || ((this.rel.length > 8) && (this.rel == el.rel));
                });
            } else {
                $('#flickr').hide("slide", {direction: "right"}, myConfig.styles.slide.speed);
            }
        }
    }
});