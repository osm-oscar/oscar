self.addEventListener('message', function (e) {
    importScripts('../turf/turf.min.js');
    var merged = turf.merge(e.data.shapes);
    self.postMessage({"merged" : merged});
}, false);