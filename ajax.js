function update_variable(varname)
{
    var req = false;
    
    // we only need support firefox / konqueror / chrome, etc
	req = new XMLHttpRequest();
    
	req.open("POST", "/get.obd?" + varname, true);
    req.onreadystatechange = function()
    {
        if (req.readyState == 4)
        {
            // check if variable is valid (if not, don't update
            if (req.responseText)
            {

                // set color back to normal
                if (varname != "debug")
                    document.getElementById(varname).style.color = "#f00000";
                
                if (varname == "con_km")
                {
                    // automatically switch to l/h on con_km == -1
                    if (req.responseText == -1)
                    {
                        document.getElementById("con_km").innerHTML = document.getElementById("con_h").innerHTML;
                        document.getElementById("con_km_unit").innerHTML = "l/h";
                    }
                    else
                    {
                        document.getElementById("con_km").innerHTML = req.responseText;
                        document.getElementById("con_km_unit").innerHTML = "l/km";
                    }
                    
                }
                else
                {
                    document.getElementById(varname).innerHTML = req.responseText;
                }
            }
            else
            {    
                if (varname != "debug")
                    document.getElementById(varname).style.color = "#888888";
            }
        }
    }
    // send newline to request variable
    req.send('\n');
}


// show image tmp in img after it is completely loaded
function image_loaded(tmp, img)
{
    if (tmp.complete)
    {
        // document.getElementById("debug").innerHTML = "image loaded<br/>" + Date.parse(new Date());
        
        // show image
        replace = document.getElementById("img_" + img);
        replace.src = tmp.src;
        
        // hide the "loading" popup
        if (tmp.src.search("consumption.png") != -1)
            document.getElementById("update_popup_con").style.visibility = 'hidden'; 
        else if (tmp.src.search("speed.png") != -1)
            document.getElementById("update_popup_speed").style.visibility = 'hidden'; 
        
    }
    else
    {
        //document.getElementById("debug").innerHTML = "waiting<br/>" + Date.parse(new Date());
        
        // check again in 10ms
        setTimeout("image_loaded(" + tmp + "," + img + ")", 10);
    }
}


function update_image(img)
{
    var tmp = new Image();
    tmp.style.visibility = 'hidden';
    
    tmp.src = "/" + img + ".png?" + Date.parse(new Date());
    
    tmp.onload = function()
    {
        /* only show image if its completely loaded
         * to prevent images from flickering
         */
        if (tmp.complete && tmp.width)
        {         
            replace = document.getElementById("img_" + img);
            replace.src = tmp.src;
        
            // hide the "loading" popup
            if (tmp.src.search("consumption.png") != -1)
                document.getElementById("update_popup_con").style.visibility = 'hidden'; 
            else if (tmp.src.search("speed.png") != -1)
                document.getElementById("update_popup_speed").style.visibility = 'hidden';         
        }
    }
    
    // image_loaded(tmp, img);
}


function update_all()
{
    update_variable("con_h");
    update_variable("con_km");    
    update_variable("rpm");
    update_variable("speed");
    update_variable("temp1");
    update_variable("temp2");
    update_variable("voltage");
    
    update_variable("con_av_short");
    update_variable("con_av_medium");
    update_variable("con_av_long");
    
    update_variable("speed_av_short");
    update_variable("speed_av_medium");
    update_variable("speed_av_long");
    
    update_image("speed");
    update_image("consumption");
    
    // restart timer
    setTimeout ( "update_all()", 500 );
}

function update_debug()
{
    update_variable("debug");
    setTimeout ( "update_debug()", 500 );    
}



function reset_counters()
{
    update_variable("reset");
    document.getElementById("debug").innerHTML = "counters resetted.";
}


// set graph to timespan: short, medium, long
function av_con(timespan)
{
    // show "loading" popup
    document.getElementById("update_popup_con").style.visibility = 'visible';
    update_variable("av_con_graph:" + timespan);
}

// set graph to timespan: short, medium, long
function av_speed(timespan)
{
    // show "loading" popup
    document.getElementById("update_popup_speed").style.visibility = 'visible';    
    update_variable("av_speed_graph:" + timespan);
}


function start()
{
    update_all();
    update_debug();
    
    // add shortcuts
    document.onkeyup = function(e)
    {      
        // shortcut for resetting counters
        if (e.which == 82)      // r
            reset_counters();
        
        // shortcut for consumption graph timespans
        else if (e.which == 81) // q
            av_con("short");
        else if (e.which == 65) // a
            av_con("medium");
        else if (e.which == 89 || e.which == 90) // y or z (to support qwertz/qwerty keyboards)
            av_con("long");
        
        // shortcut for speed graph timespans
        else if (e.which == 87) // w
            av_speed("short");
        else if (e.which == 83) // s
            av_speed("medium");
        else if (e.which == 88) // x
            av_speed("long");
    }
}
