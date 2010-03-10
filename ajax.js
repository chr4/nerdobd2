function update_variable(varname)
{
    var req = false;
    
    // we only need support firefox / konqueror
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
    tmp = new Image();
    tmp.style.visibility = 'hidden';
    
    tmp.src = "/" + img + ".png?" + Date.parse(new Date());
    
    image_loaded(tmp, img);
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
    setTimeout ( "update_debug()", 200 );    
}

function start()
{
    update_all();
    update_debug();
}

function reset_counters()
{
    update_variable("reset");
}


// set graph to timespan: short, medium, long
function av_con(timespan)
{
    update_variable("av_con_graph:" + timespan);
}

// set graph to timespan: short, medium, long
function av_speed(timespan)
{
    update_variable("av_speed_graph:" + timespan);
}