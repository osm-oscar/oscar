define(["dagre-d3", "d3", "jquery", "oscar", "state", "tools"], function (dagreD3, d3, $, oscar, state, tools) {
    var tree = {
        graph: undefined, // the graph
        renderer: new dagreD3.render(),

        /**
         * visualizes the DAG
         *
         * @param root
         */
        visualizeDAG: function (root) {
            state.visualizationActive = true;

            // Set up an SVG group so that we can translate the final graph.
            $("#dag").empty();
            var svg = d3.select("svg"),
                svgGroup = svg.append("g");

            tree._initGraph(svg, svgGroup);

            // build the graph from current DAG
            this._recursiveAddToGraph(root, this.graph);
            this._roundedNodes();

            $("#tree").css("display", "block");

            // draw graph
            svgGroup.call(this.renderer, this.graph);

            // Center the graph
            var xCenterOffset = ($("#tree").width() - this.graph.graph().width) / 2;
            svgGroup.attr("transform", "translate(" + xCenterOffset + ", 20)");
            svg.attr("height", this.graph.graph().height + 40);

            this._addInteractions();
        },

        _initGraph: function (svg, svgGroup) {
            // Create the input graph
            this.graph = new dagreD3.graphlib.Graph()
                .setGraph({
                    nodesep: 15,
                    ranksep: 75,
                    rankdir: "TB",
                    marginx: 10,
                    marginy: 10
                })
                .setDefaultEdgeLabel(function () {
                    return {};
                });

            // Set up zoom support
            var zoom = d3.behavior.zoom().on("zoom", function () {
                svgGroup.attr("transform", "translate(" + d3.event.translate + ")" +
                    "scale(" + d3.event.scale + ")");
            });
            svg.call(zoom);

            this.graph.graph().transition = function (selection) {
                return selection.transition().duration(500);
            };
        },

        /**
         * adds a click event to all "Show Children" links, which loads the sub-hierarchy
         *
         * @private
         */
        _addClickToLoadSubHierarchy: function () {
            $(".treeNodeSub").each(function (key, value) {
                $(value).on("click", function () {
                    var id = $(this).attr("id");
                    var node = state.DAG.at(id);
                    var marker = node.marker;
                    state.markers.removeLayer(marker);
                    state.items.clusters.drawn.erase(id);
                    map.loadSubhierarchy(id, function () {
                        //state.map.off("zoomend dragend", state.handler);
                        //state.map.fitBounds(node.bbox);
                        //state.map.on("zoomend dragend", state.handler);
                    });
                });
            });
        },

        _addClickToLoadItems: function () {
            $("#left_menu_parent").css("display", "block");
            $(".treeNodeItems").each(function (key, value) {
                $(value).on("click", function () {
                    var id = $(this).attr("id");
                    map.loadItems(id);
                });
            });
        },

        _recursiveAddToGraph: function (node, graph) {
            if (node.name) {
                this.graph.setNode(node.id, tree._nodeAttr(node));
                for (var child in node.children) {
                    if (node.children[child].count) {
                        this.graph.setNode(node.children[child].id);
                        this.graph.setEdge(node.id, node.children[child].id, {
                            lineInterpolate: 'basis',
                            class: "origin-" + node.id
                        });
                        this._recursiveAddToGraph(node.children[child], graph);
                    }
                }
            }
        },

        /**
         * returns the label for a leaf in the tree
         *
         * @param name of the node
         * @returns {string} label-string
         */
        _leafLabel: function (node) {
            return "<div class='treeNode'><div class='treeNodeName'>" + node.name.toString()
                + "<span class='badge'>" + node.count + "</span></div><a id='"
                + node.id + "' class='treeNodeItems' href='#'>Load Items</a></div>";
        },

        /**
         * returns the label for a node (non-leaf) in the tree
         *
         * @param name of the node
         * @param id of the node
         * @returns {string} label-string
         */
        _nodeLabel: function (node) {
            return "<div class='treeNode'><div class='treeNodeName'>" + node.name.toString()
                + "<span class='badge'>" + node.count + "</span></div><a id='" + node.id
                + "' class='treeNodeSub' href='#'>Show Children</a><a id='" + node.id
                + "' class='treeNodeItems' href='#'>Load Items</a></div>";
        },

        /**
         * returns the attributes for a node
         *
         * @param node TreeNode instance
         * @returns {*} attributes for the node
         */
        _nodeAttr: function (node) {
            if (!node.children.length || (node.children.length && !node.children[0].count)) {
                return {
                    labelType: "html",
                    label: tree._nodeLabel(node)
                };
            } else {
                return {
                    labelType: "html",
                    label: tree._leafLabel(node)
                }
            }
        },

        /**
         * mouseover effect for nodes in the tree
         *
         * @param id node-id
         * @private
         */
        _hoverNode: function (id) {
            d3.selectAll(".origin-" + id).selectAll("path").style("stroke", "#007fff");
        },

        /**
         * mouseout effect for nodes in the tree
         *
         * @param id node-id
         * @private
         */
        _deHoverNode: function (id) {
            d3.selectAll(".origin-" + id).selectAll("path").style("stroke", "black");
        },

        /**
         * rounds the edges of nodes
         *
         * @private
         */
        _roundedNodes: function () {
            this.graph.nodes().forEach(function (v) {
                var node = tree.graph.node(v);
                node.rx = node.ry = 5;
            });
        },

        /**
         * refreshs/redraws the tree, the node-id defines a node, which subtree changed
         *
         * @param id node-id
         */
        refresh: function (id) {
            // ugly hack: attributes for nodes cannot be changed, once set (or they will not be recognized). so we have
            // to remove the node & create a new one with the wished properties
            var pathtimer = tools.timer("refresh");
            var parents = this.graph.inEdges(id);
            this.graph.removeNode(id);
            //d3.select("svg").select("g").call(this.renderer, this.graph);
            this.graph.setNode(id, {label: state.DAG.at(id).name.toString(), labelStyle: "color: white"});
            for (var i in parents) {
                this.graph.setEdge(parents[i].v, id, {
                    lineInterpolate: 'basis',
                    class: "origin-" + parents[i].v
                });
            }

            // update the subtree of the clicked node
            this._recursiveAddToGraph(state.DAG.at(id), this.graph);
            this._roundedNodes();
            d3.select("svg").select("g").call(this.renderer, this.graph);
            this._addInteractions();
            pathtimer.stop();
        },

        /**
         * adds interactions to the graph-visualization
         * 1) mouseover effects
         * 2) mouseout effects
         * 3) possibility to load subhierarchy of nodes
         *
         * @private
         */
        _addInteractions: function () {
            d3.selectAll(".node").on("mouseover", this._hoverNode.bind(this));
            d3.selectAll(".node").on("mouseout", this._deHoverNode.bind(this));
            this._addClickToLoadSubHierarchy();
            this._addClickToLoadItems();
        },

        /**
         *
         * @param node to which should be found one path to the root
         */
        onePath: function (node) {
            function walker(node) {
                var parentNode;
                for (var parent in node.parents) {
                    parentNode = node.parents[parent];
                    if (!parentNode) {
                        continue;
                    }
                    if (walkerCounter.count(parentNode.id)) {
                        walkerCounter.insert(parentNode.id, walkerCounter.at(parentNode.id) + 1);
                    } else {
                        walkerCounter.insert(parentNode.id, 1);
                    }
                    walker(parentNode);
                }
            }

            $("#dag").empty();
            var svg = d3.select("svg"),
                svgGroup = svg.append("g");
            tree._initGraph(svg, svgGroup);

            var walkerCounter = tools.SimpleHash();
            var onPath = tools.SimpleHash();

            walker(node);

            var currentNode = state.DAG.at(0xFFFFFFFF); // root
            var childNode, nextNode, mostWalkers = 0;

            while (currentNode.id != node.id) {
                this.graph.setNode(currentNode.id, tree._nodeAttr(currentNode));

                for (var child in currentNode.children) {
                    childNode = currentNode.children[child];
                    if (childNode.id == node.id) {
                        nextNode = childNode;
                        break;
                    } else if (walkerCounter.at(childNode.id) > mostWalkers) {
                        nextNode = childNode;
                        mostWalkers = walkerCounter.at(childNode.id);
                    }
                }

                this.graph.setNode(nextNode.id);
                this.graph.setEdge(currentNode.id, nextNode.id, {
                    lineInterpolate: 'basis',
                    class: "origin-" + currentNode.id
                });

                onPath.insert(currentNode.id, currentNode);
                currentNode = nextNode;
                mostWalkers = 0;
            }

            this.graph.setNode(node.id, tree._nodeAttr(node));
            tree.refresh(node.id);
            d3.select("svg").select("g").call(this.renderer, this.graph);
            tree._addInteractions();
        },

        hideChildren: function (node) {
            var childNode;
            for (var child in node.children) {
                childNode = node.children[child];
                tree.graph.removeNode(childNode.id);
            }
        }

    };

    return tree;
})
;
