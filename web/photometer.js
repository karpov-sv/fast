Photometer = function(parent_id){
    var panel = $("<div/>", {class:"photometer panel panel-default"});

    var header = $("<div/>", {class:"panel-heading"}).appendTo(panel);
    var title = $("<h3/>", {class:"panel-title"}).html("Photometer ").appendTo(header);
    this.throbber = $("<span/>", {class:"glyphicon glyphicon-refresh pull-right"}).appendTo(title);
    this.connstatus = $("<span/>", {class:"label label-default"}).appendTo(title);
    this.hw_connstatus = $("<span/>", {class:"label label-default"}).appendTo(title);

    this.body = $("<div/>", {class:"panel-body"}).appendTo(panel);

    var list = $("<ul/>", {class:"list-group"}).appendTo(this.body);

    this.status = $("<li/>", {class:"list-group-item"}).appendTo(list);

    /* Controls */
    var tmp = $("<div/>", {class: "list-group-item"}).appendTo(list);
    this.controls = $("<ul/>", {class:"list-inline"}).appendTo(tmp);

    tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
    this.makeButton("Shutter:").appendTo(tmp);
    this.makeButton("Open", "send_command Co").appendTo(tmp);
    this.makeButton("Close", "send_command Cc").appendTo(tmp);

    tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
    this.makeButton("Mirror:").appendTo(tmp);
    this.makeButton("Install", "send_command Mi").appendTo(tmp);
    this.makeButton("Remove", "send_command M#").appendTo(tmp);

    tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
    this.makeButton("Polarizer:").appendTo(tmp);
    this.makeButton("Install", "send_command /").appendTo(tmp);
    this.makeButton("Remove", "send_command |").appendTo(tmp);

    tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
    this.makeButton("Filters:").appendTo(tmp);
    this.makeButton("<", "send_command f<").appendTo(tmp);
    this.makeButton("#", "send_command f#").appendTo(tmp);
    this.makeButton(">", "send_command f>").appendTo(tmp);

    tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
    this.makeButton("FF:").appendTo(tmp);
    this.makeButton("None", "send_command L#").appendTo(tmp);
    this.makeButton("U", "send_command LU").appendTo(tmp);
    this.makeButton("B", "send_command LB").appendTo(tmp);
    this.makeButton("V", "send_command LV").appendTo(tmp);
    this.makeButton("R", "send_command LR").appendTo(tmp);

    tmp = $("<li/>", {class:"btn-group"}).appendTo(this.controls);
    this.makeButton("Blocks:").appendTo(tmp);
    this.makeButton("<", "send_command b<").appendTo(tmp);
    this.makeButton("#", "send_command b#").appendTo(tmp);
    this.makeButton(">", "send_command b>").appendTo(tmp);

    this.cmdline = $("<input>", {type:"text", size:"40", class:"form-control"});

    $("<div/>", {class:"input-group"}).append($("<span/>", {class:"input-group-addon"}).html("Command:")).append(this.cmdline).appendTo(list);

    this.cmdline.pressEnter($.proxy(function(event){
        this.sendCommand(this.cmdline.val());
        event.preventDefault();
    }, this));

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

    this.requestState();
}

Photometer.prototype.requestState = function(){
    $.ajax({
        url: "/photometer/status",
        dataType : "json",
        timeout : 1000,
        context: this,

        success: function(json){
            this.throbber.animate({opacity: 1.0}, 200).animate({opacity: 0.1}, 400);

            /* Crude hack to prevent jumping */
            st = document.body.scrollTop;
            sl = document.body.scrollLeft;
            this.updateStatus(json.photometer_connected, json.photometer);
            document.body.scrollTop = st;
            document.body.scrollLeft = sl;
        },

        complete: function( xhr, status ) {
            setTimeout($.proxy(this.requestState, this), this.delayValue);
        }
    });
}

Photometer.prototype.updateStatus = function(connected, status){
    this.status.html("");

    if(connected){
        show(this.body);
        show(this.hw_connstatus);

        this.connstatus.html("Connected");
        this.connstatus.removeClass("label-danger").addClass("label-success");

        if(status['connected'] == 1){
            show(this.status);
            show(this.controls);

            this.hw_connstatus.html("HW Connected");
            this.hw_connstatus.removeClass("label-danger").addClass("label-success");

            var state = "Mirror: " + label(status['mir'],  status['mir'] == 'EMCCD' ? 'success' : 'warning');

            state += " Polarizer: " + label(status['pol']);
            state += " Filters: " + label(status['flt']);
            state += " Block: " + label(status['blk']);
            state += " Shutter: " + label(status['cap'] + ((status['cap'] == 'sky') ? ' / open' : ' / closed'), status['cap'] == 'sky' ? 'success' : 'danger');

            var lgt = status['lgt'];
            if(lgt)
                lgt = lgt.replace(new RegExp('_','g'), '');

            state += " FF: " + label(lgt ? lgt : "None", lgt ? "danger" : "success");

            this.status.html(state);
        } else {
            hide(this.status);
            hide(this.controls);

            this.hw_connstatus.html("HW Disconnected");
            this.hw_connstatus.addClass("label-danger").removeClass("label-success");

            this.status.html("");
        }


    } else {
        hide(this.body);
        this.connstatus.html("Disconnected");
        this.connstatus.removeClass("label-success").addClass("label-danger");

        hide(this.hw_connstatus);
    }
}

Photometer.prototype.sendCommand = function(command){
    $.ajax({
        url: "/photometer/command",
        data: {string: command}
    });
}

Photometer.prototype.makeButton = function(text, command, title)
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
