

window.addEventListener("resize", function(event)
{
    var canvas = document.getElementById("canvas");
    var displayWidth  = canvas.clientWidth;
    var displayHeight = canvas.clientHeight;
    if (canvas.width  != displayWidth || canvas.height != displayHeight)
    {
        canvas.width  = displayWidth;
        canvas.height = displayHeight;
    }
});


