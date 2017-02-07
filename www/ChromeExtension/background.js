$.ajaxSetup({
    // Disable caching of AJAX responses
    cache: false
});

var currentUrl = "";

function functionToLoadFile(){
  jQuery.get('http://localhost:8080/url.txt', function(data) {
  	var myvar = data;
  	var parts = myvar.split(/\n/);
  	var newUrl = parts[0];
	if (newUrl != currentUrl) {
   		currentUrl=newUrl;
  		chrome.tabs.update(null, {'url' : currentUrl});
	}
  });

}

setInterval(functionToLoadFile, 1000);



