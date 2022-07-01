String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
 
String html_1 = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'/>
  <meta charset='utf-8'>
  <style>
    body    { font-size:120%;} 
    #main   { display: table; width: 300px; margin: auto;  padding: 10px 10px 10px 10px; border: 3px solid blue; border-radius: 10px; text-align:center;} 
    .button { width:200px; height:40px; font-size: 80%;  }
  </style>
  <title>TerrainTronics Harlech Castle Demo</title>
</head>
<body>
  <div id='main'>
    <h3>Terrain Tronics Wifi Controllers</h3>
    <div id='content'>
      <p id='LED_status'>Development Date 6/29/2022</p>
            <p>Designed for Harlech PG1.1</p>
      <button id='BTN_LEDA'class="button">Trigger Sound Effect board</button>
      <button id='BTN_LEDB'class="button">Lights On</button>
      <button id='BTN_LEDC'class="button">Lights Off</button>
      <button id='BTN_LEDD'class="button">Run LED Pattern</button>
    </div>
    <br />
   </div>
</body>
 
<script>
  var Socket;
  function init() 
  {
    Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
  }
 
  document.getElementById('BTN_LEDA').addEventListener('click', buttonAClicked);
  function buttonAClicked()
  {   
    sendText('t');
  }
  document.getElementById('BTN_LEDB').addEventListener('click', buttonBClicked);
  function buttonBClicked()
  {   
    sendText('1');
  }
 document.getElementById('BTN_LEDC').addEventListener('click', buttonCClicked);
  function buttonCClicked()
  {   
    sendText('2');
  }
  document.getElementById('BTN_LEDD').addEventListener('click', buttonDClicked);
  function buttonDClicked()
  {   
    sendText('0');
  }
  function sendText(data)
  {
    Socket.send(data);
  }
 
  window.onload = function(e)
  { 
    init();
  }
</script>
</html>
)=====";
