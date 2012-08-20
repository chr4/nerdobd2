function Value(Name, Unit, Accuracy)
{
  var name;
  var unit;
  var accuracy;
  
  this.update = function(data)
  {
    // if this is gps information, don't update <div>, but set location and stuff
    if( typeof(map) !== 'undefined' && name == "gps_mode" ) {
      if (data['gps_err_latitude'] < 50 && data['gps_err_longitude'] < 50)
          map.setLocation(data['gps_latitude'], data['gps_longitude'], data['gps_track']);
    }

    // switch to l/h if consumption_per_100km is nan (means that speed == 0)
    if (name == "consumption_per_100km" && isNaN(data[name]))
      this.set(data['consumption_per_h'], "l/h");
    else
      this.set(data[name], unit);
  }
  
  this.set = function(value, theunit)
  {
    // if html field is not present, skip
    if ( $('#' + name) == undefined || value == undefined)
      return;
    
    // display readable gps fix status
    if (name == "gps_mode") {
      var status;
      switch(value) {
        case 2:
          $('#' + name).html('2D')
          break;
        case 3:
          $('#' + name).html('3D')
          break;
        default:
          $('#' + name).html('no')
      }
    }
    else {
      if (!isNaN(value))
        $('#' + name).html(value.toFixed(accuracy));
      
      // set the unit for consumption accordingly
      if (name == "consumption_per_100km")
        $('#' + name + '-unit').html(theunit);
    }
  }
  
  this.construct = function()
  {
    name = Name;
    unit = Unit;
    accuracy = Accuracy;
  }
  
  this.construct();
}


function Values(Url, Timeout)
{
  var elements;
  var url;
  var timeout;  
  
  this.create = function(name, unit, accuracy)
  {
    elements.push(new Value(name, unit, accuracy));
  }
  
  this.update = function update()
  {
    function onDataReceived(json)
    {
      if (typeof(json) !== "undefined")
        for (var i = elements.length - 1; i >= 0; i--)
          elements[i].update(json);
    }
    // return if we don't have any elements yet
    if (elements.length == 0)
    {
      setTimeout(update, timeout);
      return;
    }
    else
    {
      $.ajax({
        url: url,
        method: 'GET',
        dataType: 'json',
        success: onDataReceived,
        complete: setTimeout(update, timeout)
      });
    }
  }
  
  this.construct = function()
  {
    url = Url;
    timeout = Timeout;
      
    elements = [];
    setTimeout(this.update, timeout);
  }
  
  this.construct();
}
