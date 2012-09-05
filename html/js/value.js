 var Value = function(Name, Unit, Accuracy) {
  var name = Name;
  var unit = Unit;
  var accuracy = Accuracy;
  
  var set = function(value, theunit) {
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

  return {
    update: function(data) {
      // if this is gps information, don't update <div>, but set location and stuff
      if( typeof(map) !== 'undefined' && name == "gps_mode" ) {
        if (data['gps_err_latitude'] < 50 && data['gps_err_longitude'] < 50)
          map.setLocation(data['gps_latitude'], data['gps_longitude'], data['gps_track']);
      }

      // switch to l/h if consumption_per_100km is nan (means that speed == 0)
      if (name == "consumption_per_100km" && isNaN(data[name]))
        set(data['consumption_per_h'], "l/h");
      else
        set(data[name], unit);
    }
  }
}


var Values = function(Url, Timeout) {
  var elements = [];
  var url = Url;
  var timeout = Timeout;
 
  var _init = function() { 
    setTimeout(update, timeout);
  }

  var update = function update() {
    var onDataReceived = function(json) {
      if (typeof(json) !== "undefined")
        for (var i = elements.length - 1; i >= 0; i--)
          elements[i].update(json);
    }
    // return if we don't have any elements yet
    if (elements.length == 0) {
      setTimeout(update, timeout);
      return;
    }
    else {
      $.ajax({
        url: url,
        method: 'GET',
        dataType: 'json',
        success: onDataReceived,
        complete: setTimeout(update, timeout)
      });
    }
  }

  _init();

  return {
    create: function(name, unit, accuracy) {
      elements.push(new Value(name, unit, accuracy));
    }
  }
}
