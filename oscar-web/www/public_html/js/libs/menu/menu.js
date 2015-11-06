var menu = {
    displayCategories: function () {
        var c = $("#categories");
        if (c.is(":empty")) {
            var d = "<div class='list-group'>";
            for (category in menu.categories) {
                d += "<a style='border: none' class='list-group-item' onclick=\"menu.displaySubCategories('" + category + "')\"><i class='fa fa-" + menu.categories[category].img + "'></i>&nbsp; " + menu.categories[category].desc + "</a>";
            }
            d += "</div>";
            c.append(d);
        }
    },
    displaySubCategories: function (category) {
        var c = $("#subCategories");
        c.empty();
        var d = "<div class='list-group'>";
        for (subcategory in menu.categories[category]["subcategories"]) {
            // exists a special key?
            key = menu.categories[category]["subcategories"][subcategory].key ? menu.categories[category]["subcategories"][subcategory].key : menu.categories[category]["key"];
            d += "<a style='border: none' class='list-group-item' onclick=\"menu.appendToSearchString('@" + key + ":" + menu.categories[category]["subcategories"][subcategory].value + "')\">" + menu.categories[category]["subcategories"][subcategory].desc + "</a>";
        }
        d += "</div>";
        c.append(d);
    },
    appendToSearchString: function (string) {
        var c = $("#search_text");
        c.val(c.val() + string);
    },
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
                greek: {value: "greek", desc: "Greek"},
            }
        },
        amenity: {
            key: "amenity",
            desc: "Amenity",
            img: "home",
            subcategories: {
                restaurant: {value: "restaurant", desc: "Restaurant"},
                cafe: {value: "cafe", desc: "Café"},
                pharmacy: {value: "pharmacy", desc: "Pharmacy"},
                hospital: {value: "hospital", desc: "Hospital"},
                police: {value: "police", desc: "Police"},
                wc: {value: "toilets", desc: "Toilets"},
                atm: {value: "atm", desc: "ATM"},
                fuel: {value: "fuel", desc: "Fuel"},
                parking: {value: "parking", desc: "Parking"},
                bank: {value: "bank", desc: "Bank"},
            }
        },
        shopping: {
            key: "shop",
            desc: "Shopping",
            img: "shopping-cart",
            subcategories: {
                eat: {value: "supermarket", desc: "Supermarket"},
                clothes: {value: "clothes", desc: "Clothes"},
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
                oepnv: {value: "platform", desc: "ÖPNV"},
            }
        },
        leisure: {
            key: "leisure",
            desc: "Leisure",
            img: "futbol-o",
            subcategories: {
                park: {value: "park", desc: "Park"},
                pitch: {value: "pitch", desc: "Pitch"},
                water: {value: "swimming_pool", desc: "Water"},
            }
        }
    }
};