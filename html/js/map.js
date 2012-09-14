var Map =function() {
  var map;
  var lastLocation;
  var currentLocation;
  var marker;

  var _init = function() {
    map = new L.Map('map');
    L.tileLayer('tiles/{z}/{x}/{y}.png', { minZoom: 10, maxZoom: 18 } ).addTo(map);

    // set initial map point to frankfurt
    var ffm = [ 50.118845, 8.660713 ];
    map.setView(ffm, 15);

    // setup icon for current position
    var carIcon = L.icon({
                    iconUrl: '/css/images/car-grey.png',
                    iconSize: [ 25, 50 ],
                    iconAnchor: [ 13, 25 ],
                    shadowUrl: '',
                    shadowSize: [ 0, 0 ],
                    popupAnchor: [ 13, 25 ]
                  });

    marker = L.marker(ffm, { icon: carIcon });
    marker.addTo(map);

    lastLocation = [ 0, 0 ];
  }

  var = setLocation = function(lat, lng, track) {
    // return if we don't have latlng
    if ( isNaN(lat) || isNaN(lng))
      return;

    currentLocation = [ lat, lng ];

    // [ 0, 0 ] means no location, return
    if (currentLocation[0] == 0 && currentLocation[1] == 0)
      return;

    // pan to the new location and update location circle
    if (currentLocation[0] != lastLocation[0] && currentLocation[1] != lastLocation[1]) {
      map.panTo(currentLocation);
      marker.setLatLng(currentLocation);

      /*
       * FIXME (TODO)
       * arrow turning doesn't work anymore
       *
      // rotate arrow into driving direction
      if ( !isNaN(track) ) {
        $(marker._icon).css('-webkit-transform', 'rotate(' + track + 'deg)');
        $(marker._icon).css('-moz-transform', 'rotate(' + track + 'deg)');
      }
      */

      lastLocation = currentLocation;
    }
  }

  _init();

  return {
    setLocation: setLocation
  }
}
