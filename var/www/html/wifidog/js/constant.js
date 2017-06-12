var host = 'http://' + location.hostname;
//var host = 'http://192.168.0.5';
var base_url = host + ":8080/";
var name = '';
//$(function() {
//	getUser(function(data){
//		console.log(data);
//	});
//});

function getUser(func){
	console.log("base_url:"+base_url);
	$.get(host + ':8888/'+'getname',function(data){
		console.log(data);
		if(isEmpty(data)){
			window.location.href = host+"/wifidog/login.html";
			return;
		}
		//console.log(data);
		func(data);
	});
}

function isEmpty(data){
	data = data.trim();
	return data == null|| data == 'null' || data == '';
}

function getParams(name){
	var reg = new RegExp("(^|&)"+name+"=([^&]*)(&|$)");
	var r = window.location.search.substr(1).match(reg);
	if(r!=null)return unescape(r[2]);
	return;
}

function GetLocalIPAddr() {
	var oSetting = null;
	var ip = null;
	try {
		oSetting = new ActiveXObject("rcbdyctl.Setting");
		ip = oSetting.GetIPAddress;
		if(ip.length == 0) {
			return "没有连接到Internet";
		}
		oSetting = null;
	} catch(e) {
		return ip;
	}
	return ip;
}