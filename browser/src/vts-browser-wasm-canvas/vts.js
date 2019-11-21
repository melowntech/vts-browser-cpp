
var canvas = document.getElementById("canvas")

function updateCanvasSize()
{
    var displayWidth  = canvas.clientWidth
    var displayHeight = canvas.clientHeight
    if (canvas.width  != displayWidth || canvas.height != displayHeight)
    {
        canvas.width  = displayWidth
        canvas.height = displayHeight
    }
}

updateCanvasSize()
window.addEventListener("resize", updateCanvasSize)

canvas.addEventListener("webglcontextlost", function()
{
    alert('WebGL context lost. You will need to reload the page.')
})
