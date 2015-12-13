define(["jquery"], function () {
    // http://stackoverflow.com/questions/1802936/stop-all-active-ajax-requests-in-jquery

    var xhrPool = [];
    $(document).ajaxSend(function (e, jqXHR, options) {
        xhrPool.push(jqXHR);
    });

    $(document).ajaxComplete(function (e, jqXHR, options) {
        xhrPool = $.grep(xhrPool, function (x) {
            return x != jqXHR
        });
    });

    var abort = function () {
        $.each(xhrPool, function (idx, jqXHR) {
            jqXHR.abort();
        });
    };

    var oldbeforeunload = window.onbeforeunload;
    window.onbeforeunload = function () {
        var r = oldbeforeunload ? oldbeforeunload() : undefined;
        if (r == undefined) {
            // only cancel requests if there is no prompt to stay on the page
            // if there is a prompt, it will likely give the requests enough time to finish
            abort();
        }
        return r;
    }
});
