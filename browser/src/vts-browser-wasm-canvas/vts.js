
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
var draggableZIndex = 100
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
        elmnt.style.zIndex = draggableZIndex++
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

// mapconfig
var mapconfigCpp
var viewPresetCpp
function mapconfig()
{
    let url = document.getElementById("mapconfig").value
    mapconfigCpp(url)
}
function viewPreset()
{
    let select = document.getElementById("viewPreset")
    let preset = select.options[select.selectedIndex].innerHTML
    viewPresetCpp(preset)
}

// position
var positionCpp
function position()
{
    let pos = document.getElementById("positionInput")
    positionCpp(pos.value)
}
function delayedPosition()
{
    setTimeout(function(){position()});
}
function positionToClipboard()
{
    let pos = document.getElementById("positionCurrent")
    pos.select()
    document.execCommand("copy")
}

// initialize
var Module =
{
    onRuntimeInitialized: function()
    {
        searchCpp = Module.cwrap("search", null, ["string"])
        gotoPositionCpp = Module.cwrap("gotoPosition", null, ["number", "number", "number", "number"])
        applyOptionsCpp = Module.cwrap("applyOptions", null, ["string"])
        getOptionsCpp = Module.cwrap("getOptions", "string", null)
        mapconfigCpp = Module.cwrap("setMapconfig", null, ["string"])
        viewPresetCpp = Module.cwrap("setViewPreset", null, ["string"])
        positionCpp = Module.cwrap("setPosition", null, ["string"])
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

