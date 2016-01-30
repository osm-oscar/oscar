define(["dagre-d3", "d3", "jquery"], function (dagreD3, d3, $) {
    var tree = {
        graph: undefined, // the graph
        state: undefined, // contains tree-datastructure
        renderer : new dagreD3.render(),
        visualizeDAG: function (root, state) {
            state.visualizationActive = true;
            this.state = state;

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
            this.graph.nodes().forEach(function (v) {
                var node = tree.graph.node(v);
                // Round the corners of the nodes
                node.rx = node.ry = 5;
            });

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

            // draw graph
            svgGroup.call(this.renderer, this.graph);

            // make the region, which are also drawn on the map, interactive
            d3.selectAll(".type-LOADABLE").on("click", this._nodeOnClick);

            // Center the graph
            var xCenterOffset = ($("#tree").width() - this.graph.graph().width) / 2;
            svgGroup.attr("transform", "translate(" + xCenterOffset + ", 20)");
            svg.attr("height", this.graph.graph().height + 40);
        },

        _recursiveAddToGraph: function (node, graph) {
            if (node.name) {
                var attr = {label: node.name.toString()};
                if (this.state.items.clusters.drawn.count(node.id)) {
                    attr = {label: node.name.toString(), class: "type-LOADABLE", labelStyle: "color: white"};
                }
                this.graph.setNode(node.id, attr);
                for (var child in node.children) {
                    if (node.children[child].name) {
                        attr = {label: node.children[child].name.toString()};
                        if (this.state.items.clusters.drawn.count(node.children[child].id)) {
                            attr = {
                                label: node.children[child].name.toString(),
                                class: "type-LOADABLE",
                                labelStyle: "color: white"
                            };
                        }
                        this.graph.setNode(node.children[child].id, attr);
                        this.graph.setEdge(node.id, node.children[child].id, {lineInterpolate: 'basis'});
                        this._recursiveAddToGraph(node.children[child], graph)
                    }
                }
            }
        },

        _nodeOnClick: function (id) {
            var marker = tree.state.DAG.at(id).marker;
            if (marker.shape) {
                tree.state.map.removeLayer(marker.shape);
            }
            tree.state.markers.removeLayer(marker);
            tree.state.items.clusters.drawn.erase(id);
            tree.state.regionHandler({rid: id, draw: true});
        },

        refresh: function(id){
            var parents = this.graph.inEdges(id);
            this.graph.removeNode(id);
            d3.select("svg").select("g").call(this.renderer, this.graph);
            this.graph.setNode(id, {label: tree.state.DAG.at(id).name.toString(), labelStyle: "color: white"});
            for(var i in parents){
                this.graph.setEdge(parents[i].v, id, {lineInterpolate: 'basis'});
            }

            this._recursiveAddToGraph(tree.state.DAG.at(id), this.graph);
            this.graph.nodes().forEach(function (v) {
                var node = tree.graph.node(v);
                // Round the corners of the nodes
                node.rx = node.ry = 5;
            });
            d3.select("svg").select("g").call(this.renderer, this.graph);
        }

    };

    return tree;
});