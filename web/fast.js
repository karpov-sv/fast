Fast = function(parent_id, base="/fast", title="FAST"){
    this.base = base;
    this.title = title;

    this.last_status = {};

    var panel = $("<div/>", {class:"fast panel panel-default"});

    var header = $("<div/>", {class:"panel-heading"}).appendTo(panel);
    var title = $("<h3/>", {class:"panel-title"}).html(title + " ").appendTo(header);
    this.throbber = $("<span/>", {class:"glyphicon glyphicon-refresh pull-right"}).appendTo(title);
    this.connstatus = $("<span/>", {class:"label label-default"}).appendTo(title);

    this.body = $("<div/>", {class:"panel-body"}).appendTo(panel);

    // this.misc = $("<div/>", {class:""}).appendTo($("<div/>").appendTo(this.body));

    // this.fast_image = $("<img/>", {src:this.base + "/image.jpg", class:"image img-responsive center-block", style:"width:100%;max-width:512px;"}).appendTo(this.misc);
    // this.fast_image.on('click', $.proxy(function(){
    //     if(this.fast_image.css("max-width") == "512px"){
    //         this.fast_image.css("width", "initial");
    //         this.fast_image.css("max-width", "100%");
    //     } else {
    //         this.fast_image.css("width", "100%");
    //         this.fast_image.css("max-width", "512px");
    //     }
    // }, this));
    // new Updater(this.fast_image, 1000);

    var list = $("<ul/>", {class:"list-group"}).appendTo(this.body);

    this.state = $("<li/>", {class:"list-group-item"}).appendTo(list);

    //var tmp = $("<div/>", {class: "list-group-item"}).appendTo(list);
    this.misc = $("<div/>", {class:"list-group-item"}).appendTo($("<div/>").appendTo(list));

    this.fast_image = $("<img/>", {src:this.base + "/image.jpg", class:"image img-responsive center-block", style:"width:100%;max-width:512px;image-rendering:auto;"}).appendTo(this.misc);
    this.fast_image.on('click', $.proxy(function(){
        if(this.fast_image.css("max-width") == "512px"){
            this.fast_image.css("width", "initial");
            this.fast_image.css("max-width", "100%");
        } else {
            this.fast_image.css("width", "100%");
            this.fast_image.css("max-width", "512px");
        }
    }, this));
    new Updater(this.fast_image, 1000);



    this.status = $("<li/>", {class:"list-group-item"}).appendTo(list);

    /* Controls */
    var tmp = $("<div/>", {class: "list-group-item"}).appendTo(list);
    this.controls = $("<ul/>", {class:"list-inline"}).appendTo(tmp);

    if(this.title != "Guider"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Acquisition:").appendTo(tmp);
        this.acquisition_start = this.makeButton("Start", "start").appendTo(tmp);
        this.acquisition_stop = this.makeButton("Stop", "stop").appendTo(tmp);
    }

    if(this.title != "Old Guider"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Storage:").appendTo(tmp);
        this.storage_start = this.makeButton("Start", "storage_start").appendTo(tmp);
        this.storage_stop = this.makeButton("Stop", "storage_stop").appendTo(tmp);
    }

    if(this.title != "Old Guider"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Set Object",
                        function() {
                            bootbox.prompt({
                                title: "Object Name",
                                value: this.last_status['object'],
                                callback: $.proxy(function(result){
                                    if(result){
                                        this.sendCommand("set_object " + result);
                                    }
                                }, this)
                            });
                        }, this).appendTo(tmp);

        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Scale:").appendTo(tmp);
        this.makeButton("Full", "jpeg_params 0 1").appendTo(tmp);
        this.makeButton("99%", "jpeg_params 0.01 0.99").appendTo(tmp);
        this.makeButton("95%", "jpeg_params 0.05 0.95").appendTo(tmp);
        this.makeButton("90%", "jpeg_params 0.1 0.9").appendTo(tmp);

        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Total Img", function() {popupImage(this.base+'/total_image.jpg', 'Total image', 1)}).appendTo(tmp);
        this.makeButton("Total Flux", function() {popupImage(this.base+'/total_flux.jpg', 'Light Curve - Total', 1)}).appendTo(tmp);
        this.makeButton("Current Flux", function() {popupImage(this.base+'/current_flux.jpg', 'Light Curve - Current', 1)}).appendTo(tmp);
    }


    tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
    this.makeButton("Exposure:").appendTo(tmp);
    if(this.title == "Guider"){
        this.makeButton("0.01", "set_grabber exposure=0.01").appendTo(tmp);
        this.makeButton("0.128", "set_grabber exposure=0.128").appendTo(tmp);
        this.makeButton("0.5", "set_grabber exposure=0.5").appendTo(tmp);
        this.makeButton("1", "set_grabber exposure=1").appendTo(tmp);
        this.makeButton("5", "set_grabber exposure=5").appendTo(tmp);
        this.makeButton("10", "set_grabber exposure=10").appendTo(tmp);
    } else if(this.title == "Old Guider"){
        this.makeButton("0.5", "set_grabber exposure=0.5").appendTo(tmp);
        this.makeButton("1", "set_grabber exposure=1").appendTo(tmp);
        this.makeButton("5", "set_grabber exposure=5").appendTo(tmp);
        this.makeButton("10", "set_grabber exposure=10").appendTo(tmp);
    } else {
        this.makeButton("0.01", "set_grabber exposure=0.01").appendTo(tmp);
        this.makeButton("0.1", "set_grabber exposure=0.1").appendTo(tmp);
        this.makeButton("0.2", "set_grabber exposure=0.2").appendTo(tmp);
        this.makeButton("0.5", "set_grabber exposure=0.5").appendTo(tmp);
        this.makeButton("1", "set_grabber exposure=1").appendTo(tmp);
        this.makeButton("5", "set_grabber exposure=5").appendTo(tmp);
        this.makeButton("10", "set_grabber exposure=10").appendTo(tmp);
    }

    if(this.title == "Guider"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Amplification:").appendTo(tmp);
        this.makeButton("Min", "set_grabber amplification=0").appendTo(tmp);
        this.makeButton("Medium", "set_grabber amplification=511").appendTo(tmp);
        this.makeButton("Max", "set_grabber amplification=1023").appendTo(tmp);
    }

    if(this.title == "FAST"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("MGain:").appendTo(tmp);
        this.makeButton("0", "set_grabber mgain=0").appendTo(tmp);
        this.makeButton("1000", "set_grabber mgain=1000").appendTo(tmp);
        this.makeButton("2000", "set_grabber mgain=2000").appendTo(tmp)
        this.makeButton("3000", "set_grabber mgain=3000").appendTo(tmp);
        this.makeButton("Max", "set_grabber mgain=4096").appendTo(tmp);

        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Binning:").appendTo(tmp);
        this.makeButton("1x1", "set_grabber binning=1").appendTo(tmp);
        this.makeButton("2x2", "set_grabber binning=2").appendTo(tmp);
        this.makeButton("3x3", "set_grabber binning=3").appendTo(tmp);
        this.makeButton("4x4", "set_grabber binning=4").appendTo(tmp);

        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Countdown:").appendTo(tmp);
        this.makeButton("None", "set_countdown 0").appendTo(tmp);
        this.makeButton("100", "set_countdown 100").appendTo(tmp);
        this.makeButton("1000", "set_countdown 1000").appendTo(tmp);
        this.makeButton("10000", "set_countdown 10000").appendTo(tmp);
    }

    if(this.title == "FAST"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Params: ").appendTo(tmp);
        this.makeButton("Readout",
                        function() {
                            bootbox.prompt({
                                title: "Readout Port",
                                inputType: "select",
                                inputOptions: [{text:"Multiplication", value:0},
                                               {text:"Normal", value:1}],
                                value: this.last_status['pvcam_port'],
                                callback: $.proxy(function(result){
                                    if(result){
                                        this.sendCommand("set_grabber port=" + result);
                                    }
                                }, this)
                            });
                        }, this).appendTo(tmp);
        this.makeButton("Clear",
                        function() {
                            bootbox.prompt({
                                title: "Clear Mode",
                                inputType: "select",
                                inputOptions: [{text:"Never", value:0},
                                               {text:"Pre-Exposure", value:1},
                                               {text:"Pre-Sequence", value:2}],
                                value: this.last_status['pvcam_clear'],
                                callback: $.proxy(function(result){
                                    if(result){
                                        this.sendCommand("set_grabber clear=" + result);
                                    }
                                }, this)
                            });
                        }, this).appendTo(tmp);
    }

    tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
    this.makeButton("Exit",
                    function() {
                        bootbox.confirm({
                            message: "Are you sure you want to stop the daemon?",
                            callback: $.proxy(function(result){
                                if(result){
                                    this.sendCommand("exit");
                                }
                            }, this)
                        });
                    }, this).appendTo(tmp);

    this.cmdline = $("<input>", {type:"text", size:"40", class:"form-control"});

    $("<div/>", {class:"input-group"}).append($("<span/>", {class:"input-group-addon"}).html("Command:")).append(this.cmdline).appendTo(list);

    this.cmdline.pressEnter($.proxy(function(event){
        this.sendCommand(this.cmdline.val());
        event.preventDefault();
    }, this));

    // this.misc = $("<div/>", {class:""}).appendTo($("<div/>").appendTo(this.body));

    // this.fast_image = $("<img/>", {src:this.base + "/image.jpg", class:"image img-responsive center-block", style:"width:100%;max-width:512px;"}).appendTo(this.misc);
    // this.fast_image.on('click', $.proxy(function(){
    //     if(this.fast_image.css("max-width") == "512px"){
    //         this.fast_image.css("width", "initial");
    //         this.fast_image.css("max-width", "100%");
    //     } else {
    //         this.fast_image.css("width", "100%");
    //         this.fast_image.css("max-width", "512px");
    //     }
    // }, this));
    // new Updater(this.fast_image, 1000);

    var footer = $("<div/>", {class:"panel-footer"});//.appendTo(panel);

    var form = $("<form/>").appendTo(footer);
    // this.throbber = $("<span/>", {class:"glyphicon glyphicon-transfer"}).appendTo(form);
    this.delayValue = 2000;
    this.delay = $("<select/>", {id:getUUID()});
    $("<label>", {for: this.delay.get(0).id}).html("Refresh:").appendTo(form);
    $("<option>", {value: "1000"}).html("1").appendTo(this.delay);
    $("<option>", {value: "2000", selected:1}).html("2").appendTo(this.delay);
    $("<option>", {value: "5000"}).html("5").appendTo(this.delay);
    $("<option>", {value: "10000"}).html("10").appendTo(this.delay);
    this.delay.appendTo(form);
    this.delay.change($.proxy(function(event){
        this.delayValue = this.delay.val();
    }, this));

    panel.appendTo(parent_id);

    this.timer = 0;
    this.requestState();
}

