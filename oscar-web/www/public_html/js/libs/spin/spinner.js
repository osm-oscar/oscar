define(["jquery", "state", "conf", "spin"], function ($, state, config, spinner) {
    return spin = {
        spinner: new spinner(config.spinnerOpts),

        /**
         * displays the spinner
         */
        displayLoadingSpinner: function () {
            if (state.timers.loadingSpinner !== undefined) {
                clearTimeout(state.timers.loadingSpinner);
                state.timers.loadingSpinner = undefined;
            }
            if (state.loadingtasks > 0) {
                var target = document.getElementById('loader'); // TODO: use jquery
                spin.spinner.spin(target);
            }
        },

        /**
         * start the loading spinner, which displays after a timeout the spinner
         */
        startLoadingSpinner: function () {
            if (state.timers.loadingSpinner === undefined) {
                state.timers.loadingSpinner = setTimeout(spin.displayLoadingSpinner, config.timeouts.loadingSpinner);
            }
            state.loadingtasks += 1;
        },

        /**
         * ends a spinner instance
         */
        endLoadingSpinner: function () {
            if (state.loadingtasks === 1) {
                state.loadingtasks = 0;
                if (state.timers.loadingSpinner !== undefined) {
                    clearTimeout(state.timers.loadingSpinner);
                    state.timers.loadingSpinner = undefined;
                }
                spin.spinner.stop();
            }
            else {
                state.loadingtasks -= 1;
            }
        }
    };
});