rm /home/developer/workspace/hls/*
ffmpeg -re -i "udp://@:6000?overrun_nonfatal=1&fifo_size=531914" \
-map 0:p:1 -hls_flags omit_endlist -c copy -hls_time 5 -hls_list_size 10 -hls_wrap 8 /home/developer/workspace/hls/1_channel.m3u8 \
-map 0:p:2 -hls_flags omit_endlist -c copy -hls_time 5 -hls_list_size 10 -hls_wrap 8 /home/developer/workspace/hls/2_channel.m3u8 \
-map 0:p:3 -hls_flags omit_endlist -c copy -hls_time 5 -hls_list_size 10 -hls_wrap 8 /home/developer/workspace/hls/3_channel.m3u8 \
-map 0:p:4 -hls_flags omit_endlist -c copy -hls_time 5 -hls_list_size 10 -hls_wrap 8 /home/developer/workspace/hls/4_channel.m3u8
