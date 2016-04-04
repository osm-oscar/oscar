define(["jquery", "search"], function ($, search) {
    return menu = {
        /**
         * Displays the main-categories
         */
        displayCategories: function () {
            var c = $("#categories");
            if (c.is(":empty")) {
                var container = $("<div class='list-group'></div>");
                for (var category in this.categories) {
                    $("<a style='border: none' class='list-group-item'><i class='fa fa-" + menu.categories[category].img
                        + "' category='" + category + "'></i>&nbsp; " + menu.categories[category].desc + "</a>").on("click", menu.displaySubCategories).appendTo(container);
                }
                $("<div class='ui-widget'><label for='tagsSearch' data-toggle='tooltip' data-placement='right' title='Search for tags not covered by the menu above'>Tagsearch: </label><input id='tagsSearch' placeholder='Was möchten Sie suchen?'></div>").appendTo(container);
                c.append(container);

                $("#tagsSearch").autocomplete({
                    source: function (request, response) {
                        var service = "http://taginfo.openstreetmap.org/api/4/tags/popular?sortname=count_all&sortorder=desc&page=1&rp=8&query=" + request['term'];
                        var result = [];

                        $.getJSON(service, function (data) {
                            for (var suggestion in data.data) {
                                result.push("@" + data.data[suggestion].key + ":" + data.data[suggestion].value);
                            }
                            response(result);
                        });
                    },
                    select: function(event, ui){
                        $("#search_text").tokenfield('createToken', {value:  ui.item.value, label:  ui.item.value});
                    }
                });
                $('[data-toggle="tooltip"]').tooltip();
            }
        },

        /**
         * Shows all sub-categories for a main-category
         *
         * @param e a main-category
         */
        displaySubCategories: function (e) {
            var category = $(e.target.children[0]).attr("category");
            var c = $("#subCategories");
            c.empty();
            var container = $("<div class='list-group'></div>");
            for (var subcategory in menu.categories[category]["subcategories"]) {
                // exists a special key?
                var key = menu.categories[category]["subcategories"][subcategory].key ? menu.categories[category]["subcategories"][subcategory].key : menu.categories[category]["key"];
                $("<a style='border: none' class='list-group-item' key='" + key +"' value='" + menu.categories[category]["subcategories"][subcategory].value + "'>"
                    + menu.categories[category]["subcategories"][subcategory].desc + "</a>").on("click", function(e){menu.appendToSearchString(e); search.doCompletion();}).appendTo(container);
            }

            c.append(container);
        },

        /**
         * Appends a string to the current search-string
         *
         * @param e a string that should be appended
         */
        appendToSearchString: function (e) {
            var el = $(e.target);
            $("#search_text").tokenfield('createToken', {value:  "@" + el.attr("key") + ":" + el.attr("value"), label:  el.attr("value")});
        },

        /**
         * all main-categories and their sub-categories
         */
        categories: {
            food: {
                key: "cuisine",
                desc: "Cuisine",
                img: "cutlery",
                subcategories: {
                    pizza: {value: "pizza", desc: "Pizza"},
                    burger: {value: "burger", desc: "Burger"},
                    kebap: {value: "kebap", desc: "Kebap"},
                    sushi: {value: "sushi", desc: "Sushi"},
                    italian: {value: "italian", desc: "Italian"},
                    chinese: {value: "chinese", desc: "Chinese"},
                    japanese: {value: "japanese", desc: "Japanese"},
                    german: {value: "german", desc: "German"},
                    mexican: {value: "mexican", desc: "Mexican"},
                    greek: {value: "greek", desc: "Greek"}
                }
            },
            amenity: {
                key: "amenity",
                desc: "Amenity",
                img: "home",
                subcategories: {
                    restaurant: {value: "restaurant", desc: "Restaurant"},
                    hotel: {key: "tourism", value: "hotel", desc: "Hotel"},
                    cafe: {value: "cafe", desc: "Café"},
                    bar: {value: "bar", desc: "Bar"},
                    pharmacy: {value: "pharmacy", desc: "Pharmacy"},
                    hospital: {value: "hospital", desc: "Hospital"},
                    police: {value: "police", desc: "Police"},
                    wc: {value: "toilets", desc: "Toilets"},
                    atm: {value: "atm", desc: "ATM"},
                    fuel: {value: "fuel", desc: "Fuel"},
                    parking: {value: "parking", desc: "Parking"},
                    bank: {value: "bank", desc: "Bank"}
                }
            },
            shopping: {
                key: "shop",
                desc: "Shopping",
                img: "shopping-cart",
                subcategories: {
                    eat: {value: "supermarket", desc: "Supermarket"},
                    bakery: {value: "bakery", desc: "Bakery"},
                    mall: {value: "mall", desc: "Mall"},
                    clothes: {value: "clothes", desc: "Clothes"},
                    shoes: {value: "shoes", desc: "Shoes"},
                    kiosk: {value: "kiosk", desc: "Kiosk"},
                    butcher: {value: "butcher", desc: "Butcher"}
                }
            },
            transport: {
                key: "public_transport",
                desc: "Transport",
                img: "car",
                subcategories: {
                    airport: {key: "aeroway", value: "taxiway", desc: "Airport"},
                    taxi: {key: "amenity", value: "taxi", desc: "Taxi"},
                    ferry: {key: "amenity", value: "ferry_terminal", desc: "Ferry"},
                    oepnv: {value: "platform", desc: "ÖPNV"}
                }
            },
            leisure: {
                key: "leisure",
                desc: "Leisure",
                img: "futbol-o",
                subcategories: {
                    dance: {value: "dance", desc: "Dance"},
                    playground: {value: "playground", desc: "Playground"},
                    park: {value: "park", desc: "Park"},
                    pitch: {value: "pitch", desc: "Pitch"},
                    water: {value: "swimming_pool", desc: "Water"}
                }
            }
        }
    }
});