var fs = require("fs")
var path = require("path")

var root = path.join("/media")

var sqlite3 = require('sqlite3');

function createMovie()
{
	var obj = new Object();
	obj.Name="";
	obj.ImagePath="";
	obj.VedioPath="";
	obj.getInsertSql=function()
	{
		if(this.ImagePath == "")
			this.ImagePath = "ironMain.jpg";
        var sql="insert into movie_table (Name,ImagePath,VedioPath) values ('"+this.Name+"','"+this.ImagePath+"','"+this.VedioPath+"')";	
		return sql;
	}
	return obj;
}
/*
var db =new sqlite3.Database('/home/developer/workspace/database/demo.db',function(){
	db.all("select * from movie_table",function(err,res){
		if(!err)
		{
		     var str = JSON.stringify(res);
		     var obj = JSON.parse(str);
		     console.log(obj[0]);
		}
	});
});
*/
var map = new HashMap(); 
var arrayMovies = new Array();
readDirSync(root);
readDirImage(root);
console.log(map.size());
var movieArray = map.values();
for(var key in movieArray)
{
    console.log(movieArray[key].getInsertSql());
}


var db =new sqlite3.Database('/home/developer/workspace/database/demo.db',function(){
	console.log("---"+arrayMovies);
	var delsql = "delete from movie_table";
	db.run(delsql,function(){	
        	for(var key in movieArray)
		{
		//	var words= arrayMovies[key].split(".");
		//	if(words.length > 0)
	                 
				//console.log(words[0]+";;"+words[1]);
				db.run(movieArray[key].getInsertSql(),function(err,res){
						console.log(err);
						db.all("select * from movie_table",function(err,res){
						if(!err)
						{
		    					 var str = JSON.stringify(res);
		     						var obj = JSON.parse(str);
			     						console.log(obj);
						}
					});							
				});			
		}
	});
});


function readDirSync(path){
	var pa = fs.readdirSync(path);
	pa.forEach(function(ele,index){
		var info = fs.statSync(path+"/"+ele)	
		//console.log(info);
		if(info.isDirectory()){
			//console.log("dir: "+ele)
			readDirSync(path+"/"+ele);
		}else{
			//console.log("file: "+ele)
			//arrayMovies.push(ele);
			var words=ele.split(".");
			if(words.length>1)
			{
				//console.log("---------1");
				if(words[1] == "mp4")
				{
					var obj;
					//console.log("---------2");
					if(map.containsKey(words[0]))
					{
						obj = map.get(words[0]);
					}else
					{
						obj=createMovie();
						obj.Name = words[0];
						//obj.VedioPath = ele;
						var tempVedioPath = path +"/"+ele;
						obj.VedioPath = tempVedioPath.substr(7);
						map.put(obj.Name,obj);
					}
					//console.log(obj.getInsertSql());
				}
			}
		}	
	})
}

function readDirImage(path){
	var pa = fs.readdirSync(path);
	pa.forEach(function(ele,index){
		var info = fs.statSync(path+"/"+ele)	
		if(info.isDirectory()){
			console.log("dir: "+ele)
			readDirImage(path+"/"+ele);
		}else{
			//console.log("file: "+ele)
			//arrayMovies.push(ele);
			var words=ele.split(".");
			if(words.length>1)
			{
				//console.log("---------1");
				if(words[1] == "jpg" || words[1] == "png")
				{
					var obj;
					//console.log("---------2");
					if(map.containsKey(words[0]))
					{
						obj = map.get(words[0]);
						//obj.ImagePath=ele;
						//obj.VedioPath = ele;
						var tempImagePath = path +"/"+ele;
						obj.ImagePath= tempImagePath .substr(7);
					}				
				}
			}
		}	
	})
}


function readDir(path){
	fs.readdir(path,function(err,menu){	
		if(!menu)
			return;
		menu.forEach(function(ele){	
			fs.stat(path+"/"+ele,function(err,info){
				if(info.isDirectory()){
					//console.log("dir: "+ele)
					readDir(path+"/"+ele);
				}else{
					//console.log("file: "+ele)
					arrayMovies.push(ele);
					console.log(arrayMovies.length);
				}	
			})
		})			
	})
}

function HashMap(){  
    //定义长度  
    var length = 0;  
    //创建一个对�? 
    var obj = new Object();  
  
    /** 
    * 判断Map是否为空 
    */  
    this.isEmpty = function(){  
        return length == 0;  
    };  
  
    /** 
    * 判断对象中是否包含给定Key 
    */  
    this.containsKey=function(key){  
        return (key in obj);  
    };  
  
    /** 
    * 判断对象中是否包含给定的Value 
    */  
    this.containsValue=function(value){  
        for(var key in obj){  
            if(obj[key] == value){  
                return true;  
            }  
        }  
        return false;  
    };  
  
    /** 
    *向map中添加数�?
    */  
    this.put=function(key,value){  
        if(!this.containsKey(key)){  
            length++;  
        }  
        obj[key] = value;  
    };  
  
    /** 
    * 根据给定的Key获得Value 
    */  
    this.get=function(key){  
        return this.containsKey(key)?obj[key]:null;  
    };  
  
    /** 
    * 根据给定的Key删除一个�?
    */  
    this.remove=function(key){  
        if(this.containsKey(key)&&(delete obj[key])){  
            length--;  
        }  
    };  
  
    /** 
    * 获得Map中的所有Value 
    */  
    this.values=function(){  
        var _values= new Array();  
        for(var key in obj){  
            _values.push(obj[key]);  
        }  
        return _values;  
    };  
  
    /** 
    * 获得Map中的所有Key 
    */  
    this.keySet=function(){  
        var _keys = new Array();  
        for(var key in obj){  
            _keys.push(key);  
        }  
        return _keys;  
    };  
  
    /** 
    * 获得Map的长�?
    */  
    this.size = function(){  
        return length;  
    };  
  
    /** 
    * 清空Map 
    */  
    this.clear = function(){  
        length = 0;  
        obj = new Object();  
    };  
}  
