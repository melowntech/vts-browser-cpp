
var canvas = document.getElementById("canvas")

// canvas size
function updateCanvasSize()
{
    let displayWidth  = canvas.clientWidth
    let displayHeight = canvas.clientHeight
    if (canvas.width  != displayWidth || canvas.height != displayHeight)
    {
        canvas.width  = displayWidth
        canvas.height = displayHeight
    }
}
updateCanvasSize()
window.addEventListener("resize", updateCanvasSize)

// lost opengl context
canvas.addEventListener("webglcontextlost", function()
{
    alert('WebGL context lost. You will need to reload the page.')
})

// collapsible elements
{
    let coll = document.getElementsByClassName("collapsible")
    for (let i = 0; i < coll.length; i++)
    {
        let content = coll[i].nextElementSibling
        content.style.display = "none"
        coll[i].addEventListener("click", function()
        {
            if (content.style.display === "block")
                content.style.display = "none"
            else
                content.style.display = "block"
        });
    }
}

// draggable elements
var draggableZIndex = 1
function draggableElement(elmnt)
{
    var pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0
    function elementDrag(e)
    {
        e = e || window.event
        e.preventDefault()
        pos1 = pos3 - e.clientX
        pos2 = pos4 - e.clientY
        pos3 = e.clientX
        pos4 = e.clientY
        elmnt.style.top = (elmnt.offsetTop - pos2) + "px"
        elmnt.style.left = (elmnt.offsetLeft - pos1) + "px"
        elmnt.style.zIndex = draggableZIndex++
    }
    function closeDragElement()
    {
        document.onmouseup = null
        document.onmousemove = null
    }
    function dragMouseDown(e)
    {
        e = e || window.event
        e.preventDefault()
        pos3 = e.clientX
        pos4 = e.clientY
        document.onmouseup = closeDragElement
        document.onmousemove = elementDrag
    }
    elmnt.querySelector('.header').onmousedown = dragMouseDown
}
{
    let coll = document.getElementsByClassName("draggable")
    for (let i = 0; i < coll.length; i++)
        draggableElement(coll[i])
}

// search
var searchCpp;
var gotoPositionCpp;
function search()
{
    if (!searchCpp)
        return
    searchCpp(document.getElementById("searchQuery").value)
}
function gotoPosition(x, y, z, ve)
{
    if (!gotoPositionCpp)
        return
    gotoPositionCpp(x, y, z, ve)
}

// apply options
var applyOptionsCpp
var getOptionsCpp
function applyOption(e)
{
    let t = e.target
    if (t.tagName == "SELECT")
    {
        if (isNaN(t.value))
            applyOptionsCpp('{ "' + t.name + '":"' + t.value + '" }')
        else
            applyOptionsCpp('{ "' + t.name + '":' + t.value + ' }')
    }
    else if (t.tagName == "INPUT")
    {
        if (t.type == "checkbox")
            applyOptionsCpp('{ "' + t.name + '":' + (t.checked ? "true" : "false") + ' }')
        else if (t.type == "range" || t.type == "number")
            applyOptionsCpp('{ "' + t.name + '":' + t.value + ' }')
    }
}
function setOption(t, options)
{
    if (t.tagName == "SELECT")
        t.value = options[t.name]
    else if (t.tagName == "INPUT")
    {
        if (t.type == "checkbox")
            t.checked = options[t.name] == "true"
        else if (t.type == "range" || t.type == "number")
            t.value = options[t.name]
    }
}
function tileDiagnosticsClick(button)
{
    let content = button.nextElementSibling
    if (content.style.display === "block")
        applyOptionsCpp('{ "debugRenderTileDiagnostics":false }')
    else
        applyOptionsCpp('{ "debugRenderTileDiagnostics":true }')
}
var Module =
{
    onRuntimeInitialized: function()
    {
        searchCpp = Module.cwrap("search", null, ["string"])
        gotoPositionCpp = Module.cwrap("gotoPosition", null, ["number", "number", "number", "number"])
        applyOptionsCpp = Module.cwrap("applyOptions", null, ["string"])
        getOptionsCpp = Module.cwrap("getOptions", "string", null)
    },
    onMapCreated: function()
    {
        // initialize options events
        {
            let optionsRaw = getOptionsCpp()
            let lines = optionsRaw.split("\n").filter(function(l)
            {
                return l.match(/\:/)
            })
            let options = new Object()
            lines.forEach(function(l)
            {
                l = l.replace(/\,|\"/g, "")
                let pair = l.split(":")
                let name = pair[0].trim()
                let value = pair[1].trim()
                options[name] = value
            })
            let coll = document.getElementsByClassName("options")
            for (let i = 0; i < coll.length; i++)
            {
                setOption(coll[i], options)
                coll[i].onchange = applyOption
            }
        }
        // clear status
        {
            document.getElementById("status").innerHTML = ""
        }
    }
}

