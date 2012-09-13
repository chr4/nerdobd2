var Map =function() {
  var map;
  var lastLocation;
  var currentLocation;
  var marker;
  var tiles;

  var _init = function() {
    map = new L.Map('map');
    tiles = new L.TileLayer('tiles/{z}/{x}/{y}.png', {minZoom: 10, maxZoom: 18});
    map.addLayer(tiles);

    // set initial map point to frankfurt
    var ffm = new L.LatLng(50.118845, 8.660713);
    map.setView(ffm, 15);

    // setup icon for current position
    var carIcon = L.Icon.extend({
                    shadowUrl: '',
                    shadowSize: new L.Point(0, 0),
                    iconUrl: '/css/images/car-grey.png',
                    iconSize: new L.Point(25, 50),
                    iconAnchor: new L.Point(13, 25),
                    popupAnchor: new L.Point(13, 25)
                  });

    marker = new L.Marker(ffm, {icon: new carIcon()});
    map.addLayer(marker);

    lastLocation = new L.LatLng(0, 0);
  }

  _init();

  return {
    setLocation: function(lat, lng, track) {
      // return if we don't have latlng
      if ( isNaN(lat) || isNaN(lng))
        return;

      currentLocation = new L.LatLng(lat, lng);

      // 0.0 means no location, return
      if (currentLocation.equals(new L.LatLng(0, 0)))
        return;

      // pan to the new location and update location circle
      if (!currentLocation.equals(lastLocation)) {
        map.panTo(currentLocation);
        marker.setLatLng(currentLocation);

        // rotate arrow into driving direction
        if ( !isNaN(track) ) {
          $(marker._icon).css('-webkit-transform', 'rotate(' + track + 'deg)');
          $(marker._icon).css('-moz-transform', 'rotate(' + track + 'deg)');
        }

        lastLocation = currentLocation;
      }
    }
  }
}
