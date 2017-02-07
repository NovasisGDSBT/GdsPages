#!/bin/sh
# AUTHENTICATION and INITIALIZATION

# PARSE QUERY STRING
[ "$REQUEST_METHOD" = "POST" ] && read QUERY_STRING
saveIFS=$IFS
IFS='&'
set -- $QUERY_STRING

# CHECK CREDENTIALS
if [ "$1" == "username=gds" ]; then
	if [ "$2" == "password=gds" ]; then
# AUTHENTICATION SUCCESS 
		echo "<meta http-equiv=\"refresh\" content=\"1; url=/cgi-bin/main_menu.cgi\"/>"
	fi
else
# AUTHENTICATION FAILURE
    echo "<meta http-equiv="Content-Type" content="text/html; CHARSET=utf-8">"

    echo "<html>"
    echo "<head>"
    echo "	<center><h3>AUTHENTICATION PAGE</h3></center>"
    echo "</head>"
    echo "<body>"
    echo "<br>"
    echo "	<form action=\"auth.cgi\" method=\"POST\">"
    echo "	        USERNAME: <input type=\"text\" name=\"username\">&nbsp;&nbsp;default: novasis"
    echo "              <br>"
    echo "              <br>"
    echo "	        PASSWORD: &nbsp;<input type=\"password\" name=\"password\">&nbsp;&nbsp;default: novasis"
    echo "              <br>"
    echo "              <br>" 
    echo "	        <button type=\"submit\" value=\"LOGIN\">LOGIN</button>"	
    echo "      </form>" 
    echo "</body>"
    echo "</html>"

    echo "<p>AUTHENTICATION FAILURE TRY AGAIN</p>"

fi



exit 0
