var Graph = function(Name, Timespan)
{
  var name = Name;
  var index = 0;
  var timespan = 0;
  var interval = 0;
  var timespan = 0;

  var options = {
    series: {
      shadowSize: 0
    },
    legend: {
      show: true,
      position: 'nw',
      backgroundOpacity: 0,
      labelFormatter: function (label, series) {
        return '<span style="font-family:helvetica;">' + label + '</span> ' +
               '<span id="graph_' + name + '_increase" class="graph_modifier">[+]</span> ' +
               '<span id="graph_' + name + '_decrease" class="graph_modifier">[-]</span>';
      }
    }
  };

  var series = {
    data: []
  };

  var setTimespan = function(newTimespan) {
    console.info('setting timespan for graph "' + name + '" to ' + newTimespan);

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

    updateLabel();
  }

  var getLocalTime = function() {
    // get localtime
    var t = new Date();
    return t.getTime() - t.getTimezoneOffset() * 60000;
  }

  var updateLabel = function() {
    // display hours if graph is more than 60min
    if (timespan > 3600000)
      series.label = name + " (last " + timespan / 3600000 + "h)";
    else
      series.label = name + " (last " + timespan / 60000 + "min)";
  }

  var updateOptions = function() {
    var now = getLocalTime();

    // adjust the xaxis, so graph moves forward
    options.xaxis.min = now - timespan;

    // prevent small empty gaps at the beginning of the graph
    options.xaxis.max = now - 1000;
  }

  var appendData = function(data) {
    var i;
    var t = new Date();

    // add timezone difference to each dataset
    for (i = data.length - 1; i > 0; i--)
      data[i][0] -= t.getTimezoneOffset() * 60000;

    // update series data
    series.data = data;

    // loop reverse, for better performance
    for (i = series.data.length - 1; i > 0; i--) {
      // after we find the first element that is outdated
      if (series.data[i][0] <= getLocalTime() - timespan) {
        // remove everything up to this element
        series.data.splice(0, i);
        break;
      }

      /* display gaps of information as gaps,
       * don't just connect the dots
       * if the time between two dots is more than 20 sec
       * don't connect them (i.e. add a dot at -1)
       */
      if (series.data[i][0] > series.data[i - 1][0] + 20000) {
        // add two new dots with y value -1
        series.data.splice(i, 0, [ series.data[i][0], -1 ]);
        series.data.splice(i, 0, [ series.data[i - 1][0], -1 ]);
      }
    }

    updateOptions();
    updateLabel();
    plot();
  }

  var update = function() {
    var onDataReceived = function(json) {
      if (typeof(json.index) !== "undefined")
        index = json.index;

      if (typeof(json.data) !== "undefined")
        appendData(json.data);
    }

    $.ajax({
      url: name + ".json" +
           "?index=" + index +
           "&timespan=" + timespan / 1000,
      method: 'GET',
      dataType: 'json',
      success: onDataReceived,
      error: function(jqXHR, textStatus, errorThrown) {
        console.info("error graph '" + name + "': " + textStatus + " : " + errorThrown);
      },
      complete: setTimeout(function() { update(); }, interval)
    });
  }

  var plot = function() {
    console.info("plotting graph '" + name + "'");
    $.plot($("#graph-" + name), [ series ], options);

    // add onclick handlers for modifing timespan
    $('#graph_' + name + '_increase').click(function() {
      setTimespan(timespan * 2);
      plot();
    });

    $('#graph_' + name + '_decrease').click(function() {
      setTimespan(timespan / 2);
      plot();
    });
  }

  var setOptions = function(Options) {
    jQuery.extend(options, Options);
  }

  var setSeries = function(Series) {
    jQuery.extend(series, Series);
  }

  var _init = function() {
    console.info("creating graph '" + name + "'");

    setTimespan(Timespan);
    update();
  }

  _init();

  return {
    setOptions: setOptions,
    setSeries: setSeries,
    setTimespan: setTimespan
  }
}
