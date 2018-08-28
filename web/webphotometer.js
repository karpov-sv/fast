$(document).ready(function(){
    path = window.location.pathname;

    if(path == '/photometer.html'){
        photometer = new Photometer("#contents");
        document.title = "Photometer";
    } else if(path == '/fast.html') {
        fast = new Fast("#contents", base="/fast");
        document.title = "FAST";
    } else if(path == '/guider.html'){
        guider = new Fast("#contents", base="/guider", title="Guider");
        document.title = "Guider";
    } else if(path == '/oldguider.html'){
        guider = new Fast("#contents", base="/oldguider", title="Old Guider");
        document.title = "Old Guider";
    } else if(path == '/mppp.html'){
        guider = new Fast("#contents-left", base="/oldguider", title="Old Guider");
        fast = new Fast("#contents-right", base="/fast");
        document.title = "MPPP";
    } else {
        photometer = new Photometer("#contents");
        fast = new Fast("#contents2", base="/fast");
        guider = new Fast("#contents3", base="/guider", title="Guider");
    }

    //$('.selectpicker').selectpicker();
});
