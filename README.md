# linphone-sdk-patch
修改linphone-sdk 4.4.2，增加一个虚拟camera，用于通话时，把rtsp和rtmp做为视频源。

编译

把这个补丁覆盖下载的代码树，然后就可以编译了。
mkdir build
cmake .. -DLINPHONESDK_PLATFORM=Android -DENABLE_GPL_THIRD_PARTIES=YES -DENABLE_NON_FREE_CODECS=YES -DENABLE_VIDEO=YES -DENABLE_FFMPEG=YES -DENABLE_VPX=NO
make


如何使用

1、选择摄像头 Live Stream(YUV)

2、呼叫时，设置setCameraParam为rtsp或者rtmp地址：

CallParams params = mLc.createCallParams(null);

......
params.setCameraParam("rtsp://ip ..."); 
......

mLc.inviteAddressWithParams(address, params);


linphoone-sdk下载：

git clone -b 4.4.2 https://gitlab.linphone.org/BC/public/linphone-sdk.git --recursive
