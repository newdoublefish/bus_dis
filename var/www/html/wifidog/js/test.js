var player;
function fillData()
{
	//var url_cgi = 'http://192.168.10.105/demo.cgi';
	var base_url = 'http://'+document.location.hostname+':8888';
	console.log(base_url);
	$.get(base_url,function(data1){
			
			console.log("11111111111111111");
			
			//console.log(data1);	
			var obj=JSON.parse(data1);
			wrapData(obj);
			console.log("22222222222222222");
	});	
}

function wrapData(data) {

	console.log(data);
	if(data.msg != 'ok') return;
	$('#videoList').empty();
	data = data.data;
	console.log('-----2!');
	$.each(data.movieList, function(index, val) {
		var img = document.createElement("img");
		img.setAttribute("src", base_url + "/" + val.imagePath);
		console.log(base_url + "/movie/" + val.imagePath);
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
			console.log("onClick 1");
			if((data.powers == 1) || (data.powers == -1 && data.type == 1)){
				if ($(this).find('video').length > 0) return;
				if( typeof(player)!="undefined"){
					player.dispose();
				}
				console.log("onClick 2");
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
					player.src({
						src: path,
						type: 'video/mp4',
						withCredentials: true
					});
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
	return base_url + "/" + path;

	
}