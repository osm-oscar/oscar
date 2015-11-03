/*GPL-v2*/
define(['jbinary'], function (jBinary) { return {
	__funDefs : {
		'jBinary.littleEndian': true,
		'jBinary.mimeType': 'application/octet-stream',
		
		//A ItemIndexRleDe, returns a single array
		ItemIndexRleDe : jBinary.Template({
			setParams: function (dataLength) {
				this.dataLength = dataLength;
				this.baseType = { values: ['array', 'uint8', dataLength] };
			},

			read: function () {
				var res = [];
				var d = this.baseRead();
				if (this.dataLength > 0) {
					var tmp = [];
					for(var i=0, j=0; j < d.values.length; ++i) {
						var retVal = 0;
						var myLen = 0;
						var myShift = 0;
						do {
							retVal |= (d.values[j+myLen] & 0x7F) << myShift;
							++myLen;
							myShift += 7;
						}
						while (myLen < 5 && d.values[j+myLen-1] & 0x80);
						j +=  myLen;
						
						tmp[i] = retVal;
					}
					//res[d.size-1] = 0;
					//now decode the array, the first two entries are size and count, so skip these
					d.size = tmp[0];
					var prev = 0;
					var i = 2;
					for(j=0; i < tmp.length && j < d.size;) {
						val = tmp[i]; ++i;
						if (val & 0x1) {
							rle = (val >> 1);
							val = tmp[i]; ++i;
							val >>= 1;

							--rle;//acount for last bit set
							while(rle) {
								prev += val;
								res[j] = prev; ++j;
								--rle;
							}
							prev += val;//acount for last bit set
							res[j] = prev; ++j;
						}
						else {
							prev += (val >> 1);
							res[j] = prev; ++j;
						}
					}
					if (res.length !== d.size || i < tmp.length) {
						throw new RangeError("ItemIndexDecoding failed with out of bounds");
					}
				}

				/*padding alignment to 4 bytes*/
				var offsetOverhead = this.dataLength % 4;
				if (offsetOverhead) {
					this.binary.skip(4 - offsetOverhead);
					this.binary.view._bitOffset = 0;
				}

				return res;
			}
		}),
		ItemIndexNative : jBinary.Template({
			setParams: function () {
				this.baseType = { count: 'uint32' };
			},

			read: function () {
				var d = this.baseRead();
				var dv = this.binary.read({values: ['array', 'uint32', d.count]});
				if (dv.values === undefined) {
					console.log("ItemIndexNative: values are undefined");
					return [];
				}
				else {
					return dv.values;
				}
			}
		}),
		//A single ItemIndex, parses raw data and returns {id : int, values : [int]}
		ItemIndex: jBinary.Template({
			setParams: function (itemIndexType) {
				this.baseType = { id: 'uint32', dataLength: 'uint32'};
				this.itemIndexType = itemIndexType;
			},

			read: function () {
				var d = this.baseRead();
				var idx;
				if (!d.dataLength) {
					idx = [];
				}
				else {
					var tmp;
					if (this.itemIndexType === 16) {
						tmp = this.binary.read({ value : ['ItemIndexRleDe', d.dataLength] });
					}
					else if (this.itemIndexType === 32) {
						tmp = this.binary.read({ value : 'ItemIndexNative'});
					}
					else {
						throw TypeError("Unsupported ItemIndex type: " + itemIndexType);
					}
					idx = tmp.value;
				}
				return {id : d.id, values : idx};
			}
		}),
	   
	   ItemIndexSet: jBinary.Template({
			setParams: function () {
				this.baseType = {
									version: 'uint32',
									idxformat: 'uint32',
									idxcompression: 'uint32',
									idxcount: 'uint32'
								};
			},

			read: function () {
				var header = this.baseRead();
				var d = this.binary.read({ values : ['array', ['ItemIndex', header.idxformat], header.idxcount]});
				return d.values;
			}
		}),
		BinarySubSet: jBinary.Template({
			setParams: function () {
				this.baseType = {
									type: 'uint8',
									rootChildrenSize: 'uint32',
									rootChildrenIds: ['array', 'uint32', 'rootChildrenSize'],
									data : ['array', 'uint32']
								};
			},

			read: function () {
				var d = this.baseRead();
				var res = {"type" : (d.type == 's' ? "full" : "sparse"), "rootchildren" : d.rootChildrenIds, regions : {}};
				for(i=0; i < d.data.length;) {
					var myId = d.data[i];
					var myApxitems = d.data[i+1];
					var myCellpositionsSize = d.data[i+2];
					var myChildrenSize = d.data[i+3]
					var myCellPositions = d.data.slice(i+4, i+4+myCellpositionsSize);
					var myChildren = d.data.slice(i+4+myCellpositionsSize, i+4+myCellpositionsSize+myChildrenSize);
					res.regions[myId] = {"apxitems" : myApxitems, "cellpositions" : myCellPositions, "children" : myChildren};
					i += 4+myCellpositionsSize+myChildrenSize;
				}
				
				return res;
			}
		}),
		///An array of CellInfo objects
		///A CellInfo = { .cellId = int, .fetched=bool, .fullIndex = bool, fetched?(.index = [int] : .indexId) }
	   CellQueryResult: jBinary.Template({
			setParams: function () {
				this.baseType = {
									cellInfo: 'U32Array',
									indcs: 'ItemIndexSet',
									cellIdxIds: 'U32Array',
									subSet : 'BinarySubSet'
									//subSet: ['string', undefined, 'utf-8']
								};
			},

			read: function () {
				var d = this.baseRead();
				//var mySubSet = JSON.parse(d.subSet);
				var mySubSet = d.subSet;
				var res = {cellInfo: [], subSet : mySubSet };
				if (d.cellInfo.length > 0) {
					function CellInfo(cellInfo) {
						this.cellId = cellInfo >> 2;
						this.fetched = cellInfo & 0x2;
						this.fullIndex = cellInfo & 0x1;
					}
					
					for(var i=0, f=0, j=0; i < d.cellInfo.length; ++i) {
						var cI = new CellInfo(d.cellInfo[i]);
						if (cI.fetched) {
							cI.index = d.indcs[f];
							++f;
						}
						else if (!cI.fullIndex) {
							cI.indexId = d.cellIdxIds[j];
							++j;
						}
						res.cellInfo[i] = cI;
					}
				}
				return res;
			}
		}),
		//Returns {
		//			'ohPath' : [region ids as int], 
		//			regionInfo : {
		//							regionid : [{'id' : <childid>, 'apxitems' : <apxitems in child>}]
		//			}
		//}
		//children of regions are sorted in descending order of their apxitems 
		
	    SimpleCellQueryResult: jBinary.Template({
			setParams: function () {
				this.baseType = {
									ohPath: 'U32Array',
									childrenInfoSize : 'uint32', //region children
									childrenInfo : ['array', 'U32Array', 'childrenInfoSize']
								};
			},

			read: function () {
				var d = this.baseRead();
				var myRegionInfo = {}
				for(i=0; i < d.childrenInfo.length; ++i) {
					var regionInfo = d.childrenInfo[i];
					var regionId = regionInfo[0];
					var myChildrenInfo = [];
					for(j=1, count=0; j < regionInfo.length; j += 2, ++count) {
						myChildrenInfo[count] = {
													'id' : regionInfo[j],
													'apxitems' : regionInfo[j+1]
												};
					}
					myChildrenInfo.sort(function(a, b) {
											return b['apxitems'] - a['apxitems'];
										});
					myRegionInfo[regionId] = myChildrenInfo;
				}
				return {ohPath : d.ohPath, regionInfo : myRegionInfo};
			}
		}),
	   ReducedCellQueryResult: jBinary.Template({
			setParams: function () {
				this.baseType = {
									index: 'ItemIndex',
									subSet : 'BinarySubSet'
								};
			},

			read: function () {
				var d = this.baseRead();
				var res = {index: d.index, subSet: d.subSet};
				return res;
			}
		}),
		U32Array: jBinary.Template({
			setParams: function () {
				this.baseType = {
									count: 'uint32',
									value : ['array', 'uint32', 'count']
								};
			},

			read: function () {
				var d = this.baseRead();
				return d.value;
			}
		})
	},//end __fundefs
		//Returns a sserialize.ItemIndex, oscar-www currently only returns ItemIndexSet even for single index calls
		itemIndexFromRaw: function(raw) {
			var tmp = new jBinary(raw, this.__funDefs);
			return tmp.read('ItemIndex', 0);
		},
		itemIndexSetFromRaw: function(raw) {
			var tmp = new jBinary(raw, this.__funDefs);
			return tmp.read('ItemIndexSet', 0);
		},
		cqrFromRaw: function(raw) {
			var tmp = new jBinary(raw, this.__funDefs);
			return tmp.read('CellQueryResult', 0);
		},
		simpleCqrFromRaw: function(raw) {
			var tmp = new jBinary(raw, this.__funDefs);
			return tmp.read('SimpleCellQueryResult', 0);
		},
		reducedCqrFromRaw: function(raw) {
			var tmp = new jBinary(raw, this.__funDefs);
			return tmp.read('ReducedCellQueryResult', 0);
		},
		asArray: function(raw, tn) {
			var tmp = new jBinary(raw, this.__funDefs);
			var ret = tmp.read(['array', tn]);
			return ret;
		},
		asU32Array: function(raw) {
			var tmp = new jBinary(raw, this.__funDefs);
			return tmp.read('U32Array', 0);	
		}
		
};});//end of module return
