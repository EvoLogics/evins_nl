#!/usr/bin/env python
import rospy
from evins_nl.msg import NLData, NLDataReport, NLProtocol, NLCommand

def listener(data):
    if data.command.status != NLCommand.UNDEFINED:
        rospy.loginfo("Answer\n%s", data)

def set_protocol():
    pub = rospy.Publisher("/evins_nl_2/protocol", NLProtocol, tcp_nodelay=True, queue_size=2)
    sub = rospy.Subscriber("/evins_nl_2/protocol", NLProtocol, listener, queue_size=1)
    msg = NLProtocol()
    msg.protocol = "polling"
    msg.command.id = NLCommand.SET
    msg.command.status = NLCommand.UNDEFINED
    msg.header.stamp = rospy.Time.now()
    rospy.loginfo(msg)
    pub.publish(msg)

def talker():
    pub = rospy.Publisher("/evins_nl_2/data", NLData, tcp_nodelay=True, queue_size=1)
    rospy.loginfo(pub)
    
    msg = NLData()
    msg.datatype = NLData.SENSITIVE
    msg.command.id = NLCommand.SEND
    msg.command.status = NLCommand.UNDEFINED
    msg.command.subject = NLCommand.DATA
    msg.data = "test qos data"
    msg.route.destination = 1

    while not rospy.is_shutdown():
        msg.header.stamp = rospy.Time.now()
        rospy.loginfo(msg)
        pub.publish(msg)
        rate.sleep()

if __name__ == '__main__':
    try:
        rospy.init_node('dummy_qos')
        rate = rospy.Rate(0.1)
        rate.sleep()
        set_protocol()
        talker()
    except rospy.ROSInterruptException:
        pass