Fast.prototype.requestState = function(){
    $.ajax({
        url: this.base + "/status",
        dataType : "json",
        timeout : 1000,
        context: this,

        success: function(json){
            this.throbber.animate({opacity: 1.0}, 200).animate({opacity: 0.1}, 400);

            /* Crude hack to prevent jumping */
            st = document.body.scrollTop;
            sl = document.body.scrollLeft;
            this.updateStatus(json.fast_connected, json.fast);
            document.body.scrollTop = st;
            document.body.scrollLeft = sl;
        },

        complete: function( xhr, status ) {
            clearTimeout(this.timer);
            this.timer = setTimeout($.proxy(this.requestState, this), this.delayValue);
        }
    });
}

Fast.prototype.updateStatus = function(connected, status){
    this.status.html("");

    this.last_status = status;

    if(connected){
        show(this.body);
        show(this.fast_image);

        if(status['simcam'] == '1'){
            this.connstatus.html("Connected to Simulator");
            this.connstatus.removeClass("label-success").addClass("label-danger");
        } else {
            this.connstatus.html("Connected");
            this.connstatus.removeClass("label-danger").addClass("label-success");
        }

        var state = "Acquisition: ";
        if(status['acquisition'] == '1'){
            state += label('Acquiring', 'success');
            //this.fast_image.css('opacity', 1.0);
            if(status['storage'] == '1')
                this.fast_image.css('border', '2px solid green');
            else
                this.fast_image.css('border', '2px solid gray');

            if(this.title != "Guider"){
                enable(this.acquisition_stop);
                disable(this.acquisition_start);
            }
        } else {
            state += label('Idle');
            //this.fast_image.css('opacity', 0.3);
            this.fast_image.css('border', '2px solid red');
            if(this.title != "Guider"){
                disable(this.acquisition_stop);
                enable(this.acquisition_start);
            }
        }

        if(status['vs56'] != '1'){
            state += " Storage: ";
            if(status['storage'] == '1'){
                state += label('On', 'success');
                enable(this.storage_stop);
                disable(this.storage_start);
            } else {
                state += label('Off');
                disable(this.storage_stop);
                enable(this.storage_start);
            }

            state += " Object: " + label(status['object']);

            if(status['countdown']*1.0 > 0)
                state += " Countdown: " + label(status['countdown'], "warning");

            state += " Mean: " + label(status['image_mean']);

            state += " Free: " + label((status['free_disk_space']/1024/1024/1024).toFixed(1) + " Gb");

            if(status['acquisition'] == '1'){
                state += " Last acquired: " + label((1.0*status['age_acquired']).toFixed(1), status['age_acquired'] < 1.0 ? "success" : "danger");
                if(status['storage'] == '1')
                    state += " Last stored: " + label((1.0*status['age_stored']).toFixed(1), status['age_stored'] < 1.0 ? "success" : "danger");
            }
        }

        this.state.html(state);

        state = "Exposure: " + label(status['exposure']);
        if(status['pvcam'] == '1'){
            state += " FPS: ";
            if(status['acquisition'] == '1')
                state += label((1.0*status['fps']).toPrecision(3));
            else
                state += label("-");
        }
        if(status['pvcam'] == '1'){
            state += " PMode: ";
            if(status['pvcam_pmode'] == '0')
                state += label('Normal');
            else
                state += label('Frame Transfer');

            state += " Clear: ";
            if(status['pvcam_clear'] == '0')
                state += label('Never');
            else if(status['pvcam_clear'] == '1')
                state += label('Pre-Exposure');
            else if(status['pvcam_clear'] == '2')
                state += label('Pre-Sequence');
            else
                state += label('Unknown');

            state += " Readout: ";
            if(status['pvcam_port'] == '0')
                state += label('Multiplication');
            else
                state += label('Normal');

            state += " Gain: " + label(status['pvcam_gain']) + " / " + label(status['pvcam_mgain']);

            state += " Binning: " + label(status['binning']+"x"+status['binning']);

            state += " Temperature: " + label(status['temperature']);

        } else if(status['vslib'] == '1'){
            state += " Amplification: " + label(status['amplification']);
        }

        this.status.html(state);
    } else {
        hide(this.body);
        hide(this.fast_image);

        this.connstatus.html("Disconnected");
        this.connstatus.removeClass("label-success").addClass("label-danger");
    }
}

Fast.prototype.sendCommand = function(command){
    $.ajax({
        url: this.base + "/command",
        data: {string: command}
    });
}

Fast.prototype.makeButton = function(text, command, title)
{
    if(typeof(command) == "string")
        return $("<button>", {class:"btn btn-default", title:title}).html(text).click($.proxy(function(event){
            this.sendCommand(command);
            event.preventDefault();
        }, this));
    else if(typeof(command) == "function")
        return $("<button>", {class:"btn btn-default", title:title}).html(text).click($.proxy(command, this));
    else
        return $("<button>", {class:"btn btn-default disabled", title:title}).html(text);
}
