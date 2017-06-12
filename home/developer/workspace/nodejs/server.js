var http = require("http");
var sqlite3 = require('sqlite3');
var url=require('url');
var fs = require("fs")
var path = require("path")
var redis = require('redis');

function getClientIp(req) {
    return req.headers['x-forwarded-for'] ||
    req.connection.remoteAddress ||
    req.socket.remoteAddress ||
    req.connection.socket.remoteAddress;
};

var get_client_ip = function(req) {
    var ip = req.headers['x-forwarded-for'] ||
        req.ip ||
        req.connection.remoteAddress ||
        req.socket.remoteAddress ||
        req.connection.socket.remoteAddress || '';
	var gr = ip.split(":");
         return gr[gr.length-1];
};

function getMovieResStr(data)
{
      return "{\"code\":\"1000\",\"data\":{\"powers\":1,\"type\":0,\"movieList\":"+data+"},\"msg\":\"ok\"}"; 	
}

function getLiveResStr(data)
{
      return "{\"code\":\"1000\",\"data\":{\"powers\":1,\"type\":1,\"movieList\":"+data+"},\"msg\":\"ok\"}"; 	
}


function makeMovie(name,videoPath,imagePath)
{
	var obj = new Object();
	obj.name = name;
	obj.vedioPath = videoPath;
	obj.imagePath = imagePath;
	return obj;
}


http.createServer(function(request, response){
	var requesturl=url.parse(request.url).pathname;
	console.log(requesturl);
	if(requesturl=="/video")
	{
		var movieArray=new Array();		
       		 var db =new sqlite3.Database('/home/developer/workspace/database/demo.db',function(){
			db.all("select * from movie_table",function(err,res){
			if(!err)
			{
				//console.log(res.length);
				for(var key in res)
				{
					movieArray.push(makeMovie(res[key].Name,res[key].VedioPath,res[key].ImagePath));	
				}
				//console.log(movieArray);
		    		 var str = JSON.stringify(movieArray);
  				response.setHeader("Access-Control-Allow-Origin", "*");
				response.writeHead(200,{"Content-Type":"text/plain"});
				//console.log(getMovieResStr(str));
  				response.write(getMovieResStr(str));
  				response.end();
			}else
			{
  				response.writeHead(200,{"Content-Type":"text/plain"});
  				response.write("error");
  				response.end();			
			}
			});	
		});
	}else if(requesturl=="/live")
	{
		var movieArray=new Array();		
       		 var db =new sqlite3.Database('/home/developer/workspace/database/demo.db',function(){
			db.all("select * from live_table",function(err,res){
			if(!err)
			{
				//console.log(res.length);
				for(var key in res)
				{
					movieArray.push(makeMovie(res[key].Name,res[key].VedioPath,res[key].ImagePath));	
				}
				//console.log(movieArray);
		    		 var str = JSON.stringify(movieArray);
  				response.setHeader("Access-Control-Allow-Origin", "*");
				response.writeHead(200,{"Content-Type":"text/plain"});
				//console.log(getLiveResStr(str));
  				response.write(getLiveResStr(str));
  				response.end();
			}else
			{
  				response.writeHead(200,{"Content-Type":"text/plain"});
  				response.write("error");
  				response.end();			
			}
			});	
		});	
	}else if(requesturl=="/getname")
	{	
		response.setHeader("Access-Control-Allow-Origin", "*");
		response.writeHead(200,{"Content-Type":"text/plain"});
		var key = "username#"+get_client_ip(request);
		console.log("key--"+key);
		var client = redis.createClient(6379,'127.0.0.1');
		
		client.get(key,function(err,res){  
			if(err){  
       				console.log(err); 
				response.write("error"); 
			}else{  
       				console.log(res);  
				response.write(res);
			}
			client.end(true);
			client.quit();
			response.end();  
		});
		
	}
}).listen(8888);
