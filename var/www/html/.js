$(function() {
//	var player = videojs('example-video');
//	player.play();
	$('.logo').attr('href','user.html?username='+name);
	$('.nav').find('div').click(function(){
		$('.nav').find('div').removeAttr('class');
		$(this).attr('class','selected');
		getData($(this).attr('data-type'));
	});
	$('.nav div').first().trigger('click');
	$('#logout').click(function(){
		window.location.href = host + ":2060/wifidog/disconnect";
	});
	getUser(function(data){
		
	});
});


function getData(type){
	if(type == 2){//资讯
		var data = '{"data":[{"img":"img/n1.jpg","title":"习近平会见洪秀柱 谈了这几点非常重要","time":"一小时前","url":"http://view.inews.qq.com/a/NEW2016110202728102"},{"img":"img/n2.jpg","title":"丁俊晖接班人浮出水面 18岁少年绝杀世界冠军","time":"一小时前","url":"http://m.news.baidu.com/news?bd_page_type=1#/detail/7287311734101904386?_k=jta5xb"},{"img":"img/n3.jpg","title":"险资持仓布局真相 减持银行围猎消费基建（名单）","time":"一小时前","url":"http://m.news.baidu.com/news?bd_page_type=1#/detail/4883285209224492577?_k=qqjkjq"},{"img":"img/n4.jpg","title":"多头环境强度由赚钱效应大小决定","time":"一小时前","url":"http://m.news.baidu.com/news?bd_page_type=1#/detail/7787085099271015766?_k=7whde5"},{"img":"img/n5.jpg","title":"10月份中国制造业和非制造业采购经理指数双升","time":"一小时前","url":"http://m.news.baidu.com/news?bd_page_type=1#/detail/8039434671559804273?_k=8rbkw1"}]}';
		wrapNews(JSON.parse(data));
		return;
	}
	if(type == 3){//周边
		var div = '<div id="main""><div id="fis_elm_pager__qk_7"><div class="styleguide index-widget-catcaption container"><div class="caption"><div class="row"><div class="-col-auto title -ft-large">生活服务</div><div><div class="spliter"></div></div></div></div></div></div><div id="fis_elm_pager__qk_8"><div class="styleguide index-widget-grid container grid-container-1 "><div class="row grid"><a class="thumbnail -col3 item item-nopadding"><div class="thumb-img nearby-icon ui3-canyin"style="background-image: url(&quot;//webmap1.bdimg.com/mobile/simple/static/index/images/index-nb-round-2x_f82aa9f.png&quot;);"></div><div class="caption -ft-base grid-caption">找美食</div></a><a class="thumbnail -col3 item item-nopadding"><div id="index-hoteltip-widget"></div><div class="thumb-img nearby-icon ui3-hotel"style="background-image: url(&quot;//webmap1.bdimg.com/mobile/simple/static/index/images/index-nb-round-2x_f82aa9f.png&quot;);"></div><div class="caption -ft-base grid-caption">订酒店</div></a><a class="thumbnail -col3 item item-nopadding"><div class="thumb-img nearby-icon ui3-sale"style="background-image: url(&quot;//webmap1.bdimg.com/mobile/simple/static/index/images/index-nb-round-2x_f82aa9f.png&quot;);"></div><div class="caption -ft-base grid-caption">团购</div></a><a class="thumbnail -col3 item item-nopadding"><div class="thumb-img nearby-icon ui3-caterchart"style="background-image: url(&quot;//webmap1.bdimg.com/mobile/simple/static/index/images/index-nb-round-2x_f82aa9f.png&quot;);"></div><div class="caption -ft-base grid-caption">排行榜</div></a></div><div class="row grid"><a class="thumbnail -col3 item item-nopadding"><div class="thumb-img nearby-icon ui3-jingdian"style="background-image: url(&quot;//webmap1.bdimg.com/mobile/simple/static/index/images/index-nb-round-2x_f82aa9f.png&quot;);"></div><div class="caption -ft-base grid-caption">查景点</div></a><a class="thumbnail -col3 item item-nopadding"><div class="thumb-img nearby-icon ui3-takeout"style="background-image: url(&quot;//webmap1.bdimg.com/mobile/simple/static/index/images/index-nb-round-2x_f82aa9f.png&quot;);"></div><div class="caption -ft-base grid-caption">外卖</div></a><a class="thumbnail -col3 item item-nopadding"><div id="index-movietip-widget"></div><div class="thumb-img nearby-icon ui3-movie"style="background-image: url(&quot;//webmap1.bdimg.com/mobile/simple/static/index/images/index-nb-round-2x_f82aa9f.png&quot;);"></div><div class="caption -ft-base grid-caption">看电影</div></a><a class="thumbnail -col3 item item-nopadding"><div class="thumb-img nearby-icon ui3-more"style="background-image: url(&quot;//webmap1.bdimg.com/mobile/simple/static/index/images/index-nb-round-2x_f82aa9f.png&quot;);"></div><div class="caption -ft-base grid-caption">更多</div></a></div></div></div><div id="fis_elm_pager__qk_9"><div class="styleguide index-widget-catcaption container -compacted"><div class="caption"><div class="row"><div class="-col-auto title -ft-large">出行服务</div><div><div class="spliter"></div></div></div></div></div></div><div id="fis_elm_pager__qk_10"><div class="styleguide index-widget-grid container -compacted grid-container-2 grid-no-margin"><div class="row grid"><a class="thumbnail -col3 item item-nopadding"><div class="thumb-img nearby-icon ui3-subway"style="background-image: url(&quot;//webmap1.bdimg.com/mobile/simple/static/index/images/index-nb-round-2x_f82aa9f.png&quot;);"></div><div class="caption -ft-base grid-caption">地铁图</div></a><a class="thumbnail -col3 item item-nopadding"><div class="thumb-img nearby-icon ui3-bus"style="background-image: url(&quot;//webmap1.bdimg.com/mobile/simple/static/index/images/index-nb-round-2x_f82aa9f.png&quot;);"></div><div class="caption -ft-base grid-caption">公交站</div></a><a class="thumbnail -col3 item item-nopadding"><div class="thumb-img nearby-icon ui3-taxi"style="background-image: url(&quot;//webmap1.bdimg.com/mobile/simple/static/index/images/index-nb-round-2x_f82aa9f.png&quot;);"></div><div class="caption -ft-base grid-caption">打车</div></a><a class="thumbnail -col3 item item-nopadding"><div class="thumb-img nearby-icon ui3-travelmore"style="background-image: url(&quot;//webmap1.bdimg.com/mobile/simple/static/index/images/index-nb-round-2x_f82aa9f.png&quot;);"></div><div class="caption -ft-base grid-caption">更多</div></a></div></div></div></div>';
		$('#videoList').empty();
		$('#videoList').append(div);
		
		$('html,body').animate({scrollTop: '0px'}, 300);
				var container = $('.top');
				$(container).scrollTop(
					0
				); 
		return;
	}
//	var url_cgi = base_url+ 'test.cgi?type='+type+'&username='+name;
	if(type == 0)
	{
		var url_cgi = 'http://'+document.location.hostname+':8888/video';
		$.get(url_cgi, function(data) {
			var obj=JSON.parse(data);
			wrapData(obj);
		});
		return;
	}

	if(type==1)
	{
		var url_cgi = 'http://'+document.location.hostname+':8888/live';
		$.get(url_cgi, function(data) {
			var obj=JSON.parse(data);
			wrapData(obj);
		});
		return;		
	}
	
//	var data;
//  if (type == 0) {
//		data = '{"code":"1000","data":{"powers":0,"type":0,"movieList":[{"imagePath":"BornInChina.jpg","name":"BornInChina","vedioPath":"BornInChina"},{"imagePath":"十二公民.jpg","name":"十二公民","vedioPath":"十二公民"},{"imagePath":"无人区.jpg","name":"无人区","vedioPath":"无人区"}]},"msg":"ok"}';
//	} else {
//		data = '{"code":"1000","data":{"powers":0,"type":0,"movieList":[{"imagePath":"cctv-2.png","name":"cctv-2","vedioPath":"3"},{"imagePath":"cctv-7.png","name":"cctv-7","vedioPath":"4"}]},"msg":"ok"}';
//	}
//	wrapData(JSON.parse(data));
}

