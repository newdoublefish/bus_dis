ls /dev/ttyACM*
if [ "$?" != "0" ] 
then
	echo "4g card doesnot insert!!!"
	ifup wlp1s0
	sh /home/developer/workspace/route/ipforward.sh&
else
	echo "4g card is insert!!!"
	sh /home/developer/workspace/4g/4g.sh&
	sh /home/developer/workspace/route/ipforwardPpp.sh
fi
