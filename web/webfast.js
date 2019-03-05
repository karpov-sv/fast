$(document).ready(function(){
    path = window.location.pathname;

    if(path == '/fast.html') {
        fast = new Fast("#contents", base="/fast");
        document.title = "FAST";
    }

    //$('.selectpicker').selectpicker();
});