function wrapNews(data){
	$('#videoList').empty();
	$.each(data.data, function(index, data) {
		var imgUrl = base_url + "/image/" + data.img;
		var div = '<div class="index-list-item" style="display: block;" data-url="'+data.url+'"><a href="'+data.url+'"><div class="index-list-main showleft"><div class="index-list-image"><i class="ivideoplay"></i><img src="'+imgUrl+'"data-save="true"class=""></div><div class="index-list-main-text"><div class="index-list-main-title">'+data.title+'</div><div class="index-list-bottom"><div class="index-list-main-time"><b class="tip-time">'+data.time+'</b><b class="tip-hot tip-fillred">热点</b></div></div></div></div></div>';
		
		$('#videoList').append(div);
		
		$('.index-list-item').off('click');
		$('.index-list-item').click(function() {
//			window.open($(this).attr('data-url')); 
//window.location.href=data.url;
		});
	});
	
	$('html,body').animate({scrollTop: '0px'}, 300);
			var container = $('.top');
			$(container).scrollTop(
				0
			); 
}
var player;
function wrapData(data) {
	if(data.msg != 'ok') return;
	$('#videoList').empty();
	data = data.data;
	$.each(data.movieList, function(index, val) {
		var img = document.createElement("img");
		img.setAttribute("src", base_url + "/" + val.imagePath);
		var span = document.createElement("span");
		span.innerText = val.name;
		var video_title = document.createElement("div");
		video_title.setAttribute("class", "video-title");
		video_title.appendChild(span);

		var video_innerIcon = document.createElement("div");
		video_innerIcon.setAttribute("class", "wa-play-icon");
		var video_innerBox = document.createElement("div");
		video_innerBox.setAttribute("class", "video-innerbox");
		video_innerBox.appendChild(video_innerIcon);

		var video_playBox = document.createElement("div");
		video_playBox.setAttribute("class", "video-playbox");

		var video_img = document.createElement("div");
		video_img.setAttribute("class", "video-img");
		video_img.setAttribute("data-id", "data-id");
		video_img.setAttribute("data-name", val.name);
		video_img.setAttribute("data-type", data.type);
		video_img.setAttribute("data-path",val.vedioPath);
		video_img.appendChild(img);
		video_img.appendChild(video_title);
		if(data.powers == 1)
		video_img.appendChild(video_innerBox);
		video_img.appendChild(video_playBox);

		var video_item = document.createElement("div");
		video_item.setAttribute("class", "video-item");
		video_item.appendChild(video_img);

		var li = document.createElement("li");
		li.appendChild(video_item);
		$('#videoList').append(li);
		$('.video-img').off('click');
		$('.video-img').click(function() {
			if(data.powers == 0){
				var a=confirm("请充值！");
				if(a == true){
					showPay();
				}
				return;
			}
			if((data.powers == -1 && data.type == 0)){
				alert("当前外网不可用！");
				return;
			}
			if((data.powers == 1) || (data.powers == -1 && data.type == 1)){
				if ($(this).find('video').length > 0) return;
				if( typeof(player)!="undefined"){
					player.dispose();
				}
	    			$('.video-playbox').empty();
//	    			var source = document.createElement("source");
//	    			source.setAttribute("type", "application/x-mpegURL");
//	    			source.setAttribute("src", getVideoPath($(this).attr("data-type"), $(this).attr("data-path")));
	    			var video = document.createElement("video");
	    			video.setAttribute("id", "player");
	    			video.setAttribute("class","video-js vjs-default-skin");
	    			video.setAttribute("width", "750");
	    			video.setAttribute("height", "220");
	    			video.setAttribute("controls", "controls");
	    			//			video.setAttribute("autoplay", "autoplay");
	    			video.setAttribute("currentTime",0);
//	    			video.appendChild(source);
	    			$(this).find('.video-playbox').append(video);
	    			var path = getVideoPath($(this).attr("data-type"), $(this).attr("data-path"));
//	    			$('video').currentTime=0;
//					player = new MediaElementPlayer('#player',{enableAutosize: true});
//					setTimeout(function () {      
// 						player.play();
//					}, 300);
				
					 player = videojs('#player', { /* Options */ }, function() {
						console.log('Good to go!');
//						this.pause();
					  	this.play(); // if you don't trust autoplay for some reason
					  	// How about an event listener?
					  	this.on('ended', function() {
					    console.log('awww...over so soon?');
					});
					
					if(data.type == 0)
					{
						player.src({
							src: path,
							type: 'video/mp4',
							withCredentials: true
						});
						console.log("------------------video/mp4");			
					}else if(data.type==1)
					{
							player.src({
							src: path,
							type: 'application/x-mpegURL',
							withCredentials: true
							});	
						console.log("------------------application/x-mpegURL");					
					}
				});
			}
		});
	});
	$('html,body').animate({scrollTop: '0px'}, 300);
			var container = $('.top');
			$(container).scrollTop(
				0
			); 
}
var hasPlayed = false;
function getVideoPath(type, path) {
	if (type == 0) {
		//return base_url + "/movie/" + path + "/out.m3u8?t="+Date.parse(new Date());
		return base_url + "/" + path;
	} else {
		//return base_url + "/live/" + path + "/mystream.m3u8?t="+Date.parse(new Date());
		console.log(base_url);	
		return base_url+"/"+path+"?t="+Date.parse(new Date());
	
	}
	
}
