/*the config has to have a property named url defining the url to the completer*/
define(['jquery', 'sserialize', 'leaflet', 'module'], function (jQuery, sserialize, L, module) {

    /** Code from http://www.artandlogic.com/blog/2013/11/jquery-ajax-blobs-and-array-buffers/
     * Register ajax transports for blob send/recieve and array buffer send/receive via XMLHttpRequest Level 2
     * within the comfortable framework of the jquery ajax request, with full support for promises.
     *
     * Notice the +* in the dataType string? The + indicates we want this transport to be prepended to the list
     * of potential transports (so it gets first dibs if the request passes the conditions within to provide the
     * ajax transport, preventing the standard transport from hogging the request), and the * indicates that
     * potentially any request with any dataType might want to use the transports provided herein.
     *
     * Remember to specify 'processData:false' in the ajax options when attempting to send a blob or arraybuffer -
     * otherwise jquery will try (and fail) to convert the blob or buffer into a query string.
     */
    jQuery.ajaxTransport("+*", function (options, originalOptions, jqXHR) {
        // Test for the conditions that mean we can/want to send/receive blobs or arraybuffers - we need XMLHttpRequest
        // level 2 (so feature-detect against window.FormData), feature detect against window.Blob or window.ArrayBuffer,
        // and then check to see if the dataType is blob/arraybuffer or the data itself is a Blob/ArrayBuffer
        if (window.FormData && ((options.dataType && (options.dataType === 'blob' || options.dataType === 'arraybuffer')) ||
                (options.data && ((window.Blob && options.data instanceof Blob) ||
                (window.ArrayBuffer && options.data instanceof ArrayBuffer)))
            )) {
            return {
                /**
                 * Return a transport capable of sending and/or receiving blobs - in this case, we instantiate
                 * a new XMLHttpRequest and use it to actually perform the request, and funnel the result back
                 * into the jquery complete callback (such as the success function, done blocks, etc.)
                 *
                 * @param headers
                 * @param completeCallback
                 */
                send: function (headers, completeCallback) {
                    var xhr = new XMLHttpRequest(),
                        url = options.url || window.location.href,
                        type = options.type || 'GET',
                        dataType = options.dataType || 'text',
                        data = options.data || null,
                        async = options.async || true,
                        key;

                    xhr.addEventListener('load', function () {
                        var response = {}, status, isSuccess;

                        isSuccess = xhr.status >= 200 && xhr.status < 300 || xhr.status === 304;

                        if (isSuccess) {
                            response[dataType] = xhr.response;
                        } else {
                            // In case an error occured we assume that the response body contains
                            // text data - so let's convert the binary data to a string which we can
                            // pass to the complete callback.
                            response.text = String.fromCharCode.apply(null, new Uint8Array(xhr.response));
                        }

                        completeCallback(xhr.status, xhr.statusText, response, xhr.getAllResponseHeaders());
                    });

                    xhr.open(type, url, async);
                    xhr.responseType = dataType;

                    for (key in headers) {
                        if (headers.hasOwnProperty(key)) xhr.setRequestHeader(key, headers[key]);
                    }
                    xhr.send(data);
                },
                abort: function () {
                    jqXHR.abort();
                }
            };
        }
    });

    return {

        completerBaseUrl: module.config().url,
        maxFetchItems: 100,
        maxFetchShapes: 100,
        maxFetchIdx: 100,
        cqrCounter: 0,
        itemCache: {},
        shapeCache: {},
        idxCache: {},
        cellIdxIdCache: {},
        cqrOps: {'(': '(', ')': ')', '+': '+', '-': '-', '/': '/', '^': '^'},
        cqrRegExpEscapes: {
            '*': '*',
            '(': '(',
            ')': ')',
            '|': '|',
            '.': '.',
            '^': '^',
            '$': '$',
            '[': ']',
            ']': ']',
            '-': '-',
            '+': '+',
            '?': '?',
            '{': '}',
            '}': '}',
            '=': '!'
        }, //*(|.^$)[]-+?{}=!,
        cqrEscapesRegExp: new RegExp("^[\*\(\|\.\^\$\)\[\]\-\+\?\{\}\=\!\,]$"),

        Item: function (d) {

            if (d.shape.t === 4) {
                for (i in d.shape.v.outer) {
                    for (j in d.shape.v.outer[i]) {
                        if (d.shape.v.outer[i][j].length !== 2) {
                            alert("BAM");
                            break;
                        }
                    }
                }
            }

            var myPtr = this;

            return {
                data: d,
                p: myPtr,

                id: function () {
                    return this.data.id;
                },
                osmid: function () {
                    return this.data.osmid;
                },
                type: function () {
                    return this.data.type;
                },
                score: function () {
                    return this.data.score;
                },
                size: function () {
                    return this.data.k.length;
                },
                key: function (pos) {
                    return this.data.k[pos];
                },
                value: function (pos) {
                    return this.data.v[pos];
                },
                name: function () {
                    if (!this.data || !this.data.k)
                        return "";
                    var languageName = window.navigator.userLanguage || window.navigator.language;
                    languageName = "name:" + languageName.substr(0, 2);
                    var namePos = this.data.k.indexOf(languageName);
                    if (namePos === -1) {
                        namePos = this.data.k.indexOf("name");
                    }

                    if (namePos !== -1) {
                        return this.data.v[namePos];
                    }
                    else {
                        return "HAS_NO_NAME";
                    }
                },
                asLeafletItem: function (successCB, errorCB) {
                    if (this.p.shapeCache[this.id()] !== undefined) {
                        try {
                            successCB(this.p.leafletItemFromShape(this.p.shapeCache[this.id()]));
                        }
                        catch (e) {
                            errorCB("", e);
                        }
                    }
                    else {
                        this.p.fetchShapes([this.id()], function () {
                            this.asLeafletItem(successCB, errorCB);
                        }, errorCB);
                    }
                },
                asDescList: function (dtCssClass, ddCssClass) {
                    var message = "";
                    for (var i = 0; i < this.data.k.length; ++i) {
                        message += '<dt class="' + dtCssClass + '">' + this.data.k[i] + '</dt><dd class="' + ddCssClass + '">' + this.data.v[i] + "</dd>";
                    }
                    return message;
                },
                toRadians: function (p) {
                    return p * Math.PI / 180;
                },
                toDegrees: function (p) {
                    return p * 180 / Math.PI;
                },
                centerPoint: function () {
                    // see http://mathforum.org/library/drmath/view/51822.html for derivation
                    var phi1 = this.toRadians(this.bbox()[0][0]), lambda1 = this.toRadians(this.bbox()[0][1]);
                    var phi2 = this.toRadians(this.bbox()[1][0]);
                    var deltalambda = this.toRadians((this.bbox()[1][1] - this.bbox()[0][1]));

                    var Bx = Math.cos(phi2) * Math.cos(deltalambda);
                    var By = Math.cos(phi2) * Math.sin(deltalambda);

                    var phi3 = Math.atan2(Math.sin(phi1) + Math.sin(phi2),
                        Math.sqrt((Math.cos(phi1) + Bx) * (Math.cos(phi1) + Bx) + By * By));
                    var lambda3 = lambda1 + Math.atan2(By, Math.cos(phi1) + Bx);
                    lambda3 = (lambda3 + 3 * Math.PI) % (2 * Math.PI) - Math.PI; // normalise to -180..+180?

                    return [this.toDegrees(phi3), this.toDegrees(lambda3)];
                },
                //[southWest, northEast]
                bbox: function () {
                    return [[this.data.bbox[0], this.data.bbox[2]], [this.data.bbox[1], this.data.bbox[3]]];
                }
            };
            /*end of item object*/
        },
//sqId is the sequence id of this cqr
        CellQueryResult: function (data, parent, sqId) {
            return {
                d: data,
                p: parent,
                sid: sqId,
                sequenceId: function () {
                    return this.sid;
                },
                regionIndexCache: {},
                isFull: function () {
                    return true;
                },
                hasResults: function () {
                    return this.d.cellInfo.length > 0;
                },
                subSet: function () {
                    return this.d.subSet;
                },
                query: function () {
                    return this.d.query;
                },
                cellPositions: function (regionId, dest) {
                    if (this.subSet().regions[regionId] !== undefined) {
                        var myRegion = this.subSet().regions[regionId];
                        var myCellPositions = myRegion.cellpositions;

                        for (i in myCellPositions) {
                            var cellPosition = myCellPositions[i];
                            dest[cellPosition] = cellPosition;
                        }

                        if (this.subSet().type === 'sparse') {
                            var myChildren = myRegion.children;
                            for (i in myChildren) {
                                this.cellPositions(myChildren[i], dest);
                            }
                        }
                    }
                },
                //get the complete index
                regionItemIndex: function (regionId, successCB, errorCB) {
                    if (this.subSet().regions[regionId] === undefined) {
                        errorCB("", "Invalid region");
                        return;
                    }
                    if (this.regionIndexCache[regionId] !== undefined) {
                        successCB(this.regionIndexCache[regionId]);
                        return;
                    }
                    var result = {};
                    var fMC = [];
                    var idcesToFetch = [];
                    var myRegion = this.subSet().regions[regionId];
                    var myCellPositions = {};
                    this.cellPositions(regionId, myCellPositions);
                    for (i in myCellPositions) {
                        var cellInfo = cqr.cellInfo[myCellPositions[i]];
                        if (cellInfo.fullIndex) {
                            fMC.push(cellInfo.cellId);
                        }
                        else if (!cellInfo.fetched) {
                            idcesToFetch.push(cellInfo.indexId);
                        }
                        else {
                            for (x in cellInfo.index.values) {
                                result[cellInfo.index.values[x]] = 0; //TODO:use a set data structure
                            }
                        }
                    }
                    var myPtr = this;
                    var finalFetch = function (successCB, errorCB) {
                        myPtr.p.getIndexes(idcesToFetch,
                            function (ideces) {
                                for (i in ideces) {
                                    var idx = ideces[i].values;
                                    for (j in idx) {
                                        result[idx[j]] = idx[j];
                                    }
                                }
                                var finalResult = [];
                                for (i in result) {
                                    finalResult.push(parseInt(i));
                                }
                                finalResult.sort();
                                myPtr.regionIndexCache[regionId] = finalResult;
                                successCB(finalResult);
                            },
                            errorCB
                        );
                    };
                    if (fMC.length) {//fetch the cell item index ids first
                        this.p.getCellsItemIndexIds(fMC,
                            function (idxIds) {
                                for (i = 0; i < idxIds.length; ++i) {
                                    idcesToFetch.push(idxIds[i]);
                                }
                                finalFetch(successCB, errorCB);
                            },
                            errorCB
                        );
                    }
                    else {
                        finalFetch(successCB, errorCB);
                    }
                },
                regionItemIds: function (regionId, successCB, errorCB, resultListOffset) {
                    var myPtr = this;
                    this.regionItemIndex(regionId,
                        function (index) {
                            successCB(index.slice(resultListOffset, resultListOffset + myPtr.p.maxFetchItems));
                        },
                        errorCB
                    );
                },
                ohPath: function () {
                    return [];
                },
                ohPathRegions: function () {
                    return [];
                }
            };
        },
//data is sserialize.SimpleCellQueryResult with { 'query' : string, regionId : <int> }
        SimpleCellQueryResult: function (data, parent, sqId) {
            var tmp = {
                p: parent,
                d: data,
                sid: sqId,
                sequenceId: function () {
                    return this.sid;
                },
                isFull: function () {
                    return false;
                },
                query: function () {
                    return this.d.query;
                },
                regionItemIds: function (regionId, successCB, errorCB, resultListOffset) {
                    this.p.simpleCqrItems(this.d.query,
                        function (itemIds) {
                            successCB(regionId, itemIds);
                        },
                        errorCB,
                        this.p.maxFetchItems, regionId, resultListOffset);
                },
                rootRegionChildrenInfo: function () {
                    return this.d.regionInfo[0xFFFFFFFF];
                },
                rootRegionApxItemCount: function () {
                    return this.d.rootRegionApxItemCount;
                },
                //returns [regionIds] in successCB,
                //maxOverlap in percent, prunes regions that overlap with more than maxOverlap percent cells of the currently selected set of children regions
                getMaximumIndependetSet: function (regionId, maxOverlap, successCB, errorCB) {
                    if (regionId === undefined) {
                        regionId = 0xFFFFFFFF;
                    }
                    this.p.simpleCqrMaxIndependentChildren(this.d.query, successCB, errorCB, regionId, maxOverlap);
                },
                //returning an array in successCB with objects={id : int, apxitems : int}
                //returns rootRegionChildrenInfo if regionId is undefined
                regionChildrenInfo: function (regionId, successCB, errorCB) {
                    if (regionId === undefined) {
                        regionId = 0xFFFFFFFF;
                    }
                    var tmp = this.d.regionInfo[regionId];
                    if (tmp !== undefined) {
                        successCB(tmp);
                    }
                    else {
                        var myPtr = this;
                        this.p.simpleCqrChildren(this.d.query,
                            function (regionInfo) {
                                myPtr.d.regionInfo[regionId] = regionInfo[regionId];
                                successCB(myPtr.d.regionInfo[regionId]);
                            },
                            errorCB,
                            regionId);
                    }
                },
                hasResults: function () {
                    var tmp = this.d.regionInfo[0xFFFFFFFF];
                    return tmp !== undefined && tmp.length > 0;
                },
                ohPath: function () {
                    return this.d.ohPath;
                }
            };
            return tmp;
        },
        leafletItemFromShape: function (shape) {
            switch (shape.t) {
                case 1://GeoPoint
                    return L.circle(shape.v, 10.0);
                case 2://GeoWay
                    return L.polyline(shape.v);
                case 3://GeoPolygon
                    return L.polygon(shape.v);
                case 4://geo multi polygon
                    return L.multiPolygon(shape.v.outer);
                default:
                    throw Error("oscar::leafletItemFromShape: invalid shape");
                    return null;
            }
        },

///Fetches the items in arrayOfItemIds and puts them into the cache. notifies successCB
        fetchItems: function (arrayOfItemIds, successCB, errorCB) {
            var fetchItems = [];
            var remainingFetchItems = [];
            for (i = 0; i < arrayOfItemIds.length; ++i) {
                if (this.itemCache[arrayOfItemIds[i]] === undefined) {
                    if (fetchItems.length < this.maxFetchItems) {
                        fetchItems.push(arrayOfItemIds[i]);
                    }
                    else {
                        remainingFetchItems.push(arrayOfItemIds[i]);
                    }
                }
            }
            if (!fetchItems.length) {
                successCB();
                return;
            }
            var params = {};
            params['which'] = JSON.stringify(fetchItems);
            params['shape'] = "false";
            var qpath = this.completerBaseUrl + "/itemdb/multiple";
            var myPtr = this;
            jQuery.ajax({
                type: "POST",
                url: qpath,
                data: params,
                mimeType: 'text/plain',
                success: function (plain) {
                    try {
                        json = JSON.parse(plain);
                    }
                    catch (err) {
                        errorCB("Parsing the fetched items failed with the following parameters: " + JSON.stringify(params), err);
                        return;
                    }
                    for (var itemD in json) {
                        var item = myPtr.Item(json[itemD]);
                        myPtr.itemCache[item.id()] = item;
                    }
                    if (json.length > 0 && json.length < fetchItems.length) {
                        remainingFetchItems.splice(remainingFetchItems.length, 0, fetchItems);
                    }
                    if (remainingFetchItems.length) {
                        myPtr.fetchItems(remainingFetchItems, function () {
                            successCB();
                        }, errorCB);
                    }
                    else {
                        successCB();
                    }
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },
        /*
         on success: successCB is called with a single Item instance
         on error: errorCB is called with (textStatus, errorThrown)
         */
        getItem: function (itemId, successCB, errorCB) {
            var myPtr = this;
            this.fetchItems([itemId],
                function () {
                    var item = myPtr.itemCache[itemId];
                    if (item === undefined) {
                        errorCB("Fetching items failed", "");
                        return;
                    }
                    successCB(item);
                },
                function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            );
        },

        /*
         on success: successCB is called with an array of Items()
         on error: errorCB is called with (textStatus, errorThrown)
         */
        getItems: function (arrayOfItemIds, successCB, errorCB) {
            var myPtr = this;
            this.fetchItems(arrayOfItemIds,
                function () {
                    var res = [];
                    for (var i = 0; i < arrayOfItemIds.length; ++i) {
                        var item = myPtr.itemCache[arrayOfItemIds[i]];
                        if (item === undefined) {
                            errorCB("Fetching items failed", "Item with id " + arrayOfItemIds[i] + " is undefined");
                            return;
                        }
                        res.push(item);
                    }
                    successCB(res);

                },
                function (textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            );
        },

        fetchShapes: function (arrayOfItemIds, successCB, errorCB) {
            var fetchShapes = [];
            var remainingFetchShapes = [];
            for (i = 0; i < arrayOfItemIds.length; ++i) {
                var tmpItemId = parseInt(arrayOfItemIds[i]);
                if (this.shapeCache[tmpItemId] === undefined) {
                    if (fetchShapes.length < this.maxFetchShapes) {
                        fetchShapes.push(tmpItemId);
                    }
                    else {
                        remainingFetchShapes.push(tmpItemId);
                    }
                }
            }
            if (!fetchShapes.length) {
                successCB();
                return;
            }
            var params = {};
            params['which'] = JSON.stringify(fetchShapes);
            var qpath = this.completerBaseUrl + "/itemdb/multipleshapes";
            var myPtr = this;
            jQuery.ajax({
                type: "POST",
                url: qpath,
                data: params,
                mimeType: 'text/plain',
                success: function (plain) {
                    try {
                        json = JSON.parse(plain);
                    }
                    catch (err) {
                        errorCB("Parsing the fetched shapes failed with the following parameters: " + JSON.stringify(params), err);
                        return;
                    }
                    for (var shapeD in json) {
                        myPtr.shapeCache[shapeD] = json[shapeD];
                    }
                    if (json.length > 0 && json.length < fetchShapes.length) {
                        remainingFetchShapes.splice(remainingFetchShapes.length, 0, fetchShapes);
                    }
                    if (remainingFetchShapes.length) {
                        myPtr.fetchShapes(remainingFetchShapes, function () {
                            successCB();
                        }, errorCB);
                    }
                    else {
                        successCB();
                    }
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },
        /*
         on success: successCB is called with an object of shapes: { id : shape-description }
         on error: errorCB is called with (textStatus, errorThrown)
         */
        getShapes: function (arrayOfItemIds, successCB, errorCB) {
            var myPtr = this;
            this.fetchShapes(arrayOfItemIds,
                function () {
                    var res = {};
                    for (var i = 0; i < arrayOfItemIds.length; ++i) {
                        var sid = arrayOfItemIds[i];
                        var shape = myPtr.shapeCache[sid];
                        if (shape === undefined) {
                            errorCB("Fetching shapes failed", "Shape with id " + sid + " is undefined");
                            return;
                        }
                        res[sid] = shape;
                    }
                    successCB(res);

                },
                function (textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            );
        },
        /*
         on success: successCB is called with a single Item instance
         on error: errorCB is called with (textStatus, errorThrown)
         */
        getShape: function (itemId, successCB, errorCB) {
            var myPtr = this;
            this.fetchShapes([itemId],
                function () {
                    var shape = myPtr.shapeCache[itemId];
                    if (shape === undefined) {
                        errorCB("Fetching shape failed", "");
                        return;
                    }
                    successCB(shape);
                },
                function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            );
        },

        fetchIndexes: function (arrayOfIndexIds, successCB, errorCB) {
            var idcsToFetch = [];
            var remainingIdcsToFetch = [];
            for (i in arrayOfIndexIds) {
                if (this.idxCache[arrayOfIndexIds[i]] === undefined) {
                    if (idcsToFetch.length < this.maxFetchIdx) {
                        idcsToFetch.push(arrayOfIndexIds[i]);
                    }
                    else {
                        remainingIdcsToFetch.push(arrayOfIndexIds[i]);
                    }
                }
            }
            if (!idcsToFetch.length) {
                successCB();
                return;
            }
            var params = {};
            params['which'] = JSON.stringify(idcsToFetch);
            var qpath = this.completerBaseUrl + "/indexdb/multiple";
            var myPtr = this;
            jQuery.ajax({
                type: "POST",
                url: qpath,
                data: params,
                dataType: 'arraybuffer',
                mimeType: 'application/octet-stream',
                success: function (data) {
                    idcs = sserialize.itemIndexSetFromRaw(data);
                    for (i = 0; i < idcs.length; ++i) {
                        var idx = idcs[i];
                        myPtr.idxCache[idx.id] = idx;
                    }
                    if (idcs.length > 0 && idcs.length < idcsToFetch.length) {
                        //some indeces are still missing, try again
                        remainingIdcsToFetch.splice(remainingIdcsToFetch.length, 0, idcsToFetch);
                    }
                    if (remainingIdcsToFetch.length) {
                        myPtr.fetchIndexes(remainingIdcsToFetch, function () {
                            successCB();
                        }, errorCB);
                    }
                    else {
                        successCB();
                    }
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },
        /*
         on success: successCB is called with an sserialize::ItemIndex
         on error: errorCB is called with (textStatus, errorThrown)
         */
        getIndex: function (indexId, successCB, errorCB) {
            var myPtr = this;
            this.fetchIndexes([indexId],
                function () {
                    successCB(myPtr.idxCache[indexId]);
                },
                function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            );
        },
        /*
         on success: successCB is called with a sserialize::ItemIndexSet
         on error: errorCB is called with (textStatus, errorThrown)
         */
        getIndexes: function (arrayOfIndexIds, successCB, errorCB) {
            var myPtr = this;
            this.fetchIndexes(arrayOfIndexIds,
                function () {
                    var res = [];
                    for (i = 0; i < arrayOfIndexIds.length; ++i) {
                        res[i] = myPtr.idxCache[arrayOfIndexIds[i]];
                    }
                    successCB(res);
                },
                function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            );
        },
        fetchCellsItemIndexIds: function (arrayOfCellIds, successCB, errorCB) {
            var cellsIdxIdsToFetch = [];
            for (i in arrayOfCellIds) {
                if (this.cellIdxIdCache[arrayOfCellIds[i]] === undefined) {
                    cellsIdxIdsToFetch.push(arrayOfCellIds[i]);
                }
            }
            if (!cellsIdxIdsToFetch.length) {
                successCB();
                return;
            }
            var params = {};
            params['which'] = JSON.stringify(cellsIdxIdsToFetch);
            var qpath = this.completerBaseUrl + "/indexdb/cellindexids";
            var myPtr = this;
            jQuery.ajax({
                type: "POST",
                url: qpath,
                data: params,
                dataType: 'arraybuffer',
                mimeType: 'application/octet-stream',
                success: function (data) {
                    tmp = sserialize.asArray(data, 'uint32');
                    for (i = 0; i < tmp.length; ++i) {
                        myPtr.cellIdxIdCache[cellsIdxIdsToFetch[i]] = tmp[i];
                    }
                    if (tmp.length > 0 && tmp.length < cellsIdxIdsToFetch.length) {
                        myPtr.fetchCellsItemIndexIds(cellsIdxIdsToFetch, function () {
                            successCB();
                        }, errorCB);
                    }
                    else {
                        successCB();
                    }
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },

        getCellsItemIndexIds: function (arrayOfCellIds, successCB, errorCB) {
            var myPtr = this;
            this.fetchCellsItemIndexIds(arrayOfCellIds,
                function () {
                    var res = [];
                    for (i in arrayOfCellIds) {
                        res[i] = myPtr.cellIdxIdCache[arrayOfCellIds[i]];
                    }
                    successCB(res);
                },
                errorCB
            );
        },
        getItemParentIds: function (itemId, successCB, errorCB) {
            var qpath = this.completerBaseUrl + "/itemdb/itemparents/" + itemId;
            jQuery.ajax({
                type: "GET",
                url: qpath,
                dataType: 'arraybuffer',
                mimeType: 'application/octet-stream',
                success: function (raw) {
                    res = sserialize.asU32Array(raw);
                    successCB(res);
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },
        getItemsRelativesIds: function (itemId, successCB, errorCB) {
            var qpath = this.completerBaseUrl + "/itemdb/itemrelatives/" + itemId;
            jQuery.ajax({
                type: "GET",
                url: qpath,
                dataType: 'arraybuffer',
                mimeType: 'application/octet-stream',
                success: function (raw) {
                    res = sserialize.asU32Array(raw);
                    successCB(res);
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },
        /*
         on success: successCB is called with an array of itemids
         */
        getTopKItemIds: function (cqr, cellPositions, k, successCB, errorCB) {
            var params = {};
            params['q'] = cqr.query;
            params['which'] = JSON.stringify(cellPositions);
            params['k'] = k;
            var qpath = this.completerBaseUrl + "/cqr/clustered/items";
            jQuery.ajax({
                type: "POST",
                url: qpath,
                data: params,
                mimeType: 'application/json',
                success: function (jsondesc) {
                    successCB(JSON.parse(jsondesc));
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },
        /*
         on success: successCB is called with sserialize/ReducedCellQueryResult representing matching cells
         */
        completeReduced: function (query, successCB, errorCB) {
            var params = {};
            params['q'] = query;
            params['sst'] = "binary";
            var qpath = this.completerBaseUrl + "/cqr/clustered/reduced";
            jQuery.ajax({
                type: "GET",
                url: qpath,
                data: params,
                dataType: 'arraybuffer',
                mimeType: 'application/octet-stream',
                success: function (raw) {
                    cqr = sserialize.reducedCqrFromRaw(raw);
                    cqr.query = params['q'];
                    successCB(cqr);
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },
        /*
         on success: successCB is called with the sserialize/CellQueryResult
         on error: errorCB is called with (textStatus, errorThrown)
         */
        completeFull: function (query, successCB, errorCB) {
            var params = {};
            params['q'] = query;
            params['sst'] = "binary";
            //params['sst'] = "flatjson";
            var qpath = this.completerBaseUrl + "/cqr/clustered/full";
            var myPtr = this;
            var mySqId = this.cqrCounter;
            this.cqrCounter += 1;
            jQuery.ajax({
                type: "GET",
                url: qpath,
                data: params,
                dataType: 'arraybuffer',
                mimeType: 'application/octet-stream',
                success: function (raw) {
                    cqr = sserialize.cqrFromRaw(raw);
                    cqr.query = params['q'];
                    successCB(myPtr.CellQueryResult(cqr, myPtr, mySqId));
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },
        completeSimple : function(query, successCB, errorCB, ohf, globalOht) {
            var params = {};
            params['q'] = query;
            if (ohf !== undefined) {
                params['oh'] = ohf;
            }
            if (globalOht) {
                params['oht'] = 'global';
            }
            else {
                params['oht'] = 'relative';
            }
            var qpath = this.completerBaseUrl + "/cqr/clustered/simple";
            var myPtr = this;
            var mySqId = this.cqrCounter;
            this.cqrCounter += 1;
            jQuery.ajax({
                type: "GET",
                url: qpath,
                data: params,
                dataType : 'arraybuffer',
                mimeType: 'application/octet-stream',
                success: function( raw ) {
                    var cqr = sserialize.simpleCqrFromRaw(raw);
                    cqr.query = params['q'];
                    successCB(myPtr.SimpleCellQueryResult(cqr, myPtr, mySqId));
                },
                error: function(jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },
        simpleCqrItems: function (query, successCB, errorCB, numItems, selectedRegion, resultListOffset) {
            var params = {};
            params['q'] = query;
            if (numItems !== undefined) {
                params['k'] = numItems;
            }
            if (resultListOffset !== undefined) {
                params['o'] = resultListOffset;
            }
            if (selectedRegion !== undefined) {
                params['r'] = selectedRegion;
            }
            var qpath = this.completerBaseUrl + "/cqr/clustered/items";
            jQuery.ajax({
                type: "GET",
                url: qpath,
                data: params,
                dataType: 'arraybuffer',
                mimeType: 'application/octet-stream',
                success: function (raw) {
                    itemIds = sserialize.asU32Array(raw);
                    successCB(itemIds);
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },
        simpleCqrChildren: function (query, successCB, errorCB, selectedRegion) {
            var params = {};
            params['q'] = query;
            if (selectedRegion !== undefined) {
                params['r'] = selectedRegion;
            }
            var qpath = this.completerBaseUrl + "/cqr/clustered/children";
            jQuery.ajax({
                type: "GET",
                url: qpath,
                data: params,
                dataType: 'arraybuffer',
                mimeType: 'application/octet-stream',
                success: function (raw) {
                    praw = sserialize.asU32Array(raw);
                    var childrenInfo = [];
                    for (var i = 0; i < praw.length; i += 2) {
                        childrenInfo.push({'id': praw[i], 'apxitems': praw[i + 1]});
                    }
                    var res = {};
                    childrenInfo.sort(function (a, b) {
                        return b['apxitems'] - a['apxitems'];
                    });
                    res[selectedRegion] = childrenInfo;
                    successCB(res);
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },
        simpleCqrMaxIndependentChildren: function (query, successCB, errorCB, selectedRegion, maxOverlap) {
            var params = {};
            params['q'] = query;
            if (selectedRegion !== undefined) {
                params['r'] = selectedRegion;
            }
            if (maxOverlap !== undefined) {
                params['o'] = maxOverlap;
            }
            var qpath = this.completerBaseUrl + "/cqr/clustered/michildren";
            jQuery.ajax({
                type: "GET",
                url: qpath,
                data: params,
                mimeType: 'application/json',
                success: function (jsondesc) {
                    successCB(jsondesc);
                },
                error: function (jqXHR, textStatus, errorThrown) {
                    errorCB(textStatus, errorThrown);
                }
            });
        },
        cqrRexExpFromQuery: function (query) {
            myRegExpStr = "";
            var tokens = [];
            var tokenString = "";
            var qtype = 'substring';
            for (i = 0; i < query.length; ++i) {
                while (query[i] === ' ' || this.cqrOps[query[i]] !== undefined) { //skip whitespace, ops and braces
                    ++i;
                }
                if (query[i] === '?') {
                    qtype = 'suffix';
                    ++i;
                }
                if (query[i] === '"') {
                    if (qtype === 'substring') {
                        qtype = 'exact';
                    }
                    ++i;
                    while (i < query.length) {
                        if (query[i] == '\\') {
                            ++i;
                            if (i < query.length) {
                                tokenString += query[i];
                                ++i;
                            }
                            else {
                                break;
                            }
                        }
                        else if (query[i] == '"') {
                            ++i;
                            break;
                        }
                        else {
                            tokenString += query[i];
                            ++i;
                        }
                    }
                    if (i < query.length && query[i] === '?') {
                        if (qtype === 'exact') {
                            qtype = 'prefix';
                        }
                        else { //qtype is suffix
                            qtype = 'substring';
                        }
                        ++i;
                    }
                }
                else {
                    while (i < query.length) {
                        if (query[i] === '\\') {
                            ++i;
                            if (i < query.length) {
                                tokenString += query[i];
                                ++i;
                            }
                            else {
                                break;
                            }
                        }
                        else if (query[i] === ' ' || query[i] === '?') {
                            break;
                        }
                        else if (query[i] === '(' || query[i] === ')') {
                            break;
                        }
                        else if (this.cqrOps[query[i]] !== undefined) {
                            if (tokenString.length && tokenString[tokenString.length - 1] === ' ') {
                                break;
                            }
                            else {
                                tokenString += query[i];
                                ++i;
                            }
                        }
                        else {
                            tokenString += query[i];
                            ++i;
                        }
                    }
                    if (i < query.length && query[i] === '?') {
                        if (qtype === 'exact') {
                            qtype = 'prefix';
                        }
                        else { //qtype is suffix
                            qtype = 'substring';
                        }
                        ++i;
                    }
                }
                tokens.push({value: tokenString, qtype: qtype});
                tokenString = "";
            }
            var totalRegExp = "^(";
            for (i in tokens) {
                var token = tokens[i];
                var tokenRegExp = "";
                var j = 0;
                if (token.value[0] === '@') { //tag query
                    j = 1;
                    if (token.qtype === 'substring') {
                        token.qtype = 'prefix';
                    }
                }
                else {//name query, add name-tagaddr:street
                    tokenRegExp += "((name(:.?.?)?)|ref|int_ref|(addr:(street|city|place|country|hamlet|suburb|subdistrict|district|province|state))):";
                }
                if (token.qtype === 'substring' || token.qtype === 'suffix') {
                    tokenRegExp += ".*";
                }
                for (; j < token.value.length; ++j) {
                    var curChar = token.value[j];
                    var regExpStr = this.cqrEscapesRegExp.toString();
                    if (this.cqrEscapesRegExp.test(curChar)) {
                        tokenRegExp += "\\";
                    }
                    tokenRegExp += token.value[j];
                }
                if (token.qtype === 'substring' || token.qtype === 'prefix') {
                    tokenRegExp += ".*";
                }
                if (totalRegExp[totalRegExp.length - 1] === ')') {
                    totalRegExp += "|";
                }
                totalRegExp += "(" + tokenRegExp + ")";
            }
            totalRegExp += ")$";
            return new RegExp(totalRegExp, "i");
        }

    };
    /*end of returned object*/

});
/*end of define and function*/

