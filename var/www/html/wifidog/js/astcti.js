function GetXmlHttpObject()
   {
    var xmlHttp=null
    try
      {
       xmlHttp=new XMLHttpRequest();
      }
    catch (e)
      {
       try
         {
           xmlHttp=new ActiveXObject("Msxml2.XMLHTTP");
         }
        catch (e)
          {
           xmlHttp=new ActiveXObject("Microsoft.XMLHTTP");
          }
        }
 return xmlHttp;

}

 

function get_from_perl()
{
var xmlHttp=null
  xmlHttp=GetXmlHttpObject()
  if(xmlHttp==null){
    alter("Browser does not support HTTP Request.")
    return
  }
var url="/test.cgi";
xmlHttp.onreadystatechange=stateChanged
xmlHttp.open("GET",url,true)
xmlHttp.send(null)

  function stateChanged()
   {
    if (xmlHttp.readyState==4 || xmlHttp.readyState=="complete")
      {
       var r_str=xmlHttp.responseText;
       var divNode = document.getElementById('cgi_result');
        //6.将服务器的数据显示在客户端
       divNode.textContent = r_str;   
       //document.getElementById("cgi_result").innerHTML=eval("'" + xmlHttp.responseText + "'");
      }

   }

}
