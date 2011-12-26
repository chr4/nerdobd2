function Graph(Name, Timespan)
{
  var name = "";
  var index = 0;
  var timespan = 0;
  var interval = 0;
  
  var options = {};
  var series = {};
  
  this.setIndex = function(newIndex)
  {
    index = newIndex;
  }
  
  this.getIndex = function()
  {
    return index;
  }
  
  this.setTimespan = function(newTimespan)
  {
    // reset index, if new timespan is larger than the old one
    // so we will fetch older data as well
    if (timespan < newTimespan)
      index = 0;
    
    // update timespan
    timespan = newTimespan;
    
    // don't update more than once a second
    if (timespan < 30000)
      interval = 1000;
    else
      interval = timespan / 300;
  }
  
  this.getTimespan = function()
  {
    return timespan;
  }
  
  this.getLocalTime = function()
  {
    // get localtime
    var t = new Date();
    return t.getTime() - t.getTimezoneOffset() * 60000;
  }
  
  this.updateLabel = function()
  {
    // display hours if graph is more than 60min
    if (timespan > 3600000)
      series.label = name + " (last " + timespan / 3600000 + "h)";
    else
      series.label = name + " (last " + timespan / 60000 + "min)";
  }
  
  this.updateOptions = function()
  {
    var now = this.getLocalTime();
    
    // adjust the xaxis, so graph moves forward
    options.xaxis.min = now - timespan;
    
    // prevent small empty gaps at the beginning of the graph
    options.xaxis.max = now - 1000; 
  }
  
  this.appendData = function(data)
  {
    var i;
    
    // append new data to series
    series.data = $.merge(series.data, data);
    
    // loop reverse, for better performance
    for (i = series.data.length - 1; i > 0; i--)
    {
      // after we find the first element that is outdated
      if (series.data[i][0] <= this.getLocalTime() - timespan)
      {
        // remove everything up to this element
        series.data.splice(0, i);
        break;
      }
      
      /* display gaps of information as gaps,
       * don't just connect the dots
       * if the time between two dots is more than 20 sec
       * don't connect them (i.e. add a dot at -1)
       */
      if (series.data[i][0] > series.data[i - 1][0] + 20000)
      {
        // add two new dots with y value -1
        series.data.splice(i, 0, [ series.data[i][0], -1 ]);
        series.data.splice(i, 0, [ series.data[i - 1][0], -1 ]);
      }
    }
    
    this.updateOptions();
    this.updateLabel();
    this.plot();
  }
  
  this.update = function()
  {
    // this is out of scope in onDataReceived
    var that = this;
    
    function onDataReceived(json)
    {
      if (typeof(json.index) !== "undefined")
        that.setIndex(json.index);
      
      if (typeof(json.data) !== "undefined")
        that.appendData(json.data);
    }
    
    $.ajax({
      url: name + ".json" +
      "?index=" + index +
      "&timespan=" + timespan / 1000,
      method: 'GET',
      dataType: 'json',
      success: onDataReceived,
      complete: setTimeout(function() { that.update(); }, interval)
    });
  }
  
  this.plot = function()
  {
    $.plot($("#graph-" + name), [ series ], options);
  }
  
  this.setOptions = function(Options)
  {
    jQuery.extend(options, Options);
  }
  
  this.setSeries = function(Series)
  {
    jQuery.extend(series, Series);
  }
  
  this.construct = function()
  {
    name = Name;
    this.setTimespan(Timespan);
    
    // create data array and set label
    series.data = [];
    
    // disable shadows for better performance
    options.series = {};
    options.series.shadowSize = 0;
    
    // configure the legend
    options.legend = {};
    options.legend.show = true;
    options.legend.position = "nw";
    options.legend.backgroundOpacity = 0;
    options.legend.labelFormatter = function (label, series) {
      return '<span style="font-family:helvetica;">' + label + '</span>';
    }
    
    this.updateLabel();
    
    this.update();
  }
  
  this.construct();
}
