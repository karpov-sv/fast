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

    var list = $("<ul/>", {class:"list-group"}).appendTo(this.body);

    this.state = $("<li/>", {class:"list-group-item"}).appendTo(list);

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
    this.controls_type = "";

    this.makeControls("");

    this.cmdline = $("<input>", {type:"text", size:"40", class:"form-control"});

    $("<div/>", {class:"input-group"}).append($("<span/>", {class:"input-group-addon"}).html("Command:")).append(this.cmdline).appendTo(list);

    this.cmdline.pressEnter($.proxy(function(event){
        this.sendCommand(this.cmdline.val());
        event.preventDefault();
    }, this));

    var footer = $("<div/>", {class:"panel-footer"});//.appendTo(panel);

    var form = $("<form/>").appendTo(footer);
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

Fast.prototype.makeControls = function(type){
    this.controls.html("");

    if(type != "vs2001"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Acquisition:").appendTo(tmp);
        this.acquisition_start = this.makeButton("Start", "start").appendTo(tmp);
        this.acquisition_stop = this.makeButton("Stop", "stop").appendTo(tmp);
    }

    if(type != "vs56"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Storage:").appendTo(tmp);
        this.storage_start = this.makeButton("Start", "storage_start").appendTo(tmp);
        this.storage_stop = this.makeButton("Stop", "storage_stop").appendTo(tmp);
    }

    if(type != "vs56"){
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
        if(type == "csdu" || type == "andor2" || type == "gxccd")
            this.makeButton("99.9%", "jpeg_params 0.01 0.999").appendTo(tmp);
        this.makeButton("99%", "jpeg_params 0.01 0.99").appendTo(tmp);
        this.makeButton("95%", "jpeg_params 0.05 0.95").appendTo(tmp);
        this.makeButton("90%", "jpeg_params 0.1 0.9").appendTo(tmp);
        this.makeButton(":").appendTo(tmp);
        this.makeButton("x1", "jpeg_params scale=1").appendTo(tmp);
        this.makeButton("x2", "jpeg_params scale=2").appendTo(tmp);
        this.makeButton("x4", "jpeg_params scale=4").appendTo(tmp);

        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Quality:").appendTo(tmp);
        this.makeButton("10%", "jpeg_params quality=10").appendTo(tmp);
        this.makeButton("50%", "jpeg_params quality=50").appendTo(tmp);
        this.makeButton("95%", "jpeg_params quality=95").appendTo(tmp);

        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Total Img", function() {popupImage(this.base+'/total_image.jpg', 'Total image', 1)}).appendTo(tmp);
        this.makeButton("Total Flux", function() {popupImage(this.base+'/total_flux.jpg', 'Light Curve - Total', 1)}).appendTo(tmp);
        this.makeButton("Current Flux", function() {popupImage(this.base+'/current_flux.jpg', 'Light Curve - Current', 1)}).appendTo(tmp);
    }

    tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
    this.makeButton("Exposure:").appendTo(tmp);
    if(type == "vs2001" || type == "csdu"){
        this.makeButton("0.01", "set_grabber exposure=0.01").appendTo(tmp);
        this.makeButton("0.128", "set_grabber exposure=0.128").appendTo(tmp);
        this.makeButton("0.5", "set_grabber exposure=0.5").appendTo(tmp);
        this.makeButton("1", "set_grabber exposure=1").appendTo(tmp);
        this.makeButton("5", "set_grabber exposure=5").appendTo(tmp);
        this.makeButton("10", "set_grabber exposure=10").appendTo(tmp);
    } else if(type == "vs56"){
        this.makeButton("0.5", "set_grabber exposure=0.5").appendTo(tmp);
        this.makeButton("1", "set_grabber exposure=1").appendTo(tmp);
        this.makeButton("5", "set_grabber exposure=5").appendTo(tmp);
        this.makeButton("10", "set_grabber exposure=10").appendTo(tmp);
    } else if(type == "andor2"){
        this.makeButton("0", "set_grabber exposure=0").appendTo(tmp);
        this.makeButton("0.01", "set_grabber exposure=0.01").appendTo(tmp);
        this.makeButton("0.03", "set_grabber exposure=0.03").appendTo(tmp);
        this.makeButton("0.1", "set_grabber exposure=0.1").appendTo(tmp);
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

    if(type == "vs2001" || type == "csdu"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Amplification:").appendTo(tmp);
        this.makeButton("Min", "set_grabber amplification=0").appendTo(tmp);
        this.makeButton("Default", "set_grabber amplification=170").appendTo(tmp);
        this.makeButton("Medium", "set_grabber amplification=511").appendTo(tmp);
        this.makeButton("Max", "set_grabber amplification=1023").appendTo(tmp);
    }

    if(type == "andor2"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Amplification:").appendTo(tmp);
        this.makeButton("Min", "set_grabber amplification=0").appendTo(tmp);
        this.makeButton("Medium", "set_grabber amplification=150").appendTo(tmp);
        this.makeButton("Max", "set_grabber amplification=300").appendTo(tmp);
    }

    if(type == "andor"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Cooling:").appendTo(tmp);
        this.makeButton("Off", "set_grabber cooling=0").appendTo(tmp);
        this.makeButton("-10", "set_grabber cooling=1 temperature=-10").appendTo(tmp);
        this.makeButton("-20", "set_grabber cooling=1 temperature=-20").appendTo(tmp);
        this.makeButton("-30", "set_grabber cooling=1 temperature=-30").appendTo(tmp);
        this.makeButton("-40", "set_grabber cooling=1 temperature=-40").appendTo(tmp);

        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Preamp:").appendTo(tmp);
        this.makeButton("12 bit", "set_grabber preamp=0").appendTo(tmp);
        this.makeButton("16 bit", "set_grabber preamp=1").appendTo(tmp);
   }

    if(type == "gxccd"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Cooling:").appendTo(tmp);
        this.makeButton("Off", "set_grabber temperature=50").appendTo(tmp);
        this.makeButton("0", "set_grabber temperature=0").appendTo(tmp);
        this.makeButton("-10", "set_grabber temperature=-10").appendTo(tmp);
        this.makeButton("-20", "set_grabber temperature=-20").appendTo(tmp);
        this.makeButton("-30", "set_grabber temperature=-30").appendTo(tmp);

        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Shutter:").appendTo(tmp);
        this.makeButton("Dark", "set_grabber shutter=0").appendTo(tmp);
        this.makeButton("Light", "set_grabber shutter=1").appendTo(tmp);

        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Preflash:").appendTo(tmp);
        this.makeButton("Off", "set_grabber preflash=0").appendTo(tmp);
        this.makeButton("2s", "set_grabber preflash=2").appendTo(tmp);
   }

    if(type == "pvcam"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("MGain:").appendTo(tmp);
        this.makeButton("0", "set_grabber mgain=0").appendTo(tmp);
        this.makeButton("1000", "set_grabber mgain=1000").appendTo(tmp);
        this.makeButton("2000", "set_grabber mgain=2000").appendTo(tmp)
        this.makeButton("3000", "set_grabber mgain=3000").appendTo(tmp);
        this.makeButton("Max", "set_grabber mgain=4096").appendTo(tmp);
    }
    if(type == "pvcam" || type == "csdu" || type == "andor2"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Binning:").appendTo(tmp);
        this.makeButton("1x1", "set_grabber binning=1").appendTo(tmp);
        this.makeButton("2x2", "set_grabber binning=2").appendTo(tmp);
        this.makeButton("4x4", "set_grabber binning=4").appendTo(tmp);
    }

    if(type == "andor"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Binning:").appendTo(tmp);
        this.makeButton("1x1", "set_grabber binning=0").appendTo(tmp);
        this.makeButton("2x2", "set_grabber binning=1").appendTo(tmp);
        this.makeButton("3x3", "set_grabber binning=2").appendTo(tmp);
        this.makeButton("4x4", "set_grabber binning=3").appendTo(tmp);
        this.makeButton("8x8", "set_grabber binning=4").appendTo(tmp);
    }

    if(type == "andor2"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Window:").appendTo(tmp);
        this.makeButton("Full", "set_grabber x1=1 y1=1 x2=1024 y2=1024").appendTo(tmp);
        this.makeButton("1/2", "set_grabber x1=257 y1=257 x2=768 y2=768").appendTo(tmp);
        this.makeButton("1/4", "set_grabber x1=385 y1=385 x2=640 y2=640").appendTo(tmp);
    }

    if(type == "pvcam" || type == "andor" || type == "andor2" || type == "qhy" || type == "gxccd"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Countdown:").appendTo(tmp);
        this.makeButton("None", "set_countdown 0").appendTo(tmp);
        this.makeButton("100", "set_countdown 100").appendTo(tmp);
        this.makeButton("1000", "set_countdown 1000").appendTo(tmp);
        this.makeButton("10000", "set_countdown 10000").appendTo(tmp);
    }

    if(type == "pvcam"){
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

    if(type == "pvcam" || type == "andor2"){
        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Cooling:").appendTo(tmp);
        this.makeButton("Off", "set_grabber cooling=0").appendTo(tmp);
        this.makeButton("On", "set_grabber cooling=1").appendTo(tmp);

        tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
        this.makeButton("Shutter:").appendTo(tmp);
        this.makeButton("Auto", "set_grabber shutter=0").appendTo(tmp);
        this.makeButton("Open", "set_grabber shutter=1").appendTo(tmp);
        this.makeButton("Close", "set_grabber shutter=2").appendTo(tmp);
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



    this.controls_type = type;
}

Fast.prototype.requestState = function(){
    $.ajax({
        url: this.base + "/status",
        dataType : "json",
        timeout : 1000,
        context: this,

        success: function(json){
            this.throbber.stop().animate({opacity: 1.0}, 200).animate({opacity: 0.1}, 400);

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

        types = ["pvcam", "csdu", "vs2001", "vs56", "andor", "andor2", "qhy", "gxccd"];
        for(var i = 0; i < types.length; i++)
            if(status[types[i]] == '1' && this.controls_type != types[i])
                this.makeControls(types[i]);

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

            state += " Mean: " + label(status['mean']);

            state += " Free: " + label((status['free_disk_space']/1024/1024/1024).toFixed(1) + " Gb");

            if(status['acquisition'] == '1'){
                state += " Frames: " + label(status['num_acquired']) + " / " + label(status['num_stored'])

                state += " Last acquired: " + label((1.0*status['age_acquired']).toFixed(1), status['age_acquired'] < 1.0 ? "success" : "danger");
                if(status['storage'] == '1')
                    state += " Last stored: " + label((1.0*status['age_stored']).toFixed(1), status['age_stored'] < 1.0 ? "success" : "danger");
            }
        }

        this.state.html(state);

        state = "Exposure: " + label(status['exposure']);
        if(status['pvcam'] == '1' || status['csdu'] == '1' || status['andor2'] == '1' || status['qhy'] == '1' || status['gxccd'] == '1'){
            state += " FPS: ";
            if(status['acquisition'] == '1')
                state += label((1.0*status['fps']).toPrecision(3), 'primary', 'Read-out: ' + status['readout'] + ' s');
            else
                state += label("-");
        }
        if(status['andor'] == '1'){
            state += " FPS: " + label(status['fps']);
        }
        if(status['csdu'] == '1'){
            state += " Binning: " + label(status['binning']+"x"+status['binning']);
        } else if(status['pvcam'] == '1'){
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

        } else if(status['andor'] == '1'){
            temperaturestatus = {0:'Off', 1:'Cooling', 2:'Stabilized', 3:'Drift', 4:'Not Stabilized', 5:'Fault', 6:'Overheating'};
            shutterstatus = {0:'Rolling', 1:'Global'};
            binning = {0:'1x1', 1:'2x2', 2:'3x3', 3:'4x4', 4:'8x8'}
            preamp = {0:'12 bit', 1:'16 bit'}

            state += " Cooling: " + label(status['cooling'] == '0' ? 'Off' : 'On');
            state += " Temperature: " + label(status['temperature']) + ' ' + label(temperaturestatus[status['temperaturestatus']]);
            state += " Shutter: " + label(shutterstatus[status['shutter']]);
            state += " Preamp: " + label(preamp[status['preamp']]);
            state += " Binning: " + label(binning[status['binning']]);
            if(status['x1'])
                state += ' ' + label(status['x1'] + ' ' + status['y1'] + ' ' + status['x2'] + ' ' + status['y2']);
        } else if(status['andor2'] == '1'){
            temperaturestatus = {20034:'Off', 20035:'Not Stabilized', 20036:'Stabilized', 20037:'Cooling'};
            shutterstatus = {0:'Auto', 1:'Open', 2:'Closed'};

            state += " Cooling: " + label(status['cooling'] == '0' ? 'Off' : 'On');
            state += " Temperature: " + label(status['temperature']) + ' ' + label(temperaturestatus[status['temperaturestatus']]);
            state += " Shutter: " + label(shutterstatus[status['shutter']]);
            state += " Binning: " + label(status['binning']+"x"+status['binning']) + ' ' +
                label(status['x1'] + ' ' + status['y1'] + ' ' + status['x2'] + ' ' + status['y2']);
            state += " VS: " + label(status['vsspeed']);
            state += " HS: " + label(status['hsspeed']) + ' ' + label(status['speed']) + ' MHz';
        } else if(status['qhy'] == '1'){
            state += " Binning: " + label(status['binning']+"x"+status['binning']);
            state += " Gain: " + label(status['gain']);
            state += " Offset: " + label(status['offset']);
            state += " Temperature: " + label(status['temperature']) + ' @ ' + label(status['temppower']/255);
        } else if(status['gxccd'] == '1'){
            state += " Read mode: " + label(status['readmode']);
            state += " Gain: " + label(status['gain']);
            state += " Shutter: " + label(status['shutter'] == '0' ? 'Dark' : 'Light', status['shutter'] == '0' ? 'primary' : 'success');
            state += " Filter: " + label(status['filter']);
            state += " Preflash: " + label((1.0*status['preflash']).toFixed(1), status['preflash'] == '0' ? 'primary' : 'warning');
            // state += " Binning: " + label(status['binning']+"x"+status['binning']);
            state += " Window: " + label(status['x0']+" "+status['y0'] + " " + status['width']+"x"+status['height']);
            state += " Temperature: " + label((1.0*status['temperature']).toFixed(1)) + ' / ' + label((1.0*status['target_temperature']).toFixed(1)) + ' @ ' + label((100.0*status['temppower']).toFixed(0) + '%', status['temppower'] == '0' ? 'primary' : 'success');
        }

        if(status['vslib'] == '1' || status['csdu'] == '1' || status['andor2'] == '1'){
            state += " Amplification: " + label(status['amplification']);
        }

        // state += " Postprocess: " + label(status['postprocess'] == '0' ? 'Off' : 'On');
        if(status['dark'] == '1')
            state += " " + label("Dark", "success");
        else
            state += " " + label("No Dark", "warning");

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
