//
//  HTML PAGE
//
const char PAGE_ApplicationConfiguration[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<a href="/"  class="btn btn--s"><</a>&nbsp;&nbsp;<strong>Application Configuration</strong>
<hr>
<br>
<form action="" method="get">
<table border="0"  cellspacing="0" cellpadding="3" style="width:650px" >
<tr><td align="right">API key:</td><td><input type="text" style="width:350px"   id="constant1" name="constant1" value=""></td></tr>
<tr><td align="right">Location:</td><td><input type="text" style="width:350px"   id="constant2" name="constant2" value=""></td></tr>
<tr><td align="right">Board Name:</td><td><input type="text" style="width:350px"   id="constant3" name="constant3" value=""></td></tr>
<tr><td align="right">Constant4:</td><td><input type="text" style="width:350px"   id="constant4" name="constant4" value=""></td></tr>
<tr><td align="right">Constant5:</td><td><input type="text" style="width:350px"   id="constant5" name="constant5" value=""></td></tr>
<tr><td align="right">Constant6:</td><td><input type="text" style="width:350px"   id="constant6" name="constant6" value=""></td></tr>
<tr><td align="right">IOTappStore1:</td><td><input type="text" style="width:350px"   id="IOTappStore1" name="IOTappStore1" value=""></td></tr>
<tr><td align="right">IOTappStorePHP1:</td><td><input type="text" style="width:350px"  id="IOTappStorePHP1" name="IOTappStorePHP1" value=""></td></tr>
<tr><td align="right">IOTappStore2:</td><td><input type="text"    style="width:350px"  id="IOTappStore2" name="IOTappStore2" value=""></td></tr>
<tr><td align="right">IOTappStorePHP2:</td><td><input type="text" style="width:350px"  id="IOTappStorePHP2" name="IOTappStorePHP2" value=""></td></tr>

<tr><td colspan="2" align="center"><input type="submit" style="width:150px" class="btn btn--m btn--blue" value="Save"></td></tr>
</table>
</form>

<script>

function GetState()
{
  setValues("/admin/applvalues");
}


window.onload = function ()
{
  load("style.css","css", function() 
  {
    load("microajax.js","js", function() 
    {
          setValues("/admin/applvalues");

    });
  });
}
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}

</script>


)=====";

//
//  SEND HTML PAGE OR IF A FORM SUMBITTED VALUES, PROCESS THESE VALUES
// 

void send_application_configuration_html()
{
  if (webServer.args() > 0 )  // Save Settings
  {    
    for ( uint8_t i = 0; i < webServer.args(); i++ ) {
      if (webServer.argName(i) == "constant1")    constant1 =   urldecode(webServer.arg(i));
      if (webServer.argName(i) == "constant2")    constant2 =   urldecode(webServer.arg(i));
      if (webServer.argName(i) == "constant3")    constant3 =   urldecode(webServer.arg(i));
      if (webServer.argName(i) == "constant4")    constant4 =   urldecode(webServer.arg(i));
      if (webServer.argName(i) == "constant5")    constant5 =   urldecode(webServer.arg(i));
      if (webServer.argName(i) == "constant6")    constant6 =   urldecode(webServer.arg(i));
      if (webServer.argName(i) == "IOTappStore1")    IOTappStore1 =   urldecode(webServer.arg(i));
      if (webServer.argName(i) == "IOTappStorePHP1") IOTappStorePHP1 =   urldecode(webServer.arg(i));
      if (webServer.argName(i) == "IOTappStore2")    IOTappStore2 =   urldecode(webServer.arg(i));
      if (webServer.argName(i) == "IOTappStorePHP2") IOTappStorePHP2 =   urldecode(webServer.arg(i));
      }
      constant1.toCharArray(config.constant1,50);
      constant2.toCharArray(config.constant2,50);
      constant3.toCharArray(config.constant3,50);
      constant4.toCharArray(config.constant4,50);
      constant5.toCharArray(config.constant5,50);
      constant6.toCharArray(config.constant6,50);
      IOTappStore1.toCharArray(config.IOTappStore1,40);
      IOTappStorePHP1.toCharArray(config.IOTappStorePHP1,40);
      IOTappStore2.toCharArray(config.IOTappStore2,40);
      IOTappStorePHP2.toCharArray(config.IOTappStorePHP2,40);
      
  }
   webServer.send ( 200, "text/html", PAGE_ApplicationConfiguration ); 
  Serial.println(__FUNCTION__); 
}



//
//   FILL THE PAGE WITH VALUES
//

void send_application_configuration_values_html()
{

  String values ="";

  values += "constant1|" + (String) config.constant1 + "|input\n";
  values += "constant2|" + (String) config.constant2 + "|input\n";
  values += "constant3|" + (String) config.constant3 + "|input\n";
  values += "constant4|" + (String) config.constant4 + "|input\n";
  values += "constant5|" + (String) config.constant5 + "|input\n";
  values += "constant6|" + (String) config.constant6 + "|input\n";
  values += "IOTappStore1|" + (String)    config.IOTappStore1 + "|input\n";
  values += "IOTappStorePHP1|" + (String) config.IOTappStorePHP1 + "|input\n";
  values += "IOTappStore2|" + (String) config.IOTappStore2 + "|input\n";
  values += "IOTappStorePHP2|" + (String) config.IOTappStorePHP2 + "|input\n";
  
  webServer.send ( 200, "text/plain", values);
  writeConfig();
  Serial.println(__FUNCTION__); 
  
}
