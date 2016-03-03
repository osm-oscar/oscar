define(["dagre-d3", "d3", "jquery", "oscar", "state"], function (dagreD3, d3, $, oscar, state) {
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

            // build the graph from current DAG
            this._recursiveAddToGraph(root, this.graph);
            this._roundedNodes();

            $("#tree").css("display", "block");

            // Set up an SVG group so that we can translate the final graph.
            $("#dag").empty();
            var svg = d3.select("svg"),
                svgGroup = svg.append("g");

            // Set up zoom support
            var zoom = d3.behavior.zoom().on("zoom", function () {
                svgGroup.attr("transform", "translate(" + d3.event.translate + ")" +
                    "scale(" + d3.event.scale + ")");
            });
            svg.call(zoom);

            this.graph.graph().transition = function (selection) {
                return selection.transition().duration(500);
            };

            // draw graph
            svgGroup.call(this.renderer, this.graph);

            // Center the graph
            var xCenterOffset = ($("#tree").width() - this.graph.graph().width) / 2;
            svgGroup.attr("transform", "translate(" + xCenterOffset + ", 20)");
            svg.attr("height", this.graph.graph().height + 40);

            this._addInteractions();
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
                    var marker = state.DAG.at(id).marker;
                    state.markers.removeLayer(marker);
                    state.items.clusters.drawn.erase(id);
                    map.loadSubhierarchy(id);
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
            /**
             * returns the label for a leaf in the tree
             *
             * @param name of the node
             * @returns {string} label-string
             */
            function leafLabel(node) {
                return "<div class='treeNode'><div class='treeNodeName'>" + node.name.toString() + "<span class='badge'>" + node.count + "</span></div><a id='" + node.id + "' class='treeNodeItems' href='#'>Load Items</a></div>";
            }

            /**
             * returns the label for a node (non-leaf) in the tree
             *
             * @param name of the node
             * @param id of the node
             * @returns {string} label-string
             */
            function nodeLabel(node) {
                return "<div class='treeNode'><div class='treeNodeName'>" + node.name.toString() + "<span class='badge'>" + node.count + "</span></div><a id='" + node.id + "' class='treeNodeSub' href='#'>Show Children</a><a id='" + node.id + "' class='treeNodeItems' href='#'>Load Items</a></div>";
            }

            /**
             * returns the attributes for a node
             *
             * @param node TreeNode instance
             * @returns {*} attributes for the node
             */
            function nodeAttr(node) {
                if (!node.children.length || (node.children.length && !node.children[0].count)) {
                    return {
                        labelType: "html",
                        label: nodeLabel(node)
                    };
                } else {
                    return {
                        labelType: "html",
                        label: leafLabel(node)
                    }
                }
            }

            if (node.name) {
                this.graph.setNode(node.id, nodeAttr(node));
                for (var child in node.children) {
                    if (node.children[child].name) {
                        this.graph.setNode(node.children[child].id);
                        this.graph.setEdge(node.id, node.children[child].id, {
                            lineInterpolate: 'basis',
                            class: "origin-" + node.id
                        });
                        this._recursiveAddToGraph(node.children[child], graph)
                    }
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
            var parents = this.graph.inEdges(id);
            this.graph.removeNode(id);
            d3.select("svg").select("g").call(this.renderer, this.graph);
            this.graph.setNode(id, {label: state.DAG.at(id).name.toString(), labelStyle: "color: white"});
            for (var i in parents) {
                this.graph.setEdge(parents[i].v, id, {
                    lineInterpolate: 'basis',
                    class: "origin-" + parents[i].v,
                });
            }

            // update the subtree of the clicked node
            this._recursiveAddToGraph(state.DAG.at(id), this.graph);
            this._roundedNodes();
            d3.select("svg").select("g").call(this.renderer, this.graph);
            this._addInteractions();
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

        onePath: function (node) {

            function removeSiblingsAndSubTrees(node){
                var parentNode, siblingNode;
                for(var parent in node.parents){
                    parentNode = node.parents[parent];
                    for(var child in parentNode.children){
                        siblingNode = parentNode.children[child];
                        if(siblingNode.id != node.id){
                            removeChildren(siblingNode);
                            tree.graph.removeNode(siblingNode.id);
                        }
                    }
                }
            }

            function removeChildren(node){
                var childNode;
                for(var child in node.children){
                    childNode = node.children[child];
                    if(childNode.children.length){
                        removeChildren(childNode);
                    }
                    tree.graph.removeNode(childNode.id);
                }
            }

            removeSiblingsAndSubTrees(node);
        }

    };

    return tree;
})
;
