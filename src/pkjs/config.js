var initialized = false;
var options = {};

Pebble.addEventListener("ready", function() {
  console.log("ready called!");
  initialized = true;
});

Pebble.addEventListener("showConfiguration", function() {
  console.log("showing configuration");
  Pebble.openURL('http://www.puhgee.de/huequeen_config.html?'+encodeURIComponent(JSON.stringify(options)));
});

Pebble.addEventListener("appmessage", function(e) {
  var values = e.payload.url.split('?');
  
  var options;
  if (values[2] >= 0)
    options = "{\"bri\":" + values[1] +",\"sat\":" + values[2] +"}";
  else
    options = "{\"bri\":" + values[1] +"}";
  //console.log(options);
  //parse this to valid json
  var req = new XMLHttpRequest();
  //console.log("Options = " + values[0]);
  req.open('PUT', values[0], true);
  req.send(options);
  }
);

Pebble.addEventListener("webviewclosed", function(e) {
  console.log("configuration closed");
  // webview closed
  //Using primitive JSON validity and non-empty check
  if (e.response.charAt(0) == "{" && e.response.slice(-1) == "}" && e.response.length > 1) {
    options = JSON.parse(decodeURIComponent(e.response));
    console.log("Options = " + JSON.stringify(options));
    Pebble.sendAppMessage(options);
  } else {
    console.log("Cancelled");
  }

});