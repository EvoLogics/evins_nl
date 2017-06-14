#!/usr/bin/env python
# sendim script
import rospy
from evins_nl.msg import NLProtocolInfo, NLCommand

def listener(data):
    if data.command.status != NLCommand.UNDEFINED:
        rospy.loginfo("Answer\n%s", data)

if __name__ == '__main__':
    try:
        rospy.init_node('sim')
        pub = rospy.Publisher("/evins_nl_1/protocolinfo", NLProtocolInfo, tcp_nodelay=True, queue_size=2)
        rospy.loginfo(pub)
        rate = rospy.Rate(1)
        sub = rospy.Subscriber("/evins_nl_1/protocolinfo", NLProtocolInfo, listener, queue_size=1)
        
        msg = NLProtocolInfo()
        msg.command.id = NLCommand.GET
        msg.protocol = "sncfloodr"
        
        msg.command.status = NLCommand.UNDEFINED
        msg.header.stamp = rospy.Time.now()

        rate.sleep()
        rospy.loginfo(msg)
        pub.publish(msg)
        rate.sleep()
        rate.sleep()

    except rospy.ROSInterruptException:
        pass
